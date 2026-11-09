# Target measurement source review, round 2

2026-11-08. Source re-review after M1; no native execution by this reviewer.

**Verdict: accept the collector for the initial six-round baseline.** M1
from [round 1](measurement-review.md) is resolved. `snapshot()` now returns
raw validation errors. The initial snapshot is persisted before a preflight
rejection writes empty commands and explicit failed status; the final snapshot
is persisted before postflight validation records failure. Timing boundaries,
native gates and the agreed estimator are unchanged.

The acceptance and provenance limits in round 1 still apply. This approval
covers source structure, not successful execution or a parity outcome. The
coordinator owns native execution and evidence admission.
