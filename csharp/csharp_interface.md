# `csharp_funcs.h` interface design

## Overview

`csharp_funcs.h` is a **C#-flavoured** code-generation layer over
[`c_funcs.h`](c_interface.md) / [`asm_funcs.h`](asm_interface.md). It shows how
core CLR/C# features lower to x64 MASM: **virtual dispatch through a
MethodTable**, **boxing/unboxing with a runtime type check** (throwing
`InvalidCastException` on a bad cast), and **`using` / `IDisposable`**
(deterministic `Dispose()` at scope exit). Emits no assembly of its own — all
output flows through the existing `write_*` emitters.

`csharp_main.c` generates `csharp_output_<option>.asm`, a console program that
prints a feature tour — including a **garbage-collection tour** driven by the
selected collector — and exits 0. The filename is suffixed with the active
`-DGC_OPTION` because the collector's assembly is baked in; `build.bat` builds and
console-tests it across the collector options.

## Object model

Reference-type instances are **GC heap objects**. `cs_construct(heap, &Class, name)`
calls `gc_new`, the same allocator the collector tour uses, and builds the
object in that memory. Call it from inside a function, after `gc_start`. The
name argument is the dictionary tag of the pointer, the same role as any other
`CValue` name.

`c_ptr_data` still emits the MethodTable and the type-handle constants, and
those are data globals, because they are immutable metadata shared by every
instance. An instance is not one of those globals. Reading `cs_construct` and
seeing a string name is what makes the allocation look like `cpp_construct`,
which does bake a global. The C# path does not. The collector header is:

```
[obj + 0]   GC descriptor
[obj + 8]   collector meta (refcount / mark / forwarding)
[obj + 16]  MethodTable pointer   ; polymorphic classes only
[obj + 24]  ... fields            ; or [obj + 16] when there is no MethodTable
```

A **MethodTable** is the CLR analogue of a vtable — a qword array of method
pointers per virtual slot. A class is described with the **class-definition
builder** (below) and `cs_class_end` bakes the table:

```
Animal$vtable  dq offset Animal$Speak   ; slot 0
Dog$vtable     dq offset Dog$Speak      ; slot 0
```

Method names are not unique across classes. One helper concatenates every
emitted symbol with `$`: `Class$method`, `Class$vtable`. A name that already
contains `$` is rejected.

A **boxed value type** is a heap object, not a static global. Its payload is a
type handle plus the copied value:

```
[box + 0]   GC descriptor
[box + 8]   collector meta
[box + 16]  Int32_type    ; the box's runtime type
[box + 24]  42            ; the boxed payload
```

Type handles are named constants (`Int32_type = 1`, `String_type = 2`) emitted
by `cs_cli_init`. The shared `c_ptr_data` / `c_word_data` helpers still emit the
MethodTable and other static metadata; instances and boxes do not use them.

## Class-definition builder

A class is described between `cs_class_begin("Dog")` and `cs_class_end()`;
the builder (state in `csharp_funcs.h`) accumulates the MethodTable slot list and
bakes `Class$vtable` into `.data` at `cs_class_end`. It also emits that class's
GC descriptor (header + MethodTable qword, if any + one qword per field). Fields
are scalar words, so the descriptor lists no traced pointers.

- `cs_method_begin(name)` — ordinary (non-virtual) member (also constructors and
  `Dispose`).
- `cs_virtual_begin(name)` — virtual member; occupies the next MethodTable slot.
- `cs_field(name)` — a data field after the MethodTable pointer.
- `cs_virtual_decl(name)` on a class marked `cs_class_interface()` — an abstract
  slot with no body, fixing the slot numbering shared by derived classes.

Each opener mangles `Class$method`, opens a proc with an **implicit receiver**
(`this` = parameter 0), and returns a stable `CFunc*` (closed with
`cs_method_end`); `cs_this()` yields the receiver inside a body. `this` is
parameter 0. `c_begin_function` tracks it. A call saves that register and
restores it. `c_dispose` drops a value from that save.

`cs_construct(heap, &Class, name)` allocates the object with `gc_new`, writes
the MethodTable pointer at the first payload qword (polymorphic classes), zeroes
fields, and **returns a `CsObject` handle**. Call it from inside a function,
after `gc_start`. That handle is passed to `cs_send(&method, obj)` and
`cs_using_own(&u, dispose, obj)`. Inside a member body,
`cs_this_field(name, type)` addresses `this->field`; from outside,
`cs_field_ref(&obj, name, type)` addresses `obj.field` through the instance
pointer. The FileStream demo uses a field `open` its ctor sets
(`cs_field_store_imm`) and Dispose reads back.

## Initialisation

Call `cs_init()` once at the top of `main`, instead of poking the assembler
directly. It selects case-sensitive identifiers (the CLR is case-sensitive;
Win32 externs are mixed-case) and resets the class-builder state.
Only `core/main.c` still calls `write_set_case_sensitive_identifiers()` directly.

