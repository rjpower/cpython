#ifndef Py_INTERNAL_GC_WALK_H
#define Py_INTERNAL_GC_WALK_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

/* Precise C frames (boil/design/11-precise-roots.md, section 2).
 *
 * Py_GC_PRECISE on a function's definition, or on a prior declaration,
 * places it in section py_gc_precise.  The collector's stack walk scans no
 * frame whose pc lies in that section: the function keeps every object it
 * holds across a collecting call anchored where the collector sees it (a
 * frame slot, a pushed value span, a traced field, a static).  The build
 * runs root_check's C mode (Tools/cases_generator/root_check_c.py) over
 * every file that uses the marker, so a function marked before it passes
 * the check fails the build.
 *
 * Py_GC_ARGS_ANCHORED: a static precise function whose callers anchor its
 * arguments; the C mode checks every call, and that it is called only from
 * precise functions and never through its address.
 *
 * Py_GC_SLOTS on a PyValue * parameter: it points into a frame's slots,
 * which the collector visits, so a load through it is anchored until a
 * store through any PyValue *. */

#if defined(__ELF__) && (defined(__GNUC__) || defined(__clang__))
#  define Py_GC_PRECISE __attribute__((section("py_gc_precise")))
#else
#  define Py_GC_PRECISE
#endif
#define Py_GC_ARGS_ANCHORED
#define Py_GC_SLOTS


/* --- the stack walk (note 11 section 3) ---------------------------------- */

/* _PyGC_CaptureCaller() is in assembly (Python/gc_capture.S) where it can
 * read the callee-saved registers before any prologue; the walk itself is
 * on for x86-64 ELF with glibc's _dl_find_object() (2.35).  Elsewhere the
 * stacks are scanned conservatively in full. */
#if defined(__ELF__) && defined(__GNUC__) \
    && (defined(__x86_64__) || defined(__aarch64__))
#  define _PyGC_CAPTURE_ASM 1
#endif
/* The detach entries capture their caller's context in assembly too
 * (Python/gc_capture.S): PyEval_SaveThread(), PyEval_ReleaseThread() and
 * their pinned forms (note 11 section 13, question 16). */
#if defined(_PyGC_CAPTURE_ASM) && defined(__x86_64__)
#  define _PyGC_DETACH_ASM 1
#endif
#if defined(_PyGC_CAPTURE_ASM) && defined(__x86_64__) && defined(__GLIBC__) \
    && (__GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 35))
#  define _PyGC_WALK 1
#endif

/* The callee-saved registers, in DWARF order: x86-64 rbx, rbp, r12 to r15;
 * arm64 x19 to x30, d8 to d15. */
#if defined(__aarch64__)
#  define _PyGC_NSAVED 20
#else
#  define _PyGC_NSAVED 6
#endif

/* One frame stopped at a call: the pc after the call, the stack pointer at
 * the call, and the callee-saved registers' values there. */
typedef struct {
    uintptr_t pc;
    uintptr_t sp;
    uintptr_t regs[_PyGC_NSAVED];
} _PyGCContext;

/* Fill ctx with the caller's context at the call to this function. */
PyAPI_FUNC(void) _PyGC_CaptureCaller(_PyGCContext *ctx);

/* A thread's record of where it detached (3.3, question 16): its caller's
 * context at the detach entry.  A pinned record's caller does not return
 * before the thread reattaches (Py_BEGIN_ALLOW_THREADS and the internal
 * pinned entries), so every frame outward of the capture persists, and a
 * collection walks from the capture.  Of an unpinned one (a direct
 * PyEval_SaveThread(), which may return detached), a collection scans the
 * registers and the stack from cap.sp up conservatively. */
typedef struct {
    _PyGCContext cap;
    int pinned;
    int valid;
} _PyGCDetachRecord;

/* Frame classes (note 11 section 2) */
#define _PyGC_FRAME_PRECISE     1
#define _PyGC_FRAME_CORE        2   /* core opaque: the interpreter's image */
#define _PyGC_FRAME_EXTENSION   3   /* extension opaque: any other object */

/* Flags of a registered code range */
#define _PyGC_RANGE_PRESERVES_NOTHING  1   /* saves no callee-saved register */
#define _PyGC_RANGE_TEST               2   /* a test's: its fallbacks are its */

/* A decoded frame descriptor, for the pc it was looked up with. */
#define _PyGC_RULE_SAME       0   /* the register is the caller's */
#define _PyGC_RULE_OFFSET     1   /* the caller's value is at CFA + off */
#define _PyGC_RULE_UNDEFINED  2   /* not preserved */
#define _PyGC_CFA_SP          (-1)

