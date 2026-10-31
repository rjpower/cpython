#include "parts.h"
#include "util.h"


static PyValue
pyweakref_check(PyObject *module, PyValue arg)
{
    PyObject *obj = PyValue_Object(arg);
    if (obj == NULL) {
        return PyValue_NULL;
    }
    NULLABLE(obj);
    return PyValue_FromResult(PyLong_FromLong(PyWeakref_Check(obj)));
}

static PyValue
pyweakref_checkref(PyObject *module, PyValue arg)
{
    PyObject *obj = PyValue_Object(arg);
    if (obj == NULL) {
        return PyValue_NULL;
    }
    NULLABLE(obj);
    return PyValue_FromResult(PyLong_FromLong(PyWeakref_CheckRef(obj)));
}

static PyValue
pyweakref_checkrefexact(PyObject *module, PyValue arg)
{
    PyObject *obj = PyValue_Object(arg);
    if (obj == NULL) {
        return PyValue_NULL;
    }
    NULLABLE(obj);
    return PyValue_FromResult(PyLong_FromLong(PyWeakref_CheckRefExact(obj)));
}

static PyValue
pyweakref_checkproxy(PyObject *module, PyValue arg)
{
    PyObject *obj = PyValue_Object(arg);
    if (obj == NULL) {
        return PyValue_NULL;
    }
    NULLABLE(obj);
    return PyValue_FromResult(PyLong_FromLong(PyWeakref_CheckProxy(obj)));
}

static PyObject *
pyweakref_newref(PyObject *module, PyObject *args)
{
    PyObject *obj;
    PyObject *callback = NULL;
    if (!PyArg_ParseTuple(args, "O|O", &obj, &callback)) {
        return NULL;
    }
    NULLABLE(obj);
    return PyWeakref_NewRef(obj, callback);
}

static PyObject *
pyweakref_newproxy(PyObject *module, PyObject *args)
{
    PyObject *obj;
    PyObject *callback = NULL;
    if (!PyArg_ParseTuple(args, "O|O", &obj, &callback)) {
        return NULL;
    }
    NULLABLE(obj);
    return PyWeakref_NewProxy(obj, callback);
}


static PyMethodDef test_methods[] = {
    {"pyweakref_check", _PyCFunction_CAST(pyweakref_check), METH_O},
    {"pyweakref_checkref", _PyCFunction_CAST(pyweakref_checkref), METH_O},
    {"pyweakref_checkrefexact", _PyCFunction_CAST(pyweakref_checkrefexact), METH_O},
    {"pyweakref_checkproxy", _PyCFunction_CAST(pyweakref_checkproxy), METH_O},
    {"pyweakref_newref", pyweakref_newref, METH_VARARGS},
    {"pyweakref_newproxy", pyweakref_newproxy, METH_VARARGS},
    {NULL},
};

int
_PyTestLimitedCAPI_Init_Weakref(PyObject *m)
{
    return PyModule_AddFunctions(m, test_methods);
}
