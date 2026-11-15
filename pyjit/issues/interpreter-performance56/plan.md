# Interpreter performance: combined plan

Status: selected57 adds dictionary provider cleanup and lifetime repair to
selected56 after peer review, focused qualification and direct measurement.
The [current report](../comprehensions57/report.md) records the small broad gain,
accepted losses and rejected larger composition. This active stage succeeds
model55. Read the [design](../../design/proposals/interpreter-performance56.md)
and [critical audit](review.md), then the experiments below. The [goal](../../GOAL.md),
[architecture](../../design/runtime-architecture.md), [status](../../STATUS.md)
and [handoff](../../HANDOFF.md) own direction, selected contracts and exact state.

The active workstream is [IR semantics59](../register-semantics59/README.md):
revisit operation and ownership/storage contracts, implement ordered trials,
integrate winners and repeat the broad review against the remaining gap. The
preceding workstream is [interpreter parity58](../interpreter-parity58/README.md).
Its [reviewed design](../../design/proposals/interpreter-parity58.md) and
[research ledger](../interpreter-parity58/research.md) replace the isolated
followup ordering below with a coherent ownership/effect/lowering strategy.
Round58 is design-complete; implementation and native qualification have not
started. Selected57 remains the baseline. Preserve the earlier designs and
rejected experiments as evidence, not as selected implementation.

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

The [IR-semantics workstream](../register-semantics59/README.md) owns the single
ordered trial list. Its [design](../../design/proposals/register-semantics59.md),
[peer review](../register-semantics59/peer.md) and
[trial report](../register-semantics59/report.md) distinguish proposals, source
packets, qualified executables and measured selections. The first loop-binding
executable, standalone builders and builder+Invoke composition all lose their
performance screens. Invoke is being isolated from builders. The continuation
action source packet is complete with actual peer GO; root owns generation and
native gates. Resident-value/backend alternatives require all-word coverage
and a nonoverlapping native budget.

Root owns integration, generation, serialized native jobs and quiet timing.
Sol workers implement and cross-review bounded components in separate worktrees.
Astra reviews the combined residual architecture after coherent measured cycles,
not individual patches. Close conditional edge/borrow lanes without a new
representation if the actual surviving census provides no consumer. Resident
value/region proposals must state their reachable coverage and remaining native
budget; a small pilot alone does not establish a path to parity.

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
