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

/* The stack pointer, or 0 where it cannot be read.  Inlined, so that it is
 * the caller's. */
static inline __attribute__((always_inline)) uintptr_t
_PyGC_StackAddress(void)
{
#if _Py__has_builtin(__builtin_stack_address)
    return (uintptr_t)__builtin_stack_address();
#elif defined(__GNUC__) && defined(__x86_64__)
    uintptr_t sp;
    __asm__ volatile("movq %%rsp, %0" : "=r"(sp));
    return sp;
#elif defined(__GNUC__) && defined(__aarch64__)
    uintptr_t sp;
    __asm__ volatile("mov %0, sp" : "=r"(sp));
    return sp;
#else
    return 0;
#endif
}

/* The live collector, which owns memory.  Python/gc_live.c has
 * the policy: when to collect, finalizers, weak references, the sweep and
 * the gc module's view.  The mark and the heap walks are here, in
 * Python/gc_trace.c, beside the verifier's, whose roots they share. */

/* The object map: one bit for each 8-byte word of the address space below
 * 2**48, set for the first word of every heap block's initialized object.
 * _Py_NewReference() sets it and freeing the block clears it (see
 * Objects/obmalloc.c), so whether a block holds an object is never guessed
 * from its contents: a block of raw data in an object heap, a free block
 * whose first word is an encoded free-list link and an object under
 * construction all have a clear bit.  The header word is all type and flags.
 *
 * The map has two levels: _PyGC_object_map has one entry for each 32 MiB
 * chunk of the address space, the size and alignment of a mimalloc segment,
 * and points to the chunk's bits, 512 KiB, mapped when the first object in
 * the chunk is made.  Both are anonymous mappings, which the conservative
 * scan of the loaded objects' data segments never reads, and their pages
 * are only resident where objects are: the bits cost 1/64 of the heap.
 * A chunk is one segment and a segment has one owning thread, so the bits
 * are written without atomics; the entries are installed with a
 * compare-exchange. */
#define _PyGC_MAP_CHUNK_SHIFT 25
#define _PyGC_MAP_CHUNKS ((size_t)1 << (48 - _PyGC_MAP_CHUNK_SHIFT))
#define _PyGC_MAP_CHUNK_WORDS \
    (((size_t)1 << _PyGC_MAP_CHUNK_SHIFT) / sizeof(void *) / 64)

extern uint64_t **_PyGC_object_map;
extern uint64_t *_PyGC_ObjectMapChunk(uintptr_t addr);

static inline uint64_t *
_PyGC_ObjectMapWord(uintptr_t addr, uint64_t *chunk)
{
    return &chunk[(addr >> 9) & (_PyGC_MAP_CHUNK_WORDS - 1)];
}

static inline uint64_t
_PyGC_ObjectMapBit(uintptr_t addr)
{
    return (uint64_t)1 << ((addr >> 3) & 63);
}

static inline uint64_t *
_PyGC_ObjectMapChunkOf(uintptr_t addr)
{
    uint64_t **map = (uint64_t **)_Py_atomic_load_ptr_relaxed(&_PyGC_object_map);
    return map != NULL
        ? (uint64_t *)_Py_atomic_load_ptr_relaxed(&map[addr >> _PyGC_MAP_CHUNK_SHIFT])
        : NULL;
}

/* Does the block at op hold an initialized object? */
static inline int
_PyGC_IsHeapObject(const PyObject *op)
{
    uintptr_t addr = (uintptr_t)op;
    uint64_t *chunk = _PyGC_ObjectMapChunkOf(addr);
    return chunk != NULL
        && (*_PyGC_ObjectMapWord(addr, chunk) & _PyGC_ObjectMapBit(addr)) != 0;
}

