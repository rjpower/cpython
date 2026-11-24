# PyJIT register instructions and executable semantics

**Recommendation. Keep the eight-byte register instruction head, give every operation named operands and typed exits, and make its definition an executable control-flow graph. Use explicit destinations throughout. Keep the top-of-stack optimization as a write-through machine-register cache in the baseline JIT.** The interpreter, baseline and tracer should all consume the same verified graph. Stack effects should survive in the public bytecode description and its decoder.

The first port should be **ADD, including its generic Python arm, followed by instance-value attribute load**. Together they test arithmetic, allocation failure, reflected dispatch, feedback validity, aliasing, finalizer reentry and generated native code. A design that makes those two definitions longer or less precise than their Python behavior warrants needs revision before the bulk port.

This proposal targets `pyjit/regions89-g0d1` at `dd7e484be64`. I ran the checkout’s record builder and verifier in memory and inspected its generated products. The supplied release executable identifies itself as **`d344a3dcd5a`, GCC 15.2.0**. Its read-only probes establish current instruction shapes and behavior on that build. The published timings belong to earlier named checkpoints. No build, tree edit or new benchmark run was made.

## 1. Survey

The strongest precedent is a combination of three designs. **Ignition** makes operand roles inspectable. **Sparkplug** shows how much of a baseline compiler can be reduced to frame-compatible code and calls to shared builtins. **SpiderMonkey’s CacheIR** shows how to express a speculative operation once in a form that both baseline code and an optimizer can use. The last of these is the closest match for the proposed semantics language.

