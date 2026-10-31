/* The live tracing collector (note 06, step 5a).

   Nothing is reference counted and this collector owns the
   memory of every object.  When the bytes allocated since the last
   collection pass a threshold (the trigger), an object allocation schedules
   a collection, which runs where the eval loop next checks its breaker; one
   that passes a second threshold without reaching it collects at once
   (_PyGC_LiveOnThreshold()).  A collection runs with the GIL held:

   1. The mark (Python/gc_trace.c) marks everything reachable from the
      precise and conservative roots, then walks the heaps: each unmarked
      object gets _Py_GC_DEAD_FLAG and is listed.
   2. Weak references with callbacks to dead objects are cleared; the
      callbacks of those that are themselves live are queued.
   3. A dead GC object with a legacy tp_del that is in a cycle of dead
      objects goes to gc.garbage, as everything does with DEBUG_SAVEALL; it
      and what it reaches are resurrected and not finalized.  Each other
      dead object with a finalizer that has not run (PEP 442), or with a
      tp_del that has not run since it was last reachable, is queued, and it
      and every dead object it reaches are resurrected for this cycle.
   4. Each remaining dead object whose dealloc would notify watchers, call
      m_free or a capsule destructor, or release a buffer to an exporter
      that defines __release_buffer__ has those calls queued, once, and it
      and what it reaches are resurrected (queue_destroy_hooks()).
   5. The remaining weak references to dead objects, and those to the
      objects of step 4, are cleared.
   6. The sweep calls tp_dealloc on every object still dead, those whose
      dealloc reads its referents first, referrer first, and types last,
      with the frees of their blocks deferred (PyObject_Free() sees
      GCState.sweeping), then frees the blocks.  A
      dealloc can therefore read the other dead objects' memory; it must not
      allocate objects or run Python code, which debug builds check.
   7. After the collection the queue runs: the weak reference callbacks,
      the finalizers, then the destroy calls.  After a collection that an
      allocation ran at once, it waits for the eval breaker.

   Collections run in the interpreter that allocates; the mark covers every
   interpreter that shares its object heaps.  Each interpreter has its own
   queue (note 13), which runs only on its threads and only while it is
   open: from the end of the interpreter's pycore_interp_init() to its
   _PyGC_Fini().  Python code, gc.callbacks and the collector's Python-level
   debug output need the collecting interpreter's queue open. */

#include "Python.h"
#include "pycore_capsule.h"       // _PyCapsule_Destroy()
#include "pycore_ceval.h"         // _Py_set_eval_breaker_bit()
#include "pycore_code.h"          // _PyCode_Destroy()
#include "pycore_dict.h"          // _PyDict_SendEvent()
#include "pycore_function.h"      // _PyFunction_NotifyDestroy()
#include "pycore_genobject.h"     // _PyGen_FinalizerIsNoop()
#include "pycore_initconfig.h"    // _PyStatus_OK()
#include "pycore_interp.h"        // PyInterpreterState.gc
#include "pycore_interpframe.h"   // _PyFrame_GetLocalsArray()
#include "pycore_memoryobject.h"  // _PyManagedBuffer_Type
#include "pycore_mimalloc.h"      // _mi_gc_segment_of()
#include "pycore_moduleobject.h"  // _PyModule_FreeState()
#include "pycore_object.h"
#include "pycore_gc_trace.h"      // _PyGC_TraceLiveMark()
#include "pycore_gc_walk.h"       // Py_GC_PRECISE
#include "pycore_object_alloc.h"  // _PyObject_MallocWithType()
#include "pycore_pyerrors.h"
#include "pycore_pystate.h"       // _PyThreadState_GET()
#include "pycore_tstate.h"        // _PyThreadStateImpl
#include "pycore_typeobject.h"    // _PyType_NotifyDestroy()
#include "pycore_value.h"         // _PyThreadState_PushValueSpan()
#include "pycore_weakref.h"       // _PyWeakref_ClearRef()


#include <stdlib.h>

typedef struct _gc_runtime_state GCState;


/* While a collection cannot run, try again after this many more bytes. */
#define RETRY_BYTES (1024 * 1024)

/* What a queued call does with op. */
enum live_call_kind {
    CALL_CALLBACK,   // callback(op), op a weak reference
    CALL_FINALIZE,   // op's tp_finalize (PEP 442)
    CALL_LEGACY,     // op's legacy tp_del
    CALL_DESTROY,    // the destroy calls of op's dealloc (run_destroy_hook())
};

/* A queued call: a node of an interpreter's queue (GCState.queue_head)
   or of a collection's batch.  Nodes come from the C library's malloc,
   one per call, so that splicing a batch onto a queue allocates nothing. */
struct _PyGCLiveCall {
    struct _PyGCLiveCall *next;
    PyObject *op;
    PyObject *callback;
    enum live_call_kind kind;
};
typedef struct _PyGCLiveCall live_call;

/* GCState.queue_phase.  A queue opens when its interpreter can run Python
   code, at the end of pycore_interp_init(), and closes in _PyGC_Fini(),
   after its last drain.  Nothing runs from a queue that is not open. */
enum {
    QUEUE_INITIALISING = 0,
    QUEUE_OPEN = 1,
    QUEUE_CLOSED = 2,
};

/* A collection's calls, in the order they run, before they are spliced
   onto a queue. */
typedef struct {
    live_call *head;
    live_call *tail;
} call_batch;

/* The collector's own memory comes from the C library's malloc: the heap
   walks must not see it, and it must not trigger a collection. */
struct _gc_live_state {
    // Immortal heap objects that refer to others: roots.
    PyObject **immortal;
    size_t nimmortal;
    size_t immortal_cap;
    // The second pass of an explicit collection is not verified again.
    int skip_verify;
    // Set during a collection that an allocation runs, where Python code
    // cannot run: the gc.callbacks are not called, debug output goes to the
    // C stderr, and the queue waits for the eval breaker.
    int no_python;
    // The thread states that finalization unlinked (_PyGC_LiveKeepThreads()).
    PyThreadState *kept_threads;
    // The exception that was set when the collection started.  The stack
    // below the collection's entry is not scanned (collect_from_entry()).
    PyObject *exc;
    // Totals for gc.get_stats()
    Py_ssize_t bytes_freed;
    Py_ssize_t live_bytes;
    Py_ssize_t pause_max_ns;
    // The root phase's counters (_PyGC_LiveGetRootStats()), and the
    // process they were counted in: a forked child starts from zero.
    _PyGCRootStats root_total;
    _PyGCRootStats root_last;
    long root_pid;
    // The stack walk's frame descriptors (Python/gc_walk.c)
    _PyGCWalkCache walk_cache;
};

Py_GC_PRECISE static long
current_pid(void)
{
#ifdef HAVE_GETPID
    return (long)getpid();
#else
    return 0;
#endif
}

Py_GC_PRECISE static struct _gc_live_state *
get_live(PyInterpreterState *interp)
{
    if (interp->gc.queue_phase == QUEUE_CLOSED) {
        return NULL;  // _PyGC_Fini() freed it; nothing collects here again
    }
    if (interp->gc.live == NULL) {
        interp->gc.live = calloc(1, sizeof(struct _gc_live_state));
        if (interp->gc.live != NULL) {
            interp->gc.live->root_pid = current_pid();
        }
    }
    return interp->gc.live;
}

/* The interpreter's descriptor cache for the stack walk, or NULL. */
_PyGCWalkCache *
_PyGC_LiveWalkCache(PyInterpreterState *interp)
{
    struct _gc_live_state *ls = get_live(interp);
    return ls == NULL ? NULL : &ls->walk_cache;
}

/* Count one mark's root phase: each mark is charged once, to the
   interpreter that ran it, whichever interpreters' heaps it covered. */
static void
add_root_stats(struct _gc_live_state *ls, _PyGCRootStats *s)
{
    long pid = current_pid();
    if (pid != ls->root_pid) {
        ls->root_total = (_PyGCRootStats){0};
        ls->root_pid = pid;
    }
    s->collections = 1;
    ls->root_last = *s;
    _PyGCRootStats *total = &ls->root_total;
#define ADD(name) total->name += s->name;
    _PyGC_ROOT_STATS_FIELDS(ADD)
#undef ADD
    for (int i = 0; i < _PyGC_SCAN_CLASSES; i++) {
        total->words[i] += s->words[i];
        total->hits[i] += s->hits[i];
    }
}

/* PYTHON_GCROOTSTATS=path appends an interpreter's root counters to path
   when it is finalized, as one JSON line with the process and interpreter
   ids, for summing over a test suite (boil/tools/measure/root_stats.py).
   A forked child that has not collected writes nothing, so that its
   parent's counts are not counted twice; a process that exits without
   finalizing writes nothing.  A measurement switch: a file that cannot be
   written is ignored. */
static void
dump_root_stats(PyInterpreterState *interp, struct _gc_live_state *ls)
{
    const char *path = getenv("PYTHON_GCROOTSTATS");
    if (path == NULL || path[0] == '\0' || ls->root_pid != current_pid()) {
        return;
    }
    FILE *out = fopen(path, "a");
    if (out == NULL) {
        return;
    }
    const _PyGCRootStats *s = &ls->root_total;
    static const char *const names[] = _PyGC_SCAN_CLASS_NAMES;
    fprintf(out, "{\"pid\": %ld, \"interp\": %lld", ls->root_pid,
            (long long)interp->id);
#define DUMP(name) fprintf(out, ", \"" #name "\": %lld", (long long)s->name);
    _PyGC_ROOT_STATS_FIELDS(DUMP)
#undef DUMP
    for (int i = 0; i < _PyGC_SCAN_CLASSES; i++) {
        fprintf(out, ", \"%s_words\": %lld, \"%s_hits\": %lld",
                names[i], (long long)s->words[i],
                names[i], (long long)s->hits[i]);
    }
    fprintf(out, "}\n");
    fclose(out);
}

static GCState *
get_gc_state(void)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    return &interp->gc;
}


/* --- roots the collector holds ----------------------------------------- */

void
_PyGC_LiveAddImmortal(PyObject *op)
{
    if (!_PyGC_IsHeapObject(op)) {
        return;  // a static object
    }
    PyTypeObject *tp = Py_TYPE(op);
    if (tp->tp_traverse == NULL && !(tp->tp_flags & Py_TPFLAGS_HEAPTYPE)
        && !_PyGC_HasDictWord(tp))
    {
        return;  // refers to nothing
    }
    PyThreadState *tstate = _PyThreadState_GET();
    PyInterpreterState *interp = tstate != NULL ? tstate->interp
                                                : _PyInterpreterState_Main();
    if (interp->gc.queue_phase == QUEUE_CLOSED) {
        // The interpreter's heap is the main interpreter's from its close.
        interp = _PyInterpreterState_Main();
    }
    struct _gc_live_state *ls = get_live(interp);
    if (ls == NULL) {
        Py_FatalError("out of memory recording an immortal object");
    }
    if (ls->nimmortal == ls->immortal_cap) {
        size_t cap = ls->immortal_cap ? ls->immortal_cap * 2 : 256;
        PyObject **objs = realloc(ls->immortal, cap * sizeof(PyObject *));
        if (objs == NULL) {
            Py_FatalError("out of memory recording an immortal object");
        }
        ls->immortal = objs;
        ls->immortal_cap = cap;
    }
    ls->immortal[ls->nimmortal++] = op;
}

int
_PyGC_LiveVisitRoots(PyInterpreterState *interp, visitproc visit, void *arg)
{
    for (live_call *c = interp->gc.queue_head; c != NULL; c = c->next) {
        Py_VISIT(c->op);
        Py_VISIT(c->callback);
    }
    struct _gc_live_state *ls = interp->gc.live;
    if (ls == NULL) {
        return 0;
    }
    for (size_t i = 0; i < ls->nimmortal; i++) {
        PyObject *op = ls->immortal[i];
        Py_VISIT(Py_TYPE(op));
        (void)_PyGC_TraceTraverse(op, visit, arg);
    }
    Py_VISIT(ls->exc);
    return 0;
}

/* _testinternalcapi.gc_queue_fail_alloc(n): the nth node allocation from
   now fails, once. */
static int fail_alloc_countdown = 0;

int
_PyGC_LiveFailAlloc(int n)
{
    int left = fail_alloc_countdown;
    fail_alloc_countdown = n;
    return left;
}

static live_call *
call_new(PyObject *op, PyObject *callback, enum live_call_kind kind)
{
    if (fail_alloc_countdown > 0 && --fail_alloc_countdown == 0) {
        return NULL;
    }
    live_call *c = malloc(sizeof(live_call));
    if (c != NULL) {
        *c = (live_call){NULL, op, callback, kind};
    }
    return c;
}

static void
batch_append(call_batch *b, live_call *c)
{
    c->next = NULL;
    if (b->tail == NULL) {
        b->head = c;
    }
    else {
        b->tail->next = c;
    }
    b->tail = c;
}

/* The first node appended after `last`, a tail saved earlier. */
static inline live_call *
batch_after(call_batch *b, live_call *last)
{
    return last == NULL ? b->head : last->next;
}

