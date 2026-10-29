#ifndef Py_INTERNAL_TSTATE_H
#define Py_INTERNAL_TSTATE_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_interpframe_structs.h"  // _PyInterpreterFrame
#include "pycore_mimalloc.h"        // struct _mimalloc_thread_state
#include "pycore_qsbr.h"            // struct qsbr
#include "pycore_gc_walk.h"         // _PyGCDetachRecord
#include "pycore_structs.h"


// Every PyThreadState is actually allocated as a _PyThreadStateImpl. The
// PyThreadState fields are exposed as part of the C API, although most fields
// are intended to be private. The _PyThreadStateImpl fields not exposed.
typedef struct _PyThreadStateImpl {
    // semi-public fields are in PyThreadState.
    PyThreadState base;

    // Embedded base frame - sentinel at the bottom of the frame stack.
    // Used by profiling/sampling to detect incomplete stack traces.
    _PyInterpreterFrame base_frame;

    // The reference count field is used to synchronize deallocation of the
    // thread state during runtime finalization.
    Py_ssize_t refcount;

    // These are addresses, but we need to convert to ints to avoid UB.
    uintptr_t c_stack_top;
    uintptr_t c_stack_soft_limit;
    uintptr_t c_stack_hard_limit;

    // PyUnstable_ThreadState_ResetStackProtection() values
    uintptr_t c_stack_init_base;
    uintptr_t c_stack_init_top;

    PyObject *asyncio_running_loop; // Strong reference
    PyObject *asyncio_running_task; // Strong reference

    // Reused for the thread's lifetime; owns placeholders while they resolve.
    PyObject *lazy_imports;

    // Distinguishes between yield and return from PyEval_EvalFrame().
    // See gen_send_ex2() in Objects/genobject.c
    enum {
        GENERATOR_RETURN = 0,
        GENERATOR_YIELD = 1,
    } generator_return_kind;

    /* Head of circular linked-list of all tasks which are instances of `asyncio.Task`
       or subclasses of it used in `asyncio.all_tasks`.
    */
    struct llist_node asyncio_tasks_head;
    struct _qsbr_thread_state *qsbr;  // only used by free-threaded build
    struct llist_node mem_free_queue; // delayed free queue

#ifdef WITH_MIMALLOC
    struct _mimalloc_thread_state mimalloc;
#endif

    // The collector's records of this thread's C stack (Python/gc_trace.c,
    // Python/gc_walk.c).
    struct {
        // Where the thread last detached (_PyGC_RecordDetach()); "valid"
        // is cleared when it attaches.
        _PyGCDetachRecord detach;
        // Where the live collector was entered, for the walk of the
        // collecting thread's own stack; entry.sp is 0 outside a
        // collection.
        _PyGCContext entry;
        // This thread is running the live collector's queue.
        int running_queue;
        // The value roots, innermost first (_PyThreadState_PushValueRoot())
        struct _PyValueRoot *value_roots;
        // The site (_PyGC_SITE_*) armed for one collection, or 0, and the
        // collections the sites have run (_PyGC_INJECT())
        int inject_site;
        int64_t injected;
        // Allocations since the last PYTHON_GCSTRESS collection
        int stress_count;
        // A reference tracer is running (_PyReftracerTrack()): no
        // collection starts, so a tracer cannot collect.
        int in_tracer;
    } gc_trace;



} _PyThreadStateImpl;

/* The sites where _testinternalcapi.gc_collect_at() arms one collection
 * (note 11 section 8.3, work/10a-1.md item 8): transitions that allocation
 * intervals do not reliably hit.  _PyGC_INJECT() is a load and a compare
 * in every build, so that the gated builds run the injection tests. */
#define _PyGC_SITE_DETACH       1  /* a detach entry, before the record */
#define _PyGC_SITE_PUSH_FRAME   2  /* _PyThreadState_PushFrame(), before
                                      the frame is initialized */
#define _PyGC_SITE_SPAN_REBASE  3  /* _PyValueSpan_Rebase(), after it */
#define _PyGC_SITE_MKSTACK      4  /* do_mkstack(), after each value */
#define _PyGC_SITE_INIT_LOCALS  5  /* initialize_locals(), after *args and
                                      **kwargs are stored */
#define _PyGC_SITE_RETURN       6  /* _RETURN_VALUE, after the caller's slot
                                      takes the value, before the clear */
#define _PyGC_SITE_TAKE_OWNERSHIP 7  /* take_ownership(), before the
                                        frame object of the caller is made */
#define _PyGC_SITE_SLOT_BOX     8  /* box_owner_for_descr(), after an inline
                                      owner's box is in the caller's span */
#define _PyGC_SITE_RECEIVER_BOX 9  /* method_check_args() and
                                      wrapperdescr_call(), after an inline
                                      receiver's box is in the caller's
                                      span */
#define _PyGC_SITE_SETTER_BOX   10 /* _testinternalcapi's SetterBox setter,
                                      between its box and the store */
#define _PyGC_SITE_MATCH_ARGS   11 /* _PyEval_MatchClass(), after the
                                      __match_args__ lookup */
#define _PyGC_SITE_UNPACK_DICT  12 /* _PyStack_KwNames(), after the names'
                                      allocation */
#define _PyGC_SITE_SLOT_LOOKUP  13 /* lookup_method_ex(), after a slot
                                      function's method lookup */
#define _PyGC_SITE_COUNT        14

PyAPI_FUNC(void) _PyGC_InjectCollect(PyThreadState *tstate);
// The configured process-wide allocation interval; 0 means stress is off.
PyAPI_FUNC(int) _PyGC_LiveGetStress(void);
// PYTHON_GCSTRESS=every from now on; 0 turns it off
PyAPI_FUNC(void) _PyGC_LiveSetStress(PyInterpreterState *interp, int every);

#define _PyGC_INJECT(tstate, site) \
    do { \
        if (((_PyThreadStateImpl *)(tstate))->gc_trace.inject_site \
            == (site)) \
        { \
            _PyGC_InjectCollect(tstate); \
        } \
    } while (0)

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_TSTATE_H */
