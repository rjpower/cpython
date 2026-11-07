# Bounded canonical-home result experiment

Landed refinement: R5 uses the reviewed RHS form
`attr = IR_CANONICAL_HOME_DUP(found)`, rather than the original two-argument
statement proposed below. See [RHS review](canonical-output-rhs-review.md) and
[integration receipt](integration-r5.md) for the implemented contract and its
qualification. The original proposal remains design history.

2026-11-07. Design follow-up to `canonical-output-audit.md`, runtime source
`25ce1654801`; no runtime edits or measurements by this owner. The earlier
STOP remains valid for the *unamended* internal store interface. This packet
defines the narrow precondition and gates needed before a shape-result
direct-publication experiment; it is not an implementation or speed claim.

## Boundary decision

`_PyObject_StoreInstanceValue(PyObject *, PyObject *, PyValueBorrow)` is an
internal `PyAPI_FUNC`, declared in `pycore_object.h:918–920`. Its current
comment says borrowed value/NULL deletion but omits the general home
precondition. The surrounding value design already says `_PyValue_PointerSteal`
is for boxes/canonical constructors and that a home never receives a box
(`pycore_value.h:705–719`); `_PyValue_IsHome` defines canonical home bits and
`_PyValue_CheckHome` reports violations in debug (`560–630`). The callee
currently checks the stored value only **after** writing it to an inline home
(`dictobject.c:7231–7232`). Make the precondition explicit in its header:
`value` is NULL or a borrowed **canonical home value**. A raw pointer-form
boxed exact int/float, or an untagged static immortal, is not a valid input
to this home-writing API even though `PyValue_IsLanguageValue` accepts its
bit pattern. Check `assert(_PyValue_IsHome(value))` at entry in debug before
any duplicate, callback or store; unlike configurable `_PyValue_CheckHome`,
this assertion is fatal in debug. This is a precondition on the internal
PyValue API, not a new runtime conversion. It is compatible with current
in-tree callers and adds no release-path work. Extensions and native/JIT
adapters are rebuilt or ported for this runtime; a caller with only
`PyObject *` must use `PyValue_FromObjectBorrow` before calling the internal
home writer, as the public object adapter already does. Source or binary
compatibility with a caller that passes raw boxed pointers to this internal
API is not an admission gate; Python behavior through the public APIs is.

Do not silently accept arbitrary raw pointer-form exact numbers by calling
`_PyValue_Publish` on every general instance store: that would add an object
test and exact-type loads to the write path for ordinary values, and it
would not repair an untagged static immortal without a broader conversion.
This contract change is a hard prerequisite, not an optional debug aid. If
the coordinator instead decides to support out-of-tree callers that pass
merely `PyValue_IsLanguageValue` values, retain ordinary publication and
design a separate normalizing API. Do not silently optimize reads while
leaving the writer contract ambiguous.

The in-tree writer audit is bounded and concrete. `store_instance_attr_lock_held`
duplicates its canonical borrowed input into an inline slot;
`_PyObject_StoreInstanceValue` is called from the generic setter in
`Objects/object.c` and the `PyObject *` adapter in `Objects/dictobject.c`.
The latter calls `PyValue_FromObjectBorrow`. Public native
`PyObject_GenericSetAttr` and value-slot thunks also use `_Py_VALUE_ARG`,
which expands to that canonical constructor (`pycore_value_slots.h:345`);
`Modules/_decimal` uses the value-slot thunk for its override. The
specialized `_STORE_ATTR_SHAPE` and `_STORE_ATTR_SHAPE_ADD` take from a
canonical frame home (`Python/bytecodes.c:2947–2977`). Initializers write
NULL; copy/move paths preserve bits; materialization/clear sets shape
`_Py_SHAPE_NONE` (`Objects/dictobject.c:7062, 7401–7407, 7475–7476`). The
specializer declines `_Py_SHAPE_NONE` (`Python/specialize.c:905–911`). No
other in-tree assignment to `values->values` in a shaped object was found by
the source search recorded in the audit. This still requires independent
review of alternate native adapters before changing generated code.

## Producer and generated boundary

Only **after R4's shared publication contract lands** and the input
precondition plus writer audit are accepted, add one trusted source
declaration next to `_LOAD_ATTR_SHAPE`'s home read:
`IR_CANONICAL_HOME_DUP(attr, found)`. It replaces
`attr = PyValue_XDUP(found)` and means: duplicate a borrowed value already
stored in a canonical home; the resulting C-owned `attr` has the same bits
and ownership that `PyValue_XDUP` would have produced. Its debug expansion
uses a fatal `assert(_PyValue_IsHome(found))` before duplicating. The
generator's *existing* `result_publication_contract(inst, fmt)` records this
producer fact for the named result and checks the output identity, single
unconditional declaration, no later mutation/address-taking/escape, and
matching `IR_RETIRE_PUBLISHED_RESULT(attr, receiver)`. It does **not** claim
to prove the C expression that produced `found` points into a canonical
home; that premise is established by the source/writer review above and the
debug assertion. Reject a branch or ambiguous join. No handler-name list,
new pointer-provenance parser, second opcode table or general type framework
is justified for one result.

Only at the marker's **first temporary-destination flush**, use a direct
canonical store: assert destination is NULL or the same value, assign the
bits, and run `_PyValue_CheckTemp` in debug. Keep the existing
`_PyValue_Publish` for a folded local's final `IR_STORE_DST`, for all other
outputs, and for every producer without the checked declaration. Keep the
existing receiver-release, failed-local-result, error unwind, refresh and
companion-output logic unchanged. The shared `result_phase` still describes
publication-before-retirement; canonical *source* is a narrower checked
producer fact used by emission, not a new global value type or JIT recovery
snapshot.

The retained R2 GCC executable (build ID `adade788...`) has the first
type tests and conditional `_PyValue_Canonicalize` call at
`0x352df2–0x352e24`, and a separate local-store sequence at
`0x352fa9–0x352fd8`. The experiment succeeds mechanically only if the
first sequence disappears while the local/error paths remain correct.
Run focused inline int/float, boxed large number, exact-number subclass,
static immortal, mortal object, replacement finalizer, callback/reentry and
folded-local tests in debug/release. Then compare actual GCC and matched
shape-heavy plus non-shape workloads. If the boundary precondition is not
accepted, a source path violates it, or elapsed results do not justify the
extra DSL declaration, stop and retain ordinary `_PyValue_Publish`. This is
an R5 experiment with its own review and measurement, not part of R4's
phase-contract landing.