| System | Where the instruction and its meaning live | What PyJIT should take from it |
|---|---|---|
| **V8 Ignition** | `bytecodes.h` lists the opcode, implicit register use and ordered operand types. For example, `GetNamedProperty` writes the accumulator and takes a register, constant-pool index and feedback slot. `Add` reads and writes the accumulator. Short stores declare an implicit destination register. Scalable operands have single, double and quadruple scales. Handler bodies use the CodeStubAssembler family of operations. | Declare register reads, writes, immediate kinds and feedback in the instruction list. Define semantic operations over typed values in the body. Ignition’s accumulator saves bytes in a variable-length stream. PyJIT’s existing add already fits its destination, both inputs and feedback in one head. [Bytecode list](https://github.com/v8/v8/blob/main/src/interpreter/bytecodes.h) · [Operand definitions](https://github.com/v8/v8/blob/main/src/interpreter/bytecode-operands.h) · [Handler source](https://github.com/v8/v8/blob/main/src/interpreter/interpreter-generator.cc) · [Ignition design](https://v8.dev/blog/ignition-interpreter) |
| **V8 builtins and ICs** | Call descriptors specify interfaces and registers. Torque gives builtins and macros typed parameters, results and named continuation labels. A `transitioning` call invalidates transient refinements. Feedback vectors hold slot-specific observations. Load ICs can distinguish uninitialized, monomorphic, polymorphic and megamorphic sites. | Give helpers, guards, continuations and stubs signatures. Kill heap refinements across reentry. Keep observations separate from the operation and require an executable guard before using them. Python’s ownership obligations and finalizer checkpoints need additional declarations of our own. [Torque manual](https://v8.dev/docs/torque) · [Call descriptors](https://github.com/v8/v8/blob/main/src/codegen/interface-descriptors.h) · [Feedback implementation](https://github.com/v8/v8/blob/main/src/objects/feedback-vector.cc) |
| **V8 Sparkplug, Maglev and TurboFan/Turboshaft** | Sparkplug walks bytecode, emits native control flow, maintains interpreter-compatible frames, maps native PCs to bytecode positions and calls shared builtins. Maglev builds a speculative SSA graph from bytecode and feedback, with frame state for exits. V8’s documented top-tier pipeline can take the Maglev graph through Turbolev into Turboshaft. The older bytecode graph builder also feeds the top tier through a different frontend. | Share the executable input, feedback vocabulary and frame contract. The optimizing tier needs its own value graph. V8 has tier-specific visitors and lowerings. Generating the full PyJIT arm graph for all consumers is a stronger requirement we are choosing. [Sparkplug](https://v8.dev/blog/sparkplug) · [Maglev](https://v8.dev/blog/maglev) · [Top-tier pipeline](https://chromium.googlesource.com/v8/v8/+/main/docs/compiler/turbofan/compiler-turbofan.md) |
| **JavaScriptCore** | `BytecodeList.rb` declares operands such as `VirtualRegister` and identifier indexes, plus per-op metadata. The representation separates instruction bytes from mutable metadata and supports wider encodings. LLInt implements handlers in offlineasm. The Baseline JIT compiles bytecode, DFG parses it with profiling information, and FTL lowers the optimized DFG through B3 and its machine backend. | Generate encoders, decoders and typed metadata access from one schema. Keep an optimizer IR that can express more than a bytecode. The spec does not generate every tier’s executable body. Moving the PyJIT interpreter into assembly would also discard a working generated C target. [Bytecode specification](https://github.com/WebKit/WebKit/blob/main/Source/JavaScriptCore/bytecode/BytecodeList.rb) · [Format and metadata](https://webkit.org/blog/9329/a-new-bytecode-format-for-javascriptcore/) · [LLInt](https://github.com/WebKit/WebKit/blob/main/Source/JavaScriptCore/llint/LowLevelInterpreter.asm) · [B3](https://webkit.org/blog/5852/introducing-the-b3-jit-compiler/) |
| **SpiderMonkey** | Its Baseline Interpreter and Baseline JIT share a frame layout and much of a templated emitter. The interpreter loads dynamic operands where the compiler knows constants. CacheIR is a small, linear IC language with typed value IDs, stub fields and immediates. Guards and idempotent loads precede a final result operation, which may have effects. Warp snapshots bytecode and IC data and transpiles selected CacheIR to MIR. | Share the ordered guard and action sequence. Store site data separately so one stub serves many sites. PyJIT needs a richer result protocol for errors, finalizers and frame switches. CacheIR’s single final effect is an excellent subset for a property-read prefix. [Baseline code sharing](https://hacks.mozilla.org/2019/08/the-baseline-interpreter-a-faster-js-interpreter-in-firefox-70/) · [CacheIR](https://firefox-source-docs.mozilla.org/js/cacheir.html) · [Warp and tiering](https://firefox-source-docs.mozilla.org/js/how-we-optimize.html) |
| **LuaJIT** | `lj_bc.h` describes 32-bit ABC and AD instructions and operand modes. The DynASM interpreter implements them. The recorder explicitly translates bytecodes and observed behavior into trace IR. Snapshots map optimized values back to VM slots. | Keep decode inexpensive and make exit maps part of trace construction. A register bytecode alone does not eliminate a second implementation in the recorder. LuaJIT maintains that mapping explicitly. PyJIT can generate much of it. [Formats](https://github.com/LuaJIT/LuaJIT/blob/v2.1/src/lj_bc.h) · [Interpreter](https://github.com/LuaJIT/LuaJIT/blob/v2.1/src/vm_x64.dasc) · [Recorder](https://github.com/LuaJIT/LuaJIT/blob/v2.1/src/lj_record.c) · [Snapshots](https://github.com/LuaJIT/LuaJIT/blob/v2.1/src/lj_snap.c) |
| **HHVM** | HHBC is stack-oriented. Its opcode descriptions include immediates and stack behavior. The region JIT selects profiled bytecode regions, builds higher-level HHIR with guards and side exits, then lowers to vasm for machine code. | Keep semantic effects and ownership visible above machine lowering. Profile-guided region selection belongs to a later optimizer. It would add compilation cost to a baseline intended to compile every warm method cheaply. [HHBC opcode list](https://github.com/facebook/hhvm/blob/master/hphp/runtime/vm/opcodes.h) · [Region JIT](https://hhvm.com/blog/2017/02/17/region-jit.html) · [HHIR and vasm](https://hhvm.com/blog/10205/llvm-code-generation-in-hhvm) |
| **Dalvik and ART** | DEX specifies numbered registers, typed instruction formats, compact invoke lists and consecutive-register `/range` invokes. Results of invokes have a constrained `move-result` protocol. ART’s graph builder reads DEX and produces optimizing IR. Interpretation, JIT and AOT share the DEX contract. | Use named argument vectors and a range encoding where arguments are actually consecutive. Give a Python call its result destination on the call itself. A separate result instruction would add a boundary precisely where PyJIT already needs a saved recipient for return, unwind and monitoring. [DEX specification](https://source.android.com/docs/core/runtime/dalvik-bytecode) · [ART instruction builder](https://android.googlesource.com/platform/art/+/master/compiler/optimizing/instruction_builder.cc) · [ART JIT](https://source.android.com/docs/core/runtime/jit-compiler) |
| **PyPy** | RPython translates the language interpreter and constructs a meta-tracer around annotated JIT merge points. The tracer follows the interpreter’s operations. Guards retain resume information, and the blackhole interpreter handles recovery. | Recording an operation once and specializing the executed path is the right model. PyJIT’s unrestricted C helpers, C extension reentry and reference counting make full meta-tracing a poor fit. Make only the declared graph transparent and give every opaque call a contract. [RPython tracing machinery](https://rpython.readthedocs.io/en/latest/jit/pyjitpl5.html) · [Backend and recovery](https://rpython.readthedocs.io/en/latest/jit/backend.html) |
| **Pyston v2** | The baseline translates CPython bytecodes directly with DynASM. It uses ICs, C helpers, fixed registers for VM state and deferred value-stack operations. Its source explicitly describes reconstructing the value stack for tracing. | Pyston is direct evidence for a small Python baseline with fixed-register helper conventions. Deferred ownership would introduce reconstruction at PyJIT baseline exits. The write-through policy below keeps the useful register reuse while preserving owning homes. [Pyston JIT source and calling convention](https://github.com/pyston/pyston/blob/pyston_main/Python/aot_ceval_jit.c) |
| **Cinder/CinderX** | The HIR builder translates CPython bytecode while tracking stack and locals in `FrameState`. HIR supports Python-specific operations and inlining. A separate generator lowers HIR to LIR. | Keep a recoverable Python state at speculative exits and express reference operations in optimizing IR. Rebuilding stack state throughout a baseline compiler would repeat work already done by PyJIT’s first-call lowering. [HIR builder](https://github.com/facebookincubator/cinderx/blob/main/cinderx/Jit/hir/builder.cpp) · [LIR generator](https://github.com/facebookincubator/cinderx/blob/main/cinderx/Jit/lir/generator.cpp) · [Inliner](https://engineering.fb.com/2022/05/02/open-source/cinder-jits-instagram/) |
| **CPython 3.13 and 3.14** | The instruction DSL composes stack-effect uops. Tier 2 forms and optimizes uop traces. The experimental JIT copies and patches native stencils produced with Clang at build time. The optimizer has separately authored abstract semantics. The optional 3.14 tail-calling **interpreter** uses small C handler functions and a compiler calling convention. It is independent of JIT stencils. | Build-time Clang is a viable code-production tool. Stencils do not give the tracer an inspectable semantic graph, and guaranteed tail calls do not specify what a Python frame transfer means. Treat compiler choice, interpreter dispatch and stencil production as three separate experiments. [PEP 744](https://peps.python.org/pep-0744/) · [3.13 JIT build](https://github.com/python/cpython/blob/3.13/Tools/jit/README.md) · [3.14 JIT build](https://github.com/python/cpython/blob/3.14/Tools/jit/README.md) · [Abstract operations](https://github.com/python/cpython/blob/3.14/Python/optimizer_bytecodes.c) · [3.14 interpreter](https://docs.python.org/3.14/whatsnew/3.14.html#a-new-type-of-interpreter) |
| **GraalPy and Truffle** | GraalPy switched to a bytecode interpreter, and its current bytecode DSL root uses `@GenerateBytecode` and operation declarations. Truffle can generate interpreter machinery, specialize operations and partially evaluate the interpreter with Graal. | Analyzable operation definitions can serve both interpretation and compilation. Truffle’s managed execution, invalidation and partial-evaluation machinery supply much of that result. For PyJIT the practical equivalent is a bounded typed language, generated C and explicit RC and recovery. [GraalPy bytecode root](https://github.com/oracle/graalpython/blob/master/graalpython/com.oracle.graal.python/src/com/oracle/graal/python/nodes/bytecode_dsl/PBytecodeDSLRootNode.java) · [Truffle bytecode DSL](https://github.com/oracle/graal/blob/master/truffle/docs/bytecode_dsl/BytecodeDSL.md) · [Partial evaluation](https://www.graalvm.org/latest/graalvm-as-a-platform/language-implementation-framework/) |

Shi, Gregg, Beatty and Ertl’s 2005 experiment eliminated over 47% of executed VM instructions while increasing bytecode size by about 25% in their JVM translation. The Lua 5.0 authors show why assigning expression values to registers and using compact multiformat instructions can remove moves and dispatches. Both results support doing stack simulation once in the producer. Neither measures the proposed change here. PyJIT already executes register words. Its prospective gains are better generated paths, fewer unnecessary moves, and a much simpler way to establish correctness across tiers. **There is no published cross-VM result that settles PyJIT’s accumulator choice, PIC width, or expected speedup.** [Virtual Machine Showdown](https://www.usenix.org/events/vee05/full_papers/p153-yunhe.pdf) · [The Implementation of Lua 5.0](https://www.lua.org/doc/jucs05.pdf)

## 2. Assessment of today

### What the checkout actually contains

I reproduced **343 handler records** by running `build_records(analysis, compute_formats(analysis))`. The source-instruction and IR-handler inventories together reference **373 distinct uops**. The executable-handler inventory references **367** of them. The six source-only definitions are `EXTENDED_ARG`, `NOT_TAKEN` and four fused local load/store instructions. The source-instruction inventory contributes 353 and the IR-only paths add 20. This reconciles the counts in the cutover record and gives the migration a precise denominator. Parsing the component header itself produces **438 named rows**, including 209 `HELPER`, 114 `MACRO`, 65 `PRIMITIVE` and 16 `FIELD` rows. The cutover’s “about 600” is high for this checkout. [Record builder][records] · [Cutover census and remaining tables][cutover]

| Evidence at `dd7e484be64` | Cost or asset |
|---|---|
| `_GUARD_NOS_INT` mentions an unused operand to reach the left input. The add returns its operands under new names so two later pop uops can retire them. The generated function form reads the correct registers, then carries those intermediate identities through detach and close. | The arithmetic already acts on two named values. Stack composition obscures the point at which overflow can raise with both inputs intact. [Add definition][old-add] · [Generated add][generated-add] |
| `SWAP` declares an unused middle array and two cross-moved results. `check_unused` rejects a used input below an unused stack value. The analyzer checks a `replaces` declaration against stack positions. | These checks establish properties of the notation. A parallel move of named owner tokens and an `inout` cursor need none of them. [SWAP][swap] · [Analyzer checks][stack-checks] |
| `UNPACK_SEQUENCE` declares `unused[oparg], top[0]`. List iteration declares `next_index ... replaces(index)`. `SEND_GEN` composes `_PUSH_FRAME` after an operation whose successful path already enters the generator. Stack-only superinstructions have fatal executable bodies. | Output spans, edge-specific assignments and terminal transfers deserve direct representations. Unreachable execution bodies belong outside the IR inventory. [Unpack][unpack] · [List cursor][list-iter] · [SEND and dead tail][old-send] · [Stack-only instructions][stack-only] |
| `verify_handler` composes facts by simulating a stack of value numbers. `normal_transfer_kind` still checks selected C statement shapes. The generator has object-reader tables and a release-name regex used in publication checks. | The per-uop inference fallback really is gone. There is still semantic duplication in the path from verified declarations to emitted code and cold metadata. [Handler verifier][verify] · [Transfer certification][transfer-check] · [Cutover inventory][cutover] |
| A wrong `enter` declaration on `IR_JIT_EVENT` caused debugger analysis to discard temporary origins at RESUME and backward jumps. `test_sys_settrace` went from eight known errors to 39 under `always`. Correcting the row removed the 31 new errors. | The event runs code that may itself call Python. The event word does not perform the nested call’s frame push. Named homes help, but only a correct **direct transfer** declaration fixes this bug. [Incident and correction][debug-incident] · [Debugger use of frame effects][debug-effects] |
| The decoder expands `EXTENDED_ARG`, skips inline caches, splits superinstructions and recovers exception ranges. The assembler emits word-indexed EH and a PC map. `_PyCodeIR` and `_PyCodeExec` already separate immutable and executing state. | Keep this division. Parts of `register-ir.md` still describe direct compiler emission and public word offsets. This checkout lowers from `co_code` on demand and maps public offsets back to stack bytecode. [Decoder][decoder] · [First-call lowering][lower] · [Image and PC map][image] · [Public offsets][pcmap] |
| A call head already has `dst`, callable, operand count and feedback, followed by four packed operands per payload word. Existing shape-read and method PICs already execute selected polymorphic cases. | Calls do not require every source to be in a contiguous caller window. G2 needs shared, verified PIC semantics and native lowering, not the invention of polymorphic feedback. [Call encoding][call-encoding] · [PIC nomination][pic-selection] · [PIC readers][pic-read] |
| The active interpreter is one generated computed-goto evaluator. G3a’s pin implementation and emitted switch routine exist. The production driver still takes a code reference per turn and skips eligible `RESUME_JIT` words. | Preserve the completed G3a mechanisms. Wiring ordinary transfers to the routine and admitting the declared `RESUME_CHECK` are separate work. The older interpreter design’s GCC tail-dispatch description does not describe this head. [Evaluator][evaluator] · [Driver][driver] · [Pin and switch][switch] |

A read-only probe makes the accumulator decision tangible. On the supplied release executable, `return a+b` disassembled as four public instructions and lowered to three heads. After warmup the middle head was `BINARY_OP_ADD_INT(dst=3, left=0, right=1, fb=4091)`. `co_code` stayed identical. For `return (a+b)*(b-c)`, the three arithmetic heads wrote homes 4, 5 and 4, and the multiply read 4 and 5. For `return f(x)`, the call had a head and one `IR_OPERANDS` payload. An inlined list comprehension still had executable `SWAP`s on normal and exceptional paths. These are useful first migration fixtures. The executable accepts `-X jit=off|on|always` although `--help-xoptions` omits that private option. I also checked reflected addition and the int51 overflow boundary in all three modes. [Option parser][jit-options] · [Call word length][call-length]

**What carries over unchanged in meaning.** Keep G0’s input modes `borrowed`, `consumed`, `taken`, `closed`, and result modes `owned`, `inline`, `borrowed`, `moved(x)`. Keep representation facts separate from Python type facts, named producer evidence, correlated status/results, effect classes, feedback access, per-raise `holding(...)`, restartable guard exits, declared frame transfers and generator boundaries. Retain the component implementations, review evidence and verifier obligations. Change the subjects of those checks from stack positions to named values, homes and edges. An ordinary mortal borrow is a typed view anchored by an owner. It is not an independently storable frame value. Exact large ints and NaNs can be boxed. [Contract vocabulary][vocabulary] · [G0 design][g0] · [PyValue encoding and ownership][pyvalue]

## 3. The instruction set

### Encoding and operands

Keep the common head and its existing dispatch economy.

```text
bits     63..52     51..40     39..28     27..16        15..0
field       D          C          B          A       flags | handler

Add       feedback    right      left       dst
GetAttr   name        feedback   receiver   dst
Call      feedback    count      callable   dst       + operand words
Jump      target high target low    —         —       absolute head index
ForIter   done delta  feedback   iter_pair  item
Send      done delta  feedback   send_group completed
```

The current low 16 bits are **nine handler bits and seven flag bits**. Preserve that split for ordinary heads during migration. Generate a dense private handler domain and a separate mapping to public bytecode IDs. A build-time capacity assertion controls it. An internal handler number should have no `.pyc` or `dis` significance. The current `IR_NO_DST` value is `0xfff`, outside the compact home domain. `IR_FB_SHARED` has the same bit pattern in a different operand kind. The decoder must type the field before interpreting it. [Word layout and flags][word] · [Feedback addressing][fb-address]

| Logical operand | Contract |
|---|---|
| `Src<Value>`, `Dst<Value>`, `InOut<T>` | Name homes. A source binding says whether the home survives or supplies a temporary owner. A destination may be `Discard` only where declared. `InOut` has separate entry and outgoing value identities. |
| `Local`, `Cell`, `Free` | Address Python-visible storage with its binding rules. A local read can raise `UnboundLocalError`. Reading a cell, borrowing a local and moving a local to a saved-binding home are distinct operations. |
| `ConstId`, `NameId`, `Imm<T>` | Index the code’s constants or names, or carry a checked immediate such as a comparison kind or unpack count. Give method-call intent its own logical type. Decode packed source-bytecode flags once. |
| `Feedback<K>` | Identifies a site of a declared schema. An optional entry may resolve to `NoFeedback`. Every operation must still have all its semantic operands when profiling is disabled or storage is full. Move essential operation selectors out of optional feedback. |
| `Target`, `Next` | Identify instruction **heads** of this image. Keep absolute CD for simple jumps and signed D relative to the end of the instruction for compact branches. `Next` skips the whole payload. |
| `Args`, `InSpan`, `OutSpan`, `IterPair`, `SendGroup` | Typed aggregates. Declare each element’s role, length, ownership and conditional presence. An iterator pair names receiver and cursor. A compact send group names receiver, cursor and the home reused for outgoing send input and incoming yielded value. The declaration can also bind separate homes. |

**Wide form.** Keep ordinary payload words for compact call lists. Add one private `Wide` head for an instruction whose home, index, count, displacement or operand tuple cannot fit. Its operand bits encode a checked index into an immutable per-image descriptor containing the family, logical operands with 32-bit indexes, target and any vector slice. Use a checked 32-bit index assembled from the low 32 operand bits, with the remaining bits reserved. A wide instruction has one executable head. The executing copy holds its selected arm and feedback reference in a side entry indexed by the descriptor. The wide interpreter adapter decodes to the same semantic operands and dispatches to the same arm. Baseline compilation resolves the descriptor once. Ordinary heads pay no width test beyond normal dispatch.

Choose widths and bind symbolic targets before freezing PCs. Promote out-of-range branches monotonically, then emit EH, source maps, entry tables and return continuations from the final layout. No branch, observer redirect or native entry can target a payload. Implementing wide homes also requires widening the allocator’s indexes, frame `return_slot`, EH recipients, owner programs and debugger analysis. Merely widening the decoder would leave valid Python functions failing or truncating a return destination. The checkout currently rejects some valid stack code when it exceeds the register field limits. [Assembler limits][limits] · [Frame fields][frame] · [Call payloads][call-encoding]

**Accumulator decision.** Use explicit sources and destinations at every instruction. The translator can call its top symbolic value `tos`, and the baseline allocator can remember the result of the preceding head. Neither needs a distinguished interpreter home. In the probed multiply, both intermediate results must survive simultaneously. In the probed loop, the add already reads and writes the accumulator *chosen by the Python program*, the local `total`. A second architectural accumulator would add transfer and recovery rules without shrinking either compact head. Measure reusable predecessor results and avoided loads when implementing G6. A physical-register preference can change without changing the ISA.

### Instruction inventory

These are families with one generic definition and zero or more arms. An arm can narrow admitted inputs while every miss still reaches the family’s Python behavior.

| Family | Representative instructions and purpose |
|---|---|
| Values and bindings | `Const`, `ReadLocal`, `MoveLocalAndClear`, `WriteLocal`, `DeleteLocal`, `Dup`, `Move`, `Drop`, `CellGet/Set/Delete`, `MakeCell`, `CopyFreeVars`. Make ownership and unbound values explicit. Parallel moves are an edge operation with a cycle-safe lowering. |
| Names, namespaces and functions | `GlobalGet/Set/Delete`, `NameGet/Set`, `ImportName/From`, `MakeFunction`, `SetFunctionAttribute`, `LoadBuildClass`. Preserve mapping hooks, fallback to builtins, closure ownership and arbitrary Python reentry. |
| Scalars and protocols | `Add`, other binary and in-place operations, unary operations, `Compare`, `Identity`, `Contains`, `Truth`, conversion and formatting. Keep operation identity immutable even if `Add` and `InplaceAdd` select the same exact-int arm. |
| Attributes and items | `GetAttr`, `GetMethod`, `SetAttr`, `DelAttr`, `LoadSpecial`, `GetSuperAttr`, `GetItem/SetItem/DelItem`, slice operations. `GetMethod` has two named destinations, callable and self-or-NULL. |
| Aggregate construction and unpacking | `BuildTuple/List/Set/Map`, `ListAppend`, `SetAdd`, `MapPut`, `Extend`, `Merge`, `UnpackExact`, `UnpackEx`. Name container, elements and output destinations. Keep display construction, mapping update and call-keyword merge distinct where their exceptions differ. |
| Branches and loops | `Jump`, `BranchTruth`, `BranchNone`, `CompareBranch`, `GetIter`, `ForIter`, `GetAIter`, `GetANext`, `EndAsyncFor`. Each edge declares its own definitions and owner retirements. |
| Calls and frame control | `Call`, `CallKw`, `CallEx`, `Return`, `Resume`, `Poll`. Exact Python, bound method, generator creation and native calling conventions are arms with different verified outcomes. |
| Suspension and delegation | `GetAwaitable`, `GetYieldFromIter`, `Send`, `Yield`, `CleanupThrow`, generator creation and completion. Name the input recipient, yielded recipient, completed recipient and real continuation. |
| Exceptions and context managers | `Raise`, `Reraise`, `EnterExcept`, `LeaveExcept`, exception matching and exception-group operations, `WithExceptCall`. EH edges carry the preserved context and optional `lasti`. |
| Pattern matching | `MatchMapping`, `MatchSequence`, `MatchKeys`, `MatchClass`, length tests and branches. Distinguish mismatch, result, and Python exception. |
| Runtime operations and observation | Typed intrinsics for rare operations, periodic work, instrumentation entry and observer exits. Every intrinsic has a row. A transparent wrapper refers to the immutable underlying head and its selected edge. |

### First-call lowering

Keep the stack decoder. Have it produce a CFG with a symbolic stack **only in the frontend**. At block entry, assign identities to incoming values and saved contexts. Each decoder rule binds these identities to named register operands and successor edges. At a join, choose block-parameter homes and insert parallel copies only where predecessors disagree. Track owner occupancy separately from future reads. A temporary with no future read can still owe a finalizer. Preserve the existing `regcfg` passes where they prove these conditions. Lose a fold whenever evaluation order, reentry, observability or an exceptional edge would change. [Existing lowering sequence][decoder] · [Current canonicalization][regcfg]

| Construct | Required lowering |
|---|---|
| **Calls and argument windows** | Evaluate callable, optional receiver and arguments in Python order. Capture a local into an owning temp when a later expression could rebind it. Keep the current packed source list, including distinct `ConstId` for keyword names and explicit absence for self or kwargs. A proven consecutive span may use an `Args.range` binding. The callee receives a contiguous initialized parameter window. Move temporary owners, duplicate surviving local owners, and create enough owners for repeated argument values. `CallEx` performs iterable and mapping conversion with its own error checkpoints. Save the continuation after every payload word. No mandatory caller-side shuffle. |
| **Unpacking and comprehensions** | `UnpackExact(src, out)` takes an ordered destination vector or span. The generic iterator arm collects into rooted scratch, detects too few or too many items, and cleans partial results on error. `UnpackEx` additionally produces the starred list. Keep later Python target stores as operations whenever a target can run code or raise. `a, obj.x = it` must permit `a` to be set before `obj.x` raises. An inlined comprehension gets named saved-local, iterator, cursor, collection and item homes. Its normal and exceptional exits restore the saved outer binding, including unbound. Lower bookkeeping `SWAP`s to renames or parallel moves. In a generator expression evaluate the outer iterable at creation as the language requires. |
| **Exception tables, `with`, `async with`** | Translate the source protected interval, target, depth and `lasti` bit into a handler-head target, preserved home identities, homes to retire, exception recipient and optional `lasti` recipient. Keep distinct recipes for an operation error and a later observer error after publication. For `with`, preserve lookup of `__exit__` before `__enter__`, retain the exit callable and optional receiver, and establish the protected body only after entry succeeds. A normal exit calls it with three `None`s. An exceptional exit passes the exception triple and tests the returned truth value, which can itself raise. For `async with`, await entry and exit separately and preserve exception state through each suspension. Retain traceback and chaining if exit raises. [Context-manager producer][with-code] · [EH assembly][eh] |
| **Generators, `yield`, `send`, `yield from`** | A generator call creates the generator-owned frame. A `Yield(out, resume_dst)` suspends with the successor and `resume_dst` stored in that frame. `Send` has one edge for a yielded value and another for a completed result. A generator child can satisfy either later through its saved continuation. `yield from` delegates `send`, `throw` and `close` through the relevant protocol, including a non-generator subiterator. `StopIteration.value` feeds only the completion edge. An uninitialized `StopIteration.value` maps to `None` on the audited path. Preserve `GeneratorExit`, coroutine reuse and exception-to-`RuntimeError` conversion where required. Retain optimized delegated boundaries only with a proof for every connected consumer. [SEND contract][old-send] · [Completion routine][complete] · [Cutover fixes][cutover] |
| **Pattern matching** | Preserve subject evaluation once, sequence and mapping eligibility, order of key evaluation, class checks and `__match_args__`. Model `MatchKeys` and `MatchClass` as success with owned captures, mismatch, or raise. Keep `AttributeError` handling where the class-pattern protocol requires it. Preserve duplicate-key and invalid-class errors. Lower OR-pattern joins with the same named bindings, and execute the guard after successful binding. Reuse current binding behavior on failed matches without promising stronger rollback than Python specifies. [Current matching operations][match] |

### Feedback, quickening and frame state

Allocate immutable words, width descriptors, source maps and EH data once per lowered code object. Allocate executing heads, counters, mutable IC entries and native units under its exec. A frame pins its executable code. For an ordinary head, quicken by replacing the handler ID while preserving flags, operands, length and public PC. For `Wide`, update the selected arm in its side entry. Monitoring can replace the executing selection while retaining the underlying arm.

Start with the existing **16-byte feedback header, 48-byte attribute entry and lazily allocated 120-byte four-row poly table**. Keep the current eight-byte offset units, shared header for overflow, backoff counter and `COLD/MONO/POLY/MEGA/OFF` vocabulary. Make each schema declare its fields, its allowed writers, whether a field is required for generic execution, and how a prediction is validated. `NoFeedback` selects generic execution. Failure to allocate optional PIC storage leaves the Python operation executable. [Feedback structures][feedback] · [Existing state transitions][pic-selection]

Publish a complete cache recipe before making its arm selectable. On the selected GIL build, prohibit callbacks in that publication interval. A recipe contains the arm kind, the receiver/type/version guards, the layout-specific guard, and its offset or guarded payload. A guarded object payload carries no owning edge. Read it only after establishing the live owner and matching versions. A compiled site may bind a scalar guard and offset together if the nomination proves their relationship for the site’s immutable name. Otherwise read a coherent live row. Never combine a compiled offset with a newly loaded, unrelated version. No raw poly-row address survives reentry or table growth. A guard miss runs the generic operation once. Mark compiled code stale on an **executed incompatible plan change**. A site alternating among already admitted PIC rows should not force repeated recompilation. Keep observation retirement independent of staleness. [G0 staleness rules][stale] · [Current resolver][resolver]

The register file remains `frame->base`, with locals/cells/frees, hidden constant homes, discard storage and temporaries. Every physical home is initialized. At every executable baseline head, each live language value has an owning canonical home, every vacant home contains VM NULL, and a declared internal role may contain its cursor or other permitted control value. Python `None`, absent binding, a discard recipient and a tagged cursor are distinct. Code, function, current frame, return recipient, continuation, exception state and generator state are part of recovery. Reentry through C creates a new evaluation activation. Suspending a generator retains its owned frame storage. [Frame definition][frame] · [Image layout][image] · [PyValue rules][pyvalue]

### Debuggers and public bytecode

Keep `co_code`, public opcode IDs, `.pyc`, source locations and public exception data in stack-bytecode coordinates. Ordinary `dis` reads that representation. IR disassembly remains a separate diagnostic, with pristine and adaptive views. A private encoding change alone requires no `.pyc` magic change when no private IR is serialized. Test an unchanged `.pyc` in both runtimes. [Public disassembly][dis] · [Offset conversions][pcmap]

Generate a cold map for each head and observable edge giving `(public offset, semantic origin, phase, actual successor, home transfer, EH context)`. Include source-only spans and zero-width source events where needed. Machine PCs map to heads or to an explicitly named in-word checkpoint. Use the public offset for `f_lasti`, traceback and monitoring arguments. Use the actual semantic origin and phase for unwinding. A folded load’s public offset can map to the next head without making that head a valid debugger destination.

For `f_lineno`, run the existing stopped-frame plan against the generated edge data. Require an allowed event, compatible block/handler and suspension context, and a proof of provenance for each required incoming temporary and saved binding. Plan all moves and acquire necessary owners before committing. Publish restored bindings, target head and redirect generation before retiring surplus owners. Recheck after reentry. A stale pending call or yield continuation must honor a successful redirect. Reject an unsupported jump with the established Python error. A matching line number or an initialized slot alone proves too little. [Debugger planner][debug-plan] · [Known regression][debug-incident]

On enabling `sys.settrace` or `sys.monitoring`, retire affected native entries and links. Active units acknowledge the change at the next declared reentry, poll, resume or backedge check. Generate observed interpreter paths for LINE, call/return, branch, raise/handled/unwind, yield/resume and applicable C-call events. Preserve event values, ordering and callback-error origin. For opcode tracing, I recommend one instruction event per admitted semantic head mapped to a public offset, with no event on operand payloads. Preserve source line events across folds. This is an explicit granularity choice for the owner.

## 4. The semantics language

Add a private register definition source, for example `Python/ir_ops.def`. Keep a separate public stack-opcode schema for compiler, marshal, `dis`, stack-effect queries and decoding. A register definition contains a **family interface**, **encoding**, **feedback schema**, **arms**, **component calls**, and **terminators**. Its body is a small typed CFG with immutable intermediate values, named mutable homes, checked operations and explicit exits. Arbitrary C belongs behind a reviewed component row.

The examples below are proposed source notation. Names such as `publish` and `complete_send` denote specified operations that expand into the graph for every target. Bracketed lists bind actual operands. Capitalized checkpoints are part of the definition and go into the generated metadata.

### Ownership and execution rules

- `use(x)` reads a value. `dup(v)` creates an owner. `move(h)` takes the owner and marks its source vacant. A `Src` can bind to a surviving local or a consumable temp. `acquire(x)` duplicates the former and takes the latter. A borrow may cross a call only with an independent live anchor that the call cannot invalidate. Heap-interior addresses have a shorter lifetime and must be reacquired after a heap kill.
- `publish({dst=v,...}, retire=[...])` is a declared parallel home update. Evaluate source identities and obtain output owners first. Collect displaced owners once per **physical home**, clear consumed inputs, install all outputs, establish the declared phase, then close the saved owners in the declared order. Two live output obligations need two owners even if their bits match. `dst==src`, repeated inputs and `Discard` are first-class bindings. A surviving local rebound by a callback is read afresh when its *current* value is displaced. A delayed close cannot overwrite that rebinding.
- A guard may `miss` only before any language-visible effect, allocation, duplicated owner, move, publication or outstanding temporary obligation. The generic arm receives the original operands. `again` is a separate adaptive redispatch after permitted feedback changes and before performing the operation. After a commit, continue at a typed label or report the declared error. An effectful descriptor, call or arithmetic overload executes once.
- Checkpoints name the frame, semantic origin, successor if completed, held owners, vacant homes, published outputs and exception context. A PRE owner pattern can occur after a Python call has raised. It describes cleanup and **does not authorize replay**. The verifier tracks restartability independently of the home pattern.
- `effects(...)` uses the existing coverage lattice. Source and destination roles declare ordinary operand reads and home publications. Examples explicitly include publication writes and potentially reentrant releases in the enclosing arm. `runs_python` covers arbitrary Python effects, `polls` and `switches_frame` retain their separate obligations. A result’s representation and its ownership are separate promises.

### 1. A guard

```text
guard InlineInt(x: Borrow<Value>)
    effects() feedback() exits(pass, miss)
    pass_facts(x: inline_int)
{
    if PyValue_IsInlineInt(x) -> pass
    else                      -> miss
}
```

`PyValue_IsInlineInt` has a declared `TEST(inline_int | rest)` row. The verifier checks the success refinement against that row. An exact `PyLong_Type` alone is insufficient because an exact int can be boxed. A pair of these guards can use the existing declared conjunction lowering if their failure continuations agree and no effect intervenes. [Primitive row][int-row]

### 2. Integer add and its generic arm

```text
family Add(dst: Dst<Value>, x: Src<Value>, y: Src<Value>, fb: Feedback<Binary>)
    encoding ABCF(dst, x, y, fb)
    success next(def dst: owned, consume_temps [x,y])
    errors  BoxFailed(holding [x,y]), PythonFailed(retired_temps [x,y])

arm Int of Add region(inline)
    effects(writes, allocates, raises, releases) feedback() exits(next, miss, raise)
{
    InlineInt(use(x)) else miss
    InlineInt(use(y)) else miss
    a = int_scaled(use(x))
    b = int_scaled(use(y))
    checked_add_i64(a, b)
      fits(s)     -> finish(from_int_scaled(s))
      overflow    -> {
          z = box_i64(unscale(a) + unscale(b)) else raise BoxFailed
          goto finish(z)
      }
    finish(z: Owned<ExactInt>):
      publish({dst=z}, retire=[y,x,old(dst)])
      next
}

arm Generic of Add region(shared)
    effects(runs_python, raises) feedback(read(entry), write(entry), write(handler))
    exits(next, again, raise)
{
    adapt_before_use(fb, Add, x, y) else again
    args = anchor_for_call([x,y])
    z = value_binary_op(NB_ADD, args.x, args.y)
    if error(z):
        retire_failed_call(args, [y,x], preserve_exception)
        raise PythonFailed
    publish({dst=z}, retire=[args, y, x, old(dst)])
    next
}
```

`anchor_for_call` holds the evaluated operands across reflected methods and writable `f_locals`. It reuses a proven stable owner or acquires an independent one. Its named cleanup consumes each acquired anchor once. On the fast path the scaled signed overflow test is exactly the int51 range test. The sum of two decoded int51 values fits signed 64 bits, so the overflow continuation boxes the exact sum. On allocation failure the add has neither detached its inputs nor replaced `dst`. At `finish` the common fact is exact int. Only the fitting edge proves an inline result. `InplaceAdd` can share this arm while retaining `NB_INPLACE_ADD` on its generic path. [Current arithmetic][old-add] · [Boxing and generic helper contracts][binary-row] · [Generated generic cleanup][generic-stub]

### 3. Instance-value attribute load

```text
family GetAttr(dst: Dst<Value>, obj: Src<Value>, name: NameId, fb: Feedback<Attr>)
    success next(def dst: owned, consume_temps [obj])
    generic PythonGetAttr(obj, name) -> owned | raise AttrFailed

arm InstanceValue of GetAttr region(inline)
    effects(reads, writes, releases)
    feedback(read(entry.type_version), read(entry.offset)) exits(next, miss)
{
    row = admitted_instance_slot(fb, name) else miss
    o = guard_object(use(obj)) else miss
    guard_current_nonzero_type_version(o, row.type_version) else miss
    guard_managed_inline_values_valid(o) else miss
    v = borrow_value_slot(o, row.offset)
    guard_present(v) else miss
    result = dup(v)
    publish({dst=result}, retire=[obj, old(dst)])
    next
}
```

Nomination proves that generic lookup for this **type and name** admits the instance slot. It accounts for descriptor precedence and custom attribute hooks. Execution proves the nomination still holds, then checks current storage and presence. A null slot goes through ordinary Python lookup, including missing-attribute behavior. Acquire the result before releasing the receiver, even if both designate the same object or `dst` aliases `obj`. `GetMethod` uses the same admitted value-read component when appropriate, but declares both its callable and nullable receiver results. [Instance-value arm][instance] · [Generic method lookup][attr-generic]

### 4. Polymorphic attribute load

```text
arm PolyShape of GetAttr region(shared)
    effects(reads, writes, releases)
    feedback(read(entry), read(poly_rows)) exits(next, miss)
{
    o = guard_object(use(obj)) else miss
    tv = nonzero_type_version(o) else miss
    rows = live_shape_rows(fb, name, min=2, max=4) else miss
    for row in rows bounded(4):
        if row.kind != ShapeValue or row.type_version != tv: continue
        if !valid_inline_values(o) or current_shape(o) != row.shape: continue
        v = borrow_value_slot(o, row.offset)
        if absent(v): continue
        result = dup(v)
        publish({dst=result}, retire=[obj, old(dst)])
        next
    miss
}
```

The search has only read effects until a row succeeds. `live_shape_rows` checks the site state and schema before exposing the rows. No callback may intervene between guard, slot access and `dup`. Begin with the existing nomination rule, including distinct type versions for its admitted shape rows. The full-key loop also supports a later extension to two shapes of one type once the nomination and invalidation tests cover it. Keep method-descriptor rows in their own typed recipe, including the no-instance-dictionary or valid-values/shadowing checks. A fifth distinct observation enters megamorphic policy. Start with the generic stub and backoff there.

G2’s first native version should call this shared bounded probe. The next candidate may inline the first one or two proven guard sequences and use the same shared suffix. Collect first-row hit rate, total probes, generic misses and bytes per site before choosing how many rows to inline. [Current admission rules][pic-selection] · [Current method and shape probes][pic-read]

### 5. Exact-args Python call

```text
family Call(dst: Dst<Value>, fn: Src<Value>, self: OptionalSrc,
            args: Args, fb: Feedback<Call>)
    continuation caller(site, next_head, dst)
    outcomes next(result: owned), enter(child), raise(checkpoint)

arm PythonExact of Call region(split)
    effects(reads, writes, allocates, releases, switches_frame, raises)
    feedback(read(entry)) exits(miss, enter, raise)
{
    guard_default_eval_frame(tstate) else miss
    f = guard_plain_python_function(use(fn), fb.func_version) else miss
    guard_ordinary_code_and_exact_positional_binding(f, self, args) else miss
    guard_frame_ready_and_arena_space(tstate, f.code) else miss
    guard_recursion_budget(tstate) else miss

    child = reserve_exact_frame(f.code) else raise ReserveFailed(caller, holding operands)
    bind_exact(child, fn, self, args)          # move temps, duplicate survivors
    save_continuation(caller, site, next_head, dst)
    link_and_enter(child, caller)             # publish chain and debit recursion once
    start_frame(child) else raise StartFailed(current=child)
    enter(child, child.entry_head)
}
```

This arm admits an ordinary Python function with the verified entry and binding shape. A generator function has a creation arm. An expanded bound method also needs guards and owners for its function and receiver. `bind_exact` is an infallible commit after reservation for the admitted shape. It must publish a completely initialized child before any callback-capable retirement that could expose it. Keep the callable and code owners distinct because `function.__code__` can change. An allocation or first-call preparation that can reenter belongs before the guarded fast transaction or in an explicit slow continuation with revalidation. The simple unchecked arena path can prove `ReserveFailed` unreachable and omit it. Once binding has started, error cleanup owns the failed transaction and cannot call the target a second time.

The generic family implements positional and keyword binding, native/vectorcall bridges, non-callable errors and PEP 523 hooks. A native completion publishes the result in the caller and performs the declared end poll. A Python entry resumes at the child’s real first head, which may initialize cells before RESUME. [Existing guards][exact] · [Argument ownership][bind] · [Frame-transfer order][g3-order]

### 6. `ForIter` over a list

```text
family ForIter(item: Dst<Value>, it: IterPair, done: Target, fb: Feedback<Iter>)
    body next(keep it.receiver, def it.cursor, def item)
    exhausted branch(done, retire it, preserve previous item binding)

arm List of ForIter region(split)
    effects(reads, writes, releases, runs_python, raises)
    feedback() exits(next, branch, miss, raise, redirect)
{
    list = guard_exact_list(use(it.receiver)) else miss
    i = guard_index_from_getiter(use(it.cursor)) else miss
    if unsigned(i) >= current_length(list):
        complete_for(it, done, value=None)
          -> branch(done) | raise CompletionError | redirect(published_head)

    v = dup(borrow_published_list_element(list, i))
    publish({it.cursor=next_tagged_index(i), item=v}, retire=[old(item)])
    next
}
```

The list owner survives on the body edge. Reload its length each step. A Python loop may mutate the list between steps. The successful read and duplication take place in a callback-free interval under the GIL. Exhaustion removes the iterator and cursor, preserves the last item binding, and invokes the declared completion/observation protocol. The ordinary unobserved exact-list exit can narrow that protocol to a direct branch and retirement. A producer/consumer proof can eliminate the explicit cursor tag test. An arbitrary tagged integer is insufficient evidence for an unchecked element access. [Current list iteration][list-iter] · [Completion implementation][complete]

### 7. `Send` to a generator

```text
family Send(receiver: Src<Value>, cursor: InOut<CursorOrNull>, sent: Src<Value>,
            yielded: Dst<Value>, completed: Dst<Value>, done: Target, fb: Feedback<Send>)
    yielded_edge   next(def yielded, keep receiver and applicable cursor)
    completed_edge branch(done, def completed, retire delegation inputs)
    child_edge     enter(child, waiting=SendContinuation)

arm Generator of Send region(shared)
    effects(reads, writes, releases, switches_frame, raises, runs_python)
    feedback() exits(miss, enter, raise)
{
    guard_default_eval_frame(tstate) else miss
    g = guard_exact_generator_or_coroutine(use(receiver)) else miss
    guard_null(use(cursor)) else miss
    guard_created_accepts_only_none(g, use(sent)) else miss
    guard_claimable(g) else miss
    claim(g) else raise ClaimFailed(holding inputs)
    save_send_continuation(parent, site, next_head, yielded, done, completed)
    v = acquire(sent)
    link_generator_and_exception_state(g, parent)
    publish_current_and_account_recursion(g.frame)
    install_resume_input(g.frame, v) else raise ResumeFailed(current=g.frame)
    enter(g.frame, g.frame.resume_head)
}
```

`claim` is the commit point. On the supported GIL path the successful guard makes the claim a store. The declaration can cover a claim-failure path for a build whose claim can fail. The generator receives the sent owner, or discards it at its declared resume recipient after publishing the new current frame and exception state. A child `Yield` suspends and delivers to `yielded`. A child return delivers to `completed` and branches to `done`. The continuation records which owner changes have already happened. It never decodes a later `POP_TOP` to find out who owns a value.

The generic arm handles protocol receivers and virtual cursors and discriminates `NEXT(value)`, `RETURN(value)` and `ERROR`. `CleanupThrow` has its own error and completion recipe. The observation path distinguishes fallible PRE observation with original owners from POST observation with committed outputs. A callback that redirects or raises must not resume the abandoned continuation. These are precisely the cases hidden by a dead `_PUSH_FRAME` tail. [Current SEND variants][old-send] · [Delegation routines][delegate] · [PRE/POST completion][complete]

### 8. `Return`

```text
family Return(value: Src<Value>)
    outcomes return(caller, successor), exit(activation, value), raise(checkpoint)

arm Ordinary of Return region(split)
    effects(writes, releases, runs_python, switches_frame, raises)
    feedback() exits(return, exit, raise, redirect)
{
    publish_origin(frame, site)
    pin_departing_after_last_guard(activation, frame)
    result = acquire_heap_safe(value)
    dying = frame
    caller = unlink_and_leave_recursion(dying)
    restore_generator_exception_state_if_needed(dying, caller)
    retire_frame_holding_result(dying, result)

    route = current_return_continuation(caller)
    complete_return(route, result)
      ordinary(succ) -> return(caller, succ)
      external(v)    -> exit(activation, v)
      redirected(h)  -> redirect(h)
      failed(cp)     -> raise cp
}
```

The result remains rooted while the dying frame is cleared. Unlink the frame **before** releasing anything it owns. A finalizer during retirement sees the caller current and can change its control generation. Resolve that before writing a stale recipient or continuation. On an ordinary return, install the result before closing the caller’s displaced owner. On generator return, completion can fail in the **consumer** frame after the child has retired. The error route must use that frame and its SEND/FOR origin. The execution pin is a separate code owner. Install the new pin before releasing the previous one, and invalidate cached home facts across a reentrant release of the old pin. [Return implementation][return] · [G3 transfer order][g3-order]

### Verification and the helper boundary

Retain `ir_contract.py`’s vocabulary, fact lattice, effect coverage, row parsing, C signature checks and the intent of V1–V13. Replace its C token walk and simulated operand stack **for ported families** with a worklist over typed blocks. At a join, owner obligations and checkpoint shape must be compatible, value identities get block parameters, and facts intersect. A verifier can compute what the body requires to *reject* a declaration. It cannot grant an undeclared effect, fact, exit or optimization.

| Check | Implementation rule |
|---|---|
| Names, types and feedback | Every call, field read, indirect slot target, cache field and terminal has a declaration. Check argument and correlated result types, nullability, allowed feedback access and field lifetime. Check all supported build branches. Unknown constructs fail generation. |
| Ownership | Track linear owner tokens separately from value identities and home addresses. Check anchor lifetime, repeated operands, aliasing, span initialization, partial failure, displaced owners and moves into children. Each exit must account for every token. `moved(x)` carries the surviving facts of that value. |
| Guards, facts and effects | Verify the guard predicate against a `TEST`, `MEASURE` or explicit contextual guard. A contextual guard grants only its declared control permission. Heap writes, releases, polls, reentry and frame switches kill the relevant heap facts. A mutable local can also lose its **value identity** across reentry. An independently rooted value retains its intrinsic representation facts. |
| Restartability and exits | Check guard misses before all commits, including generated publication and closings. Require an explicit target and frame per terminating path. No implicit fallthrough after `enter`, `yield` or `return`. Verify per-site raise checkpoints, direct versus nested frame switches, declared pc writes and completion targets. |
| Families, regions and products | The specialized arm implements the family’s external outcomes on its admitted domain. A miss is identity on program state. Every inlined node has a tested lowering. A split transfers only the live typed values in its continuation descriptor. Check all generated products, including monitored arms, cap fallbacks and generator-boundary expansions, against the same contract. |

**Keep `pycore_ir_components.h` and strengthen it.** Many rows already have C parameter and result types. Add logical types for homes, SR​​C/destination roles, cursors, site-bound feedback fields, guarded heap paths, continuation signatures and register/clobber descriptors. Give each C escape a signature, argument modes, result or status relation, effects, exits, callback/PC publication needs, build domain and evidence. Keep `PRIMITIVE`, `HELPER`, `ROUTINE`, `FIELD`, `SLOT`, `DESTRUCTOR`, `CONSTANT` and checked intrinsic rows. A composed stub receives a declared region and ABI descriptor with a mechanically checked projection of its graph.

Move the semantic parts of `IR_MARKER_EFFECTS`, `IR_HELPER_EFFECTS`, `IR_HELPER_CALLEES`, observer `SIGNATURES/HELPERS`, `TRANSFER_TOKENS`, `GUARD_TOKENS`, view/read and poly tables into those declarations. Convert `normal_transfer_kind` text pins into checked transfer nodes. Replace `RELEASE_LIKE` in publication checks with the release effect. Retain purely syntactic handling for the public stack DSL while the compiler and tooling need it. A C signature assertion proves neither ownership nor effects of an arbitrary C body. The evidence comment, component review and adversarial runtime test remain part of that trust boundary. [Verifier entry points][verify] · [Component schema][components] · [Cutover’s table-by-table audit][cutover]

## 5. Generated targets from one source

**Interpreter.** Generate C labels and computed-goto dispatch for the measured configuration. A compact handler binds fields directly, with no generic decoded-instruction allocation in its hot path. An uncommon wide adapter resolves a descriptor. Ordinary C control flow implements graph edges. Shared runtime routines handle complex calls, unwind and suspension. Generate function forms from this same graph for cold uses, coverage and mixed migration. An instruction’s graph carries exactly the same semantics when its dispatch terminator is bound to a C label, status return or native branch. [Current evaluator][evaluator] · [Generated function form][generated-add]

**Compiler choice.** Keep GCC 15.2 as the selection control. A separate Clang experiment can generate one same-signature C function per handler with `musttail` on dispatch and `preserve_none` on the internal convention. `musttail` makes failure to generate a required tail call diagnosable. `preserve_none` changes register preservation and parameter allocation. Neither fixes the registers for a mixed-compiler private ABI without a checked adapter. GCC 15 added `musttail`. The local GCC 15.2 preprocessor reports `__has_attribute(musttail)==1` and `__has_attribute(preserve_none)==0`. GCC 16 documents x86 `preserve_none` and warns that its parameter convention can change. A claim that GCC categorically lacks either feature is out of date. [GCC 15 changes](https://gcc.gnu.org/gcc-15/changes.html) · [GCC 16 x86 ABI](https://gcc.gnu.org/onlinedocs/gcc-16.1.0/gcc/x86-Attributes.html) · [Clang attributes](https://clang.llvm.org/docs/AttributeReference.html#musttail)

Build-time Clang stencils are a second, independent experiment. They could reduce assembler maintenance for an opaque cold region or improve a fixed-register stub. They cost a pinned build toolchain, object/relocation extraction, target-specific fixups, reproducibility checks and unwind validation. They also obscure the structure the tracer needs if used as the semantic source. Generate them from the verified graph and retain the graph. No LLVM runtime dependency. Compare GCC/computed-goto, Clang/computed-goto and Clang/tail-call at the same revision and settings. For stencils, hold the host compiler constant and change only the stub producer. Rebuild the control under any proposed new compiler before attributing a difference to the ISA. Upstream’s 3.14 results are comparisons against its own Clang configuration. [3.14 dispatch comparison](https://docs.python.org/3.14/whatsnew/3.14.html#a-new-type-of-interpreter) · [Stencil toolchain](https://github.com/python/cpython/blob/3.14/Tools/jit/README.md)

**Baseline inline templates.** Feed the verified leaf subgraph directly to `jit_template_generator.py`. Bind operands, source modes and destinations at compile time. Lower through the existing closed set in `jit_lower.c` and `jit_asm.c`. Generate guard fusion from declared predicate identities, CSE only through permitted reads, and dead-test removal from facts on the actual edge. Emit checked arithmetic and publication into the selected registers. Outline boxing, miss, error, final release and retirement paths. A new machine primitive needs a C reference, a typed model, differential tests and a lowering. An operation that calls an unmodeled helper gets a generated stub call. It does not acquire a handwritten opcode emitter. [Leaf representation][leaf] · [Template generator][templates] · [Generated add template][native-add]

**Out-of-line stubs.** Use the G3 native boundary convention on x86-64 SysV. `r12=tstate`, `r13=frame`, `r14=word0`, `r15=base`, `rbx=unit`. Preserve the native frame chain, 16-byte call alignment, common spill reservation and unwind rows. Generate a fixed entry descriptor for each shared region. At a whole-word site, supply `rdx=&head` and the pinned registers. A shared ABI thunk can supply `rdi=r12` and `rsi=r13` and tail-jump to the ordinary GCC C stub returning `int` in `eax`. Put that conversion once per shared entry if it beats G1a’s existing three-argument site. Both versions obey the same descriptor. Never ask C to read an undeclared register local. Split regions add a generated, typed spill-area descriptor for live values and owners. It is dead at every word and frame transfer. [Current native register convention][switch] · [G1a ABI][abi]

| Completion | Native action |
|---|---|
| `0`, `BRANCH=8` | Continue at the next head or static target. A branch site may test the common branch outcome directly. |
| `MISS=1`, `AGAIN=16` | MISS alone runs the generic stub on untouched operands. AGAIN redispatches the same unexecuted language operation after feedback work. |
| `ESCAPED=2` | Qualifies an allowed same-frame outcome. Test retirement and continue or leave at the successor that outcome names. A last-reference release can set it. |
| `RAISED=4` | Unwind from the declared checkpoint and current frame. A completed call or child return cannot be replayed. |
| `SWITCH=32` | Adopt the G3 extension. The switching path has published `tstate->current_frame` and its successor. The switch routine reloads the pinned registers and returns the target for a per-site `jmp`. `SWITCH` subsumes the retirement test for the departing unit. |
| `redirect`, `exit` | Bind first to existing cold transfer forms. When converting the last such form, reserve distinct standalone statuses for a published dynamic same-frame head and an activation exit, respectively. The descriptor names where the target or external result lives. An arbitrary debugger target cannot fit `BRANCH`’s static-target meaning. |

A compiled Python call and return use one native activation. The small machine `call switch; jmp rax` sequence returns from the switch routine before jumping into the selected unit. It does not leave a native return address per Python frame. A cold, observed or retired target can resume in the interpreter. A Python call made *through C* legitimately creates another activation. Execution pins keep a departing unit alive through last-reference destruction, callbacks and its final native instructions. [G3 switch design][g3] · [Driver and pin lifecycle][driver]

**Tracer.** Keep the earlier survey’s single recorder in the interpreter. Transfer a hot baseline loop to its header, record the actually executed edges, arm, call target and completion, then expand the same graph into SSA. Keep `home → value`, owner tokens, heap effects and logical frames separate. A guard refines its successful value. A recorded branch becomes a path guard. A loop header joins incoming values with phis. An opaque row becomes a call with declared effects, clobbers, results and exception edges. For a recorded PIC hit, emit the selected row’s actual guards or exit to the family head before commit. Observing a row at recording time proves nothing about its validity at execution time.

A snapshot names a head or typed continuation, logical frame chain, source origin, owners, exception state and before/after-effect phase. A guard before `__add__` can leave at ADD. An exit after `__add__` must preserve its returned owner and resume at the completed successor. Initially keep boxed owners wherever exit materialization could otherwise allocate. Add fallible materialization only with rooted intermediates and an OOM checkpoint. Start by materializing before all opaque or reentrant calls. The tracer can eventually defer home stores and cancel RC pairs with recovery proofs. [Earlier tracing design][trace-design]

**G6.** At baseline, a machine register may cache `(frame, home, value-version, bits, proven facts)` after a write-through store. It owns no additional reference. A direct fallthrough can use it to skip a load or repeated guard. An aliasing store kills or replaces its entry. Calls, finalizer paths, polls, redirects, switches, observation and external entry kill it unless an explicit descriptor proves preservation. Give each word an externally enterable path with an empty cache. A direct predecessor may branch to an internal continuation that accepts its declared cache state. Initially allow forwarding only on single-predecessor straight-line edges. All joins, backedges and mixed-tier entries can use the ordinary entry. Thus an arbitrary head always works with homes alone. Measure whether one or two caller-saved value registers pay for themselves before expanding the policy.

## 6. Migration and acceptance

Treat `dd7e484be64` as the **contract oracle**, and a frozen executable of the matching revision as the runtime oracle when implementation begins. Use the supplied older executable only for labeled probes. Freeze a census mapping each of the 373 declarations and 343 records to a new family, shared component, compatibility-only decoder rule or proven unreachable source form. Include monitored, learned-only, IR-only and generator-boundary variants. Compare normalized behavior by named edge, owner state, feedback access and error phase. A textual match of generated C is unnecessary. Any semantic difference needs a concrete source case, justification and test. The JIT-event incident demonstrates why an incorrect old row must be corrigible.

Each port changes the decoder binding, family, generated products and users of its metadata in **one unit**, and removes the old executable definition of that family in that unit. A shared old uop may remain while another unported family still uses it. A build-time ownership manifest rejects missing families and duplicate providers. During migration, generated dispatch tables can contain both kinds of family. There is one provider per family, with no runtime old/new selector. Golden binaries and historical records provide the independent comparison.

The sizes below are planning estimates in focused engineer-days per unit or batch, including targeted tests and review. The coordinator owns integration, measured builds and shared generator files.

| Order | Unit and owner | Size and exit condition |
|---|---|---|
| 0 | **Oracle, decoder inventory and G3 boundary review — Opus** | 1–2 days. Freeze the census, emitted plans, private/public PC conventions, known failures, and actual status of pins, switch and RESUME. Establish matching controls. |
| 1 | **Typed register graph, row types, verifier and C backend — Opus** | 3–5 days, split into independently reviewable schema, verifier and emitter changes. Admit one complete ADD family with generic protocol, reflected call, int overflow, aliasing and error cleanup. Generate C, form, template, metadata and stub from it before widening the port. |
| 2 | **Home transfers, edge state, PC/EH and wide encoding — Opus** | 2–4 days for the home/edge interface and 2–4 for wide support end to end. Port moves, locals and branches in small batches. Prove jumps around comprehension save/restore and all field boundaries, including a wide return destination. Can proceed alongside the ADD backend after operand types freeze. |
| 3 | **Scalar and container ports — Sonnet batches** | 1–2 days each, roughly 10–25 old declarations or a small coherent family per batch. Parallel batches for numeric/in-place, comparisons/truth, constants/cells, item access, construction/unpack and namespaces. Keep difficult protocols as typed C rows. Integrate only complete families. |
| 4 | **Attribute/IC protocol and native pilot — Opus, then Sonnet** | 2–3 days for instance read, generic lookup, monomorphic and bounded poly guards. Separate 1–2 day batches for stores, class/module reads, methods, super and descriptors. G2 becomes a consumer of this port. Measure ordinary and method sites separately. |
| 5 | **Frame ABI and exact Call/Return — Opus** | 3–5 days, split at pin coverage, RESUME, production switch wiring, then exact binding/return. Reuse G3a’s existing code. Keep the driver code ref until every retiring form, capped fallback and boundary variant pins correctly. Generated `RESUME_CHECK` compares breaker and instrumentation version and runs generic RESUME on a miss. Remove the driver shortcut and redundant frame lookup only after mixed-tier and observed entries pass. |
| 6 | **Remaining calls and iteration — Sonnet with Opus review at transfers** | 1–3 days per batch for positional/general calls, keywords/defaults, bound methods, native conventions, `CallEx`, list/tuple/range/dict iteration. Parallelize along family lines after the continuation ABI freezes. Preserve exact native error and polling behavior. |
| 7 | **Suspension, EH and observation — Opus for interfaces, Sonnet for bounded ports** | 2–4 days each for generator/send/yield completion, exception/with/async-with, and debugger/monitoring integration. Port matching, imports and rare intrinsics in parallel against frozen row types. Cover generator-owned frames, callbacks at PRE and POST, abrupt close/throw and ancestor jumps. |
| 8 | **Delete the stack-semantic generator and audit coverage — Opus** | 2–3 days. Remove stack binding from record/template generation, exchange and replacement verification, window inference, body-shape pins, regex authorizations, dead tails and executable stack-only handlers. Keep the public decoder schema. Audit Makefile regeneration, opcode metadata, disassembly, specialization, frame/debugger consumers and build products together. |
| 9 | **Native quality and tracing milestones — Opus for first ports** | G1c becomes per-family graph simplification and code measurement. G4 adds exact call/return inline prefixes and typed shared continuations on the proven G3 switch. G6 adds write-through forwarding. Then T0 records one loop with a real call, branch exit and exception edge using this graph. Sonnet can extend proven patterns in subsequent batches. |

G1b’s inventory should be generated from the new arm descriptors as families arrive. G1c need not wait for rare opcodes. Its historic ADD comparison was **32 instructions on one named old path**, with a target around **15 before displaced close or 21 with it**. TA separately reports a longest-path change from 34 to 28. These are different comparisons. Use the same operand homes, destination occupancy, guards, code version and counting boundary for every before/after claim. Likewise G3D’s 137 driver/entry/exit instructions in a 491-instruction call episode are an attribution, not a promised 137-instruction net saving. [Inline-arm ledger][ledger] · [Wave measurements][wave] · [G3 episode][g3]

**Correctness gates at every unit**

1. Verify the family census, reproducible generation, typed C assertions and negative verifier fixtures. Include miss after `dup`, borrowed interior pointer across a callback, missing partial-span cleanup, incorrect direct `enter`, unchecked completion target and incompatible block-parameter owners.
2. Run the linked candidate through interpreter, function forms, forced baseline, ordinary warmup, instrumented execution and applicable wide forms. Force the actual arm and exit under test. Compare results, exceptions, side-effect sequence, traceback offsets, home state and continuation. Differential comparisons can ignore approved incidental differences only if the test states which difference it permits.
3. Exercise local/temp/discard destinations, `dst==src`, repeated operands, finalizers that rebind locals or enable observation, weakrefs/cycles, allocation faults, mutation between iterations and after IC warmup, version invalidation and exceptions from hooks. For transfers, mix compiled and interpreted callers/children, nested C reentry, recursion limits, suspended generators, unit retirement and the last code reference. Verify native unwind through deep Python recursion.
4. At integration checkpoints run release and assert suites, `test_ir*`, generated-case and JIT/template/assembler tests, `test_sys_settrace`, monitoring, pdb/bdb, generators/coroutines/asyncgen and affected extension tests. Apply sanitizer and stress runs to lifetime and ABI changes. Record each known failure by test and cause. The cutover reports eight known tracing errors and a known object-reader test failure in its respective runs. A rise to 39 is a regression even if another intermediate binary also has 39. [Cutover qualification][cutover] · [Project admission rules][validation]

**Measurement points.** Record the pre-port control, ADD/attribute pilot, scalar batch, PICs, production frame switching, full interpreter migration, G4 and G6 separately. Keep the selected GCC configuration, immutable workload roster, hash and affinity matched within each comparison. Pin diagnostic commands to CPUs 12–14 as in this review. Use the owner’s established quiet timing protocol and identify its actual CPU for formal selection. Report all-29 cold and warm elapsed times, interpreter-off versus its control, JIT-on versus the matched interpreter, and `always` separately. Include per-workload ratios, first-call lowering and compilation time, executed native instructions, hot/cold bytes, stubs versus templates, PIC hits by row, generic misses, frame switches versus driver turns and retirement/recompile counts.

The published interpreter ratio **0.876** is golden87 versus upstream. The **1.022** JIT warm ratio is `a5502da133f` versus golden87 with JIT off at warmup 32. Keep nbody **0.748**, spectral_norm **0.792**, richards **1.157** and unpack_sequence **1.380** visible. Measure hot-path work before considering placement. A smaller generated source file or a favorable arithmetic kernel is insufficient evidence for selection. [Live status][status] · [Wave measurement conditions][wave]

## 7. Risks and owner decisions

| Decision | Recommendation and evidence required |
|---|---|
| **Architectural accumulator** | Approve explicit destinations and optional G6 forwarding. Reopen the encoding question only if a real instruction-density or dispatch study demonstrates a gain after accounting for moves, folded locals, observation and both JIT tiers. The present probes establish fit and dataflow. They do not measure register-cache speed. |
| **Width and compatibility** | Approve a cold wide descriptor and widen every recipient/index that crosses a frame or recovery boundary. Keep public stack serialization. Test generated Python beyond compact bounds and round-trip its `.pyc`. The wide path’s frequency and first-call memory cost are unmeasured. |
| **Meaning of a word boundary** | Define it as entry to an executable head, including independently enterable EH, resume and debugger targets. Operand data and split continuations have no public entry. Keep owning homes at every baseline head. This fixes the responsibilities of G6, unwind and entry tables. |
| **Observable behavior** | Preserve Python results, protocol order, exceptions, traceback origins, ownership, line/call/return events and safe supported debugger transfers. Approve semantic-head opcode granularity and bounded tracing-switch latency. Permit finalizer timing changes only at a documented state where the finalizer sees consistent frames, bindings and exception context. |
| **IC breadth and invalidation** | Start with existing storage and four rows. Retain the GIL qualification. Admit recipes individually. Invalidate or revalidate on type/dict/function mutation, instrumentation change and version reuse. If a version can wrap or be recycled, require the runtime’s invalidation guarantee or an epoch before binding it in native code. Defer free-threaded lock/lifetime design to its own qualification. |
| **Compiler and machine code** | Select GCC/computed-goto and the existing assembler for the first migration. Permit a matched Clang dispatch or stencil trial once the semantic pilot passes. Adopt either only with whole-suite evidence, verified ABI/unwind and tighter hot code or a measured maintenance benefit. |
| **Scope of trusted C** | Keep the 438 reviewed rows as the starting ledger. Strengthen types and add evidence where semantics cross a helper boundary. Prioritize calls, generator transitions, observer callbacks, indirect slots and guarded cache payloads. A generator can prove composition while still trusting a wrong row. The JIT-event regression is the example to retain permanently. |
| **When to resume optimization** | Resume G1c with ADD, G2 with `GetAttr`, and G3a’s production wiring with the frame contract. Start G4 after switch and pin qualification. Start G6 after independent-entry tests. Keep the optimizing method tier deferred under tiers87. The tracer’s first acceptance case should include an effectful call and a recoverable exit, not just an integer loop. [Three-phase plan][tiers] |

The first implementation review should be able to point to **one ADD definition** and answer four questions directly from it: which homes own the operands when boxing fails, what happens if `__radd__` mutates a local, where a failed inline guard resumes, and which machine instructions the fitting int path actually executes. The corresponding test for a frame transfer is equally concrete: from the `Return` definition and its rows, a reviewer must be able to identify the owner keeping the departing code alive, the moment the dying frame becomes invisible, and the frame blamed if generator completion raises. Those are the two acceptance cases I would use to authorize the remaining ports.

[records]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Tools/cases_generator/ir_generator.py:6131
[cutover]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/register-semantics59/regions89/g0d/g0d1/report.md:119
[old-add]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Python/bytecodes.c:831
[generated-add]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Python/generated_ir_cases.c.h:1071
[swap]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Python/bytecodes.c:6695
[stack-checks]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Tools/cases_generator/analyzer.py:438
[unpack]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Python/bytecodes.c:2351
[list-iter]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Python/bytecodes.c:4812
[old-send]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Python/bytecodes.c:1982
[stack-only]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Python/bytecodes.c:6893
[verify]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Tools/cases_generator/ir_contract.py:4505
[transfer-check]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Tools/cases_generator/ir_generator.py:8422
[debug-incident]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/register-semantics59/regions89/g0d/g0d1/differences.md:867
[debug-effects]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Objects/frameobject.c:1921
[decoder]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Python/flowgraph.c:4487
[lower]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Python/ir.c:920
[image]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Include/internal/pycore_ir.h:653
[pcmap]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Include/internal/pycore_ir.h:769
[call-encoding]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Include/internal/pycore_ir.h:186
[pic-selection]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Python/specialize.c:632
[pic-read]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Python/ceval_macros.h:1018
[evaluator]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Python/ceval.c:3009
[driver]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Python/ceval.c:2880
[switch]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Python/jit.c:228
[jit-options]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Python/jit.c:43
[call-length]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Include/internal/pycore_ir.h:994
[vocabulary]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Tools/cases_generator/ir_contract.py:42
[g0]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/register-semantics59/regions89/rd/design.md:62
[pyvalue]: /home/power/code/cpython/.worktrees/python-jit/pyjit/design/pyvalue.md:70
[word]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Include/internal/pycore_ir.h:136
[fb-address]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Include/internal/pycore_ir.h:220
[limits]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Python/ir_assemble.c:1203
[frame]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Include/internal/pycore_interpframe_structs.h:29
[regcfg]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Python/regcfg.c:503
[with-code]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Python/codegen.c:5461
[eh]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Python/ir_assemble.c:128
[complete]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Python/ceval.c:3958
[match]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Python/bytecodes.c:4453
[feedback]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Include/internal/pycore_feedback.h:55
[stale]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/register-semantics59/regions89/rd/design.md:997
[resolver]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Python/jit.c:583
[dis]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Lib/dis.py:221
[debug-plan]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Objects/frameobject.c:3139
[int-row]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Include/internal/pycore_ir_components.h:135
[binary-row]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Include/internal/pycore_ir_components.h:394
[generic-stub]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Python/generated_ir_stubs.c.h:159
[instance]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Python/bytecodes.c:3246
[attr-generic]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Python/bytecodes.c:3180
[exact]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Python/bytecodes.c:5589
[bind]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Python/ceval.c:1949
[g3-order]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/register-semantics59/regions89/g3/design.md:611
[delegate]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Python/ceval_routines.h:21
[return]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Python/bytecodes.c:1782
[components]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Include/internal/pycore_ir_components.h:7
[leaf]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Tools/cases_generator/ir_leaf.py:657
[templates]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Tools/cases_generator/jit_template_generator.py:197
[native-add]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Python/jit_templates.c.h:45
[abi]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/register-semantics59/regions89/g1a/abi.md:7
[g3]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/register-semantics59/regions89/g3/design.md:62
[trace-design]: /home/power/code/cpython/.worktrees/python-jit/pyjit/design/jit-tracing-research-20261101.md:66
[ledger]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/register-semantics59/regions89/rd/design.md:1076
[wave]: /home/power/code/cpython/.worktrees/python-jit/pyjit/issues/register-semantics59/regions89.md:68
[validation]: /home/power/code/cpython/.worktrees/python-jit/pyjit/design/validation.md
[status]: /home/power/code/cpython/.worktrees/python-jit/pyjit/STATUS.md:3
[tiers]: /home/power/code/cpython/.worktrees/python-jit/pyjit/design/proposals/tiers87.md