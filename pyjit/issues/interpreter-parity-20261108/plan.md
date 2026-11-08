# One-workload interpreter parity

2026-11-08. Coordinator execution plan. Start from qualified runtime
`0953757f4e7` and canonical documentation `4f72b563f34`. Preserve that batch's
measurements and unrelated workspace changes. The next selected workload is
`logging_silent`, with the JIT disabled and GCC unchanged.

## Outcome and admission

Drive the unchanged real logging workload to upstream parity through reusable
interpreter improvements. Its fresh pinned elapsed candidate/upstream factor
is 1.657289. The preceding qualified campaign reported instructions and cycles
per workload loop of 1.720317 and 1.629600 under its own protocol. These are
different measurements. The benchmark
executes ten disabled `logger.debug(message)` calls per loop and checks an
empty output stream. Do not change logging or the benchmark to manufacture a
win. Richards, attribute cases and non-call kernels remain regression controls.

The parity claim requires two fresh campaigns with six balanced upstream/
candidate rounds each, identical fixed loop counts, seven timings per process,
one CPU-affinity policy checked in native children, unchanged source/build/
module identities and explicit JIT-off checks. The geometric mean of paired
elapsed ratios must be at most 1.00, with every round at most 1.02. Use the
arithmetic mean of each process's seven values, form one candidate/upstream
ratio per round, then take their geometric mean. Each campaign has three
upstream-first and three candidate-first rounds, with the same fixed loop
count in both campaigns. Retain all
values and host-load observations. A noisy or incomplete capture does not
establish parity; this repeatability rule is not a statistical equivalence
proof. Reviewed meaningful progress remains useful but is labeled
progress, not parity.

At the next coherent integration checkpoint, qualify the combined source,
report the chosen workload and control results, and update the user. Continue
the parity objective if that checkpoint leaves a gap; do not recover obsolete
zero-box or startup-RSS gates from historical artifacts.

## Workstreams

Execution authorized after comparative review. The selected bounded batch is
typed code homes and successful-refresh cleanup; dictionary leaf-result
completion (M1); ordinary-NOP continuation (M2b); and fact propagation through
guarded Python call entry (P3), subject to each exact implementation review.
M1 and M2b address the inner path that the call-entry change does not cover.
They preserve the instruction format and observation words. The selected
batch is integrated through `10627c339fa`: P1/P2 `0539e0c1ff9`, M2b
`73c20472a9f`, M1 `d93b83800a5`, and P3 `10627c339fa`. Each passed native
release/debug builds, focused checks and independent post-review. Combined correctness qualification and final performance admission are now
complete for this bounded checkpoint. Logging parity remains open. M2b's stats
capture confirms 151 handler entries per loop, down from R5's 161, with the
same 20 frames and nine POP entries. Boundary semantics remain in the IR.
Do not describe the entire eight-item review as implemented.
See the [recommendation ledger](recommendations.md) and the independent
[architectural alternatives review](alternatives-review.md).

The table assigns owner roles, not live agents. Initial comparison and baseline
review are complete; Terra owns bounded implementation, Sol owns exact
contract and code review, and the coordinator serializes native checks.

| Stream | Owner | Deliverable |
| --- | --- | --- |
| V8 comparison | Sol, high effort | Source-backed comparison with 5–10 ranked recommendations, separating baseline bytecode, optimizing IR, Python semantics and implementation choices |
| Comparative diagnostics | Terra, medium effort | Reconciled call routes, existing dispatch counts and bounded phase/frame/observer diagnostics; no architectural choice implied |
| Documentation | Luna, medium effort | Goal, stage, status and handoff reconciliation; accepted contract amendments after review |
| Integration | Coordinator | Frozen baseline, native probes/counters, review decisions, commits, merged checks and performance admission |

Workers have disjoint file ownership and do not run shared builds, full suites
or native benchmarks. First useful packets have 30–60 minute budgets and
progress files. The coordinator owns all native work and serializes quiet
measurement with builds/tests. Use an isolated diagnostic diff for counters;
release timing must not contain them.

## Execution ladder

