# Garbage collection — design and trade-offs

The collectors are emitted as straight-line procedures over the asm and c
helpers. New collector code should use `c_funcs.h` (`c_begin_function`,
`c_call_ext`, `c_if`) rather than raw `write_*` calls.

This document explains the design space behind the four garbage collectors in
this project and why each works the way it does. For the concrete interface,
file layout, and build matrix, see [`GC_imp.md`](GC_imp.md); this document is the
"why" behind the "how".

The generated x64 programs manage heap memory with a running collector. There
are four strategies:

- **A — reference counting** (non-moving, eager)
- **B — mark-sweep** with optional free-list coalescing (non-moving)
- **C — compacting / moving** (Cheney semispace copy)
- **D — conservative scanning** (non-moving)

Everything the collectors need is emitted through the existing `write_*` / `c_*`
helpers; the collector *is* generated assembly, but almost every collector choice
imposes obligations on *how* code is generated — what metadata is emitted, what
invariants the register discipline keeps, and what barriers run at pointer writes
and calls. Those obligations are what distinguish the four options.

---

## 1. Machinery shared by every collector

Four pieces underlie every option, regardless of marking or compaction strategy.

### 1.1 A real heap allocator

The heap is a single `VirtualAlloc` reservation (1 MB), obtained through the
`c_extern` mechanism. Objects are allocated by an emitted `gc_alloc(size,
descOffset) -> ptr` procedure — a bump allocator for the moving collector, a
first-fit free-list allocator (in `gc_common.h`) for the non-moving collectors.
Every object carries a 16-byte header the collector owns: a **type-descriptor
pointer** at `[obj+0]` and a **meta word** at `[obj+8]` used as a refcount, a mark
bit, or a forwarding pointer depending on the collector.

### 1.2 Per-type pointer maps (GC descriptors)

A precise collector must know which of an object's fields are pointers. Each type
emits a **GC descriptor** beside its other tables:

```
Node_gcdesc:
    dq  <object size in bytes>
    dq  <number of pointer fields>
    dq  <offset of pointer field 0>
    ...
```

The generator has this information at layout time — it knows each field's type and
therefore which fields are pointers. Mark-sweep, the compactor, the refcount
cascade free, and the conservative mark all read this map. Conservative
collection uses it only after a scanned word has been confirmed as an object
base. The stack scan itself does not consult it. `GC_SCAN=full` can keep a
child without that walk, which is why that flag build is not a proof of the map.

### 1.3 Root identification

Roots are the pointers *outside* the heap that keep objects alive — globals, stack
slots, and registers. There are two ways to find them, and the choice is the
single biggest fork in the design:

- **Precise roots** (used by mark-sweep and compaction). The mutator keeps managed
  pointers in known homes and tells the collector where they are. This project
  uses a **root shadow stack** rather than return-address-keyed stack maps: a
  runtime array the collector walks, holding either the live pointer (mark-sweep)
  or the address of the memory slot holding it (compaction). It is a precise,
  complete technique — used by real managed runtimes and LLVM's shadow-stack GC —
  and, crucially, it lets a moving collector *rewrite* each root without parsing
  native frames.
- **Conservative roots** (Option D). No metadata at all: at collection time the
  collector scans the stack and saved registers and treats any word that *looks
  like* a heap pointer as a root. Simple, at the cost of precision.

### 1.4 Safepoints and register spilling

For a precise collector, a collection may run only where the live-pointer picture
is valid — a **safepoint**, in practice every allocation. At a safepoint every
live pointer must be findable, which collides with a register allocator that keeps
pointers in volatile registers a call may clobber. The project addresses this two
ways:

- The collectors keep managed pointers in **callee-saved registers** (or, for the
  compactor, in memory **slots**), which the collector routines are written never
  to disturb. This is the discipline the tours actually use.
- `c_funcs.h` spills by default, into packed slots inside a 256-byte frame
  (`[rbp-n]`, or `[r12+n]` when the frame is aligned to 64 or 4096). Each
  activation has its own slots. A value created by `c_dup`, `c_local_imm`,
  `c_param`, or a call is tracked, and eviction rewrites that tracked copy.
  Calls save and restore named registers separately. A value passed to
  `c_dispose` is left out of that save. The collectors here do not use
  that spiller; they keep their own register and slot discipline. See
  [`c_architecture.md`](../c/c_architecture.md).

The recurring theme below: **the more precisely and mutably a collector must locate
live pointers, the more discipline it demands** — which is exactly why reference
counting and conservative scanning are the low-friction options and compaction the
most demanding.

---

## 2. Option A — Reference counting

**Idea.** Each object header holds a count of live references. A write barrier
increments the count on every pointer copy and decrements it on every overwrite or
scope exit; the object is freed when the count reaches zero.