static int
queue_push(call_batch *b, PyObject *op, PyObject *callback,
           enum live_call_kind kind)
{
    live_call *c = call_new(op, callback, kind);
    if (c == NULL) {
        return -1;
    }
    batch_append(b, c);
    return 0;
}

/* Move every node of b to the tail of gc's queue.  queue_ready is set
   exactly while an open queue is non-empty (run_queue_ex() clears it as it
   takes the last node), so that whichever of the owner's threads next takes
   the GIL drains it (note 13 invariant 6), not only the thread whose
   collection queued the calls. */
static void
queue_splice(GCState *gc, call_batch *b)
{
    if (b->head == NULL) {
        return;
    }
    if (gc->queue_tail == NULL) {
        gc->queue_head = b->head;
    }
    else {
        gc->queue_tail->next = b->head;
    }
    gc->queue_tail = b->tail;
    b->head = b->tail = NULL;
    if (gc->queue_phase == QUEUE_OPEN) {
        _Py_atomic_store_int_relaxed(&gc->queue_ready, 1);
    }
}

/* What op's dealloc would call under reference counting, besides freeing:
   the DESTROY event of the dict, type, function and code watchers, a code
   object's co_extra free functions, a module's m_free, a capsule's destructor, the release of a memoryview's or
   managed buffer's active acquisition.  The sweep may not run code or
   allocate, so the collector calls these first, with op and what it
   reaches resurrected, as reference counting calls them while op is
   intact.  op is swept at a later collection, without them. */
static int
run_destroy_hook(PyThreadState *tstate, PyObject *op)
{
    int consumed = 0;
    PyTypeObject *tp = Py_TYPE(op);
    if (PyDict_Check(op)) {
        // CPython's own watchers hear of it in dict_dealloc().
        PyDictObject *mp = (PyDictObject *)op;
        int bits = (int)(mp->_ma_watcher_tag & DICT_WATCHER_MASK
                         & ~DICT_RESERVED_WATCHERS_MASK);
        if (bits) {
            _PyDict_SendEvent(bits, PyDict_EVENT_DEALLOCATED, mp, PyValue_NULL, PyValue_NULL);
        }
    }
    else if (PyType_Check(op)) {
        _PyType_NotifyDestroy((PyTypeObject *)op);
    }
    else if (tp == &PyFunction_Type) {
        _PyFunction_NotifyDestroy(op);
    }
    else if (tp == &PyCode_Type) {
        _PyCode_Destroy(op);
    }
    else if (tp == &PyCapsule_Type) {
        _PyCapsule_Destroy(op);
    }
    else if (tp == &_PyManagedBuffer_Type || tp == &PyMemoryView_Type) {
        /* An earlier callback may have acquired another export since this
           node was selected.  Only actual consumption acknowledges release. */
        consumed = _PyMemoryView_ReleaseForDealloc(op);
        if (consumed) {
            op->ob_flags |= _Py_GC_NOTIFIED_FLAG;
        }
    }
    else {
        assert(PyModule_Check(op));
        _PyModule_FreeState(op);
    }
    if (_PyErr_Occurred(tstate)) {
        // The watchers report their own errors; m_free and destructors
        // have no way to.  The repr of a freed module could fail.
        PyErr_FormatUnraisable("Exception ignored while destroying a %s "
                               "object", tp->tp_name);
    }
    return consumed;
}

#ifdef Py_DEBUG
/* Note 13 invariant 1: a call runs on the interpreter that owns its object,
   the main interpreter for an object of a closed or deleted one. */
static void
assert_owner(PyThreadState *tstate, PyObject *op)
{
    mi_segment_t *seg = _mi_gc_segment_of((uintptr_t)op);
    PyInterpreterState *here = tstate->interp;
    if (seg == NULL || (void *)seg->subproc == (void *)here->mimalloc.subproc) {
        return;
    }
    int ok = 0;
    if (_Py_IsMainInterpreter(here)) {
        ok = 1;  // a deleted interpreter's object
        HEAD_LOCK(&_PyRuntime);
        for (PyInterpreterState *i = _PyRuntime.interpreters.head; i != NULL;
             i = i->next)
        {
            if ((void *)i->mimalloc.subproc == (void *)seg->subproc) {
                ok = i->gc.queue_phase == QUEUE_CLOSED;
                break;
            }
        }
        HEAD_UNLOCK(&_PyRuntime);
    }
    if (!ok) {
        _PyObject_ASSERT_FAILED_MSG(op, "a queued call runs on an "
                                        "interpreter that does not own it");
    }
}
#else
#  define assert_owner(tstate, op) ((void)0)
#endif

/* May the collector run Python code on tstate: queued calls, gc.callbacks,
   debug output through sys.stderr? */
static inline int
live_python_ok(PyThreadState *tstate)
{
    PyInterpreterState *interp = tstate->interp;
    return interp->gc.queue_phase == QUEUE_OPEN
           && (interp->gc.live == NULL || !interp->gc.live->no_python);
}

/* Run the queued callbacks and finalizers.  A collection that one of them
   triggers queues more at the tail, and this loop runs those too.  Other
   collectors leave delivery to this drainer and return: a callback may
   be waiting for one of those threads to finish collecting. */
static int
run_queue_ex(PyThreadState *tstate, int closing)
{
    GCState *gc = &tstate->interp->gc;
    _PyThreadStateImpl *ts = (_PyThreadStateImpl *)tstate;
    if (gc->queue_draining || gc->queue_head == NULL
        || gc->queue_phase != QUEUE_OPEN)
    {
        return 0;
    }
    /* A signal that arrived while the collection marked is handled first,
       as the eval breaker handles signals before a scheduled collection: a
       handler run in a callback here would have its exception reported as
       unraisable instead of raised in the code the collection interrupted.
       The queue waits for the breaker, which handles the signal and then
       runs it (_Py_RunGC()). */
    if (!closing
        && _Py_eval_breaker_bit_is_set(tstate, _PY_SIGNALS_PENDING_BIT)
        && _Py_ThreadCanHandleSignals(tstate->interp)
        && _PyInterpreterState_GetFinalizing(tstate->interp) == NULL)
    {
        _Py_ScheduleGC(tstate);
        return 0;
    }
    int ncompleted = 0;
    gc->queue_draining = 1;
    ts->gc_trace.running_queue = 1;
    PyValue slots[3];
    _PyValueRoot span;
    _PyThreadState_PushValueSpan(tstate, &span, slots);
    PyObject **exc = _PyValueSpan_ObjectSlot(&span);
    PyObject **op = _PyValueSpan_ObjectSlot(&span);
    PyObject **callback = _PyValueSpan_ObjectSlot(&span);
    *exc = _PyErr_GetRaisedException(tstate);
    live_call *node;
    while ((node = gc->queue_head) != NULL) {
        // Publish the active call's roots before removing its queue roots.
        *op = node->op;
        *callback = node->callback;
        gc->queue_head = node->next;
        if (gc->queue_head == NULL) {
            gc->queue_tail = NULL;
        }
        if (gc->queue_head == NULL) {
            _Py_atomic_store_int_relaxed(&gc->queue_ready, 0);
        }
        live_call call = *node;
        free(node);
        assert_owner(tstate, call.op);
        if (call.callback != NULL) {
            PyObject *r = PyObject_CallOneArg(call.callback, call.op);
            if (r == NULL) {
                PyErr_FormatUnraisable("Exception ignored while calling "
                                       "weakref callback %R", call.callback);
            }
            PyWeakReference *wr = (PyWeakReference *)call.op;
            if (wr->wr_callback == call.callback) {
                wr->wr_callback = NULL;
            }
        }
        else if (call.kind == CALL_DESTROY) {
            ncompleted += run_destroy_hook(tstate, call.op);
        }
        else if (call.kind == CALL_LEGACY) {
            // As subtype_dealloc() does under reference counting: the weak
            // references go first.
            ncompleted++;
            if (_PyType_SUPPORTS_WEAKREFS(Py_TYPE(call.op))) {
                PyObject_ClearWeakRefs(call.op);
            }
            call.op->ob_flags |= _Py_GC_DEL_CALLED_FLAG;
            Py_TYPE(call.op)->tp_del(call.op);
            if (_PyErr_Occurred(tstate)) {
                PyErr_FormatUnraisable("Exception ignored in tp_del of %R",
                                       call.op);
            }
        }
        else {
            ncompleted++;
            PyObject_CallFinalizer(call.op);
            if (_PyErr_Occurred(tstate)) {
                PyErr_FormatUnraisable("Exception ignored while finalizing "
                                       "%R", call.op);
            }
        }
        *op = *callback = NULL;
    }
    _PyErr_SetRaisedException(tstate, *exc);
    _PyThreadState_PopValueRoot(tstate, &span);
    ts->gc_trace.running_queue = 0;
    gc->queue_draining = 0;
    if (gc->queue_head != NULL) {
        _Py_ScheduleGC(tstate);
    }
    return ncompleted;
}

Py_GC_PRECISE static int
run_queue(PyThreadState *tstate)
{
    return run_queue_ex(tstate, 0);
}


/* --- resurrection --------------------------------------------------------- */

typedef struct {
    PyObject **items;
    size_t n;
    size_t cap;
    int oom;
} objstack;

/* Fail one scratch-stack growth, independently of queue/order/debug storage. */
static int fail_work_alloc_countdown;

int
_PyGC_LiveFailWorkAlloc(int n)
{
    int left = fail_work_alloc_countdown;
    fail_work_alloc_countdown = n;
    return left;
}

static int
objstack_push(objstack *s, PyObject *op)
{
    if (s->oom) {
        return -1;
    }
    if (s->n == s->cap) {
        size_t limit = (size_t)PY_SSIZE_T_MAX / sizeof(PyObject *);
        if (s->cap == limit) {
            s->oom = 1;
            return -1;
        }
        size_t cap = s->cap ? (s->cap > limit / 2 ? limit : s->cap * 2)
                            : 1024;
        if (fail_work_alloc_countdown > 0
            && --fail_work_alloc_countdown == 0)
        {
            s->oom = 1;
            return -1;
        }
        PyObject **items = realloc(s->items, cap * sizeof(PyObject *));
        if (items == NULL) {
            s->oom = 1;
            return -1;
        }
        s->items = items;
        s->cap = cap;
    }
    s->items[s->n++] = op;
    return 0;
}

static inline int
is_dead(PyObject *op)
{
    return _PyGC_IsHeapObject(op) && (op->ob_flags & _Py_GC_DEAD_FLAG);
}

static int
resurrect_visit(PyObject *op, void *arg)
{
    objstack *s = arg;
    if (s->oom) {
        return -1;
    }
    if (op != NULL && is_dead(op)) {
        if (objstack_push(s, op) < 0) {
            return -1;
        }
        op->ob_flags &= ~_Py_GC_DEAD_FLAG;
    }
    return 0;
}

/* Keep op, which is dead, and every dead object it reaches. */
static int
resurrect(objstack *s, PyObject *op)
{
    if (resurrect_visit(op, s) < 0) {
        return -1;
    }
    while (s->n > 0) {
        PyObject *o = s->items[--s->n];
        if (resurrect_visit((PyObject *)Py_TYPE(o), s) < 0
            || _PyGC_TraceTraverse(o, resurrect_visit, s) != 0)
        {
            s->oom = 1;
            return -1;
        }
    }
    return 0;
}


/* --- the collection ------------------------------------------------------- */

static void
invoke_gc_callback(PyThreadState *tstate, const char *phase,
                   struct gc_generation_stats *stats, int young)
{
    GCState *gcstate = &tstate->interp->gc;
    if (gcstate->callbacks == NULL || PyList_GET_SIZE(gcstate->callbacks) == 0
        || !live_python_ok(tstate))
    {
        return;
    }
    assert(PyList_CheckExact(gcstate->callbacks));
    PyObject *info = Py_BuildValue("{sisnsnsnsd}",
        "generation", young ? 0 : NUM_GENERATIONS - 1,
        "collected", stats->collected,
        "uncollectable", stats->uncollectable,
        "candidates", stats->candidates,
        "duration", stats->duration);
    if (info == NULL) {
        PyErr_FormatUnraisable("Exception ignored on invoking gc callbacks");
        return;
    }
    PyObject *phase_obj = PyUnicode_FromString(phase);
    if (phase_obj == NULL) {
        PyErr_FormatUnraisable("Exception ignored on invoking gc callbacks");
        return;
    }
    PyObject *stack[] = {phase_obj, info};
    for (Py_ssize_t i = 0; i < PyList_GET_SIZE(gcstate->callbacks); i++) {
        PyObject *cb = PyList_GET_ITEM(gcstate->callbacks, i);
        PyObject *r = PyObject_Vectorcall(cb, stack, 2, NULL);
        if (r == NULL) {
            PyErr_FormatUnraisable("Exception ignored while "
                                   "calling GC callback %R", cb);
        }
    }
}

/* Clear the weak references with callbacks to dead objects, and queue the
   callbacks of those that are live (gc.c's handle_weakref_callbacks()). */
