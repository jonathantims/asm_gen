# `asm_funcs.h` interface design
## Overview

The interface is for writing assembly directly, with more readability and with static checks. Helpers are added as experience demands. A new check belongs next to the helper that needs it, not in a second language.

Two examples of that style:

- `write_rep_movsb` takes a destination, a source, and a count even though the mnemonic `rep movsb` has no operands. The CPU's operands are implicit (`rdi`, `rsi`, `rcx`). The wrapper checks that the caller placed the values in those registers, and aborts generation if not. Naming the registers is what makes the call readable; the check is what makes the names mean something.
- `mov` is one mnemonic with three checks. `write_init_mov` is the first store into an untracked location and records its tag. `write_set_mov` updates a location whose tag must already match. `write_replace_mov` repurposes a register and overwrites its tag. The three names are the semantics the mnemonic does not have.

The repository's purpose, and the examples built on this emitter, are described in [`README.md`](../README.md).

`asm_funcs.h` is a C interface for generating Windows x64 MASM (`ml64`) assembly text. It is not an assembler and it does not execute instructions. Instead, the generator calls many small `write_*` functions that append MASM tokens to a global output buffer. The generator then writes that buffer to `output.asm`.

The important design feature is that most meaningful operands are passed as typed value cells:

```c
typedef struct {
    const char* name;    /* where the value lives */
    const char* type;    /* real logical tag */
    const char* use_as;  /* optional tag used at call boundary */
    int         is_register; /* set when write_extern / write_proc copy the parameter list */
} AsmDataField;
```

Those tags are checked by generation-time dictionaries. A mismatched parameter, an accidental reuse of a register under the wrong meaning, or an attempt to write a global through the wrong tag aborts the generator before bad assembly is emitted.

`core/asm_main.c` is the canonical example. It generates a program with:
- a 1 MB memory-mapped circular log file,
- a private `log_write` routine,
- a Win32 window procedure,
- a `main` procedure that maps the file, logs messages, creates a window, runs
a message loop, and exits.

Register spilling is a frame utility in this header, not a second allocator.
`asm_frame_prologue`, `asm_frame_slot`, and `asm_frame_epilogue` open a packed
slot area, hand out sized operands, and restore the preserved registers. The C
value layer calls them. A language wrapper can call them directly. Section 13
describes the frame.

