# 13: Callbacks and finalizers run on the interpreter that owns them

Status: adopted 2026-10-28 10:50 PDT after review rounds 1 and 2 (both
revise; Review below) and three coordinator rulings (the last
subsection of Review), for step 7a (boil/PROCESS.md section 0, moved ahead of step 7
on 2026-10-28). It retires finding B of note 08 (the hack register's
shared-heap row) and note 11 question 18, and closes the collector holes
found while reading for it: a shared-heap interpreter with its own GIL
(section 7.2), the heap of a deleted shared-heap interpreter (section
5.3), and destruction metadata read through the wrong interpreter's
tables (section 3.4). Coordinator rulings of 2026-10-28 are marked
"(ruled)".

## Decisions

1. **The owner of a queued call is the interpreter whose sub-process
   holds the block of the object it is about**: a weak reference
   callback belongs to the weak reference's owner, a finalizer, a legacy
   `tp_del` and a destroy hook to the dead object's (section 2, ruled).
2. **One queue per interpreter, of linked nodes.** A collection builds
   its calls into a batch, resurrects from it, and splices each node onto
   its owner's queue in collection order. Routing allocates nothing
   (section 3).
3. **A queue drains only on a thread of its owner**, at the safepoints
   that drain it today, whether or not a collection may run. A thread
   learns that its interpreter's queue has work when it takes the GIL
   (section 4).
4. **A queue opens at the end of its interpreter's `pycore_interp_init()`
   and closes in `_PyGC_Fini()`, after a last drain.** gc.callbacks and
   the collector's Python-level debug output follow the same predicate
   (sections 4, 5).
5. **At the close, a shared-heap interpreter's heap becomes the main
   interpreter's**: calls that reach it later go to the main
   interpreter; at deletion its segments, immortal registry, registered
   roots and kept threads are adopted, so its live blocks stay traced
   and swept; interpreter-local destruction metadata on its surviving
   objects is retired on its own thread first (section 5, ruled).
6. **Nothing is dropped except a never-opened isolated interpreter's
   calls**, whose heap no collection visits again (section 5.4).
7. **A domain shares one GIL.** `use_main_obmalloc` with `OWN_GIL` is
   rejected at configuration with `InterpreterError`, an intentional
   compatibility break (section 7.2, ruled).
8. **Destruction metadata is the owner's**: discovery reads the owner's
   watcher masks and code-extra table, and registering a watcher or
   setting a code extra on an object another interpreter owns is refused
   (section 3.4, ruled for discovery; the refusal is round 2's).
9. **Order holds across owners**: a call waits for the earlier calls of
   other owners in its batch that reach its object, without any thread
   waiting for another interpreter (section 3.5).
10. **A collection pins its domain from the mark to the splice**, so no
    interpreter it names is freed under it (section 3.2).

## 1. The problem

The live collector marks a domain: the interpreter that collects, or,
for any interpreter that shares the main interpreter's object allocator
(`use_main_obmalloc`, the legacy configuration), every such interpreter
together (`live_domain()`, Python/gc_trace.c). The calls it queues go to
the collecting interpreter's queue (`queue_push(ls, ...)` with
`ls = tstate->interp->gc.live`, Python/gc_live.c) and run on the
collecting thread at the end of the collection (`collect_pass()` then
`run_queue()`).

So a collection on a legacy subinterpreter runs the main interpreter's
weak reference callbacks and `__del__` methods. 10a rung 1b recorded the
C stack (10a-1b.md, end gates): `new_interpreter` > `pycore_init_types` >
`_PyTypes_InitTypes` > `type_ready` > `functools_wraps` >
`meth_get__qualname__` > `PyUnicode_FromFormat` > `PyErr_CheckSignals` >
`collect_from_entry` > `collect_pass` > `run_queue` >
`PyErr_FormatUnraisable` > `PyStructSequence_New`, which faults because
the new interpreter's unraisable-hook type does not exist yet. A probe
named the callback (`importlib._bootstrap._get_module_lock.<locals>.cb`)
and the interpreter it ran on (1, which has no importlib during
`pycore_init_types`). A second site ran a main-interpreter `__del__`
through `slot_tp_finalize` from `test_current_tstate_matches`. Any
collection during a shared-heap interpreter's start-up can do this;
`PYTHON_GCSTRESS` only makes it frequent.

The fault has two parts, and the design fixes both: calls run on the
wrong interpreter, and calls run in an interpreter that cannot yet run
Python code. The second holds for an interpreter's own calls too:
importlib's `_ModuleLock` callbacks of interpreter 1 itself can be queued
during its `_PyImport_InitCore()`.

## 2. Ownership

**The rule.** Every interpreter has its own mimalloc sub-process
(`interp->mimalloc.subproc`, created in `init_interpreter()`,
Python/pystate.c), and every thread state's heaps allocate segments in
its interpreter's sub-process (`tld->segments.subproc`,
`tstate_mimalloc_bind()` in pystate.c, with the comment "to keep
Python objects from different interpreters separate"). A thread that
exits abandons its segments, and only a thread of the same sub-process
reclaims them (Objects/mimalloc/segment.c, `segment->subproc !=
heap->tld->segments.subproc` refuses the reclaim). So the segment of a
heap object's block names the interpreter whose thread allocated that
block. The owner of an object is that interpreter, and after the
interpreter's close the main interpreter (section 5).

"The block" is the current one. `PyObject_Realloc()` and
`_PyObject_GC_Resize()` allocate the new block in the resizing thread's
heap, so a shared object resized by another interpreter moves to that
interpreter, and loses its weak references, as any reallocated object
does (note 06 18.5). Preserving the first allocator would need an
interpreter field per object or a cross-heap reallocation; neither is
worth it for an operation that only C code performs on its own objects.

The owner of each kind of call:

| Kind | `live_call` | Owner |
|---|---|---|
| weak reference callback | `CALL_CALLBACK`, op the weak reference | the weak reference's owner: the interpreter where `weakref.ref(x, cb)` or `weakref.finalize` ran |
| finalizer (PEP 442) | `CALL_FINALIZE` | the dead object's owner |
| legacy `tp_del` | `CALL_LEGACY` | the dead object's owner |
| destroy hook (`run_destroy_hook()`) | `CALL_DESTROY` | the dead object's owner |

