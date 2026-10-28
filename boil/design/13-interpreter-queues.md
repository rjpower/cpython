# 13: Callbacks and finalizers run on the interpreter that owns them

Status: design, 2026-10-28, for step 7a (boil/PROCESS.md section 0,
moved ahead of step 7 on 2026-10-28). It retires finding B of note 08
(the hack register's shared-heap row) and note 11 question 18, and closes
two collector holes found while reading for it: a shared-heap
interpreter with its own GIL (section 7.2) and the heap of a deleted
shared-heap interpreter (section 5.3). Coordinator rulings of
2026-10-28 are marked "(ruled)".

## Decisions

1. **The owner of a queued call is the interpreter that allocated the
   object it is about**, read from the mimalloc sub-process of the
   object's segment: a weak reference callback belongs to the weak
   reference's interpreter, a finalizer, a legacy `tp_del` and a destroy
   hook to the dead object's (section 2, ruled).
2. **One queue per interpreter, as now, but filled by owner.** A
   collection appends its calls to the collecting queue's tail, as now,
   resurrects from them, and then moves each call owned elsewhere to its
   owner's tail, in collection order (section 3).
3. **A queue drains only on a thread of its owner**, at the safepoints
   that drain it today. A thread learns that its interpreter's queue has
   work when it takes the GIL (section 4).
4. **A queue is open from the end of its interpreter's
   `pycore_interp_init()` until `_PyGC_Fini()`.** A closed queue keeps
   its calls; gc.callbacks and the collector's Python-level debug output
   follow the same predicate. At `_PyGC_Fini()` the calls left are
   dropped with a record (section 5).
5. **The main interpreter adopts the heap of a deleted shared-heap
   interpreter**: its live blocks stay traced and swept, and their calls
   are the main interpreter's (section 5.3, ruled).
6. **A domain shares one GIL.** `use_main_obmalloc` with `OWN_GIL` is
   rejected at configuration with `InterpreterError`, an intentional
   compatibility break (section 7.2, ruled).
7. **Destroy hooks consult the owner's watcher tables** (section 3.4,
   ruled).

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
heap object names, exactly and for the object's whole life, the
interpreter whose thread allocated it. The owner of an object is that
interpreter; after the interpreter is deleted, the main interpreter
(section 5.3).

The owner of each kind of call:

| Kind | `live_call` | Owner |
|---|---|---|
| weak reference callback | `CALL_CALLBACK`, op the weak reference | the weak reference's owner: the interpreter where `weakref.ref(x, cb)` or `weakref.finalize` ran |
| finalizer (PEP 442) | `CALL_FINALIZE` | the dead object's owner |
| legacy `tp_del` | `CALL_LEGACY` | the dead object's owner |
| destroy hook (`run_destroy_hook()`) | `CALL_DESTROY` | the dead object's owner |

The lookup is `_mi_gc_segment_of(op)->subproc` against the sub-processes
of the domain the mark recorded, the same lookup `live_page_of()` makes
for every conservative word. It runs once per queued call, never per
dead object, and not at all when the domain has one member.

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
need no adoption at teardown (section 5.3), but a main-interpreter
thread could reclaim a live subinterpreter's segment, and the segment
would then name the wrong owner while that interpreter still runs. The
allocator's separation is what makes the rule exact.

**Failure cases of the chosen rule.**

- *An object made in one interpreter for another.* In a shared heap,
  C code can allocate on interpreter X an object that interpreter Y
  then uses, and its finalizer runs on X. CPython's own code switches to
  the owner's thread state for such objects (`m_copy`), and under
  reference counting the finalizer of a shared object runs on whichever
  thread drops the last reference, which is less defined than this. The
  rule is recorded in the gc documentation (`Doc/library/gc.rst`, the
  section on subinterpreters).
- *Cross-interpreter order.* Finalizers run referrer first within one
  collection (note 06 16.2). The order now holds within each owner's
  queue; between two owners it is not defined. A referrer and referent
  pair that crosses interpreters is possible only in a shared heap, and
  PEP 442 promises no order for cycles anyway.
- *Static and immortal objects.* They are not in the object heaps, so
  the walk never lists them dead and no call is about one. A weak
  reference object is always a heap object. The lookup asserts a
  segment.

## 3. The queue

### 3.1 What it holds

The queue is `struct _gc_live_state`'s FIFO array, one per interpreter
(`interp->gc.live`), as now: `live_call {op, callback, kind}`, from the C
library's `malloc`, growing by doubling, compacted when no thread is
draining it. Nothing in the entry changes; the owner is implied by the
queue that holds it. One field is added to the state: `open` (section
5).

