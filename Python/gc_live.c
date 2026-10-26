/* The live tracing collector (note 06, step 5a).

   Under Py_GC nothing is reference counted and this collector owns the
   memory of every object.  An object allocation collects when the bytes
   allocated since the last collection pass a threshold (the trigger), with
   the GIL held:

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
   4. The remaining weak references to dead objects are cleared.
   5. The sweep calls tp_dealloc on every object still dead, with the frees
      of their blocks deferred (PyObject_Free() sees GCState.sweeping), then
      frees the blocks.  A dealloc can therefore read the other dead objects'
      memory; it must not allocate objects or run Python code.
   6. After the collection, before the allocation returns, the queue runs:
      the weak reference callbacks, then the finalizers.

   Collections and the queue run in the interpreter that allocates; the mark
   covers every interpreter that shares its object heaps. */

#include "Python.h"
#include "pycore_ceval.h"         // _Py_set_eval_breaker_bit()
#include "pycore_dict.h"          // _PyInlineValuesSize()
#include "pycore_genobject.h"     // _PyGen_FinalizerIsNoop()
#include "pycore_initconfig.h"    // _PyStatus_OK()
#include "pycore_interp.h"        // PyInterpreterState.gc
#include "pycore_interpframe.h"   // _PyFrame_GetLocalsArray()
#include "pycore_object.h"
#include "pycore_gc_trace.h"      // _PyGC_TraceLiveMark()
#include "pycore_object_alloc.h"  // _PyObject_MallocWithType()
#include "pycore_pyerrors.h"
#include "pycore_pystate.h"       // _PyThreadState_GET()
#include "pycore_tstate.h"        // _PyThreadStateImpl
#include "pycore_weakref.h"       // _PyWeakref_ClearRef()

#ifdef Py_GC

#include <stdlib.h>

typedef struct _gc_runtime_state GCState;

#define AS_GC(op) _Py_AS_GC(op)
#define FROM_GC(gc) _Py_FROM_GC(gc)

/* While a collection cannot run, try again after this many more bytes. */
#define RETRY_BYTES (1024 * 1024)

/* A queued call: a weak reference callback, callback(op), or, if callback
   is NULL, op's finalizer or its legacy tp_del. */
typedef struct {
    PyObject *op;
    PyObject *callback;
    int legacy;
} live_call;

/* The collector's own memory comes from the C library's malloc: the heap
   walks must not see it, and it must not trigger a collection. */
struct _gc_live_state {
    // Immortal heap objects that refer to others: roots.
    PyObject **immortal;
    size_t nimmortal;
    size_t immortal_cap;
    // The queue, run FIFO from head.
    live_call *queue;
    size_t qhead;
    size_t qtail;
    size_t qcap;
    int running_queue;
    // The second pass of an explicit collection is not verified again.
    int skip_verify;
    // The exception that was set when the collection started.  The stack
    // below the collection's entry is not scanned (collect_and_run_queue()).
    PyObject *exc;
    // Totals for gc.get_stats()
    Py_ssize_t bytes_freed;
    Py_ssize_t live_bytes;
    Py_ssize_t pause_max_ns;
};

static struct _gc_live_state *
get_live(PyInterpreterState *interp)
{
    if (interp->gc.live == NULL) {
        interp->gc.live = calloc(1, sizeof(struct _gc_live_state));
    }
    return interp->gc.live;
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
    if (!_PyGC_IsObjectHeader(op)) {
        return;  // a static object
    }
    PyTypeObject *tp = Py_TYPE(op);
    if (tp->tp_traverse == NULL && !(tp->tp_flags & Py_TPFLAGS_HEAPTYPE)) {
        return;  // refers to nothing
    }
    PyThreadState *tstate = _PyThreadState_GET();
    PyInterpreterState *interp = tstate != NULL ? tstate->interp
                                                : _PyInterpreterState_Main();
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
    struct _gc_live_state *ls = interp->gc.live;
    if (ls == NULL) {
        return 0;
    }
    for (size_t i = 0; i < ls->nimmortal; i++) {
        PyObject *op = ls->immortal[i];
        Py_VISIT(Py_TYPE(op));
        (void)_PyGC_TraceTraverse(op, visit, arg);
    }
    for (size_t i = ls->qhead; i < ls->qtail; i++) {
        Py_VISIT(ls->queue[i].op);
        Py_VISIT(ls->queue[i].callback);
    }
    Py_VISIT(ls->exc);
    return 0;
}