`cs_cli_init()`, called from the initialised data section, emits the CLI
baked-ins: the `Int32_type` / `String_type` equates, the `Node` GC descriptor
(the collector tour's traced `next` plus a scalar `value`), and the `Box`
descriptor. `cs_box` allocates a box on the heap at run time.

## Features exhibited

### 1. Virtual dispatch via the MethodTable

`cs_vsend(receiver, &Base, "Speak")` loads `[this+16]` (the MethodTable, just
past the collector header), then `[mt + slot*8]`, then `call`s it — reaching
`Animal.Speak` or `Dog.Speak` purely from the object's MethodTable pointer. It
takes the receiver as a typed `CValue` and **resolves the slot by name** through
a base handle, so slot numbers stay out of `csharp_main.c`. `describe(Animal)`
knows nothing of the concrete type; dispatch is resolved at run time. A
non-virtual member is called with `cs_send(&method, obj)`, supplying the
implicit `this` from the `CsObject`.

### 2. Boxing / unboxing / downcast with one runtime type check

`cs_check_type` loads the type word at a payload offset, compares it with the
expected type, and returns a `CElse` so the matched and mismatched arms are
written with `c_else` / `c_end_if_else`. `cs_unbox_begin` and `cs_downcast_begin`
both call it; every unbox and every downcast takes that path.

```
mov rax, [obj+16]         ; type handle, or the MethodTable pointer
mov rdx, expected
cmp rax, rdx
jne <else>                ; -> InvalidCastException path
```

The check happens at **run time**, and it is the runtime MethodTable at
`[obj + 16]`, not the static type of the reference. Unboxing the boxed int as
`int` matches and continues; unboxing it as `string` mismatches and takes the
caught-exception path. Downcasting a `Dog` to `Dog` matches and calls
`Dog.Speak` through the method pointer. An Animal-typed reference whose table
is `Dog$vtable` also matches: the tour binds the existing Dog pointer to an
Animal-context `CValue` and passes that to `cs_downcast_begin`. A real
`Animal` (`Animal$vtable`) fails and takes the mismatch arm. Each outcome is
proven with a printed line.

### 3. `using` / `IDisposable`

`CsUsing` records the disposable and its `Dispose` method. `cs_using_end` emits
the `Dispose()` call, in reverse order of `cs_using_own`, on the path that
reaches it. That is the part of a C# `using` block this layer implements:
`Dispose` runs at a known point, and the collector is a separate question.
The object memory stays the collector's. `Dispose` here is an ordinary method
that can release a resource the collector does not know about (the FileStream
demo reads the `open` field and prints).

A CLR `using` is a `try`/`finally`. `Dispose` runs when the block exits by
falling through, by `return`, or by a thrown exception. A complete
`IDisposable` also has a finalizer that calls `Dispose` if the user never
did, and `Dispose` calls `GC.SuppressFinalize` so that finalizer does not
run twice. This layer emits the call only at `cs_using_end`. An early
`c_ret` that skips that call does not dispose. There is no finalizer and no
`SuppressFinalize`. The exception tour in `examples/ex9_exceptions` is a
vectored handler, and it is not wired to `cs_using_end`.

The simplification is that gap. Deterministic `Dispose` on the path that
reaches the end of the scope is the feature the tour shows. Matching the
CLR's finally-and-finalizer pair is left out, which is acceptable for this
layer.

### 4. Garbage-collection tour

`csharp_main.c` includes `gc.h`. `cs_cli_init`
declares the `Node` GC type, `gc_begin` opens the collector runtime, and
`gc_demo` runs the tour: it allocates two nodes, links them, prints the
addresses, drops the roots, collects, and prints a third object. Those prints
are not assertions. On the conservative collector the first two stay reachable
from callee-saved registers, so the third is a fresh address. `gc_fuzz.h` then
checks a three-node graph. Which collector runs is chosen at build time with
`-DGC_OPTION`. What the graph checks, and what the flag builds leave unproved,
is in [`../gc/GC_imp.md`](../gc/GC_imp.md) and [`../gc/GC.md`](../gc/GC.md).

## Simplifications versus real C#

- **`IDisposable` / `using` is the scope-exit call, and that is the
  simplification.** `Dispose()` runs when the generator reaches `cs_using_end`.
  A CLR `using` is a `try`/`finally`, so `Dispose` also runs on `return` and
  on a thrown exception, and a finalizer is the backup if the user never
  disposed. This layer has none of those. The collector still reclaims the
  object. See section 3.
- **MethodTables are single-slot** (built by the class builder, not written out
  by hand). Interfaces, generics, and CLR metadata are feasible and are omitted
  to keep the example small.
- **Exceptions in this layer are a branch**, not a stack-unwinding runtime. The
  `InvalidCastException` path is the mismatch label, which prints a message.
  Exceptions are implemented in `examples/ex9_exceptions` (vectored handler),
  not in this layer.
- **Boxing is a heap allocation.** A box is a GC object whose payload is a type
  handle plus the copied value.

## Build

`build.bat` compiles `csharp_main.c` to `csharp_main.exe`, runs it to emit
`csharp_output_<option>.asm`, assembles/links `csharp_output_<option>.exe`
(`/subsystem:console`), and console-tests exit 0 plus the `[C#] feature tour`
marker, repeating across the collector options selected by `-DGC_OPTION`.

The behavior program checks one method that adds 1 to a field at offset 16, and
that `cs_method_end` with nothing open fails generation. That program does not
exercise virtual send, interfaces, `using` / dispose, boxing, downcast, type
checks, or `cs_new`. `cs_new` allocates and stays on the scripted tour. The
tour's check is the exit code, the marker, and the collector graph.
[`../TEST.md`](../TEST.md) is the coverage report.