typedef struct {
    uintptr_t pc;               /* lookup pc; 0 for an empty cache slot */
    uintptr_t func_start;       /* the FDE's range */
    uintptr_t func_end;
    int32_t cfa_off;
    int32_t ra_off;             /* the return address is at CFA + ra_off */
    int8_t cfa_reg;             /* _PyGC_CFA_SP or a callee-saved index */
    uint8_t ok;                 /* 0: no descriptor for this pc */
    uint8_t ra_undefined;       /* the outermost frame */
    uint8_t extension;          /* outside the interpreter's image */
    int8_t rule[_PyGC_NSAVED];
    int32_t off[_PyGC_NSAVED];
} _PyGCFrameDesc;

/* Register [start, end) as code of class cls (process-wide; generated code
 * and tests).  desc, if not NULL, describes the frame at every return
 * address in the range, for code with no FDE; it is copied.  -1 with no
 * exception set if the table is full. */
PyAPI_FUNC(int) _PyGC_RegisterCodeRange(uintptr_t start, uintptr_t end,
                                        int cls, int flags,
                                        const _PyGCFrameDesc *desc);
PyAPI_FUNC(void) _PyGC_UnregisterCodeRange(uintptr_t start);

/* The descriptor cache, per interpreter, valid for one loader generation
 * (dl_iterate_phdr()'s dlpi_subs) and one generation of the range table. */
typedef struct _PyGCWalkCache {
    _PyGCFrameDesc *slots;
    size_t mask;
    size_t used;
    unsigned long long subs;
    unsigned int ranges;        /* the range table's generation */
    int64_t flushes;            /* flushes not yet counted in a mark's stats */
} _PyGCWalkCache;

/* Decode the descriptor for lookup_pc, uncached.  0, or -1 if there is
 * none (out->ok is 0 then too). */
PyAPI_FUNC(int) _PyGC_DecodeDescriptor(uintptr_t lookup_pc,
                                       _PyGCFrameDesc *out);

/* The descriptor of a frame that pushed the frame pointer, set it to the
 * stack pointer and saves nothing else, as the perf trampoline's does.
 * -1 where the walk is not built. */
extern int _PyGC_FramePointerDescriptor(_PyGCFrameDesc *out);

struct _PyEvalActivation;
struct _PyGCRootStats;

/* One walk of a stack.  The walk calls scan for each range of words it
 * finds opaque, conservatively; frame, if set, for each frame, before the
 * scans of the words the frame owns. */
typedef struct {
    void (*scan)(void *arg, uintptr_t start, uintptr_t end);
    void (*frame)(void *arg, uintptr_t pc, uintptr_t sp, uintptr_t cfa,
                  int cls);
    void *arg;
    _PyGCWalkCache *cache;
    uintptr_t top;              /* the stack base, exclusive */
    /* Until the eval loop is marked: a frame inside an activation's range
       is precise, except where the activations are scanned (an inline
       collection on its own thread). */
    struct _PyEvalActivation *activations;
    int activations_opaque;
    struct _PyGCRootStats *stats;
    int64_t test_fallbacks;     /* fallbacks at a pc in a test range */
} _PyGCWalk;

/* Walk outward from ctx, scanning what opaque frames own.  0 if the walk
 * reached the stack base or the outermost frame, 1 if it fell back to a
 * conservative scan of the rest. */
PyAPI_FUNC(int) _PyGC_WalkStack(_PyGCWalk *w, const _PyGCContext *ctx);

/* Flush the cache if the loader generation or the range table changed: in
 * each root phase, which counts the flushes since the last into stats, and
 * before a miss is decoded. */
extern void _PyGC_WalkCacheCheck(_PyGCWalkCache *cache,
                                 struct _PyGCRootStats *stats);
extern void _PyGC_WalkCacheFini(_PyGCWalkCache *cache);

/* Record that tstate detaches with its caller's context ctx (3.3). */
extern void _PyGC_RecordDetach(PyThreadState *tstate,
                               const _PyGCContext *ctx, int pinned);

/* The pinned detach entries for the core: the caller does not return
 * before the thread reattaches.  _PyEval_SaveThreadPinned() is in
 * Include/ceval.h, for Py_BEGIN_ALLOW_THREADS.  The *At functions are the
 * entries' bodies, called with the caller's context. */
PyAPI_FUNC(void) _PyEval_ReleaseThreadPinned(PyThreadState *tstate);
PyAPI_FUNC(void) _PyThreadState_DetachPinned(PyThreadState *tstate);
extern PyThreadState *_PyEval_SaveThreadAt(void *unused,
                                           const _PyGCContext *ctx,
                                           int pinned);
extern void _PyEval_ReleaseThreadAt(PyThreadState *tstate,
                                    const _PyGCContext *ctx, int pinned);
extern void _PyThreadState_DetachAt(PyThreadState *tstate,
                                    const _PyGCContext *ctx, int pinned);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_GC_WALK_H */
