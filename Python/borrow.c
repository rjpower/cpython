/* The borrow stack: see Include/internal/pycore_borrow.h and
   pyjit/design/native-boundary.md.

   The stack is a chain of chunks.  A run of entries never straddles two
   chunks, so a tuple run is one contiguous array.  A mark is the stack top
   when a scope opened; release pops entries down to it, retiring chunks it
   empties on the way.  Chunks come from the raw allocator, so a thread state
   can be cleared from any thread. */

#include "Python.h"
#include "pycore_borrow.h"
#include "pycore_ceval.h"         // _Py_set_eval_breaker_bit()
#include "pycore_pystate.h"       // _PyThreadState_GET()
#include "pycore_tuple.h"         // PyTuple_GET_VALUE()

#define BORROW_CHUNK_SLOTS 1024

/* Zero slots, shared by every thread: a new thread allocates nothing, and a
   mark is never NULL. */
static _PyBorrowChunk empty_chunk = {
    .previous = NULL,
    .previous_top = NULL,
    .end = empty_chunk.slots,
};

/* The run returned for an empty tuple.  It is never read. */
static PyObject *const empty_run[1] = {NULL};

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

void
_PyBorrowStack_Clear(PyThreadState *tstate)
{
    _PyBorrowStack *s = borrow_stack(tstate);
    _PyBorrowScope_Exit(tstate, empty_chunk.slots);
    assert(s->chunk == &empty_chunk);
    PyMem_RawFree(s->spare);
    s->spare = NULL;
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

void
_PyBorrowScope_Release(PyThreadState *tstate, _PyBorrowMark mark)
{
    _PyBorrowStack *s = borrow_stack(tstate);
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
        Py_XDECREF(o);
    }
}

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
    return owned;
}

PyObject *const *
_PyBorrow_TupleItems(PyObject *tuple)
{
    assert(PyTuple_Check(tuple));
    Py_ssize_t n = PyTuple_GET_SIZE(tuple);
    if (n == 0) {
        return empty_run;
    }
    PyThreadState *tstate = _PyThreadState_GET();
    _PyBorrowStack *s = borrow_stack(tstate);
    if (s->end - s->top < n && borrow_grow(s, n) < 0) {
        return NULL;
    }
    _PyBorrow_RequestRelease(tstate);
    /* Boxing an exact int or float and taking a new reference cannot run
       Python code, so nothing else pushes while the run is filled. */
    PyObject **run = s->top;
    for (Py_ssize_t i = 0; i < n; i++) {
        PyObject *item = PyValue_AsObject(PyTuple_GET_VALUE(tuple, i));
        if (item == NULL) {
            return NULL;
        }
        assert(s->top == run + i);
        *s->top++ = item;
    }
    return run;
}


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
