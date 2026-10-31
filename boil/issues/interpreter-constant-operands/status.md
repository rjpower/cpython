# Reuse exact small constant operands

- State: rejected
- Owner: qualification_review_resumed; coordinator selects implementation
- Updated: 2026-10-31
- Budget: offline research complete; 90-minute host-only prototype
- Source started: 2026-10-31 20:20:13 UTC; decision due 21:50:13 UTC
- Checkpoints: 20:50:13 domain/format; 21:20:13 fallback/observer/stack proof
- Depends on: [shared-interface review](../interpreter-shared-costs/reviews/holistic.md),
  [accepted union](../nursery-shape-landing/status.md)
- Source base: integrated `d98291018c9665ae3db6952da0b0a40a4e04992b`
- Native evidence: sealed union `e9e10d5b503`, GCC O3 image; saved profiles `0b15b4bbc9b`
- Branch/worktree: `boil/interpreter-constant-operands`; `build/worktrees/constant-operands`
- Decision: reject the thirteen-handler increment on measured elapsed regression; text growth remains diagnostic
- Runtime candidate: `7f0b1819f84ee11b354d60394a4975e1fcb0a188`, frozen about 20:56 UTC
- Current test-only candidate: `a84965eaa90267ac1dbf1b4c97ab032aaf26cedd`
- Runtime tree: `a081163bde190c453627755eda5ed3ed4951ccb6`

## Hypothesis

An operand form that materializes an exact small integer in the arithmetic word
can remove the separate constant-load dispatch and RHS admission without a
second type lattice or a fallback that misreads the operand fields.

## Experiment

Inspect actual compiler/CFG/assembler/DSL/domain/observer contracts and bounded
native instruction ranges. Read saved profiles with their original identities.
Select the smallest coherent arithmetic family, state entry/alias/error/root
proofs, and propose a finite implementation/emission decision. No native run,
benchmark, rebuild or runtime-source edit belongs to this investigation.

## Decision rule

Reject a cut whose work is already absent from native code, whose proof relies
on unchecked mutable frame facts, or whose generated fallback/effect machinery
duplicates the existing model. Recommend a bounded prototype only when concrete
native work remains and one operand-source contract can cover every entry.
No elapsed improvement is inferred from source or sampled evaluator shares.

## Now / next

The [completed comparison](performance-results.md) rejects this increment.
Coroutines was 4.47% and 11.64% slower in the two fixed rounds; both controls
also regressed. Independent [outcome review](reviews/quiet-outcome-20261031-v1.md)
verified all 80 raw elapsed durations, eight actual origin witnesses, unchanged
products, accepted quiet checks and terminal cleanup. No extra samples,
qualification or landing follow this result. Preserve the source and evidence;
any further implementation needs a separate concrete hypothesis. The history
below retains the earlier preparation decisions and their limits.