### 3.2 Filling by owner

A collection keeps its current order of work in `collect()`:
`clear_callback_weakrefs()`, `handle_finalizers()` and
`queue_destroy_hooks()` append to the collecting interpreter's queue
tail and resurrect from the range they appended, as now. After
`queue_destroy_hooks()` and before the sweep, `route_calls()` walks the
range this collection appended, `[first, qtail)`, moves each call whose
owner is another interpreter to that owner's tail, and compacts the
rest in place. Relative order is kept in every queue. Nothing runs
between the append and the move: the move precedes the sweep, the
"stop" gc.callbacks and the drain. When the domain has one member
(`_PyRuntime.interpreters.head->next == NULL` or the collecting
interpreter has its own heap) the move is skipped.

The domain is the mark's. `_PyGC_TraceLiveMark()` records it in the
mark result (the interpreters, their sub-processes and the retired
sub-processes of section 5.3) and `_PyGC_LiveOwner(res, op)` (gc_trace.c,
beside `live_page_of()`) answers the lookup. No interpreter of the domain
can be deleted between the mark and the move: deletion needs the
domain's one GIL (section 7.2), which the collecting thread holds.

### 3.3 Memory and bounds

Every interpreter of the domain is a root holder for its own queue: the
mark's precise phase calls `_PyGC_LiveVisitRoots()` for each domain
interpreter (`live_mark_roots()`), which visits each queued op and
callback. A queued object and what it reaches therefore stay alive until
the call runs, whichever interpreter's collection found it, and each
object is queued at most once: a queued object is reachable, so a later
collection does not list it dead again.

Bounds. A queue grows only with garbage its owner allocated. A busy
interpreter drains at its next safepoint, as now. An idle one (no
attached thread) keeps its calls, and the objects they resurrect, until
a thread of it runs or it is deleted; that memory is bounded by what the
idle interpreter allocated and others have since dropped. No collection
of another interpreter waits for it.

### 3.4 Destroy hooks (ruled)

Whether a dead dict or type has a destroy hook depends on the watchers
of the interpreter whose watcher bits it carries, its owner's.
`_PyGC_TraceLiveMark()` builds the watcher masks of `struct live_walk`
from the collecting interpreter only, so in a domain collection a main
interpreter's extension watcher on a main dict is tested against the
subinterpreter's table. The masks become per domain member, indexed by
the walk's current domain entry (the walk visits the heaps interpreter by
interpreter, `live_walk_heaps()`), and the retired sub-processes use the
main interpreter's. The hook then runs on the owner, where
`_PyDict_SendEvent()` reads the same table.

## 4. When a queue drains

A queue drains only on a thread whose `tstate->interp` is its owner and
only while it is open. The drain points are today's, applied to the
thread's own interpreter:

- **The end of a collection the thread ran** (`collect_pass()`), unless
  the collection runs no Python code (`no_python`, an allocation's
  inline collection). It drains its own queue only; the calls moved to
  other owners do not run here.
- **The eval breaker's GC bit** (`_Py_HandlePending()` and
  `PyErr_CheckSignals()`, both calling `_Py_RunGC()`), which drains when
  no collection is due. This covers code that allocated past the trigger
  without reaching the eval loop.
- **`gc.collect()` and `PyGC_Collect()`** on that interpreter, through
  the end of collection.
- **The interpreter's shutdown collections**, `finalize_modules()` and
  `interpreter_clear()` (section 5).
- **Before a trace, profile or monitoring hook is installed**
  (`_PyGC_LiveRunQueue()`, Python/legacy_tracing.c), so that calls of
  objects that died before tracing began run untraced.

An allocation is a collection safepoint (note 11 section 4.2) but not a
drain point: since 5a, Python code runs only where the eval loop checks
its breaker (note 06 15.2).

**Telling the owner.** An owner whose queue gains calls from another
interpreter's collection is not running: the domain has one GIL and the
collecting thread holds it (section 7.2). Every thread takes the GIL
before it runs again, and `take_gil()` already refreshes the thread's
eval breaker from its interpreter's state
(`update_eval_breaker_for_thread()`, Python/ceval_gil.c). It also sets
the GC bit when its interpreter's queue is open and non-empty. That one
load per GIL acquisition replaces any cross-thread signal, and covers
threads that attach after the move (a thread state created later, or
the main thread state that `new_interpreter()` detached and reattaches
when the subinterpreter returns). The GC bit then reaches `_Py_RunGC()`,
which drains when no collection is due, as it does after an inline
collection now.

