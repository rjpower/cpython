# Throughput follow-up after the calls/frame/publication batch

Execution proposal, 2026-11-06. Start from the final runtime identified in the
[batch plan](plan.md) and its performance report, not an older worker tree.
GCC, build flags and the selected value representation remain fixed. The
checkpoint does not imply upstream parity or completion of every value-native
adapter. Absolute zero boxing and startup RSS are not performance gates.

Current checkpoint: Integration C is `7780e32e0e2`; cold review accepts it and
the basic matched screen is mixed, with no broad win established. Release
and debug full suites both passed, so the functional gate is complete. The
final 29-kernel and pyperformance comparisons are running serially; no
performance conclusion is available yet. See the
[current status](../../STATUS.md) and [Integration C receipt](integration-c.md).

## Admission before more implementation

Finish the frozen measurements and the [admission review](admission-review.md).
Distinguish a removed source store, fewer executed native instructions and a
repeatable elapsed-time improvement. The seven-kernel screens alone do not
establish the last of these. In particular, keep the overlap-clear experiment
under review: additional bounds arithmetic may outweigh a short saved clear.
Retain the independent assertion/ownership fixes regardless of that decision.

## Parallel packets

| Priority | Packet | Owner and boundary | Smallest useful result |
| --- | --- | --- | --- |
| 1 | Attribute/helper overhead | Terra medium owns one generated helper sequence; Sol reviews its effects and ownership | Identify an actually frequent retained GCC sequence in a remaining regression; propose one shared operation contract and remove only proven repeated work |
| 1 | Remaining call adapters | Luna medium inventories selected hot call shapes; Terra medium owns one adapter; Sol reviews prepare/commit | Separate common overlap, exact overlap, keyword/EX, generator, instrumentation and native paths before selecting another fast route |
| 2 | Useful PyValue ports | Terra/Luna medium own bounded caller groups after one reviewed ownership recipe | Choose a measured workload with material bridge cost; preserve owners, errors and callback order rather than optimize census counts |
| 2 | IR phase/effect contract | Sol designs, Terra implements one producer/checker increment, Luna syncs references | Give consume, publish, retire, observe and failure phases one generated definition when another real producer needs them |
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

A concrete call candidate is cold-provider outlining. The final GCC
`call_transaction_push_python` still reserves `0xf8` bytes and loads a stack
canary before its common-overlap guards; the same function contains the slow
provider's local argument array. Check whether separating that consuming
provider removes this prologue from the common route without increasing calls
or spills elsewhere. The retained instructions establish a hypothesis, not
their dynamic cost or an expected speedup. Keep prepare/guard/commit ownership
unchanged and require a real same-workload comparison before admission.

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
SSA, recovery states and machine register allocation remain separate layers.
Before resuming native method/tracing work, review shared effects, Action and
FrameState contracts, exceptional recovery, invalidation and generator/OSR
entry. The detailed open questions remain in the canonical register-IR and JIT
designs; this batch does not close them by improving the interpreter.

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
