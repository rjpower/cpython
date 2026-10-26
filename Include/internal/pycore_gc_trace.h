#ifndef Py_INTERNAL_GC_TRACE_H
#define Py_INTERNAL_GC_TRACE_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include <setjmp.h>               // jmp_buf
#include <string.h>               // memcpy()

/* The mark phase of the tracing collector, run as a verifier under
 * reference counting (note 06, step 4).  After a full collection it marks
 * everything reachable from the roots, then reports every object with a
 * nonzero, mortal reference count that it did not reach: a "hole", a
 * reference that the tracing collector of step 5 would miss.  Nothing is
 * freed.  See Python/gc_trace.c. */

/* Verification modes (GCState.trace_mode) */
#define _PyGC_TRACE_OFF       0
#define _PyGC_TRACE_EXPLICIT  1   /* explicit full collections (gc.collect()) */
#define _PyGC_TRACE_AUTO      2   /* also automatic collections of gen >= 1 */

/* Root classes, for measuring what each contributes */
#define _PyGC_TRACE_ROOT_FRAMES     (1 << 0)  /* thread frames */
#define _PyGC_TRACE_ROOT_TSTATE     (1 << 1)  /* thread state fields */
#define _PyGC_TRACE_ROOT_INTERP     (1 << 2)  /* interpreter state fields */
#define _PyGC_TRACE_ROOT_TYPES      (1 << 3)  /* registered static types */
#define _PyGC_TRACE_ROOT_IMMORTAL   (1 << 4)  /* immortal heap objects */
#define _PyGC_TRACE_ROOT_PRECISE    0x1f
#define _PyGC_TRACE_ROOT_STACKS     (1 << 5)  /* C stacks and registers */
#define _PyGC_TRACE_ROOT_DATA       (1 << 6)  /* writable data segments */
#define _PyGC_TRACE_ROOT_STRUCTS    (1 << 7)  /* thread and interpreter
                                                 state structs, scanned
                                                 conservatively */
#define _PyGC_TRACE_ROOT_ALL        0xff

/* Called from gc_collect_main() after a collection. */
extern void _PyGC_TraceAfterCollect(PyThreadState *tstate, int generation,
                                    int manual);

/* Run a verification now; returns the number of holes, or -1 if the
 * verifier cannot run (allocator, memory).  Export for _testinternalcapi. */
PyAPI_FUNC(Py_ssize_t) _PyGC_TraceVerify(PyThreadState *tstate);

/* The holes found by the last verification, as a list of
 * (type name, address, refcount, referrer) tuples.  Export for
 * _testinternalcapi. */
PyAPI_FUNC(PyObject *) _PyGC_TraceGetHoles(PyInterpreterState *interp);

/* The number of verifications that ran.  Export for _testinternalcapi. */
PyAPI_FUNC(Py_ssize_t) _PyGC_TraceCount(PyInterpreterState *interp);

/* Remember a static (not heap allocated) type passed to PyType_Ready() so
 * that the mark can traverse it as a root. */
extern void _PyGC_TraceRegisterStaticType(PyInterpreterState *interp,
                                          PyTypeObject *type);

/* An explicitly registered root: objects that C code holds in memory the
 * collector does not scan, such as a PyMem_Malloc() block.  The node lives
 * in that memory; the default build links it into the interpreter's list
 * while the objects are held.  This is the fix for references hidden in
 * non-object memory that cannot be given a tp_traverse.  Adding needs an
 * attached thread state, for the interpreter; removing does not.  The
 * free-threaded build does not link the node. */
typedef struct _PyGCRoot {
    struct _PyGCRoot *prev;
    struct _PyGCRoot *next;
    PyInterpreterState *interp;
    PyObject **objs;
    Py_ssize_t n;
} _PyGCRoot;

extern void _PyGC_AddRoot(_PyGCRoot *root, PyObject **objs, Py_ssize_t n);
extern void _PyGC_RemoveRoot(_PyGCRoot *root);

extern PyStatus _PyGC_TraceInit(PyInterpreterState *interp);
extern void _PyGC_TraceFini(PyInterpreterState *interp);

/* The root visitors, shared by the verifier and the live collector
 * (Python/gc_live.c).  Each visits the objects one class of precise roots
 * holds (section 14.2 of note 06). */
extern int _PyGC_TraceVisitFrames(PyThreadState *p, visitproc visit, void *arg);
extern int _PyGC_TraceVisitThreadState(PyThreadState *p, visitproc visit,
                                       void *arg);
