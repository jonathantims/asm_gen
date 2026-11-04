# `cpp_funcs.h` interface design

## Overview

`cpp_funcs.h` is a **C++-flavoured** code-generation layer over
[`c_funcs.h`](c_interface.md) / [`asm_funcs.h`](asm_interface.md). It shows how
the defining features of C++ lower to plain x64 MASM: **virtual dispatch through
a vtable**, **RAII** (deterministic destruction in reverse construction order),
and **compile-time monomorphisation** (templates instantiated per type). Like
`c_funcs.h`, it emits no assembly of its own — every construct flows through the
existing `write_*` emitters, so the generation-time type-tag dictionaries keep
validating the program.

`cpp_main.c` is the worked example; it generates `cpp_output.asm`, a console
program that prints a feature tour and exits 0. `build.bat` builds and runs it,
and `test.c --console` asserts the exit code and the marker line.

## Object model

Three ways to create an object, all of them in the tour:

- `cpp_construct(&Cls, "name")` bakes a global. The string is the MASM data
  label of that one instance (`circle`), the symbol later `lea` and
  `cpp_send` use as the address of the object. It is the instance's name in
  the flat namespace, and the dictionary location of the object. It is a
  different string from the class name (`Circle`) and it is not a constructor
  argument. Two instances need two labels, because MASM has one symbol table.
- `cpp_reserve` plus `cpp_emplace` places a new instance over storage that already exists (the constructor runs again; one destructor stays registered).
- `cpp_stack_construct` subtracts a tracker-sized frame and constructs at `[rsp]`. The enclosing scope runs the destructor, then releases the reservation.

Heap allocation is not invented here. The collectors in `gc/` are where a generated program gets memory from the OS. Globals stay available because they make the tour reproducible.

A polymorphic object is laid out as one vptr per implemented interface, then the fields. The first vptr is at offset 0. Field offsets come from `align_pad` (`ALIGN_NONE`, `ALIGN_C`, `ALIGN_CACHE`, `ALIGN_PAGE`), using the process default captured by `cpp_init` (`ALIGN_C`) unless `cpp_field_mode` overrides one field.

```
[obj + 0]   vptr for the first interface
[obj + 8]   vptr for the second interface, when there is one
[obj + ...] fields
```

A **vtable** is a qword array of method pointers, one per virtual slot. Rather
than hand-writing it, the generator *describes* a class with the **class-definition
builder** (below) and `cpp_class_end` bakes the table:

```
Circle$vtable  dq offset Circle$draw   ; Shape, slot 0
Circle$Named$vtable dq offset Circle$label  ; Named, slot 0
```

The shared `c_ptr_data(name, target, tag)` helper (in `c_funcs.h`) emits a
pointer-sized (`dq offset target`) global and records its tag in the global
dictionary — used for vtables and object headers, whose fields are 8-byte
addresses (`write_initialised_data` emits `db`, which is wrong for pointers).

## Class-definition builder

A class is *described*, not hand-assembled. Between `cpp_class_begin("Circle")`
and `cpp_class_end()` the generator declares the class's members and fields, and the
builder (state kept in `cpp_funcs.h`, mirroring how a compiler keeps a class's
layout in its own tables) accumulates the vtable slot list and bakes the vtable
into `.data` at `cpp_class_end`:

- `cpp_method_begin(name)` — ordinary (non-virtual) member.
- `cpp_virtual_begin(name)` — virtual member; occupies the next vtable slot.
- `cpp_ctor_begin()` / `cpp_dtor_begin()` — constructor / destructor (paired for
  RAII, see below).
