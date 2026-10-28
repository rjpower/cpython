/* The mark phase of the tracing collector, run as a verifier.

   Reference counting still owns memory (note 06, step 4).  After a full
   collection, the verifier marks everything reachable from the roots and
   then walks every allocated object: an object with a nonzero, mortal
   reference count that was not marked is a "hole", a reference that a
   tracing collector would not see.  Each hole is either a missing root or a
   missing tp_traverse visit, and step 5 cannot let the collector free memory
   until there are none.  Nothing here frees, allocates Python objects, runs
   Python code or changes a reference count.

   Objects.  Every object lives in a thread state's mimalloc object heaps
   (Objects/obmalloc.c): one heap for non-GC objects, one for GC objects and
   one for GC objects with a pre-header, so the heap says where the object
   starts in its block.  The verifier visits those heaps and the abandoned
   segments of exited threads, and records every block as an object address
   and every page as an address range.

   Marking is iterative with an explicit mark stack.  Mark state is a hash
   set of object addresses on the side; the header is not touched.  An
   object is traversed with tp_traverse when its type has one (a type need
   not be GC to have one; such a traverse function is never called by the
   cycle collector), its attribute storage (shape slots or dict word) is
   visited when its type has one, and it is a leaf otherwise.  A static type is traversed with
   _PyType_TraverseStatic().

   Roots:

   - precise: the frames of every thread on the frame stack (all slots, the
     code, function, globals, builtins, locals and frame object), the thread
     states' object fields, the interpreter's object fields, the GC's lists,
     the registered static types and every immortal heap object;
   - conservative: every thread's C stack from its saved stack pointer to
     its base, its callee-saved registers, the writable segments of every
     loaded object, the thread and interpreter state structs and the thread
     data stacks.  A word is a root if it points into an allocated object
     block; an interior pointer marks its block.  A word on a C stack or in
     a register that points into a PyMem block (the tstate's mem heap) has
     the block scanned too, transitively, because a function's buffers are
     its locals.  PyMem blocks reached from objects or statics are not
     scanned: a type that keeps references there must visit them.

   Every pointer, precise or conservative, is checked before it is followed:
   it must be an allocated object's address or point to readable memory
   outside the heap, and its type must be a known type.  A conservative scan
   can find an object that is still being built, so an untracked GC object
   other than a tuple that only a conservative root reaches is marked but not
   traversed, until a precise reference reaches it.

   Unmarked objects are not all holes.  Garbage that a finalizer or another
   thread created during the collection is unreachable too.  As the cycle
   collector does, each unmarked object's references to other unmarked
   objects are subtracted from their counts; whatever keeps an external
   reference, and everything it reaches, is a hole, and the rest is counted
   as garbage. */

#include "Python.h"
#include "pycore_gc_trace.h"
#include "pycore_initconfig.h"    // _PyStatus_ERR()
#include "pycore_pystate.h"       // _PyInterpreterState_GET()
#include "pycore_shape.h"         // _PyShape_InitRoot()

#if defined(WITH_MIMALLOC)

#include "pycore_capsule.h"       // _PyCapsule_HasDestructor()
#include "pycore_code.h"          // _PyCode_HasExtraFree()
#include "pycore_crossinterp.h"   // _PyXIData_regitem_t
#include "pycore_dict.h"          // PyDictObject._ma_watcher_tag
#include "pycore_moduleobject.h"  // PyModuleObject
#include "pycore_frame.h"         // _PyInterpreterFrame
#include "pycore_gc.h"            // _PyObject_GC_IS_TRACKED()
#include "pycore_gc_walk.h"       // Py_GC_PRECISE, _PyGC_RegisterCodeRange()
#include "pycore_interp.h"        // _PyInterpreterState_GetFinalizing()
#include "pycore_interpframe.h"   // _PyFrame_Traverse()
#include "pycore_mimalloc.h"      // mi_heap_visit_blocks()
#include "pycore_memoryobject.h"  // _PyManagedBuffer_Type
#include "pycore_object.h"
#include "pycore_pymem.h"         // _PyMem_DebugEnabled()
#include "pycore_tstate.h"        // _PyThreadStateImpl
#include "pycore_typeobject.h"    // _PyType_TraverseStatic()

#include <setjmp.h>
#include <stdio.h>
#ifdef HAVE_DLFCN_H
#  include <dlfcn.h>              // dladdr()
#endif
#ifdef _Py_ADDRESS_SANITIZER
#  include <sanitizer/asan_interface.h>  // ASAN_POISON_MEMORY_REGION()
#endif
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_DL_ITERATE_PHDR
#  include <link.h>               // dl_iterate_phdr()
#endif
#ifdef HAVE_PTHREAD_H
#  include <pthread.h>            // pthread_getattr_np()
#endif
#ifdef MS_WINDOWS
#  include <process.h>            // _getpid()
#  define getpid _getpid
#else
#  include <unistd.h>             // getpid()
#endif

/* The verifier's own memory comes from the C library's malloc, never from
   mimalloc, so that verifying cannot change the heaps it walks. */


/* --- address sets ------------------------------------------------------ */

typedef struct {
    uintptr_t *keys;     // 0 marks an empty slot
    size_t mask;         // capacity - 1
    size_t used;
} addrset;

static inline size_t
addr_hash(uintptr_t k)
{
    uint64_t h = (uint64_t)(k >> 3) * 0x9E3779B97F4A7C15ULL;
    return (size_t)(h ^ (h >> 31));
}

static int
addrset_init(addrset *s, size_t hint)
{
    size_t cap = 1024;
    while (cap < hint * 2) {
        cap <<= 1;
    }
    s->keys = calloc(cap, sizeof(uintptr_t));
    if (s->keys == NULL) {
        return -1;
    }
    s->mask = cap - 1;
    s->used = 0;
    return 0;
}

static void
addrset_fini(addrset *s)
{
    free(s->keys);
    s->keys = NULL;
    s->used = 0;
}

static void
addrset_insert_new(uintptr_t *keys, size_t mask, uintptr_t k)
{
    size_t i = addr_hash(k) & mask;
    while (keys[i] != 0) {
        i = (i + 1) & mask;
    }
    keys[i] = k;
}

static int
addrset_grow(addrset *s)
{
    size_t cap = (s->mask + 1) * 2;
    uintptr_t *keys = calloc(cap, sizeof(uintptr_t));
    if (keys == NULL) {
        return -1;
    }
    for (size_t i = 0; i <= s->mask; i++) {
        if (s->keys[i] != 0) {
            addrset_insert_new(keys, cap - 1, s->keys[i]);
        }
    }
    free(s->keys);
    s->keys = keys;
    s->mask = cap - 1;
    return 0;
}

/* Return 1 if k was added, 0 if it was present, -1 on memory error. */
static int
addrset_add(addrset *s, uintptr_t k)
{
    assert(k != 0);
    if ((s->used + 1) * 2 > s->mask + 1 && addrset_grow(s) < 0) {
        return -1;
    }
    size_t i = addr_hash(k) & s->mask;
    while (s->keys[i] != 0) {
        if (s->keys[i] == k) {
            return 0;
        }
        i = (i + 1) & s->mask;
    }
    s->keys[i] = k;
    s->used++;
    return 1;
}

static int
addrset_has(const addrset *s, uintptr_t k)
{
    if (s->keys == NULL) {
        return 0;
    }
    size_t i = addr_hash(k) & s->mask;
    while (s->keys[i] != 0) {
        if (s->keys[i] == k) {
            return 1;
        }
        i = (i + 1) & s->mask;
    }
    return 0;
}


/* --- address ranges ---------------------------------------------------- */

typedef struct {
    uintptr_t start;
    uintptr_t end;
    // For heap pages: the block size and the object's offset in its block.
    size_t bsize;
    size_t offset;
} range_t;

typedef struct {
    range_t *items;
    size_t n;
    size_t cap;
} rangelist;

static int
rangelist_add(rangelist *l, uintptr_t start, uintptr_t end,
              size_t bsize, size_t offset)
{
    if (l->n == l->cap) {
        size_t cap = l->cap ? l->cap * 2 : 256;
        range_t *items = realloc(l->items, cap * sizeof(range_t));
        if (items == NULL) {
            return -1;
        }
        l->items = items;
        l->cap = cap;
    }
    l->items[l->n++] = (range_t){start, end, bsize, offset};
    return 0;
}

static int
range_cmp(const void *a, const void *b)
{
    uintptr_t x = ((const range_t *)a)->start;
    uintptr_t y = ((const range_t *)b)->start;
    return (x > y) - (x < y);
}

static void
rangelist_sort(rangelist *l)
{
    if (l->n > 1) {
        qsort(l->items, l->n, sizeof(range_t), range_cmp);
    }
}

/* Find the range containing addr in a sorted, non-overlapping list. */
static const range_t *
rangelist_find(const rangelist *l, uintptr_t addr)
{
    size_t lo = 0, hi = l->n;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        const range_t *r = &l->items[mid];
        if (addr < r->start) {
            hi = mid;
        }
        else if (addr >= r->end) {
            lo = mid + 1;
        }
        else {
            return r;
        }
    }
    return NULL;
}

static void
rangelist_fini(rangelist *l)
{
    free(l->items);
    l->items = NULL;
    l->n = l->cap = 0;
}


/* --- the verifier's state ---------------------------------------------- */

typedef struct {
    uintptr_t addr;
    Py_ssize_t refcnt;
    char type_name[80];
    char value[48];       // the start of an ASCII str, to identify it
    char referrer[160];
    int has_referrer;
    Py_ssize_t external;  // references from outside the unmarked objects
    int held;             // reachable from an unmarked object with one
    uintptr_t block;      // the non-object block that refers to it, or 0
    size_t block_size;
} trace_hole;

/* The live verifier's modes (PYTHON_GCVERIFY=retain|kill[:pymem,structs]).
   A leaves out the words of precise frames, which the collector no longer
   scans (rung 1's claim), and with pymem and structs also the PyMem blocks
   that words of core opaque frames point to and the struct scan (rung 3's,
   note 11 decision 5).  An object B reached comes from a word of a precise
   frame, a word of a PyMem block that a core opaque frame points to, or a
   word of a thread or interpreter struct. */
#define VERIFY_OFF      0
#define VERIFY_RETAIN   1
#define VERIFY_KILL     2
#define VERIFY_EX_PYMEM   1
#define VERIFY_EX_STRUCTS 2
#define VERIFY_FROM_PRECISE   1
#define VERIFY_FROM_CORE_MEM  2
#define VERIFY_FROM_STRUCT    3

typedef struct {
    uintptr_t word;       // the root word's address
    uintptr_t owner;      // its frame's pc, or 0
    int kind;             // VERIFY_FROM_*
    int64_t collection;   // the live collection that listed it
} verify_rec;

/* The words kept for an object that B listed: B's first, the one it
   marked the object from, then each other left-out word whose value is the
   object, in the order A listed them.  Which word comes first depends on
   the stack's layout, so a report names them all. */
#define VERIFY_RECS 8

typedef struct {
    int n;                // the words kept, at most VERIFY_RECS
    int more;             // the words past VERIFY_RECS, counted
    verify_rec rec[VERIFY_RECS];
} verify_recs;

typedef struct {
    PyObject *op;
    verify_recs recs;
} verify_entry;

/* What kill keeps of an object it killed, for the trap's report: the
   object's type name is copied, since the type may die with it. */
typedef struct {
    PyObject *op;
    verify_recs recs;
    char type[48];
} verify_killed;

struct _gc_trace_state {
    // The live verifier: its mode, the live collections it has seen, and
    // the objects that only the roots outside A reached, which retain
    // keeps (sorted by address).
    int live_verify;
    int live_verify_ex;   // VERIFY_EX_*
    int64_t live_collections;
    verify_entry *q;
    size_t nq;
    // The objects that the last collection's kill killed, sorted by
    // address, with their records
    verify_killed *killed;
    size_t nkilled;
    // Kill's quarantine: blocks freed for the objects it killed, and the
    // collection that killed them.  An object's own block keeps the
    // object's record until it is freed.
    struct {
        void *block;
        int64_t collection;
        int has_rec;
        verify_killed rec;
    } *quarantine;
    size_t nquarantine;
    size_t quarantine_cap;
    // The static types passed to PyType_Ready() in this interpreter.
    PyTypeObject **static_types;
    Py_ssize_t n_static_types;
    Py_ssize_t static_types_cap;
    addrset static_type_set;
    // Set by _PyGC_TraceVerify() to verify the collection it runs.
    int force;
    int verifying;
    // The last verification: its holes, and their number or -1 if it
    // could not run.
    trace_hole *holes;
    Py_ssize_t nholes;
    Py_ssize_t last_result;
    Py_ssize_t count;     // the verifications that ran
    // PYTHON_GCSTACKHITS=1: each object that a live mark reaches first
    // from a conservatively scanned stack or register word, with the word
    // and the frame the walk gives it to (_PyGC_TraceStackHits())
    int stack_hits_on;
    int64_t stack_hits_collection;
    _PyGCStackHit *stack_hits;
    size_t nstack_hits;
    size_t stack_hits_cap;
    int64_t stack_hits_dropped;
};

typedef struct {
    PyInterpreterState *interp;
    struct _gc_trace_state *ts;
    unsigned int roots;
    size_t debug_offset;

    addrset objects;      // every block of the object heaps, as its object
    rangelist pages;      // the object heaps' pages
    uintptr_t heap_lo;    // the bounds of the pages
    uintptr_t heap_hi;
    rangelist readable;   // readable mappings of the process
    rangelist mem_pages;  // the mem heaps' pages, for the referrer search
    addrset marked;
    addrset shallow;      // marked without traversal
    addrset deepened;     // shallow objects later traversed
    addrset good_types;   // types already checked

    PyObject **stack;
    size_t sp;
    size_t stack_cap;

    int conservative;     // the root being scanned is conservative
    int scan_mem;         // follow words into mem heap blocks
    int scan_class;       // _PyGC_SCAN_* of the words being scanned
    _PyGCRootStats *stats;  // the live mark's counters, or NULL
    _PyGCWalkCache *walk_cache;  // the stack walk's descriptors, or NULL
    addrset mem_scanned;  // mem blocks queued for scanning
    uintptr_t *mem_stack; // (start, end, class) of blocks to scan
    size_t mem_sp;
    size_t mem_cap;
    Py_ssize_t nmemblocks;
    int oom;
    Py_ssize_t nobjects;
    Py_ssize_t nmarked;
    Py_ssize_t nshallow;
    Py_ssize_t nrejected; // precise references that were not objects
    Py_ssize_t ngarbage;  // unmarked objects that only garbage refers to
    // Precise references into the heap that are not live
    // objects, which means the collector freed something reachable.
    PyObject *current;    // the object being traversed
    Py_ssize_t ndangling;
    struct {
        uintptr_t addr;
        char referrer[80];
    } dangling[16];
    // The live collector: mark bits in the headers instead of the sets.
    int live;
    uintptr_t mem_lo;     // the bounds of mem_pages
    uintptr_t mem_hi;
    PyInterpreterState *domain[64];
    int ndomain;
    mi_subproc_t *subproc[64];  // the domain's mimalloc sub-processes
    // The bounds of the segment map when the mark began.  A segment entered
    // since can hold no object: allocating one needs the GIL.
    uintptr_t seg_lo;
    uintptr_t seg_span;
    // The offset of the object in a block of each heap tag; -1 for the mem
    // heap, which holds no objects.
    Py_ssize_t tag_offset[_Py_MIMALLOC_HEAP_COUNT];
    // Weak caches and lists that the conservative scans skip
    uintptr_t skip_lo[256];
    uintptr_t skip_hi[256];
    int nskip;
    /* The live verifier (work/10a-1.md item 7).  A, the mark from the
       roots, leaves out the struct scan and the PyMem blocks that words of
       core opaque frames point to, which it lists in deferred; B marks from
       them after A, and lists each object it marks in newq. */
    int verify;
    int verify_ex;
    int phase_b;
    int frame_core;       // the walk is in a core opaque frame
    uintptr_t frame_pc;   // and the frame's pc, sp and cfa
    uintptr_t frame_sp;
    uintptr_t frame_cfa;
    // The stack or register word being marked from, for the stack hits
    uintptr_t hit_word;
    uintptr_t hit_value;
    int in_queue;         // scan_queued() is draining
    verify_rec rec;       // B's root word
    struct { uintptr_t word, value, pc; int kind; } *deferred;
    size_t ndeferred;
    size_t deferred_cap;
    verify_entry *newq;
    size_t nnewq;
    size_t newq_cap;
    addrset killed;       // kill: the objects that B listed, which die
} trace_t;

static int mark_visit(PyObject *op, void *arg);
static int live_visit(PyObject *op, void *arg);
static void live_scan_words(trace_t *t, uintptr_t start, uintptr_t stop);
static void verify_list(trace_t *t, PyObject *op);
static void verify_defer(trace_t *t, uintptr_t word, uintptr_t value,
                         int kind);
static void verify_defer_range(trace_t *t, uintptr_t start, uintptr_t end,
                               int kind);
static void scan_threads_only(trace_t *t, PyThreadState *current);


/* --- the heaps --------------------------------------------------------- */

struct walk_args {
    trace_t *t;
    size_t offset;
    rangelist *pages;
    addrset *objects;
};

static bool
walk_block(const mi_heap_t *heap, const mi_heap_area_t *area,
           void *block, size_t block_size, void *arg)
{
    struct walk_args *a = arg;
    if (block == NULL) {
        if (area->used == 0) {
            return true;
        }
        uintptr_t start = (uintptr_t)area->blocks;
        if (rangelist_add(a->pages, start, start + area->reserved,
                          area->full_block_size, a->offset) < 0) {
            a->t->oom = 1;
            return false;
        }
        return true;
    }
    if (a->objects != NULL) {
        uintptr_t op = (uintptr_t)block + a->offset;
        // A block without its object map bit is not an initialized object.
        if (!_PyGC_IsHeapObject((PyObject *)op)) {
            return true;
        }
        if (addrset_add(a->objects, op) < 0) {
            a->t->oom = 1;
            return false;
        }
        a->t->nobjects++;
    }
    return true;
}


/* Visit the object heaps of every thread state and of the exited threads,
   and the mem heaps' pages.  The other threads cannot allocate: they do not
   hold the GIL. */
