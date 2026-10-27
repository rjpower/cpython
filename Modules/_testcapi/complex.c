#include "parts.h"
#include "util.h"


static PyValue
complex_fromccomplex(PyObject *Py_UNUSED(module), PyValue obj_value)
{
    PyObject *obj = PyValue_Object(obj_value);
    if (obj == NULL) {
        return PyValue_NULL;
    }
    Py_complex complex;

    if (!PyArg_Parse(obj, "D", &complex)) {
        return PyValue_NULL;
    }

    return PyValue_FromResult(PyComplex_FromCComplex(complex));
}

static PyValue
complex_asccomplex(PyObject *Py_UNUSED(module), PyValue obj_value)
{
    PyObject *obj = PyValue_Object(obj_value);
    if (obj == NULL) {
        return PyValue_NULL;
    }
    Py_complex complex;

    NULLABLE(obj);
    complex = PyComplex_AsCComplex(obj);

    if (complex.real == -1. && PyErr_Occurred()) {
        return PyValue_NULL;
    }

    return PyValue_FromResult(PyComplex_FromCComplex(complex));
}

static PyValue
_py_c_neg(PyObject *Py_UNUSED(module), PyValue num_value)
{
    PyObject *num = PyValue_Object(num_value);
    if (num == NULL) {
        return PyValue_NULL;
    }
    Py_complex complex;

    complex = PyComplex_AsCComplex(num);
    if (complex.real == -1. && PyErr_Occurred()) {
        return PyValue_NULL;
    }

    return PyValue_FromResult(PyComplex_FromCComplex(_Py_c_neg(complex)));
}

#define _PY_C_FUNC2(suffix)                                      \
    static PyObject *                                            \
    _py_c_##suffix(PyObject *Py_UNUSED(module), PyObject *args)  \
    {                                                            \
        Py_complex a, b, res;                                    \
                                                                 \
        if (!PyArg_ParseTuple(args, "DD", &a, &b)) {             \
            return NULL;                                         \
        }                                                        \
                                                                 \
        errno = 0;                                               \
        res = _Py_c_##suffix(a, b);                              \
        return Py_BuildValue("Di", &res, errno);                 \
    };

_PY_C_FUNC2(sum)
_PY_C_FUNC2(diff)
_PY_C_FUNC2(prod)
_PY_C_FUNC2(quot)
_PY_C_FUNC2(pow)

static PyValue
_py_c_abs(PyObject *Py_UNUSED(module), PyValue obj_value)
{
    PyObject *obj = PyValue_Object(obj_value);
    if (obj == NULL) {
        return PyValue_NULL;
    }
    Py_complex complex;
    double res;

    NULLABLE(obj);
    complex = PyComplex_AsCComplex(obj);

    if (complex.real == -1. && PyErr_Occurred()) {
        return PyValue_NULL;
    }

    res = _Py_c_abs(complex);
    return PyValue_FromResult(Py_BuildValue("di", res, errno));
}


static PyMethodDef test_methods[] = {
    {"complex_fromccomplex", _PyCFunction_CAST(complex_fromccomplex), METH_O},
    {"complex_asccomplex", _PyCFunction_CAST(complex_asccomplex), METH_O},
    {"_py_c_sum", _py_c_sum, METH_VARARGS},
    {"_py_c_diff", _py_c_diff, METH_VARARGS},
    {"_py_c_neg", _PyCFunction_CAST(_py_c_neg), METH_O},
    {"_py_c_prod", _py_c_prod, METH_VARARGS},
    {"_py_c_quot", _py_c_quot, METH_VARARGS},
    {"_py_c_pow", _py_c_pow, METH_VARARGS},
    {"_py_c_abs", _PyCFunction_CAST(_py_c_abs), METH_O},
    {NULL},
};

int
_PyTestCapi_Init_Complex(PyObject *mod)
{
    if (PyModule_AddFunctions(mod, test_methods) < 0) {
        return -1;
    }

    return 0;
}