static int
clear_callback_weakrefs(call_batch *b, _PyGCMarkResult *res)
{
    if (!(res->kinds & _PyGC_DEAD_WEAKREFS)) {
        return 0;
    }
    for (Py_ssize_t i = 0; i < res->ndead; i++) {
        if (!(res->dead[i].kinds & _PyGC_DEAD_WEAKREFS)) {
            continue;
        }
        PyObject *op = res->dead[i].op;
        PyWeakReference **list = _PyWeakref_ListPtr(op, 0);
        if (list == NULL) {
            continue;  // it has none
        }
        PyWeakReference *next;
        for (PyWeakReference *wr = *list; wr != NULL; wr = next) {
            next = wr->wr_next;
            PyObject *callback = wr->wr_callback;
            if (callback == NULL) {
                continue;
            }
            // The node first: if it cannot be had, the weak reference
            // keeps its callback.
            live_call *c = call_new((PyObject *)wr, callback, CALL_CALLBACK);
            if (c == NULL) {
                return -1;
            }
            _PyWeakref_ClearRef(wr);
            if (((PyObject *)wr)->ob_flags & _Py_GC_DEAD_FLAG) {
                free(c);
                continue;  // the weak reference is garbage too
            }
            batch_append(b, c);
        }
    }
    return 0;
}

static int
cycle_visit(PyObject *op, void *arg)
{
    objstack *s = arg;
    if (s->oom) {
        return -1;
    }
    if (op != NULL && is_dead(op) && !(op->ob_flags & _Py_GC_MARKED_FLAG)) {
        // Every temporary mark must have a successful cleanup-journal entry.
        if (objstack_push(s, op) < 0) {
            return -1;
        }
        op->ob_flags |= _Py_GC_MARKED_FLAG;
    }
    return 0;
}

/* Does op reach itself through dead objects?  Only for the rare objects with
   a legacy tp_del.  The mark flag, clear on every dead object, records the
   objects visited; it is cleared again before returning.  Return -1 on an
   incomplete traversal, otherwise whether a cycle was found. */
static int
in_dead_cycle(PyObject *op, objstack *seen)
{
    seen->n = 0;
    if (_PyGC_TraceTraverse(op, cycle_visit, seen) != 0) {
        seen->oom = 1;
    }
    int found = 0;
    for (size_t i = 0; i < seen->n && !found && !seen->oom; i++) {
        PyObject *o = seen->items[i];
        if (o == op) {
            found = 1;
            break;
        }
        if (_PyGC_TraceTraverse(o, cycle_visit, seen) != 0) {
            seen->oom = 1;
        }
    }
    for (size_t i = 0; i < seen->n; i++) {
        seen->items[i]->ob_flags &= ~_Py_GC_MARKED_FLAG;
    }
    return seen->oom ? -1 : found;
}

static int
order_visit(PyObject *op, void *arg)
{
    objstack *s = arg;
    if (s->oom) {
        return -1;
    }
    if (op != NULL && is_dead(op) && !(op->ob_flags & _Py_GC_MARKED_FLAG)) {
        return objstack_push(s, op);
    }
    return 0;
}

/* Legacy-cycle resurrection can clear DEAD before ordering.  Objects that
   need no call can still connect finalizers, so this does not filter edges. */
static inline int
finalizer_eligible(PyObject *op)
{
    return (op->ob_flags & _Py_GC_DEAD_FLAG)
        && !(op->ob_flags & _Py_GC_FINALIZED_FLAG)
        && Py_TYPE(op)->tp_finalize != NULL
        && !_PyGen_FinalizerIsNoop(op);
}

/* List the dead objects that reach a dead object with a finalizer, and those
   objects, in the order their finalizers run: an object before the objects
   it reaches, as reference counting finalizes them, so that a file's
   TextIOWrapper flushes before its FileIO closes.  The order is the reverse
   of a depth-first postorder over the dead objects; within a cycle it is
   arbitrary.  The visited mark is cleared before returning. */
static int
finalizer_order(_PyGCMarkResult *res, objstack *order)
{
    int status = -1;
    objstack stack = {0};
    objstack visited = {0};
    for (Py_ssize_t i = 0;
         (res->kinds & _PyGC_DEAD_FINALIZE) && i < res->ndead; i++)
    {
        if (!(res->dead[i].kinds & _PyGC_DEAD_FINALIZE)) {
            continue;
        }
        PyObject *root = res->dead[i].op;
        if (!finalizer_eligible(root)
            || (root->ob_flags & _Py_GC_MARKED_FLAG))
        {
            continue;
        }
        if (objstack_push(&stack, root) < 0) {
            goto done;
        }
        while (stack.n > 0) {
            // An entry with bit 0 set is an object whose children are done.
            uintptr_t top = (uintptr_t)stack.items[--stack.n];
            PyObject *op = (PyObject *)(top & ~(uintptr_t)1);
            if (top & 1) {
                if (finalizer_eligible(op) && objstack_push(order, op) < 0) {
                    goto done;
                }
                continue;
            }
            if (op->ob_flags & _Py_GC_MARKED_FLAG) {
                continue;
            }
            if (objstack_push(&visited, op) < 0) {
                goto done;
            }
            op->ob_flags |= _Py_GC_MARKED_FLAG;
            if (objstack_push(&stack, (PyObject *)((uintptr_t)op | 1)) < 0
                || _PyGC_TraceTraverse(op, order_visit, &stack) != 0)
            {
                goto done;
            }
        }
    }
    status = 0;
done:
    for (size_t i = 0; i < visited.n; i++) {
        visited.items[i]->ob_flags &= ~_Py_GC_MARKED_FLAG;
    }
    free(stack.items);
    free(visited.items);
    // Reverse the postorder
    for (size_t i = 0, j = order->n; status == 0 && i + 1 < j; i++, j--) {
        PyObject *tmp = order->items[i];
        order->items[i] = order->items[j - 1];
        order->items[j - 1] = tmp;
    }
    return status;
}

/* The delivery marker, _Py_GC_DEL_CALLED_FLAG, is set when the call runs
   (run_queue()), not here: the heap walk clears it on every marked object,
   and a queued object is marked through its queue at each collection until
   its owner runs it (note 13 section 3.6).  The queue keeps it from being
   queued twice. */
static int
queue_legacy(call_batch *b, PyObject *op)
{
    return queue_push(b, op, NULL, CALL_LEGACY);
}

/* Queue the finalizers and legacy tp_dels of dead objects and resurrect what
   they reach, as gc.c does: the GC objects with a tp_del in a cycle of
   garbage, and what they reach, are not finalized but listed in *garbage,
   for gc.garbage, as is everything with DEBUG_SAVEALL.  A tp_del outside a
   cycle runs, as it does when its count drops to zero. */
static int
handle_finalizers(GCState *gc, call_batch *b,
                  _PyGCMarkResult *res, objstack *garbage)
{
    int saveall = gc->debug & _PyGC_DEBUG_SAVEALL;
    objstack s = {0};
    objstack legacy = {0};
    for (Py_ssize_t i = 0; (res->kinds & _PyGC_DEAD_DEL) && i < res->ndead;
         i++)
    {
        if (!(res->dead[i].kinds & _PyGC_DEAD_DEL)) {
            continue;
        }
        PyObject *op = res->dead[i].op;
        if (op->ob_flags & _Py_GC_DEL_CALLED_FLAG) {
            continue;
        }
        int cycle = _PyObject_IS_GC(op) ? in_dead_cycle(op, &s) : 0;
        if (cycle < 0
            || objstack_push(cycle ? garbage : &legacy, op) < 0)
        {
            goto error;
        }
    }
    s.n = 0;
    for (size_t i = 0; i < garbage->n; i++) {
        if (resurrect(&s, garbage->items[i]) < 0) {
            goto error;
        }
    }
    // A tp_del that a cycle in gc.garbage reaches does not run either: its
    // object goes to gc.garbage too.
    for (size_t i = 0; i < legacy.n; i++) {
        PyObject *op = legacy.items[i];
        if (!(op->ob_flags & _Py_GC_DEAD_FLAG) && _PyObject_IS_GC(op)
            && objstack_push(garbage, op) < 0)
        {
            goto error;
        }
    }
    live_call *last = b->tail;
    // Queue every finalizer first: a resurrected object needs its own.
    objstack order = {0};
    if (finalizer_order(res, &order) < 0) {
        free(order.items);
        goto error;
    }
    for (size_t i = 0; i < order.n; i++) {
        PyObject *op = order.items[i];
        if (finalizer_eligible(op)) {
            if (queue_push(b, op, NULL, CALL_FINALIZE) < 0) {
                free(order.items);
                goto error;
            }
        }
    }
    free(order.items);
    for (Py_ssize_t i = 0; saveall && i < res->ndead; i++) {
        PyObject *op = res->dead[i].op;
        if (!(op->ob_flags & _Py_GC_DEAD_FLAG)) {
            continue;  // reached from a legacy cycle
        }
        if ((res->dead[i].kinds & _PyGC_DEAD_GC)
            && objstack_push(garbage, op) < 0)
        {
            goto error;
        }
    }
    // tp_del after tp_finalize, as subtype_dealloc() calls them
    for (size_t i = 0; i < legacy.n; i++) {
        if ((legacy.items[i]->ob_flags & _Py_GC_DEAD_FLAG)
            && queue_legacy(b, legacy.items[i]) < 0)
        {
            goto error;
        }
    }
    for (live_call *c = batch_after(b, last); c != NULL; c = c->next) {
        if (resurrect(&s, c->op) < 0) {
            goto error;
        }
    }
    for (size_t i = 0; i < garbage->n; i++) {
        if (resurrect(&s, garbage->items[i]) < 0) {
            goto error;
        }
    }
    if (saveall) {
        for (Py_ssize_t i = 0; i < res->ndead; i++) {
            res->dead[i].op->ob_flags &= ~_Py_GC_DEAD_FLAG;
        }
    }
    free(s.items);
    free(legacy.items);
    return 0;

error:
    free(s.items);
    free(legacy.items);
    return -1;
}

/* Queue the destroy calls (run_destroy_hook()) of the dead objects that the
   finalizers did not resurrect, and resurrect them and what they reach.  An
   object with a finalizer that runs now was resurrected; its destroy calls
   wait for the death after it, as its dealloc would.  Each object's run once
   (_Py_GC_NOTIFIED_FLAG), also if a watcher makes it reachable again.  The
   weak references to a queued object are cleared with those of the dead
   (collect()): no Python code reaches it after m_free or a destructor. */
static int
queue_destroy_hooks(call_batch *b, _PyGCMarkResult *res)
{
    if (!(res->kinds & (_PyGC_DEAD_HOOK | _PyGC_DEAD_BUFFER))) {
        return 0;
    }
    live_call *last = b->tail;
    for (Py_ssize_t i = 0;
         (res->kinds & _PyGC_DEAD_HOOK) && i < res->ndead; i++)
    {
        _PyGCDeadObject *d = &res->dead[i];
        if (!(d->kinds & _PyGC_DEAD_HOOK)
            || !(d->op->ob_flags & _Py_GC_DEAD_FLAG))
        {
            d->kinds &= ~_PyGC_DEAD_HOOK;
            continue;
        }
        int is_buffer = d->kinds & _PyGC_DEAD_BUFFER;
        if (is_buffer && !_PyMemoryView_ReleaseReady(d->op)) {
            d->kinds &= ~_PyGC_DEAD_HOOK;
            continue;
        }
        if (queue_push(b, d->op, NULL, CALL_DESTROY) < 0) {
            return -1;
        }
        if (!is_buffer) {
            d->op->ob_flags |= _Py_GC_NOTIFIED_FLAG;
        }
    }
    // Queue every one first: resurrecting one can reach another.
    objstack s = {0};
    for (live_call *c = batch_after(b, last); c != NULL; c = c->next) {
        if (resurrect(&s, c->op) < 0) {
            free(s.items);
            return -1;
        }
    }
    /* Select every ready hook before retention: an unready view can reach the
       owner whose queued release will make progress.  Declining a hook alone
       would leave the view and its native acquisition eligible for sweep. */
    for (Py_ssize_t i = 0;
         (res->kinds & _PyGC_DEAD_BUFFER) && i < res->ndead; i++)
    {
        if (!(res->dead[i].kinds & _PyGC_DEAD_BUFFER)) {
            continue;
        }
        PyObject *op = res->dead[i].op;
        if ((op->ob_flags & _Py_GC_DEAD_FLAG)
            && !_PyMemoryView_ReleaseReady(op)
            && resurrect(&s, op) < 0)
        {
            free(s.items);
            return -1;
        }
    }
    free(s.items);
    return 0;
}

static void
undo_dead(_PyGCMarkResult *res)
{
    for (Py_ssize_t i = 0; i < res->ndead; i++) {
        res->dead[i].op->ob_flags &= ~_Py_GC_DEAD_FLAG;
        res->live_bytes += res->dead[i].block_size;
    }
}

#ifdef Py_DEBUG
void
_PyGC_LiveSweepViolation(PyThreadState *tstate, const char *what)
{
    PyObject *op = tstate->interp->gc.sweep_op;
    _Py_FatalErrorFormat(__func__,
                         "the dealloc of a %s object at %p %s during the sweep",
                         op != NULL ? Py_TYPE(op)->tp_name : "(unknown)",
                         (void *)op, what);
}
#endif

