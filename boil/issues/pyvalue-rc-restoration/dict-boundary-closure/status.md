# Close retained dictionary boundaries

- Stage: revise design; no implementation authorized yet
- Owner: fresh state_audit; coordinator reviews
- Scope proposal: remove unsupported borrowed APIs, keep owning/value migration APIs

The coordinator rejected a new checked-borrow compatibility subsystem: it adds
query pins, entry revalidation and a new error contract while generic callers
still must port. The simpler proposed cut removes the unsupported borrowed
GetItem/Next/SetDefault families and unused borrowed-index helper, preserves the
existing owning GetItemRef/NextEntryRef/SetDefaultRef and canonical operations,
and makes required source ports explicit compiler failures. No rejection stubs,
hidden scalar owner, second dictionary format or backward-compatibility approval.

The revised design must name exact declarations/wrappers removed, simplify the
private owning default path, port necessary same-file consumers, and identify
any genuinely required internal metadata primitive. Refresh only the relevant
existing caller ledger. Core and bundled callers all port before qualification;
API removal alone is not closure. Narrow real C fixtures exercise the owning
migration path and scalar/error behavior.

[Design workspace](../../../../build/pyvalue-dict-boundary-closure-20261102-v1).
Next: review revised deletion/port design, implement, close the actual Dict TU,
independently review code/results, commit and validate merged consumers. Older
prepared scripts remain historical; they are not an implementation.