- `cpp_implements(&Iface)` — adopt that interface's slots, one vptr per
  interface. This call is how the generator learns that Circle implements
  Shape. There is no base-clause to parse. The generator says it, inside the
  class and before any field. The call copies the interface's virtual names
  and slot numbers into the class (Shape's `draw` stays slot 0) and reserves
  the next vptr. A later `cpp_virtual_begin("draw")` binds the still-open
  slot of that name to `Circle$draw`. `cpp_class_end` emits `Circle$vtable`
  from those bindings. `cpp_vsend(..., &Shape, "draw")` looks the name up on
  the Shape handle, so the slot number is the interface's number.
- `cpp_field(name, type)` / `cpp_field_mode(name, type, align)` — a data field after the vptrs.
- `cpp_virtual_decl(name)` on a class marked `cpp_class_interface()` — an abstract
  slot with no body, fixing the slot numbering that concrete classes and virtual
  calls agree on (the base/interface of a hierarchy).

Each member opener mangles the symbol `Class$method`. MASM's namespace is
flat, so `Circle::draw` and `Square::draw` would both be the symbol `draw`
and the second `PROC` would be a redefinition. `$` is a legal identifier
character once case-sensitive identifiers are on (which `cpp_init` turns
on), and `cpp_check_ident` rejects `$` in any class, method, or field name
the generator writes. A mangled symbol therefore cannot collide with one the
generator spelled. A longer encoding in the style of a C++ compiler
(`?draw@Circle@@`) is unnecessary here: the only collision this layer has
is class plus member, and `$` already separates those two. The same helper
builds `Class$vtable`. The opener then opens a proc with an
**implicit receiver** (`this` = parameter 0), and returns a stable `CFunc*` for
the body (closed with `cpp_method_end`). Inside a body, `cpp_this()` yields the
receiver, so the generator never names a register for `this`. `cpp_class_end`
returns a `CppClass` handle; `cpp_construct(&Class, name)` bakes an object (its
vptr at field 0 for polymorphic classes, followed by one zero qword per declared
field), optionally calls the constructor, registers the destructor with the
current scope, and **returns a `CppObject` handle**. That handle is passed to
`cpp_send(&method, obj)`; inside a body `cpp_this_field(name, type)` addresses
`this->field`. The `Counter` demo mutates `this->value` across two `bump()`
calls. A static (non-virtual) member is called with `cpp_send(&method, obj)`,
supplying the receiver as the implicit `this`.

## Initialisation

A C++ generator calls `cpp_init()` once at the top of `main`, instead of poking the
assembler directly. It selects case-sensitive identifiers and resets the
class-builder state and selects `ALIGN_C`. `core/asm_main.c` still calls
`write_set_case_sensitive_identifiers()` itself because it does not use this layer.

## Features exhibited

### 1. Virtual dispatch (runtime polymorphism)

`cpp_vsend(receiver, &Iface, "draw")` performs a virtual call knowing nothing of
the concrete type. It takes the receiver as a typed `CValue` and **resolves the
slot by name** through an interface/base handle (`&Shape`), so magic slot numbers
stay out of the generator:

```
mov rax, [this]          ; rax = this->vptr
mov rax, [rax + slot*8]  ; rax = vtable[slot]
call rax                 ; this still in rcx
```

`render(shape)` in the generator calls `cpp_vsend(..., &Shape, "draw")`; whether it
reaches `Circle::draw` or `Square::draw` depends solely on which vtable the
object points at — genuine runtime polymorphism resolved at run time. A static
(non-virtual) member is called with `cpp_send(&method, obj)`, which supplies
the receiver as the implicit `this`.

### 2. RAII / deterministic destruction

A class declares its constructor and destructor through the builder
(`cpp_ctor_begin()` / `cpp_dtor_begin()`); the resulting `CppClass Guard` handle
carries both. Within a scope opened by `cpp_scope_begin`,
`cpp_construct(&Guard, "guard_obj")` calls the constructor **and automatically
registers the destructor** with the innermost open scope — the generator never
wires up destruction by hand. `cpp_scope_end` then emits the destructor calls in
**LIFO** order (reverse of construction), exactly like C++ stack unwinding at end
of block.

`cpp_funcs.h` owns a small stack of open scopes, so `cpp_construct` binds each
object to "the scope it is constructed in" without being passed one — the same
way a C++ local binds to its enclosing block. Scopes nest LIFO and
`cpp_scope_end` asserts it is closing the innermost one. This is a
**generation-time** discipline (the order is fixed as the generator runs) that
produces deterministic **runtime** destructor calls.

A **function body is itself a destructor scope**. `cpp_begin_function` /
`cpp_end_function` wrap the language-neutral `c_begin_function` /
`c_end_function` and additionally open and close a body scope, so objects
constructed directly in the body are destroyed (LIFO) when the function ends.
The scope logic is deliberately **not** coupled into `c_begin_function` — the C
layer stays free of C++ semantics; only this thin wrapper adds it.
`cpp_method_begin` builds on `cpp_begin_function`, so every method is a scope too.
Because destruction is emitted at `cpp_end_function`, a function that constructs
body-level objects should reach its end by **falling through** (the destructors
then precede the emitted `endp`); an object that must be destroyed before an
explicit early return belongs in an inner `cpp_scope_begin`/`cpp_scope_end`
block instead (as `guard_obj` is in the demo, so `~Guard()` runs mid-function).

### 3. Templates / monomorphisation

A template like `greet<T>` is instantiated **per type** at generation time.
`cpp_instantiate("greet", type, body, user)` emits one procedure (`greet_int`,
`greet_long`) and calls `body` with that type string, so the body can use the
type. There is no runtime type parameter. `cpp_operator_begin("+")` opens
`operator+`, and `cpp_add(a, b)` is the call.

## Aligned indirect calls

Virtual calls go through a register (`cpp_call_reg` builds an
`AsmCall { reg, NULL, 0, asm_void }` and emits `call rax`). `write_call` places
register arguments and emits the `CALL`. It does not reserve home space,
because a private convention is allowed to skip it. A virtual callee in this
layer is still a Windows x64 function: at the `CALL`, `rsp` must be 16-byte
aligned, and the 32 bytes at `[rsp+0]` through `[rsp+31]` are the home area
where the callee may spill `rcx`, `rdx`, `r8`, and `r9`.

On entry to a function `rsp` is 8 mod 16, because `CALL` pushed the return
address. The smallest reservation that is at least 32 bytes and leaves `rsp`
0 mod 16 is 40: 32 of home space and 8 of alignment. `cpp_frame_open` asks
`asm_call_frame_size(32)` for that size and records it, and `cpp_frame_close`
adds the same number back. If an earlier adjustment already left `rsp` 0 mod
16, the tracker returns 32, so the frame is not padded a second time. The
fixed `sub rsp, 40` that a hand-written call often uses is the special case
of that calculation when `rsp` is 8 mod 16.

## Simplifications versus real C++

- **No heap allocator in this layer.** Globals, placement into an existing global, and a stack reservation are the three forms. `gc/` is where heap allocation is generated.
- **Several interfaces, not several base classes.** `cpp_implements` adopts each interface's slot layout. Each interface has its own vptr and its own vtable. There is no RTTI and no virtual base.
- **A template is one body instantiated per type.** `cpp_instantiate` emits one procedure per concrete type and passes the type string into that body. There is no runtime type parameter.
- **RAII here is LIFO over a fixed list, not exception unwinding.** CPU exceptions are demonstrated in `examples/ex9_exceptions` (a vectored handler). They are left out of this tour so the class example stays straight-line.

## Build

Build everything with `build.bat`, which compiles `cpp_main.c` to `cpp_main.exe`,
runs it to emit `cpp_output.asm`, assembles and links `cpp_output.exe`
(`/subsystem:console`), and runs the console test asserting exit 0 and the
`[C++] feature tour` marker.

The behavior program checks one method that adds 3 to a field at offset 0, and
that `cpp_method_end` with nothing open fails generation. The scripted tour is
what exercises virtuals, interfaces, constructors, destructors, scope-end
destruction, `cpp_vsend`, inheritance, multiple fields, and methods with
arguments. That tour checks the exit code and the marker. It does not check
return values of those features. A wrapper fuzz of field counts and a virtual
call was not started. Those do not allocate. [`../TEST.md`](../TEST.md) is the
coverage report.