static inline void
_PyGC_ObjectMapSet(PyObject *op)
{
    uintptr_t addr = (uintptr_t)op;
    uint64_t *chunk = _PyGC_ObjectMapChunkOf(addr);
    if (chunk == NULL) {
        chunk = _PyGC_ObjectMapChunk(addr);
    }
    *_PyGC_ObjectMapWord(addr, chunk) |= _PyGC_ObjectMapBit(addr);
}

static inline void
_PyGC_ObjectMapClear(PyObject *op)
{
    uintptr_t addr = (uintptr_t)op;
    uint64_t *chunk = _PyGC_ObjectMapChunkOf(addr);
    if (chunk != NULL) {
        *_PyGC_ObjectMapWord(addr, chunk) &= ~_PyGC_ObjectMapBit(addr);
    }
}

/* What the collection does with a dead object, from its type, so that its
 * passes over the dead objects skip most without reading them. */
#define _PyGC_DEAD_WEAKREFS  (1 << 0)  /* the type supports weak references */
#define _PyGC_DEAD_FINALIZE  (1 << 1)  /* tp_finalize */
#define _PyGC_DEAD_DEL       (1 << 2)  /* a legacy tp_del */
#define _PyGC_DEAD_FIRST     (1 << 3)  /* its dealloc reads its referents:
                                          swept first */
#define _PyGC_DEAD_TYPE      (1 << 4)  /* a type: swept last */
#define _PyGC_DEAD_GC        (1 << 5)  /* a GC object */
#define _PyGC_DEAD_HOOK      (1 << 6)  /* its dealloc would notify watchers,
                                          call m_free or a capsule
                                          destructor, or release a buffer to
                                          Python code: the collector does
                                          that before the sweep */
#define _PyGC_DEAD_KILLED    (1 << 7)  /* PYTHON_GCVERIFY=kill killed it: its
                                          frees are quarantined */

/* An unreachable object found by the mark: its address, the size and
 * pre-header size of its block, and its _PyGC_DEAD_* kinds. */
typedef struct {
    PyObject *op;
    uint32_t block_size;
    uint16_t presize;
    uint16_t kinds;
} _PyGCDeadObject;

/* The root phase's counters (note 11 section 9).  The conservative scans
 * count the words they read by class, and the words of them that hit an
 * object; the stack scan counts the eval-loop words it excludes.  Each time
 * is in nanoseconds and includes the marking that its roots cause.  The
 * walk's counters (frames, descriptor cache, fallbacks) are zero until the
 * walk exists, so that every build reports one schema.  unpinned counts the
 * other threads visited with an unpinned detach record, whose stacks are
 * scanned conservatively (note 11 question 16).  Under PYTHON_GCVERIFY=retain
 * or kill, verify_q counts the objects that only the roots outside A reach
 * and verify_candidates the objects that such a collection listed and a
 * later one reached from A (work/10a-1.md item 7). */
#define _PyGC_SCAN_STACK    0  /* C stacks */
#define _PyGC_SCAN_REGS     1  /* captured register arrays */
#define _PyGC_SCAN_MEM      2  /* PyMem blocks the scans lead to */
#define _PyGC_SCAN_SHALLOW  3  /* blocks of objects marked without traversal */
#define _PyGC_SCAN_DATA     4  /* writable data segments */
#define _PyGC_SCAN_STRUCTS  5  /* thread and interpreter structs, data stacks */
#define _PyGC_SCAN_CLASSES  6
#define _PyGC_SCAN_CLASS_NAMES \
    {"stack", "regs", "mem", "shallow", "data", "structs"}

#define _PyGC_ROOT_STATS_FIELDS(X) \
    X(collections) X(threads) X(activations_skipped) X(eval_words_skipped) \
    X(frames_walked) X(cache_hits) X(cache_misses) X(cache_flushes) \
    X(fallbacks) X(unpinned) X(verify_q) X(verify_candidates) X(precise_ns) \
    X(stack_ns) X(data_ns) X(structs_ns) X(roots_ns)

