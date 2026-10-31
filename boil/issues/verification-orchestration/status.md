# Shared verification orchestration

- State: queued
- Updated: 2026-10-31
- Owner: unassigned
- Dependencies: finite runner scope and source ownership need assignment
- Branch/head: none assigned
- Worktree: none assigned

## Now

The verification manifest, worker launcher and identity/result components
exist. Their independent reviews explicitly exclude complete profile
orchestration. `verify.py` still reports execution as unimplemented;
`gate.sh` remains a separate numeric runner. Qualified campaigns used their
reviewed concrete helpers; this missing shared interface does not invalidate
their results or reopen landed components.

The remaining task is one named-profile run path connecting selection,
actual worker modes, source/build identity, protected results, exclusions,
counters and cleanup to existing machine/build admission.

## Next

Assign a bounded implementation proposal that reuses the landed components
and reconciles current manifest rows. Specify the numeric gate integration
without adding another runner or silently changing coverage.

## Completion

A declared profile executes through the shared path and reports truthful
outcomes, including rejected provenance, missing workers, failed tests and
cleanup failures. Preserve earlier component limits and failed invocations;
profile execution must not infer coverage from configuration alone.

## References

- [Current plan-only command](../../tools/verify.py) and [numeric gates](../../tools/gate.sh).
- [Launcher review](../../design/work/reviews/verification-launcher-landing.md).
- [Identity/result component review](../../design/work/reviews/verification-v2-component.md).
- [Manifest review](../../design/work/reviews/verification-manifest-stage1.md).