**How it works here.** `gc_new` seeds the count to 1. `gc_set` / `gc_assign` are
the write barrier: storing a pointer increfs the new referent and decrefs the one
it replaces. When a count hits zero the object is freed *immediately* — and its own
pointer fields are decref'd first, so freeing the head of a list or tree cascades
and reclaims the whole structure eagerly. Freed blocks go on the shared free list,
so the next `gc_new` can take one. The tour prints the addresses it allocated. It
does not assert that the third object reused a freed block. The check `build.bat`
runs is the three-node graph in `gc_fuzz.h`, which keeps one root live and checks
the payload. `gc_scope_end` decrefs a scope's roots in LIFO order, mirroring Rust's
`Drop`.

**Cycles.** Pure counting leaks cycles (`A -> B -> A` never reaches zero).
`GC_CYCLES=weak` (the default) provides `gc_weak_*`, which copy a pointer and do
not incref, so a caller can leave one edge of a cycle uncounted. The tours and
the graph never call `gc_weak_*`. `GC_CYCLES=trial` selects the same
`gc_collect`: an empty `ret`. The trial name is a backup trace this collector
does not run. Mark-sweep is the collector that traces. The trial build in
`build.bat` checks that the program still exits 0. It does not show a cycle
reclaimed by tracing.

**Trade-offs.** Barriers are local and cheap to emit, and reclamation is prompt and
deterministic — the cheapest precise option. The costs are a barrier on every
pointer write, a potentially long pause when a large subgraph's count hits zero,
and the cycle problem. Non-moving, so it does not defragment.

---

## 3. Option B — Mark-sweep with free-list coalescing

**Idea.** No per-write barriers. Periodically stop, **mark** everything reachable
from the roots, then **sweep** the heap, freeing unmarked objects onto a free list.

**How it works here.** The allocator records every object it hands out. `gc_collect`
marks from the shadow-stack roots — a precise traversal that follows each object's
GC-descriptor pointer map — then sweeps the recorded objects, returning unmarked
ones to the free list and clearing the mark bit on survivors. With `GC_COALESCE`
set, the sweep also merges physically adjacent free blocks (boundary-tag
coalescing) so a later large allocation can reuse the combined hole. That merge
is in the emitted collector. The tour and `gc_fuzz.h` do not check that two
neighbours became one block, and they do not check that the next object reused
the lower address.

**Trade-offs.** Marking is precise and cycle-safe, and there are no per-store
barriers. Because the collector is **non-moving**, pointers only need to be *found*,
not *rewritten* — a real simplification versus compaction. The costs are residual
fragmentation (coalescing mitigates but cannot eliminate it), slower free-list
allocation than a bump pointer, and pauses that scale with the live set and heap
size.

---

## 4. Option C — Compacting / moving collector

**Idea.** After identifying the live objects, **relocate** them so they are
contiguous (eliminating fragmentation and enabling fast bump allocation), then
**rewrite every pointer** to the objects' new addresses. This is the C#/CLR
compacting model.

**How it works here.** The heap is two equal halves; objects are bump-allocated
into the current "from" half. `gc_collect` runs Cheney's algorithm: it copies each
live object into the "to" half, packed tightly, then swaps the halves — reclaiming
garbage and defragmenting in one pass. When an object is copied, its old meta word
is overwritten with the new address (a **forwarding pointer**), so any later
reference to the old copy is detected and redirected exactly once; shared structure
stays shared and cycles terminate.

Because objects move, a pointer sitting only in a register cannot be fixed up, so
the compactor uses the **root-slot discipline**: every managed local lives in a
memory slot (`gc_slots[]`), `gc_root` registers the slot's *address* on the shadow
stack, the collector rewrites `*slot` to the new home, and `gc_load_ptr` reloads
the pointer on each use. The tour prints the addresses of `a` and `b`. A copying
collect moves a rooted object; the graph in `gc_fuzz.h` is what checks that the
rooted address changed. The `GC_MOVE` sub-flag names the strategy. `copying` is
the Cheney evacuation this collector emits. `sliding` (Lisp2 mark-compact in
place) is ignored: the flag build is still the copying collector. It is described
here as the classic alternative.

**Trade-offs.** Zero fragmentation, O(1) bump allocation, and cache-friendly layout
— the reasons production runtimes choose it — at the price of the most demanding
mutator discipline: every live pointer must have a canonical, updatable home.
Holding a buffer still across a foreign call would mean a pin. `gc_pin` and
`gc_unpin` are no-ops on this collector and on the other three. No header
defines `GC_HAS_gc_pin`.

---

## 5. Option D — Conservative scanning

**Idea.** Emit no root metadata at all. At collection time, scan the machine stack,
the saved registers, and (optionally) the heap itself, treating any word whose
value falls inside the heap and looks like a valid object address as a potential
root. This is the Boehm–Demers–Weiser model used to retrofit GC onto C/C++.

**How it works here.** The allocator records objects as in mark-sweep. `gc_collect`
spills the callee-saved registers, scans the stack (and, with `GC_SCAN=full`, the
heap too), marks conservatively anything that looks like a live object, and sweeps
the rest. `gc_root` is a no-op — roots are discovered by scanning, not declared.

