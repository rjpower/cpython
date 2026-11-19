# Interpreter performance: combined plan

## Current rollout: handlers81, then calls80

Approved 2026-11-19. The user asked to land
[handlers81](../../design/proposals/handlers81.md) and then
[operand-encoded calls](../../design/proposals/operand-calls80.md) in its
shape, and to keep the result even if the first performance gain is modest.
Gates are correctness; measurements are recorded and attributed, not gated.
The H4 shape checks (handlers81 §6) are stop-and-diagnose checks on the
mechanism.

Branch `pyjit/handlers81` from checkpoint78 `99a04733128`; builds
`BASE/handlers81-{release,debug}`; evidence `E59/handlers81/`.

| Step | Content (handlers81 §6) | State |
| --- | --- | --- |
| H0 | Shape tool `Tools/pyjit/handler_shape.py`; checkpoint78 baseline | started |
| H1a | Canonical by construction; no publication at stores | started |
| H1b | No `phase_state` in handlers; GC scans all slots | started on sibling `pyjit/handlers81-h1b`; root merges after H1a |
| H2 | Ownership rules 3, 5–7; `IR_F_DIES_*`; discard slot; upstream frame clear; transition roots deleted | |
| H3 | Bridge records and canaries out of handlers | |
| H4 | One computed-goto evaluator; tail-call backend deleted | |
| H5 | Calls80 steps 1, 2a–2c, 3 written in the new shape | |
| Report | Kernel instructions, PMU, timed geomean against checkpoint78, golden73 and upstream | |

Each step is one or more Sonnet workers with a time budget, a small-first test
ladder and a progress log in `E59/handlers81/`. Root integrates: generation,
both builds, the step's ladder, kernel instruction counts (`E59/handlers80/instr.sh`)
and the shape tool. Full suites, release then debug, run after H2, H4 and H5.

Deferred until after H5: the lean normal-return root record, `__init__` shim
certification, `RETURN_NONE`, list-subclass `append`/`pop` specialization,
`_PyValue_RichCompare` spills, and hot-code layout (H6).

## Latest checkpoint: execution-boundary73 and RETURN isolation

The [grouped review](../register-semantics59/review73.md), exact source peers and
root approved the [complete boundary contract](../../design/proposals/execution-boundary73.md).
Sol workers implemented and cross-reviewed; root integrated, generated, built,
qualified and measured. The complete generator paths and guarded callback-free
receiver close remain in the candidate; none was replaced by a generic fallback
to satisfy the checkpoint.

Boundary10 `dee619e80ab` improves coroutine/generator throughput about 6% and
spectral_norm 24.69%, but loses logging 10.44% and raytrace 1.97%. Preserved
specialization and three matched PMU pairs show logging's extra cycles without
a comparable increase in executed instructions. Native review found enlarged
shared RETURN bodies, so root adopted a bounded source correction: one private
GEN_RETURN_ROUTE, with ordinary return state and all generator consumers retained.

That correction is complete at `44f066f5fd5`. Ordinary RETURN/EMPTY text falls
below selected57; arithmetic and exact-call whole-path counts match selected57.
The extra generator hop is charged. Protocol, owner, native and error controls
pass. The final protected screen improves coroutines 8.70%, generators 6.73%,
comprehensions 3.49% and raytrace 2.61%, but loses logging 13.73%. GM 0.985010
passes; logging protection fails. Independent review recorded NO-GO under the
original rules. The user subsequently accepted this specific regression and
selected golden73 on 2026-11-16. Branch `pyjit/round73-golden` and tag
`pyjit-golden-20261116-generator-boundaries` are pushed at this exact commit.
The [report](../register-semantics59/report.md) owns
both complete results and their evidence limits.

## Next implementation priorities

Golden73 is the new comparison baseline. Selected57 remains the recovery target
for logging and an immutable control. This landing accepts one measured tradeoff;
it does not turn failed historical screens into passes or authorize regressions
in future candidates.

1. **Ordinary call/return effect separation.** Keep the ordinary THREAD return
   and result consumer together; enter callback-capable retirement through a
   terminal private entry with explicit owned inputs. Preserve exact/general CALL
   specialization, destination semantics, polling and observer/error state. The
   objective is to avoid preserving hot VM state across cold returning C calls,
   not merely to shrink handlers. Golden73 already isolates generator routing;
   repeating that outline or reusing rejected generic completion is not new work.
   Before source adoption, compare final73 against selected57 on matched logging
   counters and complete hot call/consumer paths. The earlier boundary10 PMU is
   not a diagnosis of final73. Reject a proposal without a concrete boundary it
   removes; code size and instruction counts alone do not explain the regression.
2. **Specialized native result-to-consumer lowering.** Extend the successful
   owned-result/effect interface to one complete native CALL family and its real
   destination/discard consumer. Carry guarded provider facts into that consumer;
   avoid a home store/reload only where the whole path no longer requires it.
   Keep callback roots in the reached effect path and retain all existing
   specialization fallbacks. This needs a fresh source contract proving net work
   removal beyond the rejected Invoke/provider/compact attempts, including the
   added dispatches and nonparticipant cost. It is not an approved broad rollout.
