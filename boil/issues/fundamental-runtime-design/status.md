# Fundamental runtime design review

- State: shared baseline build running; interpreter and integrated allocator/collector designs under review
- Updated: 2026-11-01
- Owner: coordinator; coroutine_design_review (IR/VM, GC review), restart_audit (IR review),
  qualification_review_resumed (shared baseline and GC)
- Source: maintained head `5ef8c34d9bf`; isolated candidates retain their own identities
- Prior research budget (closed): checkpoint 01:40 UTC, reports by 02:00 UTC

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

One native batch is running: reviewed shared GCC computed/debug/stats baseline,
activated 02:41:08.977 UTC with inclusive deadline 03:26:08.977 UTC. Actual tool
session **20126** and host supervisor PID **1348070** were live at 02:41:45;
the configure step had reached compilation. Monitor that same handle; an
observation timeout does not grant a restart. Exact activation, per-row
launch/result/cleanup and terminal evidence live under
`build/worktrees/vm-compiler-backend/build/shared-attribution-gcc-stats-v1`.
The first unexpected failure stops the batch; no automatic retry or extension.

Retention-oracle v3 helpers passed root host review but remain held, with
readiness flags false and no activation. VM0 build helpers and
the structural-GC failed-prefix witness preparation are suspended at their saved
checkpoints. The object-map source prototype has passed bounded source review;
it has no native or performance acceptance.

The old nursery-adoption dispatch's 04:18:11 UTC integration boundary is
unchanged; that dispatch is deferred. The separately scoped shared baseline
has its own activation-relative 45-minute inclusive window. Research does not
grant a new build, timing campaign, source correction or extension to an expired task.
Any resumed task receives an explicit remaining scope and clock.

## Next decision

Claude's direct medium-effort review completed, terminal zero. The
[verbatim review](claude-fable-5.1-review-20261101.md) and separate IR/VM, GC and
native audits informed the [revised work breakdown](../../design/runtime-work-breakdown.md).
Independent [plan review](work-breakdown-review-20261101.md) accepts that
conditional structure; it grants no native execution or performance adoption.
Counter-retention reporting is integrated at `129131574a8`, with sixteen related
host tests passing. Its independent [review](counter-retention-review-20261101.md)
records exact bytes, meaningful controls and limits.

The shared stats preparation froze early at 02:31:45, with no native launch.
Independent [helper review](shared-stats-helper-review-20261101-v1.md) accepted
the exact frozen capsule before activation. Root's
[source/scope review](shared-baseline-source-review-20261101.md) and actual
02:37:10 preflight verified its source, driver, helper and lock bindings.
A preview origin
finding was withdrawn after replaying the actual pinned-helper admission;
an additional host regression accepts that observer and rejects an unbound path.

The user clarified that holistic interpreter and GC improvements take priority.
The [operand-locality design](operand-locality-design-20261101.md) and
[regex extraction design](../value-native-interfaces/regex-code-extraction-design-20261101.md)
are frozen proposals, not implementation grants. Regex implementation is paused.
New source-only assignments map a reusable value/effect-aware interpreter region
(02:34–02:49, checkpoint 02:41) and one integrated stable-span allocator/collector
increment (02:33–02:48, checkpoint 02:40). They reuse the existing research and
must specify executable steps rather than another survey. No runtime patch,
compiler, target or extra census is authorized by these assignments.
The [interpreter execution proposal](interpreter-region-execution-20261101.md)
froze early and is under independent review, 02:42–02:57 (checkpoint 02:49).
The integrated-storage design's independent review runs 02:40–02:55
(checkpoint 02:47). Both reviews remain source/light only during the build.

The first census has six registered workloads/eighteen warmup-low-high processes;
it supplies existing entry/pair/box counters, not per-state death eligibility or
release timings. This is shared evidence, not a commitment to optimize regex.
The old architecture scope and frozen proposals below remain
historical evidence, not an implementation queue that must be completed.

## Prior design sequence

The user identified that the ranked hypotheses remained GC-heavy despite the
substantial IR/interpreter assessment. A direct local Claude Fable 5.1 review
evaluated the architecture and work breakdown at medium effort. The
initial high-effort plan-mode invocation was stopped on the user's correction;
the replacement uses normal print mode and local pointers, with no Python
launcher. [Request and live handle](claude-fable-5.1-request-20261101.md).
The recommendations were reconciled in the revised plan; new runtime changes
and native exposure still require their exact reviewed scope.

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
