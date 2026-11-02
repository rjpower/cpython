#include "Python.h"

#include "pycore_native_scope.h"
#include "pycore_tuple.h"


static void
capture_cleanup_error(PyObject **saved)
{
    PyObject *raised = PyErr_GetRaisedException();
    if (raised == NULL) {
        return;
    }
    if (*saved == NULL) {
        *saved = raised;
        return;
    }
    PyErr_SetRaisedException(raised);
    _PyValue_RestoreException(*saved);
    *saved = PyErr_GetRaisedException();
}

static void
close_value(PyValue *slot, PyObject **saved)
{
    PyValue value = *slot;
    *slot = PyValue_NULL;
    PyValue_XCLOSE(value);
    capture_cleanup_error(saved);
}

static void
close_object(PyObject **slot, PyObject **saved)
{
    PyObject *object = *slot;
    *slot = NULL;
    Py_XDECREF(object);
    capture_cleanup_error(saved);
}

static void
initialize_scope(_PyNativeScope *scope, PyThreadState *tstate)
{
    *scope = (_PyNativeScope){
        .tstate = tstate,
        .receiver = PyValue_NULL,
        .argument = PyValue_NULL,
        .result = {
            .value = PyValue_NULL,
            .displaced = PyValue_NULL,
            .state = _Py_NATIVE_RESULT_EMPTY,
        },
    };
    scope->arguments = &scope->argument;
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
        scope->receiver = PyValue_MakeHeapSafe(PyValue_Borrow(self_or_null));
    }
    if (total_count != 0) {
        scope->argument = PyValue_MakeHeapSafe(PyValue_Borrow(args[0]));
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
    if (PyErr_Occurred()) {
        return -1;
    }
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
        PyValue name = PyTuple_GET_VALUE(kwnames, i);
        if (!PyValue_IsObject(name) ||
            !PyUnicode_Check(_PyTuple_GET_OBJECT(kwnames, i)))
        {
            PyErr_SetString(PyExc_TypeError, "keywords must be strings");
            return -1;
        }
    }
    if ((size_t)total_count >
        (size_t)PY_SSIZE_T_MAX / (sizeof(PyValue) + sizeof(PyObject *)))
    {
        PyErr_NoMemory();
        return -1;
    }
    for (Py_ssize_t i = 0; i < total_count; i++) {
        if (!PyValue_IsLanguageValue(effective_args[i]) ||
            !PyValue_IsHeapSafe(effective_args[i]))
        {
            PyErr_SetString(PyExc_SystemError,
                            "native vector arguments must be strong values");
            return -1;
        }
    }
    /* These INCREFs cannot reenter. All metadata and the producer's owning
     * window remain rooted during the first allocation and snapshot. */
    scope->callable = Py_NewRef(callable);
    scope->kwnames = Py_XNewRef(kwnames);
    if (cfunction) {
        scope->receiver_object = Py_XNewRef(PyCFunction_GET_SELF(callable));
        scope->defining_class = (PyTypeObject *)Py_XNewRef(
            (PyObject *)PyCFunction_GET_CLASS(callable));
    }
    if (total_count > 1) {
        size_t values_size = (size_t)total_count * sizeof(PyValue);
        size_t bytes = values_size + (size_t)total_count * sizeof(PyObject *);
        scope->storage = PyMem_Malloc(bytes);
        if (scope->storage == NULL) {
            PyErr_NoMemory();
            return -1;
        }
        scope->arguments = scope->storage;
        scope->objects = (PyObject **)((char *)scope->storage + values_size);
        for (Py_ssize_t i = 0; i < total_count; i++) {
            scope->arguments[i] = PyValue_NULL;
            scope->objects[i] = NULL;
        }
    }
    /* Publish the cleanup count only after every slot is initialized. */
    scope->nargs = total_count;
    scope->positional_count = positional_count;
    for (Py_ssize_t i = 0; i < total_count; i++) {
        scope->arguments[i] = PyValue_MakeHeapSafe(
            PyValue_Borrow(effective_args[i]));
    }
    return PyErr_Occurred() ? -1 : 0;
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
    if (PyErr_Occurred()) {
        return -1;
    }
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
    if (PyErr_Occurred()) {
        return -1;
    }
    PyObject *result = PyObject_Call(scope->callable, scope->callargs, scope->kwargs);
    return _PyNativeScope_StageResult(scope, result, &scope->result);
}

