# Interpreter performance: combined plan

Status: selected57 adds dictionary provider cleanup and lifetime repair to
selected56 after peer review, focused qualification and direct measurement.
The [current report](../comprehensions57/report.md) records the small broad gain,
accepted losses and rejected larger composition. This active stage succeeds
model55. Read the [design](../../design/proposals/interpreter-performance56.md)
and [critical audit](review.md), then the experiments below. The [goal](../../GOAL.md),
[architecture](../../design/runtime-architecture.md), [status](../../STATUS.md)
and [handoff](../../HANDOFF.md) own direction, selected contracts and exact state.

The active workstream is [IR semantics59](../register-semantics59/README.md).
The closed [round69 design](../../design/proposals/interpreter-parity69.md) and
[review](../register-semantics59/review69.md) ordered one guarded-callee-summary
unit from selected57. Actual hits remove the child frame/body/retirement, but
complete projection/select paths grow in instructions and stack. Its stable
32-row early diagnostic loses DeltaBlue 3.5%, logging 17.6%, generators 1.8% and
raytrace 3.4%; GM 1.012667. Positive committed-summary redirect coverage remains
incomplete. Stop the executable without further qualification or repair.

The [round70 design](../../design/proposals/interpreter-parity70.md) and
[review](../register-semantics59/review70.md) rank five options and order one
proposal: ordinary CALL returns to its vacated scratch B0; existing caller
instructions assign/discard/move it. Change compiler CFG before encoding,
validate the new format and update producers/magic without a legacy translator.
All consumer instructions and dispatch are charged in native comparisons.

Independent adoption stops round70 before implementation. A typed continuation
can distinguish implicit children, but the proposal adds publication/consumption
work without a credible complete-consumer reduction. Existing Python discard
already bypasses destination lookup and result-home traffic. Neither the typed
alternative nor a native-only scope advances. No runtime change or timing was
performed; the trial report owns the final source evidence.

The adopted [round71 design](../../design/proposals/interpreter-parity71.md)
orders two serial, independently selectable generator units. A transports audited
canonical-bit facts through existing ownership and publication. B separates
eligible callback-free hot bodies from body-only terminal ordinary slow entries.
Both retain encoded words, homes and the five-argument handler ABI. Exact source
contracts are independently cross-peered before implementation. A need not win
for B to proceed, and any native/frame saving earned by A is excluded from B's
credit. No combined parity budget is assumed.

Both exact source contracts have independent GO. A is implemented and its frozen
source has independent post-review GO. Root has integrated it from selected57
and generated the candidate. Product review, both 117/79 builds and 24 focused
methods per role pass. Static native removal is real, but the early protected
hot8 screen rejects A: GM 1.011186, logging 1.121887 and DeltaBlue 1.039961.
Root stops its remaining native qualification and all further timing. B then
implements standalone ADD/SUB hot/cold lowering from selected57, without A.
Its source/products, both 117/79 builds and 24 focused methods per role pass;
actual hot functions have no returning C calls. The early protected screen
still rejects B: GM 1.017289, logging 1.263918 despite six improved means.
Both units are closed and unselected. The user has authorized the
[execution-state design72](../../design/proposals/execution-state72.md): separate
logical values from physical owners, carry one result through dispatch, and
generate effect boundaries and cold ownership maps. Ordinary execution chooses
layout; exception teardown consumes it. Existing argument overlap remains.
Sol 6.1 workers cross-review exact compiler/layout and execution/activation
interfaces before implementation. Root integrates a complete producer/consumer
slice, including errors and observation, before expanding the port. Native
spills and all result consumers count toward the cost; logging remains protected.
The prospectively reviewed
one-pair confirmation option is implemented and passes its host tests; default
balanced screen behavior remains unchanged.

Round68's retained delegation qualified but lost its protected screen.
Generators improved 29.2% and spectral_norm 10.7%; DeltaBlue 10.1%, coroutines
7.2% and logging 23.9% losses reject selection. Preserve the mechanism and
evidence without further timing or repair. Selected57 remains unchanged.

Both round67 units qualified their mechanisms but lost their screens. A loses
logging 21.1%, DeltaBlue 2.2% and spectral_norm 5.3%. B loses all eight aggregate
means: hot8 1.114220, DeltaBlue 1.141350 and logging 1.430373. Both are rejected.
Root combines only independently qualified broad winners; selected57 is unchanged.

Round66's earlier qualified frozen
continuation has hot8 ratio .936871 but fails logging/generator protections.
The auxiliary union stopped unimplemented. The single matched logging diagnostic
has instruction ratio .992588 and cycle ratio 1.085426; it establishes no cache/frontend
cause. Earlier losing packets remain evidence, not prerequisites. Selected57
is unchanged; the [trial report](../register-semantics59/report.md) owns all
measurements and exact identities. No new upstream result or parity budget is
inferred from these experiments.

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
| Root | Integrate reviewed72 units and validate complete execution paths | Preserve selected57/evidence; own generation, native qualification and selection |
| Sol 6.1 compiler/layout worker | Logical values, allocation and cold ownership maps | W from selected57; exact source contract, focused host checks and post-review |
| Sol 6.1 execution worker | Accumulator, effects and activation/result transfer | P from selected57; shared ABI cross-review, focused host checks and post-review |

Both72 source-contract lanes are active. Both71 screens remain closed. Fresh
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