static int
walk_heaps(trace_t *t)
{
    static const int tags[] = {
        _Py_MIMALLOC_HEAP_OBJECT, _Py_MIMALLOC_HEAP_GC,
    };
    PyInterpreterState *interp = t->interp;
    struct walk_args a = {.t = t};
    int ok = 1;
    HEAD_LOCK(&_PyRuntime);
    _Py_FOR_EACH_TSTATE_UNLOCKED(interp, p) {
        struct _mimalloc_thread_state *m = &((_PyThreadStateImpl *)p)->mimalloc;
        if (m->current_object_heap == NULL) {
            continue;  // not bound yet
        }
        for (size_t i = 0; ok && i < Py_ARRAY_LENGTH(tags); i++) {
            a.offset = t->debug_offset;
            a.pages = &t->pages;
            a.objects = &t->objects;
            ok = mi_heap_visit_blocks(&m->heaps[tags[i]], true, walk_block, &a);
        }
        if (ok) {
            a.offset = 0;
            a.pages = &t->mem_pages;
            a.objects = NULL;
            ok = mi_heap_visit_blocks(&m->heaps[_Py_MIMALLOC_HEAP_MEM], false,
                                      walk_block, &a);
        }
    }
    mi_subproc_t *subproc = interp->mimalloc.subproc;
    for (size_t i = 0; ok && i < Py_ARRAY_LENGTH(tags); i++) {
        a.offset = t->debug_offset;
        a.pages = &t->pages;
        a.objects = &t->objects;
        ok = mi_abandoned_visit_blocks(subproc, tags[i], true, walk_block, &a);
    }
    if (ok) {
        a.offset = 0;
        a.pages = &t->mem_pages;
        a.objects = NULL;
        ok = mi_abandoned_visit_blocks(subproc, _Py_MIMALLOC_HEAP_MEM, false,
                                       walk_block, &a);
    }
    HEAD_UNLOCK(&_PyRuntime);
    if (!ok || t->oom) {
        return -1;
    }
    rangelist_sort(&t->pages);
    rangelist_sort(&t->mem_pages);
    if (t->pages.n > 0) {
        t->heap_lo = t->pages.items[0].start;
        for (size_t i = 0; i < t->pages.n; i++) {
            t->heap_hi = Py_MAX(t->heap_hi, t->pages.items[i].end);
        }
    }
    return 0;
}

/* Queue [start, end) for a conservative scan, counted as class cls. */
static void
queue_range(trace_t *t, uintptr_t start, uintptr_t end, int cls)
{
    if (t->mem_sp + 3 > t->mem_cap) {
        size_t cap = t->mem_cap ? t->mem_cap * 2 : 256;
        uintptr_t *stack = realloc(t->mem_stack, cap * sizeof(uintptr_t));
        if (stack == NULL) {
            t->oom = 1;
            return;
        }
        t->mem_stack = stack;
        t->mem_cap = cap;
    }
    t->mem_stack[t->mem_sp++] = start;
    t->mem_stack[t->mem_sp++] = end;
    t->mem_stack[t->mem_sp++] = (uintptr_t)cls;
}

/* The functions that read words conservatively, and the free-list check
   that reads freed blocks, are not instrumented under ASan: their reads of
   redzones and of freed, poisoned blocks are by design. */
static int _Py_NO_SANITIZE_ADDRESS
block_in_list(const mi_page_t *page, mi_block_t *list, uintptr_t block)
{
    for (mi_block_t *b = list; b != NULL; b = mi_block_next(page, b)) {
        if ((uintptr_t)b == block) {
            return 1;
        }
    }
    return 0;
}

/* Is the mem heap block free?  A stale word on a stack can point into a
   block freed since; its contents are what it held when it was in use, and
   scanning them would keep that garbage alive.  The lists are short next to
   the work of a collection, and only blocks the conservative scan reaches
   are checked. */
static int _Py_NO_SANITIZE_ADDRESS
mem_block_is_free(uintptr_t block)
{
    mi_page_t *page = _mi_ptr_page((void *)block);
    uint8_t *start = mi_page_start(page);
    if (block >= (uintptr_t)start + page->capacity * mi_page_block_size(page)) {
        return 1;  // never handed out
    }
    return block_in_list(page, page->free, block)
        || block_in_list(page, page->local_free, block)
        || block_in_list(page, mi_page_thread_free(page), block);
}

/* Queue the mem heap block that contains addr for a conservative scan. */
static void
queue_mem_block(trace_t *t, uintptr_t addr)
{
    const range_t *r = rangelist_find(&t->mem_pages, addr);
    if (r == NULL) {
        return;
    }
    uintptr_t block = r->start + (addr - r->start) / r->bsize * r->bsize;
    // Only the live collector checks, which keeps the verifier's results
    // comparable with step 4's.
    if (t->live && mem_block_is_free(block)) {
        return;
    }
    int added = addrset_add(&t->mem_scanned, block);
    if (added <= 0) {
        t->oom |= added < 0;
        return;
    }
    queue_range(t, block, block + r->bsize, _PyGC_SCAN_MEM);
    t->nmemblocks++;
}

/* Queue the block of an object that is marked without being traversed. */
static void
queue_object_block(trace_t *t, uintptr_t addr)
{
    const range_t *r = rangelist_find(&t->pages, addr);
    if (r == NULL) {
        return;
    }
    uintptr_t block = r->start + (addr - r->start) / r->bsize * r->bsize;
    queue_range(t, block, block + r->bsize, _PyGC_SCAN_SHALLOW);
}

/* Map an address anywhere in an object block to the object, or 0. */
static uintptr_t
heap_lookup(const trace_t *t, uintptr_t addr)
{
    if (addr < t->heap_lo || addr >= t->heap_hi) {
        return 0;
    }
    const range_t *r = rangelist_find(&t->pages, addr);
    if (r == NULL) {
        return 0;
    }
    uintptr_t block = r->start + (addr - r->start) / r->bsize * r->bsize;
    uintptr_t op = block + r->offset;
    if (!addrset_has(&t->objects, op)) {
        return 0;
    }
    return op;
}


/* --- readable memory ------------------------------------------------------ */

/* Read the process's readable mappings, so that no scan touches unmapped
   memory: a thread's recorded stack can belong to a thread that has exited. */
static int
read_mappings(trace_t *t)
{
    FILE *f = fopen("/proc/self/maps", "r");
    if (f == NULL) {
        return -1;
    }
    char line[512];
    while (fgets(line, sizeof(line), f) != NULL) {
        unsigned long start, end;
        char perms[8];
        if (sscanf(line, "%lx-%lx %7s", &start, &end, perms) != 3) {
            continue;
        }
        if (perms[0] != 'r') {
            continue;
        }
        // Skip [vvar] and device mappings, which can fault on reading.
        if (strstr(line, "[vvar") != NULL || strstr(line, "/dev/") != NULL) {
            continue;
        }
        if (rangelist_add(&t->readable, start, end, 0, 0) < 0) {
            fclose(f);
            return -1;
        }
    }
    fclose(f);
    rangelist_sort(&t->readable);
    return 0;
}

static int
is_readable(const trace_t *t, uintptr_t addr, size_t size)
{
    const range_t *r = rangelist_find(&t->readable, addr);
    return r != NULL && addr + size <= r->end;
}


/* --- object and type checks ------------------------------------------------- */

static int
is_static_type(const trace_t *t, uintptr_t addr)
{
    return addrset_has(&t->ts->static_type_set, addr);
}

/* Is tp a type object?  A registered static type, a heap type whose
   metatype is a type, or a ready type outside the heap whose type is a
   type (a static type readied in another interpreter). */
static int
is_type(trace_t *t, PyTypeObject *tp, int depth)
{
    uintptr_t addr = (uintptr_t)tp;
    if (addr == 0 || (addr & (sizeof(void *) - 1)) != 0) {
        return 0;
    }
    if (addrset_has(&t->good_types, addr) || is_static_type(t, addr)) {
        return 1;
    }
    if (depth > 4) {
        return 0;
    }
    int in_heap = addrset_has(&t->objects, addr);
    if (!in_heap && !is_readable(t, addr, sizeof(PyTypeObject))) {
        return 0;
    }
    PyTypeObject *meta = Py_TYPE((PyObject *)tp);
    if (meta != &PyType_Type && !is_type(t, meta, depth + 1)) {
        return 0;
    }
    if (!(meta->tp_flags & Py_TPFLAGS_TYPE_SUBCLASS)
        && meta != &PyType_Type) {
        return 0;
    }
    unsigned long flags = tp->tp_flags;
    if (!(flags & Py_TPFLAGS_READY)) {
        return 0;
    }
    if (in_heap != ((flags & Py_TPFLAGS_HEAPTYPE) != 0)) {
        return 0;
    }
    if (addrset_add(&t->good_types, addr) < 0) {
        t->oom = 1;
    }
    return 1;
}

/* Is op the address of an object?  If heap is set it is known to be the
   address of an object block. */
static int
is_object(trace_t *t, PyObject *op, int heap)
{
    uintptr_t addr = (uintptr_t)op;
    if (!heap) {
        if ((addr & (sizeof(void *) - 1)) != 0) {
            return 0;
        }
        if (!is_readable(t, addr, sizeof(PyObject))) {
            return 0;
        }
    }
    return is_type(t, Py_TYPE(op), 0);
}


/* --- marking ------------------------------------------------------------- */

static void
push(trace_t *t, PyObject *op)
{
    if (t->sp == t->stack_cap) {
        size_t cap = t->stack_cap ? t->stack_cap * 2 : 4096;
        PyObject **stack = realloc(t->stack, cap * sizeof(PyObject *));
        if (stack == NULL) {
            t->oom = 1;
            return;
        }
        t->stack = stack;
        t->stack_cap = cap;
    }
    t->stack[t->sp++] = op;
}

/* Can a conservative root's object be traversed?  An untracked GC object
   may be under construction, with fields not yet initialized; tuples are
   always initialized and are untracked when they hold only atomic
   objects. */
static int
safe_to_traverse(PyObject *op)
{
    if (!_PyObject_IS_GC(op) || _PyObject_GC_IS_TRACKED(op)) {
        return 1;
    }
    return PyTuple_Check(op);
}

static void
mark(trace_t *t, PyObject *op, int heap)
{
    uintptr_t addr = (uintptr_t)op;
    if (!heap) {
        heap = addrset_has(&t->objects, addr);
    }
    if (!heap && heap_lookup(t, addr) != 0) {
        // A pointer into an object other than its start
        if (!t->conservative) {
            t->nrejected++;
        }
        return;
    }
    if (!heap && !t->conservative && addr >= t->heap_lo && addr < t->heap_hi
        && rangelist_find(&t->pages, addr) != NULL)
    {
        // A precise reference into an object page, to a block that is free
        // or holds no object: the collector freed a reachable object.
        if (t->ndangling < (Py_ssize_t)Py_ARRAY_LENGTH(t->dangling)) {
            t->dangling[t->ndangling].addr = addr;
            PyOS_snprintf(t->dangling[t->ndangling].referrer,
                          sizeof(t->dangling[0].referrer), "%.60s at %p",
                          t->current ? Py_TYPE(t->current)->tp_name : "a root",
                          (void *)t->current);
        }
        t->ndangling++;
        t->nrejected++;
        return;
    }
    if (!is_object(t, op, heap)) {
        if (!t->conservative) {
            t->nrejected++;
        }
        return;
    }
    if (t->conservative && !safe_to_traverse(op)) {
        int r = addrset_add(&t->marked, addr);
        if (r < 0) {
            t->oom = 1;
        }
        else if (r > 0) {
            t->nmarked++;
            t->nshallow++;
            if (addrset_add(&t->shallow, addr) < 0) {
                t->oom = 1;
            }
            /* Its fields cannot be trusted as objects, but its words can
               be scanned: a set or dict being filled holds its members
               in a table it owns. */
            queue_object_block(t, addr);
        }
        return;
    }
    int r = addrset_add(&t->marked, addr);
    if (r < 0) {
        t->oom = 1;
        return;
    }
    if (r > 0) {
        t->nmarked++;
        push(t, op);
    }
    else if (!t->conservative && addrset_has(&t->shallow, addr)) {
        // A precise reference to an object marked without traversal
        r = addrset_add(&t->deepened, addr);
        if (r < 0) {
            t->oom = 1;
        }
        else if (r > 0) {
            push(t, op);
        }
    }
}

static int
mark_visit(PyObject *op, void *arg)
{
    if (((trace_t *)arg)->live) {
        return live_visit(op, arg);
    }
    if (op != NULL) {
        mark((trace_t *)arg, op, 0);
    }
    return 0;
}

int
_PyGC_TraceTraverse(PyObject *op, visitproc visit, void *arg)
{
    PyTypeObject *tp = Py_TYPE(op);
    if (PyType_Check(op)
        && !(((PyTypeObject *)op)->tp_flags & Py_TPFLAGS_HEAPTYPE))
    {
        return _PyType_TraverseStatic((PyTypeObject *)op, visit, arg);
    }
    /* An instance owns a reference to its heap type.  The traverse function
       of a GC type visits it; other types have none or do not. */
    if ((tp->tp_flags & Py_TPFLAGS_HEAPTYPE) && !_PyType_IS_GC(tp)) {
        Py_VISIT(tp);
    }
    /* The runtime lays out an instance's attribute storage, the shape slots
       or the dict word, so the collector marks it; a type's traverse covers
       only what the type lays out.  Under reference counting a traverse that
       omitted the storage only missed cycles; here it would free live
       attributes.  A traverse that also visits it is harmless. */
    if ((tp->tp_flags & Py_TPFLAGS_INLINE_VALUES) || _PyGC_HasDictWord(tp)) {
        int err = PyObject_VisitManagedDict(op, visit, arg);
        if (err) {
            return err;
        }
    }
    if (tp->tp_traverse != NULL) {
        return tp->tp_traverse(op, visit, arg);
    }
    return 0;
}

/* Empty the mark stack.  Everything on it was reached precisely, or is safe
   to traverse. */
static void
drain(trace_t *t)
{
    int conservative = t->conservative;
    t->conservative = 0;
    while (t->sp > 0 && !t->oom) {
        PyObject *op = t->stack[--t->sp];
        t->current = op;
        mark(t, (PyObject *)Py_TYPE(op), 0);
        (void)_PyGC_TraceTraverse(op, mark_visit, t);
    }
    t->current = NULL;
    t->conservative = conservative;
}

static void
mark_root(trace_t *t, PyObject *op)
{
    if (op != NULL) {
        mark(t, op, 0);
        drain(t);
    }
}

/* Scan a range of words conservatively. */
static void _Py_NO_SANITIZE_ADDRESS
scan_range(trace_t *t, uintptr_t start, uintptr_t end)
{
    start = _Py_SIZE_ROUND_UP(start, sizeof(uintptr_t));
    for (int i = 0; i < t->nskip; i++) {
        if (start < t->skip_hi[i] && end > t->skip_lo[i]) {
            if (start < t->skip_lo[i]) {
                scan_range(t, start, t->skip_lo[i]);
            }
            if (end > t->skip_hi[i]) {
                scan_range(t, t->skip_hi[i], end);
            }
            return;
        }
    }
    // With one thread, every range the live collector scans is mapped, and
    // it does not read the mappings.
    if (t->live && t->readable.n == 0) {
        live_scan_words(t, start, end);
        return;
    }
    while (start < end) {
        // Clip to readable memory.
        const range_t *r = rangelist_find(&t->readable, start);
        if (r == NULL) {
            // Skip to the next readable mapping.
            size_t lo = 0, hi = t->readable.n;
            while (lo < hi) {
                size_t mid = lo + (hi - lo) / 2;
                if (t->readable.items[mid].start <= start) {
                    lo = mid + 1;
                }
                else {
                    hi = mid;
                }
            }
            if (lo == t->readable.n || t->readable.items[lo].start >= end) {
                return;
            }
            start = t->readable.items[lo].start;
            continue;
        }
        uintptr_t stop = Py_MIN(end, r->end);
        if (t->live) {
            live_scan_words(t, start, stop);
            start = stop;
            continue;
        }
        int conservative = t->conservative;
        t->conservative = 1;
        for (uintptr_t *p = (uintptr_t *)start; (uintptr_t)p + sizeof(uintptr_t) <= stop; p++) {
            uintptr_t w = *p;
            if (w < 4096) {
                continue;
            }
            uintptr_t op = heap_lookup(t, w);
            if (op != 0) {
                mark(t, (PyObject *)op, 1);
                drain(t);
                t->conservative = 1;
            }
            else if (t->scan_mem) {
                queue_mem_block(t, w);
            }
        }
        t->conservative = conservative;
        start = stop;
    }
}


/* Scan the queued blocks: the mem blocks that C stacks and registers
   point into, and the blocks of objects marked without traversal.  The
   mem blocks these point into are scanned too. */
static void
scan_queued(trace_t *t)
{
    int scan_mem = t->scan_mem;
    int scan_class = t->scan_class;
    t->scan_mem = 1;
    t->in_queue++;
    while (t->mem_sp > 0 && !t->oom) {
        t->scan_class = (int)t->mem_stack[--t->mem_sp];
        uintptr_t end = t->mem_stack[--t->mem_sp];
        uintptr_t start = t->mem_stack[--t->mem_sp];
        scan_range(t, start, end);
    }
    t->in_queue--;
    t->scan_mem = scan_mem;
    t->scan_class = scan_class;
}

/* --- conservative roots ----------------------------------------------------- */

static uintptr_t
stack_top(PyThreadState *p)
{
    _PyThreadStateImpl *ts = (_PyThreadStateImpl *)p;
    return ts->c_stack_top;
}

/* Scan [start, top) of a C stack conservatively: the stack of a thread
   that the walk does not read, or of a platform without the walk (note 11
   section 3.4). */
static void
scan_stack(trace_t *t, uintptr_t start, uintptr_t top)
{
    if (t->stats != NULL) {
        t->stats->threads++;
    }
    t->scan_class = _PyGC_SCAN_STACK;
    if (top > start) {
        scan_range(t, start, top);
    }
}

#ifdef _PyGC_WALK
static void
walk_scan(void *arg, uintptr_t start, uintptr_t end)
{
    scan_range((trace_t *)arg, start, end);
}

/* The frame whose words the walk scans next, for the live verifier */
static void
walk_frame(void *arg, uintptr_t pc, uintptr_t sp, uintptr_t cfa, int cls)
{
    trace_t *t = arg;
    t->frame_core = (t->verify_ex & VERIFY_EX_PYMEM) && cls == _PyGC_FRAME_CORE;
    t->frame_pc = pc;
    t->frame_sp = sp;
    t->frame_cfa = cfa;
}

/* The words of a precise frame, which A leaves out */
static void
walk_precise(void *arg, uintptr_t start, uintptr_t end)
{
    verify_defer_range((trace_t *)arg, start, end, VERIFY_FROM_PRECISE);
}
#endif

