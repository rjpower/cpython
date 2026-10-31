# Root and traversal checker audit

- State: queued
- Updated: 2026-10-31
- Owner: unassigned
- Dependencies: finite current-source reconciliation and ownership assignment
- Branch/head: none assigned
- Worktree: none assigned

## Now

Historical handoffs mix repaired runtime issues, incomplete precision coverage
and checker limitations. Reconcile them before implementation; this ledger is
not a newly established supported-runtime blocker.

Traversal auditing still uses textual member/path heuristics and bounded
pointer depth. C checking retains the block-local expected failure and opaque
dict/type boundaries. Remaining questions include SLOT/specializer coverage,
folded-reader operands, saved-exception controls and strict PyMem ownership.
Do not repeat stale claims: compiler capsules now traverse metadata, writers
have registered owners, and argument-order/hashtable negative controls exist.
Those facts do not alone close every stronger precision obligation.

## Next

Assign one finite ledger: map each archived handoff to current source and
evidence, then classify it closed, retained by contract or needing a concrete
checker/control repair. Separate diagnostic limitations from missing roots.

## Completion

Every retained finding has a source-bound disposition and owner. Checker
repairs need meaningful omission/collecting negatives; runtime repairs need
observable data/liveness evidence. Do not weaken validation to empty the list.

## References

- [Historical handoffs, §0.1/§0.3](../../archive/process-2026-10-31.md).
- [Root contract](../../design/11-precise-roots.md) and [2b scope](../../design/work/10a-2b-asbuilt.md).
- [Traversal checker](../../../Tools/boil/traverse_audit.py).
- [C checker controls](../../../Lib/test/test_generated_cases.py).
