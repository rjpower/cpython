# Adopt the nursery as the runtime architecture

- State: review
- Owner: restart_audit implements; coroutine_design_review independently reviews; coordinator integrates
- Updated: 2026-10-31
- Base: merged `d98291018c9665ae3db6952da0b0a40a4e04992b`
- Budget: design accepted; eight elapsed hours with 90-minute source and first-feedback checkpoints
- Depends on: [post-merge nursery/shape checks](../nursery-shape-landing/status.md)
- Decision: keep the functioning nursery; remove global nursery/no-nursery optionality
- Branch/worktree: `boil/gc-nursery-adoption`; `build/worktrees/nursery-adoption`
- Source started: 2026-10-31 20:18:11 UTC; source checkpoint due 21:48:11 UTC
- Overall dispatch deadline: 2026-11-01 04:18:11 UTC
- Candidate: `b34931a881b33b47a2b2f4b1b2a644c50fed963b`, frozen 20:48:24 UTC
- Further source-review/debug checkpoint: 2026-10-31 22:18:24 UTC

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
frozen in the isolated worktree from documentation checkpoint `32f14069fd2`,
whose runtime source is d98291018c9. The candidate adds mandatory admission,
actual checked minors, transactional diagnostic records, storage-lifetime
retirement and failed-collection debt/backoff. Host checks pass; twenty
focused tests are prepared. [Exact-source review](reviews/source-20261031-v1.md)
accepts b34931a881b for bounded native feedback. Concrete helpers, launch
bindings and the [held feedback plan](feedback-plan.md) received coordinator
acceptance. The first launch stopped before configuration on inconsistent
assembler path spellings in the readiness/build tool records; no build or
target test ran. [Failure evidence](feedback-results.md) is preserved. The
[independent correction review](reviews/feedback-helper-correction-20261031-v2.md)
accepted the new held helpers. Root activated their remaining-only execution
at 21:16:41 UTC under the original 22:07:46.664152 UTC deadline. Tool identity
passed and actual configuration began; build/behavior results remain pending.
The first source checkpoint covers mandatory admission, manual minor semantics
and checker support with existing automatic-major scheduling. Only after that
correctness checkpoint does the separately reviewed cadence increment begin.

The earlier [coroutine proposal](../gc-nursery-coverage/design.md) is retained
as history and needs rebasing after adoption. Its global-knob and
automatic-major-only assumptions no longer authorize implementation or a
permanent runtime switch. Broader allocation coverage follows the adopted
minor/major/checker contract and its measured scheduling decision.
