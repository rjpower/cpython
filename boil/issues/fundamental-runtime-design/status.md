# Fundamental runtime design review

- State: concrete designs frozen; Claude Fable 5.1 peer review running
- Updated: 2026-11-01
- Owner: coordinator; coroutine_design_review (IR), restart_audit (VM),
  qualification_review_resumed (allocator/GC)
- Source: maintained head `4615e52c9bb`; isolated candidates retain their own identities
- Budget: read-only research checkpoint 01:40 UTC, reports by 02:00 UTC

## Question

Does the current IR, interpreter and allocator/collector form a sound performance
architecture, or have we treated correctness and local improvements as evidence
that the architecture should be retained?

The earlier recommendation to keep the IR and collector is reopened. A memory
register VM and stable object addresses remain defensible choices; their current
encoding, temporary ownership, generated execution and storage policies require
separate evidence. The review must rank a coherent replacement direction and
falsifiable increments rather than accumulate permanent runtime options.

## Execution state

No native job is running. Retention-oracle v3 helpers passed root host review but
remain held, with readiness flags false and no activation. VM0 build helpers and
the structural-GC failed-prefix witness preparation are suspended at their saved
checkpoints. The object-map source prototype has passed bounded source review;
it has no native or performance acceptance.

The 04:18:11 UTC integration boundary is unchanged. Research does not grant a
new build, timing campaign, source correction or extension to an expired task.
Any resumed task receives an explicit remaining scope and clock.

## Next decision

The user identified that the ranked hypotheses remained GC-heavy despite the
substantial IR/interpreter assessment. A direct local Claude Fable 5.1 review
now evaluates the architecture and work breakdown at medium effort. The
initial high-effort plan-mode invocation was stopped on the user's correction;
the replacement uses normal print mode and local pointers, with no Python
launcher. [Request and live handle](claude-fable-5.1-request-20261101.md).
No new implementation or census starts before the recommendations are reconciled.

The [integrated candidate](../../design/integrated-runtime-candidate.md) passed
[independent review](review-20261101-v1.md) for bounded design selection only.
It recommends logical values before frame placement, effect-complete bounded
regions and integrated stable-address core storage. SSA, compact words, a new
backend and arenas are hypotheses rather than accepted speedups.
[Loom publication and exact readback](publication-20261101.md) are recorded.

Two new host-only tasks have explicit clocks: one actual IR-region consumer
design (01:42–02:02 UTC, checkpoint 01:52), and the iterator's missing causal
failed-prefix witness design (01:42–01:54 UTC, checkpoint 01:48). These do not
resume the expired VM0/feedback preparation grants or authorize implementation.
A third host-only task designs one integrated stable-storage cohort
(01:44–02:04 UTC, checkpoint 01:54). It must account for real allocation/death
eligibility before selecting coroutines/frames merely from source presence.

All three designs finished early and remain held:

- [IR region](ir-region-hypothesis.md), SHA256
  `b72b5e668dd9799198071cbea61ccea179de4cac42477301052cf251feccc9a2`:
  compare/forward-branch is provisional; registered-workload hotness still
  requires a separately reviewed counter-only census.
- [Stable storage](stable-storage-hypothesis.md), SHA256
  `8c9aab866f2685b74cf36ae2b42fd28cd8e764e46749f47cd72fcfada2b29e01`:
  exact coroutine/frame spans are nominated; source-predicted constructions
  are not a measured death-eligibility/survival/slack histogram.
- [Failed-prefix witness](../gc-nursery-old-work/failed-prefix-witness-design-20261101-v1.md),
  SHA256 `bdd3068e2a1beeddd1b699faadda815d4fff6f0e38b02367ce3daa8b30c7e22d`:
  direct real-prefix and independent pre/post cleanup evidence, still awaiting
  independent design disposition and any source grant.

Select one next executable hypothesis with its correctness boundary,
whole-workload performance decision and migration cost. Existing rejected
experiments stay rejected. Historical incomplete timing stays incomplete.

## Frozen research

All three reports finished early, without compiler or target execution. Their
source analyses inform the candidate; their proposals are not measured wins.

| Report | SHA256 |
| --- | --- |
| [IR](../../../build/fundamental-runtime-review-20261101/ir.md) | `aef2922b51083eab051a6a2bb2c16b2d5e8b844b4d4626dcaee32bb192b14efc` |
| [VM and codegen](../../../build/fundamental-runtime-review-20261101/vm-codegen.md) | `e8d1826fab665a0ed9e4cd40c174bfa26a08bde0ae94f37c5faea8a953904970` |
| [GC and allocator](../../../build/fundamental-runtime-review-20261101/gc-allocator.md) | `5cc2f3a01a56bf06183d73e71c87cc0f97456fbe47848869566692e5bc105459` |

The IR report's static inventory and receipt and the GC report's final evidence
and receipt remain alongside those original reports. The GC final snapshot
supersedes its early checkpoint hashes; original source/product images remain
unchanged. The repository synthesis and independent review preserve the main
findings and decisions separately from these build evidence files.
