/* The borrow stack: see Include/internal/pycore_borrow.h and
   pyjit/design/native-boundary.md.

   The stack is a chain of chunks.  A run of entries never straddles two
   chunks, so a tuple run is one contiguous array.  A mark is the stack top
   when a scope opened; release pops entries down to it, retiring chunks it
   empties on the way.  Chunks come from the raw allocator, so a thread state
   can be freed from any thread. */

#include "Python.h"
#include "pycore_borrow.h"
#include "pycore_ceval.h"         // _Py_set_eval_breaker_bit()
#include "pycore_pyerrors.h"      // _Py_FatalErrorFormat()
#include "pycore_pystate.h"       // _PyThreadState_GET()
#include "pycore_traceback.h"     // _Py_DumpStack()
#include "pycore_tuple.h"         // PyTuple_GET_VALUE()
#include "pycore_value.h"         // PyValue_AsObject()
#include "pycore_value_api.h"     // _PyValue_THUNK_AT()

#define BORROW_CHUNK_SLOTS 1024

/* Zero slots, shared by every thread: a new thread allocates nothing, and a
   mark is never NULL. */
static _PyBorrowChunk empty_chunk = {
    .previous = NULL,
    .previous_top = NULL,
    .end = empty_chunk.slots,
    .base_depth = 0,
};

/* The run returned for an empty tuple.  It is never read. */
static PyObject *const empty_run[1] = {NULL};

/* An argument vector (_PyBorrow_ValueVector) holds pointers that the stack
   does not own.  A skip entry above the vector records how many slots to step
   over.  It is a count tagged with the low bit, which no object pointer has. */
#define BORROW_SKIP(n) ((PyObject *)(((uintptr_t)(n) << 1) | 1))
#define BORROW_IS_SKIP(o) (((uintptr_t)(o) & 1) != 0)
#define BORROW_SKIP_COUNT(o) ((Py_ssize_t)((uintptr_t)(o) >> 1))

static inline _PyBorrowStack *
borrow_stack(PyThreadState *tstate)
{
    return &((_PyThreadStateImpl *)tstate)->borrow;
}

void
_PyBorrowStack_Init(_PyBorrowStack *s)
{
    s->chunk = &empty_chunk;
    s->top = s->end = empty_chunk.slots;
    s->eval_floor = NULL;
    s->spare = NULL;
}

#ifdef Py_DEBUG
/* Number of entries below p, or -1 if p is not a position on the stack at or
   below the top. */
static Py_ssize_t
borrow_depth(const _PyBorrowStack *s, PyObject **p)
{
    PyObject **limit = s->top;
    for (const _PyBorrowChunk *c = s->chunk; c != NULL; c = c->previous) {
        if (c->slots <= p && p <= limit) {
            return c->base_depth + (p - c->slots);
        }
        limit = c->previous_top;
    }
    return -1;
}
#endif

Py_ssize_t
_PyBorrow_Depth(PyThreadState *tstate)
{
    _PyBorrowStack *s = borrow_stack(tstate);
    return s->chunk->base_depth + (s->top - s->chunk->slots);
}

static void borrow_release(_PyBorrowStack *s, _PyBorrowMark mark);

/* A thread that still has a frame is a daemon thread that stopped while
   taking the GIL in mid-evaluation; its C frames may still hold borrows. */
static int
borrow_thread_has_frame(PyThreadState *tstate)
{
    return tstate->current_frame != tstate->base_frame;
}

void
_PyBorrowStack_ReleaseAll(PyThreadState *tstate)
{
    if (!borrow_thread_has_frame(tstate)) {
        borrow_release(borrow_stack(tstate), empty_chunk.slots);
    }
}

void
_PyBorrowStack_Fini(PyThreadState *tstate)
{
    _PyBorrowStack *s = borrow_stack(tstate);
    /* A thread state that failed allocation before init is zero-filled. */
    _PyBorrowChunk *chunk = s->chunk;
    /* The entries were released while the thread state could still run
       code: by PyThreadState_Clear(), and for the finalizing thread by
       interpreter_clear().  A thread stopped in mid-evaluation keeps its
       entries, as it keeps its frames, and they are not released here. */
    assert(chunk == NULL || borrow_thread_has_frame(tstate) ||
           _PyBorrow_Depth(tstate) == 0);
    while (chunk != NULL && chunk != &empty_chunk) {
        _PyBorrowChunk *previous = chunk->previous;
        PyMem_RawFree(chunk);
        chunk = previous;
    }
    PyMem_RawFree(s->spare);
    _PyBorrowStack_Init(s);
}

