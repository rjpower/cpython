/* The borrow stack: owned references behind borrowed parser results, and the
 * argument vectors of native calls from the interpreter.
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
 * its own entries or leaves them for this loop.  Debug builds fail fatally if
 * mark is not on the stack at or below the top: scopes exited out of LIFO
 * order, or a floor released entries that a live scope still owned. */
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

/* Push one owned reference for each of the first n items of `tuple`
 * (0 <= n <= its size) as one contiguous run and return the run.  Inline
 * scalars are boxed.  n == 0 returns a static empty array.  Call it with no
 * exception set, after any arity check, so a rejected call pushes nothing.
 * The only failure is memory: the stack cannot grow or a box cannot be
 * allocated.  Then return NULL with MemoryError set; items already pushed
 * stay until the scope exits.  The run is read-only and has no slot before
 * element 0: never pass PY_VECTORCALL_ARGUMENTS_OFFSET with it. */
PyAPI_FUNC(PyObject *const *) _PyBorrow_TupleItems(PyObject *tuple,
                                                   Py_ssize_t n);

/* Build a vectorcall argument vector for values[0..n) on the stack and return
 * it; the callee may use PY_VECTORCALL_ARGUMENTS_OFFSET.  As upstream passes
 * its evaluation stack, an object is passed as the pointer that values[i]
 * owns: the caller must keep the values alive and unchanged until the scope
 * exits.  Only inline scalars get new boxes, pushed as owned entries above the
 * vector; boxing cannot run Python code.  The stack steps over the vector's
 * borrowed slots when it releases.  Failure is as for TupleItems.  The caller
 * must hold its own scope around the vector and exit it after the call:
 * unlike the other pushes, this one does not ask the eval loop to release. */
PyAPI_FUNC(PyObject **) _PyBorrow_ValueVector(const PyValue *values,
                                              Py_ssize_t n);

/* Ask the innermost eval loop to release the entries above its floor at the
 * next instruction boundary.  No-op outside any eval loop. */
PyAPI_FUNC(void) _PyBorrow_RequestRelease(PyThreadState *tstate);

/* Thread-state lifecycle (Python/pystate.c, Python/pylifecycle.c).  Init
 * allocates nothing.  ReleaseAll releases every entry, including the base
 * region pushed outside any scope; finalization calls it before modules are
 * destroyed, so an explicit scope must not span Py_FinalizeEx(), and
 * PyThreadState_Clear() calls it.  It leaves the entries of a thread that
 * still has a frame (a daemon thread stopped in mid-evaluation), as
 * PyThreadState_Clear() leaves its frames.
 *
 * Code still runs on a cleared thread state: interpreter_clear() clears every
 * thread state and then collects garbage, whose finalizers call native code
 * that parses.  So, like the frame data stack, the chunks live until the
 * thread state's memory is freed, and interpreter_clear() releases the
 * finalizing thread's entries again at its end.  Fini frees the chunks; it
 * does not release entries, because the thread state may not be current.
 * Debug builds assert that none are left, except on a thread that still has
 * a frame. */
extern void _PyBorrowStack_Init(_PyBorrowStack *stack);
extern void _PyBorrowStack_ReleaseAll(PyThreadState *tstate);
extern void _PyBorrowStack_Fini(PyThreadState *tstate);

/* Visit the references that a thread's stack owns (for the GC verifier,
 * Python/gc_trace.c).  The thread state need not be current.  Argument
 * vectors are skipped: the frames that own their objects are visited. */
extern int _PyBorrowStack_Visit(PyThreadState *tstate, visitproc visit,
                                void *arg);

/* The number of entries on the stack (for tests and debug checks). */
PyAPI_FUNC(Py_ssize_t) _PyBorrow_Depth(PyThreadState *tstate);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_BORROW_H */
