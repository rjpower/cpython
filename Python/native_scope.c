#include "Python.h"

#include "pycore_borrow.h"        // _PyBorrow_ValueVector()
#include "pycore_native_scope.h"
#include "pycore_tuple.h"
#include "pycore_value_api.h"     // _PyValue_AsObjectAt()


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
            scope->argument_box = _PyValue_AsObjectAt(args[0],
                                                      BOX_SITE_CALL_ADAPTER);
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

static int
begin_vector(_PyNativeScope *scope, PyThreadState *tstate,
             PyObject *callable, const PyValue *effective_args,
             Py_ssize_t positional_count, Py_ssize_t total_count,
             PyObject *kwnames, int cfunction)
{
    initialize_scope(scope, tstate);
    scope->vector = cfunction ? 1 : 2;
    if (tstate == NULL || callable == NULL ||
        (cfunction && !PyCFunction_Check(callable)) ||
        positional_count < 0 || total_count < positional_count ||
        (total_count != 0 && effective_args == NULL) ||
        (kwnames != NULL && !PyTuple_Check(kwnames)))
    {
        PyErr_SetString(PyExc_SystemError, "invalid native vector scope arguments");
        return -1;
    }
    Py_ssize_t nkw = kwnames == NULL ? 0 : PyTuple_GET_SIZE(kwnames);
    if (nkw != total_count - positional_count) {
        PyErr_SetString(PyExc_SystemError, "native vector keyword count mismatch");
        return -1;
    }
    for (Py_ssize_t i = 0; i < nkw; i++) {
        PyValueBorrow name = PyTuple_GET_VALUE(kwnames, i);
        if (!PyValue_IsObject(name) ||
            !PyUnicode_Check(_PyTuple_GET_OBJECT(kwnames, i)))
        {
            PyErr_SetString(PyExc_TypeError, "keywords must be strings");
            return -1;
        }
    }
    for (Py_ssize_t i = 0; i < total_count; i++) {
        if (!PyValue_IsLanguageValue(effective_args[i])) {
            PyErr_SetString(PyExc_SystemError,
                            "native vector arguments must be language values");
            return -1;
        }
    }
    /* These INCREFs cannot reenter. All metadata and the producer's owning
     * window remain rooted while the arguments are boxed. */
    scope->callable = Py_NewRef(callable);
    scope->kwnames = Py_XNewRef(kwnames);
    if (cfunction) {
        scope->receiver_object = Py_XNewRef(PyCFunction_GET_SELF(callable));
        scope->defining_class = (PyTypeObject *)Py_XNewRef(
            (PyObject *)PyCFunction_GET_CLASS(callable));
    }
    /* Boxing can fail to allocate.  On failure the partial run stays above
       the mark and End releases it. */
    scope->mark = _PyBorrowScope_Enter(tstate);
    PyObject **objects = _PyBorrow_ValueVector(effective_args, total_count);
    if (objects == NULL) {
        return -1;
    }
    scope->objects = objects;
    scope->nargs = total_count;
    scope->positional_count = positional_count;
    return 0;
}

int
_PyNativeScope_BeginVector(_PyNativeScope *scope, PyThreadState *tstate,
                           PyObject *callable, const PyValue *args,
                           Py_ssize_t positional, Py_ssize_t total,
                           PyObject *kwnames)
{
    return begin_vector(scope, tstate, callable, args, positional, total, kwnames, 1);
}

int
_PyNativeScope_BeginObjectVector(_PyNativeScope *scope, PyThreadState *tstate,
                                PyObject *callable, const PyValue *args,
                                Py_ssize_t positional, Py_ssize_t total,
                                PyObject *kwnames)
{
    return begin_vector(scope, tstate, callable, args, positional, total, kwnames, 0);
}

int
_PyNativeScope_BeginTupleCall(_PyNativeScope *scope, PyThreadState *tstate,
                            PyObject *callable, PyObject *args, PyObject *kwargs)
{
    initialize_scope(scope, tstate);
    scope->vector = 3;
    if (tstate == NULL || callable == NULL || args == NULL || !PyTuple_Check(args) ||
        (kwargs != NULL && !PyDict_Check(kwargs))) {
        PyErr_SetString(PyExc_SystemError, "invalid native tuple-call arguments");
        return -1;
    }
    scope->callable = Py_NewRef(callable);
    scope->callargs = Py_NewRef(args);
    scope->kwargs = Py_XNewRef(kwargs);
    return 0;
}

int
_PyNativeScope_InvokeObjectVector(_PyNativeScope *scope)
{
    assert((scope->vector == 1 || scope->vector == 2) && !scope->ended);
    assert(scope->result.state == _Py_NATIVE_RESULT_EMPTY);
    PyObject *const *args;
    if (_PyNativeScope_Objects(scope, &args) < 0) {
        return -1;
    }
    PyObject *result = PyObject_Vectorcall(scope->callable, args,
                                         scope->positional_count, scope->kwnames);
    return _PyNativeScope_StageResult(scope, result, &scope->result);
}

int
_PyNativeScope_InvokeTupleCall(_PyNativeScope *scope)
{
    assert(scope->vector == 3 && !scope->ended);
    assert(scope->result.state == _Py_NATIVE_RESULT_EMPTY);
    PyObject *result = PyObject_Call(scope->callable, scope->callargs, scope->kwargs);
    return _PyNativeScope_StageResult(scope, result, &scope->result);
}

int
_PyNativeScope_Objects(_PyNativeScope *scope, PyObject *const **args)
{
    assert(!scope->ended);
    *args = scope->objects;
    if (!PyValue_IsNull(scope->receiver) && scope->receiver_object == NULL) {
        scope->receiver_object = _PyValue_AsObjectAt(scope->receiver, BOX_SITE_CALL_ADAPTER);
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
    if (scope->mark != NULL) {
        /* Releases the boxes of inline arguments. */
        _PyBorrowScope_Exit(scope->tstate, scope->mark);
    }
    scope->argument_objects[0] = NULL;
    Py_CLEAR(scope->argument_box);
    Py_CLEAR(scope->receiver_object);
    PyValue_CLEAR(scope->receiver);
    Py_CLEAR(scope->defining_class);
    Py_CLEAR(scope->kwnames);
    Py_CLEAR(scope->kwargs);
    Py_CLEAR(scope->callargs);
    Py_CLEAR(scope->callable);
    scope->mark = NULL;
    scope->objects = NULL;
    scope->nargs = 0;
}
