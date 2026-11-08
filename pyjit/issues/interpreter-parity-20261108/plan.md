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

Selection correction after user review: the eight V8 recommendations are a
set of alternatives to assess, not a selected call implementation sequence.
Further implementation is held while the coordinator compares structural
coverage and migration costs. P1's three-line refresh cleanup is preserved
uncommitted in the isolated worktree; it has not been built, tested or landed.
See the [recommendation ledger](recommendations.md) and the independent
[architectural alternatives review](alternatives-review.md).

The table assigns owner roles, not live agents. The initial comparison,
documentation, baseline audit and mechanism-review packets are complete.

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
1.74303 result. Source remains R5. The eight-item V8 review is complete;
architectural alternatives and recommendation selection remain open.

Independent result review reproduced the baseline. Existing stats identify
161 dispatches and 20 frames per workload loop, including nine retained POPs
and ten line-boundary NOPs. The first mechanism proposals and two peer-review
rounds are complete; observer and dictionary-invalidation proofs remain open
before implementation. See the [comparison checkpoint](comparison-checkpoint.md),
[diagnostics](diagnostic-results.md) and [mechanism review](mechanisms-peer-review-round2.md).