/* The registers that ctx captured, conservatively.  Scans a copy: the
   main thread's records are in _PyRuntime, in a range that the data scan
   skips (live_skip_caches()), and scan_range() would skip them here too. */
static void
scan_regs(trace_t *t, const _PyGCContext *ctx)
{
    uintptr_t regs[_PyGC_NSAVED];
    memcpy(regs, ctx->regs, sizeof(regs));
    t->scan_class = _PyGC_SCAN_REGS;
    scan_range(t, (uintptr_t)regs, (uintptr_t)(regs + _PyGC_NSAVED));
}

/* Scan p's stack outward from ctx up to top: walked where the walk is on
   (Python/gc_walk.c), so that only what opaque frames own is scanned;
   elsewhere the registers and the whole stack conservatively. */
static void
scan_from(trace_t *t, const _PyGCContext *ctx, uintptr_t top)
{
#ifdef _PyGC_WALK
    if (t->stats != NULL) {
        t->stats->threads++;
    }
    t->scan_class = _PyGC_SCAN_STACK;
    _PyGCWalk w = {
        .scan = walk_scan,
        .frame = walk_frame,
        .precise = t->verify ? walk_precise : NULL,
        .arg = t,
        .cache = t->walk_cache,
        .top = top,
        .stats = t->stats,
    };
    (void)_PyGC_WalkStack(&w, ctx);
    t->frame_core = 0;
#else
    scan_regs(t, ctx);
    scan_stack(t, ctx->sp, top);
#endif
}

/* This thread's stack and registers.  Not inlined, so that its frame and
   everything below the caller's are on the stack it scans. */
static Py_NO_INLINE void
scan_current_thread(trace_t *t, PyThreadState *tstate)
{
    // Not in this frame, which is opaque: its words are scanned, and the
    // context holds the values of registers that precise frames own.
    static _Py_thread_local _PyGCContext ctx;
    _PyGC_CaptureCaller(&ctx);
    uintptr_t top = stack_top(tstate);
    uintptr_t here = ctx.sp;
#if defined(HAVE_PTHREAD_GETATTR_NP) && defined(__GLIBC__)
    if (top == 0 || top < here) {
        pthread_attr_t attr;
        void *addr;
        size_t size;
        if (pthread_getattr_np(pthread_self(), &attr) == 0) {
            if (pthread_attr_getstack(&attr, &addr, &size) == 0) {
                top = (uintptr_t)addr + size;
            }
            pthread_attr_destroy(&attr);
        }
    }
#endif
    scan_from(t, &ctx, top);
}

/* C code keeps references in PyMem memory that only its stack frame
   points to: a PyUnicodeWriter's buffer, the keys list.sort() computes, a
   long vectorcall's argument array.  The blocks that the stacks and
   registers point into are scanned conservatively too, and the blocks
   they point into in turn.  Memory that only objects or static variables
   point to is not: an object must visit what it holds there. */
static void
scan_threads(trace_t *t, PyThreadState *current)
{
    t->scan_mem = 1;
    scan_threads_only(t, current);
    t->scan_mem = 0;
    scan_queued(t);
}

/* The stack and registers of another thread, as it recorded them when it
   detached (_PyGC_RecordDetach(), note 11 question 16).  A pinned record's
   frames from the capture outward persist while the thread is detached:
   the walk from the capture.  Of an unpinned one, which may have returned
   since, of a thread that has exited, or without the walk: the registers
   and the stack from the capture up, conservatively, clipped to the
   readable mappings.  Frames pushed below the capture after the detach
   are not scanned: what the thread holds it obtained attached. */
static void
scan_other_thread(trace_t *t, PyThreadState *p, int alive)
{
    _PyThreadStateImpl *ts = (_PyThreadStateImpl *)p;
    _PyGCDetachRecord *rec = &ts->gc_trace.detach;
    if (!rec->valid) {
        return;  // never attached
    }
    uintptr_t top = stack_top(p);
#ifdef _PyGC_WALK
    if (alive && rec->pinned && rec->cap.sp < top
        && (t->readable.n == 0
            || is_readable(t, rec->cap.sp, top - rec->cap.sp)))
    {
        scan_from(t, &rec->cap, top);
        return;
    }
#endif
    if (t->stats != NULL && !rec->pinned) {
        t->stats->unpinned++;
    }
    scan_regs(t, &rec->cap);
    scan_stack(t, rec->cap.sp, top);
}

/* The thread states that finalization unlinked, whose daemon threads may
   still run C code and whose frames remain (_PyGC_LiveKeepThreads()). */
static PyThreadState *
kept_threads(PyInterpreterState *interp)
{
    return _PyGC_LiveKeptThreads(interp);
}

static void
scan_threads_only(trace_t *t, PyThreadState *current)
{
    scan_current_thread(t, current);
    _Py_FOR_EACH_TSTATE_UNLOCKED(t->interp, p) {
        if (p != current) {
            scan_other_thread(t, p, 1);
        }
    }
}

#ifdef HAVE_DL_ITERATE_PHDR
static int
scan_object_segments(struct dl_phdr_info *info, size_t size, void *arg)
{
    trace_t *t = arg;
    for (int i = 0; i < info->dlpi_phnum; i++) {
        const ElfW(Phdr) *ph = &info->dlpi_phdr[i];
        if (ph->p_type != PT_LOAD || !(ph->p_flags & PF_W)) {
            continue;
        }
        uintptr_t start = info->dlpi_addr + ph->p_vaddr;
        scan_range(t, start, start + ph->p_memsz);
    }
    return 0;
}
#endif

static void
scan_data_segments(trace_t *t)
{
#ifdef HAVE_DL_ITERATE_PHDR
    dl_iterate_phdr(scan_object_segments, t);
#endif
}

/* Scan the struct at p conservatively, except for the list head at head:
 * the list of the asyncio module's C tasks, which is weak, since each task
 * unlinks itself as it dies. */
static void
scan_struct_except(trace_t *t, void *p, size_t size, struct llist_node *head)
{
    scan_range(t, (uintptr_t)p, (uintptr_t)head);
    scan_range(t, (uintptr_t)(head + 1), (uintptr_t)p + size);
}

static void
scan_thread_struct(trace_t *t, PyThreadState *p)
{
    /* Not the walk's records: the copies of registers that precise frames
       own would be taken for roots. */
    _PyThreadStateImpl *ts = (_PyThreadStateImpl *)p;
    uintptr_t skip_lo = (uintptr_t)&ts->gc_trace.detach;
    uintptr_t skip_hi = (uintptr_t)(&ts->gc_trace.entry + 1);
    uintptr_t head = (uintptr_t)&ts->asyncio_tasks_head;
    uintptr_t head_end = head + sizeof(ts->asyncio_tasks_head);
    assert(head_end <= skip_lo);
    scan_range(t, (uintptr_t)p, head);
    scan_range(t, head_end, skip_lo);
    scan_range(t, skip_hi, (uintptr_t)p + sizeof(_PyThreadStateImpl));
    /* The data stack.  A frame that is returning is unlinked from
     * current_frame before its slots are cleared, and a finalizer that
     * clearing a slot triggers can collect while the later slots still
     * hold their objects.  The current chunk holds slots up to
     * datastack_top and frame headers from datastack_limit; earlier
     * chunks are scanned whole. */
    _PyStackChunk *chunk = p->datastack_chunk;
    if (chunk == NULL) {
        return;
    }
    uintptr_t end = (uintptr_t)chunk + chunk->size;
    scan_range(t, (uintptr_t)&chunk->data[0], (uintptr_t)p->datastack_top);
    scan_range(t, (uintptr_t)p->datastack_limit, end);
    for (chunk = chunk->previous; chunk != NULL; chunk = chunk->previous) {
        scan_range(t, (uintptr_t)&chunk->data[0],
                   (uintptr_t)chunk + chunk->size);
    }
}

static void
scan_structs(trace_t *t)
{
    scan_struct_except(t, t->interp, sizeof(PyInterpreterState),
                       &t->interp->asyncio_tasks_head);
    _Py_FOR_EACH_TSTATE_UNLOCKED(t->interp, p) {
        scan_thread_struct(t, p);
    }
    for (PyThreadState *p = kept_threads(t->interp); p; p = p->next) {
        scan_thread_struct(t, p);
    }
}


/* --- precise roots ------------------------------------------------------- */

static int
visit_frame(_PyInterpreterFrame *f, visitproc visit, void *arg)
{
    Py_VISIT(f->f_globals);
    Py_VISIT(f->f_builtins);
    return _PyFrame_Traverse(f, visit, arg);
}

/* The frame headers of a chunk of the data stack, from limit, the
   innermost, to the chunk's first, which ends one word before the chunk
   does (pystate.c's chunk_frames_end()). */
static int
visit_chunk_frames(_PyStackChunk *chunk, PyObject **limit, visitproc visit,
                   void *arg)
{
    PyObject **end = (PyObject **)((char *)chunk + chunk->size) - 1;
    for (PyObject **h = limit; h + FRAME_SPECIALS_SIZE <= end;
         h += FRAME_SPECIALS_SIZE)
    {
        _PyInterpreterFrame *f = (_PyInterpreterFrame *)h;
        // Pushed and not yet initialized (_PyThreadState_PushFrame())
        if (PyValue_IsNull(f->f_executable)) {
            continue;
        }
        assert(f->owner == FRAME_OWNED_BY_THREAD);
        int err = visit_frame(f, visit, arg);
        if (err) {
            return err;
        }
    }
    return 0;
}

/* p's interpreter frames (note 11 section 4.4): every frame on its data
   stack, from push to pop, linked into the chain or not, and then the
   chain's frames that are not on the data stack, the running generator
   and coroutine frames among them.  Of a frame the interpreter owns (an
   entry frame of the eval loop, in its C frame, or the thread's base
   frame) only the return slot is visited: what a returning or yielding
   frame stores there and INTERPRETER_EXIT returns (4.3).  Its other fields
   are poisoned in debug builds. */
int
_PyGC_TraceVisitFrames(PyThreadState *p, visitproc visit, void *arg)
{
    _PyStackChunk *chunk = p->datastack_chunk;
    if (chunk != NULL) {
        int err = visit_chunk_frames(chunk, p->datastack_limit, visit, arg);
        if (err) {
            return err;
        }
        for (chunk = chunk->previous; chunk != NULL; chunk = chunk->previous) {
            err = visit_chunk_frames(chunk, &chunk->data[chunk->top], visit,
                                     arg);
            if (err) {
                return err;
            }
        }
    }
    for (_PyInterpreterFrame *f = p->current_frame; f != NULL; f = f->previous) {
        if (f->owner == FRAME_OWNED_BY_THREAD) {
            continue;
        }
        if (f->owner >= FRAME_OWNED_BY_INTERPRETER) {
            _Py_VISIT_VALUE(f->base[f->return_slot]);
            continue;
        }
        int err = visit_frame(f, visit, arg);
        if (err) {
            return err;
        }
    }
    return 0;
}

#ifdef Py_DEBUG
/* A span does not run past its buffer, when the buffer is a block of the
   mimalloc heaps (PyMem_Malloc() and the like). */
static void
check_value_span(const _PyValueRoot *r)
{
    assert(r->n >= 0);
    assert(r->n == 0 || r->base != NULL);
    if (r->n == 0 || !mi_is_in_heap_region(r->base)) {
        return;
    }
    mi_page_t *page = _mi_ptr_page(r->base);
    uintptr_t start = (uintptr_t)mi_page_start(page);
    size_t bsize = mi_page_block_size(page);
    uintptr_t block = start + ((uintptr_t)r->base - start) / bsize * bsize;
    assert((uintptr_t)(r->base + r->n) <= block + mi_usable_size((void *)block));
}
#endif

int
_PyGC_TraceVisitThreadState(PyThreadState *p, visitproc visit, void *arg)
{
    _PyThreadStateImpl *ts = (_PyThreadStateImpl *)p;
    Py_VISIT(p->c_profileobj);
    Py_VISIT(p->c_traceobj);
    Py_VISIT(p->current_exception);
    for (_PyErr_StackItem *e = p->exc_info; e != NULL; e = e->previous_item) {
        Py_VISIT(e->exc_value);
    }
    Py_VISIT(p->dict);
    Py_VISIT(p->async_exc);
    Py_VISIT(p->async_gen_firstiter);
    Py_VISIT(p->async_gen_finalizer);
    Py_VISIT(p->context);
    Py_VISIT(p->threading_local_key);
    Py_VISIT(p->threading_local_sentinel);
    Py_VISIT(ts->asyncio_running_loop);
    Py_VISIT(ts->asyncio_running_task);
    Py_VISIT(ts->lazy_imports);
    for (_PyValueRoot *r = ts->gc_trace.value_roots; r != NULL; r = r->next) {
#ifdef Py_DEBUG
        check_value_span(r);
#endif
        for (Py_ssize_t i = 0; i < r->n; i++) {
            if (PyValue_IsObject(r->base[i])) {
                Py_VISIT(PyValue_ObjectBorrow(r->base[i]));
            }
        }
    }
    return 0;
}

int
_PyGC_TraceVisitInterp(PyInterpreterState *interp, visitproc visit, void *arg)
{
    Py_VISIT(interp->sysdict);
    Py_VISIT(interp->builtins);
    Py_VISIT(interp->dict);
    Py_VISIT(interp->sysdict_copy);
    Py_VISIT(interp->builtins_copy);
    Py_VISIT(interp->imports.modules);
    Py_VISIT(interp->imports.modules_by_index);
    Py_VISIT(interp->imports.importlib);
    Py_VISIT(interp->imports.import_func);
    Py_VISIT(interp->imports.lazy_import_func);
    Py_VISIT(interp->imports.lazy_imports_filter);
    Py_VISIT(interp->imports.lazy_modules);
    Py_VISIT(interp->imports.lazy_pending_submodules);
    Py_VISIT(interp->codecs.search_path);
    Py_VISIT(interp->codecs.search_cache);
    Py_VISIT(interp->codecs.error_registry);
    Py_VISIT(interp->before_forkers);
    Py_VISIT(interp->after_forkers_parent);
    Py_VISIT(interp->after_forkers_child);
    Py_VISIT(interp->audit_hooks);
    for (int t = 0; t < PY_MONITORING_TOOL_IDS; t++) {
        Py_VISIT(interp->monitoring_tool_names[t]);
        for (int e = 0; e < _PY_MONITORING_EVENTS; e++) {
            Py_VISIT(interp->monitoring_callables[t][e]);
        }
    }
    Py_VISIT(interp->atexit.callbacks);
    Py_VISIT(interp->exc_state.errnomap);
    Py_VISIT(interp->exc_state.PyExc_ExceptionGroup);
    Py_VISIT(interp->xi.exceptions.PyExc_InterpreterError);
    Py_VISIT(interp->xi.exceptions.PyExc_InterpreterNotFoundError);
    Py_VISIT(interp->xi.exceptions.PyExc_NotShareableError);
    Py_VISIT(interp->xi.pickle.dumps);
    Py_VISIT(interp->xi.pickle.loads);
    for (_PyXIData_regitem_t *item = interp->xi.data_lookup.registry.head;
         item != NULL; item = item->next)
    {
        Py_VISIT(item->weakref);
    }
    struct _Py_interp_cached_objects *cached = &interp->cached_objects;
    Py_VISIT(cached->interned_strings);
    Py_VISIT(cached->objreduce);
    Py_VISIT(cached->generic_type);
    Py_VISIT(cached->typevar_type);
    Py_VISIT(cached->typevartuple_type);
    Py_VISIT(cached->paramspec_type);
    Py_VISIT(cached->paramspecargs_type);
    Py_VISIT(cached->paramspeckwargs_type);
    Py_VISIT(cached->constevaluator_type);
    Py_VISIT(cached->dict_descriptor);
    Py_VISIT(cached->weakref_descriptor);
    // The GC's own lists
    Py_VISIT(interp->gc.garbage);
    Py_VISIT(interp->gc.callbacks);
    for (_PyGCRoot *r = interp->gc.trace_roots; r != NULL; r = r->next) {
        for (Py_ssize_t i = 0; i < r->n; i++) {
            Py_VISIT(r->objs[i]);
        }
    }
    return 0;
}

static void
mark_precise_roots(trace_t *t)
{
    PyInterpreterState *interp = t->interp;
    t->conservative = 0;
    if (t->roots & (_PyGC_TRACE_ROOT_FRAMES | _PyGC_TRACE_ROOT_TSTATE)) {
        _Py_FOR_EACH_TSTATE_UNLOCKED(interp, p) {
            if (t->roots & _PyGC_TRACE_ROOT_FRAMES) {
                (void)_PyGC_TraceVisitFrames(p, mark_visit, t);
                drain(t);
            }
            if (t->roots & _PyGC_TRACE_ROOT_TSTATE) {
                (void)_PyGC_TraceVisitThreadState(p, mark_visit, t);
                drain(t);
            }
        }
    }
    if (t->roots & _PyGC_TRACE_ROOT_INTERP) {
        (void)_PyGC_TraceVisitInterp(interp, mark_visit, t);
        drain(t);
    }
    if (t->roots & _PyGC_TRACE_ROOT_TYPES) {
        for (Py_ssize_t i = 0; i < t->ts->n_static_types; i++) {
            mark_root(t, (PyObject *)t->ts->static_types[i]);
        }
    }
    if (t->roots & _PyGC_TRACE_ROOT_IMMORTAL) {
        // An immortal object is never freed, so what it refers to is live.
        for (size_t i = 0; i <= t->objects.mask; i++) {
            uintptr_t op = t->objects.keys[i];
            if (op != 0 && _Py_IsImmortal((PyObject *)op)
                && is_object(t, (PyObject *)op, 1))
            {
                mark_root(t, (PyObject *)op);
            }
        }
    }
}


/* --- the live collector (note 06, step 5a) ----------------------------------

   The same roots and the same conservative scans as the verifier, but the
   mark state is in the object headers: _Py_GC_MARKED_FLAG, and
   _Py_GC_SHALLOW_FLAG for an object that a conservative root reached and
   that was not safe to traverse.  An address is an object if it lies in a
   page of the object heaps, at the object's offset in its block, and the
   object map has its bit (_PyGC_IsHeapObject()).  A precise reference
   to anything else is not followed: it is a static object, whose memory the
   data segment scan covers, or an object of another interpreter's heaps.
   Immortal heap objects are never marked; the live collector keeps a list of
   those that refer to others and traverses them as roots.

   The interpreters that share the main interpreter's object allocator share
   objects (a single-phase init module's m_copy), so they are collected
   together, as one domain. */

