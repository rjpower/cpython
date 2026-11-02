# Native parsers, registration and opaque API

- Stage: revise cohesive lifetime design; no implementation authorized yet
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

## Current design review

The first full build found repeated Clinic tuple-to-object-vector mismatches
and remaining getargs container assumptions. Inspection also found keyword
GetItemRef results stored in parser output and decremented before callee use.
An owning lease must keep positional and selected keyword views alive through
their last use; a parser-local temporary cannot provide that lifetime.

Coordinator review requested an end-to-end lifetime decision before code.
Distinguish invocation input/result retirement from ordinary callee-local parser
resources. Prove the caller frame and argument window remain valid through
callee/cleanup reentry; revise the blanket deferred-parser-resource rule if the
proof supports it. Otherwise specify real typed caller-owned storage and
registration through publication before implementation. Choose one lifetime
model. Do not first build a wrapper-exit policy already intended for replacement.

The first useful slice should include a structural production wrapper family
contributing to the real build failures, with test siblings as witnesses. Real
ordinary/EX dispatch, publication and cleanup dependencies must be explicit.
No PyValue-to-object-vector casts, hidden TLS scope, fake publication or second
boxed tuple format. Existing native scope/input/result contracts remain in force
until a reviewed amendment replaces them.

[Design workspace](../../../../build/pyvalue-clinic-parser-design-20261102-v1),
[coordinator review](../../../../build/pyvalue-clinic-parser-design-20261102-v1/coordinator-review-v1.md).
Next: review the revised cohesive model and first production slice, then authorize
bounded implementation and independent code/result review.
