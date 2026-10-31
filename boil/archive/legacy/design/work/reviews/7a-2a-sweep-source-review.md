# Queue2a slice1 — independent source review

2026-10-29. Verdict: **HOLD** on the frozen four-file candidate pending the
dead-snapshot lock prerequisite below. This is a source disposition, not an
execution or landing grant. No compiler, native process, test or probe was
run by this review; the owner worktree was not edited.

## Reviewed identity

Worktree: `.worktrees/boil-queue2a-sweep`, base
`5a45f9bb6a73e7351c861faf05624ae4810f6d87`.

- Actual four-file diff and `build/Queue2a-sweep/source-v1.diff`:
  `7ba64afee8c53956f50a7f4178499a6c2d3a1d4d22435e64b6bf69f05c335721`.
- Owner source checkpoint at final read:
  `70152d2fc14a0df74344bc21013245ac2efb31374f6be81c51eec7a84004466b`,
  including the lock-risk clarification. The originally supplied checkpoint
  was `04c8e8a22eec0528f56fb7dc15205001cda3215700d286a0fbb37f0584c30247`;
  the four-file source diff remains unchanged.
- Unexecuted copied unsafe-continuation/fallback negative:
  `d34d10000845575fd141923f64600982bc856b7baef7d5dadec2f27ffa8c25c2`.
- Primary design: `boil/design/work/7a-2a-safety-design.md`, including its
  explicit conditional no-mutator proof and separate MARKED-OOM debt.

The implementation changes only `Python/gc_live.c`,
`Include/internal/pycore_gc.h`, `Modules/_testinternalcapi.c` and
`Lib/test/test_gc.py`. Source diff whitespace validation passed.

## Blocking prerequisite: prepared edges can outlive a GIL release

`collect()` now computes the first-object order at `gc_live.c:1583`, before
`clear_callback_weakrefs()`. That function calls `_PyWeakref_ListPtr()` at
`:669`; `_PyWeakref_ClearRef()` can perform another lookup while unlinking
the weakref. `Objects/weakrefobject.c:70` uses `PyMutex_Lock` on the
process-global weakref table. `Python/lock.c:579` supplies
`_PY_LOCK_DETACH`, passed into `_PyParkingLot_Park()` by the contended path
at `:119`. The source expressly supports table contention between own-GIL
interpreters. A short holder critical section does not prove the waiting
collector keeps its GIL.

The successful prepared-order argument needs unchanged strong fields from
its traversal through batching and use. Complete resurrection removes a
reachability-closed set, but that argument does not cover concurrent strong
edge mutation after a detach. Live weakrefs to initially dead objects have
not necessarily all been cleared at the first lookup. Interpreter pins
protect interpreter lifetime, not this edge snapshot. No reproduction or
particular adversarial schedule is claimed here.

The detaching weakref path is inherited, and the design now records it
honestly. Moving order capture ahead of it nevertheless makes the new
success-path ordering proof conditional; calling it existing S3 debt does
not discharge that proof. Keep the candidate frozen until the coordinator
accepts a concrete non-detaching lock prerequisite or a revised preparation
scheme with a valid storage and graph-stability argument. Merely reserving
storage before batching is not itself such a proof: computation must still
be complete, allocation-safe and based on stable edges at its use boundary.
This finding does not call for rerunning the held S1/S3 native probes.

The owner's concurrent source audit identifies a further prerequisite for
any lock change: do not switch only collector acquisitions to non-detaching
waits. Existing detaching waiters can receive mutex ownership by handoff
before reacquiring their GIL. A collector holding that GIL while waiting
without detach can then deadlock with the owner. Any proposed registry
protocol must cover all its acquisitions and its holder operations. Fixed
table hash/equality and absent destruction callbacks help, but table
allocation/free uses RAW allocator hooks, whose custom-hook contract still
needs an explicit treatment. No lock edit is approved by this review.

## Source checks that otherwise clear

`order_dead_first()` initializes both outputs and distinguishes empty
success from failure. Empty/no-FIRST paths allocate nothing. Failure
short-circuits before weakref/finalizer batching; the unconditional
`route_calls()` receives an empty batch and publishes no call. There is no
remaining first-phase heap-order fallback. Prepared indices are freed on
both sweep and rollback paths.

The ten named allocation sites are one index array, one initial edge-vector
growth for the four-reader chain, and eight adjacency/DFS scratch arrays.
All use `order_alloc()`, which bounds the byte product by
`PY_SSIZE_T_MAX`; its element sizes are nonzero constants. Sentinel counts
are added after conversion to `size_t`, so they do not overflow signed
counts. Unrepresentable array products fail before allocation/use. Edge
capacity doubling is checked before signed multiplication. Prefix sums,
DFS discovery counts and stack positions are bounded by the already
represented edge/node counts. Partial scratch allocation is freed and a
failed traversal is rejected rather than treated as a complete graph.
These are source arithmetic checks, not executed overflow fault tests.