1. Capture warmed IR for the logging loop, `Logger.debug` and `isEnabledFor`,
   plus a pinned fresh upstream/candidate baseline. Verify actual specialized
   handlers and distinguish sampled hot symbols from call-route frequency.
2. Review actual frame/window constraints and the V8 source comparison. Record
   every recommendation's disposition in a ledger: implemented, accepted
   design dependency, or explicitly deferred with reason. V8's tracing GC and
   JavaScript semantics do not license dropping Python owning homes, callbacks
   or observation rules.
3. Start with existing stats and scaling against an empty interval. Then add
   only reviewed counters that distinguish call routes, leaf dictionary hits,
   retirement and observer boundaries. Reconcile mutually exclusive outcomes.
   Inspect fixed-GCC code for actual operand tests, state traffic and spills.
   Code size and samples alone do not establish removable work or route counts.
4. Compare competing structural mechanisms: proof propagation through calls,
   phase-aware PC/home publication, ownership-aware register equivalence,
   call-return observation boundaries, constant/frame layout and operand/
   dispatch interfaces. For each, establish covered work, irreducible Python
   costs, migration risk and a discriminating experiment. Select a coherent
   batch from this comparison before authorizing more runtime edits. Small
   known-type and duplicate-check cleanups are optional companions, not a
   substitute for that architectural decision.
5. Peer-review each design before implementation. Regenerate from producers;
   use focused generator/call/default/kwargs/observer/reentry tests first.
   Compile release and debug. Post-review source and executed checks before
   landing each coherent increment; obtain a fresh cold integrated review.
6. Run pinned target/control screens at each integration. Use counters and
   GCC code to check the expected work disappeared. Reject performance-only
   complexity that cannot justify its cost. Revisit remaining bottlenecks
   rather than optimizing a declining route by assumption.
7. At the coherent checkpoint run full release/debug qualification once,
   rebuild stats and check module/API gates, then freeze the final source for
   parity and broader regression measurements. Independently review evidence,
   update canonical docs and mirror goal/plan/report to Loom.

## Standing semantic constraints

The existing frame stack, common positional overlap and known-NULL tail reuse
are already implemented. A contiguous call window is an IR choice to assess,
not evidence that the current duplicate checks are required. Compiler moves,
owning arguments, parameter mapping and observation state need separate cost
and correctness accounts.

Callers retain owners through fallible preparation. Final guards follow any
allocation/callback. Transfer commits once, with no failure/reentry before
publication. A proof valid before an escape must not be reused afterward.
Every physical home stays a canonical PyValue or NULL; future-read liveness
does not authorize early destruction of observable locals. Preserve exception,
monitoring, custom-evaluator and generator behavior. Native method/tracing
recovery remains a separate planned stage.

Raw evidence goes under
`python-boil-the-ocean/build/pyjit-link-wave1/interpreter-parity-20261108/`.
Designs/reviews and the recommendation ledger live in this experiment;
canonical design pages own the final contracts. The previous report remains
[sealed history](../../bench/results/interpreter-architecture-20261107.md).

## Current evidence

The warmed IR confirms ten general Python call sites into `Logger.debug`,
with an exact inner call into `isEnabledFor`. The new six-round pinned baseline
has geometric mean candidate/upstream elapsed factor **1.657289**, with paired
ratios 1.648208/1.665205/1.636744/1.646092/1.658995/1.689003. This is a new
measurement protocol, not proof that source improved from the older unpinned
1.74303 result. That baseline uses R5. The eight-item V8 review is complete;
the selected bounded implementation batch is described above.

Independent result review reproduced the baseline. Existing stats identify
161 dispatches and 20 frames per workload loop, including nine retained POPs
and ten line-boundary NOPs. The mechanism proposals and exact implementation reviews are complete for
the selected batch. Route diagnostics show ten exact inner calls and ten
outer allocating-overlap calls per loop, without consuming-binder/native
fallback on this workload. The full suites, API lint, temporary M1-hit
counter diagnostic and final clean-source performance campaigns are complete.
The [checkpoint report](../../bench/results/interpreter-parity-20261108.md)
records the bounded keep decision and its spectral regression. See the [comparison checkpoint](comparison-checkpoint.md),
[diagnostics](diagnostic-results.md), [cold review](cold-integrated-review.md)
and increment post-reviews. Initial failed fixture attempts are retained;
corrected tests were checked against the old runtime before acceptance.

