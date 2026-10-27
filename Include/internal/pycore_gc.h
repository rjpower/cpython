#ifndef Py_INTERNAL_GC_H
#define Py_INTERNAL_GC_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_interp_structs.h" // struct _gc_runtime_state
#include "pycore_pystate.h"       // _PyInterpreterState_GET()
#include "pycore_typedefs.h"      // _PyInterpreterFrame


/* Bit flags for ob_gc_bits (in Py_GIL_DISABLED builds)
 *
 * Setting the bits requires a relaxed store. The per-object lock must also be
 * held, except when the object is only visible to a single thread (e.g. during
 * object initialization or destruction).
 *
 * Reading the bits requires using a relaxed load, but does not require holding
 * the per-object lock.
 */


/* True if the object is currently tracked by the GC. */
static inline int _PyObject_GC_IS_TRACKED(PyObject *op) {
    /* There are no generation lists.  The bit survives so that a
       conservative root can tell an object still being built (untracked)
       from one whose fields are all set (Python/gc_live.c). */
    return (op->ob_flags & _Py_GC_TRACKED_FLAG) != 0;
}
#define _PyObject_GC_IS_TRACKED(op) _PyObject_GC_IS_TRACKED(_Py_CAST(PyObject*, op))

/* True if the object may be tracked by the GC in the future, or already is.
   This can be useful to implement some optimizations. */
static inline int _PyObject_GC_MAY_BE_TRACKED(PyObject *obj) {
    if (!PyObject_IS_GC(obj)) {
        return 0;
    }
    if (PyTuple_CheckExact(obj)) {
        return _PyObject_GC_IS_TRACKED(obj);
    }
    return 1;
}


/* set for debugging information */
#define _PyGC_DEBUG_STATS             (1<<0) /* print collection statistics */
#define _PyGC_DEBUG_COLLECTABLE       (1<<1) /* print collectable objects */
#define _PyGC_DEBUG_UNCOLLECTABLE     (1<<2) /* print uncollectable objects */
#define _PyGC_DEBUG_SAVEALL           (1<<5) /* save all garbage in gc.garbage */
#define _PyGC_DEBUG_LEAK              _PyGC_DEBUG_COLLECTABLE | \
                                      _PyGC_DEBUG_UNCOLLECTABLE | \
                                      _PyGC_DEBUG_SAVEALL

typedef enum {
    // GC was triggered by heap allocation
    _Py_GC_REASON_HEAP,

    // GC was called during shutdown
    _Py_GC_REASON_SHUTDOWN,

    // GC was called by gc.collect() or PyGC_Collect()
    _Py_GC_REASON_MANUAL
} _PyGC_Reason;

static inline int _PyGC_FINALIZED(PyObject *op) {
    return (op->ob_flags & _Py_GC_FINALIZED_FLAG) != 0;
}
static inline void _PyGC_SET_FINALIZED(PyObject *op) {
    op->ob_flags |= _Py_GC_FINALIZED_FLAG;
}
static inline void _PyGC_CLEAR_FINALIZED(PyObject *op) {
    op->ob_flags &= ~_Py_GC_FINALIZED_FLAG;
}

extern void _Py_ScheduleGC(PyThreadState *tstate);


/* Tell the GC to track this object.
 *
 * The object must not be tracked by the GC.
 *
 * NB: While the object is tracked by the collector, it must be safe to call the
 * ob_traverse method.
 *
 * See also the public PyObject_GC_Track() function.
 */
static inline void _PyObject_GC_TRACK(
// The preprocessor removes _PyObject_ASSERT_FROM() calls if NDEBUG is defined
#ifndef NDEBUG
    const char *filename, int lineno,
#endif
    PyObject *op)
{
    _PyObject_ASSERT_FROM(op, !_PyObject_GC_IS_TRACKED(op),
                          "object already tracked by the garbage collector",
                          filename, lineno, __func__);
    op->ob_flags |= _Py_GC_TRACKED_FLAG;
}

/* Tell the GC to stop tracking this object.
 *
 * The object must be tracked by the GC.
 *
 * See also the public PyObject_GC_UnTrack() which accept an object which is
 * not tracked.
 */