**Which thread.** Any thread of the owner, and every thread of it that
collects joins a drain in progress (note 06 16.6); the per-thread guard
`gc_trace.running_queue` stays.

**States in which a queue does not drain.**

| State | Where | What happens to calls |
|---|---|---|
| initialising | from `_PyGC_Init()` to the end of `pycore_interp_init()` | kept; at the opening, if the queue is non-empty, the GC bit is set on the thread so the next breaker drains |
| not running | no thread of the interpreter attached | kept; the next thread to take the GIL drains |
| finalising | `Py_EndInterpreter()` or `Py_FinalizeEx()` until `_PyGC_Fini()` | drained by the shutdown collections and the breaker; automatic collections stay off (`can_collect()`) |
| closed | after `_PyGC_Fini()` | the queue no longer exists; section 5 |

The opening is the end of `pycore_interp_init()` because that is where
an interpreter first has what a call and its error report need: its
types (the unraisable hook's struct sequence among them), `sys` with
`unraisablehook` and `stderr` slots, builtins, and importlib's core. It
is the same point for the main interpreter and for subinterpreters, and
for an embedder that stops after the core phase (`_init_main=0`).

## 5. Teardown

### 5.1 Before teardown, with the hook ready

An interpreter's teardown collects at `finalize_modules()` (twice) and
for the last time in `interpreter_clear()`, before `_PyGC_Fini()`, while
`sys`, `builtins` and the warnings state still work (their clearing
follows, Python/pystate.c). Each of those collections drains the
interpreter's own queue with a second pass (`collect_from_entry()`), as
now. Signals do not defer the drain during finalisation (`run_queue()`
tests `_PyInterpreterState_GetFinalizing()`). So every call queued for
the interpreter before its last collection runs on it, with its hook
ready. That is the answer for the normal case.

### 5.2 Calls left at the close: dropped with a record

At `_PyGC_Fini()` a queue can still hold calls in two cases:

- another interpreter's collection moved calls to it after its last
  drain (a shutdown finalizer released the GIL and a main-interpreter
  thread collected);
- the interpreter never opened: its initialisation failed and
  `new_interpreter()`'s error path tears it down.

Neither can run on its owner, whose `sys` and builtins are about to be
cleared, and running them on another interpreter is the defect this
note removes. They are dropped: a callback is cleared from its weak
reference (`wr_callback = NULL`), a finalizer's object is marked
finalized, and the legacy and destroy flags are already set at queueing.
The objects then die at a later collection without the calls, as objects
alive at the end of `Py_FinalizeEx()` do. The record: the main
interpreter's live state counts dropped calls by kind, exposed as
`_testinternalcapi.gc_queue_info()["dropped"]`, and with
`gc.DEBUG_UNCOLLECTABLE` set a line goes to the C stderr naming the
interpreter and the counts. At the main interpreter's own close the same
applies, and nothing outlives it to adopt the objects.

The design does not move such calls to the main interpreter. The
objects they concern are the closing interpreter's garbage, found
during its teardown; their code reads that interpreter's module globals
and builtins, which `interpreter_clear()` empties next.

### 5.3 The heap of a deleted interpreter (ruled)

`free_interpreter()` (pystate.c) deletes an interpreter's sub-process
only when no abandoned segment points to it. A shared-heap interpreter
can leave live blocks: strings it interned into the main interpreter's
dictionary (`has_shared_intern_dict()`, Objects/unicodeobject.c), and
any object it allocated that a main-interpreter object refers to. After
the deletion `live_domain()` no longer lists the interpreter, so
`live_page_of()` rejects addresses in its segments: such an object is
never marked, never traversed, never listed dead and never swept, and
nothing it refers to is kept alive through it. A main-interpreter
object reachable only through it is freed while it still holds the
pointer.

The fix: when a shared-heap interpreter is deleted with abandoned
segments left, its sub-process is retired to the main interpreter's live
state (`_PyGC_LiveRetireHeap()`, called from `free_interpreter()` in
place of leaving the sub-process behind). `live_domain()` adds the
retired sub-processes to the domain's sub-process table (they have no
interpreter), `live_page_of()` accepts their segments, and
`live_walk_heaps()` visits their abandoned blocks with
`mi_abandoned_visit_blocks()`. Their objects are the main interpreter's
(section 2). When a collection finds a retired sub-process with no
abandoned segment left, it deletes it (`mi_subproc_delete()`). No thread
reclaims a retired segment, since no thread has that sub-process, so
the blocks are freed by the sweep and the segment's memory returns when
it is empty; a partly used retired segment stays mapped, which is
recorded as the cost. The heap walk already visits the abandoned
segments of live interpreters' exited threads, with mimalloc collecting
each page's cross-thread frees before the visit (Objects/mimalloc/heap.c),
so a retired segment is walked by the same path. A fork child deletes
the other interpreters without finalising them
(`_PyInterpreterState_DeleteExceptMain()`, which calls
`free_interpreter()` under the head lock): their shared heaps are retired
the same way, so `_PyGC_LiveRetireHeap()` takes no lock of its own, and
their queues are dropped with the record of 5.2. An interpreter with its own heap is not retired:
no reference into its heap can exist after its last collection, and its
remaining blocks are those of dropped calls, which leak as they do
today.

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
  interpreter's queue is open, with the same predicate as the drain
  (`live_python_ok()`: not `no_python` and open). The collector's debug
  output (`debug_write()`) uses the predicate too and goes to the C
  stderr when it is false.
