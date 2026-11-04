# `c_funcs.h` interface design

## Overview

`c_funcs.h` is a thin C-like abstraction layer built on top of `asm_funcs.h`
(see [`asm_interface.md`](asm_interface.md)). Where `asm_funcs.h` makes you name
every *location* — which register or stack slot each value lives in — `c_funcs.h`
raises the abstraction to typed *values*: you declare functions, parameters, and
local variables **without** naming storage, and this layer performs automatic
register allocation. Struct layouts are computed automatically from field types.

Crucially, `c_funcs.h` emits **no assembly of its own**. Every construct flows
through the existing `write_*` emitters, so the generation-time type-tag
dictionaries keep validating the program exactly as before. `c_funcs.h` only
*plans locations*; `asm_funcs.h` still does all the writing and all the checking.

`c_main.c` is the canonical example. It regenerates — byte-for-behaviour —
the same program `core/asm_main.c` produces (a 1 MB memory-mapped circular log, a private
`log_write` routine, a Win32 window procedure, and a `main` that maps the file,
logs a message, creates a window, runs a message loop, and exits), but written
against `c_funcs.h`. The output goes to `c_output.asm`, and the shared `test.c`
suite validates both `output.exe` and `c_output.exe`.

## Design principles (inherited from `asm_funcs.h`)

- **Paired open/close helpers return a descriptor the closer consumes.**
  `c_begin_function` returns a `CFunc`; you pass `&fn` to `c_end_function`.
- **A declaration returns the value later references reuse.** `c_param`,
  `c_dup`, `c_local_imm`, `c_call_ext`, and `c_field` all return a `CValue` you
  pass to subsequent helpers rather than repeating a location literal.
- **Each logical value owns a unique type tag** (by default its variable name),
  so the dictionaries validate every value independently.
- **All output is produced by `asm_funcs.h`.** This header never calls `write()`;
  it composes the existing emitters.

## Table of contents