static inline void _PyObject_GC_UNTRACK(
// The preprocessor removes _PyObject_ASSERT_FROM() calls if NDEBUG is defined
#ifndef NDEBUG
    const char *filename, int lineno,
#endif
    PyObject *op)
{
    _PyObject_ASSERT_FROM(op, _PyObject_GC_IS_TRACKED(op),
                          "object not tracked by the garbage collector",
                          filename, lineno, __func__);

    op->ob_flags &= ~_Py_GC_TRACKED_FLAG;
}



/*
   NOTE: about untracking of mutable objects.

   Certain types of container cannot participate in a reference cycle, and
   so do not need to be tracked by the garbage collector. Untracking these
   objects reduces the cost of garbage collections. However, determining
   which objects may be untracked is not free, and the costs must be
   weighed against the benefits for garbage collection.

   There are two possible strategies for when to untrack a container:

   i) When the container is created.
   ii) When the container is examined by the garbage collector.

   Tuples containing only immutable objects (integers, strings etc, and
   recursively, tuples of immutable objects) do not need to be tracked.
   The interpreter creates a large number of tuples, many of which will
   not survive until garbage collection. It is therefore not worthwhile
   to untrack eligible tuples at creation time.

   Instead, all tuples except the empty tuple are tracked when created.
   During garbage collection it is determined whether any surviving tuples
   can be untracked. A tuple can be untracked if all of its contents are
   already not tracked. Tuples are examined for untracking in all garbage
   collection cycles. It may take more than one cycle to untrack a tuple.

   Dictionaries containing only immutable objects also do not need to be
   tracked. Dictionaries are untracked when created. If a tracked item is
   inserted into a dictionary (either as a key or value), the dictionary
   becomes tracked. During a full garbage collection (all generations),
   the collector will untrack any dictionaries whose contents are not
   tracked.

   The module provides the python function is_tracked(obj), which returns
   the CURRENT tracking status of the object. Subsequent garbage
   collections may change the tracking status of the object.

   Untracking of certain containers was introduced in issue #4688, and
   the algorithm was refined in response to issue #14775.
*/


extern Py_ssize_t _PyGC_Collect(PyThreadState *tstate, int generation, _PyGC_Reason reason);
extern void _PyGC_CollectNoFail(PyThreadState *tstate);

/* Freeze objects tracked by the GC and ignore them in future collections. */
extern void _PyGC_Freeze(PyInterpreterState *interp);
/* Unfreezes objects placing them in the oldest generation */
extern void _PyGC_Unfreeze(PyInterpreterState *interp);
/* Number of frozen objects */
extern Py_ssize_t _PyGC_GetFreezeCount(PyInterpreterState *interp);

extern PyObject *_PyGC_GetObjects(PyInterpreterState *interp, int generation);
extern PyObject *_PyGC_GetReferrers(PyInterpreterState *interp, PyObject *objs);

extern void _Py_RunGC(PyThreadState *tstate);

struct PyValue;

// GC visit callback for tracked interpreter frames
// GH-150766: exported for greenlet
PyAPI_FUNC(int) _PyGC_VisitFrameStack(_PyInterpreterFrame *frame, visitproc visit, void *arg);

/* Visit a reference that cannot be part of a cycle, such as a string, and
   that only the tracing collector (Python/gc_trace.c) needs.  Immortal
   objects are skipped: they are roots of the tracing mark, and visiting
   them would only slow the cycle collector. */
#define _Py_VISIT_ATOMIC(op)                                        \
    do {                                                            \
        PyObject *_atomic_op = _PyObject_CAST(op);                  \
        if (_atomic_op != NULL && !_Py_IsImmortal(_atomic_op)) {    \
            int _atomic_vret = visit(_atomic_op, arg);              \
            if (_atomic_vret) {                                     \
                return _atomic_vret;                                \
            }                                                       \
        }                                                           \
    } while (0)
PyAPI_FUNC(int) _PyGC_VisitValue(struct PyValue *ref, visitproc visit, void *arg);


#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_GC_H */
