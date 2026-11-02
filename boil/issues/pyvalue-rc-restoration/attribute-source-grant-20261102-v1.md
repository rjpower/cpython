# Attribute nomination and canonical execution

Adopt independently reviewed design amendment1e059e77 and clarification674355e3
in build/pyvalue-reader-consumer-design-20261102-v1. A implements; D reviews.
The first reader grant is closed; this is a distinct90-minute source cut.

Conservative start2026-11-02T02:02:00Z, concrete checkpoint02:32:00Z,
ceiling03:32:00Z. Record actual first edit. Checkpoint must include real
nonescaping nomination, generic STORE and method publication emission.

Exact source ownership follows the design ledger: specialize.c attribute
nomination call tree only; bytecodes.c generic LOAD/STORE plus three managed/
shape stores and their actual adaptive boundaries; corresponding ceval helpers,
declarations and exact generated products. Preserve the accepted Binary/O/
local/slot/readers. Five ID/format/target products remain exact; required
derived effects must be reviewed rather than suppressed.

Root hands off typeobject.c find_name_in_mro/assign_version_tag proven-type
tuple accessor hunks, the two new uncached value lookup providers and their
preflight, plus exactly _Py_type_getattro_value for class/metaclass inline
attributes. No general type cache or allocator rewrite. Handoff object.c
_PyObject_GetMethodValue, exact descriptor kind guards/uncached lookups in
GenericGetAttrWithDict/GenericSetAttrWithDict, preserving C's frozen managed
provider/error hunks. Necessary pycore_typeobject/object declarations, one
layout-preserving named Temps struct/forward declaration, and pure existing
shape transition lookup in pycore_dict.h are included. Dict C bodies remain
frozen; ask root for an exact extra handoff if required.

Nomination preflights the full ready MRO and native dictionary lookup path;
no allocation/cache insertion/scalar boxing after feedback pointer capture.
Retain ordinary nomination and existing history capacity; decline only unsafe
variants. Existing-only shape transitions fall back to INSTANCE_VALUE/NONE.
Generic semantic lookup may callback: own prior type/metadata snapshots and
preserve historical suppressed-MRO-error policy. Guard class/meta inline values
before object descriptor access. No Python hook effect may be retried.

Capture PC/slots and evaluated owners before preparation. Prep failure restores
temporary inputs before real unwind; no failed-output flush clobbers them.
Completed generic STORE retires OWNER then VALUE; fast stores oldfield then
receiver. Method helper gets an independent self snapshot and explicit boxed
view/caller Temps; original receiver remains live until both method/self outputs
publish. Drain only at published boundaries, branch on error before decoding,
and refresh even with Temps0. Folded locals preserve existing visibility.

Focused legal fixtures, host checks/generation and at most3 meaningful changed
syntax TUs30seconds each. No object/link/runtime/benchmark/ABI removal or new
representation mode. General type-cache and remaining native APIs stay required.
