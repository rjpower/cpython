# Interpreter performance: combined plan

Status: selected56 is landed after architecture review, implementation, peers,
merged qualification and direct measurement. This is the active successor
to model55. Read the [design](../../design/proposals/interpreter-performance56.md)
and [critical audit](review.md), then the experiments below. The [goal](../../GOAL.md),
[architecture](../../design/runtime-architecture.md), [status](../../STATUS.md)
and [handoff](../../HANDOFF.md) own direction, selected contracts and exact state.

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
matched upstream profiles for final selection. Keep golden54 if no qualified
composition improves the result. Startup RSS and zero-box counts are not gates.

## Documentation cleanup

The goal, active plan, architecture and status now identify this stage.
The checkpoint report owns current comparison evidence; model55's report and
reviewed source remain historical inputs. Four duplicate progress/staging pages
are retired with their exact committed contents retained in git and the stage56
evidence directory. Ten superseded Loom review/inventory/progress mirrors have
been exported with revision metadata and empty thread inventories before
retirement. All ten are now retired; 12 current mirrors retain the goal → plan →
architecture/status chain. Canonical design/review evidence remains in git.


## Followup priorities

Current qualification and selection are complete. The next proposals use the
current883 profiles; no new implementation is part of checkpoint closure.
The [independent cold review](review-integration.md) ranks five followups:

| Priority | Hypothesis and first evidence gate |
| --- | --- |
| 1. Resume and delegated yield | Count common resume inputs, recipients, yields and slow exits. Remove one repeated operation across the existing checked transfer protocol; preserve observer and exception boundaries. SEND_GEN and delegated yield dominate generator samples, while terminal completion is only 1.90%. |
| 2. Exact Python calls | Find repeated admission, binding, refresh or home work in compiled exact-call hits. Preserve overlap, independent function/code pins and callback-selected control; source-level repetition is insufficient. |
| 3. Coroutine creation and retirement | Count copied homes, live owners, clear extents and callback-capable releases. Test one bounded construction/retirement hypothesis, rather than attributing the gap to zeroing or repeating a rejected broad constructor rewrite. |
| 4. Guarded attribute-read hits | Measure repeated tag/type/shape and acquire/retire work in mono and PIC hits. Keep checked nomination, invalidation and pre-effect misses; logging's zero selector attempts rule out extra selector-loop execution there. |
| 5. Next canonical producer | Count surviving publication/bridge work and post-pass temporary owners, then migrate one hot producer into the existing shared facts. The closed zero-candidate COPY morphology and zero hot-set CALL_EX sites do not justify more work on those slices. |

Centralize compiler fact invalidation and meets when extending those contracts:
canonicality, identity, ownership, nonnullness and readable homes remain distinct.
Keep thin completion entries on one shared body. These are proposals requiring
actual native removal, balanced elapsed results and broad regression checks;
they imply neither a universal runtime call object nor a new frame ABI.


## Parallel regression lane and delegation

Root owns integration, quiet timing and selection. Assign Sol workers disjoint
proposals for (1) resume/delegated yield, (2) exact-call admission/binding, and
(3) coroutine construction/retirement, with independent cross-review. Start
only the lanes supported by dynamic counts; do not implement all ideas at once.
A Terra/Luna documentation worker updates the affected as-built contract after
integration. Astra reviews grouped architectural changes, not individual patches.

Keep raytrace, spectral_norm and nqueens as regression sentinels alongside the
focus six. Existing PMU distinguishes extra instructions from higher cycle cost;
profiles do not yet isolate the culprit. A proposed fix must name and remove
actual common-path work and preserve guard/effect/ownership contracts. Do not
weaken the stronger element guard or specialize benchmark identities.

For each increment: propose → peer → implement → focused tests → actual-diff
review → root integrate/hot comparison. Use coherent full suites at representation
or final composition gates. Require stable exact-head/source/native identities,
balanced pair ranges and direct broad comparisons before selection. If two
bounded revisions remove no native work or show no measurable benefit, close
that experiment. Do not reopen the rejected inline getter without a distinct
hypothesis. Cross-CFG borrowing needs real eligible surviving opportunities.

Only the existing GDB PrettyPrintTests class is quarantined. Six debugger-jump
errors remain explicit. Do not call the complete suite green or convert cold
profile samples into an additive savings budget.

## New comprehension evidence

The [focused diagnosis](comprehensions-analysis.md) adds a concrete ordinary-path
lane: specialized native methods currently re-enter generic value dispatch;
list iteration loses canonical result facts; list append always calls out.
The workload executes fewer inner-loop opcode words than upstream but 46.44%
more native instructions overall. Prioritize checked method entry and canonical
FOR_ITER_LIST transport alongside the resume/call lanes, then measure append
fast-path placement. Avoid assuming any one item closes the 33.7% gap or restoring
the rejected getter-inline patch without a new broad result.