static int
queue_push(struct _gc_live_state *ls, PyObject *op, PyObject *callback)
{
    if (ls->qtail == ls->qcap) {
        if (ls->qhead > 0 && !ls->running_queue) {
            memmove(ls->queue, ls->queue + ls->qhead,
                    (ls->qtail - ls->qhead) * sizeof(live_call));
            ls->qtail -= ls->qhead;
            ls->qhead = 0;
        }
        if (ls->qtail == ls->qcap) {
            size_t cap = ls->qcap ? ls->qcap * 2 : 256;
            live_call *q = realloc(ls->queue, cap * sizeof(live_call));
            if (q == NULL) {
                return -1;
            }
            ls->queue = q;
            ls->qcap = cap;
        }
    }
    ls->queue[ls->qtail++] = (live_call){op, callback, 0};
    return 0;
}

/* Run the queued callbacks and finalizers.  A collection that one of them
   triggers queues more at the tail, and this loop runs those too. */
static int
run_queue(PyThreadState *tstate)
{
    struct _gc_live_state *ls = tstate->interp->gc.live;
    if (ls == NULL || ls->running_queue || ls->qhead == ls->qtail) {
        return 0;
    }
    int nfinalized = 0;
    ls->running_queue = 1;
    PyObject *exc = _PyErr_GetRaisedException(tstate);
    while (ls->qhead < ls->qtail) {
        // The call's objects stay reachable from this frame.
        live_call call = ls->queue[ls->qhead++];
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
        else if (call.legacy) {
            // As subtype_dealloc() does under reference counting: the weak
            // references go first.
            nfinalized++;
            if (_PyType_SUPPORTS_WEAKREFS(Py_TYPE(call.op))) {
                PyObject_ClearWeakRefs(call.op);
            }
            Py_TYPE(call.op)->tp_del(call.op);
            if (_PyErr_Occurred(tstate)) {
                PyErr_FormatUnraisable("Exception ignored in tp_del of %R",
                                       call.op);
            }
        }
        else {
            nfinalized++;
            PyObject_CallFinalizer(call.op);
            if (_PyErr_Occurred(tstate)) {
                PyErr_FormatUnraisable("Exception ignored while finalizing "
                                       "%R", call.op);
            }
        }
    }
    ls->qhead = ls->qtail = 0;
    ls->running_queue = 0;
    _PyErr_SetRaisedException(tstate, exc);
    return nfinalized;
}


/* --- resurrection --------------------------------------------------------- */

typedef struct {
    PyObject **items;
    size_t n;
    size_t cap;
    int oom;
} objstack;

static void
objstack_push(objstack *s, PyObject *op)
{
    if (s->n == s->cap) {
        size_t cap = s->cap ? s->cap * 2 : 1024;
        PyObject **items = realloc(s->items, cap * sizeof(PyObject *));
        if (items == NULL) {
            s->oom = 1;
            return;
        }
        s->items = items;
        s->cap = cap;
    }
    s->items[s->n++] = op;
}

static inline int
is_dead(PyObject *op)
{
    return _PyGC_IsObjectHeader(op) && (op->ob_flags & _Py_GC_DEAD_FLAG);
}

static int
resurrect_visit(PyObject *op, void *arg)
{
    if (op != NULL && is_dead(op)) {
        op->ob_flags &= ~_Py_GC_DEAD_FLAG;
        objstack_push((objstack *)arg, op);
    }
    return 0;
}

/* Keep op, which is dead, and every dead object it reaches. */
static void
resurrect(objstack *s, PyObject *op)
{
    (void)resurrect_visit(op, s);
    while (s->n > 0) {
        PyObject *o = s->items[--s->n];
        (void)resurrect_visit((PyObject *)Py_TYPE(o), s);
        (void)_PyGC_TraceTraverse(o, resurrect_visit, s);
    }
}


/* --- the collection ------------------------------------------------------- */

