# Queue2a completion scheduling: finite implementation readiness

Source-only, 2026-10-30. Runtime audited: frozen
`boil-pre-i-union@5a45f9bb6a73e7351c861faf05624ae4810f6d87`.
The four-file late sweep-OOM V2 (`source-v2.diff` SHA bc42c1ec) is frozen
and handed to the independent native operator. This note changes none of
it. Historical S1/resource and multiple-predecessor evidence remains as
indexed in primary `7a-2a-safety-design.md`; no probe was rerun.

**Conclusion:** the sparse per-call graph and normal dispatch protocol can
be specified locally. Activation is blocked by two finite contracts: a
stable pre-batch graph snapshot (S3 owner), and close/fork handling of a
RUNNING predecessor. Ready-only transfer or a thread-list-empty assertion
cannot supply either proof. Do not implement a same-owner-only workaround:
several threads of that owner and its shutdown have the same problem.

## Actual interfaces and smallest coherent runtime change

| Source anchor at5a45 | Existing behavior | Required change/contract |
| --- | --- | --- |
| `Python/gc_live.c:84 _PyGCLiveCall`, `:309 call_new` | One next link; op/callback/kind; allocation per queued call | Stable node through completion; explicit owner, state, pending count and graph-slot identity; list removal must identify the actual node. |
| `:743 finalizer_eligible`, `:826 handle_finalizers`, `:933 queue_destroy_hooks` | Actual eligibility, legacy-cycle/SAVEALL and resurrection decisions | Reuse these decisions; potential graph slots are a superset, never a replacement eligibility table. Closed/no-op and finalized generators get no FINALIZE slot but remain reachability transit. |
| `:1424 route_calls`, `:1456 unpin_domain` | Ready FIFO splice then domain unpin | Attach all active graph nodes and complete counts before any publication/unpin; no graph allocation during routing or completion. Preserve the existing partial batch on node OOM. |
| `:277 _PyGC_LiveVisitRoots` | Visits ready nodes only | Visit ready, waiting and running nodes, including callbacks, until retirement. No unvisited PyObject pointers retained solely in libc graph storage. |
| `:475 run_queue_ex` | Removes node, copies it to C local, frees it before Python | READY→RUNNING under domain GIL, retain/root node through dispatch and error reporting; complete this exact node afterward. Per-thread recursion guard stays. |
| `Include/internal/pycore_interp_structs.h:255` | Ready head/tail, phase, ready flag, routing pins | Add waiting/running lists with allocation-free links; ready flag means runnable work only. Routing pins are not lifetime pins for a callback already running. |
| `Include/internal/pycore_tstate.h:77` | Boolean running_queue | A surviving/removed thread's active node must be identifiable for shutdown/fork; boolean alone cannot reconcile graph nodes. Exact representation is a lifecycle implementation decision. |

After the two contracts close, the smallest behavioral patch is ONE shared
scheduling mechanism for all call owners: graph preparation/publication,
three live lists/root visibility, exact completion release and lifecycle
entry/exit hooks. It need not change finalizer ordering, eligibility,
legacy/resurrection policy, sweep order or watcher metadata. Do not split
activation into “WAITING first, cleanup later”; that loses roots or leaves
permanent waits. Separate lifecycle prerequisite commits can land before
scheduler activation without changing ordinary finalizer parallelism.

## Graph and dispatch contract to implement

Let D be the initial dead-object membership and K the potential
FINALIZE/LEGACY/DESTROY slots, at most three per D object. Callbacks have no
slots. Enumerate through existing death-kind information plus authoritative
eligibility; no new manual object-kind table. Finalizers and legacy calls
are predecessors; all three slot kinds can be waiters.

Before the first call node is batched, allocate checked membership,
visited/worklist and slot storage. For each potential predecessor, walk
strong references through initial D membership, discovering each object
once per row. Each encountered target slot contributes at most one pair;
self-slot is excluded, same-object different-kind is allowed. Count rows,
checked-prefix-sum, allocate A exact targets and K+1 offsets, then fill in a
second identical traversal. Clear only visited entries between rows.
Validate every count/fill match and every integer/product bound. No K²
matrix or K² scan for independent calls; O(D+K+A) storage, potentially dense
A acknowledged. Fail before batching on incomplete traversal/allocation.