## Table of contents
1. [Output model](#1-output-model)
2. [`AsmDataField`: the value cell](#2-asmdatafield-the-value-cell)
3. [`AsmCall`: callable descriptors](#3-asmcall-callable-descriptors)
4. [Type-validation dictionaries](#4-type-validation-dictionaries)
5. [Move semantics](#5-move-semantics)
6. [Procedures](#6-procedures)
7. [Calls](#7-calls)
8. [Instruction writers](#8-instruction-writers)
9. [Data sections and struct layouts](#9-data-sections-and-struct-layouts)
10. [Worked example from `core/asm_main.c`](#10-worked-example-from-coreasm_mainc)
11. [Conventions](#11-conventions)
12. [Building](#12-building)
13. [Spill frames](#13-spill-frames)
14. [What the build checks](#14-what-the-build-checks)
## 1. Output model
The generator appends token strings to a global array:
```c
#define ASM_OUTPUT_MAX 1048576
static const char* asm_output[ASM_OUTPUT_MAX];
static int asm_output_cursor = 0;
static inline void write(const char* token) {
    asm_output[asm_output_cursor++] = token;
}
```
`write()` stores pointers; it does not copy token text. For fixed MASM tokens this is trivial because `asm.h` supplies static string constants. For computed tokens, the header uses static token tables or the bump arena so emitted pointers remain valid.

The bump arena is 64 KiB (`ASM_BUMP_ARENA_SIZE`) and is never freed. A generation is a short-lived process, and almost every allocation is a string that must live until exit, so freeing individual strings would not make the run faster or smaller in any way that matters. `asm_bump_alloc` aborts if the arena is exhausted rather than growing.

Generation state — the output buffer, the type dictionaries, the bump arena, and the indent counter — is process-global, and the generator is single-threaded. Concurrent generation means separate processes. Threading that state later is ordinary work.
Line helpers such as `write_line`, `write_line2`, `write_line3`, `write_line4`, and `write_line_def` insert indentation and MASM punctuation. They all consult:
```c
static int asm_indent_level = 0;
```
`write_proc` increments the indentation level, `write_endp` decrements it, and `write_label` temporarily dedents to place labels at the enclosing level. Section writers do not change the indentation level; `core/asm_main.c` indents the C generator code cosmetically to mirror the emitted assembly.
The generator flushes the generated tokens at the end:

```c
return write_asm_file("output.asm");
```

`write_asm_file` writes every token in the output buffer to that file.
The first call in `core/asm_main.c` is:
```c
write_set_case_sensitive_identifiers();
```
This emits:
```asm
option casemap:none
```
and is required for mixed-case imported names such as `ExitProcess`, `CreateFileA`, and `DefWindowProcA`.
## 2. `AsmDataField`: the value cell
`AsmDataField` is the common carrier for a value plus its generation-time type tag:
```c
typedef struct {
    const char* name;
    const char* type;
    const char* use_as;
    int         is_register;
} AsmDataField;
```
- `name` is the operand/location string: `"rax"`, `"rcx"`, `"logPos"`,
`"qword ptr wcInstance"`, `"qword ptr [rsp+40]"`, or an immediate such as `"0"`.
- `type` is the real logical tag for the value: `"hFile"`, `"len"`, `"src"`,
`"lpWndClass"`, etc.
- `use_as` is normally `NULL`.  Only `treat_as` sets it to say "this value is
really tag X, but at this call boundary it is intentionally used as tag Y."
- `is_register` is set when `write_extern` or `write_proc` copies the parameter
list into the bump arena. A three-field initializer leaves it 0; the copy
classifies a name with no `'['` and no space as a register. `asm_extern_reg_count`
counts the leading run of cells with the flag set. The rest are stack parameters.
The helper:
```c
static inline const char* asm_field_use_as(AsmDataField f) {
    return f.use_as ? f.use_as : f.type;
}
```
is used by call validation. Dictionary integrity checks use the real `type`, not `use_as`.
### Literals
Bare immediates should be constructed with `literal(value, tag)`:
```c
write_extern_call(&exit_process, literal("123", "exit_code"));
```
This returns an untracked `AsmDataField` with `name = "123"` and `type = "exit_code"`. It still participates in call-parameter tag checks, but because the immediate has no dictionary entry, `assert_type` passes silently.
### Void sentinel
Procedures and externs with no return value use:
```c
static const AsmDataField asm_void = {0};
```
For example:
```c
AsmExtern post_quit =
    write_common_extern("PostQuitMessage", post_quit_params, 1, asm_void);
```
## 3. `AsmCall`: callable descriptors
`AsmCall` is the callable descriptor. `AsmExtern` is a typedef of the same struct.
```c
typedef struct {
    const char* name;
    const AsmDataField* params;
    int param_count;
    AsmDataField return_reg;
} AsmCall;
typedef AsmCall AsmExtern;
```
- `name` is the target passed to `call`.
- `params[i]` is a cell naming where parameter `i` must be placed and what tag
it must have.
- `param_count` is the number of parameters.
- `return_reg` is the register/tag cell where the result appears, or
`asm_void`.
For Windows x64 integer calls, the leading parameters normally use:
```c
{"rcx", "arg0"}, {"rdx", "arg1"}, {"r8", "arg2"}, {"r9", "arg3"}
```
and parameters beyond the register set use stack slots such as:
```c
{"qword ptr [rsp+40]", "dwCreationDisposition"},
{"qword ptr [rsp+48]", "dwFlagsAndAttributes"}
```
`asm_extern_reg_count(ext)` counts the leading run of parameter cells whose `is_register` flag is set. `write_extern` and `write_proc` set that flag when they copy the list. The remainder are stack parameters.
Example from `core/asm_main.c`:
```c
static const AsmDataField create_file_params[] = {
    {"rcx", "lpFileName"}, {"rdx", "dwDesiredAccess"},
    {"r8", "dwShareMode"}, {"r9", "lpSecurityAttributes"},
    {"qword ptr [rsp+40]", "dwCreationDisposition"},
    {"qword ptr [rsp+48]", "dwFlagsAndAttributes"},
    {"qword ptr [rsp+56]", "hTemplateFile"}
};
AsmExtern create_file =
    write_common_extern("CreateFileA", create_file_params, 7,
                        (AsmDataField){"rax", "hFile"});
```
## 4. Type-validation dictionaries
The generator has two location-to-tag dictionaries, both linear arrays. The local table holds 1024 entries (`ASM_TYPE_DICT_MAX`) and the global table holds 128 (`ASM_GLOBAL_DICT_MAX`). Lookup walks the live entries from index 0. The local cap covers every `write_replace_mov` destination in one procedure, including a fresh frame-slot string for each register a call saves.

That scan is the efficient structure at these sizes. A balanced tree gives log(N) comparisons, and log2(1024) is 10, but each comparison is a pointer chase to a separately allocated node, plus the cost of allocating the node on insert. A hash table gives an expected constant time and pays for the hash and for a bucket array that is mostly empty when the live count is a few hundred. Walking the live string pointers is shorter than either of those setups, and the generator spends its time writing the `.asm` text. Overflow aborts with the cap in the message. A larger unit raises the constant. The arrays stay. The same choice is stated next to the tables in `asm_funcs.h`.
```c
static AsmTypeEntry asm_type_dict[ASM_TYPE_DICT_MAX];
static int asm_type_dict_count = 0;
static AsmTypeEntry asm_global_dict[ASM_GLOBAL_DICT_MAX];
static int asm_global_dict_count = 0;
```
They emit no assembly. They exist only to validate generation-time intent.
### Local dictionary: `asm_type_dict`
The local dictionary tracks per-procedure state:
- parameter registers and stack slots,
- scratch registers,
- return registers after calls,
- any location established by `write_init_mov` or `write_replace_mov`.
`write_proc` clears the local dictionary and seeds it with procedure parameters:
```c
asm_type_dict_clear();
for (int i = 0; i < param_count; i++) {
    asm_type_dict_set(params[i].name, params[i].type);
}
```
`write_endp` clears it again because the procedure's register meanings have gone out of scope.
### Global dictionary: `asm_global_dict`
The global dictionary tracks data globals, memory-layout field references, and named constants. It is never cleared. Entries are write-once: redeclaring a location with the same tag is harmless, but redeclaring it with a different tag aborts.
Global entries are added by:
- `write_unitialised_data`,
- `write_initialised_data`,
- `write_data_memory_layout`,
- `write_constant`.
`write_equ` by itself only emits an `equ`; it does not return an `AsmDataField` or register a tag. Use `write_constant` when later typed use is expected.
### Lookup and assertions
`asm_type_lookup(location)` checks the local dictionary first and then the global dictionary:
```c
const char* t = asm_type_dict_get(location);
if (t != NULL) return t;
return asm_global_dict_get(location);
```
This lets local register meanings shadow global names if that ever matters.
`assert_type(loc)` checks that the real tag in `loc.type` matches the tag currently recorded for `loc.name`. If the location is untracked, it passes silently:
```c
assert_type(create_mapping.return_reg);
```
In `core/asm_main.c`, return registers are often asserted immediately after Win32 calls:
```c
write_extern_call(&map_view, hMap, literal("0F001Fh", "dwAccess"),
                  literal("0", "offHigh"), literal("0", "offLow"),
                  literal("0", "numBytes"));
AsmDataField rax_pView = map_view.return_reg;
assert_type(rax_pView);
write_set_mov(pView, rax_pView);
```
## 5. Move semantics
The header intentionally separates raw `mov` emission from semantic moves.
### Raw `write_mov`
```c
write_mov(dest, src, src_type);
```
emits:
```asm
mov dest, src
```
The `src_type` is documentary only. This function does not update the dictionaries.
### `write_init_mov(loc, value, tag)`
`write_init_mov` is the first store into an untracked local location:
```c
AsmDataField tmp = write_init_mov("rbx", rax_value.name, "count");
```
It:
1. requires `loc` to be absent from both local and global dictionaries,
2. emits `mov loc, value`,
3. records `asm_type_dict[loc] = tag`,
4. returns `{ loc, tag }`.
It is an error to use it for a data global, because globals are already declared and typed.
### `write_set_mov(dest, src)`
`write_set_mov` updates a location while preserving its meaning:
```c
write_set_mov(logPos, zero_head);
write_set_mov(hFile, create_file.return_reg);
```
It:
1. requires `dest.name` to be tracked,
2. requires the tracked tag to equal `dest.type`,
3. validates `src` with `assert_type(src)`,
4. emits `mov dest.name, src.name`,
5. does not change the dictionary tag.
Use it when the destination is still conceptually the same value after the move.
### `write_replace_mov(dest, src)`
`write_replace_mov` repurposes a register or local location:
```c
AsmDataField rsi_src = write_replace_mov("rsi", rcx_src);
AsmDataField r8_len  = write_replace_mov("r8",  rdx_len);
```
It:
1. forbids `dest` if it is a data global,
2. emits `mov dest, src.name`,
3. overwrites the local dictionary entry with `src.type`,
4. returns `{ dest, src.type }`.
This is the normal way to name a scratch register after loading it.
### `change_tag(cell, type)`
`change_tag` relabels a location in place without emitting assembly:
```c
write_add(rdi_pView, r9_head);
AsmDataField rdi_dst = change_tag(rdi_pView, "dst");
```
If the location is tracked locally, the local dictionary is updated to the new tag. Globals and untracked immediates are left alone. Use this after an in-place operation changes the logical meaning of a register.
### `treat_as(x, existing_tag, as_tag)`
`treat_as` is the checked bridge for deliberate cross-tag use:
```c
treat_as(LOG_SIZE, "size", "dwMaxLow")
```
It:
1. verifies `x.type == existing_tag`,
2. calls `assert_type(x)` to ensure dictionaries agree with the real tag,
3. returns `{ x.name, x.type, as_tag }`.
The returned cell still has real type `x.type`; only call-argument validation uses `use_as`.
### `assert_type(loc)`
`assert_type` is a dictionary-integrity check. It emits no assembly and uses `loc.type`, never `loc.use_as`. If `loc.name` is untracked, there is no recorded state to contradict, so the check passes.
## 6. Procedures
`write_proc` begins a procedure and returns a descriptor:
```c
AsmDataField rcx_src = {"rcx", "src"};
AsmDataField rdx_len = {"rdx", "len"};
const AsmDataField log_params[] = {rcx_src, rdx_len};
AsmExtern log_proc = write_proc("log_write", log_params, 2, asm_void);
```
It emits:
```asm
log_write proc
```
then increments indentation, clears the local dictionary, seeds parameter locations, and stores the current return register for `write_ret`.
`write_endp(&proc)` closes it:
```c
write_endp(&log_proc);
```
It decrements indentation, emits:
```asm
log_write endp
```
and clears local type state.
`write_proc` does not opt into a preserved-register mask. A body written with it is not failed for writing a callee-saved register or for calling a Windows function. `c_begin_function` is what opts in; see [`c_architecture.md`](../c/c_architecture.md).
`write_ret()` first asserts the current procedure's declared return register:
```c
static inline void write_ret(void) {
    assert_type(asm_current_return_reg);
    write_line(ASM_RET);
}
```
For void procedures (`asm_void`), the assertion is a no-op. For non-void procedures, this catches returning with a wrongly tagged register.
## 7. Calls
All typed calls are driven by `AsmExtern`.
### Shared core: `write_callv`
`write_callv` is the common implementation used by `write_call` and `write_extern_call`. For each argument it:
1. reads one `AsmDataField`,
2. computes the effective parameter tag with `asm_field_use_as`,
3. compares that tag to `ext->params[i].type`,
4. calls `assert_type` on the real argument,
5. emits register-argument `mov` instructions,
6. emits a direct `call ext->name` for local descriptors, or
   `call qword ptr [__imp_Name]` for imported descriptors,
7. records `ext->return_reg` in the local dictionary if the call returns a
value.
For example, this private call:
```c
write_call(&log_proc, offsetMsgStarted, msgStartedLen);
```
loads `rcx` and `rdx` according to `log_proc.params`, validates `"src"` and `"len"`, and emits `call log_write`.
### Private calls: `write_call`
`write_call(&proc, ...)` performs only logical argument marshalling and the `call`. It does not reserve shadow space, does not align `rsp`, and does not place stack arguments. It is intended for private generator-defined routines such as `log_write`.
### External/Windows calls: `write_extern_call`
`write_extern_call(&ext, ...)` wraps `write_callv` with Windows x64 ABI scaffolding. It:
1. snapshots arguments,
2. computes how many are register vs stack parameters,
3. reserves 32 bytes of shadow space plus stack-argument space,
4. adds an 8-byte pad if needed so `rsp` is 16-byte aligned at `call`,
5. writes 5th+ arguments to declared stack slots using `r11` as a scratch,
6. delegates register moves and an IAT-indirect
   `call qword ptr [__imp_Name]` to the shared call core,
7. restores `rsp`.
The function cross-checks each stack parameter declaration against the expected slot (`"qword ptr [rsp+40]"`, `"qword ptr [rsp+48]"`, ...). This catches a bad external descriptor before assembly is generated.

By default, imported descriptors emit Visual C-style calls through the import
address table:
```asm
extern __imp_ExitProcess:qword
call qword ptr [__imp_ExitProcess]
```
The import library resolves `__imp_ExitProcess` to the IAT slot, avoiding a
linker-generated `ExitProcess` trampoline. For a linker that does not support
Visual C import symbols, compile the generator with:
```text
/DASM_USE_VISUAL_C_DLL_CALLS=0
```
or define `ASM_USE_VISUAL_C_DLL_CALLS` as `0` before including `asm_funcs.h`.
The fallback emits `extern ExitProcess:proc` and `call ExitProcess`. The setting
applies only to descriptors introduced by `write_extern` or
`write_common_extern`; calls to procedures returned by `write_proc` remain
direct local calls in either mode.
Example:
```c
write_extern_call(&create_window,
    literal("0",          "dwExStyle"),
    offsetClassName,
    offsetWindowTitle,
    literal("10CF0000h",  "dwStyle"),
    literal("80000000h",  "x"),
    literal("80000000h",  "y"),
    literal("80000000h",  "nWidth"),
    literal("80000000h",  "nHeight"),
    literal("0",          "hWndParent"),
    literal("0",          "hMenu"),
    wcInstance,
    literal("0",          "lpParam"));
```
### Declarations: `write_common_extern`
`write_common_extern` emits:
```asm
extern __imp_Name:qword
```
This matches Visual C's import-library convention. `write_extern_call` calls
through the imported address directly, so the linker does not need to synthesize
a procedure-symbol trampoline. With `ASM_USE_VISUAL_C_DLL_CALLS=0`, the same
function emits `extern Name:proc` instead. It returns an `AsmExtern` by value:
```c
static const AsmDataField exit_params[] = {{"rcx", "exit_code"}};
AsmExtern exit_process =
    write_common_extern("ExitProcess", exit_params, 1, asm_void);
```
`write_common_extern` copies the parameter list into the bump arena (via `write_extern`) and sets `is_register` on that copy, so the caller's array does not have to outlive the descriptor.
## 8. Instruction writers
The header contains many mechanically generated wrappers for MASM registers, instructions, directives, and keywords. Most of them paste a mnemonic. Which of them the build runs, and which it does not, is in section 14.
### Register-name writers
Register writers append a raw register token without a newline:
```c
write_rax();
write_eax();
write_ax();
write_rcx();
write_r8();
write_r9();
```
They are low-level token emitters. Most generator code instead passes register names as strings in `AsmDataField` cells because typed instruction and call helpers need the tag.
### Data movement
Data movement includes:
- `write_mov(dest, src, src_type)`: raw `mov`,
- `write_set_mov(dest, src)`: update an existing typed location,
- `write_replace_mov(dest, src)`: repurpose a register/location,
- `write_init_mov(loc, value, tag)`: establish a first local value.
### Arithmetic and logic
Arithmetic/logic helpers such as:
```c
write_add(rdi_pView, r9_head);
write_sub(r10_size, r9_head);
write_cmp(r8_len, r10_space);
write_xor(wnd_proc.return_reg, wnd_proc.return_reg);
```
take `AsmDataField` operands when both operands have meaningful logical tags. These wrappers emit the instruction using `.name`. Their comments note that the tags are currently recorded mainly for readability; if the operation changes the destination's meaning, follow it with `change_tag`.
Some simple one-operand wrappers such as `write_inc`, `write_dec`, and some forms of `write_imul` take raw operand strings. Use the typed forms where the header provides them.
### String operations with mandated registers
`write_rep_movsb(dst, src, count)` emits:
```asm
rep movsb
```
The CPU instruction has implicit operands, so the wrapper verifies the caller placed values in the required registers:
- destination must be `rdi`,
- source must be `rsi`,
- count must be `rcx`.
If any location is wrong, generation aborts. In `log_write`:
```c
AsmDataField rsi_src = write_replace_mov("rsi", rcx_src);
AsmDataField rcx_space = write_replace_mov("rcx", r10_space);
write_rep_movsb(rdi_dst, rsi_src, rcx_space);
```
### Control flow
Raw jump helpers emit a single jump:
```c
write_jmp("mw_msg_loop");
write_jbe("lw_single");
write_jb("lw_store");
write_jne("wp_default");
write_jle("mw_msg_done");
```
Labels are emitted with:
```c
const char* loop = write_label("mw_msg_loop");
```
which writes `mw_msg_loop:` at the enclosing indentation level and returns the
name, so a backward branch reuses the handle rather than repeating the literal:
```c
    write_jmp(loop);
```
Folded compare-and-branch helpers keep the condition and operands together and
return the target label, so the matching `write_label` reuses the same handle:
```c
const char* size_ok =
    write_if_unsigned_less_than_or_equal_then_jump_to("lw_size_ok",
                                                      rdx_len, LOG_SIZE);
    write_set_mov(rdx_len, LOG_SIZE);
write_label(size_ok);
```
The four variants are:
```c
write_if_unsigned_less_than_or_equal_then_jump_to("lw_size_ok", rdx_len, LOG_SIZE);
write_if_unsigned_less_than_then_jump_to("lw_store", r9_head, LOG_SIZE);
write_if_not_equal_then_jump_to("mw_window_ok", rax_hWnd, literal("0", "hWnd"));
write_if_signed_less_than_or_equal_then_jump_to("mw_msg_done", eax_result, zero_result);
```
They emit `cmp dest, src` and the matching conditional jump (`jbe`, `jb`, `jne`, or `jle`).
## 9. Data sections and struct layouts
### Sections
The main section writers are:
```c
write_data();   /* .data  */
write_dataq();  /* .data? */
write_const();  /* .const */
write_code();   /* .code  */
```
`core/asm_main.c` uses `.data` for initialized strings and structured initializers, `.data?` for uninitialized handles and message storage, then `.code` for procedures.
### Scalars
`write_initialised_data(name, value, type)` emits a byte definition:
```c
AsmDataField logName =
    write_initialised_data("logName", "\"output.log\",0", "lpFileName");
```
which generates:
```asm
logName db "output.log",0
```
It returns `{ "logName", "lpFileName" }` and registers the global tag.
`write_unitialised_data(name, type)` emits a qword reservation:
```c
AsmDataField hFile = write_unitialised_data("hFile", "hFile");
```
which generates:
```asm
hFile dq ?
```
and registers `hFile` as a global of tag `"hFile"`.
### Constants
`write_constant(name, expr, type)` wraps `write_equ`, returns a typed cell, and registers the symbol globally:
```c
AsmDataField LOG_SIZE = write_constant("LOG_SIZE", "100000h", "size");
AsmDataField msgStartedLen =
    write_constant("msgStartedLen", "$ - msgStarted", "len");
```
Use `write_equ(dest, src)` directly only when no typed symbol is needed later.

`write_offset_constant(name, field, type)` is a specialised form for an
`offset` equate that references an existing data-field handle rather than
repeating its label:
```c
AsmDataField logName = write_initialised_data("logName", "\"log.txt\",0", "lpFileName");
AsmDataField offsetLogName =
    write_offset_constant("offset_logName", logName, "lpFileName");   /* -> offset_logName equ offset logName */
```
It emits the static token `offset ` followed by `field.name` (no combined
string is allocated, since both parts already persist) and registers `name`
globally under `type`. Prefer it over `write_constant("offset_logName",
"offset logName", ...)` so the label is never spelled twice. It requires a
bare-label handle; struct members resolved through a memory layout are sized
operands (`qword ptr wcInstance`), not raw labels, so use the struct's base
label for `offset` of the whole struct.
### Memory-layout struct system
The struct system describes data layouts with explicit field metadata:
```c
AsmMemoryLayout* layout = create_memory_layout(count, "TAG");
set_memory_layout_entry(index, name, value, tag, offset, size);
AsmMemoryLayoutRefs refs = write_data_memory_layout(layout);
```
Each field has:
- an optional label `name`,
- an initializer `value`,
- a logical `tag`,
- an explicit byte `offset`,
- a byte `size`.
`write_data_memory_layout` emits `db`, `dw`, `dd`, or `dq` according to field size. Explicit offsets allow aliases that overlap at the same offset, which supports union-like labels. Aliases are emitted with MASM `label`; the storage field that follows reserves the bytes. Unsupported sizes, gaps, and
partial mid-field overlaps abort.
The return value is:
```c
typedef struct {
    const AsmDataField* fields;
    int count;
} AsmMemoryLayoutRefs;
```
`fields[i]` is a ready sized memory operand cell such as:
```c
{"qword ptr wcInstance", "hInstance"}
```
and is registered in the global dictionary.
`core/asm_main.c` uses this for `WNDCLASSA`:
```c
AsmMemoryLayout* wndClass_layout = create_memory_layout(11, "WNDCLASSA");
set_memory_layout_entry(wndClassField++, "wndClass", "0",
                        "style", 0, 4);
set_memory_layout_entry(wndClassField++, "lpfnWndProc", "offset WndProc",
                        "lpfnWndProc", 8, 8);
set_memory_layout_entry(wndClassField++, "wcInstance", "0",
                        "hInstance", 24, 8);
AsmMemoryLayoutRefs wndClass = write_data_memory_layout(wndClass_layout);
AsmDataField wcInstance = wndClass.fields[wcInstanceIndex];
```
and later patches that field with the module handle:
```c
write_extern_call(&get_module, literal("0", "lpModuleName"));
AsmDataField rax_hInstance = get_module.return_reg;
write_set_mov(wcInstance, rax_hInstance);
```
The `MSG` buffer demonstrates a base alias:
```c
set_memory_layout_entry(msgField++, "msgBuf",  "?", "lpMsg", 0, 8);
set_memory_layout_entry(msgField++, "msgHwnd", "?", "hwnd",  0, 8);
```
Both names share offset 0; `msgBuf` can be used for `offset msgBuf`, while the real storage is also described field-by-field.
## 10. Worked example from `core/asm_main.c`
This slice shows the type system and code emission working together in `log_write`.
The procedure descriptor declares a private register convention:
```c
AsmDataField rcx_src = {"rcx", "src"};
AsmDataField rdx_len = {"rdx", "len"};
const AsmDataField log_params[] = {rcx_src, rdx_len};
AsmExtern log_proc = write_proc("log_write", log_params, 2, asm_void);
```
`write_proc` emits `log_write proc`, indents the body, and records:
```text
rcx -> src
rdx -> len
```
The first branch clamps `len` to the 1 MB ring size:
```c
const char* size_ok =
    write_if_unsigned_less_than_or_equal_then_jump_to("lw_size_ok",
                                                      rdx_len, LOG_SIZE);
    write_set_mov(rdx_len, LOG_SIZE);
write_label(size_ok);
```
`LOG_SIZE` was created with:
```c
AsmDataField LOG_SIZE = write_constant("LOG_SIZE", "100000h", "size");
```
so it is a globally registered constant. `write_set_mov(rdx_len, LOG_SIZE)` validates that `rdx` still holds `"len"` and that `LOG_SIZE` still holds `"size"` before emitting the `mov`.
The routine then names scratch-register contents:
```c
AsmDataField rsi_src = write_replace_mov("rsi", rcx_src);
AsmDataField r8_len  = write_replace_mov("r8",  rdx_len);
AsmDataField r9_head = write_replace_mov("r9",  logPos);
AsmDataField rdi_pView = write_replace_mov("rdi", pView);
```
Each `write_replace_mov` emits a `mov` and records the destination register's new tag. After address arithmetic:
```c
write_add(rdi_pView, r9_head);
AsmDataField rdi_dst = change_tag(rdi_pView, "dst");
```
After the add, `rdi` holds a write cursor (`"dst"`) rather than the mapped base (`"pView"`). `change_tag` updates the local dictionary without emitting assembly.
For the wrapped-copy case, `rep movsb` requires implicit registers:
```c
AsmDataField rcx_space = write_replace_mov("rcx", r10_space);
write_rep_movsb(rdi_dst, rsi_src, rcx_space);
```
The wrapper checks `rdi`, `rsi`, and `rcx` exactly, so a call with the right tags in the wrong registers fails during generation.
When the write head is updated:
```c
write_set_mov(logPos, r9_head);
write_ret();
```
`logPos` is a global from `.data?`:
```c
AsmDataField logPos = write_unitialised_data("logPos", "head");
```
`write_set_mov` confirms the global still has tag `"head"`. `write_ret` is void here, so it only emits `ret`.
The same descriptor is reused from `main`:
```c
write_call(&log_proc, offsetMsgStarted, msgStartedLen);
```
The call helper validates that the arguments have tags `"src"` and `"len"`, loads `rcx` and `rdx`, and emits `call log_write`.
## 11. Conventions
- Paired open/close writers return or consume descriptors.  `write_proc`
returns an `AsmExtern`; the caller passes `&proc` to `write_endp`.
- Declaration writers return the symbol or cell needed later.  For example,
`write_constant` returns an `AsmDataField`, and scalar data writers return typed global cells.
- Prefer named cells (`rdx_len`, `LOG_SIZE`, `offsetMsgStarted`) over repeated
string literals.
- Use `literal(value, tag)` for untracked immediates.
- Use `asm_void` for no-return procedures and externs.
- Use `write_set_mov` to update an existing meaning, `write_replace_mov` to
repurpose a register, and `write_init_mov` only for first stores into untracked local locations.
- Use `change_tag` after in-place arithmetic changes what a register means.
- Use `treat_as` for deliberate tag boundary crossings, especially constants
passed as a different Win32 parameter type.
- Let `write_extern_call` own Windows x64 shadow space, stack arguments, and
alignment. Use `write_call` only for private conventions.
- Indent C generator `write_*` calls to mirror the assembly being emitted: section
contents one level in, procedure bodies one level in, and conditional bodies an additional level in. The `write_proc` / `write_endp` pair stays at the outer level, and a `write_if_*` call sits at the enclosing level with its guarded body indented beneath it, up to the matching `write_label`. The indentation is cosmetic: it introduces no braces and does not change C scope.
- `write_extern` and `write_proc` copy the parameter list into the bump arena and set `is_register` on that copy. The caller's array does not have to outlive the descriptor.

## 12. Building

Run `build.bat` from the repository root. Do not compile, assemble, or link by hand. The script compiles `core/asm_main.c`, runs it to write `output.asm`, assembles that with `ml64`, and links `output.exe`. It does the same for `c/c_main.c` (`c_output.asm`, `c_output.exe`), runs `test/test.c` against both programs, then builds the language-wrapper tours and the examples. Everything it writes goes in `out/`.

Those compiles link `/NODEFAULTLIB`. `mainCRTStartup` calls `commit_stack_pages` before `main`, and `write_proc` of `main` emits the same walk into the generated image. A generated thread entry calls `write_commit_stack` itself. There is no `__chkstk` on either side.

## 13. Spill frames

`asm_frame_prologue(preserved, align)` opens one frame for the procedure that is being emitted.

- `preserved` is a bitmask of callee-saved GPRs. Bit `i` is `asm_gpr_index` `i` (`rax` is 0, `rcx` is 1, …, `r15` is 15). `rsp` (bit 4) and `rbp` (bit 5) are never pushed: `rbp` is the frame pointer and `rsp` is the stack. The prologue records the mask with `asm_set_preserved` and `asm_mark_saved`, so a later write to a preserved register outside the mask still fails in `asm_note_write_reg`.
- `align` is 16, 64, or 4096. A value below 16 is treated as 16.

The 16-byte prologue is `push rbp`, `mov rbp, rsp`, `sub rsp, 256`, then a push of each selected register from low index to high. Slots are `[rbp-n]`. `n` is the distance of the slot's lowest byte from `rbp`, so the address is aligned when `n` is. The reservation is 256 bytes.

64 and 4096 are the other two alignments the frame accepts, and they are the hardware widths those alignments exist for. 64 is one x64 cache line. A local that is going to be the target of a cache-line-sized move, or that must not share a line with a neighbour, asks for `ALIGN_CACHE`. 4096 is one x64 page. A local that must not cross a page, or that the program will hand to something that expects a page-aligned address, asks for `ALIGN_PAGE`. `rbp` after `mov rbp, rsp` is only 16-byte aligned, because that is what the Windows x64 call gives you. `and rsp, -64` or `and rsp, -4096` produces the stronger alignment, and `r12` keeps that base so the `and` does not have to be repeated for every slot.

The reservation is one fixed subtraction so the prologue does not grow a byte at a time as slots are taken. 256 bytes is 32 qwords. That covers the Windows home area (32 bytes) several times over and leaves room for the call-site saves and the evicted values of one procedure: each save takes a qword, and a procedure that is spilling is already under register pressure, not allocating arrays. A page-aligned frame reserves the whole page instead, because the alignment request was for a page and the slot has to fit inside the aligned region. The space is the procedure's frame. It is released by the epilogue. It is not a heap allocation and it is not retained across calls as a second stack. A function that never spills still pays the subtraction; the alternative is a frame whose size is patched at the end of generation, which this emitter does not do.

A 64-byte or 4096-byte prologue pushes `rbp` and the preserved registers first, then `and rsp, -align`, `sub rsp, 256` (or 4096 for a page), and `mov r12, rsp`. Slots are `[r12+n]`. `r12` holds the aligned base until the epilogue pops the saved copy. The page reservation is 4096 bytes.

`asm_frame_slot(size, align)` and `asm_frame_slot_at(size, align, &lo)` allocate a packed slot and return a bump-allocated operand string (`byte` / `word` / `dword` / `qword ptr`). `size` is 1, 2, 4, or 8. `align` may not exceed the frame alignment. `lo` is the key `asm_frame_release` expects. `asm_frame_operand` rebuilds the text for a key that is already taken. The frame is full when the reservation cannot hold the slot; generation aborts.

`asm_frame_epilogue` pops the preserved registers in reverse order and restores `rsp` and `rbp`. The 16-byte path pops first (the pushes sit below the 256-byte gap) and then `mov rsp, rbp`. The aligned path `lea rsp, [rbp-8*npush]` so the `and` and the reservation are discarded, then pops. The epilogue puts the preserved registers back, so those writes are not a clobber the caller can observe.

`asm_dispose_location` marks a location dead until the procedure ends, or until a later `write_init_mov` or `write_replace_mov` stores a new value there. A read or an in-place update of a disposed location is a generation error. `c_dispose` is the caller at the C layer: it also releases the register, so spill and call save/restore skip that value.

The C layer's `c_begin_function` / `c_end_function` are the usual callers. `c_set_preserved` and `c_set_frame_align` choose the mask and the alignment for the next function. Call-site save/restore and register-pressure eviction are both clients of `asm_frame_slot_at`; they are different operations and are described in [`c_architecture.md`](../c/c_architecture.md).

## 14. What the build checks

`build.bat` runs `core/asm_main.c` through to `output.exe`. The behavior program in `test/beh_tests.h` uses a handful of these writers: `write_proc`, `write_replace_mov` and `write_set_mov` (both call `write_mov`), `write_add`, `write_call` / `write_extern_call`, the frame helpers, and `write_init_mov`. [`../TEST.md`](../TEST.md) is the coverage report.

A mnemonic catalog was not built. One known-valid form of each integer and SSE mnemonic, assembled once in a dead procedure, would show that `ml64` accepts the spelling. It would not show that the instruction does the right thing. Privileged and legacy mnemonics (`wrmsr`, `hlt`, `pusha`, and the like) would fail that assemble even when the helper is intentional.

More than 500 `write_*` helpers only paste a mnemonic. Nothing in the build checks that `ml64` accepts that text, and nothing checks flags, SSE, x87, string instructions, `cmov`, `setcc`, shifts other than `shl`, `div` / `idiv`, `adc` / `sbb`, rotates, or bit scans.

Also without a behavior check:

- `write_replace_mov`, including the error when a global is repurposed.
- `write_set_mov` when the recorded tag does not match. The behavior program does reject `write_set_mov` of an untracked location.
- `asm_mem` / `asm_mem_index` displacements other than the ones the examples happen to emit.
- The type dictionary when the two arms of a branch assign different tags. The emitter records one tag for the location. Both arms in the current tests write the same tag.

`write_comment` emits `; text`. The MASM `COMMENT` directive is still the mnemonic string in `asm.h`. Emitting that directive makes its first character a delimiter and hides every following line until the character appears again. The comment on `write_comment` in `asm_funcs.h` is the longer account.

Several generation failures call `exit(1)` rather than `host_fail`: the bump arena, local and global dictionary overflow, a frame size or stack slot out of range, and a memory layout whose offsets do not line up. `host.h` defines `exit` as `ExitProcess`. The longjmp hook in the behavior suite sees `host_fail` only. `asm_tests.h` checks alignment, a few call-frame sizes, and whether a name is a register. It does not generate a program.