int
_PyBorrowStack_Visit(PyThreadState *tstate, visitproc visit, void *arg)
{
    _PyBorrowStack *s = borrow_stack(tstate);
    PyObject **top = s->top;
    for (const _PyBorrowChunk *c = s->chunk; c != NULL; c = c->previous) {
        /* Downward, as release walks, to find each vector's skip entry
           before its slots. */
        for (PyObject **q = top; q > c->slots;) {
            PyObject *o = *--q;
            if (BORROW_IS_SKIP(o)) {
                q -= BORROW_SKIP_COUNT(o);
                continue;
            }
            Py_VISIT(o);
        }
        top = c->previous_top;
    }
    return 0;
}

void
_PyBorrow_RequestRelease(PyThreadState *tstate)
{
    if (borrow_stack(tstate)->eval_floor != NULL) {
        _Py_set_eval_breaker_bit(tstate, _PY_BORROW_RELEASE_BIT);
    }
}

/* Open a chunk with room for n entries on top of the current one. */
static int
borrow_grow(_PyBorrowStack *s, Py_ssize_t n)
{
    _PyBorrowChunk *chunk = s->spare;
    if (chunk != NULL && chunk->end - chunk->slots >= n) {
        s->spare = NULL;
    }
    else {
        Py_ssize_t size = Py_MAX(n, BORROW_CHUNK_SLOTS);
        if ((size_t)size > (PY_SSIZE_T_MAX - sizeof(_PyBorrowChunk))
                           / sizeof(PyObject *)) {
            PyErr_NoMemory();
            return -1;
        }
        chunk = PyMem_RawMalloc(sizeof(_PyBorrowChunk)
                                + (size_t)size * sizeof(PyObject *));
        if (chunk == NULL) {
            PyErr_NoMemory();
            return -1;
        }
        chunk->end = chunk->slots + size;
    }
    chunk->previous = s->chunk;
    chunk->previous_top = s->top;
    chunk->base_depth = s->chunk->base_depth + (s->top - s->chunk->slots);
    s->chunk = chunk;
    s->top = chunk->slots;
    s->end = chunk->end;
    return 0;
}

/* Keep one emptied chunk of the standard size for the next growth; an
   oversized chunk for a long tuple is not kept for the thread's lifetime. */
static void
borrow_chunk_retire(_PyBorrowStack *s, _PyBorrowChunk *chunk)
{
    if (s->spare == NULL && chunk->end - chunk->slots == BORROW_CHUNK_SLOTS) {
        s->spare = chunk;
    }
    else {
        PyMem_RawFree(chunk);
    }
}

static void
borrow_release(_PyBorrowStack *s, _PyBorrowMark mark)
{
    /* Re-read the stack on every iteration: a deallocator may push or pop. */
    while (s->top != mark) {
        if (s->top == s->chunk->slots) {
            /* The chunk is empty: pop it. */
            _PyBorrowChunk *done = s->chunk;
            assert(done != &empty_chunk);
            s->chunk = done->previous;
            s->top = done->previous_top;
            s->end = s->chunk->end;
            borrow_chunk_retire(s, done);
            continue;
        }
        PyObject *o = *--s->top;
        if (BORROW_IS_SKIP(o)) {
            /* A vector lies wholly in one chunk and holds no mark: stepping
               over it never passes a mark in this chunk. */
            PyObject **vector = s->top - BORROW_SKIP_COUNT(o);
            assert(vector >= s->chunk->slots);
            assert(!(s->chunk->slots <= mark && mark <= s->top) ||
                   vector >= mark);
            s->top = vector;
            continue;
        }
        Py_XDECREF(o);
    }
}