The lookup, `_PyGC_LiveOwner(res, op)` (gc_trace.c, beside
`live_page_of()`), maps `_mi_gc_segment_of(op)->subproc` to the domain
interpreter the mark recorded, the same lookup `live_page_of()` makes
for every conservative word, and then applies the close (a closed
owner's calls go to the main interpreter). It runs once per queued
call, never per dead object, and not at all when the domain has one
member.

This is the convention CPython already uses where it decides ownership:
PEP 684 makes the allocator state the interpreter's, a single-phase
module's `m_copy` is created on a temporary main-interpreter thread state
so that it belongs to the main interpreter (`switch_to_main_interpreter()`,
Python/import.c), and cross-interpreter data is released in the
interpreter that made it (`_PyEval_AddPendingCall()` in
Python/crossinterp.c).

**Rejected: the callable's interpreter.** A weak reference callback is a
callable, not a function. A builtin function, a bound method of a C
type, a `functools.partial` or a C callable has no interpreter, and a
Python function has one only through its globals dict's module, which
can be a shared object. A finalizer has no callable to ask at all:
`slot_tp_finalize` looks `__del__` up on the type when it runs. The rule
would need a second rule for the cases it cannot answer.

**Rejected: the type's interpreter.** Static builtin types are shared by
every interpreter, and they are the types of the objects whose
finalizers matter most: generators, coroutines and async generators
(`_PyGen_Finalize`), `_io`'s file objects. A heap type of a single-phase
module is shared through `m_copy`. A user class defined in the main
interpreter can have instances made in a subinterpreter that reached the
class through a shared object. The type answers for none of these, and a
type would need an interpreter field it does not have.

**Rejected: one sub-process per domain.** If shared-heap interpreters
used the main interpreter's sub-process, their abandoned segments would
need no adoption at teardown, but a main-interpreter thread could
reclaim a live subinterpreter's segment, and the segment would then name
the wrong owner while that interpreter still runs. The allocator's
separation is what makes the rule exact; adoption (5.3) joins the
sub-processes only once the interpreter has no threads left.

**Failure cases of the chosen rule.**

- *An object made in one interpreter for another.* In a shared heap,
  C code can allocate on interpreter X an object that interpreter Y
  then uses, and its finalizer runs on X. CPython's own code switches to
  the owner's thread state for such objects (`m_copy`), and under
  reference counting the finalizer of a shared object runs on whichever
  thread drops the last reference, which is less defined than this. The
  rule is recorded in `Doc/library/gc.rst`.
- *Cross-interpreter order.* Finalizers run referrer first within one
  collection (note 06 16.2), and a referrer and referent pair can cross
  interpreters in a shared heap: a `BufferedWriter` made on a
  subinterpreter over a `FileIO` made on the main interpreter. Section
  3.5 keeps the order across owners.
- *Static and non-heap immortal objects.* They are not in the object
  heaps, so the walk never lists them dead and no call is about one. A
  weak reference object is always a heap object. The lookup asserts a
  segment. Heap immortals (objects registered with
  `_PyGC_LiveAddImmortal()`, such as the `all` and `any` that
  `pycore_init_builtins()` makes immortal) are in the heaps but never
  die; section 5.3 keeps their registry.

## 3. The queue

### 3.1 What it holds

Each interpreter's GC state, `struct _gc_runtime_state` in the
interpreter struct (as built in rung 1; the design first put it in the
malloc'd `struct _gc_live_state`), holds a FIFO list of nodes,
`live_call {next, op, callback, kind}`, with head and tail pointers,
replacing the array, and a list of waiting nodes (3.5), whose
nodes carry a count of the calls they wait for and the list of nodes
that wait for them. A node comes from the C library's `malloc`
when the call is queued, and is freed after the call runs. The owner is
implied by the queue that holds the node. The same struct holds the
phase (initialising, open, closed; section 5), the ready flag (section
4), the pin count (3.2) and the counters of 5.4. The queue therefore
exists for the interpreter's whole life, independent of the live state,
and splicing onto it never allocates.

Nodes rather than an array because routing moves calls between queues:
a splice cannot fail and needs no compaction, so the memmove, the
`running_queue` count that guards it and the doubling go. The cost is
one `malloc` and one `free` per call, against a call that runs Python
code.

### 3.2 Filling by owner

`collect()` keeps its order of work. `clear_callback_weakrefs()`,
`handle_finalizers()` and `queue_destroy_hooks()` append nodes to the
collection's batch, a local list, and resurrect from the nodes they
appended, as they do today from the range they appended to the queue.
`clear_callback_weakrefs()` allocates a callback's node before it clears
the weak reference, so an allocation failure leaves the weak reference
and its callback intact (today the callback is lost). After
`queue_destroy_hooks()` and before the sweep, `route_calls()` splices
each node of the batch onto its owner's tail, in batch order. Nothing
runs between the batch and the splice: it precedes the sweep, the
"stop" gc.callbacks and the drain.

**Failure.** Node allocation and the order table of 3.5 are the only
allocations, and they share one failure path, today's in `collect()`:
the dead are undone, nothing is freed, and the pin is released. The
table is allocated before the first node: **no batch exists when the
table's allocation can fail**, so that failure routes nothing. A node's failure leaves the calls batched before it, which
are ordered with the table already held (3.5) and routed by splicing,
so every queued call reaches its owner, in order, and no call runs on
another interpreter.

The domain is the mark's: `_PyGC_TraceLiveMark()` copies the domain's
interpreters and sub-processes into the mark result. **The pin.** The
GIL does not keep them alive: `PyInterpreterState_Delete()` detaches its
thread, releasing the GIL, before it unlinks and frees the interpreter
(pystate.c), so it can run while another thread's collection is between
its mark and its splice. `live_domain()` therefore increments a runtime
count of pinned collections under the head lock, and `collect()`
decrements it under the head lock after `route_calls()`. The unlink in
`PyInterpreterState_Delete()` (and the fork child's disposal, 5.5,
where no collection can run) takes the head lock and, while the count is
non-zero, releases it and waits on a `PyEvent` that the decrement sets.
The deleting thread holds neither the GIL nor the head lock while it
waits, and the collection it waits for needs neither from it, so the
wait ends. Every interpreter the batch can name, and its live state,
therefore exists until the splice is done.

### 3.3 Memory and bounds

Every interpreter of the domain is a root holder for its own queue: the
mark's precise phase calls `_PyGC_LiveVisitRoots()` for each domain
interpreter (`live_mark_roots()`), which visits each queued op and
callback. A queued object and everything it reaches stay alive until
the call runs, whichever interpreter's collection found it, and each
object is queued at most once: a queued object is reachable, so a later
collection does not list it dead again.

What a queue holds is the transitive graph of its calls' objects, and in
a shared heap that graph can include other interpreters' objects. A
busy interpreter drains at its next safepoint, as now. An idle one (no
attached thread) keeps the graph until a thread of it runs or it closes.
So the memory an idle owner retains is bounded by the graphs of its own
garbage, which can be large, and its latency by how long it stays idle;
no collection of another interpreter waits for it. Section 11 measures
both.

### 3.4 Destruction metadata belongs to the owner (ruled)

Two kinds of per-object state are indices into an interpreter's tables:
a dict's or type's watcher bits (`_ma_watcher_tag`, `tp_watched`) and a
code object's `co_extra` slots (`interp->co_extra_freefuncs`). Discovery
reads them through the collecting interpreter today:
`_PyGC_TraceLiveMark()` builds `struct live_walk`'s watcher masks from
`tstate->interp`, and `live_destroy_hook_of()` calls
`_PyCode_HasExtraFree()`, which reads `_PyInterpreterState_GET()`'s
table (Objects/codeobject.c). The run reads them through the running
interpreter (`_PyDict_SendEvent()`, `free_code_extras()`).

The masks become per domain entry, set from each interpreter's tables;
`live_walk_heaps()` sets the current entry as it walks each
interpreter's heaps and abandoned segments; `live_destroy_hook_of()`
takes the owner and calls `_PyCode_HasExtraFree(owner, op)` (the
function gains the parameter; gc_trace.c is its one caller). The same
owner argument covers function and code watchers, which are per
interpreter rather than per object (`active_func_watchers`,
`active_code_watchers`): discovery tests the owner's set, and the event
is sent on the owner. The hook runs on the owner (section 2), so the run
reads the same tables. No
index is ever read through another interpreter's table: section 5.3
retires the indices of a closing interpreter's surviving objects before
its heap changes owner.

**Registration is the owner's.** Discovery through the owner's tables
is right only if every index on an object is the owner's. Today
interpreter A can set an extra on B's code object with A's index
(`PyUnstable_Code_SetExtra()`), or watch B's dict or type with A's
watcher id (`PyDict_Watch()`, `PyType_Watch()`), which only a shared
heap allows; the bit or slot then means a different watcher or free
function in B's table, and B's watcher with the colliding id hears of a
dict it never watched. These three functions now look the object's
owner up (`_PyGC_LiveOwnedBy(op, interp)`, the segment lookup of section 2
compared with the attached interpreter under the head lock, without an
unpinned foreign interpreter pointer escaping) and refuse a
foreign object: `PyDict_Watch()` and `PyType_Watch()` raise
`ValueError` ("cannot watch an object owned by another interpreter"),
and `PyUnstable_Code_SetExtra()` raises `SystemError`, both documented
(`Doc/c-api/dict.rst`, `type.rst`, `code.rst`). `PyUnstable_Code_GetExtra()`
on a foreign code object returns no extra. The reserved dict watcher ids
(`DICT_RESERVED_WATCHERS_MASK`: the builtins and module watchers that
`pylifecycle.c`, `moduleobject.c` and the specializer install) are
exempt: every interpreter registers the same callbacks under them, so
they mean the same in every table, and the collector already leaves them
to `dict_dealloc()`. This is a compatibility
break confined to shared heaps (an isolated interpreter cannot reach a
foreign object), recorded with 7.2's in PROCESS.md section 0.2
(ruled). Keeping the
registering interpreter beside each bit or slot was the alternative;
it costs a field per watched object and still leaves the question of
which interpreter runs the foreign watcher when the object dies on its
owner.

Queue2b's first implementation also guards `PyDict_Unwatch()` and
`PyType_Unwatch()` symmetrically. Public type Watch/Unwatch reject
ownerless static types with ValueError: global `tp_watched` bits cannot
represent interpreter-local callback IDs. Heap immortals keep their real
allocator owner. This is an additional explicit compatibility break.

This first slice covers registration/access and destruction discovery.
Ordinary foreign mutation dispatch remains a required separate contract:
dict events, type modification and function MODIFY_CODE/DEFAULTS/
KWDEFAULTS/QUALNAME events still consult the mutator's tables. Function
CREATE belongs to allocation; DESTROY is covered by owner discovery and
delivery. Code exposes only CREATE and DESTROY events. No silent event
drop or thread-state switch is introduced here; retirement/adoption and a
complete owner-safety claim remain blocked on the remaining work.

### 3.5 Completion order for dependent calls

A queue's FIFO invocation order does not imply completion order: several
threads of one owner can drain it, and a finalizer can release the GIL.
The S1 probe reproduces an inner finalizer running while its referrer's
finalizer is blocked. The same dependency crosses owners: closing a
`FileIO` before its unreachable `BufferedWriter` finishes flushing loses
the buffer. Rooting both preserves memory, not the resource.

**The rule.** A call R waits for every earlier call P in its batch whose
object reaches R's object through dead objects, irrespective of owner.
R is released only after all those predecessors complete. All waits point
backwards in batch order and therefore cannot cycle. Unrelated calls need
no wait and may run concurrently, including within one owner. A waiting
call is a root on its owner's waiting list (3.3); completion of its last
unfinished predecessor splices it onto that owner's queue and publishes
`queue_ready`.

**Which calls.** Predecessors are finalizers and legacy `tp_del`s, which
may use their objects' referents. Waiters are finalizers, legacy `tp_del`s
and destroy hooks. Weak reference callbacks neither wait nor are waited
for; their batch order relative to finalizers is unchanged.

**Computing it (rung 2a design obligation).** Dependency state is needed
whenever the dead set contains at least two potentially dependent
call-bearing objects, even of one owner. It is allocated before the first
node is batched. Allocation failure takes 3.2's single failure path with
an empty batch; no calls or sweep run from an incomplete dependency graph.

The earlier per-owner computation and its `O(K*D)` claim assumed that one
owner's drains completed serially. They do not. Waiting only for the last
earlier call of an owner does not cover a still-running earlier call of
that owner. Rung 2a must specify and review an exact dependency graph,
its checked storage bound, deduplication and completion accounting before
implementation. Its proof must cover several simultaneous unfinished
predecessors of one waiter. The same graph and release rule serve both
same-owner and cross-owner dependencies; an owner-count shortcut is invalid.

**What waiting costs.** A main-interpreter call that waits for an idle
subinterpreter's finalizer waits until that interpreter runs or closes
(5.1 drains it at the close). That is the dependency itself: the file
cannot be closed before the writer that owns its buffer has flushed.
Section 11 measures how long such calls wait.

### 3.6 Delivery markers

`queue_legacy()` sets `_Py_GC_DEL_CALLED_FLAG` when it queues a
`tp_del`, and the heap walk clears that flag on every marked object as
"reachable again" (gc_trace.c), so a call waiting in an idle queue,
marked through the queue at each collection, loses its marker and its
object's next death would run `tp_del` again. Delivery is recorded at
execution instead: `run_queue()` sets `_Py_GC_DEL_CALLED_FLAG` just
before it calls `tp_del`, and `handle_finalizers()` relies on the queue
root, not the flag, to keep a queued object from being queued twice. A
PEP 442 finalizer already records delivery at execution
(`PyObject_CallFinalizer()` sets the finalized flag), and a destroy
hook's `_Py_GC_NOTIFIED_FLAG` is never cleared by the walk, so the two
other kinds need no change.

## 4. When a queue drains

A queue drains only on a thread whose `tstate->interp` is its owner and
only while it is open. The drain points are today's, applied to the
thread's own interpreter:

- **The end of a collection the thread ran** (`collect_pass()`), unless
  the collection runs no Python code (`no_python`, an allocation's
  inline collection). It drains its own queue only; the calls spliced to
  other owners do not run here.
- **The eval breaker's GC bit** (`_Py_HandlePending()` and
  `PyErr_CheckSignals()`, both through `_Py_RunGC()`).
- **`gc.collect()` and `PyGC_Collect()`**, through the end of collection.
- **The interpreter's shutdown collections and its close** (section 5).
- **Before a trace, profile or monitoring hook is installed**
  (`_PyGC_LiveRunQueue()`, Python/legacy_tracing.c), so that calls of
  objects that died before tracing began run untraced.

An allocation is a collection safepoint (note 11 section 4.2) but not a
drain point: since 5a, Python code runs only where the eval loop checks
its breaker (note 06 15.2).

**Collecting and draining are separate permissions.** Today
`_Py_RunGC()` drains only when no collection is due, and when one is due
but `can_collect()` refuses (gc disabled, a zero threshold, an
interpreter finalising, a stress limit of zero) it returns with the bit
already cleared and the queue full. Now it collects if a collection is
due and allowed, and then, in every case, drains its interpreter's queue
if the queue is open and non-empty. Draining needs only that the thread
can run Python code here, which a breaker check and
`PyErr_CheckSignals()` guarantee; `gc.disable()` stops automatic
collection, not the delivery of calls already found. If the drain
defers for a pending signal, `run_queue()` sets the bit again, as now.
If the queue is not open, the bit is consumed and the opening sets it
again.

**Telling the owner.** An owner whose queue gains calls from another
interpreter's collection is not running: the domain has one GIL and the
collecting thread holds it (section 7.2). Every thread takes the GIL
before it runs again, and `take_gil()` already refreshes the thread's
eval breaker from its interpreter's state
(`update_eval_breaker_for_thread()`, Python/ceval_gil.c). It also sets
the GC bit when its interpreter's `gc.queue_ready` is set (the queue
open and non-empty, maintained by the splice and the drain under the
GIL). That one load per GIL acquisition replaces any cross-thread
signal and covers threads that attach after the splice, such as the
main thread state that `_testcapi.run_in_subinterp()` detaches and
reattaches with `PyThreadState_Swap()`. A thread that never takes the
GIL again (a daemon thread hung at shutdown) never drains, and its
interpreter's close drains instead (5.1).

