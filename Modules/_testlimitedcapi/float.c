#include "parts.h"
#include "util.h"


static PyValue
float_check(PyObject *Py_UNUSED(module), PyValue arg)
{
    PyObject *obj = PyValue_Object(arg);
    if (obj == NULL) {
        return PyValue_NULL;
    }
    NULLABLE(obj);
    return PyValue_FromResult(PyLong_FromLong(PyFloat_Check(obj)));
}

static PyValue
float_checkexact(PyObject *Py_UNUSED(module), PyValue arg)
{
    PyObject *obj = PyValue_Object(arg);
    if (obj == NULL) {
        return PyValue_NULL;
    }
    NULLABLE(obj);
    return PyValue_FromResult(PyLong_FromLong(PyFloat_CheckExact(obj)));
}

static PyValue
float_fromstring(PyObject *Py_UNUSED(module), PyValue arg)
{
    PyObject *obj = PyValue_Object(arg);
    if (obj == NULL) {
        return PyValue_NULL;
    }
    NULLABLE(obj);
    return PyValue_FromResult(PyFloat_FromString(obj));
}

static PyValue
float_fromdouble(PyObject *Py_UNUSED(module), PyValue arg)
{
    PyObject *obj = PyValue_Object(arg);
    if (obj == NULL) {
        return PyValue_NULL;
    }
    double d;

    if (!PyArg_Parse(obj, "d", &d)) {
        return PyValue_NULL;
    }

    return PyValue_FromResult(PyFloat_FromDouble(d));
}

static PyValue
float_asdouble(PyObject *Py_UNUSED(module), PyValue arg)
{
    PyObject *obj = PyValue_Object(arg);
    if (obj == NULL) {
        return PyValue_NULL;
    }
    double d;

    NULLABLE(obj);
    d = PyFloat_AsDouble(obj);
    if (d == -1. && PyErr_Occurred()) {
        return PyValue_NULL;
    }

    return PyValue_FromResult(PyFloat_FromDouble(d));
}

static PyObject *
float_getinfo(PyObject *Py_UNUSED(module), PyObject *Py_UNUSED(arg))
{
    return PyFloat_GetInfo();
}

static PyObject *
float_getmax(PyObject *Py_UNUSED(module), PyObject *Py_UNUSED(arg))
{
    return PyFloat_FromDouble(PyFloat_GetMax());
}

static PyObject *
float_getmin(PyObject *Py_UNUSED(module), PyObject *Py_UNUSED(arg))
{
    return PyFloat_FromDouble(PyFloat_GetMin());
}


static PyMethodDef test_methods[] = {
    {"float_check", _PyCFunction_CAST(float_check), METH_O},
    {"float_checkexact", _PyCFunction_CAST(float_checkexact), METH_O},
    {"float_fromstring", _PyCFunction_CAST(float_fromstring), METH_O},
    {"float_fromdouble", _PyCFunction_CAST(float_fromdouble), METH_O},
    {"float_asdouble", _PyCFunction_CAST(float_asdouble), METH_O},
    {"float_getinfo", float_getinfo, METH_NOARGS},
    {"float_getmax", float_getmax, METH_NOARGS},
    {"float_getmin", float_getmin, METH_NOARGS},
    {NULL},
};

int
_PyTestLimitedCAPI_Init_Float(PyObject *mod)
{
    if (PyModule_AddFunctions(mod, test_methods) < 0) {
        return -1;
    }

    return 0;
}
