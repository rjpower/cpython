/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_modsupport.h"    // _PyArg_CheckPositional()

PyDoc_STRVAR(_testlimitedcapi_pack_full_version__doc__,
"pack_full_version($module, major, minor, micro, level, serial, /)\n"
"--\n"
"\n");

#define _TESTLIMITEDCAPI_PACK_FULL_VERSION_METHODDEF    \
    {"pack_full_version", _PyCFunction_CAST(_testlimitedcapi_pack_full_version), METH_FASTCALL, _testlimitedcapi_pack_full_version__doc__},

static PyObject *
_testlimitedcapi_pack_full_version_impl(PyObject *module, int major,
                                        int minor, int micro, int level,
                                        int serial);

static PyValue
_testlimitedcapi_pack_full_version(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    int major;
    int minor;
    int micro;
    int level;
    int serial;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("pack_full_version", nargs, 5, 5)) {
        goto exit;
    }
    major = PyLong_AsIntValue(args[0]);
    if (major == -1 && PyErr_Occurred()) {
        goto exit;
    }
    minor = PyLong_AsIntValue(args[1]);
    if (minor == -1 && PyErr_Occurred()) {
        goto exit;
    }
    micro = PyLong_AsIntValue(args[2]);
    if (micro == -1 && PyErr_Occurred()) {
        goto exit;
    }
    level = PyLong_AsIntValue(args[3]);
    if (level == -1 && PyErr_Occurred()) {
        goto exit;
    }
    serial = PyLong_AsIntValue(args[4]);
    if (serial == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = _testlimitedcapi_pack_full_version_impl(module, major, minor, micro, level, serial);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_testlimitedcapi_pack_version__doc__,
"pack_version($module, major, minor, /)\n"
"--\n"
"\n");

#define _TESTLIMITEDCAPI_PACK_VERSION_METHODDEF    \
    {"pack_version", _PyCFunction_CAST(_testlimitedcapi_pack_version), METH_FASTCALL, _testlimitedcapi_pack_version__doc__},

static PyObject *
_testlimitedcapi_pack_version_impl(PyObject *module, int major, int minor);

static PyValue
_testlimitedcapi_pack_version(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    int major;
    int minor;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("pack_version", nargs, 2, 2)) {
        goto exit;
    }
    major = PyLong_AsIntValue(args[0]);
    if (major == -1 && PyErr_Occurred()) {
        goto exit;
    }
    minor = PyLong_AsIntValue(args[1]);
    if (minor == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = _testlimitedcapi_pack_version_impl(module, major, minor);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}
/*[clinic end generated code: output=44e6c68120d8b4e9 input=a9049054013a1b77]*/