static int
shares_main_heap(PyInterpreterState *interp)
{
    return _Py_IsMainInterpreter(interp)
        || (interp->feature_flags & Py_RTFLAGS_USE_MAIN_OBMALLOC);
}

/* The interpreters collected with interp.  The caller holds the head lock. */
static void
live_skip(trace_t *t, void *p, size_t size)
{
    assert(t->nskip < (int)Py_ARRAY_LENGTH(t->skip_lo));
    t->skip_lo[t->nskip] = (uintptr_t)p;
    t->skip_hi[t->nskip++] = (uintptr_t)p + size;
}

/* The caches of borrowed references in an interpreter, and its lists of
   asyncio tasks, which the deallocs of their objects clear.  Scanning them
   would keep the objects.  The initial thread state's walk records hold
   registers that precise frames own, which would be taken for roots.  The
   main interpreter and its initial thread state are in _PyRuntime, which the
   scan of the data segments reads, and the struct scan of an interpreter
   covers its initial thread state; the struct scan skips the lists and the
   records of the other thread states. */
static void
live_skip_caches(trace_t *t)
{
    t->nskip = 0;
    for (int d = 0; d < t->ndomain; d++) {
        PyInterpreterState *interp = t->domain[d];
        live_skip(t, interp->func_state.func_version_cache,
                  sizeof(interp->func_state.func_version_cache));
        live_skip(t, interp->types.type_version_cache,
                  sizeof(interp->types.type_version_cache));
        live_skip(t, &interp->asyncio_tasks_head, sizeof(struct llist_node));
        live_skip(t, &interp->_initial_thread.asyncio_tasks_head,
                  sizeof(struct llist_node));
        _PyThreadStateImpl *ts = &interp->_initial_thread;
        live_skip(t, &ts->gc_trace.detach,
                  (char *)(&ts->gc_trace.entry + 1) - (char *)&ts->gc_trace.detach);
    }
}

static int
live_domain(trace_t *t, PyInterpreterState *interp)
{
    t->ndomain = 0;
    if (!shares_main_heap(interp)) {
        t->domain[t->ndomain++] = interp;
    }
    else {
        for (PyInterpreterState *i = _PyRuntime.interpreters.head; i != NULL;
             i = i->next)
        {
            if (!shares_main_heap(i)) {
                continue;
            }
            if (t->ndomain == (int)Py_ARRAY_LENGTH(t->domain)) {
                return -1;
            }
            t->domain[t->ndomain++] = i;
        }
    }
    for (int d = 0; d < t->ndomain; d++) {
        t->subproc[d] = t->domain[d]->mimalloc.subproc;
    }
    t->seg_lo = mi_atomic_load_acquire(&_mi_gc_segmap_lo);
    uintptr_t hi = mi_atomic_load_acquire(&_mi_gc_segmap_hi);
    t->seg_span = hi > t->seg_lo ? hi - t->seg_lo : 0;
    t->tag_offset[_Py_MIMALLOC_HEAP_MEM] = -1;
    t->tag_offset[_Py_MIMALLOC_HEAP_NOSCAN] = -1;
    t->tag_offset[_Py_MIMALLOC_HEAP_OBJECT] = t->debug_offset;
    t->tag_offset[_Py_MIMALLOC_HEAP_GC] = t->debug_offset;
    live_skip_caches(t);
    return 0;
}

/* The object heap page of the domain that holds addr, through the segment
   map that mimalloc keeps for the collector (_mi_gc_segment_of()), or NULL.
   An address in a segment's metadata, in a free span or in a mem heap page
   is not in an object page.  The slices of a free span keep stale back
   offsets, so the page a slice leads to must start at offset 0, be in use
   and cover the slice. */
static inline mi_page_t *
live_page_of(const trace_t *t, uintptr_t addr)
{
    if (addr - t->seg_lo >= t->seg_span) {
        return NULL;
    }
    mi_segment_t *seg = _mi_gc_segment_of(addr);
    if (seg == NULL) {
        return NULL;
    }
    if (seg->subproc != t->subproc[0]) {
        int d = 1;
        while (d < t->ndomain && seg->subproc != t->subproc[d]) {
            d++;
        }
        if (d == t->ndomain) {
            return NULL;  // another interpreter's heap
        }
    }
    size_t info = seg->segment_info_slices;
    size_t idx = (addr - (uintptr_t)seg) >> MI_SEGMENT_SLICE_SHIFT;
    if (idx < info) {
        return NULL;  // the segment's metadata
    }
    mi_slice_t *slice;
    if (seg->kind == MI_SEGMENT_HUGE) {
        slice = &seg->slices[info];  // the only page
    }
    else {
        slice = &seg->slices[idx];
        size_t back = slice->slice_offset / sizeof(mi_slice_t);
        if (back > idx - info) {
            return NULL;
        }
        slice -= back;
        if (slice->slice_offset != 0
            || (size_t)(slice - seg->slices) + slice->slice_count <= idx)
        {
            return NULL;
        }
    }
    if (slice->block_size <= 1) {
        return NULL;  // a free span (0) or not a page (1)
    }
    mi_page_t *page = (mi_page_t *)slice;
    if (page->heap_tag >= _Py_MIMALLOC_HEAP_COUNT
        || t->tag_offset[page->heap_tag] < 0)
    {
        return NULL;
    }
    return page;
}

/* The start of the block of page that holds addr, or 0 if addr is before
   the first block or past the blocks the page has handed out. */
static inline uintptr_t
live_block_of(const mi_page_t *page, uintptr_t addr)
{
    uintptr_t start = (uintptr_t)page->page_start;
    if (addr < start) {
        return 0;
    }
    size_t bsize = page->block_size;
    size_t n = (addr - start) / bsize;
    if (n >= page->capacity) {
        return 0;
    }
    return start + n * bsize;
}

/* The object at addr, or containing addr if interior is set, or NULL. */
static inline PyObject *
live_lookup(const trace_t *t, uintptr_t addr, int interior)
{
    mi_page_t *page = live_page_of(t, addr);
    if (page == NULL) {
        return NULL;
    }
    uintptr_t block = live_block_of(page, addr);
    if (block == 0) {
        return NULL;
    }
    uintptr_t op = block + t->tag_offset[page->heap_tag];
    if (op != addr && !interior) {
        return NULL;
    }
    if (!_PyGC_IsHeapObject((PyObject *)op)) {
        return NULL;
    }
    return (PyObject *)op;
}

/* Can an object that only a conservative root reached be traversed?  An
   untracked GC object other than a tuple may be under construction, and a
   non-GC type with a traverse function or a dict word has no flag to say
   that it is not. */
static inline int
live_safe_to_traverse(PyObject *op)
{
    PyTypeObject *tp = Py_TYPE(op);
    if (tp->tp_traverse == NULL && !_PyGC_HasDictWord(tp)) {
        return 1;
    }
    if (!_PyObject_IS_GC(op)) {
        return 0;
    }
    return (op->ob_flags & _Py_GC_TRACKED_FLAG) || PyTuple_Check(op);
}

/* The stack hits keep at most this many records between reads */
#define STACK_HITS_MAX (1 << 18)

/* List op, which the mark reaches first from the stack or register word
   being scanned */
static void
stack_hit(trace_t *t, PyObject *op)
{
    struct _gc_trace_state *ts = t->ts;
    if (ts->nstack_hits == ts->stack_hits_cap) {
        size_t cap = ts->stack_hits_cap ? 2 * ts->stack_hits_cap : 1024;
        void *h = cap <= STACK_HITS_MAX
                  ? realloc(ts->stack_hits, cap * sizeof(*ts->stack_hits))
                  : NULL;
        if (h == NULL) {
            ts->stack_hits_dropped++;
            return;
        }
        ts->stack_hits = h;
        ts->stack_hits_cap = cap;
    }
    _PyGCStackHit *e = &ts->stack_hits[ts->nstack_hits++];
    *e = (_PyGCStackHit){
        .op = (uintptr_t)op,
        .word = t->scan_class == _PyGC_SCAN_STACK ? t->hit_word : 0,
        .value = t->hit_value,
        .pc = t->frame_pc,
        .sp = t->frame_sp,
        .cfa = t->frame_cfa,
        .scan_class = t->scan_class,
        .collection = ts->stack_hits_collection,
    };
    // The type may die with the object, so its name is copied
    PyOS_snprintf(e->type_name, sizeof(e->type_name), "%s",
                  Py_TYPE(op)->tp_name);
}

static inline void
live_mark(trace_t *t, PyObject *op, int conservative)
{
    uint16_t flags = op->ob_flags;
    if (flags & _Py_IMMORTAL_FLAGS) {
        return;  // a root, traversed from the immortal list
    }
    if (flags & _Py_GC_MARKED_FLAG) {
        if ((flags & _Py_GC_SHALLOW_FLAG) && !conservative) {
            // A precise reference to an object marked without traversal
            op->ob_flags = flags & ~_Py_GC_SHALLOW_FLAG;
            push(t, op);
        }
        return;
    }
    t->nmarked++;
    if (t->phase_b) {
        verify_list(t, op);
    }
    if (t->hit_value != 0) {
        stack_hit(t, op);
    }
    if (conservative && !live_safe_to_traverse(op)) {
        op->ob_flags = flags | _Py_GC_MARKED_FLAG | _Py_GC_SHALLOW_FLAG;
        t->nshallow++;
        /* Its fields cannot be trusted as objects, but its words can be
           scanned: a set or dict being filled holds its members in a table
           it owns. */
        mi_page_t *page = live_page_of(t, (uintptr_t)op);
        uintptr_t block = live_block_of(page, (uintptr_t)op);
        queue_range(t, block, block + page->block_size, _PyGC_SCAN_SHALLOW);
        return;
    }
    op->ob_flags = flags | _Py_GC_MARKED_FLAG;
    PyTypeObject *tp = Py_TYPE(op);
    if (tp->tp_traverse == NULL && !(tp->tp_flags & Py_TPFLAGS_HEAPTYPE)
        && !_PyGC_HasDictWord(tp))
    {
        return;  // a leaf
    }
    push(t, op);
}

#ifdef Py_DEBUG
/* A precise reference into an object page that is not an object: in a debug
   build a freed block is filled with PYMEM_DEADBYTE, and finding that where
   the type should be means the collector freed a reachable object. */
static void
live_check_dangling(trace_t *t, uintptr_t addr)
{
    if ((addr & (sizeof(void *) - 1)) != 0) {
        return;
    }
    mi_page_t *page = live_page_of(t, addr);
    if (page == NULL || live_block_of(page, addr) == 0) {
        return;
    }
    if (*(uintptr_t *)(addr + sizeof(uintptr_t)) == (uintptr_t)0xDDDDDDDDDDDDDDDDULL) {
        fprintf(stderr, "gc: %s at %p refers to a freed object at %p\n",
                t->current ? Py_TYPE(t->current)->tp_name : "a root",
                (void *)t->current, (void *)addr);
        Py_FatalError("the collector freed a reachable object");
    }
}
#endif

static int
live_visit(PyObject *op, void *arg)
{
    trace_t *t = arg;
#ifdef Py_DEBUG
    // _Py_VISIT_VALUE() drops inline values, so an inline encoding here
    // came from a PyObject * field that was given a value's bits.  It is
    // reported with stdio: the collector cannot run Python code.
    if (op != NULL && PyValue_IsInline(_PyValue_FromBits((uintptr_t)op))) {
        fprintf(stderr, "Fatal Python error: an inline value in a "
                "PyObject * field of a '%.100s' object\n",
                t->current != NULL ? Py_TYPE(t->current)->tp_name
                                   : "(root)");
        fflush(stderr);
        abort();
    }
#endif
    PyObject *o = live_lookup(t, (uintptr_t)op, 0);
    if (o != NULL) {
        live_mark(t, o, 0);
    }
#ifdef Py_DEBUG
    else if (op != NULL) {
        live_check_dangling(t, (uintptr_t)op);
    }
#endif
    return 0;
}

static void
live_drain(trace_t *t)
{
    while (t->sp > 0 && !t->oom) {
        PyObject *op = t->stack[--t->sp];
        t->current = op;
        (void)live_visit((PyObject *)Py_TYPE(op), t);
        (void)_PyGC_TraceTraverse(op, live_visit, t);
    }
    t->current = NULL;
}

/* Traverse a static type, a root that is not marked.  type_ready()
   registers a type before it sets the type's own type and allocates in
   between, so this does not read ob_type. */
static void
live_traverse_static_type(PyTypeObject *type, trace_t *t)
{
    t->current = (PyObject *)type;
    (void)_PyType_TraverseStatic(type, live_visit, t);
    live_drain(t);
}

static void _Py_NO_SANITIZE_ADDRESS
live_scan_words(trace_t *t, uintptr_t start, uintptr_t stop)
{
    _PyGCRootStats *stats = t->stats;
    int cls = t->scan_class;
    // What the words reach is attributed to them.  The blocks they queue
    // are scanned later, without attribution.
    int hits = t->ts != NULL && t->ts->stack_hits_on && !t->in_queue
               && !t->phase_b
               && (cls == _PyGC_SCAN_STACK || cls == _PyGC_SCAN_REGS);
    if (stats != NULL && stop > start) {
        stats->words[cls] += (int64_t)((stop - start) / sizeof(uintptr_t));
    }
    for (uintptr_t *p = (uintptr_t *)start;
         (uintptr_t)p + sizeof(uintptr_t) <= stop; p++)
    {
        uintptr_t w = *p;
        PyObject *op = live_lookup(t, w, 1);
        if (op != NULL) {
            if (stats != NULL) {
                stats->hits[cls]++;
            }
            if (t->phase_b && !t->in_queue) {
                t->rec.word = (uintptr_t)p;
            }
            if (hits) {
                t->hit_word = (uintptr_t)p;
                t->hit_value = w;
            }
            live_mark(t, op, 1);
            live_drain(t);
            t->hit_word = t->hit_value = 0;
        }
        else if (t->scan_mem && w >= t->mem_lo && w < t->mem_hi) {
            if (t->frame_core && !t->phase_b) {
                verify_defer(t, (uintptr_t)p, w, VERIFY_FROM_CORE_MEM);
            }
            else {
                queue_mem_block(t, w);
            }
        }
    }
}

struct live_walk {
    trace_t *t;
    size_t offset;
    int pages;            // record the mem heaps' pages, not the blocks
    _PyGCMarkResult *res;
    size_t dead_cap;
    PyObject **objs;      // for _PyGC_TraceLiveHeapObjects()
    size_t nobjs;
    size_t objs_cap;
    int failed;
    // The dict and type watcher IDs with a callback (live_has_destroy_hook())
    int dict_watchers;
    int type_watchers;
};

/* Would op's dealloc, under reference counting, make a call that may run
   code or allocate: a dealloc watcher's DESTROY event, a code object's
   co_extra free functions, a module's m_free, a capsule's destructor, or
   the release of a buffer whose exporter defines __release_buffer__?  The
   sweep forbids both, so gc_live.c makes these calls before it
   (queue_destroy_hooks()).  Each runs once per object.  Only the types with
   _Py_TPFLAGS_DESTROY_HOOK have such instances. */
static inline int
live_destroy_hook_of(const struct live_walk *w, PyInterpreterState *interp,
                     PyObject *op, PyTypeObject *tp)
{
    if (tp->tp_flags & Py_TPFLAGS_DICT_SUBCLASS) {
        return (((PyDictObject *)op)->_ma_watcher_tag & w->dict_watchers) != 0;
    }
    if (tp->tp_flags & Py_TPFLAGS_TYPE_SUBCLASS) {
        return (((PyTypeObject *)op)->tp_watched & w->type_watchers) != 0;
    }
    if (tp == &PyFunction_Type) {
        return interp->active_func_watchers != 0;
    }
    if (tp == &PyCode_Type) {
        return interp->active_code_watchers != 0 || _PyCode_HasExtraFree(op);
    }
    if (tp == &PyCapsule_Type) {
        return _PyCapsule_HasDestructor(op);
    }
    if (tp == &_PyManagedBuffer_Type || tp == &PyMemoryView_Type) {
        return _PyMemoryView_DeallocReleasesWithCode(op);
    }
    // Only a heap type can subclass module, so most types skip the MRO walk.
    if (tp == &PyModule_Type
        || ((tp->tp_flags & Py_TPFLAGS_HEAPTYPE) && PyModule_Check(op)))
    {
        return ((PyModuleObject *)op)->md_state_free != NULL;
    }
    return 0;
}

static inline int
live_has_destroy_hook(const struct live_walk *w, PyInterpreterState *interp,
                      PyObject *op, PyTypeObject *tp)
{
    if (!(tp->tp_flags & _Py_TPFLAGS_DESTROY_HOOK)) {
        assert(!live_destroy_hook_of(w, interp, op, tp));
        return 0;
    }
    if (op->ob_flags & _Py_GC_NOTIFIED_FLAG) {
        return 0;
    }
    return live_destroy_hook_of(w, interp, op, tp);
}

