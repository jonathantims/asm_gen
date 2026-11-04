# asm_gen

Do new language features require a new language? This repository asks whether C — a small, well known language — can host them by generating code. The generator is an ordinary C program. It emits a Windows x64 program, and the features live in the generator: turn a check on and the emitted program contains it; leave the check off and the emitted program has no trace of it.

The historical arc is interpreters, then compilers, and now generation as something a programmer does directly, especially with AI help. The programmer writes the feature once, as C that prints machine code, instead of waiting for a language or a toolchain to grow the feature.

The generator emits Windows x64 MASM (`ml64`). The text it writes is the program that runs. That is a deliberate target, for the reasons below.

## Why the target is assembly

A generated program has two jobs that pull against each other. The feature has to be exact: a check present, or gone; a particular width; a particular call; a layout the generator can name while it is still running. The result also has to be tight. The instructions that implement the feature should be the instructions the feature needs, and the build should contain nothing whose job is to recover a good program afterwards.

### C hides the state the generator has to control

C is a request to a compiler. The request carries a large amount of state the generator does not own and cannot see. Integer promotions and the usual arithmetic conversions decide widths. Sequencing rules decide which expression runs first. Signed overflow, strict aliasing, and effective types decide which programs are meaningless. The as-if rule decides which of the meaningful ones may be rewritten. Register assignment, stack slots, instruction selection, and whether a load is still needed are all downstream of that. A generator that emits C is feeding a second program, the optimizer, and then hoping the second program still means what the first one asked for.

That hope gets expensive in step with the feature. A bounds check that is a compare and a branch when the feature is on, and is absent when it is off, is a fact about the instruction stream. Emitted as C, it is a fact the compiler is allowed to reconsider: it may delete the check, it may keep a check the feature meant to omit, it may widen the index, it may reassociate the address arithmetic. A higher-precision shadow of a floating-point expression, a cycle counter around one call, a trap that must be a single `ud2`, a frame whose slots are allocated while the generator runs: each of these is a fact about registers, flags, and memory. C makes those facts implicit. Getting them back means writing C that is carefully ugly, and then reading the compiler's assembly to see whether the ugliness survived.

Assembly keeps the same facts in the open. The generator names the register, the width, the flag, and the label. What it emits is what the processor runs. Efficiency is local. Choose the instruction, choose the register, spill when the pool is full. A disabled feature is implemented by leaving its instructions out, which is why an example can search the assembly for a token and require that the token be missing. Nothing runs after the generator to put that token in or take it out.

### Another language is a second machine

Generating the source of a higher-level language means keeping two models honest at once. One is the CPU: registers, the stack, calls, memory. The other is the virtual machine the language embodies: objects, prototypes, reference counts, moves and drops, a collector, a vtable, a borrow checker whose answer the backend will lower again. The generator has to predict both, including the ways that language's own compiler will turn the virtual machine into instructions. A feature that needs a particular machine state — the receiver in `rcx`, the tag in the low bits of a word, the call that a profiler must be able to delay and no other — has to be written as source that the language implementation is still free to rearrange.

The wrappers in this repository are that virtual machine, and they lower it themselves. A JavaScript property walk, a Rust drop, a C++ virtual call, a collector barrier: each is a plan that becomes registers and memory. The model of the language and the model of the machine are one program. There is no further toolchain reinterpreting both of them.

### The machine is already available

C and the languages above it expose the processor through a narrow door: intrinsics, builtins, inline assembly extensions, and whatever the current compiler release decided to recognise. The door is narrower than the machine. A generator that needs `rdtsc`, a read of the carry or overflow flag, `ud2`, `rep movsb` with its real registers, a non-temporal store, the Windows shadow space on a call, a call through a register, or a vectored handler has to ask for something the language may outline, reorder, reject, or spell differently next year.

Here those are ordinary emitter calls, next to the structured helpers. `write_mov` and `c_if_eq` cover the common case and check tags while they do it. A feature that needs a specific instruction calls the matching helper, in the same C function as the structured code around it. Dropping down to the instruction is the normal path, because the instruction stream is the target and nothing sits in front of it.

## Why there is no syntax tree

The usual design is to build an abstract syntax tree, analyse it, and lower it. I built production generators that way for a long time, and I was convinced it was the right design. A target-independent tree, analyses that see the whole program, one optimizer, several backends. Those are real advantages for a compiler that is itself the product. They did not hold up for generation used as a way to write programs.

A tree is advertised as independent of the machine. In practice it does not stay independent once the output has to be fast. The features that matter are machine facts: this add sets flags, this call uses the Windows registers, this value is a 16-bit atom, this loop wants a counted register because that is what the hardware does well. Put those facts in the tree and the tree is a second syntax for one backend. Leave them out, so the tree can stay pure, and the lowerer has to emit a cautious general translation: extra moves, memory where a register would do, a call where a flag would do. The result is slow in a systematic way. "Independent" and "efficient" turned out to be two different trees. The projects I worked on picked one of them while still paying for the vocabulary of the other, and the one they picked was the tree bound to the target.

