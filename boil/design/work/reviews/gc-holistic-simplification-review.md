# GC simplification: a useful collector before a complete lifecycle system

Source/design review, 2026-10-30. No implementation or execution authorized by
this note. It challenges the earlier completion, retirement and native-cleanup
proposals; those remain historical designs, not requirements accepted here.

## Recommendation

Ship a deliberately bounded first configuration: **one interpreter, a GIL,
nonmoving full-heap mark/sweep, the existing hybrid roots, and one automatic
callback drainer**. Keep the current safety repairs. Defer cross-interpreter
heap sharing, late teardown callbacks and callback-bearing multithreaded fork.
Do not build adoption, metadata revocation, borrow leases or a completion DAG to
support those features before this configuration is useful.

The hard remaining normal-runtime issue is native resource release. My smallest
first support boundary would temporarily reject Python-defined buffer-protocol
acquisitions and require ported native exporters' release callbacks to be
noncollecting, non-Python and nonblocking. That is a substantive compatibility
choice, not a finding that current buffer code is safe. It needs coordinator/user
agreement before implementation. If Python buffer callbacks must work now, keep
their safety work on the critical path, with a finite design review of existing
clear/release mechanisms before adding a public type capability.

Yes: my earlier proposals became overengineered for this first supported
configuration. They addressed real counterexamples, but combined useful runtime
collection with stronger concurrency, owner retirement and embedding guarantees.
The cheaper correction is to narrow the guarantee, not implement every proposed
mechanism. This note does not authorize dropping an active safety gate or silently
changing note 11/13's contracts.

## What the evidence establishes

The primary runtime already has the proposed basic collector. `Python/gc_live.c`
marks a full domain, preserves queued objects and their referents, processes
weakrefs/finalization/resurrection, and sweeps without reference counts. Note 11's
precise interpreter/value roots coexist with conservative opaque C-frame roots.
Changing this to another collector is a restart, not a simplification.

The landed `5a45f9bb6a7` composition has seven-profile qualification and closure.
The distinct `168f30a96ff` safety union adds late ordering-allocation rollback,
snapshot/debug-output safety and journal-before-MARKED OOM cleanup. Its bounded
controls are complete; its broad qualification is ongoing. These statuses must
not be merged into an all-GC-safe claim. Owner metadata is another separately
reviewed candidate, not evidence for lifecycle adoption.

Measured four-row profiling attributed about 54/47/31 percent of the slow
coroutine/comprehension/generator runs to collection, and none to collection in
the Richards control. These are samples from the documented diagnostic cohort.
They justify working on shared collection costs; they do not prove old-object
rescanning is most of that work, that generations remove it, or that this plan
reaches 2×. Many candidates were collected: death bookkeeping itself matters.

## Guarantees needed first

| Required during supported execution | Deliberately weaker first guarantee |
| --- | --- |
| Never reclaim reachable objects, including detached/opaque C state and running callbacks | No promise of prompt finalization or prompt release of unreachable cycles |
| No Python, replaceable allocator hook or detachment over an unrooted dead snapshot | User finalizers may block and may have ordinary program-lock deadlocks |
| OOM leaves marks, DEAD state, accounting and queued work coherent | No general completion-order promise across arbitrary finalizer kinds |
| A release callback sees valid acquisition metadata exactly once | No automatic cleanup of unsupported Python-defined buffer acquisitions |
| Queue and running-call roots survive nested collection and exceptions | Other threads' `gc.collect()` may return while automatic callbacks remain pending |
| An enabled API has a real safe implementation | Unsupported interpreter/fork/embedding modes fail at admission, rather than run a partial implementation |

