# Amend the first-tier proposal; decode before implementation

2026-11-01. Fable's direct medium-effort [review](claude-fable-5.1-review-20261101.md)
returned terminal0. The coordinator adopts **amend**, with the following
source-grounded dispositions. Original design bytes and source facts stay frozen.
There is no source implementation or native-tier execution grant.

| Peer finding | Coordinator disposition |
| --- | --- |
| Observer guard needs more than version equality | Accept explicit guards, but the frozen design already said equality alone is insufficient and required no tracing/profile/monitoring. Require actual instrumentation-disabled state, trace/profile hooks and reentrancy checks, tagged-values enabled, ordinary entry and no pending breaker; invalidate descriptors on instrumentation changes. Do not call the original prose a version-only implementation. |
| NULL compact-int input can dereference NULL | Accept. Check non-NULL before any compact-int accessor. Admit only proven bound borrowed locals; a separate potentially unbound `LOAD_FAST_CHECK` remains unsupported. Retain an explicit negative test. |
| Per-word IP writes inside a no-call leaf add work | Accept a pure scalar interval: no allocation, callback, ownership release or error creation; frame homes/IP stay at entry until success. Overflow declines without changes; commit publishes the continuation. Observer admission and bounded execution must establish this contract; do not transfer the result to calling or yielding regions. |
| Entry temporaries already start NULL | Accept live-out-only commit where actual frame initialization and home versions prove it; retain guards for local overwrites. Do not emit redundant NULL stores merely to mirror interpreter steps. |
| Native assembler/publication is already almost ready | Reject that premise. `_Py_JIT` currently compiles publication away; emitter return/32-bit-store forms and correct code lifetime are real work. Keep them explicit. |
| Add positive-constant floor division | Defer selection until the actual word witness. This could extend the same consumer from four to seven integer operations, but no lowering expansion is granted yet. Choose one exact candidate before implementation and scores; no adaptive rescue after an elapsed loss. |
| Defer profiler registration/CFI | Narrow. A first no-call, RSP-preserving correctness/elapsed experiment may defer publication, but must report unavailable profiler/unwind coverage. Correct publication/leaf unwind remain landing obligations after a useful elapsed result; do not install stencil-frame rules or claim an anonymous code page is already profiled. |

Local SSA versions of finalized homes remain a reasonable finite representation
for a straight-line consumer. They neither validate memory homes as the best
general IR nor supply loop joins. Actual emitter register assignment, moves,
spills, admission and commit costs must be reviewed separately.

The immediate next gate is a **ten-minute observational word capture** on the
qualified RC image, using existing `_testinternalcapi.get_ir`, `dis` format
readers and exported `_PyCode_Instr*` accessors. Bind actual code/stdlib/CAPI
origins and record pristine versus executing words, folded-local masks,
destinations, constants, feedback and exception ranges. Do not run a benchmark
or add a compiler merely to inspect this representation. A mismatching consumer
stops the proposed shape; it does not authorize a broader compiler.

After that witness and critical RC qualification, review one exact bounded
source attempt. First implement the admission/rollback/commit contract with a
scalar C leaf and focused behavioral/exit tests; then replace the leaf with
machine code and repeat those tests plus encoding/capacity/W-to-RX failure
checks. These are stages of one implementation, not permanent runtime options.
Do not promise that all infrastructure fits ninety minutes.

Positive native-execution evidence needs changing arguments and a consumed
result. A helper such as `return (x*3+y)-(y-1)` can keep the recurrence in its
caller; `x=x*3+y` inside a helper may instead decline because a boxed argument
owner would be overwritten. Keep that latter case as an ownership-decline
control, not an assumed successful native path. The actual words decide.

Preserve the proposed fixed elapsed/RSS comparison against the qualified RC
parent, including the universal entry-check tax on controls. Freeze the exact
primary/aggregate/variability decision before scores. A loss or inconclusive
result receives no extra samples or broader lowering menu. Useful elapsed
precedes expensive qualification and publication closure. The collector remains
RC; no GC work is reopened by this review.