1. [`CType`: scalar types](#1-ctype-scalar-types)
2. [The register pool](#2-the-register-pool)
3. [`CValue`: a tracked value](#3-cvalue-a-tracked-value)
4. [Functions and parameters](#4-functions-and-parameters)
5. [Locals, moves, and arithmetic](#5-locals-moves-and-arithmetic)
6. [Structured control flow](#6-structured-control-flow)
7. [Calls](#7-calls)
8. [Structs](#8-structs)
9. [Interop with the asm layer](#9-interop-with-the-asm-layer)
10. [Worked example: `log_write`](#10-worked-example-log_write)
11. [Conventions and limits](#11-conventions-and-limits)

## 1. `CType`: scalar types

A `CType` is a scalar type descriptor: a documentation `name`, a `size` in bytes
(1/2/4/8), and an `is_signed` flag that selects signed vs unsigned comparisons.

```c
typedef struct { const char* name; int size; bool is_signed; } CType;

static const CType C_CHAR, C_BOOL, C_SHORT, C_INT, C_UINT, C_LONG, C_PTR;
```

`size` selects the **register width** (e.g. a `C_INT` value in register 0 is
`eax`; a `C_PTR` value is `rax`). Struct padding is not implied by `size`; it
follows the alignment mode in [`c_architecture.md`](c_architecture.md). Because these are
`static const` variables, they are *not* constant expressions in C — arrays of
`CParam`/`CStructFieldSpec` that reference them must be ordinary locals, not
`static`/`const` initializers.

## 2. The register pool

The heart of automatic allocation is a 16-entry table of the general-purpose
registers, each carrying its four width-specific names:

```c
typedef struct {
    const char* names[4];   /* byte, word, dword, qword */
    bool is_volatile;       /* Windows x64 caller-saved: rax, rcx, rdx, r8-r11 */
    bool reserved;          /* rsp/rbp: never allocated */
    bool in_use;            /* a live CValue currently owns it */
} CReg;

static CReg c_regs[16];
```

Allocation hands out registers in a deliberate order:

```c
static const int c_alloc_order[] = { 0, 8, 9, 10, 11, 3, 12, 13, 14, 15, 1, 6, 7, 2 };
/*                                   rax r8 r9 r10 r11 rbx r12..r15   rcx rsi rdi rdx */
```

Volatile scratch (`rax`, `r8`–`r11`) is handed out first so short-lived
temporaries never touch a callee-saved register we would otherwise have to
preserve; the callee-saved set comes next; the ABI argument / string-op
registers (`rcx`, `rsi`, `rdi`, `rdx`) are last, since those are usually wanted
for a *specific* purpose and are pinned explicitly.

Helpers: `c_reg_name(idx, size)` (sized name), `c_reg_index(name)` (reverse
lookup by any width-name), `c_alloc_reg()` (claim the next free register, spilling
a registered victim when the pool is exhausted; see [Spilling](c_architecture.md#6-spilling)),
and `c_pool_reset()` (release all non-reserved registers,
done at every procedure boundary).

## 3. `CValue`: a tracked value

```c
typedef struct {
    const char* name;   /* logical identity */
    CType       type;   /* scalar type (width/signedness) */
    const char* loc;    /* current location: a sized register or memory operand */
    const char* tag;    /* asm type tag (defaults to name) */
    int         reg;    /* owning pool index, or -1 for memory/immediate/result */
    bool        live;
} CValue;
```

`c_cell(v)` projects a `CValue` down to the `AsmDataField {loc, tag}` cell the
asm layer consumes. A value whose `reg == -1` is not owned by the pool (a global,
a struct field, an immediate, or a call result).

Several constructors produce `CValue`s that are not register temporaries:

| Constructor | Produces |
| --- | --- |
| `c_global(name, type, tag)` | an uninitialised data global (a mutable program variable); wraps `write_unitialised_data` |
| `c_constant(name, value, type, tag)` | a named equate usable in comparisons/arithmetic; wraps `write_constant` |
| `c_imm(value, type, tag)` | an inline immediate operand (e.g. a comparison right-hand side) |
| `c_wrap(field, type)` | adopts any existing `AsmDataField` (global, constant, struct field) as a `CValue` |
| `c_ptr_data(name, target, tag)` | a pointer-sized data global initialised to `dq offset target` (vtables, headers) |
| `c_word_data(name, value, tag)` | a pointer-sized data global initialised to `dq value` |
| `c_byte_length(name, data, tag)` | a length equate `name = $ - <data>` measuring an emitted byte run, hiding the `$ - label` idiom |
| `c_addr(symbol, tag)` | an `"offset <symbol>"` address operand (used at call sites and in data) |
| `c_string(name, text, off_tag, len_tag)` | emits a read-only string and returns a `CStr {off, len}` — the bytes, an address constant, and a byte-length constant in one call |
| `c_offset(symbol)` | the `"offset <symbol>"` initializer *string*, for a struct field that stores a code/data address |

Because globals are `CValue`s, they are assigned with the same helpers as
locals: `c_assign(&pView, pv)` / `c_assign_imm(&logPos, "0")` rather than a raw
`write_set_mov`. The `c_ptr_data` / `c_word_data` / `c_byte_length` / `c_addr`
helpers let a generator declare pointer tables, object headers, and
string-length constants without hand-writing `dq`, `offset`, or `$ - label`
assembly. **Any `CValue` wrapping a `dq`/`c_word_data`/`c_ptr_data` global must be
typed `C_PTR`** (a qword), or `c_dup`/arithmetic will emit a size-mismatched
32-bit `mov`.

`c_string` bundles the three raw steps a string datum otherwise needs
(`write_initialised_data` for the bytes, `write_offset_constant` for its address,
and a `$ - name` length equate) into one call that returns operand cells ready to
pass as call arguments, so a generator never spells out an offset constant, an
`AsmDataField`, or the location-counter idiom. Its `off_tag` / `len_tag` tag the
address and length to match the parameters they are handed to (call-argument
validation checks them). `c_offset` is its data-initializer sibling: it returns
the bare `"offset <symbol>"` text for a struct field that stores an address (e.g.
a WNDCLASS `lpfnWndProc` slot), keeping the `offset` keyword out of the generator.

## 4. Functions and parameters

### External functions

```c
AsmExtern c_extern(const char* name, const char* const* param_tags, int nparam,
                   CType ret, const char* ret_tag);   /* ret_tag NULL => void */
```

`c_extern` declares an imported function that follows the Windows x64 calling
convention, without naming any storage. You supply just one **type tag** per
parameter; `c_extern` places the first four in `rcx`/`rdx`/`r8`/`r9` and the rest
in the caller's stack slots (`qword ptr [rsp+40]`, `+48`, …), and returns the
result in the ABI return register sized to `ret` (`al`/`ax`/`eax`/`rax`). The
`param_tags` array is only read during the call, so a local/compound array is
fine; the ABI cells it builds are stored in the bump arena so they outlive every
matching call site.

```c
const char* create_file_params[] = {
    "lpFileName", "dwDesiredAccess", "dwShareMode", "lpSecurityAttributes",
    "dwCreationDisposition", "dwFlagsAndAttributes", "hTemplateFile"
};
AsmExtern create_file = c_extern("CreateFileA", create_file_params, 7, C_PTR, "hFile");

const char* exit_params[] = {"exit_code"};
AsmExtern exit_process = c_extern("ExitProcess", exit_params, 1, C_PTR, NULL); /* void */
```

For a function that does **not** follow this convention, drop down to the
`asm_funcs.h` API (`write_common_extern` / `write_extern`) and name the locations
yourself.

### Internal functions

```c
typedef struct { const char* name; CType type; } CParam;

CFunc c_begin_function(const char* name, const CParam* params, int nparam,
                       CType ret, const char* ret_tag);   /* ret_tag NULL => void */
CValue c_param(CFunc* f, int i);
void   c_ret(CFunc* f);        /* emit `ret` (validated against declared return) */
void   c_ret_zero(CFunc* f);   /* xor result reg, ret */
void   c_end_function(CFunc* f);
```

`c_begin_function` places the first four parameters in `rcx`/`rdx`/`r8`/`r9`
(sized to each type). With spilling on, which is the default, a fifth parameter
is the operand `qword ptr [rbp+48]` (then `[rbp+56]`, …). The prologue is
`push rbp` / `mov rbp, rsp`, so the caller's `[rsp+40]` is the callee's
`[rbp+48]`. With spilling off, the same parameter is named
`qword ptr [rsp+40]`. It resets the register pool, seeds the local type
dictionary via `write_proc`, and marks the parameter registers `in_use` so
allocation won't clobber a live parameter. The returned `CFunc` embeds the
`AsmExtern` descriptor. Pass `&fn` to `c_end_function`. Register arguments go
to `c_call_proc` through `&fn.ext`.

`c_call_proc` forwards to `write_call`, which moves register arguments and does
not store a stack argument. `c_call_ext` forwards to `write_extern_call`, which
stores stack arguments and accepts a stack parameter only when its location
text is exactly `qword ptr [rsp+40]` (then `+48`, …). A procedure opened with
`c_begin_function` while spilling is on fails that check, because its cell says
`[rbp+48]`. One cell cannot satisfy both spellings. A fifth argument that does
run is the behavior test's `fifth`: `write_proc` declares
`qword ptr [rsp+40]`, `c_call_ext` stores it, and the body reads
`qword ptr [rbp+48]` after `asm_frame_prologue`. There is no test of
`c_begin_function` with five parameters together with `c_call_proc`.

```c
const CParam lw_params[] = {{"src", C_PTR}, {"len", C_PTR}};
CFunc log_write = c_begin_function("log_write", lw_params, 2, C_PTR, NULL); /* void */
    CValue src = c_param(&log_write, 0);   /* bound to rcx */
    CValue len = c_param(&log_write, 1);   /* bound to rdx */
    /* ... body ... */
    c_ret(&log_write);
c_end_function(&log_write);
```

## 5. Locals, moves, and arithmetic

| Helper | Meaning |
| --- | --- |
| `c_dup(name, type, src)` | copy `src` into a **freshly allocated** register as a new variable tagged `name` |
| `c_local_imm(name, type, imm)` | allocate a register and initialise it to an immediate |
| `c_assign(&dst, src)` / `c_assign_imm(&dst, imm)` | `dst := …`, **preserving** `dst`'s meaning (`write_set_mov`) |
| `c_add(&dst, src)` / `c_sub(&dst, src)` | read-modify-write |
| `c_zero(&dst)` | `dst := 0` (xor) |
| `c_relabel(&v, tag)` | relabel in place after a value is repurposed (updates the dictionary) |
| `c_free(&v)` | release the register back to the pool |
| `c_dispose(&v)` | release the register, and make a later read or write a generation error. A call does not spill or restore it |
| `c_mark_const(&v)` | after the initial store, a later set (`c_assign`, `c_add`, `c_zero`, and the other mutators) is a generation error |
| `c_rep_movsb(dst, src, count)` | `rep movsb`; places the operands into `rdi`/`rsi`/`rcx` itself |
| `c_into(reg, type, src, tag)` | copy `src` into a **specific pinned** register |

Move selection is handled by an internal `c_materialize`, which picks
`write_init_mov` (first store into an untracked location) vs `write_replace_mov`
(repurpose an already-tracked register) by consulting `asm_type_lookup`. This
keeps the local dictionary consistent as registers are reused across a procedure.

The dictionaries are the linear tables in `asm_funcs.h`: 1024 local entries
(`ASM_TYPE_DICT_MAX`) and 128 global entries (`ASM_GLOBAL_DICT_MAX`). A lookup
scans the array. At these sizes that scan is the efficient structure. A tree
or a hash table spends more on the node or the hash than a walk of the live
pointers, and generation already spends its time writing text. The local table
is cleared at each procedure. It is sized for every `write_replace_mov`
destination in one procedure, including the fresh frame slot each call save
uses. The global table is write-once for the program. Overflow aborts. A
larger unit raises the constant. The reasoning is in
[`c_architecture.md`](c_architecture.md).

`c_rep_movsb` **hides** the instruction's fixed-register requirement: `rep movsb`
mandates `rdi`/`rsi`/`rcx` and clobbers all three, so the helper emits the moves
into those registers itself and the generator never names them. It moves the two
sources before loading `count` into `rcx` (so a source that happens to live in
`rcx` is read first). Because the pool hands out `rax`/`r8`–`r11`/`rbx`/… well
ahead of `rsi`/`rdi`/`rcx`, the generator's live values sit in registers `rep movsb`
leaves untouched; to reuse a source across several copies, hold it in such a
register (e.g. `c_dup` a parameter first — a parameter arriving in `rcx` would
otherwise be destroyed by the first copy).

`c_into` pins a value in a mandated register. `rep movsb` is the typical case:
the instruction reads `rsi`, `rdi`, and `rcx` and has no operands of its own.
`c_rep_movsb` places those registers itself. When the generator must pin them
before a raw `write_rep_movsb`, it does so with `c_into`:

```c
CValue rdi_dst = c_into("rdi", C_PTR, dst, "dst");
CValue rsi_src = c_into("rsi", C_PTR, src, "src");
CValue rcx_n   = c_into("rcx", C_PTR, n, "count");
write_rep_movsb(c_cell(rdi_dst), c_cell(rsi_src), c_cell(rcx_n));
```

The result stays in that register. A value pinned this way is not a spill victim.

## 6. Structured control flow

Labels are generated automatically (`Lc_0`, `Lc_1`, …) into `asm_funcs.h`'s bump
arena, so they outlive emission (`write()` stores pointers without copying).

```c
CLabel loop = c_label();      /* a jump-back target */
c_goto(loop);                 /* unconditional jump */

CIf c = c_if_eq(a, b);        /* body runs when a == b */
    /* guarded body */
c_end_if(&c);
```

The four structured conditionals run the guarded body (everything between the
`c_if_*` call and its `c_end_if`) exactly when the named condition holds. Each is
implemented by branching **past** the body on the *negated* condition, reusing
`asm_funcs.h`'s four folded compare-and-jump helpers:

| Helper | Body runs when | Skips past body via |
| --- | --- | --- |
| `c_if_eq(a, b)` | `a == b` | `jne` (`write_if_not_equal_then_jump_to`) |
| `c_if_ugt(a, b)` | `a > b` (unsigned) | `jbe` (`write_if_unsigned_less_than_or_equal_…`) |
| `c_if_uge(a, b)` | `a >= b` (unsigned) | `jb` (`write_if_unsigned_less_than_…`) |
| `c_if_sgt(a, b)` | `a > b` (signed) | `jle` (`write_if_signed_less_than_or_equal_…`) |

These four negations exactly cover every branch in the reference program. Per the
project's indentation convention, the guarded body is indented one level between
`c_if_*` and `c_end_if`.

`c_end_if`, and the other closers (`c_end_if_else`, `c_end_switch`, `c_loop_end`),
check that the pointer closes the most recent open block of that kind. A pointer
to an older block aborts generation. The closer is both the readable end of the
block and the check that it matches.

### If / else

When both arms are needed, `c_if_eq_else` / `c_if_ne_else` return a `CElse`
handle carrying the else-label and done-label. The generator writes the then-arm,
calls `c_else` (which emits `jmp done` then the else-label), writes the else-arm,
and closes with `c_end_if_else` (emits the done-label). This keeps the two-way
branch symmetric without the generator naming any label:

```c
CElse e = c_if_eq_else(a, b);   /* then-arm runs when a == b */
    /* then-arm */
c_else(&e);
    /* else-arm */
c_end_if_else(&e);
```

### Switch

`c_switch(value)` opens a no-fall-through switch over an integer `CValue`;
`c_case(key)` starts a case (each case skips the previous one via `cmp`/`jne`),
`c_break` / `c_default` / `c_end_switch` close cases. Each case body must divert
itself (a `c_ret` or `c_break`); there is no implicit fall-through.

## 7. Calls

```c
CValue c_call_ext(CType ret_ctype, const AsmExtern* ext, ...);
CValue c_call_proc(CType ret_ctype, const AsmExtern* proc, ...);
void   c_call_ext_void(const AsmExtern* ext, ...);
void   c_call_proc_void(const AsmExtern* proc, ...);
```

These are functions. C has no overloads, so the four shapes are four functions:
a result or none, and the Windows x64 frame (`c_call_ext`) or a private
convention (`c_call_proc`). The return type stays the first parameter, which
keeps the argument order of the call sites.

They are `__declspec(noinline)`, and `va_start` is the first statement, before
`c_spill_volatiles`. MSVC's `va_start` reads the home slots of the real call
frame. Inlining a variadic function whose first parameter is a 16-byte `CType`
drops those slots: under the x64 convention that struct is passed as a pointer,
while the `va_arg` macro in `host.h` still steps one 8-byte slot per argument.
A call before `va_start` can move the frame the macro reads. A macro at the
call site would have hidden that order inside every generator. The functions keep
it in one place.

Arguments are ordinary `asm_funcs.h` operand cells — immediates via
`literal(...)`, globals / offset constants, or `c_cell(local)` — passed straight
through. The underlying `write_extern_call` / `write_call` emit the `mov <argreg>,
<arg>` for each argument, validate each argument's tag against the declared
parameter tag, emit the `CALL`, and record the return register in the local
dictionary. `c_bind_return` then wraps that return register as a `CValue` of the
requested `CType`.

```c
CValue hf = c_call_ext(C_PTR, &create_file,
                       offsetLogPath, literal("0C0000000h", "dwDesiredAccess"),
                       /* ... */);
c_call_ext_void(&exit_process, literal("123", "exit_code"));
```

`c_call(CFunc* f, ...)` is a thin variadic void call over an internal proc that
takes the `CFunc` directly (rather than `&f->ext`), so the caller never
touches the `.ext` member. Use it for internal procedure calls whose result is
discarded; it forwards its arguments to the same `write_callv` core.

`c_addr(symbol, tag)` returns the `"offset <symbol>"` operand cell as a `CValue`,
so generators pass an address without writing the `offset` literal. `c_ptr_data`,
`c_word_data`, and `c_byte_length` (below) round out the leak-free data helpers.

Argument marshalling assumes an argument doesn't sit in a register another
argument is about to overwrite. A value created by `c_dup`, `c_local_imm`,
`c_param`, or a call is tracked, and a call saves and restores that register.
A value pinned with `c_into` is not
an eviction victim. An untracked volatile is poisoned if it is read after the
call.

Stack arguments are the gap in section 4. `c_call_proc` does not store them.
`c_call_ext` stores them only for a descriptor whose stack cells are
`qword ptr [rsp+40]` and the slots after it. The behavior program does not
cover calls with 0, 1, or 3 arguments, or arguments past the fifth. `c_extern`
imports in that program are `ExitProcess` only. The window demo calls Win32
once, with fixed arguments.

## 8. Structs

Field offsets follow the active alignment mode. The default is `ALIGN_NONE`:
fields pack tight, and any padding is an explicit field. `align_set_default`
sets the mode for later structs, `c_struct_set_align` overrides one struct, and
`c_struct_field_mode` overrides one field. The modes are described in
[`c_architecture.md`](c_architecture.md). `align_pad` in `host.h` is the single
implementation.

```c
CStruct wc = c_struct_begin("wndClass", "0");   /* pad initializer: "0" in .data */
c_struct_field(&wc, "",            "0",              "style",       C_INT); /* @0 base */
c_struct_field(&wc, "lpfnWndProc", "offset WndProc", "lpfnWndProc", C_PTR); /* @8  */
c_struct_field(&wc, "wcInstance",  "0",              "hInstance",   C_PTR); /* @24 */
/* ... */
CStructRef wndClass = c_struct_emit(&wc);
CValue wcInstance = c_field(&wndClass, "wcInstance", C_PTR);  /* patched at runtime */
```

The offsets in those comments are the `ALIGN_C` layout. `c_main.c` opts into
that mode because the record is Win32 interop. Under the default, `ALIGN_NONE`,
the pointer would be packed at the next byte.

- `base_name`, passed to `c_struct_begin`, is the MASM label of byte 0 of this
  instance. When the first field's name is blank, `c_struct_emit` uses
  `base_name` as that field's label, so the name and the first field are the
  same address. `offset wndClass` is the pointer an API such as
  `RegisterClassA` wants. A sized operand such as `qword ptr wndClass` would
  load the first qword, which is a different value. `c_struct_base` returns
  `{base_name, tag}` for `write_offset_constant`. The tag is what the
  dictionary checks when that address is passed to a call. `c_field` is the
  other lookup: a named member, as a sized memory operand.
- The **field tag** is explicit and separate from the `CType` (which supplies
  only the size), mirroring `set_memory_layout_entry`. Choosing a tag
  like `"hInstance"` lets a field match the param tag of the API that consumes it.
- **`pad_value`** must be `"0"` in an initialised `.data` section and `"?"` in an
  uninitialised `.data?` section — `ml64` requires `?` in `.data?`.

`c_struct_emit` builds the padded layout, emits it via
`create_memory_layout` / `set_memory_layout_entry` / `write_data_memory_layout`,
and returns per-field references. The WNDCLASSA layout above totals 72 bytes and
the MSG layout 48 bytes, matching `core/asm_main.c`'s hand-written layouts.

`c_struct_base(&ref, tag)` returns the struct's base label as a bare-label
operand cell (`{base_name, tag}`), suitable for `write_offset_constant`:
```c
CStructRef wndClass = c_struct_emit(&wc);
AsmDataField offsetWndClass =
    write_offset_constant("offset_wndClass", c_struct_base(&wndClass, "lpWndClass"), "lpWndClass");
```
Unlike `c_field` (which yields the sized `qword ptr <field>` operand), this hands
back the raw base label, so `offset <struct>` reuses the struct reference rather
than repeating the literal.

## 9. Interop with the asm layer

`c_funcs.h` is designed to be mixed freely with raw `asm_funcs.h` calls:

- `c_global` / `c_constant` wrap the data-declaration emitters and hand back
  `CValue`s directly; `c_wrap(field, type)` adopts an `AsmDataField` you already
  have (a struct-field ref, or any operand) as a `CValue`.
- `c_cell(value)` goes the other way, projecting a `CValue` to an `AsmDataField`
  for a raw emitter or a **call argument** (calls take asm operand cells).
- `c_treat_as(value, existing_tag, as_tag)` re-tags a `CValue` across a tag
  boundary at a call site (mirrors `asm_funcs.h`'s `treat_as`).

Call arguments are still expressed as asm operand cells — immediates via
`literal(...)`, address constants via `write_constant` /
`write_offset_constant` (the latter reuses a data-field handle for an
`offset X` equate instead of repeating the label), or `c_cell(value)` — since
that is the natural boundary the underlying `write_*_call` validates against. So
`c_main.c` opens sections with `write_data` / `write_dataq` / `write_code`
directly and emits the string data with `write_initialised_data`, while the
C-like layer covers functions, externs, variables, control flow, calls, and
structs.

## 10. Worked example: `log_write`

The clamp-and-wrap ring-buffer append from `c_main.c` shows the layer end to end
— every value auto-allocated (the generator names no register), `c_rep_movsb`
hiding the `rdi`/`rsi`/`rcx` requirement, structured conditionals, and `CValue`
globals assigned with `c_assign`:

```c
CFunc log_write = c_begin_function("log_write", lw_params, 2, C_PTR, NULL);
    CValue src = c_dup("src", C_PTR, c_param(&log_write, 0));  /* stable copy */
    CValue len = c_param(&log_write, 1);

    CIf tooBig = c_if_ugt(len, LOG);      /* clamp len to LOG_SIZE */
        c_assign(&len, LOG);
    c_end_if(&tooBig);

    CValue head  = c_dup("head", C_PTR, logPos);   /* logPos/pView are c_global CValues */
    CValue dst   = c_dup("dst", C_PTR, pView);
    c_add(&dst, head);                    /* dst = pView + head */
    CValue space = c_dup("space", C_PTR, LOG);
    c_sub(&space, head);                  /* space = LOG_SIZE - head */

    CIf wrap = c_if_ugt(len, space);      /* record spans the end: wrap */
        c_rep_movsb(dst, src, space);             /* copy tail (auto rdi/rsi/rcx) */
        c_assign(&dst, pView);                    /* restart cursor at base */
        CValue rem = c_dup("len", C_PTR, len);
        c_sub(&rem, space);                       /* remaining = len - space = new head */
        c_rep_movsb(dst, src, rem);               /* copy remainder */
        c_relabel(&rem, "head");
        c_assign(&logPos, rem);
        c_ret(&log_write);
    c_end_if(&wrap);

    c_rep_movsb(dst, src, len);           /* single-copy path */
    c_add(&head, len);
    CIf noWrap = c_if_uge(head, LOG);
        c_sub(&head, LOG);
    c_end_if(&noWrap);
    c_assign(&logPos, head);
    c_ret(&log_write);
c_end_function(&log_write);
```

Because `src` arrives in `rcx` (parameter 0) — the register `rep movsb` uses for
its count — it is copied into a stable auto-allocated register (`c_dup`) once, up
front, so it survives both copies on the wrap path.

## 11. Conventions and limits

- **The 256-byte frame is the spill reservation, not a wasted buffer.** A GPR is a general-purpose register: one of the sixteen integer registers `rax`, `rcx`, `rdx`, `rbx`, `rsp`, `rbp`, `rsi`, `rdi`, and `r8` through `r15`. The byte, word, and dword spellings (`al`, `eax`, `r8d`) name that same GPR. It is one `sub rsp, 256` so generation does not have to patch the prologue after it has seen every spill. 256 bytes is 32 qwords: the 32-byte Windows home area, plus a qword for each GPR a call saves, plus the values evicted when the allocator runs out of registers. Slots are packed to the value's size inside that reservation, and the epilogue gives the whole reservation back. A function that spills nothing still subtracts 256. That is the cost of a fixed frame. It is not a heap, and it does not accumulate across calls. `ALIGN_CACHE` (64, one cache line) and `ALIGN_PAGE` (4096, one page) are the other two widths, used when a local has to have that alignment. `rbp` is only 16-byte aligned, so those frames align `rsp` and keep the base in `r12`. The page frame reserves 4096 because the slot has to sit inside the aligned page.
- **Spilling is on.** `c_begin_function` opens a 256-byte frame. Slots are
  packed to the value's size and addressed as `[rbp-n]`. `c_set_frame_align`
  before the function selects 64 or 4096, which aligns a base in `r12` and
  addresses slots as `[r12+n]`. `c_set_preserved` replaces the saved set for
  the next function. `c_stack_local` places an uninitialised local at
  `ALIGN_NONE`, `ALIGN_C`, `ALIGN_CACHE`, or `ALIGN_PAGE`.
  `c_spill_off` / `c_spill_on` disable eviction locally. `C_DISABLE_SPILL`
  builds without the frame. `C_ENABLE_SPILL` is accepted and changes nothing.
  A call saves each named live register the callee may clobber and restores
  the same register, so that `CValue` stays valid. A value is an eviction
  victim when it was created by `c_dup`, `c_local_imm`, `c_param`, or a call.
  Those values carry an id into a table the allocator updates, so the caller
  does not register a stable address. A value pinned with `c_into` is not a victim. A value that was not saved is poisoned
  on use after a call, not aborted at the call.
  `c_free` hands a register back to the pool. `c_dispose` does that and marks the value dead, so a later use is a generation error and a call does not spill or restore it. `c_mark_const`, applied after the initial store, makes a later set a generation error. The reasoning is in
  [`c_architecture.md`](c_architecture.md).
- **Tag discipline is unchanged.** All the `asm_funcs.h` dictionary rules still
  apply; `c_funcs.h` just chooses locations. A default tag equal to the variable
  name keeps each value independently validated; `c_relabel` (over
  `change_tag`) is the sanctioned way to repurpose a register in place.
- **Strings must outlive emission.** Synthesised labels and computed stack-slot
  strings are stored in `asm_funcs.h`'s bump arena (`asm_bump_alloc`) because
  `write()` stores pointers without copying.
- **Static-const `CType` caveat.** The `C_*` type constants are not C constant
  expressions, so `CParam` / struct-field arrays that reference them must be
  automatic locals, not `static`/`const`.
- **Build.** Run `build.bat`. It produces `output.exe` (from `core/asm_main.c`)
  and `c_output.exe` (from `c_main.c`) in `out/`, and runs the shared `test.c`
  suite against both. [`../TEST.md`](../TEST.md) is the coverage report.
- **What the behavior program checks.** One generated program in
  `test/beh_tests.h`, plus the PCG stream in `test/fuzz_beh.h`. Covered at
  this layer: spill of a volatile across a call, fifteen live values that
  spill, a stack slot across that call, `c_add` of two parameters, one fifth
  stack argument through `write_proc` and `c_call_ext` (section 4), and the
  small ALU / loop / compare / switch / struct fuzz. Generation failures that
  longjmp through `host_fail` cover one unsaved preserved call, one unsaved
  `write_replace_mov` (it calls `write_mov`), a type mismatch, a bad `treat_as`, a second `write_init_mov`,
  `write_set_mov` of an untracked location, a wrong stack-slot string,
  spill-off poison, a write of a const value, and a read of a disposed value.
  `c_tests.h` checks register-name width, the spill flag, all 16 GPR indexes,
  `c_mark_const` on an untracked value, and `c_cell` of a disposed value.
  It does not generate a program.
- **Operations with no behavior check.** `c_rep_movsb`, `c_and_imm`,
  `c_relabel`, `c_goto`, `c_loop_continue`. One-armed compares other than the
  three the fuzz emits (`==`, unsigned `>`, signed `>`). That program has no
  `c_if_ult`, `c_if_slt`, or `c_if_ne` except `c_if_ne_else`. Nested `if` /
  `else` / `loop` / `switch`, and a switch with no default. Structs emitted
  under `ALIGN_NONE`, `ALIGN_CACHE`, and `ALIGN_PAGE`. The fuzz checks
  `align_pad` on the host, then emits one struct. Unions. A field read through
  `c_field` rather than base plus offset. Narrow returns (`C_CHAR`, `C_SHORT`)
  and mixing widths in one procedure. `c_set_preserved` and
  `c_set_frame_align` for a cache-line or page frame. More than one live value
  spilled under a random allocation pattern: the fixed test spills a specific
  set, and the fuzz keeps a handful of locals so a `mem, mem` instruction is
  not emitted. Poison, spill, and the type dictionary across an `if` / `else`
  join are only hit by the small branch fuzz. The emitter records one tag for
  a location. Both arms in the current tests write the same tag. `c_string`,
  `c_global`, `c_constant`, `c_byte_length`, and `c_ptr_data` are not in that
  program. `c_call_ext` argument marshalling for a real Win32 signature is the
  window demo, once, with fixed arguments.
- **Failures that still call `exit(1)`.** `c_struct_emit` overflow, a missing
  `c_field`, and `c_into` of an unknown register. The bump arena, the type
  dictionaries, a bad frame size, and a layout that does not line up do the
  same in `asm_funcs.h`. `host.h` defines `exit` as `ExitProcess`. With
  `host_fail_env` set, those paths still leave the process. The longjmp hook
  sees `host_fail` only.
