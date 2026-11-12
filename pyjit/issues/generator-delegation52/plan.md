# Generator delegation plan

Build explicit delegation and suspension semantics into the register IR and
interpreter. The selected baseline is golden51, `0e5e61f4c6b`; it stays immutable
while the candidate is reviewed, implemented and measured. The
[design](../../design/generator-delegation.md) incorporates the single broad
[Astra review](astra-review.md); [resolution](review-resolution.md) and
component peer review are complete. Source implementation is in progress; no
new native qualification or performance result is established.
[Status](../../STATUS.md) owns admission and the
[baseline report](../interpreter-round51/report.md) owns existing measurements.

## Hypothesis and scope

Current generator samples rank SEND_GEN at 27.4%, YIELD_VALUE at 13.9% and the
backward jump at 5.1%. These are locations, not removable fractions. The compiler
knows yield-from/await delegation but emits SEND, delegated YIELD, RESUME and a
backward jump. A semantic DELEGATE operation and explicit suspension/resumption
continuations can remove repeated instruction routing and unnecessary value
publication while keeping one owning-frame model.

The design covers yield, completion, send, throw, close, ordinary/native provider
fallback, ownership, exception state, frame inspection and observation. The first
implementation must be a coherent semantic slice, with explicit limits. A renamed
SEND or jump shortcut alone does not establish the proposed abstraction.
Whole-chain forwarding is separately gated; no assumption permits skipping
required running-state, recursion or exception/frame visibility updates.

## Workstreams and gates

1. **Design:** Sol writes the operation/state/encoding/ownership contract and an
   independent Sol audits current consumers and feasibility. Include concrete
   same-word encoding, source-independent semantic PCs, callbacks/redirects,
   invalidation, fallback and the actual work removed.
2. **Broad review:** Astra reviews the complete proposal once. Resolve blockers
   and incorporate feedback before implementation. Use Sol for subsequent
   component review/debugging rather than Astra in the inner loop.
3. **Implementation:** divide compiler/IR/metadata and runtime/observation work
   by explicit interfaces. Use isolated source subtrees or nonoverlapping file
   ownership. Port all affected consumers and generated products. No optional
   competing evaluator, owner ledger or load-bearing debug coordinates.
4. **Focused integration:** regenerate, compile release/debug, run targeted
   producer/constructor negatives and protocol/ownership/observer tests. Obtain
   independent review of the actual source and native path. Use the smallest
   meaningful checks while components are changing.
5. **Performance:** compare the coherent candidate directly with actual golden51
   and upstream using unchanged kernels and one GCC configuration. Screen, then
   confirm claimed gains and material losses. Reject ineffective work or retain
   a clearly bounded architectural increment only with measured benefit.
6. **Final qualification and promotion:** run all29 guardrails, applicable full
   release/debug diagnostics and the value API gate at the final integration
   point. Keep GDB PrettyPrintTests quarantined, other checks enabled. Report
   remaining established failures. Promote only after final independent review.
7. **Close:** update selected contracts, report, tables, plan, status and Loom
   mirrors. Remove task-only scratch/worktrees after retaining required evidence;
   preserve selected builds, immutable controls and all unrelated work.

## Measurements

Hot set: **generators, coroutines and DeltaBlue**. Mandatory **logging_silent**
guard; comprehensions is an additional suspension/container guard. Two balanced
pairs screen; six confirm target gains and suspicious losses. All29 unchanged
kernels run at the final integration point against golden51 and upstream.

Use GCC15.2 with the existing release configuration, CPU15, hash0 and JIT off.
Ratios are candidate/control elapsed time; lower is faster. Preserve raw vectors,
source/native/module/configuration identities and each regression. Do not multiply
ratios from separate campaigns. Serialize quiet timing and PMU collection against
builds, tests and bulk cleanup. Native instruction/profile evidence supports the
mechanism but does not replace application elapsed time.

The comparison must answer both whether the new model helps and what gap remains
versus upstream. A first candidate gets one measured decision; follow-ups require
a concrete source/native finding, not an open-ended rescue queue. Startup RSS,
zero-box counts and exact CPython finalizer timing are not throughput gates.
