/* The borrow stack: owned references behind borrowed parser results.
 *
 * Tuples and dicts store canonical PyValues, so an inline int or float has
 * no object a parser could lend.  A parser that must hand out a borrowed
 * PyObject * pushes an owned reference onto the current thread's borrow
 * stack and returns it.  The runtime opens a borrow scope (a mark on the
 * stack) around every native function body and every eval-loop activation;
 * the references above the mark are released when the scope exits.
 *
 * See pyjit/design/native-boundary.md. */

#ifndef Py_INTERNAL_BORROW_H
#define Py_INTERNAL_BORROW_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_tstate.h"        // _PyThreadStateImpl, _PyBorrowStack

typedef PyObject **_PyBorrowMark;

/* Pop and Py_DECREF the entries above mark, newest first.  Cannot fail and
 * does not touch the error indicator (deallocators preserve it).  Reentrant:
 * code run by a deallocator pushes above the current top and either releases
 * its own entries or leaves them for this loop. */
PyAPI_FUNC(void) _PyBorrowScope_Release(PyThreadState *, _PyBorrowMark);

static inline _PyBorrowMark
_PyBorrowScope_Enter(PyThreadState *tstate)
{
    return ((_PyThreadStateImpl *)tstate)->borrow.top;
}

static inline void
_PyBorrowScope_Exit(PyThreadState *tstate, _PyBorrowMark mark)
{
    if (((_PyThreadStateImpl *)tstate)->borrow.top != mark) {
        _PyBorrowScope_Release(tstate, mark);
    }
}

/* Both functions below act on the current thread state. */

/* Steal `owned`, push it, and return it as a borrowed reference that is
 * valid until the current scope exits.  On allocation failure, release
 * `owned`, set MemoryError and return NULL. */
PyAPI_FUNC(PyObject *) _PyBorrow_Keep(PyObject *owned);

/* Push one owned reference per item of `tuple` as one contiguous run and
 * return the run.  Inline scalars are boxed.  An empty tuple returns a static
 * empty array.  On failure, return NULL with MemoryError; items already pushed
 * stay until the scope exits.  The run is read-only and has no slot before
 * element 0: never pass PY_VECTORCALL_ARGUMENTS_OFFSET with it. */
PyAPI_FUNC(PyObject *const *) _PyBorrow_TupleItems(PyObject *tuple);

/* Ask the innermost eval loop to release the entries above its floor at the
 * next instruction boundary.  No-op outside any eval loop. */
PyAPI_FUNC(void) _PyBorrow_RequestRelease(PyThreadState *tstate);

/* Thread-state lifecycle (Python/pystate.c).  Init allocates nothing.  Clear
 * releases every entry, including the base region, and frees the chunks. */
extern void _PyBorrowStack_Init(_PyBorrowStack *stack);
extern void _PyBorrowStack_Clear(PyThreadState *tstate);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_BORROW_H */
