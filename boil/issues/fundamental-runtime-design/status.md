# Fundamental runtime design review

- State: baseline accepted; integrated storage source active; pre-target IR capture failure under bounded host repair
- Updated: 2026-11-01
- Owner: coordinator; coroutine_design_review (IR/VM), restart_audit (independent review),
  qualification_review_resumed (integrated storage)
- Source: baseline clean `417a9443a59`; storage prototype base `5ece3dc211a`; maintained runtime unchanged
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

No native job is running. Shared baseline tool session **20126** returned zero;
all ten rows completed and verified at 02:44:35.528 UTC, before the activated
03:26:08.977 ceiling. Final cleanup verified no live target PIDs. Independent
[outcome review](shared-baseline-outcome-review-20261101-v1.md) accepts the
capture, not runtime qualification or performance. Exact activation, per-row
launch/result/cleanup and terminal evidence remain under
`build/worktrees/vm-compiler-backend/build/shared-attribution-gcc-stats-v1`.
The unused window is not authority for another invocation or retry.

Integrated-storage source work runs **02:51–04:21 UTC**, mandatory ownership/
route/lookup checkpoint **03:21**, further progress **03:51**. Worktree:
`build/worktrees/gc-integrated-spans`, branch `boil/gc-integrated-spans`, actual
base `5ece3dc211a`. The exact [plan](stable-storage-mechanism-plan.md) and
[independent review](stable-storage-mechanism-review-20261101.md) govern the
attempt. Preserve ordinary deallocators/lifecycle records and collection policy;
close all ownership/wrapper/debug/quarantine/rollback paths. Host audits and
test source are allowed; compiler, target, native tests and timing are held.
At the ceiling, freeze a review candidate or explicit partial patch.

Interpreter site-capture preparation/review passed, then root activated v1 at
03:08:36.674 UTC with a 03:18:36.674 ceiling. Tool session **73805** returned
**1 before any gate or target launch**: the frozen supervisor's transitive
`feedback_inputs` import selected a same-named baseline `build_once` from the
composed host path. Its variant assertion rejected `launch`. The readiness
review missed that actual import-resolution path. No target or workload call
occurred, and no observation credit follows. Root acquired the actual image
lock at 03:10:40 and verified no live target PID. V1 activation/start/failure
and root disposition remain untouched in
`build/region-site-attribution-20261101-v1`; its unused window grants no retry.

A new host-only repair runs **03:10–03:20 UTC**, checkpoint **03:15**, in
`build/region-site-attribution-20261101-v2`. Independent review runs
**03:10–03:25**, checkpoint **03:18**. It must prove actual frozen-supervisor
dependency origins, including a conflicting cached-module case, with no target
invocation. The proposed observation remains one child, one warmup run(4), one
counted run(4), and IR snapshots outside counting. No rebuild, probe or extra
workload is added. A new finite root activation is required after exact review.

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

The shared stats preparation froze early at 02:31:45.
Independent [helper review](shared-stats-helper-review-20261101-v1.md) accepted
the exact frozen capsule before activation. Root's
[source/scope review](shared-baseline-source-review-20261101.md) and actual
02:37:10 preflight verified its source, driver, helper and lock bindings.
A preview origin
finding was withdrawn after replaying the actual pinned-helper admission;
an additional host regression accepts that observer and rejects an unbound path.

The [current counter findings](shared-baseline-counter-outcome-20261101.md)
record 242,785 integer-compare/false-bool-branch pairs per coroutine loop. This
supports the nominated family, not a PC, successful region count or speedup.
Six journals retain eighteen original dump records. All native boxing tables
saturate and the last label also aggregates overflow. Standard Py_STATS GC
fields are zero despite actual collections; they do not rank tracing GC cost.

The user clarified that holistic interpreter and GC improvements take priority.
The [operand-locality design](operand-locality-design-20261101.md) and
[regex extraction design](../value-native-interfaces/regex-code-extraction-design-20261101.md)
are frozen proposals, not implementation grants. Regex implementation is paused.
The [interpreter execution proposal](interpreter-region-execution-20261101.md)
passed [design review](interpreter-region-review-20261101-v1.md) for a bounded
prototype, with actual site attribution still required before installing the
consumer. The integrated-storage design/review passed for its source attempt;
the unnecessary separate completed-state census is removed because every
ordinary deallocator and lifecycle record remains. Storage sizing and occupancy
are measured inside the prototype's debug witnesses.

The first capture's [helper review](region-site-helper-review-20261101-v1.md)
is preserved as a historical readiness disposition. The successor must close
the missed import path with an actual host-import fixture; another AST-only
check or the failed attempt cannot supply that evidence.

The completed census has six registered workloads/eighteen warmup-low-high
processes; it supplies entry/pair/box evidence, not per-state death eligibility
or release timings. This is shared evidence, not a commitment to optimize regex.
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
