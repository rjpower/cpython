# First method slice: behavior and measurement contract

2026-11-01. Proposed contract only; no execution, sample count, source or native
authority. Validate M0→M1 as a complete method path through native branches,
Python calls, a C bridge, return and raise. This replaces the arithmetic
prototype's54-case assumption; it does not inherit that roster or any score.

## Four small program families

Use ordinary Python source with changing inputs and an independent interpreter
reference. Exact source/inputs and future exposure are frozen before execution.
No program-name matching belongs in the compiler.

| Program family | Work and observable oracle | Why it belongs |
|---|---|---|
| Varying loop/call recurrence | Loop over varying input values; both branch directions update an accumulator through a real Python callee. Include zero/one/many iterations, changed callee and a bounded exceptional input. Compare result and ordered call arguments, not just a final constant. | Native CFG/backedge, Python call/return and continuation within one complete method; no invariant arithmetic shortcut. |
| Mixed objects and C callback | For A's proposed `work(n, cb)`→`bridge(i, cb)`→`abs(cb(i))`, let cb return changing integers or objects with Python `__abs__`; callbacks update a ledger and may reenter Python. Compare output, alias identities, mutations and callback order. | Real C→Python reentry with published caller state: builtin_abs→PyNumber_Absolute→nb_absolute→slot_nb_absolute/__abs__. A mocked helper return is not this path. Other object operations can follow as their adapters become available. |
| Return/raise and lifetime | A call/store has a visible sequence number, followed by success, a caught exception or an uncaught exception with finally cleanup. Replace an owned local whose destructor observes the new frame local and reenters Python. Include a generator consumer whose suspension remains exact interpreter fallback. | Exactly-once effects, owner transfer, traceback/handler depth, finalization and fallback across suspension. This is primarily a semantic diagnostic, not a finalizer microbenchmark. |
| Short methods versus sustained work | The same varying computation as a short call chain and as a long-lived loop with real call boundaries; return a checked checksum and keep input creation outside the timed body where appropriate. | Exposes outer tier resolution, compilation and transition cost separately from loop amortization. Both costs are real; neither case alone sets architecture policy. |

## Differential observations and real execution proof

Use the same pristine program with fresh state for reference and compiled runs.
The reference is the interpreter in the same runtime, selected through a
test-only mechanism or separate explicitly bound control; trace/profile hooks
are not an implicit performance control. A forced-compile semantic fixture is
valid bring-up evidence, but does not prove natural hotness or tier selection.

Compare exact return values/types, alias identity relations, ordered side
effects, callback count/order, exception class/arguments, handled/finally paths,
and program-frame traceback code/line/logical-word locations. Compare identity
relationships, not raw addresses from separate processes. Fresh namespace/code
objects and bounded input variants avoid state or specialization leakage.

Separately require evidence produced by **actual emitted native entry** and
**the compiled continuation after a real Python/C call returns**. Bind it to
the CodeUnit/native range, program code and logical word. An emitted diagnostic
first-seen mark or equivalent retained execution witness is sufficient; code
registration, compile success, an active flag or a direct arithmetic fixture is
not. Source/emission review checks that those marks are reachable only from
the claimed native paths. A compiled-but-never-called negative stays unseen;
an interpreted fallback cannot set the compiled-continuation mark. Read-only
inspection must not compile, advance hotness or invoke the program. These are
diagnostic observations, not scored hot-path instrumentation or frequency data.

## Required boundary cases, within the four families

* Exercise pre-effect resume with a visible operation still pending: after exit,
  it occurs once. Exercise post-effect resume with the ledger already changed:
  no replay, lost result or duplicate callback. Also exercise the real raising
  helper path and handler state. A test-only forced Action/exit seam may select
  a real emitted statepoint; a Python mock of the Action structure proves
  neither native recovery nor ownership. Keep forced-exit evidence labelled.
* Cover owned/borrowed/inline/NULL homes; aliases; call destination overlapping
  an old local; callback invalidation; destructor reentry; frame inspection;
  and exception-only live owners. Publish the new local before releasing the
  old one, and do recursion/argument/result transfer exactly once. Validate
  lifetime after producer frames have returned; do not demand weakref death
  inside an opaque C activation or use arbitrary repeated collections as proof.
