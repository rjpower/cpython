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

/* The caller owns the workspace through End.  The argument of a fixed scope
 * is passed as a borrowed pointer, as upstream passes its evaluation stack;
 * only an inline value is boxed, and argument_box owns that box.  The scope
 * holds no borrowed value (PyValueBorrow lives in C locals only), and no
 * pointer refers to the native body's C stack.  Variable calls do not use a
 * scope: the interpreter's call transaction calls the value vectorcall
 * (pycore_value_call.h) on the window. */
typedef struct {
    PyThreadState *tstate;
    PyValue receiver;
    PyObject *receiver_object;
    PyObject *argument_objects[1];
    PyObject *argument_box;
    PyObject **objects;
    PyObject *callable;
    Py_ssize_t nargs;
    Py_ssize_t positional_count;
    int ended;
    _PyNativeResult result;
} _PyNativeScope;

/* This callback publishes frame/PC/live-window state. It must not allocate,
 * release owners, call Python, or fail. End is a later reentry boundary. */
typedef void (*_PyNativePublishState)(void *caller_state);

/* The caller keeps the source argument alive until End; publication may move
 * it out of the window.  The scope passes an object argument borrowed, and
 * boxes an inline one at once, which can fail with MemoryError; End
 * releases the box.  This first slice accepts at most one argument and no
 * keyword tail. */
PyAPI_FUNC(int) _PyNativeScope_Begin(
    _PyNativeScope *scope, PyThreadState *tstate,
    const PyValue *args, Py_ssize_t positional_count, Py_ssize_t total_count,
    PyValue self_or_null, PyObject *kwnames);
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

/* out must be &scope->result. 0 = staged success, -1 = failure. */
PyAPI_FUNC(int) _PyCFunction_CallScoped(
    _PyNativeScope *scope, PyObject *callable, _PyNativeResult *out);

#ifdef __cplusplus
}
#endif
#endif
