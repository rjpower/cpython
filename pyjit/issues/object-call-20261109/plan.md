# Object lookup and call binding

This round starts from runtime `4d666e1b482` and the
[closed frame/call report](../frame-call-model-20261109/closing-report.md).
The user requested an Astra medium review before publishing that checkpoint
to the existing golden branch and beginning the next improvements.

## Sequence

1. Review the measured priorities and survey LuaJIT/V8 lookup and call designs.
   Separate actual mechanisms from speculative layout changes.
2. Publish the frozen runtime plus canonical documentation to
   `pyjit/golden-20261108`. Preserve the old immutable tags and controls.
   Record the known tooling and frame-jump qualification limits.
3. Select two bounded lanes: canonical attribute lookup and reusable keyword
   binding, subject to the review. Prepare independent proposals and review
   their ownership, mutation, reentry and fallback contracts before editing.
4. Implement in disjoint files or isolated patches. Root owns regeneration,
   release/debug builds, focused tests, commits and serialized native runs.
5. Post-review each actual diff. Integrate and compare unchanged relevant micro
   workloads, DeltaBlue, logging and the complete 29-kernel roster against the
   frozen checkpoint and upstream. Run full diagnostics at the combined
   checkpoint, with previously documented failures reported separately.

## Design constraints

One canonical value model, one owning frame/call ABI and one selected object
storage model. Keep fast paths as guarded plans over semantic operations.
Do not introduce a second compiler, a table/shape mode switch, arbitrary
benchmark-specific cases or exact CPython finalizer timing requirements.

Investigate guarded hash-slot lookup and name interning as bounded mechanisms.
A wholesale attribute-layout rewrite requires evidence unavailable today.
Dictionary string and integer lookup paths must be distinguished in source.

## Reporting

Work began around 23:55 UTC on 2026-11-09. Report substantive progress and actual
measurements by approximately 01:55 UTC, sooner if the checkpoint completes.
This is an active-turn reporting target, not a background scheduled job.
Canonical pages own state; Loom mirrors the current goal, plan, status and
designs. Preserve unrelated work and sealed historical evidence.
