# Guarded cache payloads and the union rule

Proposal only, 2026-11-10, against clean runtime43 `47dc23f48cf`.
No runtime/checker edits, build, native run or commit accompanies this proposal.
Astra review precedes implementation. The 306-unit API qualification reports
exactly two new rawunion findings, at the attr and snapshot payload declarations.

## The failure is a policy mismatch, not a green qualification

`Tools/build/check_value_api.py:273–327` includes `_PyValueGuarded` in WORD_TYPES.
WORD_HOLDER finds words, arrays and nested structs; rawunion rejects any union
containing them. Its stated rule forbids this even when no inactive member is
read. `parse_output` permits encoding-aware files/functions, not feedback.h.
Existing negative units cover owner, word-array and wrapped owner/borrow unions
(`raw_bits_union*`). This is an intentional conservative boundary. Current43
therefore fails the API gate despite passing native tests; do not relabel the
failure as passing or silently remove guarded words from the check.

The present runtime stores `_PyValueGuarded` as non-owning metadata. Attr handlers
load its transient borrow only after guards; snapshots select members using the
recorded opcode and copy metadata without producing a borrow. That explains why
a handler-tagged union can be legitimate. It does not make arbitrary union use,
byte-copy laundering or a declaration-name exception legitimate.

## Recommended repair: independent guarded storage

Keep the rawunion rule unchanged. Move `_PyFbAttr.class_value` outside its
existing pointer/shape union. Make `_PyFbPayload` a struct containing its
native-only descriptor/shape union and an independent `_PyValueGuarded
class_value`. No union then contains a value word or a nested word holder.
The existing generator bindings, guarded post-version load and handler-selected
snapshot semantics remain the same; C field offsets change through declarations.
Do not manufacture canonical values for native C descriptor pointers.

On this 64-bit profile, attr size is **40 → 48 bytes**. Snapshot payload size is
**8 → 16 bytes**; with FB_POLY_SIZE=4 the aggregate `_PyFbPoly` is **88 → 120
bytes (+32)**: three uint32 arrays contribute 48, handlers 8, payloads 64.
The tentative 112/+24 estimate does not match those declarations. Root's real
compile must verify new sizeof assertions before any result is reported.

`fb_row row = {0}` initializes both payload fields. The selector fills only the
field its handler uses; whole-struct snapshot copies are metadata copies with
both fields represented independently. Unused guarded metadata stays zero and
never becomes a borrow. Retain active-handler selection/reset tests: storage
separation avoids punning, but it does not authorize consuming an invalid cache.
Attr's unused pointer union can still carry object pointers or shape indices;
no guarded/owning/borrow word may be placed back into it.

Update the two size assertions and verify feedback size/alignment/4096-unit
allocation behavior, metadata generation and `Python/ir.c`'s existing
`capacity * sizeof(_PyFbPoly)` accounting. Larger attr entries can change which
late sites get private feedback, not just total bytes. A source44 test/retiming
packet must therefore include cache admission/coverage and class-attribute plus
logging/DeltaBlue/coroutine instruction/elapsed checks. Report actual metadata
bytes and any changed private-feedback limits; startup RSS is not the gate.

This supersedes the proposal's original size-preservation objective for an
actual representation-policy reason. It requires no separate user safety or
compatibility approval. The reviewed semantic optimization stays intact; the
strict encoding boundary is retained. Independent proposal review still precedes
runtime edits, and root owns API qualification before broader tests/retiming.

## Deferred alternative: a typed handler-discriminated payload

The following contract is viable design work if the measured independent-field
cost proves material. It is not the recommended qualification repair and is not
permission to add a type-name exemption.


Consolidate the attr/snapshot union into one named `_PyFbPayload` type, with
exactly four non-owning views: object pointer, native C specialization-descriptor
pointer, `_PyValueGuarded`, and uint32 shape-transition index. Keep binary native
descriptors distinct from language values: a C descriptor pointer is not a
PyObject and must never pass through a PyValue constructor. Retain the current
40-byte attr and 88-byte poly aggregate sizes. Do not add a runtime tag/frame flag;
executing/recorded handler identity remains the discriminator.

Expose only typed operations, with names provisional:

- Object, native-descriptor, guarded-value and shape constructors/setters accept
  their exact C type and initialize that same payload member.
- Readers return only that member's exact type. The guarded reader returns
  `_PyValueGuarded`, **not** PyValue/PyValueBorrow; the existing guarded loader
  remains the separate operation that creates a transient borrow after guards.
