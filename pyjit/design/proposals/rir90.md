# Rir90: register instructions carry the semantics

Proposed by root on 2026-11-24. The user decided the direction on the same
day; this plan is not yet reviewed by the user. Base: regions89's final head
(`pyjit/regions89`). It replaces decision 1 of [tiers87](tiers87.md): the
statement of what an operation does moves out of the stack uops in
`Python/bytecodes.c` and into register instruction definitions. Tiers87's
baseline and tracing phases stand.

Two documents back this plan:
- Astra's [design](rir90-astra-design.md) surveys V8, JSC, SpiderMonkey,
  LuaJIT, HHVM, ART, PyPy, Pyston, Cinder, CPython 3.13/3.14 and GraalPy.
  It proposes the instruction set, the definition language and the
  migration.
- Its [addendum](rir90-astra-addendum.md) gives the grammar, the generated
  C, the instruction families (Appendix A), the porting batches and a
  migration check (Appendix B).

Their source links point to the regions89-g0d1 worktree at `dd7e484be64`.

## The model

There are two layers and nothing in between.

1. **Stack ops are transport.** `co_code`, `.pyc`, `dis` and public offsets
   stay in stack bytecode. Their only semantics are the lowering rules,
   applied once at first call:
   - each stack op becomes register instructions with named operands;
   - evaluation order is preserved;
   - values become locals or temporaries;
   - exception-table ranges and source offsets are mapped.

   `SWAP`, `COPY` and `POP_TOP` become renames, retirements or nothing. A
   `Move` is emitted only where homes cannot be reconciled.
2. **Register instructions define all behavior**: guards, ownership,
   effects, exits, raises and frame transfers. Each family, for example
   `Add`, `GetAttr` or `Call`, has a generic arm and specialized arms. An
   arm is a typed graph of guards, language primitives and calls to
   declared component rows. A piece shared between families is a typed
   macro with named parameters, with no dispatch or handler of its own.
   Nothing with stack effects sits underneath.

The 373 stack uops are deleted as their families are ported. Comparing
against them is a migration test only: each port checks that its family
still covers the exits, raises and ownership cases the old uops declared.

## Selected design

| Area | Choice |
|---|---|
| Encoding | Eight-byte heads with explicit destination and sources and no architectural accumulator. A head holds three operands and a feedback slot, so an accumulator would save no space. A cold `Wide` descriptor handles homes, counts and targets beyond the compact fields, and so removes today's "too many local variables" RuntimeError. |
| Families | About 55 families (addendum, Appendix A). A mode selector, such as a cell operation or a namespace scope, is an immutable operand that can select its own handler number. Handler numbers are private and dense. |
| Definitions | A separate typed language in `Python/register_ops/`. Torque is the model; C is not. A recursive-descent parser reuses the existing lexer. Blocks take parameters, and names are assigned once. Every call to a component row with several outcomes must name an edge for each outcome. Loops are bounded (`each` over a checked span). The `publish` statement writes several homes at once and then retires owners in a declared order. Types, effects and feedback schemas are closed catalogs. |
| Arithmetic | No C operators in definitions. Bit and integer operations are declared `PRIMITIVE` rows with width, overflow, facts, a C body and a leaf lowering. C, assembler and SSA then cannot disagree about them. |
| Rows | The 440 component rows stay as the trusted boundary to C and gain logical types for homes, cursors, feedback fields, continuations and register descriptors. F0 decides where those typed signatures live: as columns in `pycore_ir_components.h` (Astra's choice), or as Torque-style `extern` declarations in the definition files (root's preference). Either way each row has one authoritative declaration by D. The stack-era tables are folded into the rows and deleted: marker effects, helper effects, transfer tokens, guard tokens and object readers. |
| Specialization | Family adaptation macros choose arms, nominate, retry and select poly. C rows in `specialize.c` only answer evidence questions, such as whether this name on this type is an instance slot. No C code picks a handler number. |
| Observation | Line, instruction, call, return, branch, yield and resume events are generated wrappers bound to a family and arm, not separate families. Opcode tracing reports one event per register instruction, at its public offset. The standing rule allows this change in granularity. |
| Products | From the same graph, the generator emits: <ul><li>the interpreter in C, using GCC with computed goto (the measured control);</li><li>function forms;</li><li>leaf graphs for the inline templates;</li><li>out-of-line stubs on G3's fixed register convention;</li><li>the tracer's SSA expansion.</li></ul> A trial of Clang with `musttail`/`preserve_none` comes after the pilot, at a matched revision. GCC 15.2 has `musttail` but not `preserve_none`. |
| Values across words (G6) | Write-through forwarding in the baseline, added later. A machine register may hold a copy of a value already stored to its owning home. Independent entry, escape, redirect and transfer empty the cache. |

