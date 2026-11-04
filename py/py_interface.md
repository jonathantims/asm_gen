# `py_funcs.h` interface design

## Overview

`py_funcs.h` is a **Python-flavoured** code-generation layer over
[`c_funcs.h`](c_interface.md) / [`asm_funcs.h`](asm_interface.md). It reproduces
CPython's defining runtime behaviours — **reference counting** with deallocation
at zero, **duck typing** (dynamic dispatch by attribute lookup), and **EAFP with
a runtime `TypeError`** on a missing attribute. The teaching point versus
Rust/C++: Python performs essentially **no static checking**; everything resolves
at run time. Emits no assembly of its own.

EAFP is Easier to Ask for Forgiveness than Permission. The checked call looks
before it uses a field; the required call attempts the use and fails if the
field is absent.

`py_main.c` generates `py_output_<option>.asm`, a console program that prints a
feature tour — including a **garbage-collection tour** driven by the selected
collector — and exits 0. The filename is suffixed with the active `-DGC_OPTION`;
`build.bat` builds and console-tests it across the collector options.

## Object model

Every object begins with a refcount and a pointer to its type's method table:

```
[obj + 0]  refcount
[obj + 8]  type method table pointer
```

A **method table** is a qword array of function pointers, one per attribute slot;
`0` means the attribute is absent on that type. The generator *describes* a class
with the **class-definition builder** (below), which interns each attribute name
to a fixed slot and, at `py_class_end`, bakes the table padded to a fixed width so
every type agrees on the slot numbering:

```
Dog_table    dq offset Dog_speak   ; slot 0 = speak
             dq 0                   ; slot 1 = quack (absent)
Duck_table   dq offset Duck_speak
             dq offset Duck_quack   ; Duck has quack
```

## Class-definition builder

A `class` is *described* between `py_class_begin("Dog")` and `py_class_end()`;
declare each method with `py_method_begin(name)` (implicit `self` = parameter 0,
closed with `py_method_end`). The builder (state in `py_funcs.h`) interns each
method name through a **global attribute registry** so a given name always maps to
the same table slot across all types, and `py_class_end` bakes `Name_table`
padded to `PY_MAX_ATTRS` slots (absent attributes are `0`). Padding to a fixed
width makes bake order irrelevant *and* makes any attribute safe to probe (EAFP).
`py_self()` yields the receiver inside a body. `py_field(name)` declares an
instance field (stored after the 16-byte `[refcount][typetable]` header).
`py_construct(&Class, name)` bakes an object (`refcount = 1`, table pointer, then
one zero qword per field) and **returns a `PyObject` handle**. A method is called
on that handle with `py_send(&method, obj)`, which supplies the implicit `self`;
inside a body `py_this_field(name, type)` addresses `self.field`. The `Counter`
demo mutates `self.n` across two `bump()` calls and `report()` reads it back.

## Initialisation

A Python generator calls `py_init()` once at the top of `main`, instead of poking the
assembler directly. It selects case-sensitive identifiers and resets the
class-builder and attribute-intern state. Only `core/main.c` still calls
`write_set_case_sensitive_identifiers()` directly.

## Features exhibited

### 1. Reference counting

`py_emit_refcount_ops` emits `py_incref` / `py_decref`. `incref` bumps
`[obj + 0]`; `decref` decrements it and branches on the result:

```
mov rax, [obj]
sub rax, 1
mov [obj], rax
cmp rax, 0
jne alive          ; still referenced
                   ; fall through: rax == 0 -> deallocate (__del__)
```

An object at refcount 1 → `incref` → 2 → `decref` → 1 (alive) → `decref` → 0
(deallocated). The alive-vs-freed decision is a genuine runtime branch on the
count.

### 2. Duck typing / dynamic dispatch

