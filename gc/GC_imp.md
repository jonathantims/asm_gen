# GC_imp.md — The swappable garbage collectors

This document describes the garbage-collector layer as implemented: a family of
four collectors that share one `gc_*` interface, so any of the language tours
(`csharp`, `rust`, `py`, `js`) compiles unchanged against whichever collector is
selected on the command line. For the design rationale and the trade-offs behind
each collector, see [`GC.md`](GC.md).

The four collectors are:

| Option | Header               | Strategy                                   | Moving? |
|--------|----------------------|--------------------------------------------|---------|
| **A**  | `gc_refcount.h`      | Reference counting (eager, cascading free) | no      |
| **B**  | `gc_marksweep.h`     | Mark-sweep, optional free-list coalescing  | no      |
| **C**  | `gc_compact.h`       | Compacting / moving (Cheney semispace copy)| yes     |
| **D**  | `gc_conservative.h`  | Conservative stack (+ optional heap) scan  | no      |

Every collector emits its allocator and collector routines as x64 MASM through
the existing `write_*` / `c_*` emitters — there is no hand-written assembly and no
raw-text output. Each runs on a real `VirtualAlloc` heap, owns real object
headers, executes real barriers, and actually reclaims (or, for Option C,
relocates and fixes up) memory at run time.

---

## 1. Architecture

```
              asm_funcs.h   (MASM x64 emitter, type dictionaries)
                   |
               c_funcs.h    (typed values, register pool; optional C_ENABLE_SPILL)
                   |
               gc.h         (include switch; the generator includes this once)
                   |
             gc_common.h    (shared heap prelude, object layout, typedesc, emit helpers)
                   |
gc_refcount.h  gc_marksweep.h  gc_compact.h  gc_conservative.h   (exactly one)
                   |
             gc_defaults.h  (fallback definition of every interface function)
                   |
             gc_fuzz.h
                   |
             gc_demo.h
                   |
     +-------------+-------------+-------------+
     |             |             |             |
 py_funcs.h   csharp_funcs.h  rust_funcs.h  js_funcs.h    (console / object-shape wrappers)
     |             |             |             |
 py_main.c    csharp_main.c   rust_main.c   js_main.c     (tours; each includes a GC section)
```

- **`gc.h`** is the include switch, and the one collector header a generator
  includes. It reads `-DGC_OPTION=...` and includes, in order: `gc_common.h`,
  the selected option header, `gc_defaults.h`, `gc_fuzz.h`, and `gc_demo.h`.
- **`gc_common.h`** holds everything option-independent: the `VirtualAlloc` heap
  prelude, the uniform object-header layout, `gc_typedesc` descriptor emission,
  the free-list allocator, the hex-address printer, and the managed-pointer
  helpers.
- **The option header** defines the interface functions it changes and, for each,
  does `#define GC_HAS_<name>`. A collector that supplies its own allocator does
  `#define GC_CUSTOM_ALLOC`.
- **`gc_defaults.h`** is included after the option header and provides every interface function the
  option did *not* override, guarded by `#ifndef GC_HAS_<name>`. This is the
  mechanism that lets one tour body compile against any collector: unoverridden
  operations fall back to a plain-move / no-op default.
- The language `*_funcs.h` wrappers are reused as-is for console output and for
  describing each object's field layout (which fields are pointers, fed to
  `gc_typedesc`).

### 1.1 Include-order rule

`gc_common.h` establishes shared macros and helpers; the option header (included
second) may *add* overrides but cannot un-define what common set; `gc_defaults.h`
(last) fills gaps only `#ifndef GC_HAS_x` / `#ifndef GC_CUSTOM_ALLOC`. Anything
that must be uniform across options (e.g. `gc_emit_start`, which initialises the
tospace midpoint that only the compactor uses) lives in `gc_common.h`.

---

## 2. Selecting a collector and its variant

The collector is chosen with a compiler macro:

```
cl /DGC_OPTION=refcount     ...   -> Option A
cl /DGC_OPTION=marksweep    ...   -> Option B
cl /DGC_OPTION=compact      ...   -> Option C
cl /DGC_OPTION=conservative ...   -> Option D
```

Within an option, a second macro selects the intra-option behaviour. The four
implemented sub-flags are:

| Option | Flag          | Values (default first) | Effect |
|--------|---------------|------------------------|--------|
| A      | `GC_CYCLES`   | `weak` \| `trial`      | `weak`: `gc_weak_*` copies the pointer and does not incref. The tours never call it. `trial`: `gc_collect` is an empty `ret`, the same procedure `weak` emits. The trial name is a backup trace this collector does not run. The flag build exits 0. It does not reclaim a cycle by tracing. |
| B      | `GC_COALESCE` | unset \| set           | When set, the sweep merges physically adjacent free blocks (boundary-tag coalescing). The merge is emitted. The tour and `gc_fuzz.h` do not check that neighbours became one block. |
| C      | `GC_MOVE`     | `copying` \| `sliding` | `copying` (implemented): Cheney two-semispace evacuation. `sliding` is accepted and ignored. That flag build is still the copying collector. `GC.md` describes sliding as the classic in-place alternative. |
| D      | `GC_SCAN`     | `stack` \| `full`      | `stack`: scan the stack and spilled registers conservatively, then trace the heap from confirmed objects. `full`: also scan the heap conservatively, so a pointer sitting in any heap word keeps the child. Both scans are emitted. The full-scan build does not by itself prove the descriptor walk. |

All flags have safe defaults, so `-DGC_OPTION=compact` alone builds a working
copying collector.

---

## 3. The common interface

Drivers use only these functions; every collector exposes the identical names.
Signatures (as implemented in `gc_common.h` / `gc_defaults.h` and the option
headers):

### 3.1 Setup

```c
void   gc_externs(void);          /* declare VirtualAlloc; call before the data section   */
void   gc_data_globals(void);     /* emit heap cursor/limit, free list, roots/slots        */
GcHeap gc_begin(AsmExtern print); /* emit the heap prelude + allocator + collector + hex    */
```

`GcHeap` bundles the emitted procedures the generator later calls:

```c
typedef struct {
    AsmExtern start;    /* VirtualAlloc the heap, capture the stack base   */
    AsmExtern alloc;    /* gc_alloc:  rcx=size, rdx=descOffset -> rax=obj   */
    AsmExtern collect;  /* run the collector                               */
    AsmExtern hex;      /* print "0x................" for rcx               */
} GcHeap;
```

### 3.2 Type descriptors (per-type pointer maps)

```c
GcType gc_typedesc(const char* name, int size, const int* ptr_offsets, int n_ptrs);
```

Emits a `<name>_gcdesc` global — `{ size, n_ptrs, off0, off1, ... }` — and returns
a handle carrying the size and pointer-offset base. The pointer offsets come from
the language wrapper, which already knows each field's type. The precise
collectors (B, C) read this map to trace an object's outgoing pointers; the
conservative collector (D) does not need it.

### 3.3 Allocation

```c
CValue gc_new(GcHeap* h, const GcType* t, const char* name);
```

Calls `gc_alloc(size, descOffset)` and binds the result to a managed-pointer
`CValue`. The refcount collector additionally seeds the header count to 1.

### 3.4 Field access

```c
CValue gc_get(CValue obj, int off, CType type, const char* name);   /* load field */
void   gc_set(GcHeap* h, CValue obj, int off, CValue val);          /* store field */
```

`gc_set` inserts the option's write barrier when it needs one (refcount:
incref the new referent, decref the replaced one; others: plain store).

### 3.5 Roots and scopes

```c
GcScope gc_scope_begin(void);
void    gc_root(GcScope* s, CValue* v);
void    gc_scope_end(GcHeap* h, GcScope* s);
```

