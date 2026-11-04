# The C value layer — architecture

`c_funcs.h` is a C-like code-generation layer that sits **on top of**
`asm_funcs.h`. This document explains its architecture and the reasoning behind
it. For the full call-by-call API, see [`c_interface.md`](c_interface.md); this is
the "why", that is the "what".

## 1. What it raises

`asm_funcs.h` lets you emit MASM x64 by calling `write_*` helpers where every
operand is an `AsmDataField {location, tag}` and a pair of type dictionaries
(local per-procedure + global) validate, at generation time, that each location
holds the tag it is used with. It is powerful but *location-oriented*: the caller
must name a register or stack slot for every value.

`c_funcs.h` raises that to **"C with typed values"**. The caller writes C-like
constructs — functions, parameters, locals, arithmetic, `if`, calls, structs —
**without ever naming a register or stack slot**. The layer performs automatic
storage allocation (register allocation and spilling) and reuses the
existing tag machinery so the generated program is validated exactly as before.

The key architectural decision is that `c_funcs.h` adds **no new output path**.
Every construct ultimately calls the existing `write_*` functions, so the tag
dictionaries, indentation, and `.asm` emission keep working unchanged. `c_funcs.h`
is a *planner* that decides locations; `asm_funcs.h` remains the *emitter* and the
*validator*. A `c_funcs.h` mistake therefore still surfaces as an ordinary
tag-validation abort at generation time — the correctness checks come for free.

`c_main.c` exercises the layer by regenerating, behaviour-for-behaviour, the same
program `core/asm_main.c` produces by hand — proving the abstraction loses nothing.

## 2. The primitives it drives

The value layer is a re-composition of primitives that already exist in
`asm_funcs.h`; understanding them explains why the C layer looks the way it does.

- **`AsmDataField {name, type, use_as}`** — a value cell: `name` is the location
  string, `type` the real tag, `use_as` an optional re-label. The C layer wraps
  this as a `CValue` and projects back down with `c_cell`.
- **Two dictionaries** (`asm_type_lookup` consults both): the *local*
  (`asm_type_dict`), per-procedure, seeded by `write_proc` and cleared by
  `write_endp`; the *global* (`asm_global_dict`), data globals + named constants,
  never cleared and read-only once written.
- **Three semantic moves**, which dictate how the C layer establishes vs. updates a
  value's location: `write_init_mov` (first store into an untracked location,
  *establishes* the entry), `write_replace_mov` (*repurposes* a register,
  overwrites the entry), `write_set_mov` (*updates in place*, requires a matching
  pre-existing entry). `change_tag` / `treat_as` relabel without moving.
- **Procedures / calls / arithmetic / data**: `write_proc`/`write_endp`,
  `write_call`/`write_extern_call`, `write_add`/`write_sub`/`write_cmp`, the folded
  `write_if_<cond>_then_jump_to`, and the struct helpers `create_memory_layout` /
  `set_memory_layout_entry` / `write_data_memory_layout`.

The dictionaries are linear arrays, 1024 entries local and 128 global. Lookup is
a scan from index 0 to the live count. That is the structure that fits a
dictionary this size. A balanced tree pays for allocations and pointer chasing
on every insert. A hash table pays for the hash and for a bucket array that is
mostly empty when only a few hundred keys are live. A scan of those string
pointers is shorter than either, and the generator's time goes into writing
the `.asm` text, not into these lookups. The local cap is 1024 because
`write_replace_mov` records every destination, and a call saves each live
register to a fresh frame-slot string. The tables are process-global, matching
the single-threaded generator. Overflow aborts with the cap in the message.
Raising the cap is a constant change. The arrays stay.

## 3. Design principles inherited from the codebase

`c_funcs.h` follows the same conventions as the rest of the project, which is why
its shape mirrors `asm_funcs.h`:

1. **Paired open/close helpers return a descriptor the closer consumes.**
   `c_begin_function` returns a `CFunc` passed to `c_end_function`; the struct and
   `if` helpers behave the same way.
