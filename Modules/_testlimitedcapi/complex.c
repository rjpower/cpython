#include "parts.h"
#include "util.h"


static PyValue
complex_check(PyObject *Py_UNUSED(module), PyValue arg)
{
    PyObject *obj = PyValue_Object(arg);
    if (obj == NULL) {
        return PyValue_NULL;
    }
    NULLABLE(obj);
    return PyValue_FromResult(PyLong_FromLong(PyComplex_Check(obj)));
}

static PyValue
complex_checkexact(PyObject *Py_UNUSED(module), PyValue arg)
{
    PyObject *obj = PyValue_Object(arg);
    if (obj == NULL) {
        return PyValue_NULL;
    }
    NULLABLE(obj);
    return PyValue_FromResult(PyLong_FromLong(PyComplex_CheckExact(obj)));
}

static PyObject *
complex_fromdoubles(PyObject *Py_UNUSED(module), PyObject *args)
{
    double real, imag;

    if (!PyArg_ParseTuple(args, "dd", &real, &imag)) {
        return NULL;
    }

    return PyComplex_FromDoubles(real, imag);
}

static PyValue
complex_realasdouble(PyObject *Py_UNUSED(module), PyValue arg)
{
    PyObject *obj = PyValue_Object(arg);
    if (obj == NULL) {
        return PyValue_NULL;
    }
    double real;

    NULLABLE(obj);
    real = PyComplex_RealAsDouble(obj);

    if (real == -1. && PyErr_Occurred()) {
        return PyValue_NULL;
    }

    return PyValue_FromResult(PyFloat_FromDouble(real));
}

static PyValue
complex_imagasdouble(PyObject *Py_UNUSED(module), PyValue arg)
{
    PyObject *obj = PyValue_Object(arg);
    if (obj == NULL) {
        return PyValue_NULL;
    }
    double imag;

    NULLABLE(obj);
    imag = PyComplex_ImagAsDouble(obj);

    if (imag == -1. && PyErr_Occurred()) {
        return PyValue_NULL;
    }

    return PyValue_FromResult(PyFloat_FromDouble(imag));
}


static PyMethodDef test_methods[] = {
    {"complex_check", _PyCFunction_CAST(complex_check), METH_O},
    {"complex_checkexact", _PyCFunction_CAST(complex_checkexact), METH_O},
    {"complex_fromdoubles", complex_fromdoubles, METH_VARARGS},
    {"complex_realasdouble", _PyCFunction_CAST(complex_realasdouble), METH_O},
    {"complex_imagasdouble", _PyCFunction_CAST(complex_imagasdouble), METH_O},
    {NULL},
};

int
_PyTestLimitedCAPI_Init_Complex(PyObject *mod)
{
    if (PyModule_AddFunctions(mod, test_methods) < 0) {
        return -1;
    }

    return 0;
}