/* --- referrer first ------------------------------------------------------ */

/* Reference counting deallocates a referrer before its referent, and a type
   with Py_TPFLAGS_DEALLOC_READS_REFERENTS relies on that.  The sweep
   deallocates the dead instances of such types first, and among themselves
   in this order: each is traversed for its references to the others, and
   the order is the reverse postorder of a depth-first search over those
   references, started from each in heap order.  A referrer therefore comes
   before its referent, except along a cycle, which the search breaks where
   it entered it.  Most flagged objects refer to no other (a managed buffer
   refers to its exporter), and then the order is the heap's and nothing
   more is built.  The cost is that of the flagged objects and their
   references, not of the heap. */

/* Is op, if dead, swept in the first pass? */
static inline int
sweeps_first(PyObject *op)
{
    return (PyType_HasFeature(Py_TYPE(op), Py_TPFLAGS_DEALLOC_READS_REFERENTS)
            && !PyType_Check(op));
}

typedef struct {
    PyObject *op;
    Py_ssize_t node;
} first_key;

typedef struct {
    Py_ssize_t from;       // the node being traversed
    first_key *edges;      // a reference: from node, to op
    Py_ssize_t nedges;
    Py_ssize_t cap;
    int oom;
} first_edges;

/* Separate from the node-only failure hook: fail sweep-order allocations
   without changing the queue's partial-delivery contract. */
static int fail_order_alloc_countdown = 0;

int
_PyGC_LiveFailOrderAlloc(int n)
{
    int left = fail_order_alloc_countdown;
    fail_order_alloc_countdown = n;
    return left;
}

static void *
order_alloc(void *items, size_t count, size_t item_size)
{
    if (count > (size_t)PY_SSIZE_T_MAX / item_size) {
        return NULL;
    }
    if (fail_order_alloc_countdown > 0
        && --fail_order_alloc_countdown == 0)
    {
        return NULL;
    }
    return realloc(items, count * item_size);
}

static int
first_key_cmp(const void *a, const void *b)
{
    uintptr_t x = (uintptr_t)((const first_key *)a)->op;
    uintptr_t y = (uintptr_t)((const first_key *)b)->op;
    return (x > y) - (x < y);
}

static int
first_edge_visit(PyObject *op, void *arg)
{
    first_edges *g = arg;
    if (op == NULL || g->oom || !is_dead(op) || !sweeps_first(op)) {
        return 0;
    }
    if (g->nedges == g->cap) {
        if (g->cap > PY_SSIZE_T_MAX / 2) {
            g->oom = 1;
            return 0;
        }
        Py_ssize_t cap = g->cap ? 2 * g->cap : 64;
        first_key *edges = order_alloc(g->edges, (size_t)cap,
                                      sizeof(first_key));
        if (edges == NULL) {
            g->oom = 1;
            return 0;
        }
        g->edges = edges;
        g->cap = cap;
    }
    g->edges[g->nedges++] = (first_key){op, g->from};
    return 0;
}

/* Put node[0:n], indices into res->dead in heap order, in sweep order,
   given the references edges[0:nedges].  Returns -1 if memory runs out. */
static int
order_by_edges(_PyGCMarkResult *res, Py_ssize_t *node, Py_ssize_t n,
               first_key *edges, Py_ssize_t nedges)
{
    int status = -1;
    size_t nodes = (size_t)n + 1;
    size_t refs = (size_t)nedges + 1;
    first_key *keys = order_alloc(NULL, nodes, sizeof(first_key));
    Py_ssize_t *start = order_alloc(NULL, nodes, sizeof(Py_ssize_t));
    Py_ssize_t *cursor = order_alloc(NULL, nodes, sizeof(Py_ssize_t));
    Py_ssize_t *target = order_alloc(NULL, refs, sizeof(Py_ssize_t));
    Py_ssize_t *succ = order_alloc(NULL, refs, sizeof(Py_ssize_t));
    Py_ssize_t *stack = order_alloc(NULL, nodes, sizeof(Py_ssize_t));
    Py_ssize_t *order = order_alloc(NULL, nodes, sizeof(Py_ssize_t));
    char *seen = order_alloc(NULL, nodes, 1);
    if (keys == NULL || start == NULL || cursor == NULL || target == NULL
        || succ == NULL || stack == NULL || order == NULL || seen == NULL)
    {
        goto done;
    }
    memset(start, 0, nodes * sizeof(Py_ssize_t));
    memset(seen, 0, nodes);
    for (Py_ssize_t u = 0; u < n; u++) {
        keys[u] = (first_key){res->dead[node[u]].op, u};
    }
    qsort(keys, n, sizeof(first_key), first_key_cmp);
    // The node each reference is to, or -1 for a reference to the referrer
    // itself or to an object that is not swept first (a dead type)
    for (Py_ssize_t e = 0; e < nedges; e++) {
        first_key *k = bsearch(&edges[e], keys, n, sizeof(first_key),
                               first_key_cmp);
        target[e] = (k != NULL && k->node != edges[e].node) ? k->node : -1;
        if (target[e] >= 0) {
            start[edges[e].node + 1]++;
        }
    }
    // The references of node u are succ[start[u]:start[u + 1]].
    for (Py_ssize_t u = 0; u < n; u++) {
        start[u + 1] += start[u];
        cursor[u] = start[u];
    }
    for (Py_ssize_t e = 0; e < nedges; e++) {
        if (target[e] >= 0) {
            succ[cursor[edges[e].node]++] = target[e];
        }
    }
    for (Py_ssize_t u = 0; u < n; u++) {
        cursor[u] = start[u];
    }
    // Each node is appended to order when the search leaves it.
    Py_ssize_t norder = 0;
    for (Py_ssize_t root = 0; root < n; root++) {
        if (seen[root]) {
            continue;
        }
        Py_ssize_t sp = 0;
        stack[sp++] = root;
        seen[root] = 1;
        while (sp > 0) {
            Py_ssize_t u = stack[sp - 1];
            if (cursor[u] < start[u + 1]) {
                Py_ssize_t v = succ[cursor[u]++];
                if (!seen[v]) {
                    seen[v] = 1;
                    stack[sp++] = v;
                }
            }
            else {
                sp--;
                order[norder++] = u;
            }
        }
    }
    assert(norder == n);
    // Reversed, as indices into res->dead
    for (Py_ssize_t i = 0; i < n; i++) {
        cursor[i] = node[order[n - 1 - i]];
    }
    memcpy(node, cursor, n * sizeof(Py_ssize_t));
    status = 0;
done:
    free(keys);
    free(start);
    free(cursor);
    free(target);
    free(succ);
    free(stack);
    free(order);
    free(seen);
    return status;
}

/* Prepare the remaining dead objects' first-pass order after batching and
   debug output, before the sweep starts any deallocation. */
static int
order_dead_first(_PyGCMarkResult *res, Py_ssize_t **pnode, Py_ssize_t *pn)
{
    *pnode = NULL;
    *pn = 0;
    if (!(res->kinds & _PyGC_DEAD_FIRST)) {
        return 0;
    }
    Py_ssize_t n = 0;
    for (Py_ssize_t i = 0; i < res->ndead; i++) {
        int kinds = res->dead[i].kinds;
        if ((kinds & (_PyGC_DEAD_FIRST | _PyGC_DEAD_TYPE)) == _PyGC_DEAD_FIRST
            && is_dead(res->dead[i].op))
        {
            n++;
        }
    }
    if (n == 0) {
        return 0;
    }
    Py_ssize_t *node = order_alloc(NULL, (size_t)n, sizeof(Py_ssize_t));
    if (node == NULL) {
        return -1;
    }
    n = 0;
    for (Py_ssize_t i = 0; i < res->ndead; i++) {
        int kinds = res->dead[i].kinds;
        if ((kinds & (_PyGC_DEAD_FIRST | _PyGC_DEAD_TYPE)) == _PyGC_DEAD_FIRST
            && is_dead(res->dead[i].op))
        {
            node[n++] = i;
        }
    }
    first_edges g = {0};
    for (g.from = 0; g.from < n; g.from++) {
        if (_PyGC_TraceTraverse(res->dead[node[g.from]].op,
                               first_edge_visit, &g) != 0 || g.oom)
        {
            g.oom = 1;
            break;
        }
    }
    if (g.oom
        || (g.nedges > 0 && order_by_edges(res, node, n, g.edges, g.nedges) < 0))
    {
        free(node);
        node = NULL;
    }
    free(g.edges);
    if (node == NULL) {
        return -1;
    }
    *pnode = node;
    *pn = n;
    return 0;
}

/* Inlined: the sweep calls it for every dead object, and the call alone
   costs nbody 2% of its instructions. */
static inline Py_ALWAYS_INLINE void
sweep_one(PyThreadState *tstate, GCState *gc, _PyGCDeadObject *d,
          Py_ssize_t *ncollected)
{
    PyObject *op = d->op;
    if ((op->ob_flags & (_Py_GC_DEAD_FLAG | _Py_GC_SWEPT_FLAG))
        != _Py_GC_DEAD_FLAG)
    {
        return;
    }
    if (d->kinds & _PyGC_DEAD_GC) {
        (*ncollected)++;
    }
#ifdef Py_DEBUG
    gc->sweep_op = op;
#endif
    gc->sweep_block = (char *)op;
    // The sweep stands in for _Py_Dealloc(), which reports each
    // destruction to the reference tracer.
    _PyReftracerTrack(op, PyRefTracer_DESTROY);
#ifdef Py_DEBUG
    if (d->kinds & _PyGC_DEAD_KILLED) {
        _PyGC_quarantine_tstate = tstate;
    }
#endif
    Py_TYPE(op)->tp_dealloc(op);
#ifdef Py_DEBUG
    _PyGC_quarantine_tstate = NULL;
#endif
    op->ob_flags |= _Py_GC_SWEPT_FLAG;
    if (_PyErr_Occurred(tstate)) {
        _PyErr_Clear(tstate);
    }
}

/* Call tp_dealloc on the dead objects, then free their blocks.  Instances of
   types with Py_TPFLAGS_DEALLOC_READS_REFERENTS go first, referrer first
   (order_dead_first()), so that their referents are intact (a managed
   buffer's exporter can still update its export state, a pyexpat subparser
   can still reach its parent's XML_Parser), and instances before types, so
   that a dealloc can read its type. */
static Py_ssize_t
sweep(PyThreadState *tstate, GCState *gc, _PyGCMarkResult *res,
      const Py_ssize_t *first, Py_ssize_t nfirst, int native_cleanup,
      Py_ssize_t *bytes_freed)
{
    Py_ssize_t ncollected = 0;
    PyObject *exc = _PyErr_GetRaisedException(tstate);
    gc->sweeping = 1;
    if (native_cleanup) {
        // The whole cohort was revalidated before entering this nonfailing
        // interval.  Existing sweep guards reject allocating/Python cleanup.
        for (Py_ssize_t i = 0; i < res->ndead; i++) {
            PyObject *op = res->dead[i].op;
            if ((res->dead[i].kinds & _PyGC_DEAD_FINALIZE)
                && finalizer_eligible(op))
            {
#ifdef Py_DEBUG
                gc->sweep_op = op;
#endif
                _PyGen_ClearUnreachable(op);
            }
        }
    }
    for (int phase = 0; phase < 3; phase++) {
        if ((phase == 0 && !(res->kinds & _PyGC_DEAD_FIRST))
            || (phase == 2 && !(res->kinds & _PyGC_DEAD_TYPE)))
        {
            continue;
        }
        if (phase == 0) {
            for (Py_ssize_t j = 0; j < nfirst; j++) {
                sweep_one(tstate, gc, &res->dead[first[j]], &ncollected);
            }
            continue;
        }
        for (Py_ssize_t i = 0; i < res->ndead; i++) {
            int kinds = res->dead[i].kinds;
            int op_phase = (kinds & _PyGC_DEAD_TYPE) ? 2
                           : (kinds & _PyGC_DEAD_FIRST) ? 0 : 1;
            if (op_phase == phase) {
                sweep_one(tstate, gc, &res->dead[i], &ncollected);
            }
        }
    }
    gc->sweeping = 0;
    gc->sweep_block = NULL;
#ifdef Py_DEBUG
    gc->sweep_op = NULL;
#endif
    _PyErr_SetRaisedException(tstate, exc);
    Py_ssize_t freed = 0;
    for (Py_ssize_t i = 0; i < res->ndead; i++) {
        _PyGCDeadObject *d = &res->dead[i];
        if (!(d->op->ob_flags & _Py_GC_DEAD_FLAG)) {
            res->live_bytes += d->block_size;
            continue;
        }
        freed += d->block_size;
#ifdef Py_DEBUG
        if (d->kinds & _PyGC_DEAD_KILLED) {
            _PyGC_quarantine_tstate = tstate;
        }
#endif
        _PyGC_FreeDeadBlock(d->op, (char *)d->op);
#ifdef Py_DEBUG
        _PyGC_quarantine_tstate = NULL;
#endif
    }
    *bytes_freed = freed;
    return ncollected;
}