static bool
live_walk_block(const mi_heap_t *heap, const mi_heap_area_t *area,
                void *block, size_t block_size, void *arg)
{
    struct live_walk *w = arg;
    trace_t *t = w->t;
    if (w->pages) {
        if (block != NULL || area->used == 0) {
            return true;
        }
        uintptr_t start = (uintptr_t)area->blocks;
        if (rangelist_add(&t->mem_pages, start, start + area->reserved,
                          area->full_block_size, w->offset) < 0) {
            t->oom = 1;
            return false;
        }
        return true;
    }
    if (block == NULL) {
        return true;
    }
    PyObject *op = (PyObject *)((char *)block + w->offset);
    if (!_PyGC_IsHeapObject(op)) {
        return true;
    }
    if (w->objs != NULL || w->res == NULL) {
        // Listing the objects
        if (w->failed) {
            return false;
        }
        if (w->nobjs == w->objs_cap) {
            size_t cap = w->objs_cap ? w->objs_cap * 2 : 4096;
            PyObject **objs = realloc(w->objs, cap * sizeof(PyObject *));
            if (objs == NULL) {
                w->failed = 1;
                return false;
            }
            w->objs = objs;
            w->objs_cap = cap;
        }
        w->objs[w->nobjs++] = op;
        return true;
    }
    _PyGCMarkResult *res = w->res;
    res->nobjects++;
    uint16_t flags = op->ob_flags;
    if (flags & _Py_GC_MARKED_FLAG) {
        uint16_t clear = _Py_GC_MARKED_FLAG | _Py_GC_SHALLOW_FLAG;
        if (flags & (_Py_GC_FINALIZED_FLAG | _Py_GC_DEL_CALLED_FLAG)) {
            /* Reachable again: tp_del runs at its next death, as it does
               at each one under reference counting, and so does the
               finalizer of a non-GC object, which PEP 442 does not cover. */
            clear |= _Py_GC_DEL_CALLED_FLAG;
            if (!_PyObject_IS_GC(op)) {
                clear |= _Py_GC_FINALIZED_FLAG;
            }
        }
        op->ob_flags = flags & ~clear;
        res->live_bytes += block_size;
        return true;
    }
    if ((flags & _Py_IMMORTAL_FLAGS) || w->failed) {
        // After a failure everything is kept, and the walk goes on only to
        // clear the marks.
        res->live_bytes += block_size;
        return true;
    }
    if ((size_t)res->ndead == w->dead_cap) {
        size_t cap = w->dead_cap ? w->dead_cap * 2 : 4096;
        _PyGCDeadObject *dead = realloc(res->dead, cap * sizeof(*dead));
        if (dead == NULL) {
            w->failed = 1;
            res->live_bytes += block_size;
            return true;
        }
        res->dead = dead;
        w->dead_cap = cap;
    }
    op->ob_flags = flags | _Py_GC_DEAD_FLAG;
    PyTypeObject *tp = Py_TYPE(op);
    uint16_t kinds = (_PyType_SUPPORTS_WEAKREFS(tp) ? _PyGC_DEAD_WEAKREFS : 0)
        | (tp->tp_finalize != NULL ? _PyGC_DEAD_FINALIZE : 0)
        | (tp->tp_del != NULL ? _PyGC_DEAD_DEL : 0)
        | (PyType_HasFeature(tp, Py_TPFLAGS_DEALLOC_READS_REFERENTS)
           ? _PyGC_DEAD_FIRST : 0)
        | (PyType_Check(op) ? _PyGC_DEAD_TYPE : 0)
        | (_PyObject_IS_GC(op) ? _PyGC_DEAD_GC : 0)
        | (live_has_destroy_hook(w, t->interp, op, tp) ? _PyGC_DEAD_HOOK : 0)
        | (t->killed.keys != NULL && addrset_has(&t->killed, (uintptr_t)op)
           ? _PyGC_DEAD_KILLED : 0);
    res->kinds |= kinds;
    res->dead[res->ndead++] = (_PyGCDeadObject){
        op, (uint32_t)block_size, kinds};
    return true;
}

/* Visit the blocks of the object heaps or, for the page table, the pages of
   the mem heaps, of every thread state of the domain and of its exited
   threads.  The object pages need no table: live_page_of() finds them
   through the segment map.  The caller holds the head lock. */
static int
live_walk_heaps(trace_t *t, struct live_walk *w)
{
    static const int tags[] = {
        _Py_MIMALLOC_HEAP_OBJECT, _Py_MIMALLOC_HEAP_GC,
    };
    bool all = !w->pages;
    int ok = 1;
    for (int d = 0; ok && d < t->ndomain; d++) {
        PyInterpreterState *interp = t->domain[d];
        _Py_FOR_EACH_TSTATE_UNLOCKED(interp, p) {
            struct _mimalloc_thread_state *m =
                &((_PyThreadStateImpl *)p)->mimalloc;
            if (m->current_object_heap == NULL) {
                continue;  // not bound yet
            }
            for (size_t i = 0; ok && all && i < Py_ARRAY_LENGTH(tags); i++) {
                w->offset = t->debug_offset;
                ok = mi_heap_visit_blocks(&m->heaps[tags[i]], all,
                                          live_walk_block, w);
            }
            if (ok && w->pages) {
                w->offset = 0;
                ok = mi_heap_visit_blocks(&m->heaps[_Py_MIMALLOC_HEAP_MEM],
                                          false, live_walk_block, w);
            }
            if (!ok) {
                break;
            }
        }
        mi_subproc_t *subproc = interp->mimalloc.subproc;
        for (size_t i = 0; ok && all && i < Py_ARRAY_LENGTH(tags); i++) {
            w->offset = t->debug_offset;
            ok = mi_abandoned_visit_blocks(subproc, tags[i], all,
                                           live_walk_block, w);
        }
        if (ok && w->pages) {
            w->offset = 0;
            ok = mi_abandoned_visit_blocks(subproc, _Py_MIMALLOC_HEAP_MEM,
                                           false, live_walk_block, w);
        }
    }
    if (!ok || t->oom || w->failed) {
        return -1;
    }
    return 0;
}

static int
live_build_pages(trace_t *t)
{
    struct live_walk w = {.t = t, .pages = 1};
    if (live_walk_heaps(t, &w) < 0) {
        return -1;
    }
    rangelist_sort(&t->mem_pages);
    t->mem_lo = t->mem_hi = 0;
    if (t->mem_pages.n > 0) {
        t->mem_lo = t->mem_pages.items[0].start;
        t->mem_hi = t->mem_pages.items[t->mem_pages.n - 1].end;
    }
    return 0;
}

/* Does another thread of the domain have a stack to scan?  Then the scans
   clip to the readable mappings: a recorded stack can belong to a thread
   that has exited. */
static int
live_other_threads(trace_t *t, PyThreadState *current)
{
    for (int d = 0; d < t->ndomain; d++) {
        _Py_FOR_EACH_TSTATE_UNLOCKED(t->domain[d], p) {
            if (p != current
                && ((_PyThreadStateImpl *)p)->gc_trace.detach.valid) {
                return 1;
            }
        }
        if (kept_threads(t->domain[d]) != NULL) {
            return 1;
        }
    }
    return 0;
}

static void
live_mark_thread(trace_t *t, PyThreadState *p, unsigned int roots)
{
    if (roots & _PyGC_TRACE_ROOT_FRAMES) {
        (void)_PyGC_TraceVisitFrames(p, live_visit, t);
        live_drain(t);
    }
    if (roots & _PyGC_TRACE_ROOT_TSTATE) {
        (void)_PyGC_TraceVisitThreadState(p, live_visit, t);
        live_drain(t);
    }
}

static inline int64_t
live_now(void)
{
    PyTime_t now = 0;
    (void)PyTime_PerfCounterRaw(&now);
    return (int64_t)now;
}

static void
live_mark_roots(trace_t *t, PyThreadState *tstate)
{
    unsigned int roots = t->roots;
    _PyGCRootStats *stats = t->stats;
    int64_t t0 = live_now();
    // Precise roots
    t->conservative = 0;
    for (int d = 0; d < t->ndomain && !t->oom; d++) {
        PyInterpreterState *interp = t->domain[d];
        t->interp = interp;
        _Py_FOR_EACH_TSTATE_UNLOCKED(interp, p) {
            live_mark_thread(t, p, roots);
        }
        for (PyThreadState *p = kept_threads(interp); p; p = p->next) {
            live_mark_thread(t, p, roots);
        }
        if (roots & _PyGC_TRACE_ROOT_INTERP) {
            (void)_PyGC_TraceVisitInterp(interp, live_visit, t);
            live_drain(t);
        }
        if (roots & _PyGC_TRACE_ROOT_TYPES) {
            Py_ssize_t ntypes;
            PyTypeObject **types = _PyGC_TraceStaticTypes(interp, &ntypes);
            for (Py_ssize_t i = 0; i < ntypes; i++) {
                live_traverse_static_type(types[i], t);
            }
        }
        if (roots & _PyGC_TRACE_ROOT_IMMORTAL) {
            (void)_PyGC_LiveVisitRoots(interp, live_visit, t);
            live_drain(t);
        }
    }
    int64_t t1 = live_now();
    // Conservative roots: the C stacks and registers, and the mem blocks
    // they point into, transitively.
    /* Every thread's stack is scanned from where it last recorded its
       registers: the others when they released the GIL, this one where the
       collection was entered (_PyGC_TraceCaptureEntry()). */
    if (roots & _PyGC_TRACE_ROOT_STACKS) {
        if (t->walk_cache != NULL) {
            _PyGC_WalkCacheCheck(t->walk_cache, stats);
        }
        t->scan_mem = 1;
        _PyThreadStateImpl *cur = (_PyThreadStateImpl *)tstate;
        if (cur->gc_trace.entry.sp != 0) {
            scan_from(t, &cur->gc_trace.entry, stack_top(tstate));
        }
        else {
            scan_current_thread(t, tstate);
        }
        for (int d = 0; d < t->ndomain && !t->oom; d++) {
            t->interp = t->domain[d];
            _Py_FOR_EACH_TSTATE_UNLOCKED(t->interp, p) {
                if (p != tstate) {
                    scan_other_thread(t, p, 1);
                }
            }
            for (PyThreadState *p = kept_threads(t->interp); p; p = p->next) {
                scan_other_thread(t, p, 0);
            }
        }
        t->scan_mem = 0;
        scan_queued(t);
    }
    int64_t t2 = live_now();
    // The data segments, then the thread and interpreter structs
    if (roots & _PyGC_TRACE_ROOT_DATA) {
        t->scan_class = _PyGC_SCAN_DATA;
        scan_data_segments(t);
        scan_queued(t);
    }
    int64_t t3 = live_now();
    if (roots & _PyGC_TRACE_ROOT_STRUCTS) {
        for (int d = 0; d < t->ndomain && !t->oom; d++) {
            t->interp = t->domain[d];
            t->scan_class = _PyGC_SCAN_STRUCTS;
            scan_structs(t);
            scan_queued(t);
        }
    }
    int64_t t4 = live_now();
    t->interp = tstate->interp;
    if (stats != NULL) {
        stats->precise_ns += t1 - t0;
        stats->stack_ns += t2 - t1;
        stats->data_ns += t3 - t2;
        stats->structs_ns += t4 - t3;
        stats->roots_ns += t4 - t0;
    }
}

static void
live_fini(trace_t *t)
{
    free(t->deferred);
    free(t->newq);
    addrset_fini(&t->killed);
    addrset_fini(&t->mem_scanned);
    free(t->mem_stack);
    rangelist_fini(&t->pages);
    rangelist_fini(&t->mem_pages);
    rangelist_fini(&t->readable);
    free(t->stack);
}

static inline int
is_dead(PyObject *op)
{
    return op != NULL && (op->ob_flags & _Py_GC_DEAD_FLAG);
}

/* The version caches hold weak references, which the scans skip.  The
   deallocs clear their entries, but in the current interpreter's caches,
   while an interpreter that shares the heaps can sweep another's objects:
   clear the entries for the dead objects in every cache of the domain. */
static void
live_clear_weak_caches(trace_t *t)
{
    for (int d = 0; d < t->ndomain; d++) {
        PyInterpreterState *interp = t->domain[d];
        for (size_t i = 0; i < FUNC_VERSION_CACHE_SIZE; i++) {
            struct _func_version_cache_item *slot =
                &interp->func_state.func_version_cache[i];
            if (is_dead((PyObject *)slot->func)) {
                slot->func = NULL;
            }
            if (is_dead(slot->code)) {
                slot->code = NULL;
            }
        }
        for (size_t i = 0; i < TYPE_VERSION_CACHE_SIZE; i++) {
            if (is_dead((PyObject *)interp->types.type_version_cache[i])) {
                interp->types.type_version_cache[i] = NULL;
            }
        }
    }
}

/* --- the live verifier: retain (work/10a-1.md item 7) --------------------- */

/* A lists a word that it leaves out, for B, with its value, since a
   register's copy does not outlive the walk */
static void
verify_defer(trace_t *t, uintptr_t word, uintptr_t value, int kind)
{
    if (t->ndeferred == t->deferred_cap) {
        size_t cap = t->deferred_cap ? 2 * t->deferred_cap : 64;
        void *d = realloc(t->deferred, cap * sizeof(*t->deferred));
        if (d == NULL) {
            t->oom = 1;
            return;
        }
        t->deferred = d;
        t->deferred_cap = cap;
    }
    t->deferred[t->ndeferred].word = word;
    t->deferred[t->ndeferred].value = value;
    t->deferred[t->ndeferred].pc = t->frame_pc;
    t->deferred[t->ndeferred].kind = kind;
    t->ndeferred++;
}

/* The words of [start, end) that may refer to an object or a PyMem block */
static void _Py_NO_SANITIZE_ADDRESS
verify_defer_range(trace_t *t, uintptr_t start, uintptr_t end, int kind)
{
    start = _Py_SIZE_ROUND_UP(start, sizeof(uintptr_t));
    for (uintptr_t *p = (uintptr_t *)start;
         (uintptr_t)p + sizeof(uintptr_t) <= end; p++)
    {
        uintptr_t w = *p;
        if (live_lookup(t, w, 1) != NULL
            || (w >= t->mem_lo && w < t->mem_hi))
        {
            verify_defer(t, (uintptr_t)p, w, kind);
        }
    }
}

/* B marks op, which A did not reach */
static void
verify_list(trace_t *t, PyObject *op)
{
    if (t->nnewq == t->newq_cap) {
        size_t cap = t->newq_cap ? 2 * t->newq_cap : 256;
        void *q = realloc(t->newq, cap * sizeof(*t->newq));
        if (q == NULL) {
            t->oom = 1;
            return;
        }
        t->newq = q;
        t->newq_cap = cap;
    }
    t->newq[t->nnewq].op = op;
    t->newq[t->nnewq].recs = (verify_recs){.n = 1, .rec = {t->rec}};
    t->nnewq++;
}

static int
verify_entry_cmp(const void *a, const void *b)
{
    uintptr_t x = (uintptr_t)((const verify_entry *)a)->op;
    uintptr_t y = (uintptr_t)((const verify_entry *)b)->op;
    return x < y ? -1 : x > y;
}

/* The image and offset of addr, or "heap" */
static void
verify_owner(char *buf, size_t size, uintptr_t addr)
{
#ifdef HAVE_DLFCN_H
    Dl_info info;
    if (addr != 0 && dladdr((void *)addr, &info) && info.dli_fname != NULL) {
        PyOS_snprintf(buf, size, "%s+%#lx", info.dli_fname,
                      (unsigned long)(addr - (uintptr_t)info.dli_fbase));
        return;
    }
#endif
    PyOS_snprintf(buf, size, "heap");
}

/* Where the record's word was, for a report */
static void
verify_where1(char *where, size_t size, const verify_rec *r)
{
    if (r->kind == VERIFY_FROM_PRECISE) {
        char owner[512];
        verify_owner(owner, sizeof(owner), r->owner);
        PyOS_snprintf(where, size,
                      "the word at %p of a precise frame at %s",
                      (void *)r->word, owner);
    }
    else if (r->kind == VERIFY_FROM_CORE_MEM) {
        char owner[512];
        verify_owner(owner, sizeof(owner), r->owner);
        PyOS_snprintf(where, size,
                      "a PyMem block that the word at %p of a core frame "
                      "at %s points to", (void *)r->word, owner);
    }
    else {
        char owner[512];
        verify_owner(owner, sizeof(owner), r->word);
        PyOS_snprintf(where, size, "the struct word at %p (%s)",
                      (void *)r->word, owner);
    }
}

/* Where each kept word was, for a report: "W1, and through W2, ..." */
#define VERIFY_WHERE_SIZE (VERIFY_RECS * 600 + 64)
static void
verify_where(char *where, size_t size, const verify_recs *recs)
{
    size_t len = 0;
    where[0] = '\0';
    for (int i = 0; i < recs->n && len < size; i++) {
        char one[600];
        verify_where1(one, sizeof(one), &recs->rec[i]);
        PyOS_snprintf(where + len, size - len, "%s%s",
                      i == 0 ? "" : ", and through ", one);
        len = strlen(where);
    }
    if (recs->more > 0 && len < size) {
        PyOS_snprintf(where + len, size - len, ", and %d more words",
                      recs->more);
    }
}

static void
verify_report_to(FILE *out, const verify_entry *e, int64_t now)
{
    char where[VERIFY_WHERE_SIZE];
    verify_where(where, sizeof(where), &e->recs);
    fprintf(out, "gc verify retain: candidate %s at %p: collection %lld "
            "reached it only through %s; collection %lld reached it from "
            "the roots\n", Py_TYPE(e->op)->tp_name, (void *)e->op,
            (long long)e->recs.rec[0].collection, where, (long long)now);
}

static void
verify_report(const verify_entry *e, int64_t now)
{
    verify_report_to(stderr, e, now);
    const char *path = getenv("PYTHON_GCVERIFY_LOG");
    if (path == NULL || *path == '\0') {
        return;
    }
    FILE *log = fopen(path, "a");
    if (log != NULL) {
        fprintf(log, "== pid %ld\n", (long)getpid());
        verify_report_to(log, e, now);
        fclose(log);
    }
}

#ifdef Py_DEBUG
/* A quarantined block is freed this many collections after its object died */
#define QUARANTINE_COLLECTIONS 16

_Py_thread_local PyThreadState *_PyGC_quarantine_tstate = NULL;

/* The record of the object kill killed in the block [block, block + size),
   or NULL.  The object need not start the block: the debug allocator's
   hooks put their header before it. */
static const verify_killed *
killed_record(struct _gc_trace_state *ts, const void *block, size_t size)
{
    size_t lo = 0, hi = ts->nkilled;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        if ((uintptr_t)ts->killed[mid].op < (uintptr_t)block) {
            lo = mid + 1;
        }
        else {
            hi = mid;
        }
    }
    if (lo < ts->nkilled
        && (uintptr_t)ts->killed[lo].op < (uintptr_t)block + size)
    {
        return &ts->killed[lo];
    }
    return NULL;
}

void
_PyGC_Quarantine(void *ptr)
{
    struct _gc_trace_state *ts = _PyGC_quarantine_tstate->interp->gc.trace;
    size_t size = mi_usable_size(ptr);
    const verify_killed *rec = killed_record(ts, ptr, size);
    if (ts->nquarantine == ts->quarantine_cap) {
        size_t cap = ts->quarantine_cap ? 2 * ts->quarantine_cap : 256;
        void *q = realloc(ts->quarantine, cap * sizeof(*ts->quarantine));
        if (q == NULL) {
            mi_free(ptr);  // freed at once, as without the quarantine
            return;
        }
        ts->quarantine = q;
        ts->quarantine_cap = cap;
    }
    memset(ptr, PYMEM_DEADBYTE, size);
    if (rec != NULL) {
        // An object's block: its header names the trap type, whose slots
        // and Py_TYPE() abort with the record.
        _PyShape *trap = _Py_TYPE_ROOT_SHAPE(&_PyGC_TrapType);
        if (trap->type == NULL) {
            _PyShape_InitRoot(&_PyGC_TrapType);
        }
        rec->op->ob_header = (uintptr_t)trap;
    }
#ifdef _Py_ADDRESS_SANITIZER
    ASAN_POISON_MEMORY_REGION(ptr, size);
#endif
    ts->quarantine[ts->nquarantine].block = ptr;
    ts->quarantine[ts->nquarantine].collection = ts->live_collections;
    ts->quarantine[ts->nquarantine].has_rec = rec != NULL;
    if (rec != NULL) {
        ts->quarantine[ts->nquarantine].rec = *rec;
    }
    ts->nquarantine++;
}

