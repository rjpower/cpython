#include "Python.h"

#include "pycore_borrow.h"        // _PyBorrowScope_Exit()
#include "pycore_native_scope.h"
#include "pycore_value_api.h"     // _PyBox_AT()


static void
initialize_scope(_PyNativeScope *scope, PyThreadState *tstate)
{
    *scope = (_PyNativeScope){
        .tstate = tstate,
        .receiver = PyValue_NULL,
        .result = {
            .value = PyValue_NULL,
            .displaced = PyValue_NULL,
            .state = _Py_NATIVE_RESULT_EMPTY,
        },
    };
    scope->objects = scope->argument_objects;
}

int
_PyNativeScope_Begin(_PyNativeScope *scope, PyThreadState *tstate,
                     const PyValue *args, Py_ssize_t positional_count,
                     Py_ssize_t total_count, PyValue self_or_null,
                     PyObject *kwnames)
{
    initialize_scope(scope, tstate);
    if (tstate == NULL || positional_count < 0 || positional_count > 1 ||
        positional_count != total_count || kwnames != NULL ||
        (total_count != 0 && args == NULL) ||
        (!PyValue_IsNull(self_or_null) &&
         !PyValue_IsLanguageValue(self_or_null)) ||
        (total_count != 0 && !PyValue_IsLanguageValue(args[0])))
    {
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_SystemError,
                            "invalid fixed native scope arguments");
        }
        return -1;
    }
    scope->nargs = total_count;
    scope->positional_count = positional_count;
    if (!PyValue_IsNull(self_or_null)) {
        scope->receiver = PyValue_DUP(self_or_null);
    }
    if (total_count != 0) {
        /* Pass an object borrowed, as upstream passes its stack: the caller
           keeps it alive until End.  Box an inline value now; End releases
           the box. */
        if (PyValue_IsInline(args[0])) {
            /* The callee is not known here: the scope names itself. */
            scope->argument_box = _PyValue_AsObjectFor(
                args[0],
                _PyBox_AT(BOX_SITE_CALL_ADAPTER, BOX_KIND_CALL_ADAPTER,
                          _Py_FUNC_ADDR(_PyNativeScope_Begin)));
            if (scope->argument_box == NULL) {
                return -1;
            }
            scope->objects[0] = scope->argument_box;
        }
        else {
            scope->objects[0] = PyValue_AsKnownObjectBorrow(args[0]);
        }
    }
    return 0;
}

int
_PyNativeScope_Objects(_PyNativeScope *scope, PyObject *const **args)
{
    assert(!scope->ended);
    *args = scope->objects;
    if (!PyValue_IsNull(scope->receiver) && scope->receiver_object == NULL) {
        scope->receiver_object = _PyValue_AsObjectFor(
            scope->receiver, _PyBox_CALL_ADAPTER(scope->callable));
        if (scope->receiver_object == NULL) {
            return -1;
        }
    }
    /* Begin set the arguments' objects: borrowed, or boxes of inline
       values that End releases. */
    return 0;
}

int
_PyNativeScope_StageResult(_PyNativeScope *scope, PyObject *owned_result,
                           _PyNativeResult *out)
{
    assert(!scope->ended);
    assert(out == &scope->result);
    assert(out->state == _Py_NATIVE_RESULT_EMPTY);
    if (owned_result == NULL) {
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_SystemError,
                            "native function returned NULL without an error");
        }
        return -1;
    }
    /* Preparation releases a scalar box at once (the release contract in
     * pycore_value.h); every caller has checked the result with
     * _Py_CheckFunctionResult() or holds an owned object. */
    out->value = PyValue_PrepareOwned(owned_result);
    out->state = _Py_NATIVE_RESULT_STAGED;
    return 0;
}

void
_PyNativeResult_Publish(_PyNativeResult *out, PyValue *destination,
                        _PyNativePublishState publish_state,
                        void *caller_state)
{
    assert(out->state == _Py_NATIVE_RESULT_STAGED);
    assert(!PyErr_Occurred());
    assert(publish_state != NULL);
    assert(PyValue_IsLanguageValue(out->value));
    _PyValue_CheckHome(out->value);
    assert(PyValue_IsNull(*destination) ||
           PyValue_IsLanguageValue(*destination));
    _PyValue_RecheckHome(*destination);
    out->displaced = *destination;
    *destination = out->value;
    out->value = PyValue_NULL;
    out->state = _Py_NATIVE_RESULT_PUBLISHED;
    publish_state(caller_state);
}

void
_PyNativeResult_Abort(_PyNativeResult *out,
                      _PyNativePublishState publish_error_state,
                      void *caller_state)
{
    assert(out->state == _Py_NATIVE_RESULT_EMPTY ||
           out->state == _Py_NATIVE_RESULT_STAGED);
    assert(publish_error_state != NULL);
    out->state = _Py_NATIVE_RESULT_ABORTED;
    publish_error_state(caller_state);
}

void
_PyNativeScope_End(_PyNativeScope *scope)
{
    if (scope->ended) {
        return;
    }
    _PyNativeResult *out = &scope->result;
    assert(out->state == _Py_NATIVE_RESULT_PUBLISHED ||
           out->state == _Py_NATIVE_RESULT_ABORTED);
    scope->ended = 1;
    out->state = _Py_NATIVE_RESULT_DRAINED;
    PyValue_CLEAR(out->value);
    PyValue_CLEAR(out->displaced);
    scope->argument_objects[0] = NULL;
    Py_CLEAR(scope->argument_box);
    Py_CLEAR(scope->receiver_object);
    PyValue_CLEAR(scope->receiver);
    Py_CLEAR(scope->callable);
    scope->objects = NULL;
    scope->nargs = 0;
}
