# Queue2a: sweep safety, debug output and completion dependencies

Source-only proposal, 2026-10-29. Authority: note13 §3.2–3.5 and the
coordinator's bounded design assignment. Audited runtime:
`boil-pre-i-union@5a45f9bb6a73e7351c861faf05624ae4810f6d87`.
An isolated unbuilt slice1 candidate is now prepared; no native build,
target test or probe was run. Slices2/3 remain unimplemented.
H2 `b3e6847348d` and all frozen products/evidence are untouched. This
supersedes assumptions in `7a-2.md`/`7a-2a.md`, not their historical records.
Independent source review and a separate native grant precede builds/tests.

## Actual state and preserved evidence

| Source at5a45 | Consequence |
| --- | --- |
| `gc_live.c:1505 collect` batches calls/routes/unpins; `:1223 sweep` then allocates first-object order | Revised slice1 keeps this late successful-graph boundary while moving allocation failure before any deallocation; it does not promise pre-batch rollback. |
| `:1139 order_dead_first` returns NULL; sweep falls through to heap order | Native-state dependencies can be destroyed in the wrong order even though object-block frees are deferred. |
| `:1285 debug_write` calls `PySys_WriteStderr`; collectable lines precede sweep | A Python writer can release the shared GIL while the dead snapshot remains in use. Interpreter pins protect interpreter storage, not snapshot contents. |
| DEBUG_STATS follows `free(res.dead)` but precedes publication of raw `garbage.items` to `gc.garbage` | Moving only the collectable print is insufficient: all raw object-bearing collector state must be rooted or retired before a Python write. |
| `:475 run_queue_ex` guards each thread separately, removes/frees its node before invoking Python | Ready FIFO orders invocation, not completion; retained memory does not preserve an open resource. |

Reuse `/tmp/7a-probes/s1.py`, the architecture review §5.2/S1/S3 and the
preserved `boil-7a/build/queue-2a/` evidence; do not rerun its matrices.
The latter manifest binds source052d, probe SHA460a1434 and its original
binary/config identities. Its final resource log records a blocked writer,
closed original fd and lost payload; the retained-raw-object control keeps
that resource open and preserves the payload. The predecessor log records
inner finalization with predecessor B unfinished while unrelated work
progresses. S3 debug-kill segfaults; ASan-kill reports use-after-poison in
collect after the nested legacy-domain collection. Deferred-output controls
and the graph model are historical controls, not current implementation
qualification. The old automatic-classifier/probe hold remains in PROCESS;
this explicit source/design authorization does not authorize rerunning,
migrating or expanding those native probes or raw-address fixtures.

## Slice1 — abort unsafe sweep on late ordering failure

**Revised after independent source HOLD:** keep successful ordering at the
original start-of-sweep boundary, after batching, no-callback weakref
clearing and DEBUG_COLLECTABLE output. The earlier eager-order candidate
is preserved in `boil-queue2a-sweep/build/Queue2a-sweep/source-v1.diff`
SHA7ba64afe and its pre-late checkpoint. Its no-reopening premise was not
established; no native result was claimed. The finite lock audit remains
separate, SHA79cea15a. Do not turn this first safety increment into S3/lock
repair or claim whole-GC safety.

The independently reviewable scope is `gc_live.c` ordering/status/cleanup,
a separate integer-only ordering-allocation hook using the existing
header/module pattern, and existing reader-based semantic tests.

1. Keep weakref/finalizer/destroy batching, owner routing and domain unpin
   in their existing order. After remaining weakrefs/debug output, prepare
   the **remaining** dead first-object order, immediately before calling
   sweep. The preparation must precede `gc->sweeping=1`, exception capture
   and any deallocation. Thus it observes the same post-batching graph as
   successful baseline ordering, instead of relying on a restricted early
   graph surviving possible detachment/mutation.
2. Return explicit success/error with initialized `{indices,count}`; zero
   entries succeeds without allocation. Check sentinel sizes, capacity
   doubling and byte products. Sweep only consumes a successful order;
   no ordering allocation and no heap-order fallback remain inside it.
   Existing ordinary/type-last phases and cycle ordering stay unchanged.
3. Any late preparation failure joins the same rollback as earlier node/
   stack failure: undo DEAD once, count all kept snapshot blocks in retained
   live bytes/heap size, clear local `garbage.n`, report first-pass collected/
   freed zero, free temporary ordering storage and reclaim no blocks.
   Pins were already released once and are not released again.
