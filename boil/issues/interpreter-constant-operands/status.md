# Reuse exact small constant operands

- State: active
- Owner: qualification_review_resumed; coordinator selects implementation
- Updated: 2026-10-31, 20:12 UTC
- Budget: offline research complete; 90-minute host-only prototype
- Source started: 2026-10-31 20:20:13 UTC; decision due 21:50:13 UTC
- Checkpoints: 20:50:13 domain/format; 21:20:13 fallback/observer/stack proof
- Depends on: [shared-interface review](../interpreter-shared-costs/reviews/holistic.md),
  [accepted union](../nursery-shape-landing/status.md)
- Source base: integrated `d98291018c9665ae3db6952da0b0a40a4e04992b`
- Native evidence: sealed union `e9e10d5b503`, GCC O3 image; saved profiles `0b15b4bbc9b`
- Branch/worktree: `boil/interpreter-constant-operands`; `build/worktrees/constant-operands`
- Decision: independently reviewed host-only prototype dispatched; runtime acceptance pending

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
The owner records its first-edit clock and 30/60-minute checkpoints. Source
review precedes any separately bounded native feedback or timing decision.