**Which thread.** Any thread of the owner; every thread of it that
collects joins a drain in progress (note 06 16.6), with the per-thread
guard `gc_trace.running_queue`.

**States in which a queue does not drain.**

| State | Where | What happens to calls |
|---|---|---|
| initialising | from `_PyGC_Init()` to the end of `pycore_interp_init()` | kept; at the opening, if the queue is non-empty, the GC bit is set on the thread so the next breaker drains |
| not running | no thread of the interpreter attached | kept; the next thread to take the GIL drains |
| finalising | `Py_EndInterpreter()` or `Py_FinalizeEx()` until the close | drained by the shutdown collections, the breaker and the close; automatic collections stay off (`can_collect()`) |
| closed | from the close in `_PyGC_Fini()` to deletion | none arrive: the owner lookup gives the main interpreter for a shared-heap interpreter, and no other interpreter collects an isolated one's heap |

The opening is the end of `pycore_interp_init()` because that is where
an interpreter first has what a call and its error report need: its
types (the unraisable hook's argument type among them,
`make_unraisable_hook_args()`), `sys` with `unraisablehook` and
`stderr`, builtins, and importlib's core. It is the same point for the
main interpreter and for subinterpreters, and for an embedder that
stops after the core phase (`_init_main=0`).

## 5. Teardown

### 5.1 The last drains, and the close

An interpreter's teardown collects twice in `finalize_modules()`, each
with one pass because a later collection follows; wipes the `sys` and
`builtins` module dicts (`finalize_clear_sys_builtins_dict()`); and
collects for the last time in `interpreter_clear()`, with the second
pass of `collect_from_entry()`. Each drains the interpreter's own queue;
signals do not defer a drain during finalisation (`run_queue()` tests
`_PyInterpreterState_GetFinalizing()`).

What a call can rely on differs between the two. At the
`finalize_modules()` collections, module globals, `sys` and builtins
still work. At the last collection they are wiped: a call runs, its
name lookups may fail, and its error report is structurally safe
(the hook's argument type exists, and with `sys.unraisablehook` and
`sys.stderr` gone `PyErr_FormatUnraisable()` falls back to the default
hook and then to silence, Python/errors.c). That is the state the
reference-counting build gives an object that dies at that point, and
the state in which the close runs.

**The close** is the first thing `_PyGC_Fini()` does, on the
interpreter's last thread, right after the last collection. It is a
sequence; each step finishes before the next starts:

1. *Drain.* If the queue was ever opened, `run_queue()` runs until the
   queue and the waiting list are empty, including what another
   interpreter's collection spliced here after the last drain (a
   shutdown finalizer released the GIL and a main-interpreter thread
   collected) and what the calls themselves queue.
2. *Retire the metadata* (5.2), for a shared-heap interpreter that was
   opened.
3. *Drain again*, as in step 1, for what step 2's free functions caused.
4. *Publish the close.* The phase becomes closed under the head lock.
   From here the owner lookup maps the interpreter's sub-process to the
   main interpreter (decision 5), so nothing more reaches the closed
   queue, which is in the interpreter struct (3.1). `get_live()` never
   recreates a live state. Rung 1 frees the rest of the live state (the
   immortal registry) here, as before 7a; rung 2 keeps it, to adopt or
   free it at deletion (5.3, 5.4).

Steps 1 and 3 can loop only as long as calls keep making garbage with
calls, as any drain can.