4. This is an honest **partial-delivery** contract: previously routed calls
   stay routed; already cleared weakrefs and delivered/notified state are
   not restored. Calls can run afterward and a manual collection can then
   execute its existing second pass. Do not promise an empty batch, intact
   weakrefs or that the whole public `gc.collect()` call frees nothing.
   Preserve the interrupted exception and all existing delivery semantics.

The former early-order proof required strong-reachability-closed
resurrection (whole SCC removed or unchanged), no newly introduced flagged
edges and a non-reopening domain. Batching's field writes themselves only
change flags/metadata/weak links, but the weakref registry's default mutex
can detach; DEBUG_COLLECTABLE can run Python. The new increment does not
use that proof. `order_by_edges` has private seen storage; separate existing
finalizer/cycle MARKED-OOM debt is recorded, not silently fixed here.

The no-sweep control has **no finalizers, callbacks or hooks**, so delivery
cannot trigger a second pass that hides a failed first pass. It fails all
ten allocation sites of a four-node flagged graph (index storage, edge
array growth, eight graph scratch arrays), requires the hook to fire,
first-pass count/freed bytes to stay zero and retained bytes not to shrink,
then requires ordinary successful reclamation. Reader objects own separate
native StatefulReferents; their flagged edges are Python attributes. This
lets copied heap fallback fail the named no-sweep count assertion rather
than deliberately corrupting native state.

A separate partial-delivery control queues both a weakref callback and a
finalizer, while an independent native reader chain forces late ordering
allocation. The finalizer observes first-pass freed bytes unchanged;
calls run exactly once and native referent-order guards protect eventual
reclamation, including an allowed automatic/manual second pass. The
resurrection/SCC test now checks ordinary post-resurrection ordering and
lifetime, not an eager prepared-subgraph proof. Preserve existing native
flagged-chain, ordinary referent, cycle and pyexpat checks.

## Slice2 — no Python writer until all collector snapshots are safe

Land separately; no scheduler changes. Preserve current bounded textual
format (the1000-byte local formatter), type/address-only cycle lines and
DEBUG_* semantics. Do not save PyObject pointers for later formatting.

During the unsafe interval, format complete lines into collector-owned
libc storage: a local buffer containing bounded NUL-terminated records,
checked `used+length`/growth, no Python references. Format uncollectable
lines while their objects are still protected. Use individual records at
flush, rather than one huge `%s` that PySys_WriteStderr itself could truncate.
No Python allocation, repr or writer runs while that buffer is filled.

On buffer OOM, write its already formatted prefix and the failed line to
**C stderr**, free the buffer, and select C stderr for the remaining lines
of that collection. This existing no-Python destination preserves line
order without creating a new collecting/OOM path. A blocking C write does
not detach/release the shared GIL. It may increase pause time; do not claim
bounded output latency. Overflow follows the same policy, not silent loss.

Before the first possible Python call after sweep/rollback: all `res.dead`
entries and prepared-order/dependency scratch must be consumed/retired.
The raw garbage array needs explicit protection while it is still read:
a stable existing `_PyGCRoot` span over `garbage.items[0:garbage.n]`, installed
before possible reentry, kept through attempts to append to `gc.garbage`.
On append OOM, clear the error as today, stop appending, and retire the
remaining raw array before releasing that span. Repeated collection may
then reclaim unappended objects; no later diagnostic may dereference them.
No fallible public root allocation or conservative malloc scanning claim.

Transfer the finished bytes to a pending-output list in the live state
without allocation. Drain only after collect returns, at the queue-drain
boundary where Python is permitted; inline/no-Python paths use C stderr.
Reset the collection entry as collect_pass already does before running
Python, then detach the pending list **before** the first Python write. The detached
buffer is drain-local and contains bytes only: a reentrant writer or another
thread's collection can append a new list without reallocating/freeing it.
Free detached storage on all returns; do not dereference a saved live-state
pointer after Python reentry. `_PyGC_Fini` consumes any still-pending bytes
through C stderr before freeing live state. Output failure does not alter
the interrupted exception or finalizer delivery state. No global writer
lock is held across Python; an independently collected batch can drain.