void
_PyBorrowScope_Release(PyThreadState *tstate, _PyBorrowMark mark)
{
    _PyBorrowStack *s = borrow_stack(tstate);
#ifdef Py_DEBUG
    /* A scope's Exit runs on the thread state that its Enter used. */
    assert(tstate == _PyThreadState_GET());
    if (borrow_depth(s, mark) < 0) {
        /* Releasing would pop live entries of outer scopes and then run off
           the bottom of the stack. */
        _Py_FatalErrorFormat(__func__,
            "borrow scope mark %p is not on the borrow stack of thread "
            "state %p at or below its top %p (depth %zd): a scope exited "
            "out of LIFO order, or an eval floor released entries that a "
            "live scope owned", (void *)mark, (void *)tstate,
            (void *)s->top, _PyBorrow_Depth(tstate));
    }
#endif
    borrow_release(s, mark);
}

#ifdef Py_DEBUG
/* The measured limit of rule L (pyjit/design/native-boundary.md, section 5):
   warn once per push site when the entries above the innermost eval floor
   exceed this count.  A parse in a loop whose trip count depends on the input
   needs its own scope (Py_BEGIN_BORROW_SCOPE). */
#define BORROW_SCOPE_WARN_SIZE 10000
#define BORROW_WARN_SITES 32

static void
borrow_check_size(PyThreadState *tstate, _PyBorrowStack *s, void *site)
{
    Py_ssize_t depth = _PyBorrow_Depth(tstate);
    if (depth <= BORROW_SCOPE_WARN_SIZE) {
        return;
    }
    if (s->eval_floor != NULL) {
        depth -= borrow_depth(s, s->eval_floor);
        if (depth <= BORROW_SCOPE_WARN_SIZE) {
            return;
        }
    }
    static void *warned[BORROW_WARN_SITES];
    static int nwarned;
    for (int i = 0; i < nwarned; i++) {
        if (warned[i] == site) {
            return;
        }
    }
    if (nwarned == BORROW_WARN_SITES) {
        return;
    }
    warned[nwarned++] = site;
    fprintf(stderr,
            "Warning: a borrow scope holds %zd entries (limit %d); the push "
            "at %p is in an unbounded loop without its own scope:\n",
            depth, BORROW_SCOPE_WARN_SIZE, site);
    _Py_DumpStack(fileno(stderr));
}

#  if defined(__GNUC__) || defined(__clang__)
#    define BORROW_PUSH_SITE() __builtin_return_address(0)
#  else
#    define BORROW_PUSH_SITE() NULL
#  endif
#  define BORROW_CHECK_SIZE(tstate, s) \
       borrow_check_size((tstate), (s), BORROW_PUSH_SITE())
#else
#  define BORROW_CHECK_SIZE(tstate, s) ((void)0)
#endif

PyObject *
_PyBorrow_Keep(PyObject *owned)
{
    assert(owned != NULL);
    PyThreadState *tstate = _PyThreadState_GET();
    _PyBorrowStack *s = borrow_stack(tstate);
    if (s->top == s->end && borrow_grow(s, 1) < 0) {
        Py_DECREF(owned);
        return NULL;
    }
    *s->top++ = owned;
    _PyBorrow_RequestRelease(tstate);
    BORROW_CHECK_SIZE(tstate, s);
    return owned;
}

/* The two borrow entries box for their caller (pyjit/design/proposals/
   value-api-g.md, section 1.2). In stats builds each entry states the box
   argument of its own caller and shares its body, a static function that
   takes the argument, with its For form; elsewhere the body is the entry. */
#ifdef Py_STATS
#  define BORROW_STATIC static
#  define BORROW_BODY(NAME) NAME##_body
#else
#  define BORROW_STATIC
#  define BORROW_BODY(NAME) NAME
#endif
#define borrow_box(V0) _PyValue_AsObjectFor((V0), at)

BORROW_STATIC PyObject *const *
BORROW_BODY(_PyBorrow_TupleItems)(PyObject *tuple, Py_ssize_t n
                                  _PyBox_AT_PARAM)
{
    assert(PyTuple_Check(tuple));
    assert(0 <= n && n <= PyTuple_GET_SIZE(tuple));
    assert(!PyErr_Occurred());
    if (n == 0) {
        return empty_run;
    }
    PyThreadState *tstate = _PyThreadState_GET();
    _PyBorrowStack *s = borrow_stack(tstate);
    if (s->end - s->top < n && borrow_grow(s, n) < 0) {
        return NULL;
    }
    _PyBorrow_RequestRelease(tstate);
    /* Boxing allocates, and an object allocator hook may run code; as in
       _PyBorrow_ValueVector, what that code calls leaves the top where it
       was, so the run stays contiguous. */
    PyObject **run = s->top;
    for (Py_ssize_t i = 0; i < n; i++) {
        PyObject *item = borrow_box(PyTuple_GET_VALUE(tuple, i));
        if (item == NULL) {
            return NULL;
        }
        assert(s->top == run + i);
        *s->top++ = item;
    }
    BORROW_CHECK_SIZE(tstate, s);
    return run;
}