static void
invoke_gc_callback(PyThreadState *tstate, const char *phase,
                   struct gc_generation_stats *stats)
{
    GCState *gcstate = &tstate->interp->gc;
    if (gcstate->callbacks == NULL || PyList_GET_SIZE(gcstate->callbacks) == 0) {
        return;
    }
    assert(PyList_CheckExact(gcstate->callbacks));
    PyObject *info = Py_BuildValue("{sisnsnsnsd}",
        "generation", NUM_GENERATIONS - 1,
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
clear_callback_weakrefs(struct _gc_live_state *ls, _PyGCMarkResult *res)
{
    for (Py_ssize_t i = 0; i < res->ndead; i++) {
        PyObject *op = res->dead[i].op;
        if (!_PyType_SUPPORTS_WEAKREFS(Py_TYPE(op))) {
            continue;
        }
        PyWeakReference **list = _PyObject_GET_WEAKREFS_LISTPTR_FROM_OFFSET(op);
        PyWeakReference *next;
        for (PyWeakReference *wr = *list; wr != NULL; wr = next) {
            next = wr->wr_next;
            PyObject *callback = wr->wr_callback;
            if (callback == NULL) {
                continue;
            }
            _PyWeakref_ClearRef(wr);
            if (((PyObject *)wr)->ob_flags & _Py_GC_DEAD_FLAG) {
                continue;  // the weak reference is garbage too
            }
            if (queue_push(ls, (PyObject *)wr, callback) < 0) {
                return -1;
            }
        }
    }
    return 0;
}

static int
cycle_visit(PyObject *op, void *arg)
{
    objstack *s = arg;
    if (op != NULL && is_dead(op) && !(op->ob_flags & _Py_GC_MARKED_FLAG)) {
        op->ob_flags |= _Py_GC_MARKED_FLAG;
        objstack_push(s, op);
    }
    return 0;
}

/* Does op reach itself through dead objects?  Only for the rare objects with
   a legacy tp_del.  The mark flag, clear on every dead object, records the
   objects visited; it is cleared again before returning. */
static int
in_dead_cycle(PyObject *op, objstack *seen)
{
    seen->n = 0;
    (void)_PyGC_TraceTraverse(op, cycle_visit, seen);
    int found = 0;
    for (size_t i = 0; i < seen->n && !found && !seen->oom; i++) {
        PyObject *o = seen->items[i];
        if (o == op) {
            found = 1;
            break;
        }
        (void)_PyGC_TraceTraverse(o, cycle_visit, seen);
    }
    for (size_t i = 0; i < seen->n; i++) {
        seen->items[i]->ob_flags &= ~_Py_GC_MARKED_FLAG;
    }
    return found || seen->oom;
}

static int
order_visit(PyObject *op, void *arg)
{
    if (op != NULL && is_dead(op) && !(op->ob_flags & _Py_GC_MARKED_FLAG)) {
        objstack_push((objstack *)arg, op);
    }
    return 0;
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
    objstack stack = {0};
    objstack visited = {0};
    for (Py_ssize_t i = 0; i < res->ndead; i++) {
        PyObject *root = res->dead[i].op;
        if (Py_TYPE(root)->tp_finalize == NULL
            || (root->ob_flags & _Py_GC_MARKED_FLAG))
        {
            continue;
        }
        objstack_push(&stack, root);
        while (stack.n > 0 && !stack.oom) {
            // An entry with bit 0 set is an object whose children are done.
            uintptr_t top = (uintptr_t)stack.items[--stack.n];
            PyObject *op = (PyObject *)(top & ~(uintptr_t)1);
            if (top & 1) {
                if (Py_TYPE(op)->tp_finalize != NULL) {
                    objstack_push(order, op);
                }
                continue;
            }
            if (op->ob_flags & _Py_GC_MARKED_FLAG) {
                continue;
            }
            op->ob_flags |= _Py_GC_MARKED_FLAG;
            objstack_push(&visited, op);
            objstack_push(&stack, (PyObject *)((uintptr_t)op | 1));
            (void)_PyGC_TraceTraverse(op, order_visit, &stack);
        }
    }
    for (size_t i = 0; i < visited.n; i++) {
        visited.items[i]->ob_flags &= ~_Py_GC_MARKED_FLAG;
    }
    int oom = stack.oom || visited.oom || order->oom;
    free(stack.items);
    free(visited.items);
    // Reverse the postorder
    for (size_t i = 0, j = order->n; !oom && i + 1 < j; i++, j--) {
        PyObject *tmp = order->items[i];
        order->items[i] = order->items[j - 1];
        order->items[j - 1] = tmp;
    }
    return oom ? -1 : 0;
}

static int
queue_legacy(struct _gc_live_state *ls, PyObject *op)
{
    if (queue_push(ls, op, NULL) < 0) {
        return -1;
    }
    ls->queue[ls->qtail - 1].legacy = 1;
    op->ob_flags |= _Py_GC_DEL_CALLED_FLAG;
    return 0;
}

/* Queue the finalizers and legacy tp_dels of dead objects and resurrect what
   they reach, as gc.c does: the GC objects with a tp_del in a cycle of
   garbage, and what they reach, are not finalized but listed in *garbage,
   for gc.garbage, as is everything with DEBUG_SAVEALL.  A tp_del outside a
   cycle runs, as it does when its count drops to zero. */
static int
handle_finalizers(GCState *gc, struct _gc_live_state *ls,
                  _PyGCMarkResult *res, objstack *garbage)
{
    int saveall = gc->debug & _PyGC_DEBUG_SAVEALL;
    objstack s = {0};
    objstack legacy = {0};
    for (Py_ssize_t i = 0; i < res->ndead; i++) {
        PyObject *op = res->dead[i].op;
        if (Py_TYPE(op)->tp_del == NULL
            || (op->ob_flags & _Py_GC_DEL_CALLED_FLAG))
        {
            continue;
        }
        if (_PyObject_IS_GC(op) && in_dead_cycle(op, &s)) {
            objstack_push(garbage, op);
        }
        else {
            objstack_push(&legacy, op);
        }
    }
    if (garbage->oom || legacy.oom) {
        goto error;
    }
    s.n = 0;
    for (size_t i = 0; i < garbage->n; i++) {
        resurrect(&s, garbage->items[i]);
    }
    // A tp_del that a cycle in gc.garbage reaches does not run either: its
    // object goes to gc.garbage too.
    for (size_t i = 0; i < legacy.n; i++) {
        PyObject *op = legacy.items[i];
        if (!(op->ob_flags & _Py_GC_DEAD_FLAG) && _PyObject_IS_GC(op)) {
            objstack_push(garbage, op);
        }
    }
    if (garbage->oom) {
        goto error;
    }
    size_t first = ls->qtail;
    // Queue every finalizer first: a resurrected object needs its own.
    objstack order = {0};
    if (finalizer_order(res, &order) < 0) {
        free(order.items);
        goto error;
    }
    for (size_t i = 0; i < order.n; i++) {
        PyObject *op = order.items[i];
        if ((op->ob_flags & _Py_GC_DEAD_FLAG)  // not reached from a legacy cycle
            && !(op->ob_flags & _Py_GC_FINALIZED_FLAG)
            && !_PyGen_FinalizerIsNoop(op))
        {
            if (queue_push(ls, op, NULL) < 0) {
                free(order.items);
                goto error;
            }
        }
    }
    free(order.items);
    for (Py_ssize_t i = 0; i < res->ndead; i++) {
        PyObject *op = res->dead[i].op;
        if (!(op->ob_flags & _Py_GC_DEAD_FLAG)) {
            continue;  // reached from a legacy cycle
        }
        if (saveall && _PyObject_IS_GC(op)) {
            objstack_push(garbage, op);
            if (garbage->oom) {
                goto error;
            }
        }
    }
    // tp_del after tp_finalize, as subtype_dealloc() calls them
    for (size_t i = 0; i < legacy.n; i++) {
        if ((legacy.items[i]->ob_flags & _Py_GC_DEAD_FLAG)
            && queue_legacy(ls, legacy.items[i]) < 0)
        {
            goto error;
        }
    }
    for (size_t i = first; i < ls->qtail; i++) {
        resurrect(&s, ls->queue[i].op);
    }
    for (size_t i = 0; i < garbage->n; i++) {
        resurrect(&s, garbage->items[i]);
    }
    if (saveall) {
        for (Py_ssize_t i = 0; i < res->ndead; i++) {
            res->dead[i].op->ob_flags &= ~_Py_GC_DEAD_FLAG;
        }
    }
    free(s.items);
    free(legacy.items);
    return s.oom ? -1 : 0;

error:
    free(s.items);
    free(legacy.items);
    return -1;
}

static void
undo_dead(_PyGCMarkResult *res)
{
    for (Py_ssize_t i = 0; i < res->ndead; i++) {
        res->dead[i].op->ob_flags &= ~_Py_GC_DEAD_FLAG;
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

/* Call tp_dealloc on the dead objects, then free their blocks.  Instances
   go before types, so that a dealloc can read its type. */
static Py_ssize_t
sweep(PyThreadState *tstate, GCState *gc, _PyGCMarkResult *res,
      Py_ssize_t *bytes_freed)
{
    Py_ssize_t ncollected = 0;
    PyObject *exc = _PyErr_GetRaisedException(tstate);
    gc->sweeping = 1;
    for (int types = 0; types < 2; types++) {
        for (Py_ssize_t i = 0; i < res->ndead; i++) {
            PyObject *op = res->dead[i].op;
            if (!(op->ob_flags & _Py_GC_DEAD_FLAG)
                || (PyType_Check(op) != 0) != types)
            {
                continue;
            }
            if (_PyObject_IS_GC(op)) {
                ncollected++;
            }
#ifdef Py_DEBUG
            gc->sweep_op = op;
#endif
            Py_TYPE(op)->tp_dealloc(op);
            if (_PyErr_Occurred(tstate)) {
                _PyErr_Clear(tstate);
            }
        }
    }
    gc->sweeping = 0;
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
        PyObject_Free((char *)d->op - d->presize);
    }
    *bytes_freed = freed;
    return ncollected;
}

static void
debug_cycle(const char *msg, PyObject *op)
{
    PySys_FormatStderr("gc: %s <%s %p>\n", msg, Py_TYPE(op)->tp_name, op);
}

static Py_ssize_t
collect(PyThreadState *tstate, _PyGC_Reason reason)
{
    PyInterpreterState *interp = tstate->interp;
    GCState *gc = &interp->gc;
    if (gc->collecting || gc->sweeping) {
        return 0;
    }
    struct _gc_live_state *ls = get_live(interp);
    if (ls == NULL || gc->generation_stats == NULL) {
        return 0;  // before _PyGC_Init() or after _PyGC_Fini()
    }
    gc->collecting = 1;
    ls->exc = _PyErr_GetRaisedException(tstate);
    struct gc_generation_stats *stats =
        &gc->generation_stats->old[1].items[gc->generation_stats->old[1].index];
    struct gc_generation_stats cur = {0};
    invoke_gc_callback(tstate, "start", &cur);

    PyTime_t t0 = 0, t1 = 0;
    (void)PyTime_PerfCounterRaw(&t0);
    _PyGCMarkResult res;
    objstack garbage = {0};
    Py_ssize_t ncollected = 0, bytes_freed = 0;
    if (_PyGC_TraceLiveMark(tstate, &res) < 0) {
        goto done;  // out of memory: nothing is freed
    }
    if (clear_callback_weakrefs(ls, &res) < 0
        || handle_finalizers(gc, ls, &res, &garbage) < 0)
    {
        // Out of memory: keep everything.  What was queued runs.
        undo_dead(&res);
        garbage.n = 0;
    }
    else {
        for (Py_ssize_t i = 0; i < res.ndead; i++) {
            PyObject *op = res.dead[i].op;
            if ((op->ob_flags & _Py_GC_DEAD_FLAG)
                && _PyType_SUPPORTS_WEAKREFS(Py_TYPE(op)))
            {
                _PyWeakref_ClearWeakRefsNoCallbacks(op);
            }
        }
        if (gc->debug & _PyGC_DEBUG_COLLECTABLE) {
            // Only the names and addresses: Python code that the write runs
            // cannot reach the dead objects, whose weak references are clear.
            for (Py_ssize_t i = 0; i < res.ndead; i++) {
                PyObject *op = res.dead[i].op;
                if ((op->ob_flags & _Py_GC_DEAD_FLAG) && _PyObject_IS_GC(op)) {
                    debug_cycle("collectable", op);
                }
            }
        }
        ncollected = sweep(tstate, gc, &res, &bytes_freed);
    }
    (void)PyTime_PerfCounterRaw(&t1);

    ls->bytes_freed += bytes_freed;
    ls->live_bytes = res.live_bytes;
    ls->pause_max_ns = Py_MAX(ls->pause_max_ns, (Py_ssize_t)(t1 - t0));
    cur.collections = 1;
    cur.collected = ncollected;
    cur.uncollectable = (Py_ssize_t)garbage.n;
    cur.candidates = res.nobjects;
    cur.duration = PyTime_AsSecondsDouble(t1 - t0);
    stats->collections++;
    stats->collected += cur.collected;
    stats->uncollectable += cur.uncollectable;
    stats->candidates += cur.candidates;
    stats->duration += cur.duration;
    gc->heap_size = res.nobjects - res.ndead;
    gc->long_lived_total = gc->heap_size;

    gc->alloc_bytes = 0;
    gc->alloc_limit = gc->alloc_threshold > 0
        ? Py_MAX(gc->alloc_threshold, res.live_bytes) : PY_SSIZE_T_MAX;
    free(res.dead);

    if (gc->debug & _PyGC_DEBUG_STATS) {
        PySys_WriteStderr("gc: %zd objects, %zd collected, %zd bytes freed, "
                          "%zd live bytes, %.4fs elapsed\n",
                          res.nobjects, ncollected, bytes_freed,
                          res.live_bytes, cur.duration);
    }
    if (!ls->skip_verify) {
        _PyGC_TraceAfterCollect(tstate, NUM_GENERATIONS - 1,
                                reason == _Py_GC_REASON_MANUAL);
    }
    for (size_t i = 0; i < garbage.n; i++) {
        if (gc->debug & _PyGC_DEBUG_UNCOLLECTABLE) {
            debug_cycle("uncollectable", garbage.items[i]);
        }
    }
    for (size_t i = 0; gc->garbage != NULL && i < garbage.n; i++) {
        if (PyList_Append(gc->garbage, garbage.items[i]) < 0) {
            PyErr_Clear();
            break;
        }
    }
done:
    free(garbage.items);
    invoke_gc_callback(tstate, "stop", &cur);
    gc->collecting = 0;
    _PyErr_SetRaisedException(tstate, ls->exc);
    ls->exc = NULL;
    return ncollected + cur.uncollectable;
}

/* One pass: collect, then run the queue.  The object allocations of the
   collection and of the queue go to the heap for non-GC objects, whatever
   the allocation that triggered it had selected
   (_PyObject_MallocWithType()).  Returns the number of finalizers and
   tp_dels that ran in *nfinalized. */
static Py_NO_INLINE Py_ssize_t
collect_pass(PyThreadState *tstate, _PyGC_Reason reason, int *nfinalized)
{
    _PyThreadStateImpl *ts = (_PyThreadStateImpl *)tstate;
    struct _mimalloc_thread_state *m = &ts->mimalloc;
    mi_heap_t *heap = m->current_object_heap;
    if (heap != NULL) {
        m->current_object_heap = &m->heaps[_Py_MIMALLOC_HEAP_OBJECT];
    }
    Py_ssize_t n = collect(tstate, reason);
    // A collection that the queue's code triggers records its own entry.
    ts->gc_trace.entry_sp = 0;
    *nfinalized = run_queue(tstate);
    if (heap != NULL) {
        m->current_object_heap = heap;
    }
    return n;
}

/* The entry of every collection.  The mark scans this thread's stack from
   here up, with the registers as they are here: the frames below are the
   collector's own, over stale words that earlier, deeper calls left, which
   would keep garbage alive.  Not inlined, so that the callers' frames are
   above, and kept small, since its own frame is scanned too: the work is in
   collect_pass(). */
static Py_NO_INLINE Py_ssize_t
collect_from_entry(PyThreadState *tstate, _PyGC_Reason reason)
{
    _PyThreadStateImpl *ts = (_PyThreadStateImpl *)tstate;
    // Called again from a gc callback, inside a collection: the outer entry
    // stays, since the mark that may follow is the outer collection's.
    uintptr_t outer_entry = ts->gc_trace.entry_sp;
    if (outer_entry == 0) {
        _PyGC_TraceCaptureEntry();
    }
    int nfinalized;
    Py_ssize_t n = collect_pass(tstate, reason, &nfinalized);
    /* The objects that finalizers did not resurrect are garbage again.  An
       explicit collection frees them, as the reference counting build
       does within one collection; an automatic one leaves them to the
       next.  Finalizers do not run twice, so one more pass is enough for
       the objects the first one found. */
    if (nfinalized > 0 && reason != _Py_GC_REASON_HEAP && outer_entry == 0) {
        struct _gc_live_state *ls = get_live(tstate->interp);
        _PyGC_TraceCaptureEntry();
        ls->skip_verify = 1;
        n += collect_pass(tstate, reason, &nfinalized);
        ls->skip_verify = 0;
    }
    ts->gc_trace.entry_sp = outer_entry;
    return n;
}

#define CLEAR_STACK_BYTES (16 * 1024)

/* Zero the stack below the caller's frame.  No store reaches the padding
   and some spill slots of the frames made there next; without this they
   would hold stale words, which the scan takes for roots. */
static Py_NO_INLINE void
clear_stack_below(void)
{
    char buf[CLEAR_STACK_BYTES];
    memset(buf, 0, sizeof(buf));
    // Keep the stores, which are dead to the compiler.
    __asm__ volatile("" : : "r"(buf) : "memory");
}

/* Inlined, so that the frame of collect_from_entry() is made in the cleared
   stack. */
static inline Py_ssize_t
collect_and_run_queue(PyThreadState *tstate, _PyGC_Reason reason)
{
    clear_stack_below();
    Py_ssize_t n = collect_from_entry(tstate, reason);
    // The collector's frames held what it marked, some of it soon garbage.
    clear_stack_below();
    return n;
}

void
_PyGC_LiveOnThreshold(PyThreadState *tstate)
{
    PyInterpreterState *interp = tstate->interp;
    GCState *gc = &interp->gc;
    int ok = gc->enabled && gc->alloc_threshold > 0
        && !gc->collecting && !gc->sweeping
        && tstate->state == _Py_THREAD_ATTACHED
        && ((_PyThreadStateImpl *)tstate)->mimalloc.current_object_heap != NULL
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
        return;
    }
    (void)collect_and_run_queue(tstate, _Py_GC_REASON_HEAP);
}


/* --- the interface of Python/gc.c --------------------------------------- */

void
_PyGC_InitState(GCState *gcstate)
{
#define INIT_HEAD(GEN) \
    do { \
        GEN.head._gc_next = (uintptr_t)&GEN.head; \
        GEN.head._gc_prev = (uintptr_t)&GEN.head; \
    } while (0)

    for (int i = 0; i < NUM_GENERATIONS; i++) {
        INIT_HEAD(gcstate->generations[i]);
    };
    gcstate->generation0 = &gcstate->generations[0].head;
    INIT_HEAD(gcstate->permanent_generation);
#undef INIT_HEAD
}

PyStatus
_PyGC_Init(PyInterpreterState *interp)
{
    GCState *gcstate = &interp->gc;
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
    return _PyStatus_OK();
}

void
_PyGC_Fini(PyInterpreterState *interp)
{
    GCState *gcstate = &interp->gc;
    gcstate->alloc_limit = PY_SSIZE_T_MAX;
    gcstate->garbage = NULL;
    gcstate->callbacks = NULL;
    PyMem_RawFree(gcstate->generation_stats);
    gcstate->generation_stats = NULL;
    _PyGC_TraceFini(interp);
    struct _gc_live_state *ls = gcstate->live;
    if (ls != NULL) {
        gcstate->live = NULL;
        free(ls->immortal);
        free(ls->queue);
        free(ls);
    }
}

int
_PyGC_VisitStackRef(_PyStackRef *ref, visitproc visit, void *arg)
{
    assert(PyStackRef_IsObject(*ref));
    Py_VISIT(PyStackRef_AsPyObjectBorrow(*ref));
    return 0;
}

/* Every slot holds a value or NULL (pycore_interpframe_structs.h). */
int
_PyGC_VisitFrameStack(_PyInterpreterFrame *frame, visitproc visit, void *arg)
{
    _PyStackRef *ref = _PyFrame_GetLocalsArray(frame);
    _PyStackRef *end = ref + _PyFrame_NumSlots(frame);
    for (; ref < end; ref++) {
        _Py_VISIT_STACKREF(*ref);
    }
    return 0;
}

/* Every tracked GC object, after a collection.  The caller visits them with
   automatic collection blocked, and frees the array. */
static PyObject **
snapshot(PyThreadState *tstate, Py_ssize_t *n)
{
    (void)collect_and_run_queue(tstate, _Py_GC_REASON_MANUAL);
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

/* Generations do not exist: generation 2 and None mean every object, and
   the younger generations are empty. */
PyObject *
_PyGC_GetObjects(PyInterpreterState *interp, int generation)
{
    if (generation >= 0 && generation < NUM_GENERATIONS - 1) {
        return PyList_New(0);
    }
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
    return collect_and_run_queue(tstate, _Py_GC_REASON_MANUAL);
}

Py_ssize_t
_PyGC_Collect(PyThreadState *tstate, int generation, _PyGC_Reason reason)
{
    return collect_and_run_queue(tstate, reason);
}

void
_PyGC_CollectNoFail(PyThreadState *tstate)
{
    (void)collect_and_run_queue(tstate, _Py_GC_REASON_SHUTDOWN);
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

void
_PyGC_Dump(PyGC_Head *g)
{
    PyObject_Dump(FROM_GC(g));
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

void
_PyObject_GC_Link(PyObject *op)
{
    PyGC_Head *gc = AS_GC(op);
    _PyObject_ASSERT(op, ((uintptr_t)gc & (sizeof(uintptr_t)-1)) == 0);
    gc->_gc_next = 0;
    gc->_gc_prev = 0;
}

void
_Py_RunGC(PyThreadState *tstate)
{
    if (!tstate->interp->gc.enabled) {
        return;
    }
    (void)collect_and_run_queue(tstate, _Py_GC_REASON_HEAP);
}

static PyObject *
gc_alloc(PyTypeObject *tp, size_t basicsize, size_t presize)
{
    PyThreadState *tstate = _PyThreadState_GET();
    if (basicsize > PY_SSIZE_T_MAX - presize) {
        return _PyErr_NoMemory(tstate);
    }
    size_t size = presize + basicsize;
    char *mem = _PyObject_MallocWithType(tp, size);
    if (mem == NULL) {
        return _PyErr_NoMemory(tstate);
    }
    ((PyObject **)mem)[0] = NULL;
    ((PyObject **)mem)[1] = NULL;
    PyObject *op = (PyObject *)(mem + presize);
    _PyObject_GC_Link(op);
    return op;
}

PyObject *
_PyObject_GC_New(PyTypeObject *tp)
{
    size_t presize = _PyType_PreHeaderSize(tp);
    size_t size = _PyObject_SIZE(tp);
    if (_PyType_HasFeature(tp, Py_TPFLAGS_INLINE_VALUES)) {
        size += _PyInlineValuesSize(tp);
    }
    PyObject *op = gc_alloc(tp, size, presize);
    if (op == NULL) {
        return NULL;
    }
    _PyObject_Init(op, tp);
    if (tp->tp_flags & Py_TPFLAGS_INLINE_VALUES) {
        _PyObject_InitInlineValues(op, tp);
    }
    return op;
}

PyVarObject *
_PyObject_GC_NewVar(PyTypeObject *tp, Py_ssize_t nitems)
{
    if (nitems < 0) {
        PyErr_BadInternalCall();
        return NULL;
    }
    size_t presize = _PyType_PreHeaderSize(tp);
    size_t size = _PyObject_VAR_SIZE(tp, nitems);
    PyVarObject *op = (PyVarObject *)gc_alloc(tp, size, presize);
    if (op == NULL) {
        return NULL;
    }
    _PyObject_InitVar(op, tp, nitems);
    return op;
}

PyObject *
PyUnstable_Object_GC_NewWithExtraData(PyTypeObject *tp, size_t extra_size)
{
    size_t presize = _PyType_PreHeaderSize(tp);
    size_t size = _PyObject_SIZE(tp) + extra_size;
    PyObject *op = gc_alloc(tp, size, presize);
    if (op == NULL) {
        return NULL;
    }
    memset((char *)op + sizeof(PyObject), 0, size - sizeof(PyObject));
    _PyObject_Init(op, tp);
    return op;
}

PyVarObject *
_PyObject_GC_Resize(PyVarObject *op, Py_ssize_t nitems)
{
    const size_t basicsize = _PyObject_VAR_SIZE(Py_TYPE(op), nitems);
    const size_t presize = _PyType_PreHeaderSize(Py_TYPE(op));
    _PyObject_ASSERT((PyObject *)op, !_PyObject_GC_IS_TRACKED(op));
    if (basicsize > (size_t)PY_SSIZE_T_MAX - presize) {
        return (PyVarObject *)PyErr_NoMemory();
    }
    char *mem = (char *)op - presize;
    mem = (char *)_PyObject_ReallocWithType(Py_TYPE(op), mem, presize + basicsize);
    if (mem == NULL) {
        return (PyVarObject *)PyErr_NoMemory();
    }
    op = (PyVarObject *) (mem + presize);
    Py_SET_SIZE(op, nitems);
    return op;
}

/* During the sweep PyObject_Free() leaves a dead object's block for the
   sweep to free. */
void
PyObject_GC_Del(void *op)
{
    size_t presize = _PyType_PreHeaderSize(Py_TYPE(op));
    PyObject_Free(((char *)op)-presize);
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

#endif  /* Py_GC */