- **`gc.collect()` in a subinterpreter.** It collects the domain and
  returns the domain's count of unreachable objects, as now. When it
  returns, the callbacks and finalizers of its own interpreter's garbage
  have run (with the second pass of an explicit collection, counted
  over its own finalizers); the other owners' calls have been moved to
  their queues and run at their safepoints. In the main interpreter the
  same holds with the roles swapped: a legacy subinterpreter's garbage
  that the main interpreter's collection found runs on the
  subinterpreter, at the latest at its teardown (5.1).
- **Reentrant collection.** A call that triggers a collection, or calls
  `gc.collect()`, appends to the tail of the queue being drained, and the
  drain runs it, as now. A call that creates a subinterpreter starts that
  interpreter's initialisation inside the drain: its collections move the
  draining interpreter's calls to the draining queue's tail, and its own
  queue stays closed until its `pycore_interp_init()` ends.

## 7. Domains

### 7.1 What a collection in X may run

| X | Domain | X's collection marks and sweeps | X's thread runs |
|---|---|---|---|
| own heap | X alone | X's heap | X's calls (all calls found are X's) |
| shared heap (main or legacy) | main, every shared-heap interpreter, the retired sub-processes | all of them | X's calls only; the others' are moved to their queues |

For an interpreter with its own heap nothing changes except the opening
and the close, since every call it finds is its own. The domain is
capped at 64 interpreters (`trace_t.domain`); past that the mark fails
and the collection frees nothing, as now, which the note records rather
than changes.

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
under `use_main_obmalloc`, and the break is listed in PROCESS.md section
0.1's contract list (the coordinator's row). With it, every queue
operation runs under the domain's one GIL and the queues need no lock.

**Handed to step 7.** The rejection can be lifted when a domain
collection stops every domain thread, whichever GIL it holds: a
stop-the-world across the domain's GILs (each member's eval breaker
asked to suspend, as `_PY_EVAL_PLEASE_STOP_BIT` did), and queue
operations that do not rely on one GIL. Step 7, which decides domains
and collection scheduling, owns that question.

### 7.3 Threads of other interpreters in the same domain

They are detached while the collection runs (7.2). The mark scans their
stacks from their detach records (note 11 question 16) and their
interpreters' queues as roots (3.3). The calls the collection moves to
their interpreters run when one of their threads next takes the GIL
(section 4). A thread of the main interpreter blocked in C on a lock
that a subinterpreter's thread will release does not drain until it
returns; nothing in the core waits for a callback of another
interpreter, and the tests of section 9 run the interpreter and
threading suites to find any that does.

## 8. Invariants

Each with the test of section 9 that proves it.

1. **A call runs on its owner.** Debug builds assert in `run_queue()`
   that `_PyGC_LiveOwner()` of the call's object is `tstate->interp`
   (recomputed through the segment). Tests 9.2, 9.3, 9.4, 9.6.
2. **No call runs while its queue is closed**, and no gc.callbacks or
   Python-level debug output while the collecting interpreter's is
   closed. Tests 9.1, 9.2 (the B paths under stress and kill).
3. **A queued object stays intact until its call runs**, whichever
   interpreter's collection found it and however many collections
   intervene. Test 9.4 under `PYTHON_GCVERIFY=kill`.