Representation is not approved for implementation merely because it is
sparse: a chain of K finalizers still has O(K²) reachable-call pairs. Before
committing storage, compare standard object-graph SCC condensation plus
completion tokens/DAG, potentially O(D+E+K), against this closure. Its proof
must preserve FINALIZE/LEGACY-only predecessor semantics, DESTROY-only
waiters, same-object calls, in-SCC batch order and unrelated progress. A
DESTROY cannot become a predecessor just because it lies on an object DAG
path. This is a bounded representation-review decision, not a second
algorithm specified or cleared here; no cutoff or dense fallback.

The count/fill and frozen-reachability interval require S3's reviewed
no-Python/no-detach contract, including library locks/allocator callbacks.
Slice1's late-order rollback does NOT establish it. If that contract cannot
be proved, this two-pass construction is not ready: do not assume arbitrary
post-batching graphs fit storage reserved against an earlier graph.

When actual batching stops (including node OOM), rank only actual graph
nodes in existing batch order. Compact to active pairs P→R with
rank(P)<rank(R); exclude inactive slots, callbacks and disallowed kinds.
Count EACH distinct retained predecessor of R. Publish complete graph and
all roots before unpinning. No graph-bound node may run on partial counts.
Object cycles use existing arbitrary batch order, with backwards-rank
waits; no self-wait/SCC completion barrier. Graph storage lives until all
actual graph nodes are terminal; inactive object pointers are cleared.

| Transition | Atomic under the shared domain GIL |
| --- | --- |
| New→READY/WAITING | Zero pending goes ready in batch order; positive pending goes waiting. |
| READY→RUNNING | Remove actual ready node; root it on owner's running list before Python; update ready flag from ready list alone. |
| RUNNING→DONE | After dispatch, weakref callback clearing and unraisable reporting return, consume each outgoing edge once and decrement each target once. Move a target to its CURRENT owner's ready tail only at zero. Then unlink/free predecessor and drop graph lifetime count. |

A skipped once-only finalizer or reported exception still completes its
normal dispatch. Interruption/abandonment at shutdown or fork does not.
If P1 and P2 are unfinished, finishing P2 cannot release R while P1 remains.
An unrelated zero-pending U remains runnable even on the same owner; drains
never synchronously wait for another owner. No HEAD_LOCK across Python or
waiting. Separate own-GIL domains cannot share the heap/graph; this does
not add cross-GIL atomics. Owner remap must precede freeing old GCState;
edges identify stable graph slots/nodes, not stale interpreter addresses.
Older queued/running roots protect their reachable graphs from later dead
sets; separate batches gain no implicit global barrier. This is snapshot
completion ordering, not dynamic tracking of mutations during callbacks.

## Close/fork: actual blockers, not accounting details

| Actual source | Why the current condition is insufficient |
| --- | --- |
| `gc_live.c:1936 live_close` | Checks only current thread's running_queue; drains only ready, then transfers/discards only ready and frees live state in `_PyGC_Fini`. |
| `pylifecycle.c:2361 _Py_Finalize`, `:2724 Py_EndInterpreter` | Both mark finalizing, remove other tstates and prevent them taking the GIL. A finalizer detached in a C call can be left permanently RUNNING. Py_EndInterpreter's current-frame check does not exclude another thread's dispatch. |
| `pystate.c:1853 RemoveExcept`, `:1888 DeleteList` | Unlinking is not completion. Normal path keeps removed thread states (`gc_live.c:1797`), then clears them; OS threads may still hold them. Keeping frames roots memory, not completion/resource validity. |
| `posixmodule.c:719 BeforeFork`, `:759 AfterFork_Child` | Stopping the world does not finish detached callbacks. Child removes/frees vanished tstates before disposing other interpreters. The surviving thread may itself be inside a finalizer. |
| `pystate.c:931 DeleteExceptMain` | Existing Clear-under-head-lock path is not note13 §5.5's planned allocation-free/no-Python disposal. Adding waiting-list splicing cannot repair that path. |