/* A whole-pass choice: no other callback or native observer may see these
   suspended frames before their native-only cleanup.  No graph is changed. */
static int
native_cleanup_ready(PyThreadState *tstate, _PyGCMarkResult *res)
{
#ifdef Py_GIL_DISABLED
    return 0;
#endif
    PyInterpreterState *interp = tstate->interp;
    GCState *gc = &interp->gc;
    if (!(res->kinds & _PyGC_DEAD_FINALIZE)
        || (res->kinds & (_PyGC_DEAD_DEL | _PyGC_DEAD_FIRST | _PyGC_DEAD_HOOK
                          | _PyGC_DEAD_BUFFER | _PyGC_DEAD_WEAKREFS))
        || res->ndomain != 1 || res->domain[0] != interp
        || !_Py_IsMainInterpreter(interp) || gc->queue_phase != QUEUE_OPEN
        || gc->queue_head != NULL || gc->queue_draining || gc->debug
        || (gc->callbacks != NULL && PyList_GET_SIZE(gc->callbacks) != 0)
        || _PyRuntime.ref_tracer.tracer_func != NULL
        || interp->sys_tracing_threads || interp->sys_profiling_threads
        || (interp->eval_frame != NULL
            && interp->eval_frame != _PyEval_EvalFrameDefault))
    {
        return 0;
    }
    for (size_t event = 0; event < Py_ARRAY_LENGTH(interp->monitors.tools);
         event++)
    {
        if (interp->monitors.tools[event] != 0) {
            return 0;
        }
    }
    for (size_t tool = 0; tool < Py_ARRAY_LENGTH(interp->monitoring_callables);
         tool++)
    {
        if (interp->monitoring_tool_names[tool] != NULL) {
            return 0;
        }
        for (size_t event = 0;
             event < Py_ARRAY_LENGTH(interp->monitoring_callables[tool]); event++)
        {
            if (interp->monitoring_callables[tool][event] != NULL) {
                return 0;
            }
        }
    }
    int found = 0;
    for (Py_ssize_t i = 0; i < res->ndead; i++) {
        _PyGCDeadObject *d = &res->dead[i];
        PyObject *op = d->op;
        if ((d->kinds & _PyGC_DEAD_FINALIZE) && finalizer_eligible(op)) {
            if (!_PyGen_CanClearUnreachable(tstate, op)) {
                return 0;
            }
            found = 1;
        }
    }
    return found;
}

/* Bytes only: collect_pass owns this batch across the collection.  No
   Python writer may run while collect still borrows dead/garbage arrays. */
typedef struct {
    char *data;
    size_t used;
    size_t capacity;
    int to_stderr;
} debug_output;

/* Fail one buffer growth, independently of queue/order allocations. */
static int fail_debug_alloc_countdown;

int
_PyGC_LiveFailDebugAlloc(int n)
{
    int left = fail_debug_alloc_countdown;
    fail_debug_alloc_countdown = n;
    return left;
}

Py_GC_PRECISE static void
debug_flush(debug_output *out, int python_ok)
{
    for (size_t i = 0; i < out->used; ) {
        const char *line = out->data + i;
        size_t length = strlen(line) + 1;
        if (python_ok) {
            PySys_WriteStderr("%s", line);
        }
        else {
            fputs(line, stderr);
        }
        i += length;
    }
    free(out->data);
    out->data = NULL;
    out->used = out->capacity = 0;
}

static void
debug_write(debug_output *out, const char *format, ...)
{
    char buf[1000];
    va_list va;
    va_start(va, format);
    PyOS_vsnprintf(buf, sizeof(buf), format, va);
    va_end(va);
    if (out->to_stderr) {
        fputs(buf, stderr);
        return;
    }
    size_t length = strlen(buf) + 1;
    if (out->used > (size_t)PY_SSIZE_T_MAX - length) {
        goto fallback;
    }
    size_t needed = out->used + length;
    if (needed > out->capacity) {
        size_t capacity = out->capacity ? out->capacity : 1024;
        while (capacity < needed) {
            if (capacity > (size_t)PY_SSIZE_T_MAX / 2) {
                capacity = needed;
                break;
            }
            capacity *= 2;
        }
        if (fail_debug_alloc_countdown > 0
            && --fail_debug_alloc_countdown == 0)
        {
            goto fallback;
        }
        char *data = realloc(out->data, capacity);
        if (data == NULL) {
            goto fallback;
        }
        out->data = data;
        out->capacity = capacity;
    }
    memcpy(out->data + out->used, buf, length);
    out->used = needed;
    return;

fallback:
    /* Preserve the prefix and this record without a Python/OOM callback.
       Native stdio may block, but never releases this domain's GIL. */
    debug_flush(out, 0);
    fputs(buf, stderr);
    out->to_stderr = 1;
}

static void
debug_cycle(debug_output *out, const char *msg, PyObject *op)
{
    debug_write(out, "gc: %s <%s %p>\n", msg, Py_TYPE(op)->tp_name, (void *)op);
}

/* Preserve the existing public statistics protocol: experimental minors
   use generation0's ring; ordinary full collections use generation2. */
static void
record_stats(GCState *gc, const struct gc_generation_stats *cur, int young)
{
    struct gc_stats *gs = gc->generation_stats;
    uint32_t seq = _Py_atomic_load_uint32_relaxed(&gs->update_seq);
    // An odd sequence number tells a reader that an update is in progress.
    _Py_atomic_store_uint32_relaxed(&gs->update_seq, seq + 1);
    _Py_atomic_fence_seq_cst();
    struct gc_generation_stats *prev, *stats;
    if (young) {
        struct gc_young_stats_buffer *ring = &gs->young;
        prev = &ring->items[ring->index];
        ring->index = (ring->index + 1) % GC_YOUNG_STATS_SIZE;
        stats = &ring->items[ring->index];
    }
    else {
        struct gc_old_stats_buffer *ring = &gs->old[1];
        prev = &ring->items[ring->index];
        ring->index = (ring->index + 1) % GC_OLD_STATS_SIZE;
        stats = &ring->items[ring->index];
    }
    *stats = *prev;
    stats->ts_start = cur->ts_start;
    stats->ts_stop = cur->ts_stop;
    stats->collections += cur->collections;
    stats->collected += cur->collected;
    stats->uncollectable += cur->uncollectable;
    stats->candidates += cur->candidates;
    stats->duration += cur->duration;
    stats->heap_size = cur->heap_size;
    _Py_atomic_store_uint32_release(&gs->update_seq, seq + 2);
}

/* The deletion barrier of test 9.16 (_testinternalcapi.gc_delete_barrier()).
   Armed, the next deletion of a subinterpreter stops at the start of its
   pin wait, after its close, with the head lock released, until the main
   interpreter's next collection whose domain names it has marked.  That
   collection lets it go, waits until it blocks on the pin, then routes and
   unpins.  The sequence numbers order the routing of a closed owner's
   calls and the deletion's passing of its pin wait, which precedes the
   unlink. */
static struct {
    int armed;
    PyInterpreterState *deleting;  // stopped at the barrier
    PyEvent go;
    int pin_waited;   // the deletion was seen blocked on the pin
    int timed_out;    // ... or was not, within BARRIER_WAIT_NS
    int64_t seq;
    int64_t routed;   // the seq of the routing of a closed owner's calls
    int64_t unpinned; // the seq of the deletion's passing of its pin wait
    Py_ssize_t remapped;  // the calls routed from a closed owner to main
    Py_ssize_t remapped_other;  // ... from an owner not at the barrier
} barrier;

void
_PyGC_LiveDeleteBarrierForTest(int on)
{
    memset(&barrier, 0, sizeof(barrier));
    barrier.armed = on;
}

static void
barrier_at_delete(PyInterpreterState *interp)
{
    if (!barrier.armed || _Py_IsMainInterpreter(interp)) {
        return;
    }
    barrier.armed = 0;
    _Py_atomic_store_ptr(&barrier.deleting, interp);
    HEAD_UNLOCK(&_PyRuntime);
    PyEvent_Wait(&barrier.go);
    HEAD_LOCK(&_PyRuntime);
}

#define BARRIER_WAIT_NS ((PyTime_t)10 * 1000 * 1000 * 1000)

static void
barrier_after_mark(PyThreadState *tstate, const _PyGCMarkResult *res)
{
    PyInterpreterState *deleting = _Py_atomic_load_ptr(&barrier.deleting);
    if (deleting == NULL || _PyEvent_IsSet(&barrier.go)
        || !_Py_IsMainInterpreter(tstate->interp))
    {
        return;
    }
    int named = 0;
    for (int d = 0; d < res->ndomain; d++) {
        named |= res->domain[d] == deleting;
    }
    if (!named) {
        return;
    }
    _PyEvent_Notify(&barrier.go);
    // The deletion is now in its pin wait or on its way there; this
    // collection holds the pin, so it gets there, unless its thread died
    // or hung: then the test fails on "timed_out" instead of hanging.
    PyTime_t until, now;
    (void)PyTime_MonotonicRaw(&until);
    until += BARRIER_WAIT_NS;
    for (;;) {
        HEAD_LOCK(&_PyRuntime);
        int waiting = deleting->gc.queue_unpinned != NULL;
        HEAD_UNLOCK(&_PyRuntime);
        if (waiting) {
            barrier.pin_waited = 1;
            return;
        }
        (void)PyTime_MonotonicRaw(&now);
        if (now >= until) {
            barrier.timed_out = 1;
            return;
        }
    }
}

/* Splice each call of the batch onto its owner's queue, in batch order
   (note 13 section 3.2): the owner of a weak reference callback is the
   weak reference's, of every other call its object's; an object of a
   closed interpreter belongs to the main interpreter, which owns that heap
   from the close; one in no heap of the domain, the collecting
   interpreter.  Allocates nothing.  An owner other than this one learns of
   its calls when one of its threads next takes the GIL. */
static void
route_calls(PyThreadState *tstate, const _PyGCMarkResult *res,
            call_batch *b)
{
    PyInterpreterState *here = tstate->interp;
    if (res->ndomain <= 1) {
        queue_splice(&here->gc, b);
        return;
    }
    PyInterpreterState *main = _PyInterpreterState_Main();
    live_call *next;
    for (live_call *c = b->head; c != NULL; c = next) {
        next = c->next;
        PyInterpreterState *owner = _PyGC_LiveOwner(res, c->op);
        if (owner == NULL) {
            owner = here;
        }
        if (owner->gc.queue_phase == QUEUE_CLOSED) {
            barrier.remapped_other +=
                owner != _Py_atomic_load_ptr(&barrier.deleting);
            owner = main;
            barrier.remapped++;
            barrier.routed = _Py_atomic_add_int64(&barrier.seq, 1) + 1;
        }
        call_batch one = {c, c};
        c->next = NULL;
        queue_splice(&owner->gc, &one);
    }
    b->head = b->tail = NULL;
}

/* The end of the pin that _PyGC_TraceLiveMark() took. */
static void
unpin_domain(_PyGCMarkResult *res)
{
    if (res->ndomain == 0) {
        return;
    }
    HEAD_LOCK(&_PyRuntime);
    for (int d = 0; d < res->ndomain; d++) {
        GCState *gc = &res->domain[d]->gc;
        assert(gc->queue_pins > 0);
        if (--gc->queue_pins == 0 && gc->queue_unpinned != NULL) {
            _PyEvent_Notify(gc->queue_unpinned);
            gc->queue_unpinned = NULL;
        }
    }
    HEAD_UNLOCK(&_PyRuntime);
    res->ndomain = 0;
}

/* Called by PyInterpreterState_Delete() with the head lock held, before it
   unlinks interp: returns, with the lock held, once no collection pins
   interp.  The collections it waits for need nothing from this thread,
   which holds neither the GIL nor, while it waits, the head lock. */
void
_PyGC_LiveWaitUnpinned(PyInterpreterState *interp)
{
    int stopped = barrier.armed;
    barrier_at_delete(interp);
    while (interp->gc.queue_pins > 0) {
        PyEvent unpinned = {0};
        interp->gc.queue_unpinned = &unpinned;
        HEAD_UNLOCK(&_PyRuntime);
        PyEvent_Wait(&unpinned);
        HEAD_LOCK(&_PyRuntime);
    }
    if (stopped && !barrier.armed) {
        barrier.unpinned = _Py_atomic_add_int64(&barrier.seq, 1) + 1;
    }
}

/* PYTHON_GCSTRESS=N: every Nth object allocation of a thread collects
   through the inline path, as an allocation past the threshold does.  The
   limit stays at 0, so that each allocation reaches the count. */
static int stress_every = 0;

/* Not inlined: its frame is below the collector's entry, so the walk never
   scans it, and it stays opaque (OUTLINED_OPAQUE in gc_inline_audit.py).
   Inlined, its body would be part of collect_pass()'s precise frame, which
   a collection from the queue's code walks. */
