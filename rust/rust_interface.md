# `rust_funcs.h` interface design

## Overview

`rust_funcs.h` is a **Rust-flavoured** code-generation layer over
[`c_funcs.h`](c_interface.md) / [`asm_funcs.h`](asm_interface.md). It reproduces
Rust's most distinctive properties: **ownership with move semantics checked at
generation time** (the borrow checker as a *build-time* error), **deterministic
`Drop` in LIFO order**, and **`Result<T, E>` with the `?` operator** for
error propagation. Emits no assembly of its own.

`rust_main.c` generates `rust_output_<option>.asm`, a console program that prints
a feature tour — including a **garbage-collection tour** driven by the selected
collector — and exits 0. The filename is suffixed with the active `-DGC_OPTION`;
`build.bat` builds and console-tests it across the collector options.

A Rust generator calls `rust_init()` once at the top of `main`, instead of poking the
assembler directly. It selects case-sensitive identifiers (Rust is case-sensitive;
Win32 externs are mixed-case). Rust has no class builder, so there is no builder
state to reset. Only `core/main.c` still calls
`write_set_case_sensitive_identifiers()` directly.

## The borrow checker, at generation time

The key idea: Rust rejects use-after-move **before the program runs**. This layer
models that in C, in the generator itself.

```c
typedef struct {
    const char* name;        /* variable name (for diagnostics)  */
    const char* obj_offset;  /* the value's storage              */
    const AsmExtern* drop;    /* its Drop::drop procedure         */
    bool moved;              /* true once moved out              */
} RustVal;
```

- `rust_let(scope, &v, name, storage, drop)` binds an owned value and registers
  it for `Drop`.
- `rust_use(&v)` asserts the value is still live; on a use-after-move it prints
  `error[E0382]: use of moved value` and **aborts code generation** (`exit(1)`),
  exactly as `rustc` refuses to compile. It returns the storage cell for use as a
  call argument.
- `rust_move(&src)` marks the source moved (any later `rust_use` aborts) and
  returns the new owner's storage cell.

So moving `buf` into `consume()` invalidates `buf`; uncommenting the
`rust_use(&buf)` after the move fails the **build**, not the run. This is the
generation-time analogue of the borrow checker.

## Deterministic `Drop`

`RustScope` records owned values in binding order. `rust_scope_end` walks them in
**reverse (LIFO)** and emits a `Drop::drop` call for each value **still owned**,
skipping moved-out values — so `buf` (moved into `consume`, dropped there) is not
dropped again at scope end, while `keep` is. No double free, mirroring Rust's
drop semantics.

## `Result` and the `?` operator

A fallible function leaves its `Result` discriminant in `rax` (`0 = Ok`,
`1 = Err`), produced with `rust_ok` / `rust_err`. `rust_ret_ok(f)` and
`rust_ret_err(f)` are those plus `c_ret(f)`, so a branch that only returns the
discriminant is one call. `rust_try` still reads `rax` after the callee
returns. The `?` operator is lowered by
a `RustCatch`: `rust_catch_begin` opens the guarded region, `rust_try(&catch)`
tests the discriminant and diverts to the handler on `Err`, and
`rust_catch` / `rust_catch_end` write the early-return handler:

```
cmp rax, 0
jne <catch>       ; propagate Err: early-return from the caller
                  ; fall through on Ok
```

`checked_div(10, 2)?` yields `Ok` and continues; `checked_div(1, 0)?` yields
`Err`, so `?` jumps to the early-return path — the caller returns without running
the rest of its body.

## Split: generation-time vs runtime

- **Move / borrow checking is generation-time** (a build error), matching Rust.
- **`Drop` order and `?` propagation are genuine runtime behaviour** (LIFO calls,
  a conditional branch).

## Garbage-collection tour

Alongside the ownership tour, the generator includes `gc.h`, declares a
`Node` GC type (`gc_typedesc`), opens the collector runtime with `gc_begin`, and
calls `gc_demo`: it allocates two nodes, links them, prints the addresses, drops
the roots, collects, and prints a third object. Those prints are not assertions.
On the conservative collector the first two stay reachable from callee-saved
registers, so the third is a fresh address. `gc_fuzz.h` then checks a three-node
graph. The collector is chosen at build time with `-DGC_OPTION`. What the graph
checks is in [`../gc/GC_imp.md`](../gc/GC_imp.md) and [`../gc/GC.md`](../gc/GC.md).
This is a heap allocator. The ownership tour's objects stay static globals.

## Simplifications versus real Rust

- **Ownership tracking is per-`RustVal` flag**, not a full lifetime/borrow
  analysis; it models moves and use-after-move, not references/aliasing rules.
- **Ownership-tour objects are globals**; `Drop` runs a per-value procedure. (The
  separate GC tour does use a real heap allocator.)
- **`Result` is a bare discriminant in `rax`**, not a generic tagged enum with a
  payload.

## Build

`build.bat` compiles `rust_main.c` to `rust_main.exe`, runs it to emit
`rust_output_<option>.asm`, assembles/links `rust_output_<option>.exe`
(`/subsystem:console`), and console-tests exit 0 plus the `[Rust] feature tour`
marker, repeating across the collector options selected by `-DGC_OPTION`.

The behavior program checks `Ok`, `Err`, and `Drop` of the value that was not
moved, and that a use of a moved value fails generation. It does not check
nested scopes, a move of one binding while another is still used, `?` on more
than one call, a drop that runs more than once, or a drop that does not run
when both values move. Those stay on the scripted tour, which checks the exit
code and the marker. [`../TEST.md`](../TEST.md) is the coverage report.