3. **Lifetime-driven frame allocation, after execution boundaries.** Separate
   stable locals from short-lived values in compiler liveness, reuse temporary
   homes with disjoint lifetimes, and let cold exception/observer maps describe
   that allocation. Do not reserve universal scratch or add a per-operation bitmap
   tax solely to simplify teardown. Source adoption requires an actual reduction
   in frame extent or initialization for unchanged workloads. The earlier map-only
   layout attempt did not achieve this; it is not a prerequisite to resurrect.

These are ordered design directions, not implemented gains or an assumed parity
budget. Sol 6.1 source/peer workers prepare bounded independent units; Astra
reviews a grouped architectural change rather than the component loop. Root
owns integration and quiet benchmark windows. Use focused opcode/ownership
checks during implementation, merged protocol and hot8 checks at integration,
and broader qualification at the next coherent checkpoint. Protect the newly
selected generator/coroutine gains while recovering logging.

The next measurement checkpoint also needs all29, full qualification and a fresh
direct upstream comparison of golden73. None has run yet; the accepted landing
does not imply those results. Keep current source/build controls frozen and use
fresh evidence tags. Do not combine the best cases from boundary10 and final73.

## Selected baseline and closed work

Golden73 is selected. The [current report](../register-semantics59/report.md)
owns its measurements and accepted tradeoff. The [selected57 report](../comprehensions57/report.md)
owns the prior control's qualification and upstream comparison. The
[trial report](../register-semantics59/report.md) owns rounds59–73, their
mechanism successes, incomplete gates and measured rejections. Those packets
are evidence, not prerequisites or runtime components selected for73.

Boundary73 starts from selected57. It does not import72's continuation map,
carried-arithmetic mode or frame layout. Source reviewers must preserve existing
CALL/arithmetic specialization and close every admitted generator transfer,
including live HOME results and callback retirement, before root qualification.
The bounded frozen72 diagnosis is complete; it establishes specialization losses
in raytrace but does not establish the cause of logging's extra cycles.

Read the [goal](../../GOAL.md), this plan,
[runtime architecture](../../design/runtime-architecture.md), [status](../../STATUS.md)
and [handoff](../../HANDOFF.md) for direction, contracts and execution state.
The active workstream is [IR semantics59](../register-semantics59/README.md).

## Outcome

Improve the interpreter across DeltaBlue, generators, coroutines, Richards,
comprehensions and silent logging, then qualify the whole benchmark set.
Preserve Python behavior, explicit ownership, useful observation and the gains
already delivered by canonical PyValues. Select qualified improvements against
immutable golden54 and upstream controls. Do not attribute the generator or
DeltaBlue gap to frame clearing without evidence.

## One architecture, four workstreams

| Workstream | Required design and implementation |
| --- | --- |
| Values | Distinguish canonical homes, native private results and typed borrows. Carry established canonical/object facts through producers and native interfaces; eliminate redundant conversion and ownership work. |
| Borrowing | Extend the existing borrow-selection and copy-propagation passes with an explicit use/def, effect and lifetime model. Establish where a borrowed view is legal, when it becomes an owner, and what callbacks, local mutation, escape and suspension invalidate. |
| Calls and returns | Preserve exact-call overlap and inline return. Define a common argument, binding, recipient, continuation and retirement contract for ordinary calls, generators and coroutines, with exceptional/observer work paid at reached boundaries. |
| Specialization | Use one checked description of guards, effects, ownership, result facts, cache validity and fallback across lowering and interpreter specializations. Reuse existing adaptive caches; avoid parallel handwritten proof systems. |

PyValue is already canonical throughout runtime homes, containers and rebuilt
extensions. Native numeric results can still be private boxed temporaries until
publication. That boundary is the cleanup target; this is not another value
format rollout. A borrow pass also already exists: the frontend selects safe
borrow candidates and register copy propagation folds many local reads into
consumer operands. Strong snapshots at callback and ownership boundaries remain
necessary unless a stronger lifetime proof replaces them.

## Completed checkpoint

Selected56 is 883524c180e, qualified over byte-identical cb3 runtime sources.
The [report](report.md) owns results, qualification limits and selection judgment;
[benchmark tables](benchmark-tables.md) retain every case and observed range.
Astra reviewed the architecture; Sol peers reviewed proposals and actual changes;
root generated products, ran native integration/full diagnostics and measured.

| Workstream | Completed result |
| --- | --- |
| Values | Scalar replay/receipts plus canonical list/tuple transport and exact-coroutine owner transfer; physical NULL-control remains |
| Calls | Exact-tuple CALL_EX preparation, direct native completion entries and checked generator-recipient admission; ordinary overlap/inline return preserved |
| Specialization | Two-to-four homogeneous SHAPE PIC rows share checked guard/read/acquire contracts; actual DeltaBlue/Richards use confirmed |
| Borrowing | Reproducible21-file/426-CFG census found zero eligible surviving callback-free COPY webs; no redundant pass added |
| Qualification | Full 55,023 tests per role, six baseline debugger errors; final stale trace fixture fixed with focused checks; builds 117/79 and 5+27 bootstrap/compile pass |
| Selection | All 29/golden .984655, 17 wins; upstream 1.141077, 7 wins; all six focus cases improve in every confirmation pair |

