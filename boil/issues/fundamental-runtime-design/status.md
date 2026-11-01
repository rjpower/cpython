# Fundamental runtime design review

- State: baseline/site evidence accepted; integrated-storage and interpreter-region source active; hard GC cutoff set
- Updated: 2026-11-01
- Owner: coordinator; coroutine_design_review (IR/VM), restart_audit (independent review),
  qualification_review_resumed (integrated storage)
- Source: baseline clean `417a9443a59`; storage prototype base `5ece3dc211a`; maintained runtime unchanged
- Prior research budget (closed): checkpoint 01:40 UTC, reports by 02:00 UTC

**User's hard GC cutoff: 2026-11-01 15:14:44 UTC.** The
[twelve-hour decision](../gc-parity-deadline/status.md) and
[machine-readable budget](../gc-parity-deadline/budget.json) govern all new GC
work: valid all-29 geometric-mean elapsed parity against matched upstream, or
stop tracing-GC optimization and switch to the prepared RC successor. Critical
correctness and focused tests precede timing; expensive full qualification
follows a promising measured candidate. Existing shorter source grants remain.

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

As of **04:23:23 UTC**, stable-span v4 native batch is active, session **76892**,
inclusive ceiling **05:08:23 UTC**. Hard GC cutoff remains 15:14:44.
No prototype has earned performance acceptance or integration.

- **Integrated storage:** frozen source `34deae3b29260aea286b0c4017b963e092a2db62`
  in `build/worktrees/gc-integrated-spans`. Initial source and the supported-call
  and forced-manual-collection witness corrections passed independent review.
  The [v3 helper review](span-native-helper-review-20261101-v3.md) binds fresh
  debug-v3/release-v3 products, the unchanged 26-case selection and four exact
  release skips. Four rows build/test debug then release, stopping on first
  failure. V1 aborted in an unusable legacy C-API fixture; v2 built successfully
  but exposed a disabled-GC witness mistake. Both failures remain preserved:
  [v1 outcome](span-native-outcome-20261101-v1.md),
  [v2 outcome](span-native-outcome-20261101-v2.md). V3 failed to import the test
  extension because the new manual-collection entry is internal to the core:
  [v3 outcome](span-native-outcome-20261101-v3.md). Narrow correction
  `7d10d5bfd33cb7302df8008065c4dd637e453478` moves the witness body behind an
  existing-style exported core test hook. Independent source review and
  fresh v4 helper preparation follow; no selected case ran in v3.
  The [v4 source review](span-link-review-20261101-v4.md) and
  [helper review](span-native-helper-review-20261101-v4.md) passed; fresh v4
  debug/release build and focused tests are active.
- **Early elapsed/RSS:** host-only preparation through **04:40**, checkpoint
  **04:25**, pending successful v3 products and outcome acceptance. Use the
  fixed three-arm/four-workload/two-round protocol; occupancy vectors are
  unavailable. No benchmark launch or additional executor is authorized yet.
- **Interpreter regions:** frozen clean source `40498d1e0d3773ac2a1180eb8cac7fd04831028d`
  passed [independent source review](region-source-review-20261101-v1.md).
  Seven generated products reproduce; 78 ordinary host methods and one existing
  expected failure; twelve new native methods remain unexecuted. V1 helper
  review passed, but root's preflight mistakenly rejected an empty build
  directory and then invoked the supervisor without activation. The supervisor
  correctly refused; no compiler, gate or target ran. Preserve that failed
  preactivation evidence. Minimal host-only v2 rebinding runs through **04:25**,
  checkpoint **04:18**; no source change or native launch is granted by it.
  V2 then passed host review and activated at 04:20:07. Its debug build passed,
  but the wrapper stopped at 04:21:47 before cases because its output directory
  was missing: [outcome](region-native-outcome-20261101-v2.md). Separate v3
  repair is bounded by **04:38**, checkpoint **04:30**; reuse the valid debug
  image through an exact immutable receipt bridge, with actual output-write
  preflight. Source/34-case contract are unchanged.
- **RC successor:** coherent restoration `1440f8c6fd1`, RC-only closure
  `d7df12b1d81722927e71d9734c70eeafdccfee73`, isolated in
  `build/worktrees/rc-successor`. [Independent source review](rc-source-review-20261101-v1.md)
  accepts first native feedback: exact ownership projection and artifacts match,
  both generated products reproduce, 26 generator methods pass. Source review
  is not native qualification. Host-only feedback preparation runs through
  **04:40**, checkpoint **04:25**; historical verifier APIs only, fixed RC policy,
  fresh products. Canonical autoconf and broader qualification remain pending.

The shared stats baseline completed and passed
[independent outcome review](shared-baseline-outcome-review-20261101-v1.md).
The current Fibonacci site capture completed at 03:17:49 and passed
[outcome review](region-site-outcome-review-20261101-v2.md): words4/5,
971,140 integer-compare/false-branch pairs over four loops, 27 actual collections,
zero fallback/unpinned rows. This nominates the family; it proves neither
compiler admission nor elapsed benefit. Original failed capture v1 is retained.

Old retention-oracle, VM0, structural-GC and nursery-adoption helpers remain
held/deferred. Their expired grants authorize no new invocation. Regex
implementation remains paused. Native gates and quiet measurements serialize;
independent source/review work may overlap. Retain one adopted runtime policy.

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
