# Qualify the nursery and shape simplification together

- State: active
- Owner: coordinator; qualification_review_resumed independently reviews
- Updated: 2026-10-31
- Budget: original 90-minute attempt expired; remaining-only batch under [restart plan](resume-plan.md)
- Depends on: [nursery](../gc-nursery-minimum/status.md),
  [shape admission](../shape-admission-reuse/status.md)
- Branch/worktree: boil/nursery-shape-qualification;
  ../boil-nursery-shape-qualification
- Head: e9e10d5b503eaf219797a5db44766eeab6fad4dc
- Phase: qualified source integrated; actual merge bridge and post-merge smokes next
- Execution: original attempt started 16:06:20 UTC and expired 17:36:20 UTC on 2026-10-31

## Hypothesis

The independently reviewed opt-in nursery and shape-family simplification can
preserve the supported runtime contracts together, without further source
changes or separate full qualification campaigns.

## Experiment

Combine only GC candidate 3a473612157 and interpreter candidate c46f1bef67f on
the current documentation head. Prove the source union, freeze products and run
one applicable [validation batch](../../design/validation.md). Carry each
candidate's source/feedback evidence under its actual identity; refresh native
coverage for the combined source. Include actual opt-in minor execution in
supported modes. Keep the nursery disabled by default.

## Decision rule

Integrate after the applicable combined-source checks and independent review
pass. Stop on an unexpected failure or the budget limit, retain the result and
reject this landing attempt pending a separately bounded correction or remaining
qualification batch. No performance claim or automatic repeat-to-green.

## Now / next

The thirteen-file union passed independent source review, four fresh builds,
early/backend controls, full suites in release/debug OFF/debug KILL, both
stress1000 selections and ASan16. All terminal rows and cleanup receipts passed with zero
unwind fallbacks; actual counts, skips and report-only diagnostics are in
[results](results.md).

Closure passed, including C extensions, unchanged regeneration, traversal and
nineteen declared syntax receipts. The batch finished with cleanup at 19:52:27
UTC. The 334-artifact capsule passed independent audit and six validator replays.
[Final acceptance](reviews/final-qualification-20261031.md) was recorded at
19:55:44 UTC, before the deadline.
The remaining-only batch runs from 19:14:30 to
20:14:30 UTC under the [restart plan](resume-plan.md). The original expired
attempt remains incomplete; its completed results stand through reviewed carry.

Primary checkout recovery is complete, including all five pre-existing user
files with exact preserved hashes. User work retains its original unstaged
state; see the [recovery follow-up](reviews/restart-user-note-recovery.md).
The separate frozen union is unchanged. Source is integrated with runtime
contracts updated in the same merge. Integration preparation remains held until
the actual merge identity and exact prose-only tree bridge are bound, then one
release/debug smoke pair and independent outcome review close the landing.
No new speed claim is made.
