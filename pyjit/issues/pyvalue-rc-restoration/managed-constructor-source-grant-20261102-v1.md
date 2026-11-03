# Managed inline initialization before CREATE

Adopt D's critical review of
build/pyvalue-rc-restoration-design-20261101-v1/managed-constructor-cut-v1.md.
Root implements; D reviews actual ordering, type ownership and tracer witnesses.
This is a separate15-minute cut, not a slot/storage source-window extension.

Conservative start 2026-11-02T01:20:56.000Z, concrete checkpoint
2026-11-02T01:25:56.000Z, ceiling 2026-11-02T01:35:56.000Z.
Record actual first edit.

Exact3paths: _PyType_AllocNoTrack in Objects/typeobject.c; existing
slot_trace_create callback/necessary includes in _testinternalcapi/slots.c;
test_pyvalue_slots.py focused additions. Install Py_TYPE for fixed/variable
objects before field initialization, retain variable nitems write, move existing
pure InitInlineValues before Init[Var]/reference CREATE. Init remains the sole
type INCREF/refcount/flag/NewReference operation; do not own/increment an
uninitialized object. No new initializer/API, Dict body or global InitVar change.

C freezes actual InitInlineValues/shared_keys_usable_size/InlineValuesSize.
They use installed type metadata, no object RC/flag, allocation or callback.
Cached-key count sum is unchanged by insertion and shrinks with instance
allocation on selected GIL profile; allocated inline capacity remains sufficient.
Tracer is nonallocating/error-preserving; check initialized capacity before
walking all values, header0/embedded/valid/rootshape/absentdict, and declared
slots. Cover ordinary managed and inherited managed-plus-slot construction.

Cheap gates: exact order/purity/ownership and source diff, Python AST and
at most2 actual syntax-only TUs30seconds each. Existing typeobject old container/
Clinic failures remain unqualified. No object/link/candidate/runtime/timing.

Formal design reviewe30f187a binds proposal0646c060. Actual first edit
01:21:32.664449323 UTC; concrete checkpointc8047215 at01:21:51 is timely.
Independent source reviewb11eaaad accepts exact3owned/3context paths and12AST
methods (2new), with actual fixture syntax passing. Type/declared/managed state
initializes beforeCREATE; sole type INCREF and variable prewrite remain.
Close this window early; broader type/native/runtime qualification remains open.
