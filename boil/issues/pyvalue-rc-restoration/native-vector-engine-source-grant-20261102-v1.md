# Variable native scope and real flat fixture

Adopt D's critical review of proposal326664ea at
build/pyvalue-native-vector-parser-design-20261102-v1/proposal.md.
C implements the engine; D reviews. This is a distinct60-minute source cut:
conservative start2026-11-02T02:03:00Z, concrete checkpoint02:23:00Z,
ceiling03:03:00Z. Record actual first edit. No previous window is extended.

Exact scope: pycore_native_scope.h/native_scope.c same-engine variable storage,
_PyCFunction_CallScoped and directly coupled typed flag dispatch in
Objects/methodobject.c; existing internal native fixture, its Clinic input and
generated product, and focused test_pyvalue_native_scope.py. No parser or new
registration ABI implementation. Header must freeze before the paired VM grant.
The generic VM caller is deliberately scheduled AFTER A's attribute source
freeze; engine completion alone does not close the real route.

Pin metadata before allocation; require a stable already-owning evaluated
input window, acquire independent snapshots, never reuse it after capture.
Check count/size overflow, keyword counts, actual vectorcall offset prefix and
drainable initialized partial state. Revalidate current callable flags/target
and defining metadata after any allocation/reentry, before invocation. Typed
NOARGS/O/FASTCALL/keyword/METH_METHOD dispatch preserves real arity/recursion.

Stage pure PrepareOwned; caller publishes actual CALL temporary before cleanup.
Pure TakeRetiredResult moves the box owner so generic cleanup retains original
views/snapshots→kwnames→reverse arguments→callable→result-box order. Existing
fixed O semantics stay exact. Abort/prep failure and completed body/result
failure remain distinct; never replay a body. Every owner is detached before
release, resources/storage stay live during callbacks, and primary error stays
intact. Deallocators preserve entry exceptions. No implicit TLS owner engine,
scalar cache, object/value array casts or second representation.

Generate the actual flat keyword fixture using Clinic; include a true NOARGS
fixture, focused legal lifetime/OOM/body/error tests and exact scoped entry
contract. Cheap source/AST checks then at most3 meaningful actual syntax
TUs30seconds each, not synthetic declarations. No objects/link/runtime/timing.
Recursive parser/context-aware registration remain separately reviewed work.
