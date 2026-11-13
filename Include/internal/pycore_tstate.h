#ifndef Py_INTERNAL_TSTATE_H
#define Py_INTERNAL_TSTATE_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_typedefs.h"      // _Py_valuetracefunc
#include "pycore_brc.h"             // struct _brc_thread_state
#include "pycore_freelist_state.h"  // struct _Py_freelists
#include "pycore_interpframe_structs.h"  // _PyInterpreterFrame
#include "pycore_mimalloc.h"        // struct _mimalloc_thread_state
#include "pycore_qsbr.h"            // struct qsbr
#include "pycore_structs.h"

#ifdef Py_GIL_DISABLED
struct _gc_thread_state {
    /* Thread-local allocation count. */
    Py_ssize_t alloc_count;
};
#endif

// The borrow stack (Python/borrow.c, pycore_borrow.h): owned references
// that back borrowed results handed out by the argument parsers.  The types
// live here because _PyThreadStateImpl embeds the stack.
typedef struct _PyBorrowChunk {
    struct _PyBorrowChunk *previous;
    PyObject **previous_top;    // previous chunk's top when this one opened
    PyObject **end;
    Py_ssize_t base_depth;      // entries in the chunks below this one
    PyObject *slots[];
} _PyBorrowChunk;

typedef struct {
    PyObject **top;             // next free slot
    PyObject **end;             // end of the current chunk
    PyObject **eval_floor;      // top when the innermost eval loop was
                                // entered; NULL outside any eval loop
    _PyBorrowChunk *chunk;      // initially the empty chunk in borrow.c
    _PyBorrowChunk *spare;      // one cached chunk
} _PyBorrowStack;


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

    _PyBorrowStack borrow;

    // The innermost activation of the evaluation loop (Python/ceval.h,
    // _PyEvalActivation), or NULL.  Stack-switching code saves and restores
    // it with current_frame.
    struct _PyEvalActivation *eval_activation;
    /* Frame whose observer callback is running; callback frames themselves
     * are not suspended semantic children of this frame. */
    struct _PyInterpreterFrame *observed_frame;

    // Nested outlined retirements, including detached result owners.
    struct _PyTransitionRoots *transition_roots;

    // Distinguishes between yield and return from PyEval_EvalFrame().
    // See gen_send_ex2() in Objects/genobject.c
    enum {
        GENERATOR_RETURN = 0,
        GENERATOR_YIELD = 1,
    } generator_return_kind;

    /* Only the executing verifier publishes this while holding HEAD_LOCK
     * through recursive tp_traverse calls. A frame-owner hint is not a lock. */
    unsigned int head_locked_frame_walk;

    /* Head of circular linked-list of all tasks which are instances of `asyncio.Task`
       or subclasses of it used in `asyncio.all_tasks`.
    */
    struct llist_node asyncio_tasks_head;
    struct _qsbr_thread_state *qsbr;  // only used by free-threaded build
    struct llist_node mem_free_queue; // delayed free queue

#ifdef WITH_MIMALLOC
    struct _mimalloc_thread_state mimalloc;
#endif

#ifndef Py_GIL_DISABLED
    // The callee-saved registers and stack pointer of the thread when it
    // last released the GIL, for the tracing verifier's scan of its C stack
    // (Python/gc_trace.c).  "valid" is cleared when the thread attaches.
    struct {
        uintptr_t regs[24];
        uintptr_t sp;
        int valid;
    } gc_trace;
#endif

#ifdef Py_GIL_DISABLED
    struct _gc_thread_state gc;
    struct _Py_freelists freelists;
    struct _brc_thread_state brc;
    struct {
        // The per-thread refcounts
        Py_ssize_t *values;

        // Size of the refcounts array.
        Py_ssize_t size;

        // If set, don't use per-thread refcounts
        int is_finalized;
    } refcounts;

    // Index to use to retrieve thread-local bytecode for this thread
    int32_t tlbc_index;

    // When >1, code objects do not immortalize their non-string constants.
    int suppress_co_const_immortalization;

#ifdef Py_STATS
     // per-thread stats, will be merged into interp->pystats_struct
     PyStats *pystats_struct; // allocated by _PyStats_ThreadInit()
#endif

#endif // Py_GIL_DISABLED

#if defined(Py_REF_DEBUG) && defined(Py_GIL_DISABLED)
    Py_ssize_t reftotal;  // this thread's total refcount operations
#endif

    // The functions that the event handlers call (set with
    // c_tracefunc/c_profilefunc, which stay for readers of the public
    // fields): non-NULL if and only if the public one is.
    _Py_valuetracefunc value_tracefunc;
    _Py_valuetracefunc value_profilefunc;

#ifdef Py_GIL_DISABLED
    // gh-144438: Add padding to ensure that the fields above don't share a
    // cache line with other allocations.
    char __padding[_Py_QSBR_CACHE_LINE_SIZE];
#endif
} _PyThreadStateImpl;

#if SIZEOF_VOID_P == 8
/* The cold traversal guard occupies the padding after the return-kind enum. */
static_assert(offsetof(_PyThreadStateImpl, asyncio_tasks_head) ==
              offsetof(_PyThreadStateImpl, generator_return_kind) + 8,
              "cold traversal guard occupies existing padding");
#endif

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_TSTATE_H */
