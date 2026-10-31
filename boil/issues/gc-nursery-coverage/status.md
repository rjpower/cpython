# Broaden nursery coverage and cheap reclamation

- State: queued
- Owner: restart_audit designs; coroutine_design_review independently reviews; coordinator decides
- Updated: 2026-10-31
- Budget: eight elapsed hours from first implementation edit; 90-minute source/review and first-debug checkpoints; 60-minute matched comparison
- Depends on: [shared landing](../nursery-shape-landing/status.md), then independent design acceptance

## Hypothesis

Admitting one measured generator/coroutine allocation family and avoiding its
unnecessary per-death work improves allocation-heavy workloads without expanding
the collector's lifetime protocol.

## Experiment

Choose one family after the first nursery result. Cover embedded frames and
required cleanup; measure eligible bytes, actual deaths, memory, pauses and
matched elapsed/instruction cost. Separate eligibility/lifecycle proof from
storage-return cost. Keep other allocation families on their working path.

## Decision rule

Integrate the bounded family when behavior, memory and measured cost justify it.
Reject or split when lifecycle/observer complexity dominates or the admitted
traffic is too small. Do not require every allocation family or copying to close
this experiment. Specify the exact family and comparison before starting.

## Now / next

The [design](design.md) and [independent review](reviews/coroutine-design-20261031.md)
are accepted for bounded implementation after the shared landing. The approved
design hash is `eb80298825500321988b2836ef4092e4787a9a109bf40b94c18de840e9694676`.
It selects exact native coroutines and their embedded frames, young admission
and a guarded completed-object cleanup shortcut. Three matched arms distinguish
admission from the shortcut's incremental value. Debug witnesses must observe
both constructor admissions, dispatch and actual young frees with explicit
byte units. Added promotion scans may outweigh the small sampled teardown
cost; neither coverage nor activity alone establishes a useful optimization.

No runtime changes, builds or benchmarks are running here. Implementation waits
for the shared landing. The implementation clock has not started. Ordinary generators,
async generators, allocator replacement and old-scan reduction remain outside
this family scope.
