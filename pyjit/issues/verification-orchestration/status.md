# Shared verification orchestration

- State: queued
- Updated: 2026-10-31
- Owner: unassigned
- Dependencies: one finite runner scope and source ownership
- Branch/head: none assigned
- Worktree: none assigned

## Now

The verification manifest, worker launcher and identity/result components exist.
`verify.py` validates and displays plans; shared profile execution is unimplemented.
`gate.sh` is a separate numeric runner. Qualified concrete campaigns retain their
own execution evidence; the missing shared interface does not invalidate it.

## Next

Propose one named-profile run path connecting actual selection, worker modes,
source/build identity, protected results, exclusions and cleanup to existing
machine/build admission. Reuse components and reconcile concrete manifest rows
rather than introducing another runner or silently changing coverage.

## Completion

A declared profile executes and reports actual outcomes, including provenance
rejection, missing workers, failed tests and cleanup failure. A schema, plan or
expected mode is not coverage.

## References

- [Validation contract](../../design/validation.md).
- [Tool entry points](../../tools/verification.md).
- [Launcher](reviews/launcher.md), [identity/results](reviews/identity-results.md)
  and [manifest](reviews/manifest.md) reviews.