- Snapshot construction takes the recorded handler, selects the live view and
  copies it through the corresponding typed operation. It cannot blindly copy
  a pointer view of guarded bits. Whole-payload assignment between the same
  discriminated observation is a metadata copy, not a view conversion.
- Attr producer writes and cache-operand generation use those operations.
  Descriptor/fget/getattribute cache operands retain PyObject* declarations;
  class-value operands retain guarded metadata declarations. Consumers never
  address a union view or read a different view to recover raw bits.

The implementation may use a small containing struct with a private named
union, allowing the checker to recognize direct payload-member access reliably.
The exact declaration and accessor schema, not a comment, typedef spelling or
source line, defines this reviewed boundary. This is a narrow metadata ADT,
not a new value representation or a generic tagged-value abstraction.

## Checker change required for this contract

Keep the general rawunion matcher and every other raw-bits/borrow/public rule.
Before suppressing its one declaration finding, validate **all** of:

1. The spelling path, declaration identity and complete canonical field schema
   are the reviewed internal payload. The sole word field is exactly
   `_PyValueGuarded`; no owning PyValue, typed borrow, word array, nested word
   holder, integer/raw-bit alias, byte buffer or pointer-to-word alternative is
   allowed. A renamed/copied declaration elsewhere remains a rawunion failure.
2. Every direct member access to that payload is in a finite typed accessor set.
   Validate exact function signature and body/AST shape, not merely name/path.
   Constructor and reader must use the same member; pointer reinterpretation,
   raw .bits access, taking a member address and cross-view copy fail.
3. Other units use accessors or a whole-payload metadata copy only. Add an AST
   rule for direct member access/address/cast/initialization outside the verified
   boundary, including accesses written in feedback.h itself or macro arguments.
   A file-wide feedback.h/encoding-file allowance is not acceptable.
4. The selector mapping and generator cache binding remain explicit: guarded
   metadata only for the two NONDESCRIPTOR handlers, object pointers for method/
   property/hook/class handlers, native descriptor for BINARY_OP_EXTEND, shape
   index for STORE_ATTR_SHAPE_ADD. Review snapshots against their passed opcode,
   including zero/partially populated observations. Modified selector/operand
   mappings must fail dedicated source/codegen contract tests, not become an
   unexplained new supported case.

The checker can enforce a finite exact schema/accessor/mapping contract without
claiming general C union dataflow soundness. If its clang-query output cannot
provide reliable declaration/accessor identity and member types, do not implement
a looser name filter: choose the separate-storage fallback below. This feasibility
check is part of proposal review, not a proof already completed here.

## Tests and decision

Retain all existing union/array/wrapped-owner negatives. Add positives for each
typed payload view and active-member snapshot, and negatives for:

- `_PyValueGuarded` pun union outside the boundary, including typedef/nested forms;
- owner/borrow/array/raw-integer/byte alternatives added to the approved schema;
- altered accessor return/parameter type, inactive-member return, address escape,
  raw-bit access or pointer reinterpretation inside an approved-name function;
- direct view access in a runtime consumer, header macro or same header outside
  an accessor; forged declaration name/path and copied accessor text elsewhere;
- cached scalar snapshot through the pointer view, or a handler/operand selector
  changed to an unreviewed view.

Run the checker's own positive/negative suite first, then the full API gate;
root owns generation, release/debug compilation and focused cache/runtime checks.
Astra reviews the actual checker diff and new negatives before admission. No
qualification finding may be discarded by postprocessing without the validated
contract that explains it.

The checker must also continue to reject a union containing the metadata ADT,
including through arrays and wrapped/nested holders. Add the ADT explicitly to
holder classification if its containment would otherwise stop the current
WORD_HOLDER matcher from seeing the guarded field. Its own approved boundary
must not grant a transitive exception to an outer union.

Discriminator coupling is an additional obligation: one canonical handler →
payload-kind map must govern setters, cache binding and snapshots. Mono reset
clears payload before replacing its opcode; no specialized opcode may read the
cleared row. Poly construction must publish copied typed metadata and its
matching recorded handler together before exposure. Stale observations may be
copied as inert words but never converted to borrows or object pointers without
a current owner/version proof. Source/codegen contract tests must reject changed
selector/write/reset ordering. None of these checks is implemented or proved by
this document.

**Recommendation:** approve independent storage for the bounded gate repair,
then measure its footprint and cache-admission consequences. Keep typed metadata
ADT validation deferred unless those costs justify its checker/contract scope.
Unifying every attribute/native C pointer as a canonical language word is a
broader, inappropriate change here. No blanket union exemption, removal of
_PyValueGuarded from WORD_TYPES, or unreviewed layout change is authorized.
