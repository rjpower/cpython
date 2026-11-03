# Collector work-stack OOM — independent source review

2026-10-30. **SOURCE_CLEAR for bounded native validation.** No execution,
landing or whole-GC safety grant follows. No compiler, native test or probe
ran during this review; the owner worktree was not edited.

Reviewed `boil-queue2a-workstack`, base
`009fa2dcbe8777d51099536ba894c46faa3aa889`:

- Actual four-file diff and frozen source copy:
  `3ac422d2a807d96aa70338106c23923b9793b81124731d108f91eb971941ea7d`.
- Checkpoint:
  `239ba52d2842c332b947d7e9ed47595fd519714a3fd65cffa18886b9a659138a`.
- AS-BUILT caller proof:
  `b750a424aba259988e84b692625ffe786960a67352d9929e1b1bbe54fd17d4f1`.
- Unexecuted mark-before-journal negative:
  `254e0152b3d6daf10555a23ad3a101fe3cccbb2a220ba741db8bfafdcd6ef154`.

The source diff matches its frozen copy and whitespace validation passes.
Changes are confined to gc_live, the internal integer failure-hook
declaration/VARARGS binding, and two tests in test_gc. No flag layout,
root algorithm, callback contract or scheduler change is bundled.

## Implementation checks

`objstack_push()` makes OOM sticky and returns failure before another
append. With n<=cap<=PY_SSIZE_T_MAX/sizeof(pointer), growth either doubles
safely or saturates at the item bound; a full-bound append fails. The
existing initial1024 capacity fits supported platform bounds. Allocation
failure changes neither storage, capacity nor n; successful append has
n<cap before increment. The independent growth-only hook does not consume
queue/order/debug allocation countdowns.

Both temporary-MARKED sites publish a successful journal entry first.
`in_dead_cycle()` distinguishes failure from cycle/no-cycle, clears every
journaled mark even after failure and prevents incomplete traversal from
publishing garbage. `finalizer_order()` stops the entire outer/inner walk
on root, visited, exit-marker, output or traversal failure. Its single
cleanup clears all visited marks and frees scratch; output reversal and
success occur only after a complete walk. No set mark lacks its cleanup
entry in the reviewed implementation.

All direct push/resurrect callsites were inspected. Resurrection enqueues
before clearing DEAD, stops on error, and propagates failure through the
legacy/garbage/queued-call/destroy-hook callers. Garbage, legacy and SAVEALL
appends are checked. The existing common rollback retains every dead
snapshot block and restores live accounting; already batched calls and
notified bits remain authoritative. No incomplete resurrection reaches
sweep, and no new transaction/duplicate-delivery claim is made.

The traversal proof uses the EXISTING visitor contract: nonzero visit
results propagate immediately (`Doc/c-api/gcsupport.rst:252/360`). Managed
attribute/static-type traversal follows that convention. This patch is
not a proof or compatibility fallback for an extension that ignores
visitor errors, allocates unexpectedly or releases the GIL in traversal.
Snapshot/locking and deallocator-contract work remain separate.

## Meaningful controls and limits

The slotted modern-finalizer fixture owns no independently dying referent;
its class, method and result list remain rooted. The small path maps its
three growth failures to root stack, visited journal and order output.
Each child requires the injected failure, zero collection/delivery, one
finalizer on the VERY NEXT collection and no duplicate on another
collection. This detects the stale-mark retention that an eventual retry
would hide.

The legacy fixture has only a self-link, uses existing with_tp_del, and
fails the first cycle-journal growth. It rejects premature garbage/tp_del,
then requires the next collection to publish one intact cycle before
breaking/removing it for cleanup. Both tests use actual verifier assertions
through `_queue_child` and explicitly override child stress to N0. No
ambient-stress inheritance claim is warranted.

The copied negative restores only finalizer MARKED-before-visited-push.
At stage2 the journal allocation fails with that mark unrecorded; the
following live mark treats the instance as already marked, so the immediate
finalizer-recovery assertion must fail. The no-owned-referent fixture
avoids requiring a dangling native access to establish that counterexample.
Compile/import/crash/unfired-injection failures are not credit. Neither
positive nor negative control has run at this source checkpoint.

These small controls do not exercise every scratch-growth location. Before
landing, a useful finite addition is one journal-growth failure AFTER prior
marks exist: more than one initial journal-capacity of otherwise slotted,
independent finalizers, requiring exact next-pass count/once-only recovery.
The source cleanup loop covers that case; a boundary control would verify
it without an allocation matrix or new fixture. Root should choose that
bounded addition and affected suites; it is not a source blocker to the
already proposed small-path build/controls. Partial routed-call OOM still
needs the existing queue/rollback context, not an all-or-nothing assertion.

No further source blocker was found. Preserve the source checkpoint and
negative, confirm actual allocation-stage mapping under the approved
builds, and use the existing mode-aware/auxiliary-aware protocol. No broad
repeat matrix, timing, old held raw-address probe or buffer/scheduler
implementation is authorized by this review.