* Check recovery allocation failure before irreversible work. After an effect,
  preserve its result/outcome and use the defined safe recovery path; never
  replay the effect to rebuild state. Fault prefixes restore frame/current
  exception, owners, active CodeUnit pins and entry links. Test no premature
  unmap during a callback/invalidation and eventual retirement after the active
  activation exits. No general artifact-reference cycle may leak through an
  untraversed raw owner.
* A deterministic pending-work/observer request must be serviced at the proper
  native backedge/statepoint in a bounded loop. Preserve NO_INTERRUPT semantics.
  Test trace/profile and line/instruction monitoring enablement from reentry,
  not only observers enabled before entry. Compare the observed program events
  with interpreter behavior; bounded test timeouts detect hangs, not performance.
* Generator yield/send/throw/close may leave native execution through the
  declared exact fallback. Preserve homes, exception state and continuation;
  never retain a native stack address in a suspended generator. Claim only the
  mode actually covered by the source cut.

## Generated coverage and a compact inherited regression set

Classify every **valid pristine** handler, including operation-specific forms,
NOFB, wide/window forms and exception/control transitions: direct native,
generated semantic helper, or explicit interpreter tier-down. Reject private
native handlers in persisted input. The table must come from the actual source
metadata and separately check emitted adapter presence, logical PC/field roles,
normal/error/suspend outcome and ownership/effect classification. Counts or
fixtures copied from the new header are not a semantic oracle. A supported
complete method must not silently interpret one instruction at a time while
claiming native control flow.

Useful existing source-derived regression cases are:

* `test.test_ir.AssemblerTest.test_deopt_after_borrowing_a_local`
* `test.test_ir.AssemblerTest.test_exception_from_a_pushed_frame`
* `test.test_ir.AssemblerTest.test_frames_when_instrumented`
* `test.test_ir.StoreFoldTest.test_release_runs_with_the_frame_consistent`
* `test.test_ir.RegCfgTest.test_call_result_to_a_local`
* `test.test_ir.RegCfgTest.test_pushed_frame_result_to_a_local`
* `test.test_ir.RegCfgTest.test_frame_holds_the_handler_depth`
* `test.test_generators.GeneratorDeallocTest.test_frame_locals_outlive_generator`

These cover existing interpreter invariants. Passing them interpreted is not
native coverage; the four new end-to-end programs establish that separately.
Select exact applicable tests after the source manifest; no frozen54-method
cohort, descendant census or unsupported global StackRef-debug gate is imposed.
Boundary cases apply to the paths changed by this source cut. A generator or
unimplemented construct may decline native entry or tier down for its remainder;
record that exact scope rather than requiring another unsupported feature now.

## Measurement and gate order

First: host generator/coverage checks, source review, then a finite native
behavior/ownership/failure batch and actual emitted entry/continuation review.
The first slice need not implement tracing, virtual frames or arbitrary OSR to
earn this baseline milestone; its contract must remain compatible with them.

Then diagnose the four program families with fixed inputs/exposure and matched
runtime/build policy. Report separately: cold process startup plus compilation;
first method invocation; naturally warmed work; whole-process elapsed and RSS;
compiled code/metadata size; compile count/time and helper/exit coverage from
separate diagnostic runs. Warmed results must not hide the first-use cost, and
whole-process residual is not an isolated compilation measurement. No speedup,
sample budget or acceptance threshold is invented here; freeze the question and
future finite measurement protocol before seeing values.

Use measured costs to choose native fast paths, helper boundaries, register
policy and tiering. Retain regressions and failed hypotheses. Run the broad29
distribution at an integrated coverage/performance milestone and before
adoption, not after each helper or arithmetic family as a vote on JIT existence.
Expensive full stdlib/extension/ASan/publication qualification follows useful
integrated behavior; critical ownership, raw code lifetime and exit correctness
are never deferred to that late phase.