O3 retains RHS admission at `0x4592a0`/`0x4592a3`, and the small-constant producer
stores then dispatches at `0x45de38`/`0x45de4a`. The simple source proposal is
not already optimized away. However, same-line adjacency alone does not prove
a frame-slot fact across debugger entry: `f_lineno` scans compatible stacks at
every word on the target line. A generated same-word operand source avoids
trusting the prior producer's frame slot. The [design](design.md) narrows the
consumer to existing `LOAD_SMALL_INT` producers and specifies coherent generic/
in-place, specialized, monitoring and field-layout behavior. The [results](results.md)
retain bounded native instructions and exact saved-profile helper attribution.
The [independent design review](reviews/design-20261031.md) cleared the
90-minute host-only representation prototype. It runs in the isolated worktree
from documentation checkpoint `32f14069fd2`, whose runtime source is d98291018c9.
The source froze within the prototype budget: thirteen generated operand-form
handlers, a linear use-counting compiler pass, shared metadata/readers and eleven
prepared target tests. The host suite ran 83 tests with one pre-existing expected
failure and no unexpected results; existing generated bodies/formats are unchanged. The capsule at the
isolated worktree's `build/constant-operands/candidate.json` binds the source and
host receipts. [Independent exact-source review](reviews/source-20261031-v1.md)
accepts this source for bounded feedback. The coordinator's
[helper review](reviews/feedback-helpers-20261031-v1.md) accepts the exact eleven-row
launch after independent host/control/tool replay. Root activated the
[native feedback](feedback-plan.md) at 21:20:45 UTC after the GC invocation
stopped and verified cleanup. The debug build passed; eleven new cases returned
ten passes and one source-control assertion failure, then execution stopped.
An exact-CodeType observation confirmed earlier block inlining, and the
[reviewed test-only correction](reviews/test-oracle-correction-20261031-v1.md)
preserves the raw-CFG controls. Its remaining-only batch started at 21:40 UTC
on the bridged debug image; all eleven corrected cases passed without skips,
all 74 IR tests passed, and generation ran 83 tests with one existing expected
failure and no unexpected result. Disassembly then stopped at the first
constant-load/immediate golden mismatch. A bounded unchanged-source inventory
ran at 21:57 UTC: 147 cases returned six assertions, then its host validator
stopped on an absent phase name before the fixture child ran. Terminal cleanup
passed. Raw failures remain intact; read-only replay and individual causal
classification precede any golden correction.
The separate reviewed fixture capture passed at 22:03 UTC. Three strings
remove a literal load; two are identical. Live jump targets agree with labels,
and the single removed instruction explains the expected table's offset
changes. Authorized test-only goldens froze at 22:05 UTC and passed independent
[exact source/output review](reviews/golden-correction-source-outcome-20261031-v1.md).
Root reviewed the remaining helpers and activated seven rows at 22:08 UTC.
Disassembly passed 147 cases, code objects 47 with one expected skip, and tracing
442 with one expected skip. Monitoring stopped after 54 cases on one loop-exit
offset assertion: actual8 versus expected9. Cleanup passed; release and emission
did not run. A bounded exact-CodeType observation is being prepared before any
correction. The reviewed exact-code observation confirmed branch3-to8 and
inline ADD1. Independent review accepted the sole expected-offset9-to8 change.
Root activated four remaining rows at 22:15 UTC under the original deadline.
Full monitoring passed 108 cases without skips; the release build then reached
its downward-clamped 82-second timeout. Root confirmed terminal exit1 and idle
cleanup. Partial O3 products are unqualified; release cases/emission did not run.
All planned debug scopes have now passed with explicit source bridges.

The original first-feedback deadline remains 22:20:45 UTC. The coordinator
explicitly assigned a separate twenty-minute completion scope for the three
unfinished release/emission rows, with source unchanged. Host preparation will
archive and hash all interrupted products, then propose a fresh release build
under exact reviewed helpers. Independent preservation/preparation review
accepted all 426 archive entries. Root activated the three-row completion at
22:25:24 UTC with its separate 22:45:24 UTC deadline. Fresh O3 completed in
109.473 seconds and all eleven release cases passed without skips. Emission
stopped before its target child because the ELF reader assumes ET_EXEC;
both matched images are PIE. The reviewed correction then collected emission
successfully at 22:40 UTC, with terminal exit zero and cleanup before the
unchanged completion deadline. Independent
[native review](reviews/emission-work-removal-outcome-20261031-v1.md) accepts
the removed producer dispatch, frame write and RHS loads while identifying the
remaining guards and fallbacks. The
[raw counter capture change](reviews/raw-capture-source-20261031-v1.md) passed
independent review and nine mock tests. No timing has run.
The [execution record](feedback-results.md) owns outcomes and separate clocks.
The [original held performance proposal](performance-plan.md) preselects the registered
coroutine workload, with spectral-norm support and separate controls; its
independent [design review](reviews/performance-plan-20261031.md) accepts the
proposal for later preparation, with raw counter capture and native feedback
still required. The [revised decision](performance-revision-20261031.md)
withdraws hard code-size/instruction-count proxies after user steering before
any timing. Matched evaluator text grew 18.4248%; this remains a reported cost,
not an automatic rejection. Actual elapsed performance and control regressions
decide acceptance. Broader qualification and performance acceptance remain
pending.
Independent [footprint review](reviews/footprint-outcome-20261031-v1.md) retains
the matched size measurement and explicitly supersedes the historical rejection.
The [performance revision review](reviews/performance-revision-20261031-v1.md)
accepts the held elapsed-based decision rule; it grants no timing activation.

A single five-second readiness probe of `perf stat instructions:u -- true`
returned exit 1 at 22:54 UTC: the restarted host has perf_event_paranoid=4 and
denies counter access. Raw output is retained in
`build/constant-operands-readiness-v1`; no workload ran and no system setting
changed. Prepare a separately reviewed, premeasurement amendment omitting
unavailable diagnostic counters while preserving the elapsed samples, workloads,
order, quiet window and decision rule. Quiet activation remains pending.