static Py_NO_INLINE Py_ssize_t
collect(PyThreadState *tstate, struct _gc_live_state *ls,
        _PyGC_Reason reason, debug_output *out, int young)
{
    PyInterpreterState *interp = tstate->interp;
    GCState *gc = &interp->gc;
    if (gc->collecting || gc->sweeping) {
        return 0;
    }
    if (gc->generation_stats == NULL) {
        return 0;  // before _PyGC_Init()
    }
    gc->collecting = 1;
    // The frame that a sampling profiler shows a "<GC>" frame above
    gc->frame = tstate->current_frame;
    ls->exc = _PyErr_GetRaisedException(tstate);
    struct gc_generation_stats cur = {0};
    invoke_gc_callback(tstate, "start", &cur, young);

    PyTime_t t0 = 0, t1 = 0;
    (void)PyTime_PerfCounterRaw(&t0);
    _PyGCMarkResult res;
    objstack garbage = {0};
    _PyGCRoot garbage_root;
    int garbage_rooted = 0;
    Py_ssize_t ncollected = 0, bytes_freed = 0;
    if (_PyGC_TraceLiveMark(tstate, &res, young) < 0) {
        unpin_domain(&res);
        goto done;  // out of memory: nothing is freed
    }
    add_root_stats(ls, &res.roots);
    call_batch batch = {NULL, NULL};
    Py_ssize_t *first = NULL;
    Py_ssize_t nfirst = 0;
    int native_cleanup = native_cleanup_ready(tstate, &res);
    int failed = 0;
    if (native_cleanup) {
        // Prepare everything fallible before touching a frame.  This cohort
        // currently has no referent-reading deallocators, so no order storage.
        failed = order_dead_first(&res, &first, &nfirst) < 0;
        if (!failed && !native_cleanup_ready(tstate, &res)) {
            free(first);
            first = NULL;
            nfirst = 0;
            native_cleanup = 0;
        }
    }
    if (!native_cleanup) {
        failed = clear_callback_weakrefs(&batch, &res) < 0
                 || handle_finalizers(gc, &batch, &res, &garbage) < 0
                 || queue_destroy_hooks(&batch, &res) < 0;
    }
    // Before the sweep; no Python code has run since the mark.
    barrier_after_mark(tstate, &res);
    route_calls(tstate, &res, &batch);
    unpin_domain(&res);
    if (!failed) {
        for (Py_ssize_t i = 0; !native_cleanup &&
             (res.kinds & _PyGC_DEAD_WEAKREFS) && i < res.ndead; i++)
        {
            PyObject *op = res.dead[i].op;
            int kinds = res.dead[i].kinds;
            if ((kinds & _PyGC_DEAD_WEAKREFS)
                && ((op->ob_flags & _Py_GC_DEAD_FLAG)
                    || (kinds & _PyGC_DEAD_HOOK)))
            {
                _PyWeakref_ClearWeakRefsNoCallbacks(op);
            }
        }
        if (gc->debug & _PyGC_DEBUG_COLLECTABLE) {
            // Copy text now; the eventual writer must not borrow objects.
            for (Py_ssize_t i = 0; i < res.ndead; i++) {
                PyObject *op = res.dead[i].op;
                if ((op->ob_flags & _Py_GC_DEAD_FLAG) && _PyObject_IS_GC(op)) {
                    debug_cycle(out, "collectable", op);
                }
            }
        }
        if (!native_cleanup) {
            failed = order_dead_first(&res, &first, &nfirst) < 0;
        }
        if (!failed) {
            ncollected = sweep(tstate, gc, &res, first, nfirst,
                               native_cleanup, &bytes_freed);
        }
    }
    if (failed) {
        // Out of memory: keep everything.  What was queued runs.
        undo_dead(&res);
        garbage.n = 0;
    }
    free(first);
    (void)PyTime_PerfCounterRaw(&t1);

    ls->bytes_freed += bytes_freed;
    ls->live_bytes = res.live_bytes;
    ls->pause_max_ns = Py_MAX(ls->pause_max_ns, (Py_ssize_t)(t1 - t0));
    cur.collections = 1;
    cur.collected = ncollected;
    cur.uncollectable = (Py_ssize_t)garbage.n;
    cur.candidates = res.nobjects;
    cur.duration = PyTime_AsSecondsDouble(t1 - t0);
    cur.ts_start = t0;
    cur.ts_stop = t1;
    cur.heap_size = res.nobjects;
    record_stats(gc, &cur, young);
    gc->heap_size = failed ? res.nobjects : res.nobjects - res.ndead;
    if (!young) {
        gc->long_lived_total = gc->heap_size;
        // A minor must not postpone the next automatic major.
        gc->alloc_bytes = 0;
        gc->alloc_limit = gc->alloc_threshold > 0
            ? Py_MAX(gc->alloc_threshold, res.live_bytes) : PY_SSIZE_T_MAX;
        if (stress_every > 0) {
            gc->alloc_limit = 0;
        }
    }
    free(res.dead);

    if (garbage.n != 0) {
        assert(garbage.n <= (size_t)res.ndead);
        _PyGC_AddRoot(&garbage_root, garbage.items, (Py_ssize_t)garbage.n);
        garbage_rooted = 1;
    }
    if (gc->debug & _PyGC_DEBUG_STATS) {
        debug_write(out, "gc: %zd objects, %zd collected, %zd bytes freed, "
                    "%zd live bytes, %.4fs elapsed\n",
                    res.nobjects, ncollected, bytes_freed,
                    res.live_bytes, cur.duration);
    }
    if (!ls->skip_verify) {
        _PyGC_TraceAfterCollect(tstate, young ? 0 : NUM_GENERATIONS - 1,
                                reason == _Py_GC_REASON_MANUAL);
    }
    for (size_t i = 0; i < garbage.n; i++) {
        if (gc->debug & _PyGC_DEBUG_UNCOLLECTABLE) {
            debug_cycle(out, "uncollectable", garbage.items[i]);
        }
    }
    for (size_t i = 0; gc->garbage != NULL && i < garbage.n; i++) {
        if (PyList_Append(gc->garbage, garbage.items[i]) < 0) {
            PyErr_Clear();
            break;
        }
    }
done:
    assert(res.ndomain == 0);  // unpinned on every path
    if (garbage_rooted) {
        _PyGC_RemoveRoot(&garbage_root);
    }
    free(garbage.items);
    invoke_gc_callback(tstate, "stop", &cur, young);
    gc->frame = NULL;
    gc->collecting = 0;
    _PyErr_SetRaisedException(tstate, ls->exc);
    ls->exc = NULL;
    return ncollected + cur.uncollectable;
}

/* One pass: collect, then run the queue.  The object allocations of the
   collection and of the queue go to the heap for non-GC objects, whatever
   the allocation that triggered it had selected
   (_PyObject_MallocWithType()).  Returns finalizers, tp_dels and actually
   consumed memory acquisitions in *ncompleted, after error reporting. */
Py_GC_PRECISE static Py_NO_INLINE Py_ssize_t
collect_pass(PyThreadState *tstate, struct _gc_live_state *ls,
             _PyGC_Reason reason, int *ncompleted, int young)
{
    _PyThreadStateImpl *ts = (_PyThreadStateImpl *)tstate;
    struct _mimalloc_thread_state *m = &ts->mimalloc;
    mi_heap_t *heap = m->current_object_heap;
    if (heap != NULL) {
        m->current_object_heap = &m->heaps[_Py_MIMALLOC_HEAP_OBJECT];
    }
    debug_output out = {.to_stderr = ls->no_python || !live_python_ok(tstate)};
    Py_ssize_t n = collect(tstate, ls, reason, &out, young);
    // A collection that the queue's code triggers records its own entry.
    ts->gc_trace.entry.sp = 0;
    int python_ok = !ls->no_python && live_python_ok(tstate);
    debug_flush(&out, python_ok);
    *ncompleted = python_ok ? run_queue(tstate) : 0;
    if (heap != NULL) {
        m->current_object_heap = heap;
    }
    return n;
}

/* The entry of every collection.  The mark scans this thread's stack from
   here up, with the registers as they are here: the frames below are the
   collector's own, over stale words that earlier, deeper calls left, which
   would keep garbage alive.  Not inlined, so that the callers' frames are
   above.  Precise, so the walk does not scan its own frame. */
Py_GC_PRECISE static Py_NO_INLINE Py_ssize_t
collect_from_entry(PyThreadState *tstate, _PyGC_Reason reason, int young)
{
    _PyThreadStateImpl *ts = (_PyThreadStateImpl *)tstate;
    // After the close (_PyGC_Fini()) there is no live state and nothing
    // collects; nor without memory for one.
    struct _gc_live_state *ls = get_live(tstate->interp);
    if (ls == NULL) {
        return 0;
    }
    // Called again from a gc callback, inside a collection: the outer entry
    // stays, since the mark that may follow is the outer collection's.
    _PyGCContext outer_entry = ts->gc_trace.entry;
    if (outer_entry.sp == 0) {
        _PyGC_CaptureCaller(&ts->gc_trace.entry);
    }
    int ncompleted;
    Py_ssize_t n = collect_pass(tstate, ls, reason, &ncompleted, young);
    /* The objects that finalizers did not resurrect are garbage again.  An
       explicit collection frees them, as the reference counting build
       does within one collection; an automatic one leaves them to the
       next, and so does a shutdown collection before the modules are
       cleared, which a later one follows.  Consumed memory acquisitions can
       make another owner ready.  Retain the bounded second pass; it does not
       promise to exhaust arbitrary acquisition-chain depth. */
    PyInterpreterState *interp = tstate->interp;
    int later_follows = _PyInterpreterState_GetFinalizing(interp) != NULL
                        && interp->imports.modules != NULL;
    if (ncompleted > 0 && reason != _Py_GC_REASON_HEAP && !later_follows
        && outer_entry.sp == 0)
    {
        _PyGC_CaptureCaller(&ts->gc_trace.entry);
        ls->skip_verify = 1;
        n += collect_pass(tstate, ls, reason, &ncompleted, young);
        ls->skip_verify = 0;
    }
    ts->gc_trace.entry = outer_entry;
    return n;
}


/* Can an automatic collection run now?  If not, it is tried again after
   RETRY_BYTES more. */
Py_GC_PRECISE static int
can_collect(PyThreadState *tstate)
{
    PyInterpreterState *interp = tstate->interp;
    GCState *gc = &interp->gc;
    int ok = gc->enabled && gc->alloc_threshold > 0
        && !gc->collecting && !gc->sweeping
        && tstate->state == _Py_THREAD_ATTACHED
        && ((_PyThreadStateImpl *)tstate)->mimalloc.current_object_heap != NULL
        && ((_PyThreadStateImpl *)tstate)->gc_trace.in_tracer == 0
        && _PyInterpreterState_GetFinalizing(interp) == NULL
        && _PyRuntimeState_GetFinalizing(&_PyRuntime) == NULL;
    if (ok) {
        /* The mark takes the head lock; an allocation made while this
           thread holds it cannot collect. */
        PyMutex *m = &_PyRuntime.interpreters.mutex;
        ok = _PyMutex_LockTimed(m, 0, _Py_LOCK_DONT_DETACH) == PY_LOCK_ACQUIRED;
        if (ok) {
            PyMutex_Unlock(m);
        }
    }
    if (!ok) {
        gc->alloc_limit = gc->alloc_bytes + RETRY_BYTES;
    }
    return ok;
}

int
_PyGC_LiveGetStress(void)
{
    return stress_every;
}

void
_PyGC_LiveSetStress(PyInterpreterState *interp, int every)
{
    GCState *gc = &interp->gc;
    stress_every = every;
    gc->alloc_limit = every > 0 ? 0 : gc->alloc_bytes + gc->alloc_threshold;
}

Py_GC_PRECISE void
_PyGC_InjectCollect(PyThreadState *tstate)
{
    _PyThreadStateImpl *ts = (_PyThreadStateImpl *)tstate;
    ts->gc_trace.inject_site = 0;
    if (_PyGC_LiveCollectHere(tstate)) {
        ts->gc_trace.injected++;
    }
}

Py_GC_PRECISE void
_PyGC_LiveOnThreshold(PyThreadState *tstate)
{
    GCState *gc = &tstate->interp->gc;
    if (stress_every > 0) {
        _PyThreadStateImpl *ts = (_PyThreadStateImpl *)tstate;
        if (++ts->gc_trace.stress_count >= stress_every) {
            ts->gc_trace.stress_count = 0;
            (void)_PyGC_LiveCollectHere(tstate);
        }
        gc->alloc_limit = 0;
        return;
    }
    /* Finalizers, weak reference callbacks and gc.callbacks run Python code,
       which is safe only where the eval loop checks its breaker: the
       collection waits for that, as the cycle collector's does.  Code that
       allocates a further threshold without reaching the eval loop collects
       here, and leaves the Python code to the breaker. */
    if (gc->alloc_bytes - gc->alloc_limit < gc->alloc_threshold) {
        _Py_ScheduleGC(tstate);
        return;
    }
    (void)_PyGC_LiveCollectHere(tstate);
}