Root explicitly accepts Raytrace 4.50%, spectral_norm/nqueens~3.5%, XML iterparse
2.7% and chaos2.5% regressions as an incremental tradeoff, overriding the audit's
suggested unexplained>2% tripwire. Cold selection review supports that judgment.
Golden54 stays immutable. These losses are unresolved debt, not noise.

Bitmap execution readers, IRSC2 retained transfers and full static M4 remain
independent gates. Do not complete all historical experiments as a prerequisite
for every ordinary-path improvement. Preserve their source/evidence.

## Measurement and selection

Use unchanged workloads, matched GCC configurations, fixed affinity/hash and
JIT disabled. Record source/native identity, validity checks and pair ranges.
Measure the actual count and cost of remaining conversions, count operations,
dispatches, call adapters and storage maintenance. Historical flat samples or
instruction ratios are clues, not additive removable cycle budgets.

Each integration reports what work disappeared and its hot-set result. Use
larger confirmation only when the initial evidence warrants it; run all 29 and
matched upstream profiles for final selection. Retain the current golden if no
qualified composition earns selection. Startup RSS and zero-box counts are not gates.

## Documentation cleanup

The goal, active plan, architecture and status now identify this stage.
The checkpoint report owns current comparison evidence; model55's report and
reviewed source remain historical inputs. Four duplicate progress/staging pages
are retired with their exact committed contents retained in git and the stage56
evidence directory. Ten superseded Loom review/inventory/progress mirrors have
been exported with revision metadata and empty thread inventories before
retirement. All ten are now retired; 12 current mirrors retain the goal → plan →
architecture/status chain. Canonical design/review evidence remains in git.


## Execution and delegation

The [trial report](../register-semantics59/report.md) distinguishes actual
mechanism evidence, incomplete qualification and measured rejection. Rounds59–69
remain unselected; round70 is stopped at source adoption. Preserved evidence is
not a dependency list.

| Owner | Current task | Integration boundary |
| --- | --- | --- |
| Root | Boundary73 and return-isolation checkpoints complete | Preserve selected57 and candidate evidence; own changed-candidate validation |
| Sol 6.1 compiler worker | Boundary selection, public validation and focused tests complete | Peer the next concrete source contract before implementation |
| Sol 6.1 execution worker | Complete generator transfer and callback retirement complete | Return isolation complete; preserve all consumers in successors |
| Sol 6.1 cold reviewer | Native, ownership, specialization and screen reviews complete | Audit actual whole paths and protection for any successor |

Both72 ABI implementation lanes are frozen. The layout72 and both71 screens remain closed. Fresh
diagnostic profiles are retained under `E59/architecture-diagnosis72-fresh`;
they do not establish a timing result or a removable cycle budget.
A losing screen stops the unit. A qualified broad winner may be selected before
parity; repeat grouped review and implementation against the remaining gap.

For each increment: propose → peer → implement → focused tests → actual-diff
review → root integrate/hot comparison. Full suites belong at coherent protocol
or final composition gates. Retain stable exact source/native identities and
balanced pairs, and compare all29 directly before final selection. Preserve
selected57, golden56, golden54 and upstream controls. Do not repeat a losing
screen to search for a favorable result, tune handler padding, or weaken owner,
readability or semantic admission checks.

The hot set is comprehensions, DeltaBlue, generators and coroutines; logging,
raytrace, spectral_norm and nqueens are regression sentinels. If a coherent cycle
still misses upstream parity, commission another grouped Astra review of the
actual remaining IR/interpreter costs and continue implementing measured winners.
Only existing GDB PrettyPrintTests is quarantined; six debugger-jump errors
remain explicit. Startup RSS and zero-box counts are not throughput gates.

## New comprehension evidence

The [focused diagnosis](comprehensions-analysis.md) identified repeated method
redispatch, dictionary wrappers, lost list result facts and unconditional append
calls at golden56. Its workload executed fewer inner-loop opcode words than
upstream but 46.44% more native instructions. Those concrete costs motivated
the four-lane experiment, rather than establishing that any one fix would close
the gap. Selected57 still executes 43.64% more comprehension instructions.

The [comprehension workstream](../comprehensions57/plan.md) is complete. All four
changes were implemented and the composition qualified and measured, but logging and
coroutine regressions prevent selecting it. Selected57 retains only the shared
dictionary provider and callback-lifetime repair; its small broad mean benefit
comes with explicit coroutine/raytrace and generator debt. Golden56 stays frozen.
The [report](../comprehensions57/report.md) and
[tables](../comprehensions57/benchmark-tables.md) own current comparisons.
