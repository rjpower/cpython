# Qualify the nursery and shape simplification together

- State: closed
- Outcome: landed
- Owner: coordinator; qualification_review_resumed independently reviews
- Updated: 2026-10-31
- Budget: original 90-minute attempt expired; remaining-only batch under [restart plan](resume-plan.md)
- Depends on: [nursery](../gc-nursery-minimum/status.md),
  [shape admission](../shape-admission-reuse/status.md)
- Branch/worktree: boil/nursery-shape-qualification;
  ../boil-nursery-shape-qualification
- Head: e9e10d5b503eaf219797a5db44766eeab6fad4dc
- Integration: d98291018c9665ae3db6952da0b0a40a4e04992b
- Phase: qualification, merge bridge, post-merge smokes and independent closure accepted
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

The exact source is integrated at d98291018c9, qualified on e9e10d5b503 products.
All required builds, behavior/backend controls, full-three, stress-two, ASan16
and closure passed. Independent audit accepted the 334-artifact qualification
capsule before its deadline. The actual merge bridge proves 34 prose-only
differences and 6,903 otherwise identical mode/type/blob entries.

Release/debug post-merge smokes each passed 1,937 tests with exact skips,
zero unwind fallbacks and verified cleanup. Independent
[landing closure](reviews/final-smoke-20261031.md) accepted their complete
evidence at 20:07:12 UTC. See [results](results.md) for identities and limits.
No gate remains for this source and no speedup is claimed.

Recovery also restored all five pre-existing user files to their exact hashes
and original unstaged state. Frozen evidence remains intact. The temporary
nursery switch belongs to this historical increment; the next
[adoption change](../gc-nursery-adoption/status.md) removes whole-runtime
optionality and supports actual minors under checking/stress. The nursery
architecture is retained and will be improved.