`py_dispatch_name(receiver, "speak")` loads the type table from `[self + 8]`,
resolves the attribute's interned slot, loads the slot pointer from
`[table + slot*8]`, and calls it. It takes the receiver as a typed `CValue` and
names the attribute, not a slot number. `make_speak(x)` dispatches `speak` on any
object exposing it — `Dog` and `Cat` share no base class, yet both work. "If it
walks like a duck…" resolved at call time.

### 3. EAFP and runtime `TypeError`

`py_dispatch_checked_name(receiver, "quack")` looks before it uses the field.
It resolves the slot and, if it is `0` (attribute absent), diverts to the
else-arm. It returns a `CElse`; the present and missing arms are written with
`c_else` / `c_end_if_else`. The missing arm prints `TypeError` and continues.

`py_dispatch_required_name(receiver, "quack", print, err, console)` attempts
the use. When the field is present it calls the method. When the field is
absent it prints `err` and exits the generated program with a nonzero status.
It does not return a `CElse`.

Exceptions themselves are demonstrated in `examples/ex9_exceptions`, so this tour only
shows the choice between handling a missing field and failing on it.
`require_quack(duck)` is the required call (Duck has `.quack`, so it succeeds).
`try_quack(dog)` is the checked call (Dog's slot is `0`, and the tour handles
that).

```
mov rax, [table + slot*8]
cmp rax, 0
jne have           ; call it
                   ; absent: caller handles it, or the required call exits
```

### 4. Garbage-collection tour

The generator includes `gc.h`, declares a `Node` GC type
(`gc_typedesc`), opens the collector runtime with `gc_begin`, and calls `gc_demo`:
it allocates two nodes, links them, prints the addresses, drops the roots,
collects, and prints a third object. Those prints are not assertions. On the
conservative collector the first two stay reachable from callee-saved registers,
so the third is a fresh address. `gc_fuzz.h` then checks a three-node graph.
The collector is chosen at build time with `-DGC_OPTION`. Under reference
counting, `gc_collect` is an empty `ret` for both `GC_CYCLES=weak` and
`GC_CYCLES=trial`. `gc_weak_*` is implemented and the tour does not call it.
The trial build exits 0. It does not trace. See
[`../gc/GC_imp.md`](../gc/GC_imp.md) and [`../gc/GC.md`](../gc/GC.md).

## Simplifications versus real CPython

- **The feature-tour objects and refcounts are static globals**, not heap
  `PyObject`s, so their "deallocation" prints a message. The separate GC tour
  uses a heap allocator. Under reference counting, `gc_collect` stays an empty
  `ret` for `GC_CYCLES=weak` and for `GC_CYCLES=trial`. Weak references are
  implemented and that tour does not call them.
- **Method tables are fixed per type**, not mutable `__dict__`s — attribute sets
  are static.
- **`TypeError` is a printed line**, not a real exception object with a
  traceback. The checked call leaves the branch to the caller; the required
  call exits nonzero. Exception handling itself is `examples/ex9_exceptions`.

## Build

`build.bat` compiles `py_main.c` to `py_main.exe`, runs it to emit
`py_output_<option>.asm`, assembles/links `py_output_<option>.exe`
(`/subsystem:console`), and console-tests exit 0 plus the `[Python] feature tour`
marker, repeating across the collector options selected by `-DGC_OPTION`.

The extra Python builds (`GC_CYCLES=trial`, `GC_COALESCE`, `GC_MOVE=sliding`,
`GC_SCAN=full`) are the same check: exit 0 and the marker, plus the collector
graph. They do not prove a trial trace, a neighbour merge, a slide, or a
descriptor walk that only the full scan could have done.

The behavior program checks one method that adds 5 to a field at offset 16, and
that `py_method_end` with nothing open fails generation. It does not check
attribute-table dispatch, a missing attribute, the refcount helpers, more than
one field, or `PY_MAX_ATTRS`. Those stay on the scripted tour.
[`../TEST.md`](../TEST.md) is the coverage report.
