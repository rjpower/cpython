# Canonical Python instance-slot source cut

The coordinator implements the reviewed design in
`build/pyvalue-rc-restoration-design-20261101-v1/instance-slot-cut-v1.md`.
D's independent review confirms ordinary Python-class GenericAlloc override,
core subtype constructor routes, resolved heap member offsets, tuple after-items
support and the required __class__/legacy object-member generic fallback.
value_api_consumer_map independently reviews implementation and emitted output.

Use conservative start2026-11-01 23:53:24 UTC, checkpoint2026-11-02 00:13:24,
ceiling00:53:24. Record actual first source edit. The set window closed early;
this distinct60-minute cut does not extend it.

Named scope: type_new_descriptors, _PyType_AllocNoTrack plus its pure value-field
initializer, traverse_slots, clear_slots and their status/error callers in
Objects/typeobject.c; missing VALUE_EX suppress branch in Objects/object.c;
_PyMember_SetValue cleanup status in Python/structmember.c; exact descriptor
classification/LOAD/STORE slot cases in Python/specialize.c; _LOAD_ATTR_SLOT,
_STORE_ATTR_SLOT and their exact macro composition in Python/bytecodes.c.
Add focused Modules/_testinternalcapi/slots.c and test_pyvalue_slots.py, with
only matching Setup.stdlib.in/parts.h/_testinternalcapi.c registration hunks.
Necessary internal declarations and includes belong to this scope.

Python-created slots always use canonical VALUE_EX and NULL1 initialization.
Legacy object members, __class__ type-pointer reads and VALUE empty-as-None
descriptors use their generic semantic path in this cut. Keep readonly/audit/
type-version/offset/after-items guards. Traverse pointer values only; detach
before release, preserve primary and first-new errors, propagate checked failure
after committed stores without rolling them back.

Allocation/traversal/member/specializer changes can proceed in parallel with A.
The specialized STORE producer must be reviewed as an actual emitted handler:
publish the field and consumed inputs before old-field final release; drain at
that original release point before owner POP; cover owner POP's own last-release
error/reentry too. No raw field/frame/IR pointer survives callback without fresh
validation. Agree the shared primitive with A before producer changes. A owns
generator changes/regeneration and serializes final products after root's stable
bytecode boundary; do not race regenerators or hand-edit generated products.

Tests inspect actual field bits and cover empty/delete/None/scalars/heap values,
inheritance/variable tuples, generic fallbacks, finalizer publication and cycles.
Audit native bypass allocators explicitly as extension boundaries. Cheap gates:
AST/source/actual-emission review, affected host tests, at most four actual
syntax-only TUs30seconds each. No object/link/runtime/performance claim from
this cut. Full runtime qualification remains required after dependent closure.
