# Option 2 proposal review

Approved for isolated implementation with the conditions below. Reviewed the
proposal against bf810 `_ITER_CHECK_*`, `_ITER_JUMP_*`, `_ITER_NEXT_*`,
`IRStack.store`, `emit_final_outputs`, and the shared destination macros.

Exact list/tuple home duplication produces a canonical owning item. The tagged
cursor is structural temporary state; its representation must never be exposed
as an ordinary Python result. Preserving those facts through final publication
can use the existing destination transaction without changing ownership rules.

- The proof must validate the full real producer chain and input/output/cache
  geometry, including exact type, exhaustion, accessor arguments and tagged
  increment. Retain free-threaded source guards and their current semantics.
  Changed bodies, helper shadowing, callbacks, arrays or different window/result
  geometry must fail closed. No opcode-name-only exemption.
- Direct recipient publication applies only to the final successful body-edge
  flush and the proved scalar item. Earlier flushes, failure paths and the
  observed deferred transaction retain their current physical homes. Temporary
  destinations must preserve the required B+2 identity; validate that relation
  before treating the final mover as redundant.
- Commit the cursor before any destination operation can retire a displaced
  local or discarded item. Acquire the item before replacing a local that may
  alias the container. The iterator window remains its independent owner.
- Suppress the final move only when this path actually emitted the direct
  recipient store. Do not infer this from the canonical flag alone or alter
  stack bookkeeping such that a later branch reads an unpopulated window slot.
  Keep the shared displaced-local finalization/error/redirect handling.
- Exhaustion exits before item acquisition and must retain the old destination
  binding. Mutation, empty loops, break, exceptions, subclasses and observers
  need the proposed runtime coverage. Generator negatives must distinguish
  canonical publication from direct-recipient geometry and ordering.

Root owns regeneration, release/debug compilation, focused execution and native
inspection. Admission requires both publication normalization and redundant
window traffic to disappear in real list/tuple body hits, with matched timing;
this design approval is no executed qualification or performance claim.