typedef struct _PyGCRootStats {
#define _PyGC_ROOT_STATS_FIELD(name) int64_t name;
    _PyGC_ROOT_STATS_FIELDS(_PyGC_ROOT_STATS_FIELD)
#undef _PyGC_ROOT_STATS_FIELD
    int64_t words[_PyGC_SCAN_CLASSES];
    int64_t hits[_PyGC_SCAN_CLASSES];
} _PyGCRootStats;

typedef struct {
    _PyGCDeadObject *dead;    // from malloc(); the caller frees it
    Py_ssize_t ndead;
    int kinds;                // the union of the dead objects' kinds
    Py_ssize_t nobjects;      // the objects in the heaps
    Py_ssize_t live_bytes;    // the blocks of the reachable objects
    Py_ssize_t nmarked;
    Py_ssize_t nshallow;
    _PyGCRootStats roots;     // this collection's root phase
} _PyGCMarkResult;

/* Mark everything reachable from the roots of the interpreters that share
 * tstate's object heaps, then walk the heaps: a marked object is unmarked,
 * an unmarked one gets _Py_GC_DEAD_FLAG and is listed.  Returns -1, with
 * nothing listed or flagged, if the collector's own memory ran out. */
extern int _PyGC_TraceLiveMark(PyThreadState *tstate, _PyGCMarkResult *res);

/* The interpreter's descriptor cache for the stack walk (Python/gc_walk.c),
 * or NULL if the collector's state cannot be allocated. */
extern struct _PyGCWalkCache *_PyGC_LiveWalkCache(PyInterpreterState *interp);

#ifdef Py_DEBUG
/* The verifier's kill mode: while it is set, the frees of that thread state
 * (PyObject_Free(), PyMem_Free() and the free in a moving realloc) put the
 * block in quarantine (_PyGC_Quarantine()) instead of freeing it.  The sweep
 * sets it around the dealloc and the free of each object that kill killed;
 * the GIL protects it. */
extern PyThreadState *_PyGC_quarantine_tstate;
/* Quarantine the mimalloc block at ptr: fill it with PYMEM_DEADBYTE, poison
 * it under ASan, and free it 16 collections later. */
extern void _PyGC_Quarantine(void *ptr);
#endif

/* For the test of the verifier's retain mode: create [None], hold it only in
 * a PyMem block that this core frame points to, call callable, and return
 * the list if give_back is set, else None. */
PyAPI_FUNC(PyObject *) _PyGC_TestHoldInCoreMem(PyObject *callable,
                                               int give_back);

/* The classes of roots (_PyGC_TRACE_ROOT_*) that each alone reach the object
 * at addr, or -1.  For debugging; export for _testinternalcapi. */
PyAPI_FUNC(int) _PyGC_TraceLiveRootsOf(PyThreadState *tstate, uintptr_t addr);
/* The same for the object at *hidden ^ mask, read after each mark, so that
 * the caller's stack need not hold its address. */
PyAPI_FUNC(int) _PyGC_TraceLiveRootsOfHidden(
    PyThreadState *tstate, const volatile uintptr_t *hidden, uintptr_t mask);

/* For tests of the segment map that the live mark looks objects up in: the
 * object found at addr (or containing it, if interior is set), or 0; four
 * addresses in op's segment that hold no block, or 0 where there is no such
 * address (see gc_trace.c); and the segment the map holds for addr, or 0.  Export for _testinternalcapi. */
PyAPI_FUNC(uintptr_t) _PyGC_TraceLiveLookup(PyThreadState *tstate,
                                            uintptr_t addr, int interior);
PyAPI_FUNC(void) _PyGC_TraceSegmentProbes(PyObject *op, uintptr_t probes[4]);
PyAPI_FUNC(uintptr_t) _PyGC_TraceSegmentOf(uintptr_t addr);

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

