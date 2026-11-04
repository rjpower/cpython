#ifndef Py_INTERNAL_NATIVE_SCOPE_H
#define Py_INTERNAL_NATIVE_SCOPE_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_borrow.h"        // _PyBorrowMark
#include "pycore_value.h"

typedef enum {
    _Py_NATIVE_RESULT_EMPTY,
    _Py_NATIVE_RESULT_STAGED,
    _Py_NATIVE_RESULT_PUBLISHED,
    _Py_NATIVE_RESULT_ABORTED,
    _Py_NATIVE_RESULT_DRAINED,
} _PyNativeResultState;

typedef struct {
    PyValue value;
    PyValue displaced;
    _PyNativeResultState state;
} _PyNativeResult;

/* The caller owns the workspace through End.  Arguments that are objects
 * are passed as borrowed pointers, as upstream passes its evaluation stack;
 * only inline values are boxed.  A fixed scope keeps its one argument and
 * its box in the struct.  A vector scope keeps its argument vector on the
 * borrow stack (_PyBorrow_ValueVector), above `mark`; End exits to the mark.
 * No pointer refers to the native body's C stack. */
typedef struct {
    PyThreadState *tstate;
    PyValue receiver;
    PyValue argument;
    PyObject *receiver_object;
    PyObject *argument_objects[1];
    PyValue *arguments;         // fixed scopes only
    PyObject **objects;
    _PyBorrowMark mark;         // vector scopes only
    PyObject *callable;
    PyObject *kwnames;
    PyObject *callargs;
    PyObject *kwargs;
    PyTypeObject *defining_class;
    Py_ssize_t nargs;
    Py_ssize_t positional_count;
    int vector;
    int ended;
    _PyNativeResult result;
} _PyNativeScope;

/* This callback publishes frame/PC/live-window state. It must not allocate,
 * release owners, call Python, or fail. End is a later reentry boundary. */
typedef void (*_PyNativePublishState)(void *caller_state);

/* The caller keeps the source argument alive until End; publication may move
 * it out of the window.  The scope borrows an owned argument and takes its
 * own owner only for a borrowed one.  This first slice accepts at most one
 * argument and no keyword tail. */
PyAPI_FUNC(int) _PyNativeScope_Begin(
    _PyNativeScope *scope, PyThreadState *tstate,
    const PyValue *args, Py_ssize_t positional_count, Py_ssize_t total_count,
    PyValue self_or_null, PyObject *kwnames);
/* effective_args is an owning window that stays alive and unchanged until
 * End; total includes the keyword tail.  The vector borrows its objects and
 * boxes inline values.  The C callable and all invocation metadata are pinned
 * first.  Callers do not pass the OFFSET flag with the objects. */
PyAPI_FUNC(int) _PyNativeScope_BeginVector(
    _PyNativeScope *scope, PyThreadState *tstate, PyObject *callable,
    const PyValue *effective_args, Py_ssize_t positional_count,
    Py_ssize_t total_count, PyObject *kwnames);
/* Generic adapters retain the actual invocation inputs through Publish/Abort
 * and End. The object vector has no writable prefix/OFFSET flag.
 * InvokeObjectVector also accepts a BeginVector scope after NOT_INVOKED,
 * retaining its captured owners rather than draining and replaying capture. */
PyAPI_FUNC(int) _PyNativeScope_BeginObjectVector(
    _PyNativeScope *, PyThreadState *, PyObject *, const PyValue *,
    Py_ssize_t, Py_ssize_t, PyObject *);
PyAPI_FUNC(int) _PyNativeScope_BeginTupleCall(
    _PyNativeScope *, PyThreadState *, PyObject *, PyObject *, PyObject *);
PyAPI_FUNC(int) _PyNativeScope_InvokeObjectVector(_PyNativeScope *);
PyAPI_FUNC(int) _PyNativeScope_InvokeTupleCall(_PyNativeScope *);
PyAPI_FUNC(int) _PyNativeScope_Objects(
    _PyNativeScope *scope, PyObject *const **args);
/* Consumes owned_result, including when an error is already pending. */
PyAPI_FUNC(int) _PyNativeScope_StageResult(
    _PyNativeScope *scope, PyObject *owned_result, _PyNativeResult *out);
PyAPI_FUNC(void) _PyNativeResult_Publish(
    _PyNativeResult *out, PyValue *destination,
    _PyNativePublishState publish_state, void *caller_state);
PyAPI_FUNC(void) _PyNativeResult_Abort(
    _PyNativeResult *out, _PyNativePublishState publish_error_state,
    void *caller_state);
/* Call after Publish/Abort and actual caller-state publication. End only
 * releases, so it cannot fail and leaves the current exception as it found
 * it. Its releases can run Python: reload/revalidate cached frame/IR/feedback
 * state after End. */
PyAPI_FUNC(void) _PyNativeScope_End(_PyNativeScope *scope);

/* Pure eligibility: supported flags AND their standard vectorcall target. */
PyAPI_FUNC(int) _PyCFunction_CanCallScoped(PyObject *callable);
/* out must be &scope->result. 0 = staged success, -1 = failure. A vector
 * scope alone may return +1 = NOT_INVOKED, with EMPTY result and no error.
 * The caller can use InvokeObjectVector in the same scope; it must never
 * replay a completed body. Fixed Begin keeps its 0/-1 ABI. */
PyAPI_FUNC(int) _PyCFunction_CallScoped(
    _PyNativeScope *scope, PyObject *callable, _PyNativeResult *out);

#ifdef __cplusplus
}
#endif
#endif
