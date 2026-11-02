# Native parsers, registration and opaque API

- Stage: queued design; no active source owner
- Owner: coordinator assigns a fresh implementation owner and reviewer
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
