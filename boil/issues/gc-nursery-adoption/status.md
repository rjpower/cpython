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
- Initial candidate: `b34931a881b33b47a2b2f4b1b2a644c50fed963b`, frozen 20:48:24 UTC
- Current test/fixture correction: `856222c62458bf3648e51d0bb0d278b1fbf54bd7`, frozen 21:42:14 UTC
- Current test-only candidate: `ef0af95904b87d888eee4a98b6a745835ff95085`, frozen 22:03:21 UTC
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
at 21:16:41 UTC under the original 22:07:46.664152 UTC deadline. The debug build
passed; the first selection ran twenty methods with two failures involving
private-tuple diagnostic records. Execution stopped and cleanup passed.
Exact-body observations witnessed retirement of both original target records
while preserving unrelated records; the observation's changed resize failure
location limits its causal claim about the first run. Independent design review
accepted per-target oracles and an isolated exact-last-entry metadata fixture.
The source correction froze before 21:49, passed independent exact review and
started fresh debug feedback at 21:47 UTC, session 90952. All seven rows
completed at 21:53:34 UTC; root confirmed terminal exit zero. The focused thirty
methods/51 children and both remote-stat cases passed; whole-GC OFF and KILL
each returned 204 tests with 21 expected skips. The independent
[outcome review](reviews/retirement-feedback-outcome-20261031-v1.md) accepts
this exact first feedback, including normal-child root coverage and idle
cleanup. Every row has zero fallback; each whole suite has one worker unpinned
event consistent with its passing deliberate counter test. Old products remain
sealed. Required temporary force-major and minor-debt-reset controls are next;
qualification, cadence selection and landing remain pending.
Source review found that the old final allocation would trigger a major even
after an incorrect minor debt reset. The [reviewed oracle correction](reviews/finite-major-oracle-20261031-v1.md)
now interleaves bounded allocations and manual minors under automatic GC, with
a finite actual-budget witness. Its test-only source is frozen; positive
feedback and temporary negative-source manifests are frozen. Independent
[preparation review](reviews/causal-controls-preparation-20261031-v1.md) accepts
the exact six-row [causal plan](causal-plan.md), source isolation, sealed-product
bridge and actual expected-failure witnesses. No native run of that correction
is credited yet. The new 45-minute window remains held for separate activation
after IR release completion.
The first source checkpoint covers mandatory admission, manual minor semantics
and checker support with existing automatic-major scheduling. Only after that
correctness checkpoint does the separately reviewed cadence increment begin.

The earlier [coroutine proposal](../gc-nursery-coverage/design.md) is retained
as history and needs rebasing after adoption. Its global-knob and
automatic-major-only assumptions no longer authorize implementation or a
permanent runtime switch. Broader allocation coverage follows the adopted
minor/major/checker contract and its measured scheduling decision.