Note13 §5.1 requires drain through empty READY AND WAITING before close.
The previous safety draft's proposed transfer of an opened owner's waiters
to main would change that contract; **it is not an approved resolution**.
A close blocked on a foreign predecessor cannot spin while holding the
shared GIL; the predecessor needs to resume. More fundamentally, an
already-stranded predecessor in the closing owner can never resume.

Recommended prerequisite direction, requiring coordinator/lifecycle review:
quiesce active dispatches BEFORE the finalizing state prevents GIL reentry
and BEFORE RemoveExcept/Clear. A closing-owner admission gate and a wait that
releases GIL/locks must preserve incoming-node rooting, allow other owners'
unrelated progress, and forbid reentrant self-close from waiting on itself.
Quiescence alone does not resolve a WAITING node whose predecessor belongs
to an idle foreign owner: specify cooperative progress/close ordering or
explicitly revise §5.1 and adopt its nodes/roots/metadata safely. An
application callback may never return, so this contract must acknowledge
shutdown can wait; do not manufacture completion or an unbounded leak.

### Exact cooperative barrier placement

`make_pre_finalization_calls` (`pylifecycle.c:2254`) is a suitable existing
loop to extend, not a completed barrier to wait after. It runs thread
shutdown, pending calls, atexit and subinterpreter cleanup with the world
started. Then it locks `pending.mutex` (:2327), stops the world (:2328),
takes HEAD (:2330), checks readiness, and returns with the world stopped
and pending mutex held. A callback cannot finish by reacquiring the GIL
while the finalizer waits in that final locked region.

Proposed minimal close protocol, subject to lifecycle review:

1. At `Py_EndInterpreter` / `_Py_Finalize` entry, reject a caller already
   dispatching a queued call BEFORE changing finalization state. The
   existing EndInterpreter current-frame fatal checks do not prove this
   for all C callbacks. Extending that fatal/reentrant-call contract needs
   explicit root acceptance; never self-wait.
2. In the world-started pre-finalization loop, publish an OWNER-local
   admission gate under domain GIL. Other threads of that owner stop
   starting queue dispatches; the closing thread can continue draining.
   Existing RUNNING callbacks finish normally. Other owners' unrelated
   ready callbacks remain parallel. This is shutdown admission, not a
   single-drainer rule for ordinary operation.
3. If RUNNING is nonempty, publish a dedicated wait notification before
   detaching; wait with neither GIL, pending mutex nor HEAD held. Completion
   signals under the existing domain-GIL transition protocol; recheck
   after attachment, without lost wakeups. Do not reuse the routing-pin
   event/count: its lifetime and condition are different. No timeout,
   early-success or synthetic completion. Shutdown may now wait for a
   blocked daemon finalizer; that semantic change must be documented.
4. Recheck RUNNING alongside the final `should_continue` predicates in
   the existing world-stopped/head-locked readiness check (:2335). On
   work/race, follow the existing StartTheWorld + pending-unlock loop,
   then wait outside locks. Leave the admission gate set through close
   so there is no zero-count→SetFinalizing race. Atexit/pending callbacks
   and subinterpreter finalization can themselves run/queue calls, so a
   one-shot check before the whole function is insufficient.
5. Only then set the attach-blocking finalizing state and remove/clear
   other tstates. Later shutdown collections and the closing thread's
   dispatches use existing close rules; new arrivals remain rooted and
   ready/WAITING correctly. Reconcile open/failure/reset lifecycle for
   the new gate, including initialization failure, not just success.

The preexisting `interp->finalizing=1` at API entry precedes this function;
it is distinct from `_PyInterpreterState_SetFinalizing`, which makes other
threads unable to reattach. The proposed barrier must permit completion
in that existing pre-finalization interval. Moving either marker is not
assumed. The exact queue gate/wakeup representation is implementation
work, not an already-existing API. Callback code can wait on the shutdown
caller, forming an application deadlock; the honest contract waits, rather
than timeout and run dependent destructors against unfinished resources.

This addresses RUNNING stranding. It still does not solve §5.1's foreign
WAITING drain, nor retire/adopt owner metadata. The smallest full lifecycle
prerequisite must settle both; no successful zero-RUNNING check authorizes
freeing a WAITING owner.

