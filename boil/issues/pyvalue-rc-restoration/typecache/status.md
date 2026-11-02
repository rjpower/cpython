# Canonical type cache and mutation admission

- Stage: revise design; no implementation assigned
- Owner: coordinator; independent review required before dependent source
- Hypothesis: canonical borrowed cache entries can remain fast and safe if
  version invalidation precedes observable mutation and admission stays closed
  through callback-capable mutation/retirement.

The current object cache cannot store inline class values; resize retains a
table across allocation. A late version check after duplicating a stale borrow
is unsafe. Existing installed guards also require invalidation before changed
state becomes observable.

The v2 proposal runs original Modified before writes under nested mutation
admission. Remaining proof covers direct annotation getters/setters, custom-MRO
failure/reentry, bases rollback, descendant guards, direct version setters and
static initialization/teardown with shared nonzero tags. This is a correctness
dependency, not a compatibility permission gate.

References: [v2 design](../../../../build/pyvalue-rc-restoration-design-20261101-v1/typecache-canonical-cut-v2.md),
[mutation ledger](../../../../build/pyvalue-rc-restoration-design-20261101-v1/typecache-live-mutation-ledger-v1.md),
[review](../../../../build/pyvalue-rc-native-consumer-map-20261101-v1/typecache-v2-design-review.md).
Next: close the finite proof or revise the design, then implement and test real
warm guards, mutation callbacks and cache growth in a scoped increment.
