/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_modsupport.h"    // _PyArg_CheckPositional()

PyDoc_STRVAR(_testcapi_pyfile_getline__doc__,
"pyfile_getline($module, file, n, /)\n"
"--\n"
"\n");

#define _TESTCAPI_PYFILE_GETLINE_METHODDEF    \
    {"pyfile_getline", _PyCFunction_CAST(_testcapi_pyfile_getline), METH_FASTCALL, _testcapi_pyfile_getline__doc__},

static PyObject *
_testcapi_pyfile_getline_impl(PyObject *module, PyObject *file, int n);

static PyValue
_testcapi_pyfile_getline(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    PyObject *file;
    int n;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("pyfile_getline", nargs, 2, 2)) {
        goto exit;
    }
    file = PyValue_Object(args[0]);
    if (file == NULL) {
        goto exit;
    }
    n = PyLong_AsIntValue(args[1]);
    if (n == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = _testcapi_pyfile_getline_impl(module, file, n);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_testcapi_pyfile_writeobject__doc__,
"pyfile_writeobject($module, obj, file, flags, /)\n"
"--\n"
"\n");

#define _TESTCAPI_PYFILE_WRITEOBJECT_METHODDEF    \
    {"pyfile_writeobject", _PyCFunction_CAST(_testcapi_pyfile_writeobject), METH_FASTCALL, _testcapi_pyfile_writeobject__doc__},

static PyObject *
_testcapi_pyfile_writeobject_impl(PyObject *module, PyObject *obj,
                                  PyObject *file, int flags);

static PyValue
_testcapi_pyfile_writeobject(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    PyObject *obj;
    PyObject *file;
    int flags;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("pyfile_writeobject", nargs, 3, 3)) {
        goto exit;
    }
    obj = PyValue_Object(args[0]);
    if (obj == NULL) {
        goto exit;
    }
    file = PyValue_Object(args[1]);
    if (file == NULL) {
        goto exit;
    }
    flags = PyLong_AsIntValue(args[2]);
    if (flags == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = _testcapi_pyfile_writeobject_impl(module, obj, file, flags);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_testcapi_pyobject_asfiledescriptor__doc__,
"pyobject_asfiledescriptor($module, obj, /)\n"
"--\n"
"\n");

#define _TESTCAPI_PYOBJECT_ASFILEDESCRIPTOR_METHODDEF    \
    {"pyobject_asfiledescriptor", _PyCFunction_CAST(_testcapi_pyobject_asfiledescriptor), METH_O, _testcapi_pyobject_asfiledescriptor__doc__},

static PyObject *
_testcapi_pyobject_asfiledescriptor_impl(PyObject *module, PyObject *obj);

static PyValue
_testcapi_pyobject_asfiledescriptor(PyObject *module, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyObject *obj;
    PyObject *_return_value;

    obj = PyValue_Object(arg);
    if (obj == NULL) {
        goto exit;
    }
    _return_value = _testcapi_pyobject_asfiledescriptor_impl(module, obj);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}
/*[clinic end generated code: output=f4924be02156e0c7 input=a9049054013a1b77]*/
