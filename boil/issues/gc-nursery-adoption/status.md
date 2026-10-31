# Adopt the nursery as the runtime architecture

- State: review
- Owner: restart_audit designs; coordinator assigns implementation and independent review
- Updated: 2026-10-31
- Base: merged `d98291018c9665ae3db6952da0b0a40a4e04992b`
- Budget: 45-minute design handoff; proposed implementation dispatch is eight elapsed hours with 90-minute source/review and first-feedback checkpoints
- Depends on: [post-merge nursery/shape checks](../nursery-shape-landing/status.md)
- Decision: keep the functioning nursery; remove global nursery/no-nursery optionality

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

Design prepared from the actual merged source. No runtime edits, native jobs,
commits or HEAD changes belong to this design task. Independent design review
precedes source implementation; integration smokes keep their current inputs.
The first source checkpoint covers mandatory admission, manual minor semantics
and checker support with existing automatic-major scheduling. Only after that
correctness checkpoint does the separately reviewed cadence increment begin.

The earlier [coroutine proposal](../gc-nursery-coverage/design.md) is retained
as history and needs rebasing after adoption. Its global-knob and
automatic-major-only assumptions no longer authorize implementation or a
permanent runtime switch. Broader allocation coverage follows the adopted
minor/major/checker contract and its measured scheduling decision.
