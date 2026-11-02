# Canonical type cache and mutation admission

- Stage: provider/core dependencies checkpointed; graph delta reviewed; C3/MRO design under review
- Owner: wiki_audit designs/implements finite typeobject slices; state_audit and coordinator review
- Hypothesis: canonical borrowed cache entries can remain fast and safe if
  version invalidation precedes observable mutation and admission stays closed
  through callback-capable mutation/retirement.

The pre-port object cache could not store inline class values; resize retains a
table across allocation. A late version check after duplicating a stale borrow
is unsafe. Existing installed guards also require invalidation before changed
state becomes observable.

V3 replaces the cursor walk with restarted owning child selection and gates
installed guards, cache lookup and version publication with per-interpreter
admission. Live mutations block admission through rollback/retirement. Static
teardown permanently closes lookup and Python entry for that interpreter;
shared static tags remain usable by other interpreters.

The revised design resolves the two coordinator findings: a dedicated fixed
raw write/abort path cannot call Python during terminal rejection, and the whole
general forced-version setter is removed in the later core increment. Internal
zero-tag sites have explicit descendant/teardown preconditions.

The first provider increment has a seven-path grant: types_state fields in
pycore_interp_structs.h; admission declarations in pycore_typeobject.h; canonical
entries/insertion contract in pycore_typecache.h; Python/typecache.c; the exact
typeobject lookup/admission/version-producer functions listed in the scope;
existing Modules/_testinternalcapi/typecache.c bridge; new
Lib/test/test_pyvalue_typecache.py. No other scalar bridge, object.c/abstract.c,
index or shared build is owned by this lane. Root reviews the bounded delta;
it does not stage the whole held typeobject file by default.

This increment does not admit the partial runtime. Static cache admission stays
unconnected until publication ports; mutation traversal, all four actual guards,
feedback, terminal lifecycle wiring, unsafe setter retirement and finite bundled
writers remain required. The actual cache and fixture TUs/objects pass without warnings. Full typeobject
compilation still fails: 49 errors and 39 warnings mapped to unchanged bodies.
Two host controls pass; eleven candidate methods remain unexecuted.

Checkpoint `8d5a3d6af1b` contains the six independent files. The seventh,
Objects/typeobject.c, remains held; its exact provider delta was independently
reviewed but cannot yet be committed with unrelated unresolved scalar bridges.
[Code/results review](../../../../build/pyvalue-typecache-provider-20261102-v1/coordinator-code-review.md),
[native checks](../../../../build/pyvalue-typecache-provider-20261102-v1/native-v1.json),
[commit and pending-delta receipt](../../../../build/pyvalue-typecache-provider-20261102-v1/commit.json).
The [core mutation component](core-mutation/status.md) is independently reviewed,
with six dependencies committed and its typeobject delta held. Graph repair is
also independently reviewed as a pending exact delta: eleven functions, 405 other
bodies and masked text exact, seven host controls passing, 29 candidate methods
unexecuted. Its full typeobject TU still has 47 errors/39 warnings, all in unchanged
functions/lines. C3/MRO provider design is now under independent review; construction,
guards/terminal/static/bundled and scalar providers remain required.
[Core proposal](../../../../build/pyvalue-typecache-core-scope-20261102-v1/design.md),
[bounded coordinator grant](../../../../build/pyvalue-typecache-core-scope-20261102-v1/coordinator-review.md).

[V3 design](../../../../build/pyvalue-typecache-design-20261102-v3/design.md),
[finite ledger](../../../../build/pyvalue-typecache-design-20261102-v3/finite-ledger.md),
[exact seven-path scope](../../../../build/pyvalue-typecache-design-20261102-v3/first-increment.md),
[coordinator acceptance](../../../../build/pyvalue-typecache-design-20261102-v3/coordinator-accept.md).
Earlier proposals/freezes/reviews remain historical evidence.

The fresh [composition review](../../../../build/pyvalue-source-integration-20261102-v1/shared-composition-review.md)
also identifies object-only scalar conversions in lookup_method_ex, descriptor/new
slots, type lookup/getattr, generic object getattr and abstract special-method
calling. The abstract and full object repairs are now committed; typeobject.c remains held. Their object/call bridges need
fallible conversion and owned cleanup; that work does not substitute for the
separate cache lifetime/mutation proof above.

[Graph design/review](../../../../build/pyvalue-typecache-graph-scope-20261102-v1/independent-design-review.md),
[graph code/results acceptance](../../../../build/pyvalue-typecache-graph1-20261102-v1/coordinator-code-review.md),
[graph native attribution](../../../../build/pyvalue-typecache-graph1-20261102-v1/native-classification.md),
[next finite MRO proposal](../../../../build/pyvalue-typecache-mro-scope-20261102-v1/design.md).
