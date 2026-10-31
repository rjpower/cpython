# Reuse exact constant operands

- State: closed
- Outcome: reported negative result; runtime increment unmerged
- Owner: qualification_review_resumed; coordinator owns decision
- Updated: 2026-10-31
- Source base: integrated `d98291018c9665ae3db6952da0b0a40a4e04992b`
- Runtime candidate: `7f0b1819f84ee11b354d60394a4975e1fcb0a188`
- Final test-only candidate: `a84965eaa90267ac1dbf1b4c97ab032aaf26cedd`
- Branch/worktree: `boil/interpreter-constant-operands`; `build/worktrees/constant-operands`
- Decision: reject thirteen operand-form handlers on measured elapsed regressions

## Hypothesis

A same-word exact small-integer operand can remove constant-load dispatch,
frame traffic and RHS admission while preserving generic/in-place fallback,
specialization, debugger entry and observers through shared DSL semantics.

## Result

The fixed [elapsed comparison](performance-results.md) rejected this increment.
Coroutine medians regressed 4.47% and 11.64% in its two rounds; both controls
also regressed. Independent [outcome review](reviews/quiet-outcome-20261031-v1.md)
verified all 80 raw elapsed durations, eight actual workload/build origins,
48 accepted quiet checks, unchanged products and terminal idle cleanup.
The five timed calls per scored process are serial, not independent randomized
pairs. No extra samples, qualification or landing follow this result.

The earlier 5% evaluator-text veto was withdrawn before timing through the
[elapsed-first revision](performance-revision-20261031.md). Matched text grew
18.4248%; it remains diagnostic and is not the rejection rule. The reviewed
[counter-permission amendment](performance-permission-amendment.md) omitted
unavailable instruction counters before scores without replacement samples
or system changes. Original plans, reviews and rejection receipts remain history.

## Evidence and limits

Bounded debug/release semantic selections passed through explicit test-only
bridges. The [execution record](feedback-results.md) retains original assertion
failures, corrections, interrupted products and separate release/PIE inspection
scopes. These selections were feedback, not broad landing qualification.
The [native review](reviews/emission-work-removal-outcome-20261031-v1.md) confirms
removed producer/frame/RHS work while naming remaining guards and fallbacks.
That local work removal did not establish an elapsed benefit.

Subsequent disassembly shows extra instruction-pointer/frame reloads and
next-pointer stores in unchanged common handlers. The coordinator reproduced
all twelve bounded captures exactly. This is concrete emitted work, not proof
of its share of elapsed cost, executed handler frequencies or a cache cause.

## Next

Preserve this rejected source, products and evidence. The separate
[common evaluator-state investigation](../interpreter-dispatch-state/status.md)
uses actual accepted-baseline return/yield cache transport as a new question.
It grants no retry or successor implementation for this closed increment.
