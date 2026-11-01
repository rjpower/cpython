# Fundamental runtime design review

- State: IR/spans rejected; backend focused feedback accepted, elapsed preparation; page-map source active; hard GC cutoff set
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

As of **05:49 UTC**, no native job is active. GC parity remains unproven;
the hard cutoff is **15:14:44 UTC**, without extension. Maintained runtime is
unchanged. Rejected sources, failed attempts and raw evidence remain preserved.

- **IR regions — REJECT:** source40498 passed34 selected methods per image and
  demonstrated local home/dispatch removal, then slowed coroutines10.86%/14.11%
  and four-row means16.21%/13.74%.
  [Elapsed decision](region-elapsed-outcome-review-20261101-v1.md).
  No extra samples, broad qualification or composition into another candidate.
- **Integrated spans — REJECT:** corrected source5a passed debug27 and release23
  plus four declared skips, including strict public-free and published-sweep
  witnesses. [Focused outcome](span-native-outcome-review-20261101-v5.md).
  The fixed three-arm pilot then slowed coroutines50.74%/51.07% versus parent;
  four-row means regressed14.79%/10.26%.
  [Independent elapsed/RSS decision](span-elapsed-outcome-review-20261101-v6.md)
  verified120 values, twelve origin warmups, all243 artifacts and idle cleanup.
  No extra samples or full qualification. Earlier v1–v4 failures stay failed;
  original v4 cause remains unresolved. The first elapsed launch's root-created
  empty-lock failure stays [preserved](span-elapsed-launch-outcome-20261101-v5.md).
- **Compiler/backend — focused feedback accepted:** unchanged runtime417/e9,
  fourteen rows completed05:40:26, supervisor66715 exit0 confirmed05:42:05.
  [Independent outcome](../h1-tail-dispatch/reviews/native-outcome-20261101-v3.md)
  accepts105 selected invocations:102 passes and three computed-tail-only skips;
  both tail builds execute the precise-handler case. Five actual stale children
  completed3,000 iterations, zero fallback/unpinned; all products/locks idle.
  Fixed G/C/T elapsed helpers are being prepared through06:04, checkpoint05:55.
  No compiler/backend speedup or selection is established.
- **Pre-score profile rule:** the independently reviewed
  [addendum](../gc-parity-deadline/compiler-profile-addendum-20261101.json)
  preserves the original budget bytes, deadline and all29 decision. Select T
  only if T/C and T/G pass every declared gate; else C if C/G passes; else G.
  One profile stays fixed for the timebox. Clang earns parity only against fresh
  unchanged upstream with the same compiler/backend and actual correctness.
  No mixed-compiler credit, arithmetic composition or profile shopping.
- **Page-map inventory — source attempt:** the
  [selective-port design](../gc-nursery-old-work/maintained-object-map-port-design-20261101.md)
  and [conditional root review](../gc-nursery-old-work/reviews/maintained-object-map-port-design-20261101-root.md)
  authorize one new managed worktree from417, through06:16/checkpoint06:00.
  Preserve the classifier and collector policy; exclude47 nursery/cadence/ledger
  and rejected spans. Require a real nonempty failed death-prefix, independent
  mark/SHALLOW cleanup and fault-free recovery. No source/native acceptance yet.
- **RC fallback — focused behavior and canonical generation accepted:** d7df
  passed39 methods in both fresh images, zero skips.
  [Native outcome](rc-native-outcome-review-20261101-v1.md).
  Exact cached canonical Autoconf reproduced every source byte; metadata restored
  and all7,059 source paths/1,268 old product files independently verified.
  [Canonical review](rc-canonical-review-20261101.md).
  No source commit/rebuild was needed. Broader qualification and integration
  remain owed; the collector is not yet selected.

Loom [runtime-work-breakdown](http://127.0.0.1:7878/s/6rydgj13/artifacts/runtime-work-breakdown)
retains both code-organization diagrams and measured dispositions.
Old retention, nursery-adoption and c43 helper grants remain expired; regex is
paused. Native/quiet work serializes, source/reviews overlap, and adoption must
produce one collector/backend policy.

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