BORROW_STATIC PyObject **
BORROW_BODY(_PyBorrow_ValueVector)(const PyValue *values, Py_ssize_t n
                                   _PyBox_AT_PARAM)
{
    assert(n >= 0);
    assert(!PyErr_Occurred());
    PyThreadState *tstate = _PyThreadState_GET();
    _PyBorrowStack *s = borrow_stack(tstate);
    Py_ssize_t boxes = 0;
    for (Py_ssize_t i = 0; i < n; i++) {
        assert(PyValue_IsLanguageValue(values[i]));
        _PyValue_RecheckHome(values[i]);
        boxes += PyValue_IsInline(values[i]);
    }
    /* The OFFSET slot, the vector, its skip entry and the boxes share one
       chunk, so filling them cannot fail on growth. */
    Py_ssize_t size = n + 2 + boxes;
    if (s->end - s->top < size && borrow_grow(s, size) < 0) {
        return NULL;
    }
    /* Every caller exits its own scope after the call, so a release request
       would only cost an atomic OR now and a pending-call check later. */
    PyObject **vector = s->top + 1;
    vector[-1] = NULL;
    for (Py_ssize_t i = 0; i < n; i++) {
        vector[i] = PyValue_IsInline(values[i])
            ? NULL : PyValue_AsKnownObjectBorrow(values[i]);
    }
    vector[n] = BORROW_SKIP(n + 1);
    s->top = vector + n + 1;
    /* Boxing allocates, and an object allocator hook (PyMem_SetAllocator)
       may run any code, including Python code.  What that code calls exits
       its own scope or activation, so the top comes back and the reserved
       room is still there.  The hook must not push outside a scope. */
    for (Py_ssize_t i = 0; i < n; i++) {
        if (PyValue_IsInline(values[i])) {
            PyObject *box = borrow_box(values[i]);
            if (box == NULL) {
                return NULL;
            }
            assert(s->top < s->end);
            *s->top++ = box;
            vector[i] = box;
        }
    }
    BORROW_CHECK_SIZE(tstate, s);
    return vector;
}

#ifdef Py_STATS
/* A Clinic or parser tuple view boxes for the function that parses: kind
   THUNK, owned by the caller. */
PyObject *const *
_PyBorrow_TupleItems(PyObject *tuple, Py_ssize_t n)
{
    return _PyBorrow_TupleItems_body(
        tuple, n, _PyValue_THUNK_AT(BOX_SITE_CALL_ADAPTER));
}

PyObject *const *
_PyBorrow_TupleItemsFor(PyObject *tuple, Py_ssize_t n, _PyBoxAt at)
{
    return _PyBorrow_TupleItems_body(tuple, n, at);
}

// A vector whose caller does not name the callee: owned by the caller.
PyObject **
_PyBorrow_ValueVector(const PyValue *values, Py_ssize_t n)
{
    return _PyBorrow_ValueVector_body(
        values, n, _PyBox_AT(BOX_SITE_CALL_ADAPTER, BOX_KIND_CALL_ADAPTER,
                             _Py_BOX_CALLER()));
}

PyObject **
_PyBorrow_ValueVectorFor(const PyValue *values, Py_ssize_t n, _PyBoxAt at)
{
    return _PyBorrow_ValueVector_body(values, n, at);
}
#endif


void
PyBorrowScope_Enter(PyBorrowScope *scope)
{
    scope->_mark = _PyBorrowScope_Enter(_PyThreadState_GET());
}

void
PyBorrowScope_Exit(PyBorrowScope *scope)
{
    _PyBorrowScope_Exit(_PyThreadState_GET(), (_PyBorrowMark)scope->_mark);
}
