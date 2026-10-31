# Minimal nonmoving nursery

- State: active
- Owner: string_design; coordinator integrates
- Updated: 2026-10-31
- Budget: first decision within 60 minutes of 15:15 UTC; checkpoint at 30 minutes
- Depends on: [landed GC baseline](../gc-cleanup-scanning/status.md)
- Branch: boil/gc-nursery-minimum, base b3f2bcc605c
- Worktree: ../boil-gc-nursery-minimum; source preparation

## Hypothesis

A small stable-address young cohort can run a real minor collection through the
existing mark/death engine, using a conservative old-object boundary. Existing
mimalloc storage and in-place promotion can avoid a new allocator or copying
protocol in the first increment.

## Experiment

Initially admit exact cells/tuples. Preserve full major collection, lifecycle
behavior, construction/OOM safety and finite major debt. Conservatively scan old
objects rather than require all barriers before the first useful consumer.
Check live payloads and old-to-young references, dead-young reclamation, survivor
promotion and eventual major reclamation. Inspect metadata cost and timing;
this stage makes no broad performance claim.

## Decision rule

Integrate a small functioning, independently reviewed minor path when relevant
behavior and qualification pass and the representation remains simple. Reject
or narrow if it needs a second collector/identity system or cannot preserve the
current contracts within the bounded scope. The first hour ends with an
integrate/reject decision or a separately bounded implementation follow-up;
unfinished work does not become landed by reaching the deadline.

## Now / next

The source feasibility pass selected existing allocator storage plus a young
header flag and conservative old roots as the first candidate. Close publication,
reentry, accounting and rollback details, then prepare a bounded source change.
Broader coverage and reduced old scanning are separate follow-ups.