4. **Each call runs at most once, and none is lost while its owner
   lives**: a call is moved, never copied, and dropped only at its
   owner's close, with a record. Tests 9.4 (teardown runs it), 9.5 (the
   drop is counted).
5. **Per-owner order is collection order**: callbacks, then finalizers
   referrer first, then legacy, then destroy hooks, as now. test_gc's
   existing order tests (`test_finalizer_order`,
   `test_dealloc_reads_referents`), unchanged.
6. **The domain has one GIL.** Test 9.8.
7. **Every live block of a shared heap is in the domain**, including
   those of deleted interpreters. Test 9.7 under kill.
8. **A destroy hook is decided and run against its owner's watchers.**
   Test 9.9.

## 9. Tests

New tests go in test_gc (`InterpreterQueueTests`), in child processes
where they set `PYTHON_GCSTRESS` or `PYTHON_GCVERIFY`. "Records its
interpreter" means the callback appends `_interpreters.get_current()`'s
id to a list or writes it to a pipe.

1. **The creator paths under stress, exclusions removed.**
   test_gc `test_stress_shared_heap_callbacks` without its skip; the
   stress line on test_capi at `PYTHON_GCSTRESS=1000` with only
   test_bytes' `*test_resize*` excluded (10a rung 3's writer row): every
   creator of note 11 8.2's B class runs (`SubinterpreterTest`,
   `TestThreadState.test_gilstate_matches_current`,
   `InterpreterConfigTests`, `InterpreterIDTests`,
   `TestPendingCalls.test_isolated_subinterpreter`, and the six
   `_testcapi.test_*` functions that call `Py_NewInterpreter()` through
   `Test_testcapi`). test_misc alone five times on both builds;
   test_getargs `test_gh_119213` stays in the stress set as the
   own-heap control.
2. **Main garbage while a legacy subinterpreter initialises.** The main
   interpreter disables automatic collection, builds 200 cycles each
   holding an object with `__del__` and a weak reference with a
   callback, both recording their interpreter, drops them, then creates
   twenty legacy subinterpreters (`_testcapi.run_in_subinterp`), under
   `PYTHON_GCSTRESS=1000`, and once more on debug under
   `PYTHON_GCSTRESS=1000 PYTHON_GCVERIFY=kill`. Pass: exit 0, every
   callback and finalizer ran, every recorded id is 0.
3. **`gc.collect()` in a subinterpreter.** As 9.2 without stress, then
   one legacy subinterpreter makes its own cycle with a recording
   callback and calls `gc.collect()`. Pass: inside the subinterpreter,
   its own callback has run when `gc.collect()` returns and none of the
   main interpreter's has; back in the main interpreter, after one
   statement, all of the main interpreter's have run with id 0. The
   mirror case: a persistent legacy interpreter's garbage found by the
   main interpreter's `gc.collect()` does not run on the main
   interpreter.
4. **Teardown with a non-empty queue.** A persistent legacy interpreter
   (`_interpreters.create` with the legacy config) builds a cycle whose
   `__del__` writes its interpreter id to a pipe, and returns. The main
   interpreter calls `gc.collect()` twice: the pipe stays empty and
   `_testinternalcapi.gc_queue_info(id)["length"]` is non-zero. Then
   `_interpreters.destroy()`: the pipe holds the subinterpreter's id, and
   `dropped` is unchanged. Run under kill on debug as well.
5. **The drop record.** `_testinternalcapi` runs a legacy interpreter
   whose queue it closes before a collection (a test entry,
   `gc_queue_close_for_test()`), then ends it. Pass: the callback never
   ran, `dropped["callback"]` rose by one, and the process exits 0 with
   the DEBUG_UNCOLLECTABLE line present when the flag is set.
6. **Reentrancy.** A main-interpreter weak reference callback that
   creates a legacy subinterpreter (`_testcapi.run_in_subinterp("x =
   1")`), and another that calls `gc.collect()`, under
   `PYTHON_GCSTRESS=1000`. Pass: exit 0, every callback ran once, on
   interpreter 0.
7. **The retired heap.** (a) A legacy subinterpreter stores a fresh list
   in a C static of `_testinternalcapi` (`stash_set()`) and ends; the
   main interpreter appends a fresh object to the stashed list, drops
   its own reference, collects under kill, and reads the element back.
   This fails before section 5.3 (the element is killed) and passes after.
   (b) A legacy subinterpreter interns a thousand fresh strings into the
   main interpreter's dictionary and ends; the main interpreter collects
   under kill and reads them through `sys.intern()`; the strings must be
   marked, not killed. (c) After both, a collection with the stash
   cleared frees the retired sub-process's blocks
   (`gc_queue_info()["retired"]` falls to zero once the segments empty).
