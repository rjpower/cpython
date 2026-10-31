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
- Phase: debug full OFF/N0 passed; cleanup verification precedes debug KILL
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

All four builds, early controls and backend controls completed before the
restart. Release full has a terminal pass: 51,261 tests, zero unwind fallbacks
and one report-only detached-snapshot visit. Current frozen-input and idle-process
checks passed; its missing cleanup receipt was completed with a separately
timestamped observation at 19:14:30 UTC. Debug full OFF/N0 also passed: 51,261
tests, 3,744 skips, 60,750 collections, zero fallbacks and four report-only
detached-snapshot visits. Debug KILL, stress, ASan16 and closure remain.
The expired attempt is incomplete,
not a qualification pass or a rejection of the runtime hypothesis.

The primary checkout lost its linked-worktree metadata and 4,921 tracked files.
Missing-only recovery restored the branch at a01a9b953e2 while preserving all
surviving files and build evidence. One pre-existing user edit was recovered
exactly from a loose Git blob. Four untracked legacy notes remain missing after
the bounded search; see the [recovery audit](reviews/restart-recovery-audit.md).
The separate frozen union remains the qualification source.

Independent carry and launcher review passed. The remaining-only batch started
19:14:30 UTC and ends 20:14:30 UTC. Debug full OFF/N0 returned at 19:24:28 UTC;
its cleanup verification precedes debug KILL, stress-two, ASan16 and closure
under the [restart plan](resume-plan.md).
See [results](results.md). There is no runtime landing or new speed claim.