void
_PyGC_TrapUse(PyObject *op, const char *use)
{
    PyThreadState *tstate = _PyThreadState_GET();
    PyInterpreterState *interp = tstate != NULL ? tstate->interp
                                                : _PyInterpreterState_Main();
    struct _gc_trace_state *ts = interp->gc.trace;
    for (size_t i = ts != NULL ? ts->nquarantine : 0; i-- > 0;) {
        if (!ts->quarantine[i].has_rec || ts->quarantine[i].rec.op != op) {
            continue;
        }
        const verify_killed *k = &ts->quarantine[i].rec;
        static char where[VERIFY_WHERE_SIZE];  // not on a deep stack
        verify_where(where, sizeof(where), &k->recs);
        fprintf(stderr, "gc verify kill: %s of the %s at %p, which "
                "collection %lld killed: it was reached only through %s; "
                "the last collection was %lld\n", use, k->type, (void *)op,
                (long long)k->recs.rec[0].collection, where,
                (long long)ts->live_collections);
        fflush(stderr);
        break;
    }
    Py_FatalError("gc verify kill: use of a killed object");
}

/* The slots of the trap type, which a quarantined object's header names */
static PyObject *
trap_repr(PyObject *op)
{
    _PyGC_TrapUse(op, "tp_repr");
}

static PyValue
trap_getattro(PyObject *op, PyObject *name)
{
    _PyGC_TrapUse(op, "tp_getattro");
}

static int
trap_setattro(PyObject *op, PyObject *name, PyValue value)
{
    _PyGC_TrapUse(op, "tp_setattro");
}

static PyObject *
trap_call(PyObject *op, PyObject *args, PyObject *kwargs)
{
    _PyGC_TrapUse(op, "tp_call");
}

static Py_hash_t
trap_hash(PyObject *op)
{
    _PyGC_TrapUse(op, "tp_hash");
}

static PyObject *
trap_richcompare(PyObject *op, PyObject *other, int cmp)
{
    _PyGC_TrapUse(op, "tp_richcompare");
}

static int
trap_traverse(PyObject *op, visitproc visit, void *arg)
{
    _PyGC_TrapUse(op, "tp_traverse");
}

static void
trap_dealloc(PyObject *op)
{
    _PyGC_TrapUse(op, "tp_dealloc");
}

static PyObject *
trap_iter(PyObject *op)
{
    _PyGC_TrapUse(op, "tp_iter");
}

PyTypeObject _PyGC_TrapType = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    .tp_name = "gc_verify_trap",
    .tp_basicsize = sizeof(PyObject),
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_repr = trap_repr,
    .tp_getattro = trap_getattro,
    .tp_setattro = trap_setattro,
    .tp_call = trap_call,
    .tp_hash = trap_hash,
    .tp_richcompare = trap_richcompare,
    .tp_traverse = trap_traverse,
    .tp_dealloc = trap_dealloc,
    .tp_iter = trap_iter,
};

/* Free the blocks quarantined in collection upto or before */
static void
quarantine_release(struct _gc_trace_state *ts, int64_t upto)
{
    size_t kept = 0;
    for (size_t i = 0; i < ts->nquarantine; i++) {
        void *block = ts->quarantine[i].block;
        if (ts->quarantine[i].collection <= upto) {
#ifdef _Py_ADDRESS_SANITIZER
            ASAN_UNPOISON_MEMORY_REGION(block, mi_usable_size(block));
#endif
            mi_free(block);
        }
        else {
            ts->quarantine[kept++] = ts->quarantine[i];
        }
    }
    ts->nquarantine = kept;
}
#endif

/* After B: sort what B listed, and give each object, after the word B
   marked it from, every other left-out word whose value is the object. */
static void
verify_other_words(trace_t *t)
{
    qsort(t->newq, t->nnewq, sizeof(verify_entry), verify_entry_cmp);
    for (size_t i = 0; i < t->ndeferred; i++) {
        PyObject *op = live_lookup(t, t->deferred[i].value, 1);
        if (op == NULL) {
            continue;
        }
        verify_entry key = {.op = op};
        verify_entry *e = bsearch(&key, t->newq, t->nnewq,
                                  sizeof(verify_entry), verify_entry_cmp);
        if (e == NULL) {
            continue;
        }
        verify_recs *r = &e->recs;
        int seen = 0;
        for (int j = 0; j < r->n && !seen; j++) {
            seen = r->rec[j].word == t->deferred[i].word;
        }
        if (seen) {
            continue;
        }
        if (r->n == VERIFY_RECS) {
            r->more++;
            continue;
        }
        r->rec[r->n++] = (verify_rec){.word = t->deferred[i].word,
                                      .owner = t->deferred[i].pc,
                                      .kind = t->deferred[i].kind,
                                      .collection = r->rec[0].collection};
    }
}

/* After A: report the listed objects that A reached, mark B from what A
   left out, and list what B marked, keeping for each object the words
   that reached it (verify_other_words()). */
static void
verify_mark_b(trace_t *t, PyThreadState *tstate)
{
    struct _gc_trace_state *ts = t->ts;
    int64_t now = ++ts->live_collections;
    _PyGCRootStats *stats = t->stats;
    free(ts->killed);
    ts->killed = NULL;
    ts->nkilled = 0;
    size_t kept = 0;
    for (size_t i = 0; i < ts->nq; i++) {
        verify_entry *e = &ts->q[i];
        if (e->op->ob_flags & _Py_GC_MARKED_FLAG) {
            verify_report(e, now);
            if (stats != NULL) {
                stats->verify_candidates++;
            }
        }
        else {
            ts->q[kept++] = *e;
        }
    }
    ts->nq = kept;

    t->phase_b = 1;
    for (size_t i = 0; i < t->ndeferred && !t->oom; i++) {
        uintptr_t w = t->deferred[i].value;
        t->rec = (verify_rec){.word = t->deferred[i].word,
                              .owner = t->deferred[i].pc,
                              .kind = t->deferred[i].kind,
                              .collection = now};
        PyObject *op = live_lookup(t, w, 1);
        if (op != NULL) {
            live_mark(t, op, 1);
            live_drain(t);
        }
        else {
            queue_mem_block(t, w);
        }
        scan_queued(t);
    }
    if (t->verify_ex & VERIFY_EX_STRUCTS) {
        t->rec = (verify_rec){.kind = VERIFY_FROM_STRUCT, .collection = now};
        for (int d = 0; d < t->ndomain && !t->oom; d++) {
            t->interp = t->domain[d];
            t->scan_class = _PyGC_SCAN_STRUCTS;
            scan_structs(t);
            scan_queued(t);
        }
        t->interp = tstate->interp;
    }
    t->phase_b = 0;
    if (stats != NULL) {
        stats->verify_q += (int64_t)t->nnewq;
    }

    if (t->oom) {
        return;
    }
    verify_other_words(t);
    if (t->verify == VERIFY_KILL) {
        /* What B alone marked dies, as it would with A for the roots; the
           walk tells the sweep which, so that its frees are quarantined. */
        if (addrset_init(&t->killed, t->nnewq * 2 + 16) < 0) {
            t->oom = 1;
            return;
        }
        for (size_t i = 0; i < t->nnewq; i++) {
            PyObject *op = t->newq[i].op;
            op->ob_flags &= ~(_Py_GC_MARKED_FLAG | _Py_GC_SHALLOW_FLAG);
            if (addrset_add(&t->killed, (uintptr_t)op) < 0) {
                t->oom = 1;
                return;
            }
        }
        // The records, for the trap's report when a killed object is used
        // (the quarantine copies each when it takes the object's block).
        // Without memory for them, the trap reports no record.
        ts->killed = malloc(t->nnewq * sizeof(verify_killed) + 1);
        if (ts->killed != NULL) {
            for (size_t i = 0; i < t->nnewq; i++) {
                verify_killed *k = &ts->killed[i];
                k->op = t->newq[i].op;
                k->recs = t->newq[i].recs;
                PyOS_snprintf(k->type, sizeof(k->type), "%s",
                              Py_TYPE(k->op)->tp_name);
            }
            ts->nkilled = t->nnewq;
        }
        return;
    }
    /* The list is what B marked now, with the older record of an object
       that an earlier collection listed.  What neither marked dies, and
       drops out. */
    size_t j = 0;
    for (size_t i = 0; i < t->nnewq; i++) {
        while (j < ts->nq && ts->q[j].op < t->newq[i].op) {
            j++;
        }
        if (j < ts->nq && ts->q[j].op == t->newq[i].op) {
            t->newq[i].recs = ts->q[j].recs;
        }
    }
    free(ts->q);
    ts->q = t->newq;
    ts->nq = t->nnewq;
    t->newq = NULL;
    t->nnewq = t->newq_cap = 0;
}

/* Not inlined, so that the list's address is in no register or stack slot
   of the caller */
static Py_NO_INLINE int
make_list_in(PyObject **slot)
{
    PyObject *list = PyList_New(0);
    if (list == NULL || PyList_Append(list, Py_None) < 0) {
        return -1;
    }
    *slot = list;
    return 0;
}

/* Overwrite the stack below the caller, where make_list_in() left copies.
   Not instrumented under ASan, whose redzones around buf would keep them. */
static Py_NO_INLINE void _Py_NO_SANITIZE_ADDRESS
scrub_stack(void)
{
    volatile char buf[16384];
    for (size_t i = 0; i < sizeof(buf); i++) {
        buf[i] = 0;
    }
}

Py_NO_INLINE PyObject *
_PyGC_TestHoldInCoreMem(PyObject *callable, int give_back)
{
    PyObject **block = PyMem_Calloc(4, sizeof(PyObject *));
    if (block == NULL) {
        return PyErr_NoMemory();
    }
    if (make_list_in(&block[1]) < 0) {
        PyMem_Free(block);
        return NULL;
    }
    scrub_stack();
    PyObject *r = PyObject_CallNoArgs(callable);
    PyObject *list = block[1];
    PyMem_Free(block);
    if (r == NULL) {
        return NULL;
    }
    return give_back ? list : Py_None;
}

/* The object that _PyGC_TestBorrowAcross() borrows: list[0], or the target
   of a weak reference, read from wr_object as C code that borrows it does. */
static inline PyObject *
borrowed_item(PyObject *container, int weak)
{
    if (weak) {
        return ((PyWeakReference *)container)->wr_object;
    }
    return PyList_GET_ITEM(container, 0);
}

/* The counterpart without the span: this frame registers itself precise
   while it runs, so that nothing but the borrow holds the object, and the
   C mode, which checks only marked functions, never sees it. */
static Py_NO_INLINE __attribute__((optimize("no-optimize-sibling-calls")))
PyObject *
borrow_across_bare(PyObject *container, PyObject *callback, int weak)
{
    _PyGCFrameDesc d;
    if (_PyGC_DecodeDescriptor((uintptr_t)&borrow_across_bare, &d) < 0
        || _PyGC_RegisterCodeRange(d.func_start, d.func_end,
                                   _PyGC_FRAME_PRECISE, _PyGC_RANGE_TEST,
                                   NULL) < 0)
    {
        PyErr_SetString(PyExc_RuntimeError, "no stack walk");
        return NULL;
    }
    PyObject *res = NULL;
    PyObject *item = borrowed_item(container, weak);
    PyObject *r = PyObject_CallNoArgs(callback);
    if (r != NULL) {
        PyGC_Collect();
        res = PyUnicode_FromString(Py_TYPE(item)->tp_name);
    }
    _PyGC_UnregisterCodeRange(d.func_start);
    return res;
}

Py_GC_PRECISE PyObject *
_PyGC_TestBorrowAcross(PyObject *container, PyObject *callback, int kind)
{
    int weak = kind & _PyGC_BORROW_WEAK;
    if (kind & _PyGC_BORROW_BARE) {
        return borrow_across_bare(container, callback, weak);
    }
    PyThreadState *tstate = _PyThreadState_GET();
    // The span: callback, held while list[0] may be boxed, and the borrow
    PyValue slots[2];
    _PyValueRoot span;
    _PyThreadState_PushValueSpan(tstate, &span, slots);
    _PyValueSpan_AppendObject(&span, callback);
    PyObject *item = borrowed_item(container, weak);
    _PyValueSpan_AppendObject(&span, item);
    PyObject *res = NULL;
    PyObject *r = PyObject_CallNoArgs(callback);
    if (r != NULL) {
        PyGC_Collect();
        res = PyUnicode_FromString(Py_TYPE(item)->tp_name);
    }
    _PyThreadState_PopValueRoot(tstate, &span);
    return res;
}

int
_PyGC_TraceLiveMark(PyThreadState *tstate, _PyGCMarkResult *res)
{
    *res = (_PyGCMarkResult){0};
    struct _gc_trace_state *ts = tstate->interp->gc.trace;
    int verify = ts != NULL ? ts->live_verify : VERIFY_OFF;
    int verify_ex = verify ? ts->live_verify_ex : 0;
    trace_t t = {
        .interp = tstate->interp,
        .ts = ts,
        .live = 1,
        // kill:structs and retain:structs make the struct scan B's
        .roots = (verify_ex & VERIFY_EX_STRUCTS)
                 ? _PyGC_TRACE_ROOT_ALL & ~_PyGC_TRACE_ROOT_STRUCTS
                 : _PyGC_TRACE_ROOT_ALL,
        .debug_offset = _PyMem_DebugEnabled() ? 2 * sizeof(size_t) : 0,
        .stats = &res->roots,
        .walk_cache = _PyGC_LiveWalkCache(tstate->interp),
        .verify = verify,
        .verify_ex = verify_ex,
    };
    int rc = -1;
    if (addrset_init(&t.mem_scanned, 256) < 0) {
        goto done;
    }
    HEAD_LOCK(&_PyRuntime);
#ifdef Py_DEBUG
    /* Before the page lists are built: a release can free a whole page,
       and B's lookups would read it through a stale range.  The blocks of
       the collection QUARANTINE_COLLECTIONS before this one go. */
    if (verify == VERIFY_KILL) {
        quarantine_release(ts, ts->live_collections + 1
                               - QUARANTINE_COLLECTIONS);
    }
#endif
    if (live_domain(&t, tstate->interp) < 0
        || live_build_pages(&t) < 0
        || (live_other_threads(&t, tstate) && read_mappings(&t) < 0))
    {
        HEAD_UNLOCK(&_PyRuntime);
        goto done;
    }
    if (ts != NULL && ts->stack_hits_on) {
        ts->stack_hits_collection++;
    }
    live_mark_roots(&t, tstate);
    if (t.verify) {
        verify_mark_b(&t, tstate);
    }
    /* The walk clears every mark, also after a failure, when it lists
       nothing. */
    struct live_walk w = {.t = &t, .res = res, .failed = t.oom};
    PyInterpreterState *interp = tstate->interp;
    // CPython's own dict watchers hear of a deallocation in the sweep.
    for (int i = FIRST_AVAILABLE_WATCHER; i < DICT_MAX_WATCHERS; i++) {
        if (interp->dict_state.watchers[i] != NULL) {
            w.dict_watchers |= 1 << i;
        }
    }
    for (int i = 0; i < TYPE_MAX_WATCHERS; i++) {
        if (interp->type_watchers[i] != NULL) {
            w.type_watchers |= 1 << i;
        }
    }
    int walked = live_walk_heaps(&t, &w);
    if (walked >= 0 && !w.failed) {
        live_clear_weak_caches(&t);
    }
    HEAD_UNLOCK(&_PyRuntime);
    res->nmarked = t.nmarked;
    res->nshallow = t.nshallow;
    if (walked < 0 || w.failed) {
        for (Py_ssize_t i = 0; i < res->ndead; i++) {
            res->dead[i].op->ob_flags &= ~_Py_GC_DEAD_FLAG;
        }
        free(res->dead);
        res->dead = NULL;
        res->ndead = 0;
        res->kinds = 0;
        goto done;
    }
    rc = 0;
done:
    live_fini(&t);
    return rc;
}

const _PyGCStackHit *
_PyGC_TraceStackHits(PyInterpreterState *interp, size_t *n, int64_t *dropped)
{
    struct _gc_trace_state *ts = interp->gc.trace;
    if (ts == NULL || !ts->stack_hits_on) {
        *n = 0;
        *dropped = 0;
        return NULL;
    }
    *n = ts->nstack_hits;
    *dropped = ts->stack_hits_dropped;
    return ts->stack_hits;
}

void
_PyGC_TraceClearStackHits(PyInterpreterState *interp)
{
    struct _gc_trace_state *ts = interp->gc.trace;
    if (ts != NULL) {
        ts->nstack_hits = 0;
        ts->stack_hits_dropped = 0;
    }
}

void
_PyGC_TraceOwnerOf(uintptr_t pc, char *buf, size_t size)
{
#ifdef HAVE_DLFCN_H
    Dl_info info;
    if (pc != 0 && dladdr((void *)pc, &info) && info.dli_fname != NULL) {
        if (info.dli_sname != NULL) {
            PyOS_snprintf(buf, size, "%s+%#lx (%s+%#lx)", info.dli_fname,
                          (unsigned long)(pc - (uintptr_t)info.dli_fbase),
                          info.dli_sname,
                          (unsigned long)(pc - (uintptr_t)info.dli_saddr));
        }
        else {
            PyOS_snprintf(buf, size, "%s+%#lx", info.dli_fname,
                          (unsigned long)(pc - (uintptr_t)info.dli_fbase));
        }
        return;
    }
#endif
    PyOS_snprintf(buf, size, pc == 0 ? "no frame" : "heap");
}

/* For tests of the segment map: the object that the mark finds at addr, or
   that contains it if interior is set, or 0. */
uintptr_t
_PyGC_TraceLiveLookup(PyThreadState *tstate, uintptr_t addr, int interior)
{
    trace_t t = {
        .interp = tstate->interp,
        .live = 1,
        .debug_offset = _PyMem_DebugEnabled() ? 2 * sizeof(size_t) : 0,
    };
    HEAD_LOCK(&_PyRuntime);
    int ok = live_domain(&t, tstate->interp) == 0;
    HEAD_UNLOCK(&_PyRuntime);
    return ok ? (uintptr_t)live_lookup(&t, addr, interior) : 0;
}