### 5.2 Retiring the destruction metadata

A shared-heap interpreter's objects can outlive it (5.3). Before its
heap changes owner, the close retires the indices of section 3.4 on its
surviving objects, on its own thread, with its tables:

- *Freeze.* The interpreter's `registration_frozen` flag (GCState) is
  set and never cleared. From then on, `PyUnstable_Code_SetExtra()` and
  the non-reserved `PyDict_Watch()` and `PyType_Watch()` on an object
  this interpreter owns are refused like a foreign object's (3.4), so no
  new index appears during the step. The queue stays open: the freeze
  changes registration only, so it is a flag of its own and not a
  phase.
- *Watcher bits.* One walk of the interpreter's own heaps and abandoned
  segments (the walk of `_PyGC_TraceLiveHeapObjects()`, restricted to one
  sub-process) clears every non-reserved watcher bit. No code runs
  during it, so one pass is enough.
- *Extras.* Then a loop: each iteration is a walk of the same heaps that
  stops at the first code object carrying extras and retires it with
  `_PyCode_ClearExtras(co, 1)`, which detaches the extras array from the
  code object before it calls any free function, then frees the array.
  The loop ends when a walk finds no code object with extras. No walk is
  in progress while a free function runs, so the function may allocate,
  collect, release the GIL or queue calls; step 3 of the close drains
  what it queued. A collection it runs cannot free the extras twice:
  they are no longer on the code object, so its destroy hook finds none,
  and the free function receives only the extra, never the code object.
  An object it creates cannot carry an index, since the freeze holds, so
  each iteration removes one code object's extras for good and the loop
  ends. The iterations are counted in `gc_queue_info()["retired"]`. The
  cost is O(heap × code objects with extras); the second factor is the
  count of code objects a tool has tagged, and section 11 measures it.

This step allocates nothing and has no failure path.

These are C calls into the interpreter's own extensions, made where
reference counting would have made them for objects that die at the end
of `interpreter_clear()`. A surviving code object loses its extras, a
cache whose owner is gone; a surviving dict stops being watched by
watchers that no longer exist. A never-opened interpreter (5.4) and the
fork child (5.5) run no free function: their clear is the
watcher-bit walk with `_PyCode_ClearExtras(co, 0)` applied in the same
walk (no code runs, so one pass), which frees each extras array and
leaks what its slots point to. That is a different situation, an
interpreter whose code may not run, not a second form of this step. An isolated interpreter skips it: nothing outlives it that
another interpreter can reach.

### 5.3 Adoption at deletion (ruled)

`free_interpreter()` (pystate.c) deletes an interpreter's sub-process
only when no abandoned segment points to it. A shared-heap interpreter
can leave live blocks: strings it interned into the main interpreter's
dictionary (`has_shared_intern_dict()`, Objects/unicodeobject.c), and
any object it allocated that a main-interpreter object refers to. Today,
after deletion, `live_domain()` no longer lists the interpreter, so
`live_page_of()` rejects addresses in its segments: such an object is
never marked, traversed, listed dead or swept, and a main-interpreter
object reachable only through it is freed while it still holds the
pointer. Its heap immortals lose their registry when `_PyGC_Fini()`
frees it, so what only they reach is freed too. Its registered
`_PyGCRoot` nodes stay linked to an interpreter that no longer exists.

**The adoption.** `PyInterpreterState_Delete()` unlinks the interpreter
under the head lock, once no collection pins the domain (3.2); every
mark holds the head lock from `live_domain()` to the end of the heap
walk. In the same critical section it calls `_PyGC_LiveAdopt(interp)`
(gc_live.c), which allocates nothing:

- the interpreter's live state is linked into the main interpreter's
  list of adopted states, keeping its immortal registry and its kept
  threads (`_PyGC_LiveKeepThreads()`), which the main interpreter's
  marks visit as roots (`_PyGC_LiveVisitRoots()` and
  `live_mark_roots()`'s kept-thread loop over the adopted states);
- its registered roots are relinked onto the main interpreter's list
  (`gc.trace_roots`), with `root->interp` set to the main interpreter,
  so a later `_PyGC_RemoveRoot()` finds them;
- its sub-process is put on the main interpreter's pending list.

A mark therefore sees the interpreter either listed in the domain, with
its own state, or adopted, never neither. Everything the adoption
writes is read only under the head lock.

**The absorption.** The next main-domain collection, at the start of
`_PyGC_TraceLiveMark()` under the head lock and with the GIL held,
absorbs each pending sub-process into the main interpreter's with
`_mi_subproc_absorb(main, retired)`, a new function in
Objects/mimalloc/arena-abandon.c, which follows mimalloc's own
synchronisation for abandoned segments:

- *Arena segments.* For each arena, it takes the arena's
  `abandoned_visit_lock` (blocking), scans `blocks_abandoned`, and for
  each set bit whose segment's `subproc` is the retired one, writes the
  main sub-process into the field and moves one from the retired
  sub-process's `abandoned_count` to the main one's, leaving the bit
  set. Every path that clears an abandoned bit takes that arena lock
  first when `mi_option_visit_abandoned` is on, as it is in this build
  (`mi_arena_segment_clear_abandoned_next_field()`; the reclaim on free
  is off, `mi_option_abandoned_reclaim_on_free`), so no clearer is
  between its clear and its re-set while the scan reads the arena, and
  the scan cannot miss a segment.
- *OS segments.* It takes the retired sub-process's
  `abandoned_os_visit_lock`, then the main one's, then the retired
  one's `abandoned_os_lock`, then the main one's (visit before list, as
  `mi_arena_segment_clear_abandoned_next_list()` does; retired before
  main; no other path holds two sub-processes' locks), re-points each
  segment of the retired list, appends the list to the main one's, and
  moves `abandoned_os_list_count` and `abandoned_count`.
- *Delete.* The retired sub-process's `abandoned_count` is then zero,
  which a debug build asserts, and `mi_subproc_delete()` frees it. If it
  is not zero, it stays pending and the next collection repeats.

**Who else can be in that heap.** No thread of the retired sub-process
exists: its interpreter has been unlinked after its last thread state
was deleted. With 7.2's rejection in force, every other thread of the
shared domain belongs to an interpreter that shares the GIL the
collecting thread holds, so none of them runs, allocates or reclaims.
Threads of isolated interpreters, under their own GILs, do run; they
touch the retired heap's segments only through the shared arena
bitmaps, by clearing and re-setting a bit whose segment belongs to
another sub-process, and the arena lock above excludes that. So at the
absorption nothing but the collecting thread is in that heap.

After it the segments are ordinary abandoned segments of the main
interpreter's sub-process: the walk visits them as it visits any exited
main thread's, the owner lookup gives the main interpreter, and main
threads reclaim them through mimalloc's own path, which frees empty
pages and segments. No table of retired sub-processes exists, so the
domain cap of 64 counts live interpreters only.

**The main interpreter's deletion** frees the adopted states (their
immortal arrays), drops the kept-thread references
(`decref_threadstate()`), and frees its own state;
`Py_Initialize()` after `Py_FinalizeEx()` starts from a fresh state in
`_PyGC_Init()`, which resets the phase.

**What remains.** A daemon thread of a deleted interpreter that is still
alive, hung in `take_gil()`, keeps its own heap, which it never
abandons; its frames and stack are scanned through the adopted kept
threads, but its pages are walked by no one, as today. That is
recorded in the hack register for step 7 with its condition: a daemon
thread of a legacy subinterpreter hung at that interpreter's deletion.

### 5.4 An isolated interpreter, and the never-opened case

An isolated interpreter (its own heap) is collected only by itself, so
after its last collection nothing reaches its queue; the close drains
and finds it empty. Its state is freed at deletion and its sub-process
left as today. A registered root it never removed is relinked to the
main interpreter like a shared-heap interpreter's, so that a later
removal does not touch freed memory; its pointers lead outside every
domain and the mark ignores them.

An interpreter whose initialisation failed is torn down by
`new_interpreter()`'s error path with its queue never opened, and its
calls cannot run on it (that is finding B). Its close does not drain
and runs no free function (5.2). For a shared-heap one, the close
splices the queue and the waiting list onto the main interpreter's, as
the owner rule then says. For an isolated one they are discarded: no
collection will visit that heap again, so no deallocator runs for their
objects and nothing a call would have done happens later by another
route; the objects leak with the heap, as the blocks an isolated
interpreter leaves today do. The discarded calls are counted by kind in
the main interpreter's live state, which outlives every subinterpreter,
readable as `_testinternalcapi.gc_queue_info()["discarded"]`, and a line
goes to the C stderr when `gc.DEBUG_UNCOLLECTABLE` is set. The leak of
an isolated interpreter's remaining heap at deletion predates 7a and is
handed to step 7 (a deleted isolated heap has no roots left, so a
collection of it alone, with its resource-owning deallocators, would
free it).

The main interpreter's close drains like any other. By then every other
interpreter has been deleted (`finalize_subinterpreters()`), and the
main interpreter's other threads hang in `take_gil()`, so nothing can
arrive after it; the close asserts an empty queue and waiting list.

### 5.5 The fork child

`_PyInterpreterState_DeleteExceptMain()` deletes every other
interpreter in the child, with no thread state current and under the
head lock, through `PyInterpreterState_Clear()` and `free_interpreter()`.
That path is broken today independently of the queues:
`interpreter_clear()` asserts and uses a thread state, `PyThreadState_Clear()`
wants a current thread of the same interpreter, the clearing can notify
watchers, and deleting thread states takes the head lock the caller
holds. The child replaces it with a dedicated disposal,
`dispose_after_fork(interp)` (pystate.c), which runs no Python code,
calls no watcher, destructor or free function, and takes no lock the
caller holds:

- the interpreter's thread states are unlinked and freed without
  `PyThreadState_Clear()`; their mimalloc heaps are abandoned into the
  interpreter's sub-process, the mimalloc-only part of
  `_PyThreadState_ClearMimallocHeaps()`, which the child already applies
  to the main interpreter's dead threads;
- `_PyGC_Fini()` is replaced by its allocation-free close: no drain, the
  metadata cleared without calls (5.2's failure step), then for a
  shared-heap interpreter the adoption of 5.3 (its calls, frames and
  objects become the main interpreter's garbage and run there as the
  owner rule says), and for an isolated one the discard of 5.4;
- nothing else of the interpreter is cleared: its dicts, modules and
  caches are its heap's objects, adopted or leaked with it;
- the interpreter struct and, for an isolated interpreter, its
  sub-process are freed as `free_interpreter()` does.

The extras that the child clears without calls leak; their free
functions belong to extensions of an interpreter that does not exist in
the child, and running them there is what the disposal must not do. This
is permanent behaviour of the fork child, documented in gc.rst.

## 6. The existing deferrals

- **`_PY_SIGNALS_PENDING_BIT`.** `run_queue()` returns without draining
  when a signal is pending and the thread can handle signals, and the
  breaker handles the signal first (test_io.test_signals, 5b). Only the
  main thread of the main interpreter handles signals
  (`_Py_ThreadCanHandleSignals()`), so the deferral concerns only the
  main interpreter's queue drained on its main thread, which is unchanged.
  A subinterpreter's drain never defers for signals, as now.
- **Trace, profile and monitoring.** `_PyGC_LiveRunQueue()` drains the
  installing thread's own interpreter's queue. A trace function is per
  thread, and monitoring per interpreter, so another interpreter's calls
  were never the installer's to run untraced; they run untraced on their
  owner unless its own thread has a trace function, as under reference
  counting.
- **The tracer rule.** A thread inside a `PyRefTracer` call cannot
  collect (`in_tracer`, `can_collect()`), so it neither queues nor
  drains. Unchanged.
- **gc.callbacks.** `invoke_gc_callback()` runs the collecting
  interpreter's `gc.callbacks` at "start" and "stop". They report the
  collections that interpreter ran, whatever the domain; another
  interpreter's callbacks are not told of a collection it did not run.
  They are Python code, so they run only while the collecting
  interpreter's queue is open (`live_python_ok()`: not `no_python` and
  open). The collector's debug output (`debug_write()`) uses the
  predicate and goes to the C stderr when it is false.
- **`gc.collect()` in a subinterpreter.** It collects the domain and
  returns the domain's count of unreachable objects, as now. When it
  returns, the callbacks and finalizers of its own interpreter's garbage
  have run (with the second pass of an explicit collection, counted
  over its own finalizers); the other owners' calls have been spliced to
  their queues and run at their safepoints. In the main interpreter the
  same holds with the roles swapped: a legacy subinterpreter's garbage
  that the main interpreter's collection found runs on the
  subinterpreter, at the latest at its close (5.1). `gc.get_stats()`
  counts the collections each interpreter ran, over its domain; gc.rst
  says so.
- **Reentrant collection.** A call that triggers a collection, or calls
  `gc.collect()`, adds to the tail of the queue being drained, and the
  drain runs it, as now. A call that creates a subinterpreter starts
  that interpreter's initialisation inside the drain: its collections
  splice the draining interpreter's calls to the draining queue's tail,
  and its own queue stays closed until its `pycore_interp_init()` ends.

## 7. Domains

### 7.1 What a collection in X may run

| X | Domain | X's collection marks and sweeps | X's thread runs |
|---|---|---|---|
| own heap | X alone | X's heap | X's calls (all calls found are X's) |
| shared heap (main or legacy) | main and every live shared-heap interpreter, with the adopted heaps inside main's sub-process | all of them | X's calls only; the others' are spliced to their queues |

For an interpreter with its own heap nothing changes except the opening
and the close, since every call it finds is its own. The domain is
capped at 64 live interpreters (`trace_t.domain`); past that the mark
fails and the collection frees nothing, as now, which the note records
rather than changes (handed to step 7).

### 7.2 One GIL per domain (ruled)

The domain mark scans the stacks of the other domain threads from their
detach records and sweeps their objects. That is sound only if none of
them runs during the collection. With the main GIL shared, every other
domain thread is detached while the collector holds it. But
`init_interp_settings()` accepts `use_main_obmalloc` together with
`gil=OWN_GIL` (test_capi.test_misc `InterpreterConfigTests` expects the
all-true row to work), and the free-threaded build's stop-the-world went
with it (`_PyEval_StopTheWorldAll()` is empty, pystate.c). Such an
interpreter's thread runs Python code under its own GIL while another
domain member marks and sweeps the heap it allocates from, and the
queues of section 3 would be written from two GILs.

`init_interp_settings()` rejects the combination: "an interpreter that
shares the main interpreter's object allocator must share its GIL",
which `new_interpreter()` reports as `InterpreterError` through the
`_interpreters` module. The all-true row of `InterpreterConfigTests`
moves to the expected failures, `Doc/c-api/subinterpreters.rst` says so
under `use_main_obmalloc`, and the break is a row of PROCESS.md section
0.2, the intentional compatibility breaks (ruled). With it, every
queue operation runs under the domain's one GIL and the queues need no
lock.

**Handed to step 7.** The rejection can be lifted when a domain
collection stops every domain thread, whichever GIL it holds: a
stop-the-world across the domain's GILs (each member's eval breaker
asked to suspend, as `_PY_EVAL_PLEASE_STOP_BIT` did), and queue
operations that do not rely on one GIL. Step 7, which decides domains
and collection scheduling, owns that question.

### 7.3 Threads of other interpreters in the same domain

They are detached while the collection runs (7.2). The mark scans their
stacks from their detach records (note 11 question 16) and their
interpreters' queues as roots (3.3). The calls the collection splices to
their interpreters run when one of their threads next takes the GIL
(section 4). A thread of the main interpreter blocked in C on a lock
that a subinterpreter's thread will release does not drain until it
returns; nothing in the core waits for a callback of another
interpreter, and the tests of section 9 run the interpreter and
threading suites to find any that does.

## 8. Invariants

Each with the test of section 9 that proves it.

1. **A call runs on its owner.** Debug builds assert in `run_queue()`
   that `_PyGC_LiveOwner()` of the call's object (recomputed through
   the segment, with the close applied) is `tstate->interp`. Tests 9.2,
   9.3, 9.4, 9.6, 9.10.
2. **No call runs while its queue is not open**, and no gc.callbacks or
   Python-level debug output while the collecting interpreter's is not.
   Tests 9.1, 9.2 (the B paths under stress and kill).
3. **A queued object stays intact until its call runs**, whichever
   interpreter's collection found it and however many collections
   intervene. Tests 9.4 and 9.10 under `PYTHON_GCVERIFY=kill`.
4. **Each call runs at most once, and is lost only in 5.4's discard**: a
   node is spliced, never copied, and routing cannot fail. Tests 9.4,
   9.5, 9.11 (failure injection).
5. **Order is the batch's, within and across owners**: callbacks, then
   finalizers referrer first, then legacy, then destroy hooks, and no
   call runs before an earlier call of another owner that reaches its
   object (3.5). test_gc's `test_finalizer_order` and
   `test_dealloc_reads_referents`, 9.10 and 9.14.
6. **A queue with work and an open phase is drained at its owner's next
   safepoint**, also with automatic collection off. Test 9.12.
7. **The domain has one GIL.** Test 9.8.
8. **Every live block of a shared heap is traced and swept, its heap
   immortals' referents kept, its roots valid**, also after its
   interpreter's deletion. Test 9.7 under kill and ASan.
