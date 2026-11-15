# Unit A69: independent exact source-contract peer

Status: **exact source-contract GO** for
`guarded-summary69-unit-proposal.md`, SHA256
`42195df94d5d6ed30961b1044478b5755d050be09bbe4da8212f554d1272cc93`.
The complete initial proposal and complete amended3f795 version were read,
then the final builder-context/name amendment and hash were verified. All
concrete findings below are resolved in this final contract. Root owns the
implementation release; this is not actual-source/product or runtime GO.

Authority: actual selected57 I,
`/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-integrated`,
commit `0dcba75bb3df40c32137db453b001f7e8f101de0`. Architecture/adoption peer is
`guarded-summary69-design-peer.md`; final grouped docs are Astra193b+a093.
Reviewed the complete unit proposal against actual source, including the
CALL emitter, frame setters, error ingress and feedback history. Coordinated
one explicit correction with the proposal author. Own only this peer memo;
no runtime edits, target execution, generation, builds, profiling or agents.

## Resolved completion finding and approved bounded branch

`IR_MOVE_TO_DST_OR_DISCARD` (`Python/ceval_macros.h:708`) does not consume a
pending redirect. `value_operation_refresh` (`Python/ceval.c:1350`) restores
the supplied operation PC. Existing `emit_ordered_retirement` calls
`retire_published_values`/`finish_value_bridges_ex`, which performs that reset
*inside* the helper. Classifying only after that helper returns loses the
actual landing. This is a concrete source-contract issue, not an existing
generic B0 capability.

The agreed direction preserves the adopted atomic/native-style scheduling:

1. Evaluate all guards and the bounded expression before any semantic effect.
   Acquire the canonical result. Capture every original input owner into its
   source-declared bounded owning C scalar, vacate its input home once, and
   publish the sole result in B0 before any calling close. No original input
   is subsequently fetched from the parent window. No array or second frame.
2. Retire those original scalar owners exactly once using parsed ownership
   actions. The noncalling close prefix needs no generation bookkeeping.
   At a real may-reenter boundary, snapshot the parent's actual frame-object
   generation, operation site and pending state. A NULL frame object has
   generation0; creation followed by a same-PC redirect is detected too.
3. After each callback, classify actual control *before* any PC-restoring
   refresh. SAME means original PC, unchanged generation, no pending flag.
   PENDING means deferred CALL redirect; preserve its actual return PC and
   keep B0 until final completion. APPLIED means changed actual PC, changed generation with no pending flag,
   or a pre-callback pending1 becoming pending0 even without another generation
   increment. Same-PC landings are included.
4. APPLIED is monotonic for this completed CALL: finish retiring only captured
   original scalars, then dispatch the actual landing. Never read, clear or
   publish into the old B0/window, which may contain new landing owners.
   Skip the superseded CALL's service/recipient steps. The next admitted
   summary still requires its full pre-body eval-breaker guard.
5. SAME/PENDING use the actual reached end-periodic boundary, including
   LOAD_SPECIAL suppression. Reclassify immediately after that possible
   callback, before any old-PC refresh. Successful PENDING takes B0 once into
   existing `_PyFrame_SetReturnValue`/`_PyFrame_CompleteRedirect`; SAME uses
   the generated destination/discard. Each subsequent callback must respect
   the already-committed result and actual control, not republish B0.

A summary-only cold single-close/control continuation can implement the new
classification without changing universal ordinary hooks. Its role is one
parsed owner release plus reached-control validation, not another owner-drain
or callee execution engine. The exact source must show this distinction and
must not silently call the existing PC-restoring helper first.

For end-periodic failure after originals have drained: SAME errors at the real
CALL with B0 still owning the result; PENDING cancels its deferred success flag
and takes the same CALL error route without successful result publication;
APPLIED errors at the actual landing with its actual homes, without touching
the old window. Pending cancellation agrees with actual child-abrupt cleanup
(`ceval.c:2386–2388`). No callback follows cancellation before error ingress.
Checkpoint0 remains initialized NULL-control authority, not a manufactured
stronger home certificate. Preserve a pending exception across any remaining
required close. A result already moved/discarded remains committed even if
that recipient's close redirects or fails.

Actual frame setters increment `f_lineno_generation` on successful same-PC
redirect (`Objects/frameobject.c:3528,3836`), may defer a child CALL redirect
(`:3544–3576,3842`), or detach surplus before a completed landing's callbacks
(`:3784–3865`). `_PyFrame_CompleteRedirect` (`Python/frame.c:217–228`) owns
the deferred scratch clear. The proposal must use these reached distinctions,
not infer completion from instruction-pointer equality alone.

## Producer/storage/feedback feasibility

The proposed160B scalar tail is internally consistent:16B header, four12B
paths, four16B projection records and two16B global/class records. Keep the
existing16B plan header and derive aligned tail position from real counts.
Summary-only plans are owned even with zero ordinary rows; free/sizeof and
overflow/alignment must change together. Existing non-summary allocations and
offsets stay unchanged. Validated signature/kinds/constant-slot maps are actual
builder inputs; common constructor/replace/marshal/preparation cannot install
stronger summary authority from words alone. OOM is a preparation error.

The existing CALL payload remains16B,32B including `_PyFbHeader`. Three uint16
scalar binding fields plus exclusive0x80 flag fit without borrowed pointers or
object owners. Keep version/provider/current code-tail association checks.
`fb_begin_ex` (`Python/specialize.c:488–514`) clears the payload; publishing the
arm precedes publishing its handler. Other binding arms are exclusive.