int
_PyNativeScope_Objects(_PyNativeScope *scope, PyObject *const **args)
{
    assert(!scope->ended);
    *args = scope->objects;
    if (PyErr_Occurred()) {
        return -1;
    }
    if (!PyValue_IsNull(scope->receiver) && scope->receiver_object == NULL) {
        scope->receiver_object = PyValue_AsObject(scope->receiver);
        if (scope->receiver_object == NULL) {
            return -1;
        }
    }
    for (Py_ssize_t i = 0; i < scope->nargs; i++) {
        if (scope->objects[i] == NULL) {
            scope->objects[i] = PyValue_AsObject(scope->arguments[i]);
            if (scope->objects[i] == NULL || PyErr_Occurred()) {
                return -1;
            }
        }
    }
    return 0;
}

PyObject *
_PyNativeScope_TakeRetiredResult(_PyNativeScope *scope)
{
    assert(!scope->ended);
    assert(scope->result.state == _Py_NATIVE_RESULT_STAGED ||
           scope->result.state == _Py_NATIVE_RESULT_PUBLISHED);
    PyObject *retired = scope->result.retired;
    scope->result.retired = NULL;
    return retired;
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
    /* Preparation transfers ownership without releasing a scalar box. Even
     * an invalid object-with-error return stays live until caller abort. */
    out->value = PyValue_PrepareOwned(owned_result, &out->retired);
    out->state = _Py_NATIVE_RESULT_STAGED;
    return PyErr_Occurred() ? -1 : 0;
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
    assert(PyValue_IsHeapSafe(out->value));
    assert(PyValue_IsNull(*destination) ||
           (PyValue_IsLanguageValue(*destination) &&
            PyValue_IsHeapSafe(*destination)));
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

int
_PyNativeScope_End(_PyNativeScope *scope)
{
    if (scope->ended) {
        return PyErr_Occurred() ? -1 : 0;
    }
    _PyNativeResult *out = &scope->result;
    assert(out->state == _Py_NATIVE_RESULT_PUBLISHED ||
           out->state == _Py_NATIVE_RESULT_ABORTED);
    scope->ended = 1;
    out->state = _Py_NATIVE_RESULT_DRAINED;
    PyObject *saved = PyErr_GetRaisedException();
    close_value(&out->value, &saved);
    close_value(&out->displaced, &saved);
    close_object(&out->retired, &saved);
    for (Py_ssize_t i = scope->nargs; i-- > 0;) {
        close_object(&scope->objects[i], &saved);
    }
    close_object(&scope->receiver_object, &saved);
    for (Py_ssize_t i = scope->nargs; i-- > 0;) {
        close_value(&scope->arguments[i], &saved);
    }
    close_value(&scope->receiver, &saved);
    PyObject *defining_class = (PyObject *)scope->defining_class;
    scope->defining_class = NULL;
    close_object(&defining_class, &saved);
    close_object(&scope->kwnames, &saved);
    close_object(&scope->kwargs, &saved);
    close_object(&scope->callargs, &saved);
    close_object(&scope->callable, &saved);
    /* No callback can reuse a partially drained array. Keep its storage
     * alive through every owner release, then detach before freeing it. */
    void *storage = scope->storage;
    scope->storage = NULL;
    scope->arguments = NULL;
    scope->objects = NULL;
    scope->nargs = 0;
    if (storage != NULL) {
        PyMem_Free(storage);
        capture_cleanup_error(&saved);
    }
    PyErr_SetRaisedException(saved);
    return PyErr_Occurred() ? -1 : 0;
}
