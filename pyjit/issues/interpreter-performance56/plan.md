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
All five first-cycle executables lose their screens; the
[trial report](../register-semantics59/report.md) owns those decisions.
The reviewed [next design](../../design/proposals/interpreter-parity65.md) and
[grouped ledger](../register-semantics59/review65.md) rank five architectural
options. Both fixed storage-role trials are qualified and rejected. The finite
ordinary-C compact domain passes both builds and 29 focused methods per role,
then stops at its native whole-path work gate: 47 internal instructions saved
against 93 added at entry/exit. Its 112-byte stack and 1,665-byte hot text pass
the size caps; no timing or target coverage follows. The independent complete
stable-frame deferred-await unit passes both builds, lifecycle and native gates.
Its unchanged hot8 screen cuts coroutine time 42.9% but loses logging 27.7% and
generators 5.2%; reject the standalone executable. The completed round65 review
orders a bounded return/accessor refactor, then a conditional private SEND-yield
continuation. A passes native qualification but loses its own hot8 decision:
coroutines .606811/selected57, logging 1.161661 and generators 1.055271. Reject
the standalone executable; B now implements the already reviewed isolation
hypothesis with complete reader/writer/control closure and its own gates.
Delegation spine, result continuation and anchored self have separate admission
and cost gates. All round59–62 executables
and both round63 role trials remain
rejected; selected57 is unchanged.
Earlier source and evidence remain inputs, not prerequisites.

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

The workstream's [next design](../../design/proposals/interpreter-parity65.md)
and [trial report](../register-semantics59/report.md) distinguish proposed,
source-reviewed, qualified and selected changes. All round59–62 executable
screens are closed and losing; no current runtime change is selected.

| Owner | Current task | Integration boundary |
| --- | --- | --- |
| Sol A, returns | Existing complete RETURN recipes partition THREAD versus one owned-result resumable helper; state-proved embedded accessors | Implementing in W after root/Astra proposal GO; no new opcode, mode, image mirror or classification pass |
| Sol B, continuation | Private SEND-yield target and complete semantic-PC/reader/writer contract; independent A source peer | Proposal in P; shared generator interfaces freeze after A. No B implementation before source GO and A native gate |
| Coordinator | Peer interfaces, integrate, generate, qualify and measure | A gets its own hot8 and may advance without B. B integration receives separate native/performance decisions |
| Astra | Completed [grouped65 review](../register-semantics59/review65.md) | Source `374f8e3eadb` plus clarification `ceb46447bb0` integrated; next broad review only after coherent measured outcomes |

The compact unit is closed; no roster, mirror, provider, layout or flag
expansion rescues it. Round65 retains the qualified await mechanism while
removing ordinary-path work at actual boundaries. GENERAL already has a
THREAD fast clear; EMPTY and complete return classification are priced
separately. Unit A ports all existing RETURN/YIELD source certificates with
the actual recipes. Unit B must port every actual return-PC consumer and writer,
keep semantic PCs real, preserve newer redirects and price private arm/cancel
and terminal work. It adds no ordinary frame field or universal resolver.

An A native failure stops A; a broad loss permits only the already reviewed B
isolation attempt. A passing screen can advance without B. The combined A+B
screen is a changed executable, not a repeat of a loser. The delegation spine,
native-result ownership region and anchored argument view remain deferred
under their separate complete-path gates; no implementation is concurrent.
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