/* For tests of the segment map: the segment that the map holds for addr,
   or 0. */
uintptr_t
_PyGC_TraceSegmentOf(uintptr_t addr)
{
    return (uintptr_t)_mi_gc_segment_of(addr);
}

/* For tests of the segment map: addresses in op's segment that hold no
   block, or 0 where the segment has none: its start, the last word of its
   metadata, a word in a free span, and the first word past the blocks that
   op's page has handed out. */
void
_PyGC_TraceSegmentProbes(PyObject *op, uintptr_t probes[4])
{
    mi_segment_t *seg = _mi_ptr_segment(op);
    size_t info = seg->segment_info_slices;
    probes[0] = (uintptr_t)seg;
    probes[1] = (uintptr_t)seg + info * MI_SEGMENT_SLICE_SIZE - sizeof(void *);
    probes[2] = 0;
    if (seg->kind != MI_SEGMENT_HUGE) {
        for (size_t i = info; i < seg->slice_entries;) {
            mi_slice_t *s = &seg->slices[i];
            if (s->block_size == 0) {
                // The span's last slice, whose back offset is stale
                probes[2] = (uintptr_t)seg
                    + (i + s->slice_count - 1) * MI_SEGMENT_SLICE_SIZE + 64;
                break;
            }
            i += s->slice_count;
        }
    }
    mi_page_t *page = _mi_ptr_page(op);
    probes[3] = 0;
    if (page->capacity < page->reserved) {
        probes[3] = (uintptr_t)page->page_start
            + page->capacity * page->block_size;
    }
}

/* For finding out why an object is kept: mark from each class of roots
   alone and report the classes that reach addr.  Nothing is freed. */
int
_PyGC_TraceLiveRootsOf(PyThreadState *tstate, uintptr_t addr)
{
    volatile uintptr_t hidden = addr;
    return _PyGC_TraceLiveRootsOfHidden(tstate, &hidden, 0);
}

/* The same for the object at *hidden ^ mask, which is read only after each
   mark: the caller's own stack and registers then need not hold it. */
int
_PyGC_TraceLiveRootsOfHidden(PyThreadState *tstate,
                             const volatile uintptr_t *hidden, uintptr_t mask)
{
    int found = 0;
    for (int bit = 0; bit < 8; bit++) {
        trace_t t = {
            .interp = tstate->interp,
            .live = 1,
            .roots = 1u << bit,
            .debug_offset = _PyMem_DebugEnabled() ? 2 * sizeof(size_t) : 0,
            .walk_cache = _PyGC_LiveWalkCache(tstate->interp),
        };
        if (addrset_init(&t.mem_scanned, 256) < 0) {
            return -1;
        }
        HEAD_LOCK(&_PyRuntime);
        if (live_domain(&t, tstate->interp) < 0
            || live_build_pages(&t) < 0
            || (live_other_threads(&t, tstate) && read_mappings(&t) < 0))
        {
            HEAD_UNLOCK(&_PyRuntime);
            live_fini(&t);
            return -1;
        }
        live_mark_roots(&t, tstate);
        PyObject *op = live_lookup(&t, *hidden ^ mask, 0);
        if (op != NULL && (op->ob_flags & _Py_GC_MARKED_FLAG)) {
            found |= 1 << bit;
        }
        // Clear the marks
        _PyGCMarkResult res = {0};
        struct live_walk w = {.t = &t, .res = &res, .failed = 1};
        (void)live_walk_heaps(&t, &w);
        HEAD_UNLOCK(&_PyRuntime);
        live_fini(&t);
    }
    return found;
}

PyObject **
_PyGC_TraceLiveHeapObjects(PyInterpreterState *interp, Py_ssize_t *n)
{
    trace_t t = {
        .interp = interp,
        .live = 1,
        .debug_offset = _PyMem_DebugEnabled() ? 2 * sizeof(size_t) : 0,
    };
    struct live_walk w = {.t = &t};
    HEAD_LOCK(&_PyRuntime);
    int rc = live_domain(&t, interp);
    if (rc == 0) {
        rc = live_walk_heaps(&t, &w);
    }
    HEAD_UNLOCK(&_PyRuntime);
    if (rc < 0) {
        free(w.objs);
        *n = -1;
        return NULL;
    }
    *n = (Py_ssize_t)w.nobjs;
    return w.objs;
}


/* --- holes ------------------------------------------------------------------ */

typedef struct {
    trace_t *t;
    addrset holes;
    trace_hole *list;
    Py_ssize_t n;
    PyObject *referrer;
} referrer_search;

static trace_hole *
find_hole(referrer_search *s, uintptr_t addr)
{
    // The list is sorted by address.
    Py_ssize_t lo = 0, hi = s->n;
    while (lo < hi) {
        Py_ssize_t mid = lo + (hi - lo) / 2;
        if (s->list[mid].addr < addr) {
            lo = mid + 1;
        }
        else {
            hi = mid;
        }
    }
    if (lo < s->n && s->list[lo].addr == addr) {
        return &s->list[lo];
    }
    return NULL;
}

static int
referrer_visit(PyObject *op, void *arg)
{
    referrer_search *s = arg;
    if (op == NULL || !addrset_has(&s->holes, (uintptr_t)op)) {
        return 0;
    }
    trace_hole *h = find_hole(s, (uintptr_t)op);
    if (h == NULL || h->has_referrer) {
        return 0;
    }
    PyObject *r = s->referrer;
    PyOS_snprintf(h->referrer, sizeof(h->referrer), "%.80s at %p (%s)",
                  Py_TYPE(r)->tp_name, (void *)r,
                  addrset_has(&s->t->marked, (uintptr_t)r) ? "marked" : "hole");
    h->has_referrer = 1;
    return 0;
}

static bool
mem_block_visit(const mi_heap_t *heap, const mi_heap_area_t *area,
                void *block, size_t block_size, void *arg)
{
    referrer_search *s = arg;
    if (block == NULL) {
        return true;
    }
    uintptr_t *p = block;
    uintptr_t *end = (uintptr_t *)((char *)block + block_size);
    for (; p < end; p++) {
        uintptr_t op = heap_lookup(s->t, *p);
        if (op == 0 || !addrset_has(&s->holes, op)) {
            continue;
        }
        trace_hole *h = find_hole(s, op);
        if (h != NULL && !h->has_referrer) {
            PyOS_snprintf(h->referrer, sizeof(h->referrer),
                          "non-object block at %p (%zu bytes)",
                          block, block_size);
            h->has_referrer = 1;
            h->block = (uintptr_t)block;
            h->block_size = block_size;
        }
    }
    return true;
}

static int
hole_cmp(const void *a, const void *b)
{
    uintptr_t x = ((const trace_hole *)a)->addr;
    uintptr_t y = ((const trace_hole *)b)->addr;
    return (x > y) - (x < y);
}

/* An unmarked object is not necessarily a hole.  Code that runs during the
   collection, such as finalizers, weakref callbacks or another thread that
   takes the GIL while they run, can create cyclic garbage that the
   collection did not see.  Such garbage is unreachable, so a tracing
   collector would rightly free it.  As the cycle collector does, subtract
   from each unmarked object's reference count the references that other
   unmarked objects hold; an object with references left is held from
   outside the traced graph, and it and everything it reaches are holes.
   The rest is garbage.

   Objects whose reference count is already zero are being deallocated: a
   finalizer that a dealloc function triggers can run a collection while
   the dying container still holds the members it has not released yet.
   Their references count as garbage references too.  Their traversal only
   looks addresses up, without following them, so that members already
   released are harmless. */

typedef struct {
    referrer_search *s;
    trace_hole **stack;
    Py_ssize_t sp;
} held_search;

static int
subtract_visit(PyObject *op, void *arg)
{
    trace_hole *h = find_hole((referrer_search *)arg, (uintptr_t)op);
    if (h != NULL) {
        h->external--;
    }
    return 0;
}

static int
held_visit(PyObject *op, void *arg)
{
    held_search *hs = (held_search *)arg;
    trace_hole *h = find_hole(hs->s, (uintptr_t)op);
    if (h != NULL && !h->held) {
        h->held = 1;
        hs->stack[hs->sp++] = h;
    }
    return 0;
}

/* Remove the garbage from the sorted list and return the number left. */
static Py_ssize_t
remove_garbage(trace_t *t, trace_hole *list, Py_ssize_t n)
{
    referrer_search s = {.t = t, .list = list, .n = n};
    for (Py_ssize_t i = 0; i < n; i++) {
        PyObject *op = (PyObject *)list[i].addr;
        if (safe_to_traverse(op)) {
            (void)_PyGC_TraceTraverse(op, subtract_visit, &s);
        }
    }
    for (size_t i = 0; i <= t->objects.mask; i++) {
        uintptr_t addr = t->objects.keys[i];
        if (addr == 0) {
            continue;
        }
        PyObject *op = (PyObject *)addr;
        if (Py_REFCNT(op) != 0 || !is_object(t, op, 1) || !_PyObject_IS_GC(op)) {
            continue;
        }
        /* A dying object is still referenced from its dealloc function's C
         * stack frame, so the conservative scan marked it without
         * traversing it.  Those the mark traversed already marked their
         * members. */
        if (!addrset_has(&t->shallow, addr) || addrset_has(&t->deepened, addr)) {
            continue;
        }
        (void)_PyGC_TraceTraverse(op, subtract_visit, &s);
    }
    held_search hs = {.s = &s, .stack = malloc(n * sizeof(trace_hole *))};
    if (hs.stack == NULL) {
        return n;  // report everything
    }
    for (Py_ssize_t i = 0; i < n; i++) {
        if (list[i].external > 0) {
            list[i].held = 1;
            hs.stack[hs.sp++] = &list[i];
        }
    }
    // Each hole is pushed at most once, so the stack cannot overflow.
    while (hs.sp > 0) {
        PyObject *op = (PyObject *)hs.stack[--hs.sp]->addr;
        if (safe_to_traverse(op)) {
            (void)_PyGC_TraceTraverse(op, held_visit, &hs);
        }
    }
    free(hs.stack);
    Py_ssize_t kept = 0;
    for (Py_ssize_t i = 0; i < n; i++) {
        if (list[i].held) {
            list[kept++] = list[i];
        }
    }
    t->ngarbage = n - kept;
    return kept;
}

/* Find the first referrer of each hole: an object whose tp_traverse visits
   it, or else a block of PyMem_Malloc() memory that holds its address. */
typedef struct {
    uintptr_t start;
    uintptr_t end;
    uintptr_t owner;
} owned_block;

static int
owned_block_cmp(const void *a, const void *b)
{
    uintptr_t x = ((const owned_block *)a)->start;
    uintptr_t y = ((const owned_block *)b)->start;
    return x < y ? -1 : x > y;
}

/* Name an object that points into each non-object block named as a
   referrer: the block is usually storage the object owns, such as a list's
   items or a parser's stack. */
static void
find_block_owners(trace_t *t, trace_hole *list, Py_ssize_t n)
{
    enum { MAX_BLOCKS = 64 };
    owned_block blocks[MAX_BLOCKS];
    size_t nblocks = 0;
    for (Py_ssize_t i = 0; i < n && nblocks < MAX_BLOCKS; i++) {
        if (list[i].block == 0) {
            continue;
        }
        size_t j = 0;
        while (j < nblocks && blocks[j].start != list[i].block) {
            j++;
        }
        if (j == nblocks) {
            blocks[nblocks++] = (owned_block){
                list[i].block, list[i].block + list[i].block_size, 0};
        }
    }
    if (nblocks == 0) {
        return;
    }
    qsort(blocks, nblocks, sizeof(owned_block), owned_block_cmp);
    for (size_t i = 0; i <= t->objects.mask; i++) {
        uintptr_t op = t->objects.keys[i];
        if (op == 0) {
            continue;
        }
        const range_t *r = rangelist_find(&t->pages, op);
        if (r == NULL) {
            continue;
        }
        uintptr_t start = r->start + (op - r->start) / r->bsize * r->bsize;
        for (uintptr_t *w = (uintptr_t *)start;
             (uintptr_t)w < start + r->bsize; w++)
        {
            uintptr_t v = *w;
            if (v < blocks[0].start || v >= blocks[nblocks - 1].end) {
                continue;
            }
            size_t lo = 0, hi = nblocks;
            while (lo < hi) {
                size_t mid = lo + (hi - lo) / 2;
                if (blocks[mid].start <= v) {
                    lo = mid + 1;
                }
                else {
                    hi = mid;
                }
            }
            if (lo > 0 && v < blocks[lo - 1].end && blocks[lo - 1].owner == 0) {
                blocks[lo - 1].owner = op;
            }
        }
    }
    for (Py_ssize_t i = 0; i < n; i++) {
        for (size_t j = 0; list[i].block != 0 && j < nblocks; j++) {
            if (blocks[j].start != list[i].block || blocks[j].owner == 0) {
                continue;
            }
            PyObject *owner = (PyObject *)blocks[j].owner;
            size_t len = strlen(list[i].referrer);
            PyOS_snprintf(list[i].referrer + len, sizeof(list[i].referrer) - len,
                          " of %.60s at %p%s", Py_TYPE(owner)->tp_name,
                          (void *)owner,
                          addrset_has(&t->marked, blocks[j].owner) ? "" : " (unmarked)");
        }
    }
}

static void
find_referrers(trace_t *t, trace_hole *list, Py_ssize_t n)
{
    referrer_search s = {.t = t, .list = list, .n = n};
    if (addrset_init(&s.holes, (size_t)n) < 0) {
        return;
    }
    for (Py_ssize_t i = 0; i < n; i++) {
        if (addrset_add(&s.holes, list[i].addr) < 0) {
            addrset_fini(&s.holes);
            return;
        }
    }
    for (size_t i = 0; i <= t->objects.mask; i++) {
        uintptr_t op = t->objects.keys[i];
        if (op == 0 || !is_object(t, (PyObject *)op, 1)) {
            continue;
        }
        if (addrset_has(&t->shallow, op) && !addrset_has(&t->deepened, op)) {
            continue;
        }
        if (!addrset_has(&t->marked, op) && !safe_to_traverse((PyObject *)op)) {
            continue;
        }
        s.referrer = (PyObject *)op;
        (void)_PyGC_TraceTraverse((PyObject *)op, referrer_visit, &s);
    }
    HEAD_LOCK(&_PyRuntime);
    _Py_FOR_EACH_TSTATE_UNLOCKED(t->interp, p) {
        struct _mimalloc_thread_state *m = &((_PyThreadStateImpl *)p)->mimalloc;
        if (m->current_object_heap != NULL) {
            mi_heap_visit_blocks(&m->heaps[_Py_MIMALLOC_HEAP_MEM], true,
                                 mem_block_visit, &s);
        }
    }
    mi_abandoned_visit_blocks(t->interp->mimalloc.subproc,
                              _Py_MIMALLOC_HEAP_MEM, true, mem_block_visit, &s);
    HEAD_UNLOCK(&_PyRuntime);
    for (Py_ssize_t i = 0; i < n; i++) {
        if (!list[i].has_referrer) {
            strcpy(list[i].referrer, "no heap referrer: C-held");
        }
    }
    find_block_owners(t, list, n);
    addrset_fini(&s.holes);
}

static Py_ssize_t
find_holes(trace_t *t)
{
    Py_ssize_t n = 0, cap = 0;
    trace_hole *list = NULL;
    for (size_t i = 0; i <= t->objects.mask; i++) {
        uintptr_t addr = t->objects.keys[i];
        if (addr == 0 || addrset_has(&t->marked, addr)) {
            continue;
        }
        PyObject *op = (PyObject *)addr;
        if (Py_REFCNT(op) == 0 || _Py_IsImmortal(op) || !is_object(t, op, 1)) {
            continue;
        }
        if (n == cap) {
            cap = cap ? cap * 2 : 64;
            trace_hole *l = realloc(list, cap * sizeof(trace_hole));
            if (l == NULL) {
                free(list);
                return -1;
            }
            list = l;
        }
        trace_hole *h = &list[n++];
        memset(h, 0, sizeof(*h));
        h->addr = addr;
        h->refcnt = h->external = Py_REFCNT(op);
        PyOS_snprintf(h->type_name, sizeof(h->type_name), "%.79s",
                      Py_TYPE(op)->tp_name);
        // Reading the characters of an ASCII string allocates nothing.
        if (PyUnicode_CheckExact(op) && PyUnicode_IS_ASCII(op)) {
            PyOS_snprintf(h->value, sizeof(h->value), " '%.40s'",
                          (const char *)PyUnicode_DATA(op));
            for (char *c = h->value + 2; *c != '\0'; c++) {
                if (*c < ' ' || *c > '~') {
                    *c = '?';
                }
            }
        }
    }
    if (n > 0) {
        qsort(list, n, sizeof(trace_hole), hole_cmp);
        n = remove_garbage(t, list, n);
    }
    if (n > 0) {
        find_referrers(t, list, n);
    }
    free(t->ts->holes);
    t->ts->holes = list;
    t->ts->nholes = n;
    return n;
}

static void
report_to(FILE *out, trace_t *t, Py_ssize_t nholes)
{
    Py_ssize_t unheaped = _PyMem_MiUnheapedAllocs();
    fprintf(out,
            "gc verify: %zd holes; %zd objects, %zd marked (%zd shallow), "
            "%zd non-object references, %zd allocations outside the heaps, "
            "%zd unreachable garbage, %zd mem blocks scanned\n",
            nholes, t->nobjects, t->nmarked, t->nshallow, t->nrejected,
            unheaped, t->ngarbage, t->nmemblocks);
    Py_ssize_t limit = 50;
    const char *env = getenv("PYTHON_GCVERIFY_LIMIT");
    if (env != NULL && *env != '\0') {
        limit = atol(env);
    }
    /* The holes with references that no other hole explains come first:
       each group of holes hangs from one of them.  Then holes whose
       referrer is not a hole, then the rest. */
    Py_ssize_t shown = 0;
    for (int pass = 0; pass < 3; pass++) {
        for (Py_ssize_t i = 0; i < nholes && shown < limit; i++) {
            trace_hole *h = &t->ts->holes[i];
            int group = h->external > 0 ? 0
                        : strstr(h->referrer, "(hole)") == NULL ? 1 : 2;
            if (group != pass) {
                continue;
            }
            fprintf(out, "gc verify: hole %s%s at %p refcnt %zd; referrer: %s",
                    h->type_name, h->value, (void *)h->addr, h->refcnt,
                    h->referrer);
            if (h->external > 0) {
                fprintf(out, "; %zd references from outside the holes",
                        h->external);
            }
            fputc('\n', out);
            shown++;
        }
    }
    if (shown < nholes) {
        fprintf(out, "gc verify: ... and %zd more\n", nholes - shown);
    }
    fflush(out);
}

