# Close retained dictionary boundaries

- Stage: revised API-cut design accepted; implementation active
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
ledger: 50 core/startup, 37 bundled and 24 C test/example sites remain. All port
before qualification; the provider API cut alone is not runtime closure.

[Design workspace](../../../../build/pyvalue-dict-boundary-closure-20261102-v1),
[coordinator review](../../../../build/pyvalue-dict-boundary-closure-20261102-v1/coordinator-review-v1.md).
Next: implement, close the actual Dict TU with the new configured debug headers,
independently review code/results, commit and validate merged consumers. Older
prepared scripts remain historical; they are not an implementation.

[Accepted design review](../../../../build/pyvalue-dict-boundary-closure-20261102-v1/coordinator-review-v2.md).
