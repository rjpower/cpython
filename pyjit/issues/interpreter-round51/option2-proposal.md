# Option2: canonical sequence iteration publication

Status: proposed; isolated source `BASE/worktrees/round51-option2` is unchanged
at bf810. Release build directory is `BASE/round51-option2-release`. Root owns
all regeneration, builds, native execution and timing.

FOR_ITER_LIST/TUPLE currently duplicate a value from a canonical container home,
increment a tagged cursor, then run generic publication for both cursor and item.
The item first enters window[2], then the existing destination transaction moves
it to a folded local or discards it. This repeats already-proved representation
work and can bounce the item through an unnecessary frame home.

Add a bounded source proof beside existing canonical producer proofs. Validate
the complete three-uop chain: exact sequence guard, existing length/exhaustion
branch and scalar next producer; check scalar stack shapes, feedback padding,
format window/base/destination geometry and original parsed body tokens. Match
actual guard, canonical PyList_GET_VALUE/PyTuple_GET_VALUE duplicate and tagged
cursor increment expressions. Reject changed bodies, calls, helper shadowing or
stack/control transformations. Selection depends on the proven source contract,
not an opcode-name exemption. Preserve the existing list free-threaded branches.
No new runtime flag, opcode, serialized metadata or broad C-expression inference.

Reuse existing SlotLayout canonical output/result fields for the proven item and
cursor. Window publication uses IR_PUBLISH_CANONICAL_TEMP for the cursor. At the
final successful flush, the canonical scalar item goes directly through existing
IR_STORE_CANONICAL_DST to its actual destination (temporary, folded local or
discard); suppress the redundant final window-to-destination move only when that
store was emitted. The existing shared IR_STORE_DST_IMPL still owns publication,
old-local displacement and retirement; finish_displaced_local remains unchanged.
No new destination transaction is introduced. Other window handlers and observed
adapters retain their current publication path.

Ownership/recovery: the item is an owning duplicate of a canonical container home;
the iterator's frame owner keeps that source alive through acquisition. The
cursor is a structural tagged value stored only in a temporary. Publish updated
cursor before binding/retiring the destination. The item must be stored once,
including when the loop variable aliases the original container local. The
existing exhaustion IR_COMPLETE_STEP exits before item acquisition/publication,
so an empty/exhausted loop preserves its prior destination binding. Guard failure
retains generic FOR_ITER behavior. No callback is added between guard and acquire.
Retirement/reentry observes committed recipient/cursor state, with existing
error/redirect handling. Monitoring switches to its existing instrumented route.

Files: option2 `Tools/cases_generator/ir_generator.py`; new focused
`Lib/test/test_ir_iter_canonical_codegen.py` and
`Lib/test/test_ir_iter_canonical.py`. Avoid bytecodes/C/header changes unless
review finds the current shared destination interface insufficient. Generated
products belong to root. Generator tests assert positive real handlers and
negative mutated bodies/stack formats, canonical cursor publication, direct
actual-recipient store and untouched generic/range/virtual paths. Runtime cases
cover list/tuple tagged/object values, empty/exhausted destination preservation,
aliasing, overwritten-value finalizers reading locals/reentering Python, list
mutation and observer enable/disable. Assert specialization before mutation.

Acceptance requires source review, release/debug compile and focused semantics,
native removal of generic publication/item bounce in list/tuple handlers, and
matched target results against frozen bf810 with logging/spectral guards. No
speedup is claimed. Proposal due22:55UTC; implementation23:25UTC. If direct
recipient publication requires a second transaction or weakens observer/owner
proofs, stop and report rather than widen this experiment.