The initial proposal invented an existing CALL historical-row restoration
reader; the final proposal corrects this. Actual `fb_row_of:436` records only version/type for CALL and
`fb_put_row:447` records its handler; no binding fields are stored. The actual
runtime historical consumers are shape nomination (`specialize.c:605,962`),
shape reads (`ceval_macros.h:793`) and a readonly test exporter. There is no
CALL history-row-to-live-handler restoration to port. Leave those rows inert;
revisiting a callable must reconstruct the summary arm through the current
specializer. Do not add a restoration feature or widen `_PyFbPayload` for prose.

## Bounded expression and actual readers

The final proposal fixes the seven source-declared owner shapes: no-self arities0–3 and self plus
explicit arguments0–2. Existing `CallRoleFamily` supplies input-hole semantics
(`ir_generator.py:196–225`), not a general arity expansion engine. Each chosen
form must have parsed owner moves/POP/commit/error actions; marker names alone
grant no authority. Input holes are not read or vacated; a valid result
recipient may still initialize a scratch cell that was an input hole.

Three fixed expression forms and an explicitly unrolled maximum-four-step
borrowed path are feasible without an instruction interpreter. Root/read-kind
switches, repeated path evaluation and the bounded four-row shape PIC are real
costs. Immutable site coordinates plus actual callee entries supply guards;
never use caller IR_FB/frame or learn feedback during speculation. Every
primitive must type-check the actual operand before pointer/type access,
refuse absent/current-unsupported facts, and preserve canonical nonNULL reads.

Shape/slot guard strength comes from actual owning handlers
(`bytecodes.c:2664–2740,2799–2825`), with managed layout/version/shape facts.
Global/class reads need the current function environment, table versions,
bounds/current nonNULL values, class and required metaclass versions. Same-code
function versions do not identify environments or current attribute values.
Reusing an owning handler as a borrow primitive would be a source defect.

Base-object EQ/NE is feasible only with same actual operand type and both
actual provider fields. The proposed symbols exist: `_PyObject_ValueRichCompare`
is declared in `pycore_value_ops.h:177`; `_Py_BaseObject_RichCompare` in
`pycore_typeobject.h:153`. `Objects/object.c:1103–1175` selects the actual
value provider and then the object fallback/identity. No provider call is
needed on an admitted guarded case. Unknown/custom/reflected cases decline
before effects; comp hits remain unproved. Select preserves actual RHS
identity and branch-local reads, not just truth.

Full entry/tool/recursion/stack conditions precede the expression, including
actual first plain RESUME/full eval-breaker equality, current provider,
trace/profile/caller+callee monitoring/DTrace/debug LLTRACE, FT denial and
Emscripten refusal before poll mutation. Atomic asynchronous display of the
real parent at CALL is accepted. No virtual frame, new schema or universal
owner/effect hook is needed. Nonparticipants keep ordinary hot handlers;
cold nomination/misses and optional plan/preparation costs remain charged.

## Actual-source and runtime gates

Source implementation must refuse a certificate after mutation of a primitive,
guard, source-owner/commit/error action or role shape; it cannot receive a
blanket CALL_DRAIN/name exemption. The separate generated product review must
verify only the intended families and conservative effects. Root alone runs
both builds and real-object tests for callbacks/aliasing/recipients/errors,
same-PC/applied/pending redirects, public images, binding/history lifetime,
tools/pending/recursion and actual projection/select hits. Complete native
pricing and the protected screen remain separate gates. No source-only result
is runtime qualification or a predicted speedup.


## Final exact readback and implementation release boundary

Frozen42195 resolves the old raw-B0/refresh assumption with the explicit
summary-only cold continuation, captured original scalar owners and a
NORMAL/PENDING/APPLIED outcome. Applied control remains sticky across further
original-owner closes; pre-pending1 becoming0 is also an applied transition.
Classification precedes any helper that resets PC, including the end-service
and final recipient/discard/displaced-local callbacks. Once a recipient has
consumed/moved the result, a new pending redirect uses CompleteRedirect directly;
there is no second B0 TAKE or result publication. NORMAL resumes original next;
APPLIED dispatches actual current PC. Poll errors follow the explicit three-way
policy above, with pending cancellation before error ingress and no old-home
certificate on an applied landing. This is a viable control consumer and needs
no universal owner hook, argument array or second clearing engine.

The final producer signature is the private
`_PyIR_BuildCallSummary(PyCodeObject *co, const semantic_image *image,
SummaryCandidate *out)` from `_PyCode_BuildSemanticPlan`. That supplies actual
signature, flags, names/constants and constslot maps without growing the common
semantic image or code header. Publicly constructed/replaced/marshaled code
receives the same derived proof during preparation; no tail or learned handler
is trusted from serialized/public metadata. This closes the binding-context
issue without a new global analysis lattice. The final history paragraph names
actual `fb_saw_other_shape` and leaves CALL history inert. Both real EQ symbol
declarations, all seven source owner shapes and input-hole qualification are
explicit.

No unresolved contract CHANGE remains. Root may release the bounded producer
and consumer implementation as one unit under this exact hash; no component
is independently enabled/nominated. The shared160B record/enums/accessor must
be frozen before parallel consumers reference it. Actual-diff review still
must establish that the code implements these owner/control/read proofs and
that malformed source loses its capability, followed by root's generated
products/build/runtime/native/selection gates. Neither this GO nor the source
contract establishes hits, cost targets or useful throughput.
