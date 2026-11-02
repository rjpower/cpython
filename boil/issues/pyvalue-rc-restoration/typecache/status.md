# Canonical type cache and mutation admission

- Stage: fresh finite design-proof review; no source grant
- Owner: wiki_audit closes proof; coordinator independently reviews before source
- Hypothesis: canonical borrowed cache entries can remain fast and safe if
  version invalidation precedes observable mutation and admission stays closed
  through callback-capable mutation/retirement.

The current object cache cannot store inline class values; resize retains a
table across allocation. A late version check after duplicating a stale borrow
is unsafe. Existing installed guards also require invalidation before changed
state becomes observable.

The v2 proposal runs original Modified before writes under nested mutation
admission. Fresh review found that its borrowed subclass cursor can skip an
installed sibling guard after reentrant dictionary mutation. The original walk
cannot be preserved unchanged. A replacement walk is in design; no implementation
has been admitted. Remaining proof covers direct annotation getters/setters, custom-MRO
failure/reentry, bases rollback, descendant guards, direct version setters and
static initialization/teardown with shared nonzero tags. This is a correctness
dependency, not a compatibility permission gate.

References: [v2 design](../../../../build/pyvalue-rc-restoration-design-20261101-v1/typecache-canonical-cut-v2.md),
[mutation ledger](../../../../build/pyvalue-rc-restoration-design-20261101-v1/typecache-live-mutation-ledger-v1.md),
[review](../../../../build/pyvalue-rc-native-consumer-map-20261101-v1/typecache-v2-design-review.md).
Next: close the finite proof or revise the design, then implement and test real
warm guards, mutation callbacks and cache growth in a scoped increment.

The fresh [composition review](../../../../build/pyvalue-source-integration-20261102-v1/shared-composition-review.md)
also identifies object-only scalar conversions in lookup_method_ex, descriptor/new
slots, type lookup/getattr, generic object getattr and abstract special-method
calling. Three full source files remain held. Their object/call bridges need
fallible conversion and owned cleanup; that work does not substitute for the
separate cache lifetime/mutation proof above.

Fresh review workspace: ../../../../build/pyvalue-typecache-design-20261102-v3.
No source edits or new cache implementation have been authorized.
