#include "parts.h"
#include "util.h"

#include "frameobject.h"          // PyFrame_New()


static PyValue
frame_getlocals(PyObject *self, PyValue frame_value)
{
    PyObject *frame = PyValue_Object(frame_value);
    if (frame == NULL) {
        return PyValue_NULL;
    }
    if (!PyFrame_Check(frame)) {
        PyErr_SetString(PyExc_TypeError, "argument must be a frame");
        return PyValue_NULL;
    }
    return PyValue_FromResult(PyFrame_GetLocals((PyFrameObject *)frame));
}


static PyValue
frame_getglobals(PyObject *self, PyValue frame_value)
{
    PyObject *frame = PyValue_Object(frame_value);
    if (frame == NULL) {
        return PyValue_NULL;
    }
    if (!PyFrame_Check(frame)) {
        PyErr_SetString(PyExc_TypeError, "argument must be a frame");
        return PyValue_NULL;
    }
    return PyValue_FromResult(PyFrame_GetGlobals((PyFrameObject *)frame));
}


static PyValue
frame_getgenerator(PyObject *self, PyValue frame_value)
{
    PyObject *frame = PyValue_Object(frame_value);
    if (frame == NULL) {
        return PyValue_NULL;
    }
    if (!PyFrame_Check(frame)) {
        PyErr_SetString(PyExc_TypeError, "argument must be a frame");
        return PyValue_NULL;
    }
    return PyValue_FromResult(PyFrame_GetGenerator((PyFrameObject *)frame));
}


static PyValue
frame_getbuiltins(PyObject *self, PyValue frame_value)
{
    PyObject *frame = PyValue_Object(frame_value);
    if (frame == NULL) {
        return PyValue_NULL;
    }
    if (!PyFrame_Check(frame)) {
        PyErr_SetString(PyExc_TypeError, "argument must be a frame");
        return PyValue_NULL;
    }
    return PyValue_FromResult(PyFrame_GetBuiltins((PyFrameObject *)frame));
}


static PyValue
frame_getlasti(PyObject *self, PyValue frame_value)
{
    PyObject *frame = PyValue_Object(frame_value);
    if (frame == NULL) {
        return PyValue_NULL;
    }
    if (!PyFrame_Check(frame)) {
        PyErr_SetString(PyExc_TypeError, "argument must be a frame");
        return PyValue_NULL;
    }
    int lasti = PyFrame_GetLasti((PyFrameObject *)frame);
    if (lasti < 0) {
        assert(lasti == -1);
        return PyValue_FromObject(Py_None);
    }
    return PyValue_FromResult(PyLong_FromLong(lasti));
}


static PyObject *
frame_new(PyObject *self, PyObject *args)
{
    PyObject *code, *globals, *locals;
    if (!PyArg_ParseTuple(args, "OOO", &code, &globals, &locals)) {
        return NULL;
    }
    if (!PyCode_Check(code)) {
        PyErr_SetString(PyExc_TypeError, "argument must be a code object");
        return NULL;
    }
    PyThreadState *tstate = PyThreadState_Get();

    return (PyObject *)PyFrame_New(tstate, (PyCodeObject *)code, globals, locals);
}


static PyObject *
frame_getvar(PyObject *self, PyObject *args)
{
    PyObject *frame, *name;
    if (!PyArg_ParseTuple(args, "OO", &frame, &name)) {
        return NULL;
    }
    if (!PyFrame_Check(frame)) {
        PyErr_SetString(PyExc_TypeError, "argument must be a frame");
        return NULL;
    }

    return PyFrame_GetVar((PyFrameObject *)frame, name);
}


static PyObject *
frame_getvarstring(PyObject *self, PyObject *args)
{
    PyObject *frame;
    const char *name;
    if (!PyArg_ParseTuple(args, "Oy", &frame, &name)) {
        return NULL;
    }
    if (!PyFrame_Check(frame)) {
        PyErr_SetString(PyExc_TypeError, "argument must be a frame");
        return NULL;
    }

    return PyFrame_GetVarString((PyFrameObject *)frame, name);
}


static PyMethodDef test_methods[] = {
    {"frame_getlocals", _PyCFunction_CAST(frame_getlocals), METH_O, NULL},
    {"frame_getglobals", _PyCFunction_CAST(frame_getglobals), METH_O, NULL},
    {"frame_getgenerator", _PyCFunction_CAST(frame_getgenerator), METH_O, NULL},
    {"frame_getbuiltins", _PyCFunction_CAST(frame_getbuiltins), METH_O, NULL},
    {"frame_getlasti", _PyCFunction_CAST(frame_getlasti), METH_O, NULL},
    {"frame_new", frame_new, METH_VARARGS, NULL},
    {"frame_getvar", frame_getvar, METH_VARARGS, NULL},
    {"frame_getvarstring", frame_getvarstring, METH_VARARGS, NULL},
    {NULL},
};

int
_PyTestCapi_Init_Frame(PyObject *m)
{
    return PyModule_AddFunctions(m, test_methods);
}

