#ifndef Py_INTERNAL_TSTATE_H
#define Py_INTERNAL_TSTATE_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_freelist_state.h"  // struct _Py_freelists
#include "pycore_interpframe_structs.h"  // _PyInterpreterFrame
#include "pycore_mimalloc.h"        // struct _mimalloc_thread_state
#include "pycore_qsbr.h"            // struct qsbr
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

    // The callee-saved registers and stack pointer of the thread when it
    // last released the GIL, for the tracing verifier's scan of its C stack
    // (Python/gc_trace.c).  "valid" is cleared when the thread attaches.
    struct {
        uintptr_t regs[24];
        uintptr_t sp;
        int valid;
        // The same, recorded where the live collector was entered, for the
        // scan of the collecting thread's own stack; entry_sp is 0 outside
        // a collection.
        uintptr_t entry_regs[24];
        uintptr_t entry_sp;
        // This thread is running the live collector's queue.
        int running_queue;
    } gc_trace;



} _PyThreadStateImpl;

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_TSTATE_H */
