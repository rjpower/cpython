# Register semantics and interpreter parity

Status: stage remains open. Golden82
`5194e957c5d573ef2b6112508a7b0990b7de656d`, the [gap82](gap82.md) integration,
was selected on 2026-11-20 and is pushed as `pyjit/round82-golden`; it runs
0.992 of upstream's time on the timed all-29 geomean. Golden73
`44f066f5fd55947edf709bbcf9bf17c607307e4b` (`pyjit/round73-golden`, selected
2026-11-16 with an accepted logging regression) and selected57 remain
controls. [Parity84](parity84.md) (2026-11-21) integrates profile83's clean
changes as IP2 (`4be2eb5d31f`, `pyjit/parity84`): 0.961 of golden82's time
and 0.953 of upstream's, awaiting selection. Root coordinates
integration and native validation; Sol 6.1 workers implement and cross-review.

Both [execution72](../../design/proposals/execution-state72.md) units are
implemented and rejected. The cold-layout unit demonstrated no packing gain.
The [carried-owner ABI](execution72-abi.md) passes focused runtime and native
controls but loses its protected screen: hot8 GM 1.067106, DeltaBlue 1.068427,
logging 1.210843 and raytrace 1.289980 versus selected57. The common-case result
transport remains too expensive; no frame-size or initialization reduction
was implemented. All sources and evidence are preserved; no losing trial is a
prerequisite or a candidate for retiming.

The [trial report](report.md) owns exact outcomes, qualification limits and
follow-up constraints. The [active plan](../interpreter-performance56/plan.md)
owns the next architecture decision, and [handoff](../../HANDOFF.md) owns runtime
paths and commands. Earlier59–71 attempts remain closed; their designs/reviews
are historical evidence. Execution-boundary73 has completed grouped review,
implementation, protocol qualification and the protected screen. Its complete
generator/coroutine paths improve, but logging regresses.
The subsequent shared-return isolation correction also fails logging protection.
The user selected the corrected checkpoint despite that regression.
Preserve both implementations and measured evidence; the cycle-cost cause remains
open, and another correction requires a concrete source contract. The bounded
[diagnosis](execution73-diagnosis.md) explains rejected72 without reopening its
selection. The new work must demonstrate
a complete execution-boundary saving, preserve specialization coverage, and
keep cold observation/cleanup obligations out of ordinary transfer paths where
ownership permits.

[Execution74](execution74-diagnosis.md) finds that golden73's logging cost is
op-cache placement, which moves single workloads by up to 13% with no change in
work. [Handlers75](handlers75.md) therefore makes handlers smaller and fewer
before further placement-sensitive screens.

The user requests repeated architecture → ordered implementation → measured
selection cycles until upstream parity. The hot set is comprehensions,
DeltaBlue, generators and coroutines, with logging, raytrace, spectral_norm
and nqueens as regression sentinels. Broader qualification includes all29 unchanged
workloads; golden73 was selected by explicit user exception before that run.
Equal instruction counts are diagnostic, not the throughput gate.

The [preceding review](../../design/proposals/interpreter-parity58.md) identified
lost facts and physical home traffic but constrained the existing operation
model too early. This cycle explicitly considers logical value identities,
owner obligations, output bindings, control transfers and physical window
allocation as separate decisions. Current encoding and operation semantics are
not automatic constraints. Existing selected features and rejected experiments
remain evidence; they are not proposed anew.

## Active assignments

| Owner | Deliverable | State |
| --- | --- | --- |
| Root | Golden73 integrated and pushed with accepted logging tradeoff | Own broader qualification and successor validation |
| Astra | Grouped execution-boundary review complete | Outside the component implementation loop |
| Sol 6.1 compiler worker | Specialization preservation and continuation admission complete | Return isolation source peer complete |
| Sol 6.1 execution worker | Complete generator transfer and retirement complete | Return isolation implemented and measured |

The [compact unit](compact-domain64.md) and its [independent peer](compact-domain64-peer.md)
record its bounded execution attempt. The [fixed-role unit](role-lowering63.md)
retains the source contracts and rejected measurements. The [await unit](deferred-await63.md) separates stable active identity views
from terminal storage movement. Its complete reader and lifecycle closure is
covered by the completed source review. Both role packets are qualified and
rejected; await has a qualified native mechanism and a rejected performance screen.

## Reviewed trial order

| Trial | Adjustment | State |
| --- | --- | --- |
| 1 | Bind a BODY result at final publication, independent of its staging cell | f61edc21 passes generation, both builds and 37 focused methods/role; rejected by hot-eight screen |
| 2 | Separate builder input-span geometry from result allocation; FOR family follows only with complete fallback support | d4584f8e45a passes generation, both builds and 207 methods/role; standalone loses logging/DeltaBlue, not selected |
| 3 | Connect native Invoke's explicit result-owner token to the existing CFG result and emit one ordered input-retirement plan | Composition 582 and isolated 5c60 pass focused gates but both screens lose |
| 4 | Carry an admitted recipient action in the continuation instead of repeatedly classifying raw return_slot | 14bf4eb passes both builds and 112 methods/role; screen loses, closed |
| 5 | Separate edge value identity from home identity at a demonstrated CFG allocation consumer | Demoted: no new admitted consumer beyond closed zero-COPY census |
| 6 | Keep values/projections resident across semantic operations; choose regions, cache or wider physical form by residual cost | Engine loses; compact64 stops at native work gate; complete await source independently approved |

Coordinator architecture review: GO for the bounded trials, with existing
selected outcomes, call result IDs, overlap and direct returns retained.
The review corrected the unsupported KEEP_NULL admission, distinguished native
result identity from already-fresh CFG output, and required whole-family geometry
closure before allocation freedom. No arbitrary-hole allocator or new root
protocol is assumed. These constraints still allow IR/encoding changes when an
actual consumer and all readers support them. No parity result follows from
this design approval.

No isolated source deletion counts as a performance result. Each trial follows propose → peer →
implement → focused tests → actual-diff review → integrate and measure. Full
diagnostics occur at coherent protocol/composition checkpoints. Preserve all
immutable controls and sealed evidence; do not rebuild canonical-doc sources.

After each integrated composition, report candidate/selected57 and direct
upstream ratios and residual costs. If parity is unmet, Astra reviews the
actual new state as a group before the next ordered cycle. A losing experiment
is closed with evidence; a completed plan does not complete the goal.
