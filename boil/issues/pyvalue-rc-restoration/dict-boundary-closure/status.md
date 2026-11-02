# Close retained dictionary boundaries

- Stage: independent code/result review accepted; committed as `18b6053c75f3dc4e29ee53f4aba89b0680b4c97e`
- Owner: fresh state_audit; coordinator reviews
- Scope: remove ten unsupported borrowed entrypoints and three OrderedDict aliases; keep owning/value migration APIs

The coordinator rejected a new checked-borrow compatibility subsystem: it adds
query pins, entry revalidation and a new error contract while generic callers
still must port. The simpler proposed cut removes the unsupported borrowed
GetItem/Next/SetDefault families and unused borrowed-index helper, preserves the
existing owning GetItemRef/NextEntryRef/SetDefaultRef and canonical operations,
and makes required source ports explicit compiler failures. No rejection stubs,
hidden scalar owner, second dictionary format or backward-compatibility approval.

The accepted revision names exact declarations/wrappers, simplifies the private
owning default path, ports the pure key predicate, and keeps the actually used
index-only specialization primitive. Stable-ABI manifest/products change through
their producer; owning migration fixtures exercise real retained APIs. Targeted
ledger at the API cut: 50 core/startup, 37 bundled and 24 C test/example sites.
Later increments reduce that inventory; these are not current remaining counts.
All required consumers port before qualification; the provider API cut alone is
not runtime closure.

[Design workspace](../../../../build/pyvalue-dict-boundary-closure-20261102-v1),
[coordinator review](../../../../build/pyvalue-dict-boundary-closure-20261102-v1/coordinator-review-v1.md).
The exact twelve-file cut passed independent review. Full Dict and C fixture TUs
pass with the new configuration; actual dictobject.o compilation passes. All
295 other Dict functions and 58 earlier fixture functions are byte-exact.
Stable-ABI generation is idempotent and removes exactly four exports in each of
three products. Twelve focused tests parse; ten fork-specific methods remain
unexecuted, with host controls limited to ordinary supported owning semantics.

[Accepted design review](../../../../build/pyvalue-dict-boundary-closure-20261102-v1/coordinator-review-v2.md),
[independent code/result review](../../../../build/pyvalue-dict-boundary-closure-20261102-v1/independent-review.md),
[results](../../../../build/pyvalue-dict-boundary-closure-20261102-v1/results.json),
[commit](../../../../build/pyvalue-dict-boundary-closure-20261102-v1/commit.json).
Four active canonical read providers are now committed as `99012f782ea`;
PopValue remains a missing mutation provider. Next: port remaining core/bundled/
test consumers, then validate the merged runtime. Source commit is not
runtime or ABI admission. Older prepared scripts remain historical.