**False retention and the moving constraint.** A word that is really an integer,
float, or string byte-pattern but coincidentally equals a heap address will
**falsely retain** whatever it "points" to. This is harmless to correctness but
bad for footprint, and it is exactly why the collector **must not move** anything:
rewriting a value that turns out to be a coincidental integer would corrupt it. So
conservative scanning is inherently non-moving. The tour makes this visible: `a`
and `b` remain reachable through the registers still holding them, so the scan
legitimately retains them and `c` gets fresh space — the honest behaviour of a
conservative collector, documented in `gc_conservative.h` rather than hidden.
`GC_SCAN=full` also scans the heap, so a pointer sitting in a field keeps the
child even when the descriptor walk is broken. That flag build runs the same
graph. It does not add a separate proof of the descriptor walk.

(Aside: this technique is sometimes attributed to JavaScript, but modern JS engines
are *precise* and *do* move objects; conservative scanning is the C/C++ approach,
with some engines historically scanning only native stack frames conservatively.)

---

## 6. The strategies compared

| Strategy | How live objects are identified | Metadata required | Moving? | Cycle-safe? |
| --- | --- | --- | --- | --- |
| **Reference counting** (A) | Per-object count of incoming refs; free at zero | Per-store incref/decref barriers + scope-exit decref | No | No. `gc_weak_*` can leave one edge uncounted. `gc_collect` does not trace. |
| **Tracing from roots** (B, C) | DFS/BFS from precise shadow-stack roots via per-type pointer maps | GC descriptors + shadow stack | Yes, with pointer rewriting (C) | Yes |
| **Conservative tracing** (D) | Trace from *ambiguous* roots (anything that looks like a pointer) | Almost none (heap bounds + headers) | No | Yes |

Tracing-from-roots underlies both B and C; the difference between them is not the
marking (both mark identically) but whether the sweep **relocates**.

---

## 7. Summary

- **Conservative scanning (D)** is the simplest — no root metadata, tolerant of the
  existing register model — but cannot compact and suffers false retention.
- **Reference counting (A)** is the cheapest *precise* option: local per-store
  barriers and deterministic, prompt reclamation. `gc_weak_*` is implemented and
  unused by the tours. `gc_collect` does not trace, under `weak` or under `trial`.
- **Mark-sweep with coalescing (B)** adds precise tracing. With `GC_COALESCE`
  the sweep merges adjacent free blocks. That merge is emitted and not checked
  by the tour. Being non-moving, the collector only needs to *find* pointers.
- **Compacting / moving GC (C)** delivers zero fragmentation and O(1) allocation at
  the cost of the strictest discipline: every live pointer has an updatable home,
  and the collector rewrites them after each move. Pinning is not implemented.
  `GC_MOVE=sliding` is ignored.

The decisive variable across all four is how precisely and mutably the collector
must locate live pointers at a safepoint — which is why the options range from
"almost no mutator obligations" (conservative) to "every pointer must be findable
and rewritable" (compacting).

---

## 8. What the build checks

`build.bat` assembles each language tour once per collector and runs it. The
scripted half of `gc_demo` prints addresses and must exit 0. It does not assert
those addresses. On the conservative collector a live callee-saved register is a
root, so the third demo object is a fresh address. The shared print line still
describes a reuse; that text is not an assertion.

`gc/gc_fuzz.h` appends one three-node graph to that demo. `n0` is the only root.
After `gc_collect`, the integer payloads and the pointer links are still there.
Compact also checks that the rooted address moved and that `n0.next` is not
`n1`'s stale slot. Failure is `ExitProcess` of a code from 21 through 31, listed
in [`../TEST.md`](../TEST.md). These are the tour's exit codes.

Still absent from that graph, and from the tours:

- Weak refs. `gc_weak_new` / `gc_weak_get` exist and are never called.
- Pins. `gc_pin` / `gc_unpin` are no-ops on every collector.
- Heap exhaustion.
- A graph bigger than three nodes. The C# tour holds four callee-saved registers
  until the demo releases `a`, `b`, and `c`, which leaves three.
- An assertion that an unreachable object was freed. The graph checks what a
  live root still reaches.
- Coalescing of adjacent free blocks. The mark-sweep code merges them when
  `GC_COALESCE` is set. The build does not check the merge.
- Sliding compaction. `GC_MOVE=sliding` is ignored.
- A trial trace. `GC_CYCLES=trial` leaves `gc_collect` empty.
- A proof that `GC_SCAN=full` walked descriptors. The full scan keeps a child
  whose address appears anywhere in the heap.

The four extra Python builds (`GC_CYCLES=trial`, `GC_COALESCE`,
`GC_MOVE=sliding`, `GC_SCAN=full`) are exit-0 runs of the same tour. They show
that the flag compiles and the program still passes the graph. They do not show
the sub-flag doing anything beyond the default collector.
