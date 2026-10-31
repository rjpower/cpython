# Adopt the nursery as the runtime architecture

- State: active
- Owner: restart_audit implements; coroutine_design_review independently reviews; coordinator integrates
- Updated: 2026-10-31
- Base: merged `d98291018c9665ae3db6952da0b0a40a4e04992b`
- Budget: design accepted; eight elapsed hours with 90-minute source and first-feedback checkpoints
- Depends on: [post-merge nursery/shape checks](../nursery-shape-landing/status.md)
- Decision: keep the functioning nursery; remove global nursery/no-nursery optionality
- Branch/worktree: `boil/gc-nursery-adoption`; `build/worktrees/nursery-adoption`
- Source started: 2026-10-31 20:18:11 UTC; source checkpoint due 21:48:11 UTC
- Overall dispatch deadline: 2026-11-01 04:18:11 UTC

## Hypothesis

Mandatory age admission for the existing exact cell/tuple cohort, real minor
collection and age-aware checking can replace the temporary experiment switch
without a parallel legacy collector mode. One automatic minor halfway through
the existing major allocation budget may provide useful early reclamation;
its additional all-old scan must earn its cost separately.

## Experiment

Follow the [design](design.md): remove the runtime enable field and test setter,
migrate all live consumers/tests, support actual minor execution under retain,
KILL and stress, and keep major debt independent of minor requests. Compare
the automatic cadence using temporary source variants, with actual collection
and age witnesses, fixed major budgets, matched instructions/elapsed and memory.

## Decision rule

The nursery is adopted. Reject an incorrect implementation or expensive
automatic cadence and improve it; do not restore a nursery disable option.
The halfway cadence is a hypothesis rather than an accepted policy. Adoption
with automatic majors and explicit minors is a coherent scheduling baseline
for that comparison, not a second collector mode.

## Now / next

The [independent design review](reviews/nursery-adoption-design-20261031.md)
cleared design `046b8ab0…` for the first source increment. Implementation is
running in the isolated worktree from documentation checkpoint `32f14069fd2`,
whose runtime source is d98291018c9. No target interpreter, build or native
suite is authorized before source review and a coordinated feedback plan.
The first source checkpoint covers mandatory admission, manual minor semantics
and checker support with existing automatic-major scheduling. Only after that
correctness checkpoint does the separately reviewed cadence increment begin.

The earlier [coroutine proposal](../gc-nursery-coverage/design.md) is retained
as history and needs rebasing after adoption. Its global-knob and
automatic-major-only assumptions no longer authorize implementation or a
permanent runtime switch. Broader allocation coverage follows the adopted
minor/major/checker contract and its measured scheduling decision.
