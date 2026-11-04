# `js_funcs.h` interface design

## Overview

`js_funcs.h` is a **JavaScript-flavoured** code-generation layer over
[`c_funcs.h`](../c/c_interface.md) / [`asm_funcs.h`](../core/asm_interface.md). It
reproduces three defining JS behaviours, all resolved at run time: **closures
over a fresh environment**, **prototype-chain property lookup** (a miss yields
`undefined`), and **truthiness** (coercion of a tagged word to a boolean).

`js_main.c` generates `js_output_<option>.asm`, a console program that prints a
feature tour — including a **garbage-collection tour** driven by the selected
collector — and exits 0. The filename is suffixed with the active `-DGC_OPTION`;
`build.bat` builds and console-tests it across the collector options. The banner
is `[JavaScript] feature tour`.

## Object model

An object is two header qwords, then any fields declared with `js_field`:

```
[obj + 0]  __proto__ pointer          (0 = end of chain)
[obj + 8]  own property-list head     (0 = no own properties)
[obj + 16] first js_field, one qword each
```

A property node is three qwords, not a slot in a dense table:

```
[node + 0]  next
[node + 8]  name id     (interned by js_name_id)
[node + 16] value       (a function pointer, for the tour)
```

`js_class_end` bakes one static list per class, first declared method at the
head. `js_construct` points `[+8]` at that list. `js_set` prepends a node taken
from a pool of 16, so the new property shadows the shared tail and other
objects that still point at the tail do not see it. Past the pool the generated
`js_set` executes `ud2`.

Names are mangled `Class$method`. `$` is rejected in a class or method name, the
same rule the other language layers use. Caps are 80 classes, 80 members, and
80 interned names, in line with the C# layer.

```
Proto_n0       dq 0
               dq 0                  ; name id of greet
               dq offset Proto$greet
proto_obj      dq 0                  ; end of the chain
proto_obj_t    dq offset Proto_n0
child_obj      dq offset proto_obj
child_obj_t    dq offset Child_n0    ; own methods; greet is on the prototype
```

## Class / prototype builder

JavaScript has no classes at the metal. The generator still describes a shared
prototype between `js_class_begin("Proto")` and `js_class_end()`, and declares
each method with `js_method_begin(name)` (implicit `this` = parameter 0, closed
with `js_method_end`). `js_this()` is the receiver inside a body. `js_field`
declares an own field after the header. `js_construct(&Class, name, proto)`
bakes the object and returns a `JsObject`. `js_send(&fn, obj)` calls a function
with that object as `this`.

A dense table padded out to a fixed width would be mostly zeros once most names
are absent. The list stores only the properties that exist.

## Initialisation

A JavaScript generator calls `js_init()` once at the top of `main`. It selects
case-sensitive identifiers and clears the class list and the name table. Call
`js_data()` from `write_data()` (the property pool and the closure pool) and
`js_emit_runtime()` once from `write_code()` before any `js_set`.

## Features exhibited

### 1. Closures over a fresh environment

`js_envs` is eight qwords, each a count starting at 0. `js_env_i` is the next
free cell. `makeCounter` returns that cell and advances the index. `tick` takes
the cell as its parameter, adds one, and stores it back. Two calls to
`makeCounter` therefore do not share a count: A's ticks print 1, 2, 3, and B's
first tick prints 1.

### 2. Prototype-chain lookup and `undefined`

`js_lookup(receiver, "greet")` walks the chain and returns the value word. It
does not call it. The caller tests the word against 0 and, on a hit, calls the
pointer with the receiver:

```
        mov rbx, self
loop:   cmp rbx, 0
        je  missing
        mov r10, [rbx+8]       ; own list
scan:   cmp r10, 0
        je  up                 ; end of this list -> follow __proto__
        cmp qword ptr [r10+8], name_id
        je  hit
        mov r10, [r10]
        jmp scan
up:     mov rbx, [rbx]
        jmp loop
hit:    mov r11, [r10+16]      ; the function pointer, not a call
```

`child.greet` is not on the child's list, so the walk follows `__proto__` and
finds it on the prototype. `child.missing` is found nowhere, so the result is 0
and the caller prints `undefined`. `js_set(child, "extra", extra_fn)` prepends
a node; the next lookup of `extra` hits that node and the caller calls it.

### 3. Truthiness

`js_if_truthy_else(value)` branches on a tagged word:

| word | meaning | falsy when |
| --- | --- | --- |
| 0 | null / undefined | always |
| low bits `01` | number, value is `word >> 2` | the number is 0 (the word is `1`) |
| low bits `10` | string; clear the low bits to get the address | the first byte is 0 |
| any other nonzero word | object | never |

`if (42)`, `if (0)`, `if ("")`, `if (null)`, a nonempty string, and an object
each take the arm that sentence describes. 42 is the word `(42 << 2) | 1`.

### 4. Garbage-collection tour

The generator includes `gc.h`, declares a `Node` GC type, opens
the collector runtime with `gc_begin`, and calls `gc_demo`: it allocates two
nodes, links them, prints their addresses, drops the roots, collects, and prints
a third object. Those prints are not assertions. On the conservative collector
the first two stay reachable from callee-saved registers, so the third is a
fresh address. `gc_fuzz.h` then checks a three-node graph. The collector is
chosen with `-DGC_OPTION`. See [`../gc/GC_imp.md`](../gc/GC_imp.md) and
[`../gc/GC.md`](../gc/GC.md).

## Limits versus real JavaScript

- **Fields declared with `js_field` are header slots, not list entries.** The
  property list holds function pointers in this tour. There is no separate
  data-property shape and no `this` rebinding. That is an accepted limit of
  this layer: lookup returns the stored word, and the caller decides what to
  do with it.
- **The tag is two low bits, not a NaN-box.** The falsy set above is the one
  the tour implements: null, numeric zero, and the empty string.
- **Each closure cell is a leaf on the collector's heap.** `makeCounter`
  calls `gc_alloc` with a 24-byte descriptor (16-byte header, count at
  offset 16, no pointer fields). The old eight-cell static pool is gone.
  The tour does not collect between ticks. It drops the counters before
  the GC demo, so a later collection may reclaim them. A refcount build
  can leak those two cells until exit; nothing decrefs them.
- **`js_set` draws from sixteen nodes.** `JS_PROP_POOL` is 16. Each
  property is a 3-qword node (next, name id, value) taken from the static
  `js_nodes` array and indexed by `js_node_i`. The 17th `js_set` executes
  `ud2`. The array is not grown. Those nodes stay immortal: the object's
  property list points at them, and moving them onto the collected heap
  without roots would leave that list dangling across a collection.

## Build

`build.bat` compiles `js_main.c` to `js_main.exe`, runs it to emit
`js_output_<option>.asm`, assembles and links `js_output_<option>.exe`
(`/subsystem:console`), and checks exit 0 plus the `[JavaScript] feature tour`
marker, for each `-DGC_OPTION`.

The behavior program checks one method that adds 9 to a field at offset 16, and
that `js_method_end` with nothing open fails generation. It does not exercise
the prototype chain, truthiness, `js_lookup` / `js_set`, computed properties,
`js_make_counter`, or the env type. Closures allocate, so they stay on the
scripted tour. The limits above are what this layer implements. The behavior
image does not exercise them. The tour checks the exit code and the marker.
[`../TEST.md`](../TEST.md) is the coverage report.