9. **Destruction metadata is read only through its owner's tables**,
   every index on an object is its owner's, and each extra is freed
   exactly once (`_PyCode_ClearExtras()` detaches before it frees).
   Tests 9.9 and 9.5 (c).
10. **No interpreter a batch names is freed before the splice** (the
    pin, 3.2). Test 9.16.
11. **A legacy `tp_del` runs once per death**, however long its call
    waits (3.6). Test 9.15.
12. **The fork child's disposal runs no code of a deleted interpreter**;
    a shared-heap interpreter's calls run afterwards on the main
    interpreter, as the owner rule says. Test 9.13.

## 9. Tests

New tests go in test_gc (`InterpreterQueueTests`), in child processes
where they set `PYTHON_GCSTRESS` or `PYTHON_GCVERIFY`. "Records its
interpreter" means the callback appends `_interpreters.get_current()`'s
id to a list or writes it to a pipe. Every weak reference whose callback
a test expects is kept alive outside the garbage it watches, since a
dead weak reference's callback does not run.

1. **The creator paths under stress, exclusions removed.**
   `test_stress_shared_heap_callbacks` without its skip; the stress line
   on test_capi at `PYTHON_GCSTRESS=1000` with only test_bytes'
   `*test_resize*` excluded (10a rung 3's writer row): every creator of
   note 11 8.2's B class runs (`SubinterpreterTest`,
   `TestThreadState.test_gilstate_matches_current`,
   `InterpreterConfigTests`, `InterpreterIDTests`,
   `TestPendingCalls.test_isolated_subinterpreter`, and the six
   `_testcapi.test_*` functions that call `Py_NewInterpreter()` through
   `Test_testcapi`). test_misc alone five times on both builds;
   test_getargs `test_gh_119213` stays in the stress set as the
   own-heap control.
2. **Main garbage while a legacy subinterpreter initialises.** The main
   interpreter disables automatic collection, builds 200 cycles each
   holding an object with `__del__`, keeps a weak reference with a
   callback to each cycle in a list, both recording their interpreter,
   drops the cycles, then creates twenty legacy subinterpreters
   (`_testcapi.run_in_subinterp`) under `PYTHON_GCSTRESS=1000`, and once
   more on debug with `PYTHON_GCVERIFY=kill` added. Pass: exit 0, 200
   callbacks and 200 finalizers ran, every recorded id is 0.
3. **`gc.collect()` in a subinterpreter.** As 9.2 without stress, then
   one legacy subinterpreter makes its own watched cycle and calls
   `gc.collect()`. Pass: inside the subinterpreter, its own callback has
   run when `gc.collect()` returns and none of the main interpreter's
   has; back in the main interpreter, after one statement, all of the
   main interpreter's have run with id 0. The mirror: a persistent
   legacy interpreter's garbage found by the main interpreter's
   `gc.collect()` does not run on the main interpreter.
4. **Teardown with a non-empty queue.** A persistent legacy interpreter
   (`_interpreters.create` with the legacy config) builds a cycle whose
   `__del__` writes its interpreter id to a pipe, and returns. The main
   interpreter calls `gc.collect()` twice: the pipe stays empty and
   `_testinternalcapi.gc_queue_info(id)["length"]` is non-zero. Then
   `_interpreters.destroy()`: the pipe holds the subinterpreter's id.
   Under kill on debug as well.
5. **The close.** (a) A legacy interpreter
   whose shutdown finalizer releases the GIL while a main-interpreter
   thread collects garbage of the closing interpreter: the call runs on
   the closing interpreter, at its close. (b) `_testinternalcapi`'s
   `gc_queue_never_open_for_test()` creates an isolated interpreter,
   queues a callback in it before the opening and ends it through the
   failure path: `discarded["callback"]` rises by one and the process
   exits 0; the shared-heap variant's call runs on the main interpreter.
   (c) Retirement under reentry: a code-extra free function of the
   closing interpreter calls `gc.collect()`, releases the GIL to a
   main-interpreter thread that collects, queues a weak reference
   callback, and tries to set a new extra (refused, `SystemError`
   reported unraisable); the close ends with an empty queue, every
   tagged code object's extras freed exactly once, and
   `gc_queue_info()["retired"]` equal to the number of tagged code
   objects.
6. **Reentrancy.** A main-interpreter weak reference callback that
   creates a legacy subinterpreter (`_testcapi.run_in_subinterp("x =
   1")`), and another that calls `gc.collect()`, under
   `PYTHON_GCSTRESS=1000`. Pass: exit 0, every callback ran once, on
   interpreter 0.
7. **Adopted heaps.** (a) A legacy subinterpreter stores a fresh list in
   a C static of `_testinternalcapi` (`stash_set()`) and ends; the main
   interpreter appends a fresh object to the stashed list, drops its own
   reference, collects under kill, and reads the element back. Fails
   today (the element is killed). (b) A legacy subinterpreter interns a
   thousand fresh strings into the main interpreter's dictionary and
   ends; the main interpreter collects under kill and reads them through
   `sys.intern()`. (c) A legacy subinterpreter makes a heap immortal
   (a new `_testinternalcapi.gc_add_immortal()`, which calls
   `_PyGC_LiveAddImmortal()`) of a list holding a fresh
   object, stashes it, ends; the main interpreter collects under kill
   and ASan and reads the object. (d) A legacy subinterpreter registers a
   `_PyGCRoot` (new `_testinternalcapi.gc_root_add()` and
   `gc_root_remove()`) over a fresh object
   and ends; the main interpreter collects, reads it, then removes the
   root (ASan clean). (e) 100 create, use and destroy cycles of legacy
   subinterpreters, each leaving a stashed object: no collection fails
   (the cap), the pending list empties, and RSS after the hundredth is
   recorded against the tenth.
8. **One GIL per domain.** test_capi.test_misc `InterpreterConfigTests`:
   the all-true row raises `InterpreterError`; the other rows unchanged.
