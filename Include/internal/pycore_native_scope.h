#ifndef Py_INTERNAL_NATIVE_SCOPE_H
#define Py_INTERNAL_NATIVE_SCOPE_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

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
    PyObject *retired;
    _PyNativeResultState state;
} _PyNativeResult;

/* The caller owns the workspace, including overflow storage, through End.
 * No pointer refers to the native body's C stack. */
typedef struct {
    PyThreadState *tstate;
    PyValue receiver;
    PyValue argument;
    PyObject *receiver_object;
    PyObject *argument_objects[1];
    PyValue *arguments;
    PyObject **objects;
    void *storage;
    PyObject *callable;
    PyObject *kwnames;
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

/* Source arguments must be stable and owning through capture. This first
 * slice accepts at most one argument and no keyword tail. Scope snapshots
 * are distinct owners; publication may subsequently move the source window. */
PyAPI_FUNC(int) _PyNativeScope_Begin(
    _PyNativeScope *scope, PyThreadState *tstate,
    const PyValue *args, Py_ssize_t positional_count, Py_ssize_t total_count,
    PyValue self_or_null, PyObject *kwnames);
/* effective_args is an already-owning stable window through capture; total
 * includes the keyword tail. Capture then owns independent snapshots. The
 * C callable and all invocation metadata are pinned before allocation.
 * Objects has no writable prefix cell and advertises no OFFSET flag. */
PyAPI_FUNC(int) _PyNativeScope_BeginVector(
    _PyNativeScope *scope, PyThreadState *tstate, PyObject *callable,
    const PyValue *effective_args, Py_ssize_t positional_count,
    Py_ssize_t total_count, PyObject *kwnames);
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
/* Call after Publish/Abort and actual caller-state publication. A negative
 * status leaves a published result caller-owned. Reload/revalidate cached
 * frame/IR/feedback state after End even when it succeeds. */
PyAPI_FUNC(int) _PyNativeScope_End(_PyNativeScope *scope);
/* Pure move, valid after staging/publication and before End. The caller
 * retains this owner until its route's original result-conversion point. */
PyAPI_FUNC(PyObject *) _PyNativeScope_TakeRetiredResult(_PyNativeScope *scope);

/* Pure eligibility: supported flags AND their standard vectorcall target. */
PyAPI_FUNC(int) _PyCFunction_CanCallScoped(PyObject *callable);
/* out must be &scope->result. 0 = staged success, -1 = failure. A vector
 * scope alone may return +1 = NOT_INVOKED, with EMPTY result and no error.
 * The caller drains that preparation, refreshes and uses generic semantics;
 * it must never replay a completed body. Fixed Begin keeps its 0/-1 ABI. */
PyAPI_FUNC(int) _PyCFunction_CallScoped(
    _PyNativeScope *scope, PyObject *callable, _PyNativeResult *out);

#ifdef __cplusplus
}
#endif
#endif
