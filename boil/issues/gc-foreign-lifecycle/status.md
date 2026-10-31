# Foreign interpreter lifecycle

- State: deferred
- Updated: 2026-10-31
- Owner: unassigned
- Dependencies: an explicit decision to restore foreign-interpreter support
- Branch/head: none assigned
- Worktree: none assigned

## Now

The qualified supported scope is Linux, one main interpreter, the GIL and
ordinary threads. Subinterpreter creation is explicitly rejected; ordinary
threads are not deferred by that restriction. Foreign owner retirement,
metadata transfer/adoption and cross-interpreter callback progress are outside
this milestone. They are not unfinished gates for the landed supported GC.

Earlier lifecycle, completion-graph and retirement documents contain useful
counterexamples and design choices. Their proposed assignments and activation
orders are historical inputs, not current instructions to implement adoption
or a scheduler. The current finite drainer/resource policy is not evidence
that those broader proposals are safe or implemented.

Main-interpreter shutdown and fork retain their actual supported contracts;
this deferral does not waive ordinary resource lifetime, roots or error
handling. Actual process exit may leave safe late storage for OS reclamation;
returning from `Py_FinalizeEx` to a live embedding host is a separate contract
and cannot silently rely on that assumption. There are no source changes, native jobs or active owners assigned
to foreign lifecycle work.

## Next

Reopen only after the support boundary is deliberately expanded. First define
the required user-visible creation, close, callback and fork behavior, then
review ownership, metadata/root lifetime and allocator transfer together.
Choose the smallest coherent support contract rather than reviving each old
proposal as an independent requirement.

## Completion

If reactivated, finish and validate the chosen lifecycle contract before
allowing foreign state to be freed or exposing the capability. Historical
source review alone is insufficient. Until then, preserve explicit rejection
and visible unsupported coverage; deferred does not mean abandoned or supported.

## References

- [Current architecture and scope](../../design/work/gc-architecture-next.md).
- [Qualified supported-runtime review](../../design/work/reviews/gc-main-only-final-landing.md).
- [Scope simplification review](../../design/work/reviews/gc-holistic-simplification-review.md).
- [Historical retirement/adoption plan](../../design/work/7a-2c.md).
