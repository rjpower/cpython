# Throughput follow-up after the calls/frame/publication batch

Execution plan, 2026-11-07. Start from the final runtime identified in the
[batch plan](plan.md) and its performance report, not an older worker tree.
GCC, build flags and the selected value representation remain fixed. The
checkpoint does not imply upstream parity or completion of every value-native
adapter. Absolute zero boxing and startup RSS are not performance gates.

Current checkpoint: runtime `7780e32e0e2` is clean, cold C review accepts it,
and full release/debug suites pass. Final-v4 is complete: candidate / upstream
and candidate / interim elapsed factors are 1.263136 / 1.054877 on 29 kernels
and 1.084920 / 0.902391 on the selected 12-result subset. These noisy,
sequential comparisons establish no convincing broad elapsed-time win or
upstream parity. Candidate instructions per workload loop fell 2.2%, 6.7% and
1.4% in the selected counter workloads, but remain 2.001520x, 1.774821x and
1.605447x upstream. See the [final report](../../bench/results/interpreter-throughput-20261106.md)
for limitations and the [current status](../../STATUS.md).
The fresh [holistic source assessment](holistic-assessment-20261107.md)
distinguishes accepted ownership correctness from the unfinished whole-system
simplification audit and identifies one concrete dead-state cleanup.

## Admission before more implementation

Measurements and the [admission review](admission-review.md) are complete.
The coordinator retains B's bounded known-NULL initialization contract for
clarity and reuse of a proven ownership invariant, while its throughput
benefit remains unproven and the independent reviewer preferred dropping the
clear elision. Do not extend it or treat it as a speedup. Keep the useful
provider-decline assertion and ownership clarification. The final report
records this maintenance judgment and the separate measurement history.

## Parallel packets

| Priority | Packet | Owner and boundary | Smallest useful result |
| --- | --- | --- | --- |
| 1 | Call preparation and guards | Terra medium owns one bounded `Python/ceval.c` call path; Luna medium inventories call shapes in selected profiles; Sol reviews the protocol | Explain measured cost across annotations, frame preparation, guards/revalidation and adapters, not only the stack-canary prologue; preserve transaction and fallback semantics |
| 1 | Attribute/helper overhead | Terra medium owns one DSL/generator helper sequence; Sol reviews phase/effect/output facts | Remove only repeated work proven by producer and consumer facts; preserve publication-before-release and folded-local semantics |
| 2 | Measured PyValue ports | Terra and Luna medium own bounded native caller groups selected from measured profiles; Sol reviews one ownership recipe | Reduce bridge work in measured paths while preserving owners, errors and callback order, rather than optimizing census counts |
| 2 | Shared IR phase/effect facts | Sol proposes the contract; Terra implements one producer/analyzer increment; Luna updates documentation | Give consume, publish, retire, observe and failure phases one generated definition without parallel hand-maintained metadata |
| 2 | Call bookkeeping cleanup | Terra medium owns `_PyCallTransaction.effects` and its writes in `Python/ceval.c`; Sol checks the remaining protocol | Remove the write-only record field separately from generated `CALL_EFFECTS` uses; verify focused call behavior without claiming a speedup |
| Integration | Coordinator | Shared headers, generated products, builds, quiet measurements and current documentation | Review actual composition, commit separately, measure each integration and obtain a fresh cold review |

Start each implementation packet with a 60–90 minute proposal/first-result
budget and an exact file boundary. Return a source/GCC explanation and a scope
cut before expanding the packet. Do not build a general instrumentation system
or rewrite the binder to answer one call-path question.

The call inventory should count executed adapters and supplied/bound argument
shapes for the selected workload, with a denominator and unassigned cases.
Reuse existing statistics where they answer that question. Add a targeted
counter only when it changes the choice of patch. Do not convert aggregate
hardware counts into a per-bytecode cost without dynamic IR and upstream
operation counts for the same measured region.

A concrete call candidate is cold-provider outlining, considered within the
broader preparation/frame-setup/guard audit. The retained GCC function reserves
`0xf8` bytes and loads a stack canary before common-overlap guards, while also
containing the slow provider's local argument array. These instructions are a
hypothesis, not a dynamic cost estimate. The [call follow-up audit](call-followup-audit.md)
records source, disassembly and profile-annotation evidence. Check accepted and declined call
shapes, frame preparation and guard work together; require same-workload
evidence before admission and preserve prepare/guard/commit ownership.

## Design constraints

One frame stack and the shared overlap primitive remain selected. Fallible
preparation keeps the parent owning; final guards precede an infallible transfer.
Fallback cannot follow detachment. Unsupported call shapes remain valid uses
of the consuming provider until a reviewed destination-aware protocol replaces
them. Avoid another persistent call/frame representation.

Future-read liveness is not an ownership/root map. Keep all physical homes
valid for GC, exposed frames, copying, generators and unwind. A future sparse
validity scheme needs instruction-phase and exceptional-state semantics, not
just a compiler live set. Normal thread retirement already clears locals only.

Publication facts need precise output/home postconditions. The current five
attribute flows justify the existing narrow completion fact. A producer that
mutates that output after retirement must invalidate or reestablish the fact;
do not grow distributed caches and opcode exceptions around it.

The interpreter IR describes Python execution and frame homes. Optimizing typed
SSA, recovery states and machine register allocation remain separate layers;
their gaps remain open. Before resuming native method/tracing work, review
shared effects, Action and FrameState contracts, exceptional recovery,
invalidation and generator/OSR entry. This interpreter batch does not close
those design gaps.

## Validation and boundaries

Use a release compile and the changed opcode/helper's minimal semantic and
generator tests first. Batch full release/debug suites at coherent integration
points. At each landing, freeze source and executable identities, run the basic
matched screen, and send a fresh cold reviewer the integrated code. Expand
measurement only to resolve an actual attribution or regression question.

The full-suite conversion census remains useful migration evidence, with its
recorded accounting limitations. Its cumulative events are not live allocations
or a timing profile. Ctypes raw-pointer ownership and LRU implementation retain
their separately recorded approval holds; do not reopen those through a bulk
port packet. Preserve unrelated worktrees and historical evidence.