`undo_dead()` adds every abandoned dead block back to `live_bytes` once.
Marking excluded those blocks from the initial live-byte total;
resurrection clears flags but does not add their bytes. The failed path
does not subsequently execute sweep's retained-block accounting. Keeping
`gc->heap_size` at `res.nobjects` on failure matches the retained heap.
Later existing node/stack failure still routes already batched calls,
retains blocks and discards the local garbage list. Eligibility, delivery
markers and the old node-allocation hook are unchanged. Existing temporary
MARKED cleanup failures remain separate debt, not a claim of this patch.

Under a stable strong graph, the surviving-order proof is sound: a removed
reachability-closed set contains all or none of each SCC and cannot provide
a path between surviving nodes. Restricting reverse DFS to survivors
therefore retains the existing component order and arbitrary cycle order.
`sweep_one()` skips resurrected entries. The new cycle fixture really has
direct flagged edges: `PyObject_VisitManagedDict()` at
`Objects/shapeobject.c:811` visits inline attribute values, including the
two `Sub.other` fields. Independent native referents avoid imposing an
impossible native deallocation order within that cycle.

The new integer-only hook uses the fork's correct VARARGS ABI, rejects
negative counts and preserves the old node hook. The allocation test arms
that old hook as a causal barrier: an ordering failure must leave its
countdown at one, preserve all weakrefs, free no bytes, and permit one later
successful finalization/reclamation. The copied negative deliberately
continues after ordering failure and must consume the old hook, producing
the named `ordering OOM reached queue allocation` failure. That proves the
pre-batch abort condition without requiring a corrupting deallocation.
It has not yet been executed. Both children explicitly use N0 while
preserving/asserting actual verifier mode; no uniform ambient-stress claim
is warranted for these injection fixtures.

## Next disposition

Resolve the snapshot lock prerequisite by source review first. Then refresh
the exact candidate/proof binding if needed and seek the coordinator's
bounded build/control grant. The ten-site OOM test, normal ordering and
resurrection checks, and copied named negative still need actual execution;
this review supplies no native qualification. Debug writer reentry,
completion scheduling and MARKED-OOM cleanup remain explicit separate work.

## Round 2 — late preparation

2026-10-30. Verdict: **SOURCE_CLEAR for bounded native validation** of V2.
The V1 HOLD above remains historical and applies to its eager preparation.
This verdict supplies neither an execution grant nor native/landing credit.
No native, compiler, test or probe ran during this review.

Exact current four-file diff and `source-v2.diff`:
`bc42c1ecb5d740cd9b8b9602fb6107d5875c8cefd0a8c2a65b99b2878c317b3e`.
Checkpoint `source-checkpoint.md`:
`53342b0d89a8c210997c8bd58878ca6e1e8776e580750a351e3e7f13b77b81c4`.
Revised unexecuted copied fallback negative:
`173a23a36542ee81da55602b4d15cf819cfe3b28fbaf87dbee08e4ba4b351f3c`.
The four-file diff matches the frozen copy; whitespace validation passes.

Preparation now occurs after routing, unpinning, no-callback weakref
clearing and collectable debug output, immediately before `sweep()` sets
`gc->sweeping` or captures the exception. This uses the existing late graph
boundary and does not carry prepared edges across the earlier weakref/debug
interval. It removes the new V1 proof obligation without claiming that the
inherited snapshot, debug-writer or detaching-lock paths are safe.

Both earlier batching failure and late ordering failure reach one rollback.
No `sweep_one()` or block free has occurred; collected/freed remain zero for
that pass, `undo_dead()` restores all retained bytes once, the retained heap
count is preserved and local garbage publication is discarded. Already
routed calls and changed weakrefs/notified flags remain unchanged. Pins are
released exactly once at the existing boundary. Ordering storage is freed
once. Success enters sweep with the completed order and cannot fall back.
The initialized status/empty contract and checked allocation arithmetic
reviewed above remain intact. No new lock protocol is bundled.

The revised ten-site test has no delivery-producing objects, preventing
`collect_from_entry()` from hiding the failed pass behind its automatic
second pass. Four attribute-linked flagged nodes each own an independent
native referent: all ten ordering allocations remain reachable, while the
copied unordered fallback can reach the named collected/freed assertion
without first violating a native referent dependency. It checks fired
injection, zero collection/freed bytes, retained live bytes, and subsequent
reclamation. It correctly drops the former weakref-survival and untouched
node-hook requirements.

The separate partial-delivery fixture leaves one native reader chain dead
while a queued finalizer retains another, so the late allocation is still
required. Its callback and finalizer must run exactly once in their existing
order. The finalizer records cumulative freed bytes after the failed pass
and before any automatic second pass, proving that pass freed nothing.
Subsequent collection checks eventual ordered reclamation without claiming
the entire public `gc.collect()` call freed nothing. The SCC fixture is now
an ordinary post-resurrection lifetime regression, not an eager-order proof.
All three children retain the disclosed N0 injection override and actual
verifier assertions. The integer hook and old node hook stay separate.

The revised copied negative ignores late preparation failure and restores
only the old missing-order fallback. Its expected failure is the unchanged
test's named `ordering OOM swept objects` or freed-block assertion, not an
import, startup, compiler failure or native crash. This is a meaningful
no-sweep oracle, still unexecuted. No further source blocker was found for
the coordinator's bounded validation decision. Snapshot locks, S3 output,
completion scheduling and existing MARKED-OOM debt remain open GC work.
