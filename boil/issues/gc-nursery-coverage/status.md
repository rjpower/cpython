# Broaden nursery coverage and cheap reclamation

- State: queued
- Owner: unassigned
- Updated: 2026-10-31
- Budget: one working day for a selected allocation/lifecycle family; refine before dispatch
- Depends on: [minimal nursery decision](../gc-nursery-minimum/status.md)

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

Wait for the first nursery result, then assign one family with explicit files,
checks and a decision deadline. No source or benchmark work is running here.