2. **A declaration returns the value later references reuse.** `c_param`, `c_dup`,
   `c_local_imm`, `c_call_ext`, and `c_field` return a `CValue` reused everywhere
   the variable is read/written — never a re-typed literal.
3. **One tag per logical value.** Each value owns a unique tag (default: its own
   name), so validation stays meaningful even as the *physical register* is reused.
4. **Every emit goes through `asm_funcs.h`.** No `c_funcs.h` function writes raw
   text; all output flows through `write_*`, keeping tag validation intact.
5. **Driver indentation mirrors structure** — the one-level-per-proc/if/data-section
   convention is preserved for `c_*` calls too.

## 4. The register pool — the heart of allocation

Automatic storage is a 16-entry table of the general-purpose registers, each
carrying its four width-specific names and its ABI role:

```c
typedef struct {
    const char* names[4];   /* byte, word, dword, qword: e.g. {"al","ax","eax","rax"} */
    bool is_volatile;       /* Windows x64 caller-saved: rax, rcx, rdx, r8-r11 */
    bool reserved;          /* rsp/rbp: never handed out */
    bool in_use;            /* a live CValue currently owns it */
} CReg;
```

Two consequences of the Windows x64 ABI shape the allocator:

- **Width names.** A value's `CType.size` selects the sub-register (`C_INT` → `eax`,
  `C_PTR` → `rax`), so `c_cell` always hands `write_*` a correctly sized operand —
  the sub-register correctness that hand-written asm has to track manually.
- **Allocation order.** Registers are handed out volatile-scratch first
  (`rax`, `r8`–`r11`), then callee-saved (`rbx`, `r12`–`r15`), and the ABI
  argument / string-op registers (`rcx`, `rsi`, `rdi`, `rdx`) last. Handing out
  scratch first means short-lived temporaries never gratuitously occupy a
  callee-saved register, and leaving `rcx`/`rsi`/`rdi` for last keeps them free for
  instructions that *require* them (notably `rep movsb`), so `c_rep_movsb` can hide
  its fixed-register requirement without colliding with live values.

Each fresh binding is materialised through `write_init_mov` or `write_replace_mov`
(never `write_set_mov`) so the local dictionary entry is created or overwritten
with the new value's tag — the strict internal invariant that lets a register be
safely reused by a later variable.

## 5. Calls and clobbering

Volatility is a property of the calling convention, not of a register. Windows
x64 is the built-in shortcut: volatile `rax`, `rcx`, `rdx`, `r8`–`r11`; preserved
`rbx`, `rbp`, `rsi`, `rdi`, `r12`–`r15`. The `is_volatile` flags on the pool are
that shortcut, used by the allocator. The check itself is a preserved mask in
the asm layer.

`c_begin_function` opts into the Windows x64 preserved set and saves it. The asm
layer then errors if that procedure writes a preserved register it has not saved,
or calls a callee whose volatile mask overlaps an unsaved preserved register.
Raw `write_proc` does not opt in, so the asm demos are not failed for the same
patterns. `c_free`, and the pool reset when the procedure ends, is the disown
path: a register handed back is not in use.

Around a call, `c_call`, `c_call_ext`, and `c_call_proc` copy every named
in-use register in the callee's volatile mask to a temporary frame slot and
restore that same register afterwards. The `CValue` still names the register.
Eviction rewrites the tracked copy in `c_owned`. A value passed to `c_dispose`
is not part of that save. The result is copied out of the ABI return register
before the restore, into a hold register the save skipped. A value that cannot
be saved (spill off, no frame, or a register with no owner name) is poisoned.
A later read aborts. Cross-call state can still live in a `c_global`.

`c_call_proc` moves register arguments and does not store a stack argument.
With spilling on, `c_begin_function` names a fifth parameter
`qword ptr [rbp+48]`. `write_extern_call` accepts a stack cell only as
`qword ptr [rsp+40]`. Those are the two spellings of the same slot on the two
sides of `push rbp` / `mov rbp, rsp`, and one procedure cell cannot be both.
Section 4 of [`c_interface.md`](c_interface.md) is the full account. The
behavior program's fifth argument uses `write_proc` plus `c_call_ext`.

