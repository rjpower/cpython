# Collector work-stack OOM: bounded repair plan

Source-only,2026-10-30. Audited frozen5a45 `Python/gc_live.c:573–958`.
V2/S3 operator trees/products remain untouched. No branch, native change,
new hook or target has been created/executed for this increment.

## Exact defect and small source boundary

`objstack_push` doubles capacity/products without checked arithmetic,
returns void, and can still append on subsequent calls after oom. It is
shared by temporary traversal, output/garbage lists and resurrection.

`cycle_visit` sets MARKED before recording the object in seen. Allocation
failure leaves a mark that `in_dead_cycle` cannot clear from its journal.
The latter reports OOM as a cycle; handle_finalizers does not immediately
abort on that failed traversal. `finalizer_order` similarly sets MARKED
before visited push, checks only stack.oom in its DFS loop, and can continue
traversing/marking when visited/order storage has already failed.

At the next mark, `gc_trace.c:1881` sees an existing MARKED object and
skips the normal traversal; `:2149` later keeps/clears its flag. This is a
real marking-state hazard, not only a missed ordering opportunity. The
source-only controls below use no independently dying referent so a copied
bad version can demonstrate recovery failure without intentional dangling
native state.

Proposed exact four files:

- `Python/gc_live.c`: checked/status-returning work-stack push, sticky OOM,
  successful-journal-before-MARKED ordering, immediate traversal abort and
  complete mark cleanup; adjust all existing work-stack callers to their
  existing common failure path.
- `Include/internal/pycore_gc.h` + `Modules/_testinternalcapi.c`: one narrow
  integer nth-work-stack-growth failure seam, separate from existing
  node/order/debug hooks. Zero disables/returns countdown, negatives reject.
- `Lib/test/test_gc.py`: pure-Python subprocess recovery controls using
  ordinary __del__ and existing `_testcapi.with_tp_del`; no raw addresses,
  new C object fixture or callback-contract expansion.

No gc_trace/root algorithm, queue scheduling, S3 lock/output, sweep ordering,
owner, flag layout or buffer-consumer change belongs here. A new isolated
managed branch/source base must be chosen by root before editing.

## Implementation contract for source review

Push returns failure immediately once oom is set. Check n+1, capacity
doubling and pointer-array byte products against the signed storage bound;
a failed reserve changes neither n nor storage/capacity. Publish a stack
entry only after successful allocation. No Python exception/error object
is created in collector scratch allocation.

For cycle traversal, first append op to the seen journal; only on success
set MARKED. Return nonzero from visitor to stop traversal on failure.
`in_dead_cycle` distinguishes error from no-cycle/cycle, clears every
recorded MARKED on every path and leaves no unrecorded mark. Its sole caller
must branch on error before inserting op into garbage or legacy lists.
A failed traversal must not masquerade as a conservative successful cycle.

For finalizer DFS, first record op in visited; only then set MARKED and
schedule its completion marker/children. Failure in visited, stack, order
or traversal stops the entire walk, including the outer-root loop. One
cleanup clears MARKED for all visited entries and frees both private
stacks. No output reversal or ordering success after any failed step.
Preserve the existing no-op-generator eligibility/transit rules.

The same push primitive serves resurrection: enqueue successfully before
clearing DEAD, stop processing immediately on OOM/traverse failure, and
propagate status to current rollback. Existing callers already intend to
undo DEAD/free nothing on failure; preserve already queued calls/pins and
late slice1 semantics. Keep this mechanical closure small: do not invent a
new full-graph journal, retry allocator or separate traversal framework.
Every void/ignored-status caller must be inspected rather than relying on
only the two MARKED sites. Source diff must list which callsites are
changed and prove no path sweeps after an incomplete resurrection walk.

## Meaningful recovery controls, before any native grant

One modern-finalizer fixture uses a module-rooted class with `__slots__=()`
and a __del__ recording a small token. Its unreachable instance owns no
independently dying object graph. Run separate child cases for each actual
small-path work growth (root stack, visited journal and emitted order).
Require the hook fires, failed pass delivers no finalizer, disables hook,
then the VERY NEXT ordinary collection delivers exactly one finalizer.
Repeat an unarmed collection to reject duplicate delivery. Do not accept
“eventually after an arbitrary number of retries.” Actual allocation-stage
mapping must be confirmed against final source, not guessed from names.

A legacy control uses `@with_tp_del` with only a self-link slot; its type
and method remain module-rooted. Fail the cycle seen allocation, require
rollback/no premature gc.garbage publication, then disable hook and require
the VERY NEXT collection puts that same class's instance in gc.garbage.
Break its self-link/remove it only after checking the oracle; clean up
normally. No independently dying referent is needed. Existing legacy-cycle,
resurrection/order/no-op-generator tests provide affected-suite context.

Copied negative restores mark-before-journal for the visited failure path,
retaining the new growth hook and remaining error handling. Use only the
small no-owned-referents modern fixture: stale MARKED keeps its instance
for the next heap pass, so the named immediate-recovery/finalizer-count
assertion must fail. A signal, crash, startup/import/compiler failure or
unfired hook is not the intended result. A separate cycle negative can be
proposed only if useful; one named recovery failure suffices initially.

Partial-success failure points beyond these small paths (stack growth after
prior marks/output, resurrection work growth, garbage append) need a finite
selection after actual source review. Do not claim the tiny controls cover
every scratch allocation or arbitrary traverse callback. Reuse existing
build/worker/observer/root validation after explicit grants; no new runner,
old full-matrix replay or unsafe probe follows from this source plan.
