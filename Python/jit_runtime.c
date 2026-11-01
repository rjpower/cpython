#include "Python.h"
#include "pycore_jit.h"
#include "pycore_pystate.h"

/* Stack-owned and thread-local. An already claimed request cannot be inherited
 * by recursive evaluator entries or a callback, even for the same code. */
static _Py_thread_local _PyJitTestRequest *test_request;

_PyJitTestRequest *
_PyJit_TestClaim(PyThreadState *tstate, _PyInterpreterFrame *frame)
{
    _PyJitTestRequest *request = test_request;
    if (request == NULL || request->claimed || request->tstate != tstate ||
        request->code != _PyFrame_GetCode(frame)) {
        return NULL;
    }
    request->claimed = 1;
    request->claimed_frame = frame;
    return request;
}

PyObject *
_PyJit_TestCall(PyObject *function, PyObject *args, Py_ssize_t limit)
{
#if _Py_TAIL_CALL_INTERP || defined(Py_STATS) || defined(Py_GIL_DISABLED)
    PyErr_SetString(PyExc_NotImplementedError, "semantic-step fixture requires the GIL non-stats default evaluator");
    return NULL;
#endif
    if (!PyFunction_Check(function) || !PyTuple_Check(args) || limit < 1) {
        PyErr_SetString(PyExc_ValueError, "expected function, argument tuple and positive step limit");
        return NULL;
    }
    PyCodeObject *code = (PyCodeObject *)PyFunction_GetCode(function);
    Py_INCREF(code);
    _PyJitTestRequest request = {
        .previous = test_request, .tstate = _PyThreadState_GET(),
        .code = code, .remaining = limit,
    };
    test_request = &request;
    PyObject *result = PyObject_Call(function, args, NULL);
    test_request = request.previous;
    request.claimed_frame = NULL;
    Py_DECREF(code);
    if (result == NULL) {
        return NULL;
    }
    PyObject *actions = PyTuple_New(_PyJIT_ACTION_COUNT);
    PyObject *phases = PyTuple_New(_PyJIT_PHASE_COUNT);
    if (actions == NULL || phases == NULL) {
        Py_XDECREF(actions);
        Py_XDECREF(phases);
        Py_DECREF(result);
        return NULL;
    }
    for (int i = 0; i < _PyJIT_ACTION_COUNT + _PyJIT_PHASE_COUNT; i++) {
        PyObject *tuple = i < _PyJIT_ACTION_COUNT ? actions : phases;
        int index = i < _PyJIT_ACTION_COUNT ? i : i - _PyJIT_ACTION_COUNT;
        Py_ssize_t count = i < _PyJIT_ACTION_COUNT ? request.actions[index] : request.phases[index];
        PyObject *value = PyLong_FromSsize_t(count);
        if (value == NULL) {
            Py_DECREF(actions);
            Py_DECREF(phases);
            Py_DECREF(result);
            return NULL;
        }
        PyTuple_SET_ITEM(tuple, index, value);
    }
    return Py_BuildValue("NNN", result, actions, phases);
}