The tree also removes the drop-down. Every unusual need has to be a node the tree already has: an intrinsic, an inline-assembly blob, a builtin, a black box. Those hooks are designed before the feature exists, so they are shaped by what the tree's author could imagine on the day the hook was added. A later feature that needs a flag, a label with a precise lifetime, or a call sequence the node does not mention has to be bent through the nearest hook, or the tree grows another node and every pass learns it. The hook is always a little narrower than the case in front of you, and the set of hooks is always behind the machine. The structured helpers and the instruction writers here append to the same buffer. A contract, a spill, and an `rdtsc` are three calls in one generator function. Nothing had to foresee that combination before the feature was written.

The quality problem follows directly. A lowerer that must accept every shape of tree emits a correct, average instruction sequence and leaves the sharp cases to an optimizer. The optimizer then exists to climb out of the hole the tree dug: redundant moves, aliasing treated as hostile, flags hidden behind a call, a branch that used to be one instruction and became a control-flow graph. I spent a long time writing that optimizer, and a long time reading assembly to find out whether it had won this time. This repository is the other experiment. The code that understands the feature also chooses the instructions. When the feature is off, those instructions are never written. When it is on, they are the ones the feature asked for, and the tag dictionaries check them as they are emitted. `build.bat` has no optimizer stage, because there is no recovery pass to run.

## Examples

Each example is a feature that would usually need a new language or a specialised tool. The same source builds the feature in or leaves it out.

Each example is split the same way. The `.h` file is a generic library: the check, the grid, the hook, and the helpers that turn the feature on or off. The `.c` file is one application of that library. It names the data, writes the calculation, and decides whether the feature is built in. `EX_ENABLE` is that decision. The causal profiler (`examples/ex11_causal`) and the quantised update (`examples/ex12_quantisation`) are the two that show the split in full: the header is only the library, and the application is only the calls, the data, and the switch.

- **Profiling** (`examples/ex3_profile`). Each profiled call is timed, and the program prints the time spent. That number is not the critical path.
- **Causal profiling** (`examples/ex11_causal`). This is the case where time spent and the critical path differ. Slowing one call shows whether the program actually gets slower, which a time-spent report cannot tell you. A call can dominate the profile and still sit off the critical path.
- **Floating-point shadow** (`examples/ex1_fpaccuracy`). A single-precision computation is mirrored by a higher-precision shadow, or by an expression tree kept at a chosen precision. The program can trap when the error passes a threshold.
- **Quantisation** (`examples/ex12_quantisation`). A gaussian prior update, for example after a game outcome, is a step on a finite grid of known states. The update cannot drift off that grid, the grid can be fuzzed, and the floating-point shadow is the other way of watching the same class of error.
- **Contracts** (`examples/ex4_contracts`). Pre-conditions, post-conditions, and a class invariant are woven around an account, or omitted entirely.
- **Bounds** (`examples/ex10_bounds`). An index is range-checked. With the check off, the access is an ordinary load.
- **Use-after-free** (`examples/ex6_uaf`). A liveness table traps a use of a freed object. With the check off, there is no table.
- **Fuzz** (`examples/ex7_fuzz`, `examples/ex8_fuzzmock`). A fuzz test is emitted next to the function it checks. The mocked variant re-emits the same function against a generated input instead of its real source.
- **Layout** (`examples/ex5_layout`). One description of a record is emitted either as contiguous objects or as parallel arrays. Optional and multiple values are left out of the parallel arrays and recovered through an index. The reads do not change with the representation.
- **Exceptions** (`examples/ex9_exceptions`). A throw leaves the failing procedure and lands in a handler. The example uses a vectored handler.
- **A swappable collector** (`gc/`). Reference counting, mark-sweep, compacting, and conservative scanning are four strategies behind one selection. See [`gc/GC.md`](gc/GC.md).
- **Language wrappers.** C, C++, C#, Rust, Python, and JavaScript. C is the typed-value layer ([`c/c_interface.md`](c/c_interface.md)). The other five are thin planners over that layer ([`docs/language_wrappers.md`](docs/language_wrappers.md)); C#, Rust, Python, and JavaScript each build against every collector.

## Where to read

- [`core/asm_interface.md`](core/asm_interface.md) — the MASM emitter: readability, type tags, and the checks on each helper. `core/asm_main.c` is its demo.
- [`c/c_interface.md`](c/c_interface.md) and [`c/c_architecture.md`](c/c_architecture.md) — typed values, registers, alignment, and spilling.
- [`gc/GC.md`](gc/GC.md) — why the four collectors differ.

## Build

From the repository root, run `build.bat`. It compiles the generators, runs them, assembles the emitted MASM, links, and tests. Do not run those steps by hand. Assembly, binaries, and logs are written in `out/`.

The generators, the test harness, and the programs they emit are linked without the C runtime. `mainCRTStartup` in `core/host.h` calls `commit_stack_pages` before `main`. That walk touches the stack a chunk at a time, which is what `__chkstk` would have done, and the pages stay committed afterwards. Each generated `main` does the same walk, and so does a generated thread entry, because a new thread has its own guard page.

`config.bat` contains configuration for the repo, such as where to find the Visual Studio environment. It is self-documenting, so look there for further information.

## License

The code is covered by [LICENSE](LICENSE) (BSD Zero Clause License, 0BSD). It is provided as-is, with no guarantees and no warranties, and with no limits on use.