## Checkpoint outcome and next execution plan

The selected batch is complete. Functional source `0267d428808` passed both
54,394-test suites, all module/API gates and independent source/evidence
review. Current runtime `0765e9be47f` is a comment-only successor; all three
builds identify it. Its rebuilt release interpreter text matches the measured
image apart from two equivalent string-address relocations outside the
interpreter, and debug text matches exactly. The measured image remains
retained. The task worktree/branch and temporary R5 executable copy were
removed after byte verification; evidence and unrelated worktrees remain.

Logging candidate/upstream is 1.387177 and 1.378619 in two six-round campaigns,
so parity fails. The direct retained-ELF comparison is 0.840163 on logging and
0.980981 across 29 kernels; both ELFs share current modules. The broad upstream
screen is 1.241158. Accept spectral's 1.086037 direct elapsed regression for
this checkpoint; its retired work falls slightly while cycles increase, with
cause unresolved. All 72 counter runs pass and all eight profiles have zero
lost samples, with the descriptive event-header warning retained in the
report. No extra full zero-box census or startup-RSS work is required.

The next workstreams follow the reviewed ownership comparison, not a commitment
to one encoding or another call-wrapper cleanup:

| Priority | Packet and owner role | First useful result and stop/continue gate |
| --- | --- | --- |
| 1 | Borrowed-frame evidence; Terra medium, root captures | Reuse warmed mortality/IR evidence; identify a retained mortal argument and record its attributable owner transfers, separately from frame/function/kwargs activity. Stop if the chosen pilot folds away or saves no real owner work. |
| 2 | Lifetime and representation decision; Sol design + independent Sol review | Complete the actual source/generated consumer census and owner/GC graph. Compare a private frame tag with canonical words plus explicit view/anchor metadata. Resolve ordinary guard declines, exact tag-to-owner promotion, observer entry, copy/exposure, unwind, suspension and native fallback before choosing. No runtime implementation yet. |
| 3 | Bounded implementation after design admission; Terra medium + Luna mechanical batches | Start with a real retained local-to-child argument in ordinary GIL thread frames, then explicitly cover the real general-call/kwargs logging route. Preserve generic owning fallback through one promotion boundary; update producers and every affected consumer together. A trivial exact-call demonstration alone does not close the target. |
| 4 | Fresh kwargs proof coverage; Terra diagnostic + Sol contract review | Count dict-specific freelist hit/miss and reference-tracer outcomes on the actual overlap route. Propose a no-callback creation edge only if coverage and GCC work justify it; ordinary creation and final guards remain on decline. |
| 5 | Regression and machine interface; coordinator + Sol debug | Add spectral to every integration screen. If it becomes a gate, compare P1/P2 versus R5 before the full batch to locate the worsening increment; do not assume layout or alter GCC flags from samples alone. Return/POP continuation and operand/decode changes need their own observer/cost case. |

The [source comparison](borrow-frame-followup.md),
[revised proposal](borrowed-frame-views-proposal.md) and
[round-two peer review](borrowed-frame-views-peer-review-round2.md) approve
only diagnostics and the representation decision. Remaining preimplementation
proofs are explicit, rather than hidden behind “promote or decline.” The
[fresh-kwargs analysis](followup-analysis.md) explains the tracer constraint.

Each implementation increment gets the smallest meaningful ownership/generator/
callback test, release/debug compile and independent post-review. At integration,
run two balanced target/control rounds on `logging_silent`, `richards`,
`deltablue`, `comprehensions`, `float`, `nbody` and `spectral_norm`. Keep CPU15,
GCC and JIT-off conditions fixed. Use full suites and the broader/perf campaign
at the coherent next checkpoint. Root owns builds, native probes, quiet timing,
commit and keep/reject decisions; worker roles above are a plan, not live
assignments. Preserve failed attempts and update the ledger when evidence
changes the design premise.
