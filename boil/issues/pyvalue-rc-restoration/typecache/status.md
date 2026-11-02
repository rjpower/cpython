# Canonical type cache and mutation admission

- Stage: v3 reviewed; revise two finite details before source grant
- Owner: wiki_audit revises design; coordinator reviews before source
- Hypothesis: canonical borrowed cache entries can remain fast and safe if
  version invalidation precedes observable mutation and admission stays closed
  through callback-capable mutation/retirement.

The current object cache cannot store inline class values; resize retains a
table across allocation. A late version check after duplicating a stale borrow
is unsafe. Existing installed guards also require invalidation before changed
state becomes observable.

V3 replaces the cursor walk with restarted owning child selection and gates
installed guards, cache lookup and version publication with per-interpreter
admission. Live mutations block admission through rollback/retirement. Static
teardown permanently closes lookup and Python entry for that interpreter;
shared static tags remain usable by other interpreters.

The coordinator accepted that direction and terminal policy subject to two
corrections: the rejection must use a dedicated raw write/abort primitive
(ordinary Py_FatalError may call Python), and the general forced-version setter
must be removed completely. Internal zero-tag sites must preserve descendant
invalidation or be terminal/refcount-zero teardown. Wiki_audit revises those
finite details and proposes the first bounded implementation scope. No source
grant, completed mutation proof or runtime pass exists.

[V3 design](../../../../build/pyvalue-typecache-design-20261102-v3/design.md),
[finite ledger](../../../../build/pyvalue-typecache-design-20261102-v3/finite-ledger.md),
[coordinator review](../../../../build/pyvalue-typecache-design-20261102-v3/coordinator-review.md).
V2 and its reviews remain historical evidence in their original directories.

The fresh [composition review](../../../../build/pyvalue-source-integration-20261102-v1/shared-composition-review.md)
also identifies object-only scalar conversions in lookup_method_ex, descriptor/new
slots, type lookup/getattr, generic object getattr and abstract special-method
calling. Three full source files remain held. Their object/call bridges need
fallible conversion and owned cleanup; that work does not substitute for the
separate cache lifetime/mutation proof above.

Fresh review workspace: ../../../../build/pyvalue-typecache-design-20261102-v3.
No source edits or new cache implementation have been authorized.
