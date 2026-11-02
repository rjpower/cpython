# Native parsers, registration and opaque API

- Stage: active design; no implementation authorized yet
- Owner: fresh wiki_audit designs; coordinator reviews
- Goal: make real extension routes use the accepted native owner scope, then
  support gradual opaque API adoption without exposing PyValue bits.

Fixed/variable engine and selected callers are source-reviewed. Recursive and
kwargs parser contexts, typed Clinic registration, remaining call families,
converter/buffer cleanup and the HPy-compatible backend remain unfinished.

Start with one actual Python→gateway→typed worker→caller publication route.
Scope producers, registration and generated wrappers together; a synthetic C
worker call or parser-local arena does not prove the route. Keep owned argument
views through implementation and cleanup; stack-local converter resources end
at their actual wrapper lifetime. Never cast a function pointer to a new ABI.

The [extension design](../../../design/extension-api.md) owns the public direction.
HPy ecosystem conformance does not gate the first internal interpreter checkpoint.
Next: review one cohesive parser/registration increment with real-route tests.

## Current finite design

The first full build found repeated Clinic tuple-to-object-vector mismatches
and remaining getargs container assumptions. Inspection also found keyword
GetItemRef results stored in parser output and decremented before callee use.
An explicit wrapper-owned argument lease must retain positional and selected
keyword object views through implementation and converter cleanup. A parser-local
temporary cannot supply that lifetime.

Design the first no-FASTCALL tuple/kwargs Clinic family with its producer,
generated wrappers and actual _testclinic route. Preserve old object-vector
contracts explicitly; no PyValue-to-PyObject-pointer casts or second boxed tuple
format. [Design workspace](../../../../build/pyvalue-clinic-parser-design-20261102-v1).
Next: coordinator reviews concrete scope and ownership before implementation.
