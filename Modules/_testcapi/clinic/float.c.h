/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_modsupport.h"    // _PyArg_CheckPositional()

PyDoc_STRVAR(_testcapi_float_pack__doc__,
"float_pack($module, size, d, le, /)\n"
"--\n"
"\n"
"Test PyFloat_Pack2(), PyFloat_Pack4() and PyFloat_Pack8()");

#define _TESTCAPI_FLOAT_PACK_METHODDEF    \
    {"float_pack", _PyCFunction_CAST(_testcapi_float_pack), METH_FASTCALL, _testcapi_float_pack__doc__},

static PyObject *
_testcapi_float_pack_impl(PyObject *module, int size, double d, int le);

static PyValue
_testcapi_float_pack(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    int size;
    double d;
    int le;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("float_pack", nargs, 3, 3)) {
        goto exit;
    }
    size = PyLong_AsIntValue(args[0]);
    if (size == -1 && PyErr_Occurred()) {
        goto exit;
    }
    d = PyFloat_AsDoubleValue(args[1]);
    if (d == -1.0 && PyErr_Occurred()) {
        goto exit;
    }
    le = PyLong_AsIntValue(args[2]);
    if (le == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = _testcapi_float_pack_impl(module, size, d, le);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_testcapi_float_unpack__doc__,
"float_unpack($module, data, le, /)\n"
"--\n"
"\n"
"Test PyFloat_Unpack2(), PyFloat_Unpack4() and PyFloat_Unpack8()");

#define _TESTCAPI_FLOAT_UNPACK_METHODDEF    \
    {"float_unpack", _PyCFunction_CAST(_testcapi_float_unpack), METH_FASTCALL, _testcapi_float_unpack__doc__},

static PyObject *
_testcapi_float_unpack_impl(PyObject *module, const char *data,
                            Py_ssize_t data_length, int le);

static PyValue
_testcapi_float_unpack(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    const char *data;
    Py_ssize_t data_length;
    int le;
    PyObject *_return_value;

    if (!PyArg_ParseArray(args, nargs, "y#i:float_unpack",
        &data, &data_length, &le)) {
        goto exit;
    }
    _return_value = _testcapi_float_unpack_impl(module, data, data_length, le);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}
/*[clinic end generated code: output=5e9a1c28b3e2cd6a input=a9049054013a1b77]*/