Root handling differs per collector — see [§5](#5-root-discipline).

### 3.6 Pointer-variable assignment

```c
void gc_assign(GcHeap* h, CValue* dst, CValue src);   /* refcount: incref(src);decref(old); others: plain move */
```

### 3.7 Collection, pinning, weak references

```c
void   gc_collect(GcHeap* h);
void   gc_pin(CValue* v);      /* no-op on every collector, including compact */
void   gc_unpin(CValue* v);
CValue gc_weak_new(GcHeap* h, CValue target, const char* name);   /* copy, no incref; tours never call it */
CValue gc_weak_get(GcHeap* h, CValue weak, const char* name);
```

Any function an option does not override is provided by `gc_defaults.h` as a
plain-move or no-op default, so a tour that calls `gc_pin` compiles under every
collector. No collector defines `GC_HAS_gc_pin`, so the call emits nothing.
Refcount defines `GC_HAS_gc_weak`. The other collectors get the default, which
copies the pointer into a callee-saved register and does not incref. Nothing in
the tours calls either function.

---

## 4. What each collector implements

| Interface fn        | A refcount                                   | B mark-sweep                              | C compact                                        | D conservative                              |
|---------------------|----------------------------------------------|-------------------------------------------|--------------------------------------------------|---------------------------------------------|
| allocator           | shared free-list `gc_alloc`                  | free-list `gc_alloc` recording allobjs    | bump allocator into the current half             | free-list `gc_alloc` recording allobjs      |
| `gc_typedesc`       | shared `_gcdesc`; the cascade free walks it; meta word is the count | shared `_gcdesc` precise pointer map | shared `_gcdesc`; meta word doubles as forwarding | shared `_gcdesc`. Roots are a scan; the mark walk follows the map |
| `gc_new`            | seed count = 1                               | plain alloc                               | claim a `gc_slots[]` slot; bump alloc            | plain alloc                                 |
| `gc_set` / `gc_assign` | incref/decref write barrier               | plain store                               | plain store                                      | plain store                                 |
| `gc_root`/`scope_end`| decref each root at scope end (LIFO)        | push value on the root shadow stack       | push the slot *address* on the shadow stack      | no-op (roots found by scanning)             |
| `gc_collect`        | empty `ret` (immediate free is the decref path; `trial` does not trace) | mark from roots; sweep (coalesce merges adjacent free blocks when `GC_COALESCE` is set; the build does not check the merge) | Cheney copy live objects; swap halves; fix roots (`sliding` is ignored) | scan roots conservatively; mark; sweep (`full` also scans the heap) |
| `gc_pin` / `gc_unpin`| no-op                                       | no-op                                     | no-op                                            | no-op                                       |

The heavy runtime bodies (`mark`, `sweep`, `gc_coalesce`, `incref`/`decref`,
`gc_forward`, the conservative scan) are all emitted by the option header via
ordinary `write_*` / `c_*` calls.

### 4.1 Collector register safety

Collector and mark routines use only volatile registers (`rax`, `rcx`, `rdx`,
`r8`–`r11`) so they never disturb managed pointers held in callee-saved registers.
The compactor's copy loop and the conservative scan explicitly push/pop `rsi`/`rdi`
(and the conservative collector pushes all callee-saved registers to expose
register-held roots to its scan). Collector internals use a private calling
convention (no shadow space, no saved nonvolatiles) via
`write_proc(name, NULL, 0, asm_void)`.

---

## 5. Root discipline

The mutator must let the collector find (and, when moving, rewrite) every live
pointer. Each collector uses the discipline that fits it:

- **Refcount (A).** `gc_root` records the managed local; `gc_scope_end` decrefs
  each in LIFO order, exactly mirroring Rust's `Drop`-at-scope. No tracing needed.
- **Mark-sweep and conservative (B, D).** Managed pointers live in **callee-saved
  registers** (`gc_pick_saved` hands them out in the order `rbx, r12, r13, r14,
  r15, rsi, rdi`), which the collector routines never clobber. Mark-sweep's
  `gc_root` spills the value onto a runtime **root shadow stack** (`gc_roots`) the
  collector walks; conservative's `gc_root` is a no-op because it discovers roots
  by scanning the stack and spilled registers.
- **Compact (C).** Because objects move, a pointer sitting only in a register
  cannot be fixed up. The compactor keeps every managed local in a memory **slot**
  (`gc_slots[]`); `gc_root` pushes the slot's *address* onto the shadow stack; the
  collector rewrites `*slot` to the object's new home; and `gc_load_ptr` reloads
  the pointer from the slot on each use.

The single helper `gc_load_ptr(obj, scratch)` returns the pointer directly when it
is in a register (non-moving options) or reloads it from its slot (compact), so
the one shared tour body works with both disciplines.

---

## 6. Object and heap layout

Every object has a uniform 16-byte header followed by its payload:

| Offset      | Contents                                                        |
|-------------|-----------------------------------------------------------------|
| `[obj+0]`   | descriptor pointer (`<Type>_gcdesc`)                            |
| `[obj+8]`   | meta word: refcount (A) / mark bit (B, D) / forwarding ptr (C)  |
| `[obj+16]`  | first payload field                                             |

The descriptor `<Type>_gcdesc` is `dq size, dq n_ptrs, dq off0, off1, ...`. Free
blocks repurpose the first two words as `{ next, size }`, a layout shared by the
refcount free path and the mark-sweep / conservative sweeps.

The heap is a single `VirtualAlloc` reservation of `100000h` (1 MB) bytes. The
compactor treats it as two `80000h` (512 KB) halves and bump-allocates into the
current one; `gc_emit_start` initialises the tospace midpoint (a no-op for the
other collectors).

---

## 7. Lower layer: spilling

Spilling is on by default in `c_funcs.h`. `C_DISABLE_SPILL` omits the frame.
`C_ENABLE_SPILL` is accepted and changes nothing; `build.bat` still passes it
when it compiles `py_main`. Spilling stores a register-resident value out to a
frame slot to free the register, and reloads it later. The slots are packed
inside the function's own frame (256 bytes at `[rbp-n]`, or an `r12` base when
the frame alignment is 64 or 4096), so the generated program is re-entrant.
There is no global spill array. `c_begin_function` opens the frame, and a
value created by `c_dup`, `c_local_imm`, `c_param`, or a call is already
tracked. `c_spill_area` and `c_spillable` were empty and have been removed.

```c
c_spill(&v);      /* store v out to a slot now, freeing its register */
c_reload(&v);     /* bring a spilled v back into a register */
c_alloc_reg();    /* when the pool is empty, spills a victim instead of aborting */
c_dispose(&v);    /* drop it: a call does not spill or restore it */
c_mark_const(&v); /* a later set is a generation error */
```