9. **Destruction metadata on the owner.** (a) Colliding watcher ids: the
   main interpreter and a persistent legacy interpreter each add a dict
   watcher, getting the same id, with callbacks that record their
   interpreter; each watches a dict of its own in a cycle; the other
   interpreter collects. Pass: each DEALLOCATED event reaches its own
   interpreter's watcher, on that interpreter. (b) Colliding code-extra
   indices (`PyUnstable_Eval_RequestCodeExtraIndex()` through ctypes,
   as test_code's `CoExtra` tests do) with distinct free functions: a code
   object of the subinterpreter with an extra dies in a main-interpreter
   collection and its own free function runs; one that survives the
   subinterpreter's deletion has its own free function run at the close,
   and the main interpreter's free function never runs for it. (c)
   Foreign registration: the subinterpreter's `PyDict_Watch()` and
   `PyType_Watch()` with a non-reserved id on a main-interpreter dict
   and heap type raise `ValueError`, its `PyUnstable_Code_SetExtra()` on
   a main-interpreter code object raises `SystemError`, and the reserved
   module watcher still watches a main-interpreter globals dict that the
   subinterpreter's specializer reaches. (d) Retirement while garbage:
   a tagged code object of the closing interpreter that is garbage when
   the restart loop reaches it, whose free function runs `gc.collect()`:
   the free function runs exactly once, no destroy hook runs for the
   object's extras, and the close ends normally (under kill and ASan).
10. **Mixed-owner ordering.** A batch of referrer-first finalizer pairs
    from two interpreters with entries already queued on both, grown
    past a thousand nodes, with a finalizer that releases the GIL
    (`time.sleep(0)`) while a second thread of the same owner joins the
    drain, and a nested `gc.collect()` inside one call. Pass: each
    owner's order is its collection order, every call ran once.
11. **Allocation failure.** a failure injected at each node allocation
    (`_testinternalcapi.gc_queue_fail_alloc(n)`, failing the n-th) of a collection with three owners: no call runs on
    another interpreter, no weak reference loses its callback, and the
    objects stay alive until their calls run.
12. **Progress without collection.** Calls spliced to the main
    interpreter while it has `gc.disable()`, a zero threshold and,
    separately, during its finalisation: they run at its next breaker
    check, and in a C loop that calls `PyErr_CheckSignals()`
    (new `_testinternalcapi.check_signals_loop(n)`) without another GIL
    acquisition.
13. **The fork child.** A process with a persistent legacy interpreter
    holding queued calls and stashed objects, and an isolated one, forks;
    the child exits 0, reads the stashed objects under kill, and runs no
    code of either deleted interpreter during the disposal: a recording
    dict watcher, a code-extra free function and a capsule destructor
    registered by the deleted interpreters record nothing until the
    disposal returns, and afterwards only the shared-heap interpreter's
    queued calls run, on the main interpreter.
14. **Split-owner buffered write.** A persistent legacy interpreter
    wraps a `FileIO` that the main interpreter opened and stashed
    (`stash_get()`) in a `BufferedWriter` and a `TextIOWrapper`, writes
    a line without flushing, and drops them with no cycle, as does the
    main interpreter; the main interpreter collects several times while
    the subinterpreter is idle (the file stays open:
    `gc_queue_info()["waiting"]` counts the `FileIO`'s finalizer), then
    runs the subinterpreter once, then collects. Pass: the file holds the
    line exactly once, under kill on debug as well. The same with the
    owners swapped.
15. **Legacy delivery marker.** An acyclic object with a legacy `tp_del`
    (a class made with `_testcapi.with_tp_del()` whose
    `__tp_del__` counts its calls) owned by an
    idle legacy interpreter dies; the main interpreter collects five
    times; the subinterpreter runs, delivering `tp_del` once; then five
    more collections. Pass: the counter is 1.
16. **The pin.** A debug-only hook
    (`_testinternalcapi.gc_pause_after_mark()`) makes the next collection
    of the main interpreter wait between its mark and its splice on an
    event, while a second thread deletes a legacy interpreter whose
    garbage the mark found. Pass: the deletion does not unlink until the
    event is set and the collection has spliced; the calls run on the
    main interpreter (the closed owner) under ASan.
17. **Every kind routed.** For each call kind (weak reference callback,
    PEP 442 finalizer, legacy `tp_del`, a capsule destructor, a module's
    `m_free`, a buffer release with `__release_buffer__`) and each
    direction (main garbage found by a subinterpreter's collection, and
    the reverse): the call runs once on the owner. A variant keeps the
    object alive by a conservative C-stack word for one collection
    (`_testinternalcapi.gc_hold_on_stack()`) before it dies. A weak
    reference owned by B to an object owned by A delivers its callback
    on B.
18. **Absorption under concurrency.** Four threads of two isolated
    interpreters allocate and free small and huge (OS-segment) objects
    in a loop, abandoning and reclaiming segments, while the main
    interpreter creates, fills and deletes 200 legacy interpreters and
    collects after each; under ASan and on debug. Pass: no abort, every
    pending sub-process deleted, `gc_queue_info()["absorbed"]` equals
    the deletions that left segments.

The gate set of the rung (work doc 7a-1.md) is these, test_gc,
test_weakref, test_finalization, test_capi, test_code, test_threading,
test_interpreters, test__interpreters,
test_concurrent_futures.test_interpreter_pool, test_sys, test_atexit,
test_io and test_os, then the ladder of PROCESS.md section 3 through
level 3 with the collector verification set.

## 10. What it deletes and what it hands on

Deleted:

- the collecting-interpreter rule: `run_queue()` on the collecting
  thread for every call the domain's collection found, and the comment
  at the head of gc_live.c that states it;
- the queue array, its memmove and the `running_queue` count of
  `struct _gc_live_state`;
- the skip of test_gc `test_stress_shared_heap_callbacks`;
- the B class of note 11 section 8.2 and its stress exclusion of
  test_capi.test_misc (in the verification manifest once PROCESS.md
  section 0.1's manifest exists; the coordinator's), 10a-2.md's accepted shared-heap
  failures, and the hack register's shared-heap row of note 08, each
  with this note as the retirement;
- the acceptance of `use_main_obmalloc` with `OWN_GIL`, and of
  watcher and code-extra registration on another interpreter's object;
- the fork child's `PyInterpreterState_Clear()` of other interpreters,
  replaced by the disposal of 5.5;
- `_PyCode_HasExtraFree()`'s reading of the current interpreter's table:
  it takes the interpreter as a parameter, and its one caller passes the
  owner.

Handed to step 7 (the generational collector):

- The queues and the routing. A minor collection batches and splices
  calls the same way; the owner lookup is per segment and so holds for
  young objects; the queues and the adopted states are roots that every
  minor collection visits in full (note 11 section 11's list already
  names "the finaliser queue").
- Stop-the-world across a domain's GILs, the condition for lifting 7.2.
- The domain cap of 64 live interpreters.
- A hung daemon thread's heap after its interpreter's deletion (5.3).
- The heap an isolated interpreter leaves at deletion (5.4), with its
  condition: a collection of the deleted heap alone, which has no roots.

Handed to 10b: a moving collector keeps the property that a block's
segment names its owner: a nursery is per thread, so per interpreter,
and evacuation copies into a segment of the object's own sub-process.

## 11. Measurements

Recorded, not gated (note 06 plan revision of 2026-10-27).

- **One interpreter.** The internal set's memory group (`collector.py`
  at L = 16, 128 and 512 MB, `startup.py`, `imports.py`) and the
  kernels, `instr` and `rss`, against the integration head the rung
  starts from. A one-interpreter process does no lookup and no splice;
  the node allocation replaces the array's amortised growth; the
  expected change is nil and any change is attributed. `-c pass`
  instructions, since calls queued during initialisation now wait for
  the opening.
- **The GIL check.** The pipe-read loop of note 11 question 7 (10^6
  one-byte `os.write` and `os.read` pairs, 2·10^6 GIL releases),
  instructions before and after.
- **A shared domain.** `boil/tools/measure/queue_latency.py` with K = 1,
  4 and 16 live legacy interpreters, each idle with Q = 0, 1,000 or
  100,000 queued calls: the main interpreter's collection pause and
  instructions per collection on `collector.py`'s L = 16 workload, the
  splice counter (`moved`, in the root stats), and RSS; then after 100
  create and destroy cycles (9.7 e), RSS and the mapped segments of the
  main sub-process.
- **Node cost.** A burst benchmark (`queue_latency.py --burst`):
  10^6 objects each with a cheap weak reference callback, and 10^5
  capsules with a C destructor, made garbage at once; instructions per
  call for queueing and draining, and peak RSS, against the baseline's
  array queue.
- **Waiting across owners.** In 9.14's shape with 10^4 writer and file
  pairs: how many calls wait (`waiting`), the dependency pass's
  instructions per collection, and the order table's size.
- **Retirement.** The close of a legacy interpreter with 10, 10^3 and
  10^4 tagged code objects (extras set through ctypes, as 9.9 (b)); the
  10^4 case is dynamo-shaped, a tool that tags every code object it
  compiles, over a heap of 10^6 objects. Recorded: `retired`, the walks'
  instructions, and the close's time. This is the measured limit of the
  restart loop, O(heap × extras); if it is bad, the remedy is a later
  rung's single form, not a second path.
- **Teardown.** The time and instructions of the close of a legacy
  interpreter (the drains, the metadata walk) and of its deletion
  (adoption), separately, over 9.7 (e)'s cycles; and the time of the
  absorption at the next collection.
- **Callback latency.** (a) In one interpreter, nanoseconds from the
  start of `gc.collect()` to a weak reference callback, expected
  unchanged; (b) a legacy subinterpreter that collects main-interpreter
  garbage and returns: nanoseconds from the collection to the callback
  on the main interpreter, and the bytecodes the main interpreter runs
  before it.

## 12. Rungs

Two rungs (ruled).

**Rung 1 (7a-1)**: the node queue and the batch, the owner lookup, the
phases and the drain permission, the opening and the close's drain,
routing, the pin, `queue_ready` and the `take_gil()` check, delivery at
execution, and the configuration rejection. It ends finding B and lifts
the stress exclusion of test_capi.test_misc. At its landing invariants
1, 2, 3, 4, 6, 7, 10 and 11 hold. Until rung 2, the close's publish
maps a closed shared-heap interpreter's calls to the main interpreter,
whose tables then read the closed interpreter's destruction metadata,
as every collection does today; and order holds within each owner only.

**Rung 2 (7a-2)**: order across owners (3.5), metadata by owner with
the registration refusal (3.4), the close's retirement and publish
(5.1, 5.2), adoption and absorption (5.3, 5.4), the fork child (5.5),
and the latency tool. At its landing invariants 5 (across owners), 8, 9
and 12 hold as well.

Cut order, either rung: the measurement tool first, then 9.7 (e)'s RSS
record; nothing that an invariant of section 8 rests on is cut. The work
docs (7a-1.md, 7a-2.md) list the files.

## 13. Open questions

1. **`gc.get_stats()` in a domain.** Each interpreter's stats count the
   collections it ran, over the domain. Recommendation (default unless
   ruled): keep; document in gc.rst beside the ownership rule.
2. **A main thread blocked on a subinterpreter's callback.** No core
   path is known (7.3). Recommendation: none until a test shows one;
   then the fix is that test's.
3. **The registration refusal (3.4) is a second compatibility break.**
   Ruled: accepted, recorded in PROCESS.md section 0.2 with 7.2's.
4. **File ownership for the refusal.** Ruled: Objects/dictobject.c and
   Objects/typeobject.c shared append-only (one guard hunk each under a
   7a comment); Include/internal/pycore_code.h, Doc/c-api/type.rst and
   Doc/c-api/code.rst are 7a's; Objects/codeobject.c stays shared.

## Review

### Round 1 (codex, 2026-10-28 09:59 PDT; reviews/13-interpreter-queues-review-1.md): revise

Seven blocking issues, all accepted; each changed the design.

1. *Dropping a destroy call does not cancel it* (`code_dealloc()` and
   the buffer release still run their code in a later sweep; legacy and
   non-GC finalizer flags reset on reachability). Changed: the design no
   longer drops calls of an interpreter whose heap is collected again.
   The close drains (5.1); later arrivals go to the main interpreter,
   which owns the heap from the close (decision 5); a never-opened
   shared-heap interpreter's calls are spliced there (5.4). The one
   discard left is an isolated interpreter's, whose heap no collection
   visits again, so no deallocator can run the cancelled work later
   (5.4). Test 9.5 covers the close's drain and the discard; 9.4, 9.10
   and 9.13 run every kind through later collections, 9.2 under kill
   with the conservative stack words that stress leaves.
2. *Adoption loses the roots that trace the blocks* (heap immortals'
   registry freed in `_PyGC_Fini()`; `_PyGCRoot` nodes). Changed: the
   live state is kept past the close and adopted whole at deletion, its
   immortal registry and kept threads visited by the main interpreter's
   marks, its roots relinked (5.3); §2 now names heap immortals. Tests
   9.7 (c) and (d).
3. *Retired segments have no reclamation path; the retired table
   shares the 64 cap.* Changed: no retired table. The next collection
   absorbs the deleted interpreter's sub-process into the main
   interpreter's (`_mi_subproc_absorb()`, lock order stated), after
   which mimalloc's ordinary reclaim frees empty pages and segments
   (5.3). Test 9.7 (e) runs 100 cycles and records RSS and mappings.
4. *Allocation ownership does not establish metadata ownership*
   (`_PyCode_HasExtraFree()` and the watcher masks read the current
   interpreter's tables; adopted indices; foreign watchers). Changed:
   discovery takes the owner (3.4); the close retires the indices of
   surviving objects with the owner's tables (5.2), and the fork child
   clears them without calls (5.5); the rule for watching a foreign dict
   is stated and documented (3.4). Section 3.4 is no longer cuttable
   (12). Test 9.9 (a) and (b).
5. *The GC bit can be consumed without draining.* Changed: collecting
   and draining are separate permissions in `_Py_RunGC()` (section 4).
   Test 9.12.
6. *Routing has no allocation-failure contract.* Changed: the queue is
   a list of nodes, routing is a splice that cannot fail, and the
   callback node is allocated before its weak reference is cleared
   (3.1, 3.2). Test 9.11.
7. *Routing after `_PyGC_Fini()` is undefined; `get_live()` could
   recreate state.* Changed: the close is a phase kept until deletion,
   the owner lookup maps a closed interpreter to the main interpreter,
   and `get_live()` never recreates a closed state (5.1). Test 9.5 (a).

Non-blocking, all taken: 5.1 now separates the `finalize_modules()`
collections (one pass, globals intact) from the last one (sys and
builtins wiped, error path structurally safe) and says which takes the
second pass; the test oracles keep weak references alive outside their
garbage (9 preamble, 9.2) and the adoption tests separate traversal,
immortals, roots and reclamation (9.7); mixed-owner ordering has its own
test (9.10); section 11 measures shared domains, idle queues, adoption
cycles and the GIL check; 3.3 states the transitive and cross-interpreter
retention and the idle owner's latency.

Questions: the owner is the current block's interpreter, with the
resize case stated (section 2); the fork child's no-code disposal is
named (5.5) and tested (9.13); the main interpreter's close drains and
asserts empty, and drop at the main close is no longer a question
(5.4).

### Round 2 (codex, 2026-10-28 10:22 PDT; reviews/13-interpreter-queues-review-2.md): revise

Seven blocking issues, all accepted; each changed the design.

1. *Cross-owner order loses buffered output* (a wrapper in an idle
   subinterpreter over a raw file in main). Changed: a call waits for
   the earlier foreign finalizer and legacy calls that reach its object,
   the old per-owner computation recorded here is superseded by 3.5's
   same-owner completion requirement and rung 2a's graph review. No
   interpreter waits on another synchronously; allocation failure takes
   the single rollback path (3.5, decision 9, invariant 5). Test 9.14
   checks the bytes written.
2. *The copied domain can name freed interpreters before routing*
   (`PyInterpreterState_Delete()` detaches before it unlinks).
   Changed: a collection pins its domain from the mark to the splice;
   deletion's unlink waits for no pin under the head lock; closed owners
   are canonicalised before the pin is dropped (3.2, decision 10,
   invariant 10). Test 9.16 orders mark, deletion and routing with a
   debug hook.
3. *An idle queue erases the legacy delivery marker.* Changed:
   `run_queue()` sets `_Py_GC_DEL_CALLED_FLAG` when it runs `tp_del`,
   not `queue_legacy()` when it queues it, so the walk's clearing of a
   pending object's flag loses nothing; the queue root keeps a queued
   object from being queued twice (3.6, invariant 11). Test 9.15.
4. *Allocation ownership does not establish metadata ownership* (a
   foreign code extra index, a foreign watcher bit). Changed:
   registration on another interpreter's object is refused
   (`PyDict_Watch()` and `PyType_Watch()` raise ValueError,
   `PyUnstable_Code_SetExtra()` SystemError), the reserved dict watcher
   ids excepted (3.4). Test 9.9 (c) registers foreignly. The refusal is
   a compatibility break and needs a ruling (13, question 3).
5. *Metadata retirement is not a defined transition.* Changed: 5.1
   gives the close in four steps and 5.2 the retirement: freeze new
   metadata on the closing interpreter, take a snapshot rooted by a span,
   retire, drain what the free functions queued, repeat until a pass
   finds nothing, then publish the closed phase; a snapshot allocation
   failure keeps the interpreter's tables and adopts them unretired. A
   never-opened interpreter runs no free function (5.2, 5.4). Tests 9.5
   (c) and (d).
6. *Absorption's locks do not protect arena enumeration.* Changed: the
   absorption takes each arena's `abandoned_visit_lock` before the OS
   list lock, retired sub-process before main, as mimalloc's own
   visitors do; with 7.2's rule a thread of another own-GIL interpreter
   in the domain has nothing in that heap (5.3). Test 9.18 runs isolated
   allocation and reclaim through repeated adoption, arena and OS
   segments.
7. *Fork-child disposal needs a thread state.* Changed: a dedicated
   `dispose_after_fork()` that needs no current thread, runs no
   callback, frees the states' memory under the held head lock, and
   leaks Python objects of the deleted interpreters (5.5, invariant
   12). Test 9.13 asserts disposal and the absence of forbidden calls.

Non-blocking: the test matrix routes every kind, including capsule,
module and buffer-release hooks, retention by a C-stack word, and a
weak reference owned by B on A's object (9.17); section 11 measures
node cost under a callback burst, the order pass, and teardown and
absorption separately; `_PyCode_HasExtraFree()` takes the interpreter
instead of gaining a twin (10, and the work document); file ownership is
in the work document, with the refusal's files left to the coordinator
(13, question 4).

Questions: the discard counters live in the main interpreter's state
(5.4); the main interpreter's deletion frees the adopted states and
releases their kept threads, every `Py_FinalizeEx()` cycle (5.3); the
fork child's leak is permanent and accepted, the isolated heap's is
handed to step 7 with its condition (10).

### Coordinator rulings (2026-10-28 10:50 PDT)

1. *No fallback forms.* The chain fallback of 3.5 and its counter are
   deleted: the order table is allocated before the first node and
   shares collect()'s one failure path with node allocation (3.2). The
   snapshot, its root and its failure path in 5.2 are deleted: one walk
   clears the watcher bits, then a restart loop retires one code
   object's extras per walk, so no walk is in progress while a free
   function runs; `gc_queue_info()["retired"]` counts the iterations.
   Two precisions kept each form single: the table is allocated before
   any node, since routing a partial batch unordered would lose a
   flush; `_PyCode_ClearExtras()` detaches the extras before calling
   any free function, so a collection inside one cannot free them
   twice.
2. *Compatibility breaks.* Both are accepted and recorded in PROCESS.md
   section 0.2.
3. *Two rungs* (section 12), each record naming the invariants that
   hold at its landing.