extern int _PyGC_TraceVisitInterp(PyInterpreterState *interp, visitproc visit,
                                  void *arg);
/* The static types passed to PyType_Ready() in interp. */
extern PyTypeObject **_PyGC_TraceStaticTypes(PyInterpreterState *interp,
                                             Py_ssize_t *n);
/* Visit what op refers to: tp_traverse, _PyType_TraverseStatic() for a
 * static type, and the heap type of a non-GC instance. */
extern int _PyGC_TraceTraverse(PyObject *op, visitproc visit, void *arg);

/* Store the callee-saved registers in regs[24].  regs is passed in a
 * register that is not callee-saved, so that computing it cannot overwrite
 * one.  Inlined, so that the registers are the caller's. */
static inline __attribute__((always_inline)) void
_PyGC_CaptureRegisters(uintptr_t *regs)
{
#if defined(__GNUC__) && defined(__x86_64__)
    __asm__ volatile(
        "movq %%rbx, 0(%0)\n\t"
        "movq %%rbp, 8(%0)\n\t"
        "movq %%r12, 16(%0)\n\t"
        "movq %%r13, 24(%0)\n\t"
        "movq %%r14, 32(%0)\n\t"
        "movq %%r15, 40(%0)\n\t"
        : : "D"(regs) : "memory");
#elif defined(__GNUC__) && defined(__aarch64__)
    register uintptr_t *p __asm__("x9") = regs;
    __asm__ volatile(
        "stp x19, x20, [%0, #0]\n\t"
        "stp x21, x22, [%0, #16]\n\t"
        "stp x23, x24, [%0, #32]\n\t"
        "stp x25, x26, [%0, #48]\n\t"
        "stp x27, x28, [%0, #64]\n\t"
        "str x29, [%0, #80]\n\t"
        "stp d8, d9, [%0, #88]\n\t"
        "stp d10, d11, [%0, #104]\n\t"
        "stp d12, d13, [%0, #120]\n\t"
        "stp d14, d15, [%0, #136]\n\t"
        : : "r"(p) : "memory");
#else
    // setjmp() stores the callee-saved registers, though some C libraries
    // mangle the stack and frame pointers.
    jmp_buf buf;
    (void)setjmp(buf);
    memcpy(regs, &buf, Py_MIN(sizeof(buf), 24 * sizeof(uintptr_t)));
#endif
}

#ifdef Py_GC
/* The live collector, which owns memory under Py_GC.  Python/gc_live.c has
 * the policy: when to collect, finalizers, weak references, the sweep and
 * the gc module's view.  The mark and the heap walks are here, in
 * Python/gc_trace.c, beside the verifier's, whose roots they share. */

/* Does the block at op hold an initialized object?  _Py_NewReference()
 * writes the header word; freeing the block clears it.  The reference count
 * half is a constant under Py_GC, which makes a stray match in a block of
 * raw data unlikely. */
static inline int
_PyGC_IsObjectHeader(const PyObject *op)
{
    uint64_t w = (uint64_t)op->ob_refcnt_full;
    uint32_t refcnt = (uint32_t)w;
    return ((w >> 32) & 0xffff) == _Py_GC_OBJECT_MAGIC
        && ((w >> 48) & _Py_GC_HEAP_FLAG) != 0
        && (refcnt == _Py_GC_REFCNT || refcnt == _Py_IMMORTAL_INITIAL_REFCNT);
}

/* What the collection does with a dead object, from its type, so that its
 * passes over the dead objects skip most without reading them. */
#define _PyGC_DEAD_WEAKREFS  (1 << 0)  /* the type supports weak references */
#define _PyGC_DEAD_FINALIZE  (1 << 1)  /* tp_finalize */
#define _PyGC_DEAD_DEL       (1 << 2)  /* a legacy tp_del */
#define _PyGC_DEAD_MBUF      (1 << 3)  /* a managed buffer: swept first */
#define _PyGC_DEAD_TYPE      (1 << 4)  /* a type: swept last */
#define _PyGC_DEAD_GC        (1 << 5)  /* a GC object */

/* An unreachable object found by the mark: its address, the size and
 * pre-header size of its block, and its _PyGC_DEAD_* kinds. */
typedef struct {
    PyObject *op;
    uint32_t block_size;
    uint16_t presize;
    uint16_t kinds;
} _PyGCDeadObject;