Python already allows delayed collection and recommends explicit resource close.
Its documented `__del__` contract warns of arbitrary-thread invocation and lock
deadlocks. This is precedent for a modest finalizer contract, not permission for
native memory corruption. [Python data model](https://docs.python.org/3/reference/datamodel.html#object.__del__)

## Keep the collector; reduce delivery policy

`run_queue_ex()` currently guards recursion per thread. It pops a node into an
opaque C local, then may release the GIL inside the callback. Another thread can
drain the next node. The preserved S1 evidence demonstrates invocation order is
not completion order. A dependency graph is one solution, but it is unnecessary
if parallel automatic callback execution is not a first-release guarantee.

Use one interpreter-wide drain claim, acquired/released under the GIL. Keep the
running call rooted across its entire callback/error-reporting lifetime; retain
all queued object closures. Release every queue/registry lock before invoking
code. A thread encountering an existing drainer enqueues and returns; it must
not wait for the drainer. A callback's nested collection similarly marks and
enqueues without recursive delivery. Otherwise a callback waiting for a worker
whose collection waits for that callback would introduce a collector deadlock.
Newly appended work can remain for the next safepoint after the selected batch;
there is no need for an arbitrary graph-size limit or endless recursive drain.

This prevents same-owner FIFO calls from overlapping; it does **not** prove an
arbitrary user callback can finish. A callback awaiting another automatic
finalizer behind itself can hang. One blocked callback also delays unrelated
callbacks and increases retained memory/resources. State that restriction and
test the worker-collection case. If independent finalizer progress is a required
product property, reject this simplification and retain a reviewed scheduler;
do not disguise a single-drainer deadlock as a user error after promising progress.

Keep current finalizer eligibility, resurrection, weakref and legacy-cycle
selection. Do not change all call-kind ordering merely to obtain a general
completion theorem. Explicit resource ownership/close must handle dependencies
that a language-level `__del__` ordering contract does not promise.

## Resource safety cannot be wished away

`Objects/abstract.c:PyBuffer_Release` suppresses an exporter's release callback
after the exporter has been swept. `Objects/memoryobject.c` forcibly zeros
`memoryview.exports` during deallocation and queued release. The five persistent
consumer families in PROCESS §0.3 also release buffers from cleanup paths.
These are normal-runtime obligations, regardless of interpreter shutdown.

The source-admissible cycle is concrete: an exporter E returns a backing
memoryview; M = memoryview(E); E retains H = struct.iter_unpack('B', M).
Strong edges give H→M→managed-buffer→wrapper→E→H, while H holds an actual export
of M. Arbitrary within-SCC order can release M before H. Serial callbacks and
Q=all completion tokens do not prove M has zero exports. This shape has been
source-reviewed, not executed as a new probe here. Forcing zero or ignoring a
release to make a test green is not an acceptable resolution.

The narrow profile above removes Python-defined buffer acquisition at its slot
entry, before acquiring anything; calling `__buffer__` explicitly does not gain
permission to publish an acquisition. Native exporter ports must satisfy the
stated leaf-release contract, including indirect callbacks and metadata lifetime.
This excludes some extensions until ported. It is not enough to recognize only
`slot_bf_releasebuffer` and assume every other C function is safe.

Then audit/fix each finite consumer's release transaction: preserve the original
Py_buffer and shape/stride storage through the callback; consume ownership once;
leave an inert, reentry-safe object; release exporters before freeing metadata.
Even in this profile, the forced-zero/swept-exporter scheme needs an explicit
proof for wholly dead native-only cycles, including external raw resources.
If it cannot be proved, retain that cycle safely and report the limitation, or
finish the proper release phase. An unbounded runtime leak is not an acceptable
general replacement for native cleanup.

Keeping Python buffer callbacks instead needs deferred release before unsafe
sweep, live exporter/consumer anchors and a real acquisition identity/reentry
rule. Five `tp_finalize` functions fail when a Python subclass replaces
Unpickler's finalizer or reacquires a view after resurrection. Conversely,
queueing every `tp_clear` is not a drop-in fix: subtype clearing destroys fields,
some consumers have no clear/non-GC layouts, and memoryview clear deliberately
declines while exported. Existing `tp_clear` permits objects to remain cleared
but alive and disallows concurrent automatic clear; it is a useful interface to
evaluate before adding another slot, not an already completed implementation.
[Type slot contract](https://docs.python.org/3/c-api/typeobj.html#c.PyTypeObject.tp_clear)

## Shutdown, interpreters and fork: choose an honest boundary

For a command-line process, run registered exit handlers and explicit stream
flushes while the runtime is intact, then cut off automatic callbacks and let
process termination reclaim retained objects. Do not free heap/queue/metadata
under an active callback, switch its owner, pretend it completed, or leave a
partially dismantled runtime executing it. A fast-exit route must actually end
the process; `Py_FinalizeEx()` returning to an embedding host is different.
Late finalizer delivery is not a language requirement. CPython itself documents
arbitrary teardown order, incomplete memory release and restrictions on recursive
finalization. [Interpreter finalization](https://docs.python.org/3/c-api/interp-lifecycle.html#c.Py_FinalizeEx)

Initially reject subinterpreter creation and repeated runtime finalization/
initialization in this profile. This is simpler and more enforceable than
pretending private heaps alone solve surviving immutable objects, callbacks,
runtime-global perf arenas and per-interpreter extra tables. It breaks embedding
and subinterpreter consumers, including relevant tests; expose it as an explicit
experimental support boundary. Do not retain per-deletion heaps forever and call
that equivalent to an exit leak. Repeated deletion/reinitialization needs bounded
resource reclamation before support is restored.

Keep ordinary main-interpreter single-threaded quiescent fork only. Reject fork
from callbacks or while automatic delivery is active, and reject a multithreaded
Python fork rather than repair vanished RUNNING calls. Raw external fork in an
embedding host must use an explicitly documented exec/exit-only child contract
unless the host establishes the same preconditions. This is a real compatibility
cost for prefork applications; spawn/forkserver or fork before threads are the
alternatives. CPython already rejects subinterpreter fork and warns about threads;
we would deliberately enforce a stronger boundary.
[os.fork contract](https://docs.python.org/3/library/os.html#os.fork)

## Performance after correctness

Keep the current byte-trigger/live-size heuristic first: `collect_pass()` already
sets the next limit to max(configured threshold, live bytes). Do not invent a new
heuristic before a matched threshold/RSS experiment. Use existing collection
time, candidates/collected/live bytes, pause and peak-RSS counters; compare elapsed
time as well as memory. A lower collection count alone is not a win.

Generations are later. They need the actual owner/store/constructor ledger and a
young-set correctness oracle; the code already has source-identified partial
constructor and running-frame publication issues to solve. Retire neither
conservative roots nor clearing until their independent replacement works.
An all-conservative rewrite would give up established retention improvements and
root checks without removing callback/resource problems. The Boehm collector is
a primary precedent for useful STW nonmoving mark/sweep plus deferred finalizers,
but its documented conservative retention, cycle-finalization and synchronization
limits are warnings to specify our contract, not evidence of equivalent behavior.
[Algorithm and finalization](https://www.hboehm.info/gc/gcdescr.html)

## Keep, simplify, defer, drop

| Action | Concrete scope |
| --- | --- |
| Keep | Full-heap nonmoving collection; hybrid roots and writer fixes; 168 safety repairs and qualification; owner metadata checks where retained APIs need them; actual callback roots |
| Simplify | One drainer/enqueue-and-return; explicit support admission; process-exit callback cutoff; finite release transactions and port contracts |
| Defer | Shared/private subinterpreter lifecycle support, repeated embedding teardown, Python buffer protocol if restricted, multithreaded fork, generations, map-based root removal |
| Drop from the first-release plan | Completion DAG/SCC-token machinery, owner adoption, close-time code-extra revocation, borrow leases and general new cleanup type capability |

The deferred designs can be revisited when a named workload requires their
guarantees. Do not silently delete the historical counterexamples or weaken a
supported API to make the collector appear complete.

## Alternatives and shortest path

1. **Recommended bounded profile above.** Smallest scheduler/lifecycle policy;
   substantial, explicit API restrictions. Native buffer proof/finite repairs
   remain a blocker even after restricting Python callbacks.
2. **Retain Python buffer support, still one interpreter/drainer.** Do the finite
   clear/release-phase design and acquisition-cycle proof now. Higher resource
   compatibility; no need yet for cross-owner adoption or metadata leases.
3. **Retain full interpreter and independent-callback progress guarantees.**
   Continue the reviewed scheduler/lifecycle work, including real quiescence and
   native cleanup. It is a larger product, not a necessary property of mark/sweep.
   Do not claim SCC condensation alone resolves acquisition constraints.

First finish/review the already running 168 qualification; do not restart it.
Next obtain a support-policy ruling, implement admission and the small drainer
change separately, and resolve the finite buffer boundary before declaring the
chosen profile correct. Meaningful checks are: blocked finalizer plus a worker
that collects and returns; nested collection and exceptions with intact roots;
FIFO callbacks without overlap; repeated explicit releases/reentry with intact
metadata; native consumer/exporter cycles and a causal omitted-release control;
admission rejection before side effects; and actual exit/fork behavior in the
supported profile. Preserve S1 as evidence, but no blanket matrix rerun is needed.
Then qualify the actual combined source once under the existing release/debug/
stress/ASan policy, with exclusions limited to explicitly unsupported APIs.

This review made no runtime changes and ran no native process. Source inference
is identified above; new support restrictions and scheduling semantics are
recommendations requiring a policy decision, not retroactive qualification.