/* Collect now, from C code that may hold objects only in C locals, leaving
 * the finalizers and weak reference callbacks to the eval breaker: 1 if a
 * collection ran, 0 if one cannot run here.  An allocation that failed
 * calls this before it gives up. */
extern int _PyGC_LiveCollectHere(PyThreadState *tstate);

#ifdef Py_DEBUG
/* A dealloc that the sweep called did what a dealloc must not do in
   the sweep: allocate an object, which may refer to the dead, or run Python
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

/* Clear a local whose address was taken, such as PyDict_Next()'s
 * out-parameters: it stays in its frame after its last use, where the
 * conservative scan of the stack would keep its object alive.  The local is
 * a PyObject * or a PyValue, and zero bits are NULL for both.  The barrier
 * keeps the store, which is dead to the compiler. */
#  define _PyGC_CLEAR_LOCAL(var) \
    do { \
        memset(&(var), 0, sizeof(var)); \
        __asm__ volatile("" : : "r"(&(var)) : "memory"); \
    } while (0)

/* Remember an immortal heap object: what it refers to is a root. */
extern void _PyGC_LiveAddImmortal(PyObject *op);

/* Keep the thread states that finalization unlinks, linked through next,
 * for the rest of the interpreter's life, and their threads' frames and C
 * stacks as roots: the daemon threads never return to drop what they hold,
 * as reference counting never frees it. */
extern void _PyGC_LiveKeepThreads(PyThreadState *list);
extern PyThreadState *_PyGC_LiveKeptThreads(PyInterpreterState *interp);

/* Free the block of a dead object that the sweep has deallocated
 * (PyObject_Free() without looking the block up). */
extern void _PyGC_FreeDeadBlock(PyObject *op, void *block);

/* Zero nbytes of the C stack below the caller's frame, where the frames
 * of its next callees go.  Their padding, and spill slots that a call
 * writes only on some paths, would otherwise keep the stale words that
 * earlier calls left there, which the scan takes for roots.  On x86-64 the
 * stores are inline assembly below the caller's stack pointer, so that they
 * reach the top of the region: a clearing function's own frame would cover
 * it, and the next callee's padding would land on the words that frame
 * left (test_asyncio.test_futures2, test_gc).  The caller makes calls, so
 * nothing of its own lives below its stack pointer. */
extern void _PyGC_ClearStackBuffer(size_t nbytes);

static inline void
_PyGC_ClearStackBelow(size_t nbytes)
{
#if defined(__x86_64__) && defined(__GNUC__)
    void *dst;
    unsigned long count;
    __asm__ volatile("mov %%rsp, %0\n\t"
                     "sub %2, %0\n\t"
                     "xor %%eax, %%eax\n\t"
                     "rep stosq"
                     : "=&D"(dst), "=&c"(count)
                     : "r"(nbytes), "1"(nbytes / 8)
                     : "rax", "memory");
#else
    _PyGC_ClearStackBuffer(nbytes);
#endif
}

/* Before a collection: the collector's entry frames go below the caller. */
static inline void
_PyGC_LiveClearStack(void)
{
    _PyGC_ClearStackBelow(16 * 1024);
}

/* At the eval loop's entry, for its frame and the entry's. */
static inline void
_PyGC_LiveClearEvalStack(void)
{
    _PyGC_ClearStackBelow(1024);
}

/* The root phase's counters, summed over the collections of interp, and
 * those of its last collection.  Export for _testinternalcapi. */
PyAPI_FUNC(void) _PyGC_LiveGetRootStats(PyInterpreterState *interp,
                                        _PyGCRootStats *total,
                                        _PyGCRootStats *last);

/* Totals for gc.get_stats(): the bytes the sweeps freed, the bytes the last
 * collection found live, and the longest pause in seconds. */
extern void _PyGC_LiveGetStats(PyInterpreterState *interp,
                               Py_ssize_t *bytes_freed, Py_ssize_t *live_bytes,
                               double *pause_max);



#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_GC_TRACE_H */
