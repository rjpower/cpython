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
   cycle collector) and is a leaf otherwise.  A static type is traversed
   with _PyType_TraverseStatic().

   Roots:

   - precise: the frames of every thread on the frame stack (all slots, the
     code, function, globals, builtins, locals and frame object), the thread
     states' object fields, the interpreter's object fields, the GC's lists,
     the registered static types and every immortal heap object;
   - conservative: every thread's C stack from its saved stack pointer to
     its base, its callee-saved registers, the writable segments of every
     loaded object, and the thread and interpreter state structs.  A word is
     a root if it points into an allocated object block; an interior pointer
     marks its block.

   Every pointer, precise or conservative, is checked before it is followed:
   it must be an allocated object's address or point to readable memory
   outside the heap, and its type must be a known type.  A conservative scan
   can find an object that is still being built, so an untracked GC object
   other than a tuple that only a conservative root reaches is marked but not
   traversed, until a precise reference reaches it. */

#include "Python.h"
#include "pycore_gc_trace.h"
#include "pycore_initconfig.h"    // _PyStatus_ERR()
#include "pycore_pystate.h"       // _PyInterpreterState_GET()

#if defined(WITH_MIMALLOC) && !defined(Py_GIL_DISABLED)

#include "pycore_crossinterp.h"   // _PyXIData_regitem_t
#include "pycore_frame.h"         // _PyInterpreterFrame
#include "pycore_freelist_state.h" // struct _Py_freelists
#include "pycore_gc.h"            // _PyObject_GC_IS_TRACKED()
#include "pycore_interp.h"        // _PyInterpreterState_GetFinalizing()
#include "pycore_interpframe.h"   // _PyFrame_Traverse()
#include "pycore_mimalloc.h"      // mi_heap_visit_blocks()
#include "pycore_object.h"        // _PyType_PreHeaderSize()
#include "pycore_pymem.h"         // _PyMem_DebugEnabled()
#include "pycore_tstate.h"        // _PyThreadStateImpl
#include "pycore_typeobject.h"    // _PyType_TraverseStatic()

#include <setjmp.h>
#include <stdio.h>
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
} trace_hole;

struct _gc_trace_state {
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
};

typedef struct {
    PyInterpreterState *interp;
    struct _gc_trace_state *ts;
    unsigned int roots;
    size_t debug_offset;

    addrset freed;        // blocks on the object free lists
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
    addrset mem_scanned;  // mem blocks queued for scanning
    uintptr_t *mem_stack; // (start, end) pairs of mem blocks to scan
    size_t mem_sp;
    size_t mem_cap;
    Py_ssize_t nmemblocks;
    int oom;
    Py_ssize_t nobjects;
    Py_ssize_t nmarked;
    Py_ssize_t nshallow;
    Py_ssize_t nrejected; // precise references that were not objects
    Py_ssize_t ngarbage;  // unmarked objects that only garbage refers to
} trace_t;

static int mark_visit(PyObject *op, void *arg);
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
        if (addrset_has(&a->t->freed, op)) {
            return true;  // allocated, but dead
        }
        if (addrset_add(a->objects, op) < 0) {
            a->t->oom = 1;
            return false;
        }
        a->t->nobjects++;
    }
    return true;
}

/* The objects on the interpreter's free lists are allocated blocks, but
   dead: their first word links the list and the rest is stale.  A full
   collection empties the lists; other collections do not. */
static int
read_freelists(trace_t *t)
{
    // struct _Py_freelists has only struct _Py_freelist members.
    Py_BUILD_ASSERT(sizeof(struct _Py_freelists)
                    % sizeof(struct _Py_freelist) == 0);
    struct _Py_freelists *fls = &t->interp->object_state.freelists;
    struct _Py_freelist *fl = (struct _Py_freelist *)fls;
    size_t n = sizeof(*fls) / sizeof(struct _Py_freelist);
    for (size_t i = 0; i < n; i++) {
        for (void *p = fl[i].freelist; p != NULL; p = *(void **)p) {
            if (addrset_add(&t->freed, (uintptr_t)p) < 0) {
                return -1;
            }
        }
    }
    return 0;
}

static size_t
heap_offset(trace_t *t, int tag)
{
    switch (tag) {
    case _Py_MIMALLOC_HEAP_GC:
        return t->debug_offset + sizeof(PyGC_Head);
    case _Py_MIMALLOC_HEAP_GC_PRE:
        return t->debug_offset + sizeof(PyGC_Head) + 2 * sizeof(PyObject *);
    default:
        return t->debug_offset;
    }
}

/* Visit the object heaps of every thread state and of the exited threads,
   and the mem heaps' pages.  The other threads cannot allocate: they do not
   hold the GIL. */