## Units

The integration branch is `pyjit/rir90`, cut from regions89's final head.
Records live in `E59/rir90/`. Unit builds share CPUs 0-3 and 8-11, so at
most four or five units run at once.

| Step | Units | Model | Exit |
|---|---|---|---|
| F0 | The language frontend: grammar, parser, type/effect/schema catalogs, graph data model, structural checks, diagnostics. `Add` and `GetAttr` parse and pass. | Opus | The data model is frozen for F1-F3. |
| F1 | The checker: ownership tokens, publish, checkpoints, guard-before-action, facts and effects against rows, edge totality. | Opus | Negative fixtures are refused. |
| F2 | The products: interpreter C, function forms, leaf graphs for templates, stubs, metadata. Generated code uses the value API only (closes backlog 792's generated class). | Opus | `Add.Int`'s generated C matches or beats today's: 15 statements against 20, 1 type test against 2. |
| F3 | The translator: a stack of value identities in `regcfg.c`, block parameters, home allocation, parallel moves after allocation, exceptional-edge recipes. | Opus | Comprehension and unpack examples execute 0 `Move`s. |
| P1-P5 | Pilot families (P1 bindings and moves, P2 `Add`, P3 `GetAttr`/`GetMethod` with poly, P4 `Call`/`Return` on G3a's switch, P5 truth and branches), each with its old definitions deleted. | Sonnet, with Opus integrators | Every product runs through its real ABI. |
| Freeze | One adversarial review of the verifier and one end-to-end review of products and ABI. The pilot's arms are expanded to SSA and run in a test evaluator against the C path. | Opus | The language is frozen. |
| W1 | Batches 1A-1I: numeric, value tests, items, attribute writes, displays, namespaces, text, patterns, functions, intrinsics. `Wide` runs as its own Opus unit. | Sonnet | Each complete family lands with its old definition deleted. |
| W2 | Batches 2A-2F: keyword and expanded calls, iteration, async/await/send/yield, exceptions, throw completion and async-for, resume and polls. | Sonnet; Opus reviews transfers | As W1. |
| D | Delete the stack-semantic generator: stack binding, exchange and `replaces`, windows, body pins, the release regex, stack-only handlers. Keep the public decoder and opcode schema. | Opus and Sonnet | No old provider remains. |

Astra estimates 60-130 agent-hours, or 19-35 working hours of elapsed time
at its staffing. At four or five concurrent units, expect two to four
working days. The critical path is F0 → F1-F3 → the pilot → the freeze →
the waves → D.

## Gates and measurement

- **Every unit:** its targeted tests, the smoke set, and its family's
  migration check against the old declarations. The freeze adds negative
  verifier fixtures, forced arms and exits, aliasing, finalizers,
  `f_lineno`, settrace and monitoring.
- **At wave joins and at D:** full suites, release and assert, JIT off and
  always.
- **Timing is recorded, not gated:** at the pilot, at each wave join and at
  D, on CPU 15 with the established protocol. Interpreter time is recorded
  against golden87 and JIT time against the matched interpreter.

## Existing work

- Regions89 closes after G3a's phase D merges, with final full suites and a
  timing record. G3a's execution pins, switch routine and `RESUME_CHECK`
  are pilot infrastructure for P4.
- The stack-uop units on hold now have new homes:
  - G1c becomes measured work on `Add.Int` after the pilot;
  - G2 is P3's `PolyShape` and `PolyMethod` arms;
  - G4 follows P4;
  - G6 follows the independent-entry tests.
- G0d1's leftover declared tables and backlog 792's generated checks are
  deleted with the old generator.

## Recommendations to the user

1. A separate typed definition language rather than restricted C (addendum
   §1). It costs about 250-400 lines of parser and 600-1,000 lines of
   checker, and the checker works on the same graph the products consume.
2. No C arithmetic in definitions; every bit operation is a declared
   primitive row.
3. The freeze runs the pilot's SSA in a test evaluator, so the language is
   shown to serve the tracer before 50 families are written in it.