Py_GC_PRECISE int
_PyGC_LiveCollectHere(PyThreadState *tstate)
{
    if (!can_collect(tstate)) {
        return 0;
    }
    struct _gc_live_state *ls = get_live(tstate->interp);
    if (ls == NULL) {
        return 0;  // closed, or no memory: as in collect_from_entry()
    }
    ls->no_python = 1;
    (void)collect_from_entry(tstate, _Py_GC_REASON_HEAP, 0);
    ls->no_python = 0;
    if (tstate->interp->gc.queue_head != NULL) {
        _Py_ScheduleGC(tstate);
    }
    return 1;
}


void
_PyGC_LiveKeepThreads(PyThreadState *list)
{
    if (list == NULL) {
        return;
    }
    struct _gc_live_state *ls = get_live(list->interp);
    if (ls == NULL) {
        return;
    }
    // An interpreter is finalized once.
    assert(ls->kept_threads == NULL);
    // The reference keeps a thread that hangs from freeing its state.
    for (PyThreadState *p = list; p != NULL; p = p->next) {
        _Py_atomic_add_ssize(&((_PyThreadStateImpl *)p)->refcount, 1);
    }
    ls->kept_threads = list;
}

PyThreadState *
_PyGC_LiveKeptThreads(PyInterpreterState *interp)
{
    struct _gc_live_state *ls = get_live(interp);
    return ls == NULL ? NULL : ls->kept_threads;
}


/* --- the interface of Python/gc.c --------------------------------------- */

PyStatus
_PyGC_Init(PyInterpreterState *interp)
{
    GCState *gcstate = &interp->gc;
    gcstate->young_enabled = 0;
    gcstate->young_pending = 0;
    // The main interpreter's state is static: after Py_FinalizeEx(), its
    // queue is closed and empty.
    assert(gcstate->queue_head == NULL);
    gcstate->queue_phase = QUEUE_INITIALISING;
    gcstate->generation_stats = PyMem_RawCalloc(1, sizeof(struct gc_stats));
    if (gcstate->generation_stats == NULL) {
        return _PyStatus_NO_MEMORY();
    }
    if (get_live(interp) == NULL) {
        return _PyStatus_NO_MEMORY();
    }
    gcstate->garbage = PyList_New(0);
    if (gcstate->garbage == NULL) {
        return _PyStatus_NO_MEMORY();
    }
    PyStatus status = _PyGC_TraceInit(interp);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }
    gcstate->callbacks = PyList_New(0);
    if (gcstate->callbacks == NULL) {
        return _PyStatus_NO_MEMORY();
    }
    gcstate->alloc_limit = gcstate->alloc_bytes + gcstate->alloc_threshold;
    if (_Py_IsMainInterpreter(interp) && interp->config.use_environment) {
        const char *env = getenv("PYTHON_GCSTRESS");
        if (env != NULL && *env != '\0') {
            char *end;
            long n = strtol(env, &end, 10);
            if (*end != '\0' || n < 0 || n > INT_MAX) {
                return _PyStatus_ERR("PYTHON_GCSTRESS must be a count of "
                                     "allocations");
            }
            stress_every = (int)n;
        }
    }
    if (stress_every > 0) {
        gcstate->alloc_limit = 0;
    }
    return _PyStatus_OK();
}

static int fail_open_armed = 0;
static PyObject *fail_open_referent = NULL;
static PyObject *fail_open_callback = NULL;

void
_PyGC_LiveFailOpenForTest(PyObject *referent, PyObject *callback)
{
    fail_open_armed = 1;
    fail_open_referent = referent;
    fail_open_callback = callback;
}

/* The armed test hook: queue one call, which cannot run, and fail. */
static PyStatus
fail_open(PyInterpreterState *interp)
{
    fail_open_armed = 0;
    PyObject *op;
    live_call *c;
    if (fail_open_referent != NULL) {
        op = PyWeakref_NewRef(fail_open_referent, fail_open_callback);
        c = op == NULL ? NULL
            : call_new(op, fail_open_callback, CALL_CALLBACK);
    }
    else {
        op = PyList_New(0);
        c = op == NULL ? NULL : call_new(op, NULL, CALL_FINALIZE);
    }
    fail_open_referent = fail_open_callback = NULL;
    if (c == NULL) {
        return _PyStatus_NO_MEMORY();
    }
    call_batch b = {NULL, NULL};
    batch_append(&b, c);
    queue_splice(&interp->gc, &b);
    return _PyStatus_ERR("gc_queue_never_open_for_test");
}

/* The interpreter can run Python code from now on (note 13 section 4). */
PyStatus
_PyGC_LiveOpenQueue(PyInterpreterState *interp)
{
    if (fail_open_armed) {
        return fail_open(interp);
    }
    GCState *gc = &interp->gc;
    assert(gc->queue_phase == QUEUE_INITIALISING);
    gc->queue_phase = QUEUE_OPEN;
    PyThreadState *tstate = _PyThreadState_GET();
    if (gc->queue_head != NULL) {
        _Py_atomic_store_int_relaxed(&gc->queue_ready, 1);
        if (tstate != NULL && tstate->interp == interp) {
            _Py_ScheduleGC(tstate);
        }
    }
    return _PyStatus_OK();
}

/* The close (note 13 section 5.1): an open queue drains until empty, on the
   interpreter's last thread, after its last collection.  A queue that never
   opened runs nothing here: a shared-heap interpreter's calls go to the
   main interpreter, which owns its heap from the close; an isolated one's
   are discarded, since no collection visits that heap again, and counted
   in the main interpreter's state. */
static void
live_close(PyInterpreterState *interp)
{
    GCState *gc = &interp->gc;
    if (gc->queue_draining) {
        Py_FatalError("cannot close a collector while callbacks are running");
    }
    PyThreadState *tstate = _PyThreadState_GET();
    /* The fork child clears the other interpreters from the main
       interpreter's thread (_PyInterpreterState_DeleteExceptMain()): their
       queues cannot run there, and are treated as never opened, until
       rung 2's disposal (note 13 section 5.5). */
    if (gc->queue_phase == QUEUE_OPEN && tstate != NULL
        && tstate->interp == interp)
    {
        assert(!((_PyThreadStateImpl *)tstate)->gc_trace.running_queue);
        while (gc->queue_head != NULL) {
            (void)run_queue_ex(tstate, 1);
        }
    }
    PyInterpreterState *main = _PyInterpreterState_Main();
    _PyRuntimeState *runtime = &_PyRuntime;
    HEAD_LOCK(runtime);
    if (gc->queue_head != NULL) {
        GCState *mgc = main != NULL ? &main->gc : NULL;
        if (main != NULL && interp != main
            && (interp->feature_flags & Py_RTFLAGS_USE_MAIN_OBMALLOC)
            && mgc->queue_phase != QUEUE_CLOSED)
        {
            call_batch b = {gc->queue_head, gc->queue_tail};
            queue_splice(mgc, &b);
        }
        else {
            live_call *next;
            for (live_call *c = gc->queue_head; c != NULL; c = next) {
                next = c->next;
                if (mgc != NULL) {
                    mgc->queue_discarded[c->kind]++;
                }
                free(c);
            }
        }
        gc->queue_head = gc->queue_tail = NULL;
    }
    gc->queue_phase = QUEUE_CLOSED;
    _Py_atomic_store_int_relaxed(&gc->queue_ready, 0);
    HEAD_UNLOCK(runtime);
}

void
_PyGC_Fini(PyInterpreterState *interp)
{
    GCState *gcstate = &interp->gc;
    live_close(interp);
    gcstate->alloc_limit = PY_SSIZE_T_MAX;
    gcstate->garbage = NULL;
    gcstate->callbacks = NULL;
    PyMem_RawFree(gcstate->generation_stats);
    gcstate->generation_stats = NULL;
    _PyGC_TraceFini(interp);
    struct _gc_live_state *ls = gcstate->live;
    if (ls != NULL) {
        dump_root_stats(interp, ls);
        _PyGC_WalkCacheFini(&ls->walk_cache);
        gcstate->live = NULL;
        free(ls->immortal);
        free(ls);
    }
}

void
_PyGC_LiveCollectAfterCloseForTest(PyThreadState *tstate, Py_ssize_t out[6])
{
    PyInterpreterState *interp = tstate->interp;
    _PyThreadStateImpl *ts = (_PyThreadStateImpl *)tstate;
    _PyGC_Fini(interp);
    _PyGC_CollectNoFail(tstate);
    out[0] = _PyGC_Collect(tstate, NUM_GENERATIONS - 1, _Py_GC_REASON_MANUAL);
    out[1] = PyGC_Collect();
    int64_t injected = ts->gc_trace.injected;
    _PyGC_InjectCollect(tstate);
    out[2] = ts->gc_trace.injected != injected;
    _Py_RunGC(tstate);
    out[3] = interp->gc.live != NULL;
    out[4] = 0;
    for (live_call *c = interp->gc.queue_head; c != NULL; c = c->next) {
        out[4]++;
    }
    out[5] = _Py_atomic_load_int_relaxed(&interp->gc.queue_ready);
}

int
_PyGC_VisitValue(PyValue *ref, visitproc visit, void *arg)
{
    assert(PyValue_IsObject(*ref));
    Py_VISIT(PyValue_ObjectBorrow(*ref));
    return 0;
}

/* Every slot holds a value or NULL (pycore_interpframe_structs.h). */
int
_PyGC_VisitFrameStack(_PyInterpreterFrame *frame, visitproc visit, void *arg)
{
    _PyGCValueRange values = _PyFrame_GCValues(frame);
    for (Py_ssize_t i = 0; i < values.n; i++) {
        _Py_VISIT_VALUE(values.base[i]);
    }
    return 0;
}

/* Every tracked GC object, after a collection.  The caller visits them with
   automatic collection blocked, and frees the array. */
static PyObject **
snapshot(PyThreadState *tstate, Py_ssize_t *n)
{
    (void)collect_from_entry(tstate, _Py_GC_REASON_MANUAL, 0);
    Py_ssize_t nobjs;
    PyObject **objs = _PyGC_TraceLiveHeapObjects(tstate->interp, &nobjs);
    if (objs == NULL) {
        *n = 0;
        return NULL;
    }
    Py_ssize_t j = 0;
    for (Py_ssize_t i = 0; i < nobjs; i++) {
        PyObject *op = objs[i];
        if (_PyObject_IS_GC(op) && _PyObject_GC_IS_TRACKED(op)) {
            objs[j++] = op;
        }
    }
    *n = j;
    return objs;
}

static int
referrersvisit(PyObject *obj, void *arg)
{
    PyObject *objs = arg;
    for (Py_ssize_t i = 0; i < PyTuple_GET_SIZE(objs); i++) {
        if (PyTuple_GET_ITEM(objs, i) == obj) {
            return 1;
        }
    }
    return 0;
}

PyObject *
_PyGC_GetReferrers(PyInterpreterState *interp, PyObject *objs)
{
    PyThreadState *tstate = _PyThreadState_GET();
    GCState *gc = &interp->gc;
    Py_ssize_t n;
    PyObject **all = snapshot(tstate, &n);
    if (all == NULL) {
        return PyErr_NoMemory();
    }
    gc->collecting++;
    PyObject *result = PyList_New(0);
    for (Py_ssize_t i = 0; result != NULL && i < n; i++) {
        PyObject *obj = all[i];
        traverseproc traverse = Py_TYPE(obj)->tp_traverse;
        if (obj == objs || obj == result || traverse == NULL) {
            continue;
        }
        if (traverse(obj, referrersvisit, objs)) {
            if (PyList_Append(result, obj) < 0) {
                Py_CLEAR(result);
            }
        }
    }
    gc->collecting--;
    free(all);
    return result;
}

/* Normally generation2 and None list every object.  The opt-in experiment
   additionally exposes its young cohort through generation0. */
PyObject *
_PyGC_GetObjects(PyInterpreterState *interp, int generation)
{
    int ages = interp->gc.young_enabled;
    if (generation == 1 || (generation == 0 && !ages)) {
        return PyList_New(0);
    }
    PyThreadState *tstate = _PyThreadState_GET();
    GCState *gc = &interp->gc;
    Py_ssize_t n;
    PyObject **all = ages ? _PyGC_TraceLiveHeapObjects(interp, &n)
                         : snapshot(tstate, &n);
    if (all == NULL) {
        return PyErr_NoMemory();
    }
    gc->collecting++;
    PyObject *result = PyList_New(0);
    for (Py_ssize_t i = 0; result != NULL && i < n; i++) {
        if (!_PyObject_IS_GC(all[i]) || !_PyObject_GC_IS_TRACKED(all[i])) {
            continue;
        }
        int is_young = (all[i]->ob_flags & _Py_GC_YOUNG_FLAG) != 0;
        if (ages && generation >= 0
            && (generation == 0 ? !is_young : is_young))
        {
            continue;
        }
        if (PyList_Append(result, all[i]) < 0) {
            Py_CLEAR(result);
        }
    }
    gc->collecting--;
    free(all);
    return result;
}

void
PyUnstable_GC_VisitObjects(gcvisitobjects_t callback, void *arg)
{
    PyThreadState *tstate = _PyThreadState_GET();
    GCState *gc = &tstate->interp->gc;
    Py_ssize_t n;
    PyObject **all = snapshot(tstate, &n);
    if (all == NULL) {
        return;
    }
    gc->collecting++;
    for (Py_ssize_t i = 0; i < n; i++) {
        if (!callback(all[i], arg)) {
            break;
        }
    }
    gc->collecting--;
    free(all);
}