### Fork policy proposed for explicit ruling

Fork is different: a vanished thread's RUNNING call cannot return. Prefer
explicit child cancellation/disposal, rather than making fork wait for all
callbacks or pretending they completed. Cancel the vanished RUNNING nodes
AND their transitive dependent closure. Remove cancelled READY/WAITING
nodes without invoking Python or releasing their dependents to ready;
consume graph lifetime/adjacency ownership as cancellation, never success.
A dependent reachable from a cancelled predecessor remains cancelled even
if its other predecessors completed. Unrelated nodes stay runnable under
their retained/adopted owner's normal rules. A surviving thread's own
RUNNING node remains RUNNING and may complete after returning from fork;
do not clear its per-thread dispatch guard or free its node. Whether that
surviving node can itself be in the cancelled closure must be resolved:
it cannot have started while one of its required predecessors was
unfinished, so valid pre-fork scheduler invariants should exclude it.

The cancellation walk requires already-reserved per-graph work storage,
checked once-only state transitions and no allocation or callback in the
child critical interval. Successor rows provide the traversal; each node
needs enough state to visit once, without double-decrement/free. Preserve
other predecessors' outgoing rows until their own retirement so later
completion recognizes a cancelled target rather than a freed pointer.
Keep terminal slot tombstones/index identities until graph retirement; do
not retain an edge to a freed node allocation.
Root/live-node lifetime must precede every frees/transfers; do not briefly
expose objects solely in cancelled libc descriptors. This finite contract
still needs a proof and code review, not a claim that child cleanup works.

Install child graph reconciliation before `_PyThreadState_DeleteList(...,
is_after_fork=1)` (:807 posixmodule), which currently clears/frees vanished
tstates and may call destructors. It must account for other interpreters
before `DeleteExceptMain`; that existing path is not the planned no-Python
disposal. HEAD/pending/import locks and world-stop are inherited/recreated
in stages: graph work cannot call ordinary close/drain from under them.
Tie it to note13 §5.5's dedicated disposal/adoption rather than add another
partial Clear path. This is an observable child cancellation policy:
queued dependent finalizers may never run, and external resources may
remain until process cleanup. Root must accept/document that fork-specific
limit. It is not permission to cancel blocked calls at ordinary shutdown.

Alternative: pre-fork admission/quiescence after before-fork Python hooks
and before import/HEAD/world-stop locks, accounting for the surviving
call and restoring admission in parent/child. It can block fork indefinitely
on an unrelated daemon callback and needs a broader admission protocol;
there is no such guarantee in current `PyOS_BeforeFork`. Do not silently
choose it merely to avoid specifying child cancellation.

## Finite next assignments and validation, without new probes

1. S3 owner supplies the exact stable snapshot interval/interface; this
   note adds no output-buffer or weakref-lock edits to that assignment.
2. Coordinator chooses/reviews normal-close and fork policies above. A
   finite lifecycle prerequisite owns `Python/pylifecycle.c`,
   `Python/pystate.c`, `Modules/posixmodule.c`, queue helpers in
   `Python/gc_live.c` and only needed internal declarations/tstate fields.
   Watcher retirement/adoption interfaces from note13 remain explicit;
   no automatic blanket dependency on every other GC project.
3. Implement scheduler in gc_live + GCState, any justified tstate field,
   and pure-Python `test_gc`/existing interpreter-lifecycle tests. No raw
   address fixture/new public API/effect table or old-probe migration.

Meaningful controls: two blocked predecessors of one waiter, release them
in reverse order, require waiter after BOTH and unrelated U before either;
duplicate paths yield one dependency; cycle obeys batch order; copied
skip-one-predecessor/early-completion defects fail named order assertions.
Reuse historical S1 as reproduction evidence, not new qualification. Cover
partial node OOM after complete graph reservation, eligibility/transit,
callback exclusion, roots while WAITING/RUNNING and actual-owner dispatch.
Lifecycle selection must exercise the CHOSEN close/fork policy, including
a blocked detached dispatch and fork from a surviving dispatch, not merely
queue empty after all application threads voluntarily joined. Stop before
native execution until concrete source/selection review and scoped grant.