Validate existing DEBUG_UNCOLLECTABLE/SAVEALL output, collectable/stats
output, reentrant writer, allocation-failure C output and pending-output
close. Reuse the existing S3 scheduling scenario after a reviewed grant,
requiring the writer be entered only after the unsafe snapshot is gone;
its historic ASan failure stays failed. The deferred control is not a new
native baseline. No broad matrix rerun is implied.

## Slice3 — one graph for same- and cross-owner completion

Future slice3 still needs its own pre-batch graph preparation boundary;
revised slice1 does not supply one. It also needs slice2 and the reviewed
non-detaching snapshot contract. Preserve note13's rule: only FINALIZE/LEGACY calls are
predecessors; FINALIZE/LEGACY/DESTROY calls can wait. CALLBACK calls neither
wait nor are waited for. An edge means **completion**, including weakref
clearing and unraisable-error reporting belonging to the predecessor's
whole dispatch, before the successor begins.

### Construction, deduplication and checked bound

Let D be the initial dead-object count. Make an address-sorted membership
index for precisely those D objects. Do not use current DEAD bits as
membership after resurrection. Enumerate potential `(dead-index,kind)` call
slots from the initial FINALIZE/DEL/HOOK kinds, with one slot per kind;
K<=3D, checked while counting. These are storage bounds, not claims that
all those calls will actually be queued. In particular, FINALIZE and LEGACY
may both name one object; owner count says nothing about required state.

Potential FINALIZE slots must reuse `finalizer_eligible` exactly, excluding
already-finalized objects and `_PyGen_FinalizerIsNoop` objects. DEL slots
exclude already delivered legacy calls; HOOK candidates use mark's existing
hook eligibility. Do not undo a285's gain by walking closed/no-op generators
as new finalizer roots. Such objects remain transit nodes when a genuinely
eligible predecessor reaches them.

**Revised direction, still awaiting independent review:** use standard
sparse deduplicated adjacency rather than the initial full K² bit matrix.
Before the first callback node, allocate the D-object membership index,
D-entry discovered-object worklist/visited scratch and K descriptors.
For each eligible predecessor slot P, walk initial-dead membership using
`_PyGC_TraceTraverse`. Mark each object on discovery; enumerate its finite
potential waiter slots once. Exclude the identical call slot, but retain
same-object different-kind reachability. Multiple fields/paths cannot
multiply a pair. The worklist holds each discovered object once and also
allows clearing only its visited entries between rows; do not clear D
entries for every independent finalizer.

First count each row's distinct potential successors and compute checked
prefix sums. Then allocate exactly A successor indices plus K+1 offsets
and fill them in a second identical traversal, still before any batching.
There is no Python/mutator phase between count and fill. Verify each fill
count matches its reserved row. Check K/count conversions, K+1, every
prefix sum and all element-size products before allocation; A=0 needs no
edge array. Traversal error/OOM/overflow takes empty-batch rollback. No
owner-count or arbitrary size cutoff, incomplete-row success or fallback.

Each potential slot accepts at most one successful node. Existing batching
and legacy/SAVEALL decisions stay authoritative. After batching stops,
also on later node OOM, rank actual graph-bound calls in batch order and
compact the reserved rows in place to active eligible P/R with
`rank(P)<rank(R)`; count each retained incoming edge once. Clear inactive
node/object pointers and publish the complete graph before unpinning,
Python or sweep. Callbacks have no slots, ranks or graph references.

Sparse storage is O(D+K+A); A can still be quadratic for a dense reachable
call graph, and must be measured rather than hidden. Two-pass traversal
cost is O(sum of predecessor reachable vertices/references * log D + A),
plus O(K+A) masking/accounting. Independent finalizers require no quadratic
edge storage or K² scan. This replaces the original matrix proposal; its
cost and close/fork proofs still need review before slice3 code. The
withdrawn owner shortcut and O(K*D) claim remain invalid.

### Publication, roots, locking and completion

Use READY, WAITING, RUNNING, DONE node states. Add owner waiting and running
lists to GCState; extend `_PyGC_LiveVisitRoots` to visit op/callback in all
three live states. Keep a node allocated while dispatch runs: move it from
ready to the running list **before** invoking Python; current copy-and-free
before dispatch cannot support this lifetime. Intrusive links permit
removal of the particular completing node without a last-running shortcut.
No object pointers remain solely in the libc graph after publication.

