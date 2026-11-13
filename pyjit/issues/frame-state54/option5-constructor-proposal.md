# Option 5 follow-up: establish geometry once

Proposal only. Keep the initial candidate's release-mode geometry fallback
until its measured screen is complete. Do not modify or relabel that candidate
to hide the cost of its four geometry checks.

The selected constructor does **not** reject all non-PRISTINE handlers.
`Objects/codeobject.c:_PyCode_Validate` admits any valid handler;
`check_words` validates operand extents and feedback. `_PyIR_H_PRISTINE` is
descriptive metadata, not an admission gate. Thus a public constructor can
currently supply OVERWRITE_TEMP with hostile geometry, and the initial
candidate's runtime fallback is necessary even though the compiler emits only
ordinary BINARY_OP and specialization selects the refinement.

A bounded follow-up can move this geometry authority to construction and
selection. Add one narrow validator for the actual refined handler requiring
A==B, C!=B, nlocalsplus<=B<IR_MAX_SLOTS and the existing operand/frame extents.
Apply it in the common constructor validation used by compiler-created and
public code, including code.replace and internal/test constructors. Audit any
static/deepfreeze producer separately; its ordinary words must not bypass a
refinement check. Retain specialization's existing once-per-selection geometry
predicate. Then the refined handler can omit the repeated release fallback,
keeping debug assertions of the same geometry and its narrow old-inline/new-
canonical publication invariant.

The proof binds geometry to the actual executing code's local boundary, not
mutable function code. Pristine bytes are immutable; making a changed code
object must validate again. `Python/ir.c:make_exec` copies those words. Existing
specialization and instrumentation replace only the handler field, retaining
operand fields; inspect every installer of the new refinement, including
restore from original words and any polymorphic row. The candidate already
canonicalizes polymorphic rows to ordinary ADD_INT. Continue doing so, and
allow refinement installation only from checked original words or the checked
specializer. Internal C code that manufactures an execution image directly
must satisfy the same contract, rather than relying on a debug-only assertion.

Removed work is the per-execution A/B comparison, B/C comparison,
discard comparison, local-boundary comparison and their branch path. Added
work is a cold per-constructed-refinement geometry proof; specialization's
existing selection cost stays. No extra bitmap, frame field, instruction bit,
allocation, or general decoder is needed. Root must verify actual native
comparison/branch removal and report text, dependency and reached-site changes.
Do not infer a win by subtracting source expressions.

Negative constructor tests must exercise A!=B, B==C, B local, discard and
out-of-frame operands using matching source metadata so they reach this
validator. Valid constructor-supplied geometry must retain the same inline
guards, overflow/error behavior and observation as a specialized compiler site.
Test code replacement, instrumentation enable/disable and canonical restore.
Do not merely remove the runtime check because the normal compiler does not
emit malformed input. This follow-up requires independent review and root GO
after the initial candidate's screen; no implementation is authorized here.