/* Nothing is ever moved between generations. */
void
_PyGC_Freeze(PyInterpreterState *interp)
{
}

void
_PyGC_Unfreeze(PyInterpreterState *interp)
{
}

Py_ssize_t
_PyGC_GetFreezeCount(PyInterpreterState *interp)
{
    return 0;
}

int
PyGC_Enable(void)
{
    GCState *gcstate = get_gc_state();
    int old_state = gcstate->enabled;
    gcstate->enabled = 1;
    return old_state;
}

int
PyGC_Disable(void)
{
    GCState *gcstate = get_gc_state();
    int old_state = gcstate->enabled;
    gcstate->enabled = 0;
    return old_state;
}

int
PyGC_IsEnabled(void)
{
    GCState *gcstate = get_gc_state();
    return gcstate->enabled;
}

Py_ssize_t
PyGC_Collect(void)
{
    PyThreadState *tstate = _PyThreadState_GET();
    if (!tstate->interp->gc.enabled) {
        return 0;
    }
    return collect_from_entry(tstate, _Py_GC_REASON_MANUAL, 0);
}

Py_GC_PRECISE Py_ssize_t
_PyGC_Collect(PyThreadState *tstate, int generation, _PyGC_Reason reason)
{
    int young = generation == 0 && tstate->interp->gc.young_enabled;
    if (young) {
        int extra;
        if (_PyGC_TraceGetVerifyMode(tstate->interp, &extra) != 0
            || stress_every != 0)
        {
            PyErr_SetString(PyExc_RuntimeError,
                            "experimental minor requires verifier and stress off");
            return -1;
        }
    }
    return collect_from_entry(tstate, reason, young);
}

Py_GC_PRECISE void
_PyGC_CollectNoFail(PyThreadState *tstate)
{
    (void)collect_from_entry(tstate, _Py_GC_REASON_SHUTDOWN, 0);
}

void
_PyGC_DumpShutdownStats(PyInterpreterState *interp)
{
    GCState *gcstate = &interp->gc;
    if (!(gcstate->debug & _PyGC_DEBUG_SAVEALL)
        && gcstate->garbage != NULL && PyList_GET_SIZE(gcstate->garbage) > 0) {
        const char *message;
        if (gcstate->debug & _PyGC_DEBUG_UNCOLLECTABLE) {
            message = "gc: %zd uncollectable objects at shutdown";
        }
        else {
            message = "gc: %zd uncollectable objects at shutdown; " \
                "use gc.set_debug(gc.DEBUG_UNCOLLECTABLE) to list them";
        }
        if (PyErr_WarnExplicitFormat(PyExc_ResourceWarning, "gc", 0,
                                     "gc", NULL, message,
                                     PyList_GET_SIZE(gcstate->garbage)))
        {
            PyErr_FormatUnraisable("Exception ignored in GC shutdown");
        }
        if (gcstate->debug & _PyGC_DEBUG_UNCOLLECTABLE) {
            PyObject *repr = NULL, *bytes = NULL;
            repr = PyObject_Repr(gcstate->garbage);
            if (!repr || !(bytes = PyUnicode_EncodeFSDefault(repr))) {
                PyErr_FormatUnraisable("Exception ignored in GC shutdown "
                                       "while formatting garbage");
            }
            else {
                PySys_WriteStderr("      %s\n", PyBytes_AS_STRING(bytes));
            }
        }
    }
}

#ifdef Py_DEBUG
static int
visit_validate(PyObject *op, void *parent_raw)
{
    PyObject *parent = _PyObject_CAST(parent_raw);
    if (_PyObject_IsFreed(op)) {
        _PyObject_ASSERT_FAILED_MSG(parent,
                                    "PyObject_GC_Track() object is not valid");
    }
    return 0;
}
#endif

/* Tracking says that a GC object is fully built: a conservative root may
   traverse it. */
void
PyObject_GC_Track(void *op_raw)
{
    PyObject *op = _PyObject_CAST(op_raw);
    if (_PyObject_GC_IS_TRACKED(op)) {
        _PyObject_ASSERT_FAILED_MSG(op,
                                    "object already tracked "
                                    "by the garbage collector");
    }
    _PyObject_GC_TRACK(op);
#ifdef Py_DEBUG
    traverseproc traverse = Py_TYPE(op)->tp_traverse;
    (void)traverse(op, visit_validate, op);
#endif
}

void
PyObject_GC_UnTrack(void *op_raw)
{
    PyObject *op = _PyObject_CAST(op_raw);
    if (_PyObject_GC_IS_TRACKED(op)) {
        _PyObject_GC_UNTRACK(op);
    }
}

int
PyObject_IS_GC(PyObject *obj)
{
    return _PyObject_IS_GC(obj);
}

void
_Py_ScheduleGC(PyThreadState *tstate)
{
    if (!_Py_eval_breaker_bit_is_set(tstate, _PY_GC_SCHEDULED_BIT)) {
        _Py_set_eval_breaker_bit(tstate, _PY_GC_SCHEDULED_BIT);
    }
}

PyObject *
_PyGC_LiveQueueInfo(PyInterpreterState *interp)
{
    Py_ssize_t length = 0;
    for (live_call *c = interp->gc.queue_head; c != NULL; c = c->next) {
        length++;
    }
    static const char *const phases[] = {"initialising", "open", "closed"};
    GCState *gc = &interp->gc;
    return Py_BuildValue("{snsssis{sisisisLsLsnsn}s{snsnsnsn}}",
        "length", length,
        "phase", phases[gc->queue_phase],
        "ready", _Py_atomic_load_int_relaxed(&gc->queue_ready),
        "barrier",
        "deleting", _Py_atomic_load_ptr(&barrier.deleting) != NULL,
        "pin_waited", barrier.pin_waited,
        "timed_out", barrier.timed_out,
        "routed", (long long)barrier.routed,
        "unpinned", (long long)barrier.unpinned,
        "remapped", barrier.remapped,
        "remapped_other", barrier.remapped_other,
        "discarded",
        "callback", gc->queue_discarded[CALL_CALLBACK],
        "finalize", gc->queue_discarded[CALL_FINALIZE],
        "legacy", gc->queue_discarded[CALL_LEGACY],
        "destroy", gc->queue_discarded[CALL_DESTROY]);
}

/* sys.settrace(), sys.setprofile() and sys.monitoring call this before they
   install a hook, so that the callbacks and finalizers of objects that died
   before tracing began run untraced, as they would have run under reference
   counting.  An inline collection leaves them queued for the eval breaker,
   which could otherwise run them inside the first traced step. */
void
_PyGC_LiveRunQueue(PyThreadState *tstate)
{
    (void)run_queue(tstate);
}

/* The eval breaker's call, scheduled by _PyGC_LiveOnThreshold(). */
Py_GC_PRECISE void
_Py_RunGC(PyThreadState *tstate)
{
    GCState *gc = &tstate->interp->gc;
    // Collect if one is due and may run; drain in any case, since a queue
    // can have work without a collection here: one that an allocation ran
    // left it, or another interpreter's collection put it there.
    if (gc->alloc_bytes >= gc->alloc_limit && can_collect(tstate)) {
        (void)collect_from_entry(tstate, _Py_GC_REASON_HEAP, 0);
    }
    (void)run_queue(tstate);
}

Py_GC_PRECISE Py_GC_ARGS_ANCHORED static PyObject *
gc_alloc(PyTypeObject *tp, size_t basicsize)
{
    PyThreadState *tstate = _PyThreadState_GET();
    if (basicsize > PY_SSIZE_T_MAX) {
        return _PyErr_NoMemory(tstate);
    }
    char *mem = _PyObject_MallocWithType(tp, basicsize);
    if (mem == NULL) {
        return _PyErr_NoMemory(tstate);
    }
    return (PyObject *)mem;
}

Py_GC_PRECISE PyObject *
_PyObject_GC_New(PyTypeObject *tp)
{
    PyThreadState *tstate = _PyThreadState_GET();
    _PyValueRoot type_root;
    _PyThreadState_PushValueRoot(tstate, &type_root);
    type_root.object = (PyObject *)tp;
    size_t size = _PyObject_SIZE(tp);
    size_t slots = _PyType_AllocSlotsSize(tp);
    PyObject *op = gc_alloc(tp, size + slots);
    if (op != NULL) {
        // The shape slots start empty (pycore_shape.h).
        memset((char *)op + size, 0, slots);
        _PyObject_Init(op, tp);
    }
    _PyThreadState_PopValueRoot(tstate, &type_root);
    return op;
}

Py_GC_PRECISE PyVarObject *
_PyObject_GC_NewVar(PyTypeObject *tp, Py_ssize_t nitems)
{
    if (nitems < 0) {
        PyErr_BadInternalCall();
        return NULL;
    }
    PyThreadState *tstate = _PyThreadState_GET();
    _PyValueRoot type_root;
    _PyThreadState_PushValueRoot(tstate, &type_root);
    type_root.object = (PyObject *)tp;
    size_t size = _PyObject_VAR_SIZE(tp, nitems);
    PyVarObject *op = (PyVarObject *)gc_alloc(tp, size);
    if (op != NULL) {
        _PyObject_InitVar(op, tp, nitems);
        if (tp->tp_dictoffset != 0
            && !(tp->tp_flags & Py_TPFLAGS_MANAGED_DICT))
        {
            // The dict word of a variable-size type with a managed dict
            // (type_ready_managed_dict), which its C code may not
            // initialise.
            *_PyObject_ComputedDictPointer((PyObject *)op) = NULL;
        }
    }
    _PyThreadState_PopValueRoot(tstate, &type_root);
    return op;
}

Py_GC_PRECISE PyObject *
PyUnstable_Object_GC_NewWithExtraData(PyTypeObject *tp, size_t extra_size)
{
    PyThreadState *tstate = _PyThreadState_GET();
    _PyValueRoot type_root;
    _PyThreadState_PushValueRoot(tstate, &type_root);
    type_root.object = (PyObject *)tp;
    size_t size = _PyObject_SIZE(tp) + extra_size;
    PyObject *op = gc_alloc(tp, size);
    if (op != NULL) {
        memset((char *)op + sizeof(PyObject), 0, size - sizeof(PyObject));
        _PyObject_Init(op, tp);
    }
    _PyThreadState_PopValueRoot(tstate, &type_root);
    return op;
}

PyVarObject *
_PyObject_GC_Resize(PyVarObject *op, Py_ssize_t nitems)
{
    const size_t basicsize = _PyObject_VAR_SIZE(Py_TYPE(op), nitems);
    _PyObject_ASSERT((PyObject *)op, !_PyObject_GC_IS_TRACKED(op));
    if (basicsize > (size_t)PY_SSIZE_T_MAX) {
        return (PyVarObject *)PyErr_NoMemory();
    }
    // A dict word found from the end moves with it.
    int moves = Py_TYPE(op)->tp_dictoffset < 0
                && !(Py_TYPE(op)->tp_flags & Py_TPFLAGS_MANAGED_DICT);
    PyObject *dict = moves ? *_PyObject_ComputedDictPointer((PyObject *)op) : NULL;
    char *mem = (char *)_PyObject_ReallocWithType(Py_TYPE(op), op, basicsize);
    if (mem == NULL) {
        return (PyVarObject *)PyErr_NoMemory();
    }
    op = (PyVarObject *)mem;
    Py_SET_SIZE(op, nitems);
    if (moves) {
        *_PyObject_ComputedDictPointer((PyObject *)op) = dict;
    }
    return op;
}

/* During the sweep PyObject_Free() leaves a dead object's block for the
   sweep to free. */
void
PyObject_GC_Del(void *op)
{
    PyObject_Free(op);
}

/* Every object of a GC type is traced: there is no untracked state that
   hides it from the collector. */
int
PyObject_GC_IsTracked(PyObject* obj)
{
    return _PyObject_IS_GC(obj) && Py_TYPE(obj)->tp_traverse != NULL;
}

int
PyObject_GC_IsFinalized(PyObject *obj)
{
    return _PyObject_IS_GC(obj) && _PyGC_FINALIZED(obj);
}

void
_PyGC_LiveGetRootStats(PyInterpreterState *interp, _PyGCRootStats *total,
                       _PyGCRootStats *last)
{
    struct _gc_live_state *ls = interp->gc.live;
    int counted = ls != NULL && ls->root_pid == current_pid();
    *total = counted ? ls->root_total : (_PyGCRootStats){0};
    *last = counted ? ls->root_last : (_PyGCRootStats){0};
}

/* gc.get_stats(): totals since startup. */
void
_PyGC_LiveGetStats(PyInterpreterState *interp, Py_ssize_t *bytes_freed,
                   Py_ssize_t *live_bytes, double *pause_max)
{
    struct _gc_live_state *ls = interp->gc.live;
    *bytes_freed = ls ? ls->bytes_freed : 0;
    *live_bytes = ls ? ls->live_bytes : 0;
    *pause_max = ls ? PyTime_AsSecondsDouble(ls->pause_max_ns) : 0.0;
}
