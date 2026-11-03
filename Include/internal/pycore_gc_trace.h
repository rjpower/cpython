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

/* Each diagnostic visitor enumerates one class of precise roots. */
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
