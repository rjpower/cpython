# Preserve common evaluator state

- State: closed
- Outcome: abandoned
- Owner: qualification_review_resumed; independent review by coroutine_design_review
- Updated: 2026-11-01
- Branch/worktree: `boil/interpreter-slot-base`; `build/worktrees/dispatch-slot-base`
- Candidate: `3ddc437679ebbb9f8629c1670655c58776075dfc`
- Decision: reject on the predeclared elapsed primary; do not integrate

## Hypothesis and outcome

Handler-local slot-base derivation removed the targeted baseline return/yield
cache transport. Source and native correctness feedback passed their bounded
scopes. Native inspection also found added common frame/IP stack traffic.
The subsequent fixed elapsed comparison regressed coroutines 11.98% and 11.00%,
generators 26.13% and 26.71%, with both other controls slower in both rounds.
No text-size veto or causal cache attribution decides this result.

The [performance result](performance-results.md) and
[independent outcome review](reviews/quiet-outcome-20261101-v1.md) retain the
actual source/image identities, raw-vector replay and quiet/cleanup evidence.
Root confirmed session 30844 terminal exit 0. No extra samples or qualification
are authorized under this closed attempt.

## Next

The [candidate architecture](../../design/vm-design-candidate.md) proposes
matched compiler/backend controls followed by one explicit frame-argument
increment in the existing generated tail backend. It has separate review and
execution boundaries. This source candidate and its evidence remain preserved.

Earlier [research](results.md), [source design](design.md),
[native feedback](native-feedback-results.md), [native analysis](native-analysis.md)
and [predeclared elapsed plan](performance-plan.md) retain their original scopes.
