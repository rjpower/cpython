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

/* Ownership contract: callers own, the scope borrows.  The caller keeps the
 * callable, the argument and any self_or_null alive from Begin until End, as
 * upstream keeps its evaluation stack and callable alive across a call.  The
 * scope takes no reference to any of them.  Only an inline value has no
 * object to lend: the scope boxes it, owns that box (argument_box,
 * receiver_box) and releases it in End.  The scope holds no PyValueBorrow
 * (those live in C locals only), and no pointer refers to the native body's
 * C stack.  Variable calls do not use a scope: the interpreter's call
 * transaction calls the value vectorcall (pycore_value_call.h) on the
 * window. */
typedef struct {
    PyThreadState *tstate;
    PyObject *receiver_object;      // borrowed, or receiver_box
    PyObject *receiver_box;         // owned box of an inline receiver
    PyObject *argument_objects[1];
    PyObject *argument_box;         // owned box of an inline argument
    PyObject **objects;
    PyObject *callable;             // borrowed
    Py_ssize_t nargs;
    Py_ssize_t positional_count;
    int ended;
    _PyNativeResult result;
} _PyNativeScope;

/* This callback publishes frame/PC/live-window state. It must not allocate,
 * release owners, call Python, or fail. End is a later reentry boundary. */
typedef void (*_PyNativePublishState)(void *caller_state);

/* Borrows args and self_or_null (see _PyNativeScope above); boxing an inline
 * one can fail with MemoryError.  This first slice accepts at most one
 * argument and no keyword tail. */
PyAPI_FUNC(int) _PyNativeScope_Begin(
    _PyNativeScope *scope, PyThreadState *tstate,
    const PyValue *args, Py_ssize_t positional_count, Py_ssize_t total_count,
    PyValue self_or_null, PyObject *kwnames);
PyAPI_FUNC(int) _PyNativeScope_Objects(
    _PyNativeScope *scope, PyObject *const **args);
/* Borrow an object receiver; box an inline one.  callable may be NULL. */
PyAPI_FUNC(int) _PyNativeScope_SetReceiver(
    _PyNativeScope *scope, PyValueBorrow receiver, PyObject *callable);
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

/* Borrows callable and its bound self; the caller keeps callable alive until
 * End.  out must be &scope->result. 0 = staged success, -1 = failure. */
PyAPI_FUNC(int) _PyCFunction_CallScoped(
    _PyNativeScope *scope, PyObject *callable, _PyNativeResult *out);

#ifdef __cplusplus
}
#endif
#endif
