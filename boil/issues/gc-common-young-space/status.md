# Common young allocation and reclamation

- State: queued
- Updated: 2026-10-31
- Owner: unassigned
- Dependencies: closed [GC cleanup/scanning](../gc-cleanup-scanning/status.md);
  source ownership and a bounded implementation assignment remain unassigned
- Branch/head: none assigned
- Worktree: none assigned

## Now

The current recommendation is one useful young-space allocation and
reclamation consumer in the existing collector. No nursery, minor collector,
barrier implementation or copying backend is enabled by this issue.

The existing plan combines Y1's space/lifecycle contract, Y2's complete age
boundary and Y3's safe bulk reclamation and measurements. It covers actual
generator/coroutine allocations and embedded frames, rather than requiring a
cell/tuple-only copying proof or an intermediate collector as separate work.
Stability and age are independent choices; copying needs its own alias contract.

Pending lifecycle metadata must remain non-rooting and correct through
publication, completion, reuse and OOM. Core barriers, native-owner scanning,
major collection, partial-page slack, allocator hooks and ownership all remain
part of the complete consumer. Public `mi_heap_delete()` is not a ready-made
survivor promotion operation.

## Next

Assign one owner and source scope, then close the plan's allocator/page and
lifecycle boundaries in a reviewable proposal. The interpreter attribution
track can proceed in parallel after assignment. No implementation changes or
native jobs have started.

## Completion

Deliver one reviewed experimental path with externally visible liveness and
resource behavior, boundary/oracle checks, bounded memory and matched cost
measurements. Report actual allocation and dead-storage coverage. A proof of
age or movement alone, or an unmeasured speedup estimate, does not justify a
default change.

## References

- [Y1–Y3 implementation and acceptance plan](../../design/work/gc-young-space-next.md).
- [Current recommendation](../../design/work/gc-architecture-next.md).
- [Source feasibility](../../design/work/reviews/gc-young-space-feasibility.md).
- [Allocation coverage](../../design/work/reviews/gc-young-space-coverage.md).