typedef struct {
    _PyGCDeadObject *dead;    // from malloc(); the caller frees it
    Py_ssize_t ndead;
    int kinds;                // the union of the dead objects' kinds
    Py_ssize_t nobjects;      // the objects in the heaps
    Py_ssize_t live_bytes;    // the blocks of the reachable objects
    Py_ssize_t nmarked;
    Py_ssize_t nshallow;
} _PyGCMarkResult;

/* Mark everything reachable from the roots of the interpreters that share
 * tstate's object heaps, then walk the heaps: a marked object is unmarked,
 * an unmarked one gets _Py_GC_DEAD_FLAG and is listed.  Returns -1, with
 * nothing listed or flagged, if the collector's own memory ran out. */
extern int _PyGC_TraceLiveMark(PyThreadState *tstate, _PyGCMarkResult *res);

/* Record the caller's callee-saved registers and stack pointer as the place
 * the collection was entered: the mark scans the calling thread's stack from
 * there up, not from its own frame, below which lie stale words.  Call it
 * first; reset gc_trace.entry_sp to the value it had before when done. */
extern void _PyGC_TraceCaptureEntry(void);

/* The classes of roots (_PyGC_TRACE_ROOT_*) that each alone reach the object
 * at addr, or -1.  For debugging; export for _testinternalcapi. */
PyAPI_FUNC(int) _PyGC_TraceLiveRootsOf(PyThreadState *tstate, uintptr_t addr);

/* Every object in the heaps that interp shares, in an array from malloc(),
 * or NULL with *n set to -1 if memory ran out.  The objects are not
 * kept alive: the caller must not let a collection run before it holds
 * them. */
extern PyObject **_PyGC_TraceLiveHeapObjects(PyInterpreterState *interp,
                                             Py_ssize_t *n);

/* The roots the live collector itself holds for interp: the immortal heap
 * objects that refer to others, and the queue of finalizers and weak
 * reference callbacks. */
extern int _PyGC_LiveVisitRoots(PyInterpreterState *interp, visitproc visit,
                                void *arg);

/* Called by an object allocation when alloc_bytes passes alloc_limit.  It
 * collects, then runs the finalizers and weak reference callbacks it queued,
 * unless a collection cannot run here; the allocation follows. */
extern void _PyGC_LiveOnThreshold(PyThreadState *tstate);

#ifdef Py_DEBUG
/* A dealloc that the sweep called did what a dealloc must not do under
   Py_GC: allocate an object, which may refer to the dead, or run Python
   code, which may reach them.  what says which; this aborts. */
extern void _Py_NO_RETURN _PyGC_LiveSweepViolation(PyThreadState *tstate,
                                                  const char *what);
#  define _PyGC_CHECK_NOT_SWEEPING(tstate, what) \
    do { \
        if ((tstate)->interp->gc.sweeping) { \
            _PyGC_LiveSweepViolation((tstate), (what)); \
        } \
    } while (0)
#else
#  define _PyGC_CHECK_NOT_SWEEPING(tstate, what) ((void)0)
#endif

/* Remember an immortal heap object: what it refers to is a root. */
extern void _PyGC_LiveAddImmortal(PyObject *op);

/* Zero the C stack below the caller's frame, where the collector's entry
 * frames go next.  Their padding would otherwise keep the stale words that
 * earlier calls left there, which the scan takes for roots. */
extern void _PyGC_LiveClearStack(void);

/* Free the block of a dead object that the sweep has deallocated
 * (PyObject_Free() without looking the block up). */
extern void _PyGC_FreeDeadBlock(PyObject *op, void *block);

/* The same for the eval loop's frame, which has padding too, and spill
 * slots that a call writes only on some paths. */
extern void _PyGC_LiveClearEvalStack(void);

/* Totals for gc.get_stats(): the bytes the sweeps freed, the bytes the last
 * collection found live, and the longest pause in seconds. */
extern void _PyGC_LiveGetStats(PyInterpreterState *interp,
                               Py_ssize_t *bytes_freed, Py_ssize_t *live_bytes,
                               double *pause_max);
#endif

/* Record the calling thread's callee-saved registers and stack pointer in
 * its thread state, before it releases the GIL, so that a collection in
 * another thread can scan them.  It must be called before the caller
 * changes any callee-saved register, so call it first. */
#if defined(WITH_MIMALLOC) && !defined(Py_GIL_DISABLED)
PyAPI_FUNC(void) _PyGC_TraceCaptureThread(void);
#  define _PyGC_TRACE_CAPTURE_THREAD() _PyGC_TraceCaptureThread()
#else
#  define _PyGC_TRACE_CAPTURE_THREAD() ((void)0)
#endif

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_GC_TRACE_H */
