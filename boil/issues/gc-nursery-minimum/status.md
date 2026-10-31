# Minimal nonmoving nursery

- State: review
- Owner: string_design; coordinator integrates
- Updated: 2026-10-31
- Budget: first decision within 60 minutes of 15:15 UTC; checkpoint at 30 minutes
- Depends on: [landed GC baseline](../gc-cleanup-scanning/status.md)
- Branch: boil/gc-nursery-minimum, candidate 3a473612157
- Worktree: ../boil-gc-nursery-minimum
- Phase: bounded experiment accepted for qualification; not landed
- Decision: integrate if coherent landing qualification passes

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

Candidate 3a473612157 passed independent source review, fresh O1/Py_DEBUG
build, five supported behavior checks and whole GC/tuple/types in OFF and KILL.
The latter ran 362 tests per mode, with 23 and 28 skips respectively; the five
additional KILL skips are exactly the unsupported nursery controls. A safe
full-instead-of-minor negative failed at the intended old-owner assertion.
Independent review replayed the results and verified cleanup.

The first bounded hypothesis works. The selected candidate is in
[shared landing qualification](../nursery-shape-landing/status.md) with the
interpreter change. Keep the nursery opt-in;
the all-old scan and tiny cohort establish no performance gain.

## References

- [Design](design.md).
- [Three bounded stages](plan.md).
- [Results and retained evidence](results.md).