Zero-pending nodes enter their owner's ready queue in batch order;
positive-pending nodes enter its waiting list. `queue_ready` describes only
runnable nodes, so an idle dependency does not spin the eval breaker.
Each graph counts its actual FINALIZE/LEGACY/DESTROY nodes not yet
done/discarded. Ordinary callback nodes hold no graph reference; inactive
slots do not contribute. Nodes/descriptors retain the graph until this
count reaches zero; completion never follows a freed node or adjacency array.

All graph/queue transitions take the **shared domain GIL**, as existing
owner routing does. No GIL is held across a *wait* for another owner: the
scheduler does not synchronously wait. Distinct own-GIL domains cannot
share a dependency graph under note13's heap-domain rule. Interpreter
close/remap additionally uses the existing HEAD_LOCK ordering; never call
Python or wait for a finalizer while holding that lock. No new atomic
last-call protocol or lock held across callbacks is proposed.

After dispatch and its error reporting finish, remove P from running;
for each retained P→R edge, consume it exactly once and decrement R.pending
once. If it becomes zero, move R from waiting to the current owner's ready
tail and publish ready. Unrelated zero-pending calls remain runnable while
several P calls release the GIL. Free P only after all releases, then drop
its graph-node count. A waiter cannot run when just one of several
predecessors completes. A node that is skipped by an existing once-only
finalizer check still completes its dispatch; an exception is not a
lost completion acknowledgement.

Cycles in the object graph receive the existing arbitrary batch order;
rank orients every wait backwards and prevents wait cycles. Do not make
an SCC wait for its own completion. Older queued/waiting/running nodes
root their graphs, so a later mark cannot call an object still reachable
from them dead. New batches have separate metadata and no implicit global
batch barrier. Mutations that remove snapshot edges do not cancel waits;
newly retained referents remain traced through live roots. This is the
specified snapshot completion contract, not dynamic dependency tracking.

### Close/fork boundary that must be settled before slice3 code

Current `live_close` only drains/transfers/discards ready nodes. It cannot
be left unchanged: a waiter can outlive its interpreter's live state.
The minimal normal shared-owner close must transfer waiting **and** ready
nodes to main, update each node's owner before freeing the old GCState,
and preserve pending edges/counts. It must not drain a waiting node early
or synchronously wait on another owner. An isolated never-opened domain
can discard its whole unexecuted graph, clearing node pointers/refcounts
without treating cancellation as permission to run dependent callbacks.

Two exact proofs remain blocking for slice3: (a) close/deletion cannot
free an interpreter while another thread still owns a RUNNING dispatch,
or a specified retained-owner handoff must cover it; (b) fork disposal
must account for RUNNING nodes of vanished threads and all waiting roots,
without treating an interrupted callback as completed. Current per-thread
`running_queue` assertion and ready-only disposal do not establish either.
Do not silently import all Queue2c retirement/adoption work, manufacture a
completion acknowledgement, add a leaking fallback or claim graph safety
only on the normal path. Coordinator must choose/review this finite
lifecycle extension or order that Queue2c prerequisite ahead of slice3.
Slices1 and2 remain independently implementable and landable meanwhile.

## Smallest review and validation path

First review/implement slice1, then slice2, preserving independent source
and causal controls; do not make the first safety repair wait for the
scheduler. Slice3 starts only after its storage/proof and explicit close/
fork ruling clear independent review. Its semantic controls are the
preserved one-owner writer/resource loss, two unfinished predecessors of
one waiter with reverse completion order, cross-owner writer/raw owner,
multiple paths to deduplicate, same-object FINALIZE→LEGACY, cycles and an
unrelated call that completes while predecessors are blocked. Copied
missing-edge and single-predecessor accounting negatives must fail named
ordering/resource oracles; no raw-address fixture is needed.

Use affected existing finalizer/weakref/resurrection/legacy/queue tests
first; then the relevant GC, IO, threading, pyexpat/XML and finalization
modules under ordinary and actual debug-kill, with ASan only for the
snapshot/native-resource lifetime hazard. Record actual modes, counts,
root fallback and cleanup. Broader integration gates follow the actual
union, not a repeated historical probe matrix. Measure graph bytes,
allocation failures, pending/ready/running high water, retained graphs/RSS,
queue latency and unrelated progress. Correctness and no lost/duplicate
call are gates; memory/CPU cost can reject this representation before
performance claims. No current result establishes Queue2a correctness.