The collectors emit their runtime as raw `write_*` assembly that manages its
own registers, so they do not use this spiller. Eviction rewrites the tracked
copy in `c_owned`. A call does not evict: it saves the register and restores
the same register. A value passed to `c_dispose` is left out of that save.
The full account is in [`../c/c_architecture.md`](../c/c_architecture.md).

---

## 8. Per-language integration

The GC tour is a section inside each existing `py/csharp/rust/js_main.c`, not a
separate generator. Because it uses only the `gc_*` interface, the same source
compiles under every collector; `build.bat` compiles it four times with different
`-DGC_OPTION`. `cpp` has no GC section.

The shared tour body lives in `gc_demo.h`. It allocates `a` and `b`, links
`a.next = b`, prints their addresses, drops the roots, collects, allocates `c`,
prints `c`, then runs the graph in `gc_fuzz.h`. The printed lines are not
assertions. The shared reuse sentence in that print is text, not a check. What
each collector does with `a` and `b` after the roots are dropped:

- **refcount / mark-sweep**: dropping the roots and collecting frees `a` and `b`,
  so a later `gc_new` may take a block from the free list. The print does not
  check that `c`'s address is one of those blocks. With `GC_COALESCE` the sweep
  can merge `a` and `b`. The build does not check that merge.
- **compact**: a rooted object is copied into the other half. The scripted print
  does not check the new address. `gc_fuzz.h` does, for its own root.
- **conservative**: `a` and `b` remain reachable through the callee-saved
  registers that still hold them, so `c` is a fresh address. That is the
  retention this collector is specified to do, documented in
  `gc_conservative.h`.

Each tour keeps its language-feature banner (e.g. `[Python] feature tour`) as the
test marker and prints the GC lines after it. Each emits an option-suffixed
output file, `<lang>_output_<option>.asm` → `<lang>_output_<option>.exe`.

A tour wires the collector in like this (Python shown):

```c
#include "host.h"
#include "asm.h"
#include "asm_funcs.h"
#include "c_funcs.h"
#include "gc.h"                            /* common, one collector, defaults, fuzz, demo */
#include "py_funcs.h"

    gc_externs();                         /* before the data section */
    GcTypeDesc nodeDesc = gc_type_begin("Node");   /* declare the GC type by fields */
        gc_field_ptr(&nodeDesc, "next");           /* an 8-byte managed pointer   */
        gc_field_word(&nodeDesc, "value");         /* an 8-byte scalar            */
    GcType Node = gc_type_end(&nodeDesc);          /* header size computed for you */
    ...
    gc_data_globals();                    /* in the data section */
    ...
    GcHeap heap = gc_begin(print);        /* in the code section  */
    ...
    gc_demo(&print, &heap, &Node, ...);   /* inside main */
```

`gc_type_begin` / `gc_field_ptr` / `gc_field_word` / `gc_type_end` build the
descriptor field-by-field and compute the object size (including the uniform
`GC_HDR` header) internally, so the generator never writes `GC_HDR + 16` or a raw
pointer-offset array. `gc_msg_of(str)` similarly wraps a wrapper `Str` handle for
the `gc_demo` message arguments, hiding the `{off, len}` split.

---

## 9. Build matrix

`build.bat` builds the full 16-cell matrix — each GC language
`{csharp, rust, py, js}` × each option `{refcount, marksweep, compact,
conservative}`:

```
cl /DGC_OPTION=<opt> <lang>\<lang>_main.c   (emits <lang>_output_<opt>.asm)
ml64 /c <lang>_output_<opt>.asm
link  ... /OUT:<lang>_output_<opt>.exe
test.exe --console <lang>_output_<opt>.exe "[<Lang>] feature tour"
```

In addition it runs one **flag spot-check** per option
(`refcount`+`GC_CYCLES=trial`, `marksweep`+`GC_COALESCE`, `compact`+`GC_MOVE=sliding`,
`conservative`+`GC_SCAN=full`) and a `C_ENABLE_SPILL` compile of `py_main`, which
accepts the flag and leaves spilling on. The `cpp` tour
and the base `output` / `c_output` builds run first. Every cell must assemble,
link, run, exit 0, and print its marker line. The spot-checks are that exit-0
run. They do not show a trial trace, a neighbour merge, a slide, or a descriptor
walk that the full scan uniquely proved. [`GC.md`](GC.md) section 8 lists what
the graph checks and what it leaves out: weak refs, pins, heap exhaustion, a
graph bigger than three nodes, an assertion that an unreachable object was
freed, and the four sub-flag behaviours above. `gc_pick_saved` still ends the
process with `exit(1)` when every callee-saved register is taken. That path is
outside `host_fail`.