static int
walk_heaps(trace_t *t)
{
    static const int tags[] = {
        _Py_MIMALLOC_HEAP_OBJECT, _Py_MIMALLOC_HEAP_GC, _Py_MIMALLOC_HEAP_GC_PRE,
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
            a.offset = heap_offset(t, tags[i]);
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
        a.offset = heap_offset(t, tags[i]);
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

/* Map an address anywhere in an object block to the object, or 0. */
/* Queue the mem heap block that contains addr for a conservative scan. */
static void
queue_mem_block(trace_t *t, uintptr_t addr)
{
    const range_t *r = rangelist_find(&t->mem_pages, addr);
    if (r == NULL) {
        return;
    }
    uintptr_t block = r->start + (addr - r->start) / r->bsize * r->bsize;
    int added = addrset_add(&t->mem_scanned, block);
    if (added <= 0) {
        t->oom |= added < 0;
        return;
    }
    if (t->mem_sp + 2 > t->mem_cap) {
        size_t cap = t->mem_cap ? t->mem_cap * 2 : 256;
        uintptr_t *stack = realloc(t->mem_stack, cap * sizeof(uintptr_t));
        if (stack == NULL) {
            t->oom = 1;
            return;
        }
        t->mem_stack = stack;
        t->mem_cap = cap;
    }
    t->mem_stack[t->mem_sp++] = block;
    t->mem_stack[t->mem_sp++] = block + r->bsize;
    t->nmemblocks++;
}

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
    if (op != NULL) {
        mark((trace_t *)arg, op, 0);
    }
    return 0;
}

static int
traverse(PyObject *op, visitproc visit, void *arg)
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
        mark(t, (PyObject *)Py_TYPE(op), 0);
        (void)traverse(op, mark_visit, t);
    }
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
static void
scan_range(trace_t *t, uintptr_t start, uintptr_t end)
{
    start = _Py_SIZE_ROUND_UP(start, sizeof(uintptr_t));
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


/* --- register capture ---------------------------------------------------- */

/* Store the callee-saved registers in regs.  regs is passed in a register
   that is not callee-saved, so that computing it cannot overwrite one. */
static inline __attribute__((always_inline)) void
capture_registers(uintptr_t *regs)
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

Py_NO_INLINE void
_PyGC_TraceCaptureThread(void)
{
    uintptr_t regs[24] = {0};
    capture_registers(regs);
    PyThreadState *tstate = _PyThreadState_GET();
    if (tstate == NULL) {
        return;
    }
    _PyThreadStateImpl *ts = (_PyThreadStateImpl *)tstate;
    memcpy(ts->gc_trace.regs, regs, sizeof(regs));
    // This frame and the ones above it are live while the thread is
    // detached; scanning from here also covers the caller's spills.
    ts->gc_trace.sp = (uintptr_t)__builtin_frame_address(0);
    ts->gc_trace.valid = 1;
}


/* --- conservative roots ----------------------------------------------------- */

static uintptr_t
stack_top(PyThreadState *p)
{
    _PyThreadStateImpl *ts = (_PyThreadStateImpl *)p;
    return ts->c_stack_top;
}

/* This thread's stack and registers.  Not inlined, so that its frame and
   everything below the caller's are on the stack it scans. */
static Py_NO_INLINE void
scan_current_thread(trace_t *t, PyThreadState *tstate)
{
    uintptr_t regs[24] = {0};
    capture_registers(regs);
    scan_range(t, (uintptr_t)regs, (uintptr_t)(regs + 24));
    uintptr_t top = stack_top(tstate);
    uintptr_t here = (uintptr_t)__builtin_frame_address(0);
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
    if (top > here) {
        scan_range(t, here, top);
    }
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
    while (t->mem_sp > 0 && !t->oom) {
        uintptr_t end = t->mem_stack[--t->mem_sp];
        uintptr_t start = t->mem_stack[--t->mem_sp];
        scan_range(t, start, end);
    }
    t->scan_mem = 0;
}

static void
scan_threads_only(trace_t *t, PyThreadState *current)
{
    scan_current_thread(t, current);
    _Py_FOR_EACH_TSTATE_UNLOCKED(t->interp, p) {
        if (p == current) {
            continue;
        }
        _PyThreadStateImpl *ts = (_PyThreadStateImpl *)p;
        if (!ts->gc_trace.valid) {
            continue;  // never attached
        }
        scan_range(t, (uintptr_t)ts->gc_trace.regs,
                   (uintptr_t)(ts->gc_trace.regs + 24));
        uintptr_t top = stack_top(p);
        if (top > ts->gc_trace.sp) {
            scan_range(t, ts->gc_trace.sp, top);
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

static void
scan_structs(trace_t *t)
{
    scan_range(t, (uintptr_t)t->interp,
               (uintptr_t)t->interp + sizeof(PyInterpreterState));
    _Py_FOR_EACH_TSTATE_UNLOCKED(t->interp, p) {
        scan_range(t, (uintptr_t)p, (uintptr_t)p + sizeof(_PyThreadStateImpl));
        /* The data stack.  A frame that is returning is unlinked from
         * current_frame before its slots are cleared, and a finalizer that
         * clearing a slot triggers can collect while the later slots still
         * hold their objects.  The current chunk holds slots up to
         * datastack_top and frame headers from datastack_limit; earlier
         * chunks are scanned whole. */
        _PyStackChunk *chunk = p->datastack_chunk;
        if (chunk == NULL) {
            continue;
        }
        uintptr_t end = (uintptr_t)chunk + chunk->size;
        scan_range(t, (uintptr_t)&chunk->data[0], (uintptr_t)p->datastack_top);
        scan_range(t, (uintptr_t)p->datastack_limit, end);
        for (chunk = chunk->previous; chunk != NULL; chunk = chunk->previous) {
            scan_range(t, (uintptr_t)&chunk->data[0],
                       (uintptr_t)chunk + chunk->size);
        }
    }
}


/* --- precise roots ------------------------------------------------------- */

static int
visit_frames(PyThreadState *p, visitproc visit, void *arg)
{
    for (_PyInterpreterFrame *f = p->current_frame; f != NULL; f = f->previous) {
        if (f->owner >= FRAME_OWNED_BY_INTERPRETER) {
            continue;
        }
        Py_VISIT(f->f_globals);
        Py_VISIT(f->f_builtins);
        int err = _PyFrame_Traverse(f, visit, arg);
        if (err) {
            return err;
        }
    }
    return 0;
}

static int
visit_tstate(PyThreadState *p, visitproc visit, void *arg)
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
    /* Objects the trashcan deferred: their reference count is already zero
     * but they still own their members until the chain is destroyed. */
    for (PyObject *op = p->delete_later; op != NULL;
         op = (PyObject *)(_Py_AS_GC(op)->_gc_next & ~(uintptr_t)1))
    {
        Py_VISIT(op);
    }
    return 0;
}

static int
visit_interp(PyInterpreterState *interp, visitproc visit, void *arg)
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
                (void)visit_frames(p, mark_visit, t);
                drain(t);
            }
            if (t->roots & _PyGC_TRACE_ROOT_TSTATE) {
                (void)visit_tstate(p, mark_visit, t);
                drain(t);
            }
        }
    }
    if (t->roots & _PyGC_TRACE_ROOT_INTERP) {
        (void)visit_interp(interp, mark_visit, t);
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
            (void)traverse(op, subtract_visit, &s);
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
         * traversing it.  Unmarked objects with no references may sit on a
         * free list with stale members; those the mark traversed, such as
         * the trashcan's, already marked their members. */
        if (!addrset_has(&t->shallow, addr) || addrset_has(&t->deepened, addr)) {
            continue;
        }
        (void)traverse(op, subtract_visit, &s);
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
            (void)traverse(op, held_visit, &hs);
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
        (void)traverse((PyObject *)op, referrer_visit, &s);
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
    // Holes that have a hole as referrer are listed after those that don't.
    Py_ssize_t shown = 0;
    for (int pass = 0; pass < 2; pass++) {
        for (Py_ssize_t i = 0; i < nholes && shown < limit; i++) {
            trace_hole *h = &t->ts->holes[i];
            int secondary = strstr(h->referrer, "(hole)") != NULL;
            if (secondary != pass) {
                continue;
            }
            fprintf(out, "gc verify: hole %s%s at %p refcnt %zd; referrer: %s\n",
                    h->type_name, h->value, (void *)h->addr, h->refcnt,
                    h->referrer);
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
            || !PyCode_Check(PyStackRef_AsPyObjectBorrow(f->f_executable)))
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
    addrset_fini(&t->freed);
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
    if (addrset_init(&t.freed, 1024) < 0
        || read_freelists(&t) < 0
        || addrset_init(&t.objects, 1 << 16) < 0
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
    }
    if (t.roots & _PyGC_TRACE_ROOT_STRUCTS) {
        scan_structs(&t);
    }
    HEAD_UNLOCK(&_PyRuntime);
    if (t.oom) {
        goto done;
    }
    nholes = find_holes(&t);
    report(&t, tstate, nholes);
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
    if (config->use_environment) {
        const char *env = getenv("PYTHON_GCVERIFY");
        if (env != NULL && *env != '\0') {
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
    free(ts->static_types);
    addrset_fini(&ts->static_type_set);
    free(ts->holes);
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

#else  /* !WITH_MIMALLOC || Py_GIL_DISABLED */

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