**Clobber safety net (generation-time check).** Silently reading a value that a
call destroyed is the single most common code-generation mistake at this layer.
To turn it from a runtime "garbage result" into a loud generation-time error, the
pool tracks a *clobber* state per register with **zero emitted footprint**:

- After every call, each caller-saved register that was in use and was not
  restored is marked *clobbered by* that callee. A restored register is not
  marked. The hold that received the result is not marked.
- Reading a value (any `c_cell`, and hence every compare / add / move / call
  argument built from one) whose register is still marked clobbered aborts with a
  precise message naming the value, its register, and the offending callee.
- Re-establishing a value into a register (`c_dup`, `c_local_imm`, `c_into`,
  `c_assign`, a spill reload) clears its clobber mark. So does the restore
  after the call.

The check is **flow-sensitive** so it has no false positives: the structured
control-flow helpers snapshot and merge the clobber state around each region, and
a `c_path_dead` flag (set by `c_ret`/`c_goto`/`c_break`) means a branch that
returns or jumps away contributes none of its clobbers to the join. A value is
considered clobbered at a join only if some *reachable* path destroyed it
("may-clobber" union). For example, in `WndProc` the `PostQuitMessage` call sits
on a branch that ends in `c_ret_zero`, so its clobber of `r9` (`lParam`) correctly
does **not** reach the fall-through `DefWindowProcA` call that reads `lParam` again.

## 6. Spilling

Spilling is on by default. When the pool is empty and another register is needed,
the allocator evicts a victim into the current frame and reloads it when the
value is used again. Slots are packed to the value's size inside a 256-byte
reservation addressed as `[rbp-n]`. `n` is the distance of the slot's lowest
byte from `rbp`, so a qword and a byte share the area without each taking a
full qword. Each activation record has its own `rbp`, so the generated program
is re-entrant.

`c_begin_function` opens that frame. The preserved set defaults to the Windows
x64 callee-saved registers. `c_set_preserved` replaces that mask for the next
function, and the function's volatile mask is the complement (every GPR except
`rsp` and the preserved set). `c_set_frame_align` selects 16, 64, or 4096 for
the next function. Both calls must happen before `c_begin_function`, because
the prologue is emitted once. `c_spill_off` and `c_spill_on` disable and
re-enable eviction locally. They do not remove a frame that is already open.
Building with `C_DISABLE_SPILL` omits the frame. `C_ENABLE_SPILL` is accepted
and changes nothing.

A 64-byte or 4096-byte frame cannot use `[rbp-n]`: `rbp` is only guaranteed
16-byte aligned. The prologue aligns `rsp`, keeps that base in `r12`, and
addresses slots as `[r12+n]`. `r12` stays reserved until the epilogue. The
reservation grows to 4096 bytes for a page-aligned frame.

A value created by `c_dup`, `c_local_imm`, `c_param`, or a call is a spill
victim. The caller's `CValue` is a copy. Eviction rewrites the tracked entry,
and `c_cell` reads that entry through the id, so the caller does not pass a
stable address. A value pinned with `c_into` is tracked and is not a victim. Call
save/restore puts the same register back. An unsaved value stays in its
register. If a call clobbers it, the value is poisoned and a later use aborts
generation; the call itself is not an error. There is no global spill array,
because the frame is the store.

`c_dispose` drops a value the generator is finished with. The register goes
back to the pool, a spill slot is released, and `asm_dispose_location` records
the location. A later read or set is a generation error, and a call does not
spill or restore that register. `c_mark_const` locks a value after its initial
store. `c_assign`, `c_assign_imm`, `c_add`, `c_sub`, `c_and`, `c_and_imm`, and
`c_zero` then abort. A read is still allowed. `c_main.c`'s `dispose_demo` does
both: `limit` is marked const after `c_local_imm`, and `scratch` is disposed
before the call to `dispose_sink`.

