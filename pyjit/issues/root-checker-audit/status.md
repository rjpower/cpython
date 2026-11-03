# Root and traversal checker audit

- State: queued
- Updated: 2026-10-31
- Owner: unassigned
- Dependencies: finite current-source reconciliation and ownership assignment
- Branch/head: none assigned
- Worktree: none assigned

## Now

Traversal auditing uses textual member/path heuristics and bounded pointer depth.
C checking retains expected-failure and opaque-boundary limits. Questions include
SLOT/specializer coverage, folded-reader operands, saved-exception controls and
strict PyMem ownership. Separate diagnostic limits from concrete missing roots;
this backlog is not a newly established supported-runtime blocker.

## Next

Select one bounded reconciliation: map each finding to current source and
accepted evidence, then classify it closed, retained by contract or needing a
specific control/repair. Split implementation from this backlog before execution.

## Completion

Every admitted finding has a source-bound disposition. Checker repairs use
meaningful omitted-root/collecting negatives; runtime repairs validate data and
liveness. Do not weaken checks to empty a list.

## References

- [Root/lifetime contract](../../design/gc-roots-and-lifetimes.md).
- [Precise managed-edge proposal](precise-managed-edges.md).
- [Traversal checker](../../../Tools/boil/traverse_audit.py).
- [C checker controls](../../../Lib/test/test_generated_cases.py).