/* Write an ASCII str's characters, which needs no allocation. */
static void
write_ascii(FILE *out, PyObject *s)
{
    if (s != NULL && PyUnicode_Check(s) && PyUnicode_IS_ASCII(s)) {
        fprintf(out, "%.200s", (const char *)PyUnicode_DATA(s));
    }
    else {
        fputs("?", out);
    }
}

/* Report to stderr and, if PYTHON_GCVERIFY_LOG names a file, append the
   report there too, with the process and the Python function that ran the
   collection, so that the holes of a whole test run can be collected. */
static void
report(trace_t *t, PyThreadState *tstate, Py_ssize_t nholes)
{
    if (nholes == 0) {
        return;
    }
    report_to(stderr, t, nholes);
    const char *path = getenv("PYTHON_GCVERIFY_LOG");
    if (path == NULL || *path == '\0') {
        return;
    }
    FILE *log = fopen(path, "a");
    if (log == NULL) {
        return;
    }
    // The innermost Python functions, then the innermost test method
    fprintf(log, "== pid %ld in", (long)getpid());
    int depth = 0;
    for (_PyInterpreterFrame *f = tstate->current_frame; f != NULL;
         f = f->previous)
    {
        if (f->owner >= FRAME_OWNED_BY_INTERPRETER
            || !PyCode_Check(PyValue_ObjectBorrow(f->f_executable)))
        {
            continue;
        }
        PyCodeObject *co = _PyFrame_GetCode(f);
        int is_test = PyUnicode_IS_ASCII(co->co_name)
            && strncmp((const char *)PyUnicode_DATA(co->co_name), "test", 4) == 0;
        if (depth >= 4 && !is_test) {
            continue;
        }
        fputs(depth ? ", from " : " ", log);
        write_ascii(log, co->co_qualname);
        fputs(" at ", log);
        write_ascii(log, co->co_filename);
        depth++;
        if (is_test) {
            break;
        }
    }
    // A test worker's command line names its test.
    PyObject *argv = PyDict_GetItemWithError(tstate->interp->sysdict,
                                             &_Py_ID(argv));
    if (argv != NULL && PyList_Check(argv)) {
        fputs("; argv", log);
        for (Py_ssize_t i = 0; i < PyList_GET_SIZE(argv); i++) {
            fputs(" ", log);
            write_ascii(log, PyList_GET_ITEM(argv, i));
        }
    }
    fputs("\n", log);
    report_to(log, t, nholes);
    fclose(log);
}


/* --- driver ------------------------------------------------------------------ */

static struct _gc_trace_state *
get_trace_state(PyInterpreterState *interp)
{
    if (interp->gc.trace == NULL) {
        struct _gc_trace_state *ts = calloc(1, sizeof(*ts));
        if (ts == NULL) {
            return NULL;
        }
        if (addrset_init(&ts->static_type_set, 512) < 0) {
            free(ts);
            return NULL;
        }
        interp->gc.trace = ts;
    }
    return interp->gc.trace;
}

static unsigned int
roots_from_env(void)
{
    const char *env = getenv("PYTHON_GCVERIFY_ROOTS");
    if (env == NULL || *env == '\0') {
        return _PyGC_TRACE_ROOT_ALL;
    }
    if (strcmp(env, "precise") == 0) {
        return _PyGC_TRACE_ROOT_PRECISE;
    }
    return (unsigned int)strtoul(env, NULL, 0);
}

static void
trace_fini(trace_t *t)
{
    addrset_fini(&t->mem_scanned);
    free(t->mem_stack);
    addrset_fini(&t->objects);
    addrset_fini(&t->marked);
    addrset_fini(&t->shallow);
    addrset_fini(&t->deepened);
    addrset_fini(&t->good_types);
    rangelist_fini(&t->pages);
    rangelist_fini(&t->mem_pages);
    rangelist_fini(&t->readable);
    free(t->stack);
}

static Py_ssize_t
verify(PyThreadState *tstate)
{
    PyInterpreterState *interp = tstate->interp;
    if (!_PyMem_MimallocEnabled()) {
        return -1;
    }
    /* Finalization detaches the thread states of daemon threads, whose
       frames keep their objects for good, and tears the roots down. */
    if (_PyInterpreterState_GetFinalizing(interp) != NULL
        || _PyRuntimeState_GetFinalizing(&_PyRuntime) != NULL)
    {
        return -1;
    }
    struct _gc_trace_state *ts = get_trace_state(interp);
    if (ts == NULL || ts->verifying) {
        return -1;
    }
    ts->verifying = 1;
    trace_t t = {
        .interp = interp,
        .ts = ts,
        .roots = roots_from_env(),
        .debug_offset = _PyMem_DebugEnabled() ? 2 * sizeof(size_t) : 0,
    };
    Py_ssize_t nholes = -1;
    if (addrset_init(&t.objects, 1 << 16) < 0
        || addrset_init(&t.marked, 1 << 16) < 0
        || addrset_init(&t.shallow, 1024) < 0
        || addrset_init(&t.deepened, 1024) < 0
        || addrset_init(&t.good_types, 4096) < 0
        || addrset_init(&t.mem_scanned, 256) < 0
        || read_mappings(&t) < 0
        || walk_heaps(&t) < 0)
    {
        goto done;
    }
    HEAD_LOCK(&_PyRuntime);
    mark_precise_roots(&t);
    if (t.roots & _PyGC_TRACE_ROOT_STACKS) {
        scan_threads(&t, tstate);
    }
    if (t.roots & _PyGC_TRACE_ROOT_DATA) {
        scan_data_segments(&t);
        scan_queued(&t);
    }
    if (t.roots & _PyGC_TRACE_ROOT_STRUCTS) {
        scan_structs(&t);
        scan_queued(&t);
    }
    HEAD_UNLOCK(&_PyRuntime);
    if (t.oom) {
        goto done;
    }
    /* Nothing is counted, so there are no holes to find.  The check is that
       the collector freed nothing the mark reaches. */
    (void)find_holes;
    (void)report;
    nholes = t.ndangling;
    if (nholes > 0) {
        fprintf(stderr, "gc verify: %zd dangling references; %zd objects, "
                "%zd marked\n", nholes, t.nobjects, t.nmarked);
        for (Py_ssize_t i = 0;
             i < Py_MIN(nholes, (Py_ssize_t)Py_ARRAY_LENGTH(t.dangling)); i++)
        {
            fprintf(stderr, "gc verify: dangling %p from %s\n",
                    (void *)t.dangling[i].addr, t.dangling[i].referrer);
        }
        fflush(stderr);
    }
done:
    trace_fini(&t);
    ts->verifying = 0;
    ts->last_result = nholes;
    if (nholes >= 0) {
        ts->count++;
    }
    return nholes;
}

void
_PyGC_TraceAfterCollect(PyThreadState *tstate, int generation, int manual)
{
    PyInterpreterState *interp = tstate->interp;
    int mode = interp->gc.trace_mode;
    struct _gc_trace_state *ts = interp->gc.trace;
    int force = ts != NULL && ts->force;
    if (!force) {
        if (mode == _PyGC_TRACE_OFF) {
            return;
        }
        if (manual ? generation != NUM_GENERATIONS - 1
                   : (mode != _PyGC_TRACE_AUTO || generation < 1)) {
            return;
        }
    }
    (void)verify(tstate);
}

Py_ssize_t
_PyGC_TraceVerify(PyThreadState *tstate)
{
    struct _gc_trace_state *ts = get_trace_state(tstate->interp);
    if (ts == NULL) {
        return -1;
    }
    ts->last_result = -1;
    ts->force = 1;
    (void)_PyGC_Collect(tstate, NUM_GENERATIONS - 1, _Py_GC_REASON_MANUAL);
    ts->force = 0;
    return ts->last_result;
}

Py_ssize_t
_PyGC_TraceCount(PyInterpreterState *interp)
{
    return interp->gc.trace == NULL ? 0 : interp->gc.trace->count;
}

PyObject *
_PyGC_TraceGetHoles(PyInterpreterState *interp)
{
    PyObject *list = PyList_New(0);
    if (list == NULL) {
        return NULL;
    }
    struct _gc_trace_state *ts = interp->gc.trace;
    if (ts == NULL) {
        return list;
    }
    for (Py_ssize_t i = 0; i < ts->nholes; i++) {
        trace_hole *h = &ts->holes[i];
        PyObject *item = Py_BuildValue("(sKns)", h->type_name,
                                       (unsigned long long)h->addr,
                                       h->refcnt, h->referrer);
        if (item == NULL || PyList_Append(list, item) < 0) {
            Py_XDECREF(item);
            Py_DECREF(list);
            return NULL;
        }
        Py_DECREF(item);
    }
    return list;
}

PyTypeObject **
_PyGC_TraceStaticTypes(PyInterpreterState *interp, Py_ssize_t *n)
{
    struct _gc_trace_state *ts = interp->gc.trace;
    if (ts == NULL) {
        *n = 0;
        return NULL;
    }
    *n = ts->n_static_types;
    return ts->static_types;
}

void
_PyGC_TraceRegisterStaticType(PyInterpreterState *interp, PyTypeObject *type)
{
    struct _gc_trace_state *ts = get_trace_state(interp);
    if (ts == NULL) {
        return;
    }
    int r = addrset_add(&ts->static_type_set, (uintptr_t)type);
    if (r <= 0) {
        return;
    }
    if (ts->n_static_types == ts->static_types_cap) {
        Py_ssize_t cap = ts->static_types_cap ? ts->static_types_cap * 2 : 256;
        PyTypeObject **types = realloc(ts->static_types,
                                       cap * sizeof(PyTypeObject *));
        if (types == NULL) {
            return;
        }
        ts->static_types = types;
        ts->static_types_cap = cap;
    }
    ts->static_types[ts->n_static_types++] = type;
}

/* "retain" or "kill", then optionally ":" and a comma-separated list of
   "pymem" and "structs" */
static int
parse_live_verify(const char *env, int *mode, int *ex)
{
    size_t n = strcspn(env, ":");
    if (n == 6 && strncmp(env, "retain", 6) == 0) {
        *mode = VERIFY_RETAIN;
    }
    else if (n == 4 && strncmp(env, "kill", 4) == 0) {
        *mode = VERIFY_KILL;
    }
    else {
        return -1;
    }
    *ex = 0;
    if (env[n] == '\0') {
        return 0;
    }
    const char *p = env + n + 1;
    for (;;) {
        size_t m = strcspn(p, ",");
        if (m == 5 && strncmp(p, "pymem", 5) == 0) {
            *ex |= VERIFY_EX_PYMEM;
        }
        else if (m == 7 && strncmp(p, "structs", 7) == 0) {
            *ex |= VERIFY_EX_STRUCTS;
        }
        else {
            return -1;
        }
        if (p[m] == '\0') {
            return 0;
        }
        p += m + 1;
    }
}

static int
parse_mode(const wchar_t *value)
{
    if (value == NULL || wcscmp(value, L"1") == 0) {
        return _PyGC_TRACE_EXPLICIT;
    }
    if (wcscmp(value, L"auto") == 0) {
        return _PyGC_TRACE_AUTO;
    }
    if (wcscmp(value, L"0") == 0) {
        return _PyGC_TRACE_OFF;
    }
    return _PyGC_TRACE_EXPLICIT;
}

PyStatus
_PyGC_TraceInit(PyInterpreterState *interp)
{
#ifdef Py_DEBUG
    int mode = _PyGC_TRACE_EXPLICIT;
#else
    int mode = _PyGC_TRACE_OFF;
#endif
    int requested = 0;
    const PyConfig *config = &interp->config;
    int live_verify = VERIFY_OFF;
    int live_verify_ex = 0;
    if (config->use_environment) {
        const char *env = getenv("PYTHON_GCVERIFY");
        if (env != NULL && (strncmp(env, "retain", 6) == 0
                            || strncmp(env, "kill", 4) == 0))
        {
            if (parse_live_verify(env, &live_verify, &live_verify_ex) < 0) {
                return _PyStatus_ERR("PYTHON_GCVERIFY must be retain or "
                                     "kill, then optionally a colon and a "
                                     "comma-separated list of pymem and "
                                     "structs");
            }
#ifndef Py_DEBUG
            if (live_verify == VERIFY_KILL) {
                return _PyStatus_ERR("PYTHON_GCVERIFY=kill needs a debug "
                                     "build");
            }
#endif
        }
        else if (env != NULL && *env != '\0') {
            mode = strcmp(env, "auto") == 0 ? _PyGC_TRACE_AUTO
                 : strcmp(env, "0") == 0 ? _PyGC_TRACE_OFF
                 : _PyGC_TRACE_EXPLICIT;
        }
    }
    for (Py_ssize_t i = 0; i < config->xoptions.length; i++) {
        const wchar_t *opt = config->xoptions.items[i];
        if (wcscmp(opt, L"gcverify") == 0) {
            mode = _PyGC_TRACE_EXPLICIT;
            requested = 1;
        }
        else if (wcsncmp(opt, L"gcverify=", 9) == 0) {
            mode = parse_mode(opt + 9);
            requested = mode != _PyGC_TRACE_OFF;
        }
    }
    if (mode != _PyGC_TRACE_OFF && !_PyMem_MimallocEnabled()) {
        // The verifier needs every object in a mimalloc heap.  Asking for it
        // on the command line is an error; the environment variable and the
        // debug default are ignored, so that child processes run with
        // another PYTHONMALLOC still start.
        if (requested) {
            return _PyStatus_ERR("-X gcverify requires the mimalloc allocator");
        }
        mode = _PyGC_TRACE_OFF;
    }
    interp->gc.trace_mode = mode;
    if (live_verify != VERIFY_OFF) {
        struct _gc_trace_state *ts = get_trace_state(interp);
        if (ts == NULL) {
            return _PyStatus_NO_MEMORY();
        }
        ts->live_verify = live_verify;
        ts->live_verify_ex = live_verify_ex;
    }
    if (config->use_environment) {
        const char *env = getenv("PYTHON_GCSTACKHITS");
        if (env != NULL && *env != '\0' && strcmp(env, "0") != 0) {
            struct _gc_trace_state *ts = get_trace_state(interp);
            if (ts == NULL) {
                return _PyStatus_NO_MEMORY();
            }
            ts->stack_hits_on = 1;
        }
    }
    return _PyStatus_OK();
}

void
_PyGC_TraceFini(PyInterpreterState *interp)
{
    struct _gc_trace_state *ts = interp->gc.trace;
    if (ts == NULL) {
        return;
    }
    interp->gc.trace = NULL;
    free(ts->q);
    free(ts->killed);
#ifdef Py_DEBUG
    quarantine_release(ts, INT64_MAX);
    free(ts->quarantine);
#endif
    free(ts->static_types);
    addrset_fini(&ts->static_type_set);
    free(ts->holes);
    free(ts->stack_hits);
    free(ts);
}

/* The runtime's head lock, which the verifier holds while it marks the
   roots, protects the list, so that a thread without the GIL can remove
   its root. */
void
_PyGC_AddRoot(_PyGCRoot *root, PyObject **objs, Py_ssize_t n)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    root->interp = interp;
    root->objs = objs;
    root->n = n;
    root->prev = NULL;
    HEAD_LOCK(&_PyRuntime);
    root->next = interp->gc.trace_roots;
    if (root->next != NULL) {
        root->next->prev = root;
    }
    interp->gc.trace_roots = root;
    HEAD_UNLOCK(&_PyRuntime);
}

void
_PyGC_RemoveRoot(_PyGCRoot *root)
{
    HEAD_LOCK(&_PyRuntime);
    if (root->prev != NULL) {
        root->prev->next = root->next;
    }
    else {
        assert(root->interp->gc.trace_roots == root);
        root->interp->gc.trace_roots = root->next;
    }
    if (root->next != NULL) {
        root->next->prev = root->prev;
    }
    HEAD_UNLOCK(&_PyRuntime);
    root->prev = root->next = NULL;
}

/* The public pair: a root for one PyObject* variable.  The node comes from
   the raw allocator so that it is not itself in a scanned heap. */
int
PyGC_AddRoot(PyObject **ref)
{
    _PyGCRoot *root = PyMem_RawMalloc(sizeof(_PyGCRoot));
    if (root == NULL) {
        PyErr_NoMemory();
        return -1;
    }
    _PyGC_AddRoot(root, ref, 1);
    return 0;
}

void
PyGC_RemoveRoot(PyObject **ref)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    _PyGCRoot *found = NULL;
    HEAD_LOCK(&_PyRuntime);
    for (_PyGCRoot *r = interp->gc.trace_roots; r != NULL; r = r->next) {
        if (r->objs == ref && r->n == 1) {
            found = r;
            break;
        }
    }
    HEAD_UNLOCK(&_PyRuntime);
    if (found != NULL) {
        _PyGC_RemoveRoot(found);
        PyMem_RawFree(found);
    }
}

#else  /* !WITH_MIMALLOC */

int
PyGC_AddRoot(PyObject **ref)
{
    return 0;
}

void
PyGC_RemoveRoot(PyObject **ref)
{
}

PyTypeObject **
_PyGC_TraceStaticTypes(PyInterpreterState *interp, Py_ssize_t *n)
{
    *n = 0;
    return NULL;
}

void
_PyGC_AddRoot(_PyGCRoot *root, PyObject **objs, Py_ssize_t n)
{
}

void
_PyGC_RemoveRoot(_PyGCRoot *root)
{
}

/* The free-threaded build has its own collector; the verifier is for the
   default build. */

void
_PyGC_TraceAfterCollect(PyThreadState *tstate, int generation, int manual)
{
}

Py_ssize_t
_PyGC_TraceVerify(PyThreadState *tstate)
{
    return -1;
}

Py_ssize_t
_PyGC_TraceCount(PyInterpreterState *interp)
{
    return 0;
}

PyObject *
_PyGC_TraceGetHoles(PyInterpreterState *interp)
{
    return PyList_New(0);
}

void
_PyGC_TraceRegisterStaticType(PyInterpreterState *interp, PyTypeObject *type)
{
}

PyStatus
_PyGC_TraceInit(PyInterpreterState *interp)
{
    return _PyStatus_OK();
}

void
_PyGC_TraceFini(PyInterpreterState *interp)
{
}

#endif