`c_spill` and `c_reload` force one value out and back. While a value is spilled,
its location is a memory operand, and `c_cell` still hands `write_*` a valid
cell. The frame utility itself lives in `asm_funcs.h` (`asm_frame_prologue`,
`asm_frame_slot`, `asm_frame_epilogue`) so a language wrapper can open the same
frame without going through `c_begin_function`.

## 7. Alignment and struct layout

The default is `ALIGN_NONE`. Fields are packed in declaration order, and padding
is explicit: a generator that wants a gap writes it as its own field.

Three controls select a mode:

- `align_set_default` sets the generation-time default.
- `c_struct_set_align` overrides one struct.
- `c_struct_field_mode` overrides one field.
- `c_stack_local(name, type, mode)` places an uninitialised local in the
  current frame. `ALIGN_NONE` packs to a byte. `ALIGN_C` aligns to the value's
  size. `ALIGN_CACHE` and `ALIGN_PAGE` align to 64 and 4096, and the function
  must have been opened with `c_set_frame_align` of that same mode. A slot
  alignment wider than the frame aborts generation.

The modes are:

- `ALIGN_NONE` — pack tight. A field may start at any byte.
- `ALIGN_C` — each field is aligned to its size, capped at 8. This is natural
  C alignment. It is required for Win32 interop. Aligned 1-, 2-, 4-, and
  8-byte accesses are atomic on x64.
- `ALIGN_CACHE` — like `ALIGN_C`, and the field is also placed so it does not
  straddle a 64-byte cache line. A split-line access is not atomic, and it is
  slow.
- `ALIGN_PAGE` — like `ALIGN_C`, and the field does not straddle a 4096-byte
  page. A split can fault mid-access.

All four are implemented once, as `align_pad` in `host.h`. `c_struct_field`
asks it how many bytes to insert, then drives `set_memory_layout_entry` with
the computed offset and size, so `write_data_memory_layout` and its validation
are reused. The field tag stays explicit and separate from the `CType`, which
supplies the size. A register-resident `CValue` has no address to align.

`c_main.c` calls `align_set_default(ALIGN_C)` because its `WNDCLASSA` and `MSG`
records are Win32 interop. A layout that leaves the default packs tight.

## 8. Boundaries

The layer is designed to be mixed freely with raw `asm_funcs.h` calls rather than
to replace them. Data sections are still opened with `write_data`/`write_dataq`/
`write_code`, string data is emitted with `write_initialised_data`, and call
arguments are ordinary asm operand cells (`literal(...)`, offset constants, or
`c_cell(local)`) — because the call boundary is exactly where `write_*_call`
performs its tag validation. `c_wrap` adopts an existing `AsmDataField` as a
`CValue`, and `c_cell` / `c_treat_as` go the other way, so a generator can move
between the value layer and the location layer at any point.

## 9. Production limits

The generator is single-threaded. Its buffers, dictionaries, and arena are
process-global; concurrent generation means separate processes. The bump arena
is fixed at 64 KiB and is never freed. The type dictionaries are fixed (64
local, 128 global), and the language wrappers' class tables are fixed caps.
Those are the remaining capacity limits.

The fifth-parameter mismatch in section 5 is a limit of the call helpers, not
of the frame. `c_struct_emit` overflow, a missing `c_field`, and `c_into` of
an unknown register end the process with `exit(1)`. The behavior hook does not
see them.

The operations with no behavior check are listed in section 11 of
[`c_interface.md`](c_interface.md): `c_rep_movsb`, `c_and_imm`, `c_relabel`,
`c_goto`, `c_loop_continue`, the compares and the nesting the fuzz does not
emit, calls other than the shapes that program uses, `c_string`, `c_global`,
`c_constant`, `c_byte_length`, `c_ptr_data`, struct alignment modes other than
the one struct the fuzz emits, unions, `c_field`, narrow returns,
`c_set_preserved`, `c_set_frame_align`, and a random spill pattern with more
than one victim. The emitter records one tag per location. Both arms of the
current tests write the same tag.