8. **One GIL per domain.** test_capi.test_misc `InterpreterConfigTests`:
   the all-true row raises `InterpreterError`; the other rows unchanged.
9. **Destroy hooks on the owner.** The main interpreter adds a dict
   watcher (`_testcapi.add_dict_watcher`), watches a dict in a cycle and
   drops it; a legacy subinterpreter calls `gc.collect()`. Pass: the
   watcher has not run inside the subinterpreter; it records the
   DEALLOCATED event in the main interpreter after return.

The gate set of the rung (work doc 7a-1.md) is these, test_gc,
test_weakref, test_capi, test_threading, test_interpreters,
test__interpreters, test_concurrent_futures's interpreter executor,
test_sys, test_atexit and test_io, then the ladder of PROCESS.md section
3 through level 3 with the collector verification set.

## 10. What it deletes and what it hands on

Deleted:

- the collecting-interpreter rule: `run_queue()` on the collecting
  thread for every call the domain's collection found, and the comment
  at the head of gc_live.c that states it;
- the skip of test_gc `test_stress_shared_heap_callbacks`;
- the B class of note 11 section 8.2 and its stress exclusion of
  test_capi.test_misc (in the verification manifest once PROCESS.md
  section 0.1's manifest exists), 10a-2.md's accepted shared-heap
  failures, and the hack register's shared-heap row of note 08, each
  with this note as the retirement;
- the acceptance of `use_main_obmalloc` with `OWN_GIL`.

Handed to step 7 (the generational collector):

- The queues and the routing. A minor collection queues and moves calls
  the same way; the owner lookup is per segment and so holds for young
  objects; the queues are a root class that every minor collection
  visits in full (note 11 section 11's list already names "the
  finaliser queue").
- Stop-the-world across a domain's GILs, the condition for lifting 7.2.
- The domain cap of 64.

Handed to 10b: a moving collector keeps the property that a block's
segment names its owner: a nursery is per thread, so per interpreter,
and evacuation copies into a segment of the object's own sub-process.

## 11. Measurements

Recorded, not gated (note 06 plan revision of 2026-10-27).

- **Per-collection cost.** The internal set's memory group
  (`collector.py` at L = 16, 128 and 512 MB, `startup.py`,
  `imports.py`) and the kernels, `instr` and `rss`, against the
  integration head the rung starts from. A one-interpreter process does
  no lookup and no move, so the expected change is nil; any change is
  attributed. The collector's pause (`gc.get_stats()` `max_pause`) on
  `collector.py`.
- **Routing.** A counter of calls moved per collection, in the root
  stats (`moved`), summed over the suite with the root counters.
- **Callback latency.** `boil/tools/measure/queue_latency.py`: (a) in one
  interpreter, nanoseconds from the start of `gc.collect()` to a weak
  reference callback, which should be unchanged; (b) a legacy
  subinterpreter that collects main-interpreter garbage and returns:
  nanoseconds from the collection to the callback on the main
  interpreter, and the bytecodes the main interpreter runs before it.
- **Startup.** `-c pass` instructions, since calls queued during
  initialisation now wait for the opening.

## 12. Rungs

One rung (7a-1) carries the whole design: the owner lookup and the move,
the opening and close, the take_gil check, the drop record, the retired
heaps, the per-owner watcher masks, the configuration rejection, and the
tests of section 9. If it passes its deadline, the cut order is the
measurement tool, then 9.9's watcher test with section 3.4 (a hand-off
to step 7, recorded in the hack register); the ownership, the gates, the
retired heaps and the rejection are never cut. The work doc lists the
files.

## 13. Open questions

1. **The drop at the main interpreter's own close.** Recommendation:
   drop with the record, as for a subinterpreter. Running the calls
   after `interpreter_clear()` has emptied `sys` is what the close
   exists to avoid.
2. **`gc.get_stats()` in a domain.** Each interpreter's stats count the
   collections it ran, over the domain. Recommendation: keep; document
   in gc.rst beside the ownership rule.
3. **A main thread blocked on a subinterpreter's callback.** No core
   path is known (7.3). Recommendation: none until a test shows one;
   then the fix is that test's.
