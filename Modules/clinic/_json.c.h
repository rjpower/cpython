/*[clinic input]
preserve
[clinic start generated code]*/

#ifndef Py_VALUE_CORE
#  error "value-emission Clinic output requires Py_VALUE_CORE"
#endif

#include "pycore_long_value.h"    // _PyValue_IntAsSsize_t()
#include "pycore_modsupport.h"    // _PyArg_NoKwnamesValue()
#include "pycore_value_call.h"    // METH_VALUE
#include "pycore_value_ops.h"     // _PyValue_Index()

PyDoc_STRVAR(py_scanstring__doc__,
"scanstring($module, pystr, end, strict=True, /)\n"
"--\n"
"\n"
"Scan the string s for a JSON string.\n"
"\n"
"End is the index of the character in s after the quote that started the\n"
"JSON string. Unescapes all valid JSON string escape sequences and raises\n"
"ValueError on attempt to decode an invalid string. If strict is False\n"
"then literal control characters are allowed in the string.\n"
"\n"
"Returns a tuple of the decoded string and the index of the character in\n"
"s after the end quote.");

#define PY_SCANSTRING_METHODDEF    \
    {"scanstring", _PyCFunction_CAST(py_scanstring), METH_VALUE, py_scanstring__doc__},

static PyValue
py_scanstring_impl(PyObject *module, PyObject *pystr, Py_ssize_t end,
                   int strict);

static PyValue
py_scanstring(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *pystr;
    Py_ssize_t end;
    int strict = 1;

    if (!_PyArg_NoKwnamesValue("_json.scanstring()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("scanstring", nargs, 2, 3)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("scanstring", "argument 1", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("scanstring", "argument 1", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    pystr = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    {
        Py_ssize_t ival = -1;
        PyValue iobj = _PyValue_Index(PyValue_Borrow(args[1]));
        if (!PyValue_IsNull(iobj)) {
            ival = _PyValue_IntAsSsize_t(PyValue_Borrow(iobj));
            PyValue_CLOSE(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        end = ival;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    strict = _PyValue_IsTrue(PyValue_Borrow(args[2]));
    if (strict < 0) {
        goto exit;
    }
skip_optional:
    return_value = py_scanstring_impl(PyValue_AsKnownObjectBorrow(module), pystr, end, strict);

exit:
    return return_value;
}

PyDoc_STRVAR(py_encode_basestring_ascii__doc__,
"encode_basestring_ascii($module, pystr, /)\n"
"--\n"
"\n"
"Return an ASCII-only JSON representation of a Python string");

#define PY_ENCODE_BASESTRING_ASCII_METHODDEF    \
    {"encode_basestring_ascii", _PyCFunction_CAST(py_encode_basestring_ascii), METH_VALUE|METH_O, py_encode_basestring_ascii__doc__},

static PyValue
py_encode_basestring_ascii_impl(PyObject *module, PyObject *pystr);

static PyValue
py_encode_basestring_ascii(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *pystr;

    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("encode_basestring_ascii", "argument", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("encode_basestring_ascii", "argument", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    pystr = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    return_value = py_encode_basestring_ascii_impl(PyValue_AsKnownObjectBorrow(module), pystr);

exit:
    return return_value;
}

PyDoc_STRVAR(py_encode_basestring__doc__,
"encode_basestring($module, pystr, /)\n"
"--\n"
"\n"
"Return a JSON representation of a Python string");

#define PY_ENCODE_BASESTRING_METHODDEF    \
    {"encode_basestring", _PyCFunction_CAST(py_encode_basestring), METH_VALUE|METH_O, py_encode_basestring__doc__},

static PyValue
py_encode_basestring_impl(PyObject *module, PyObject *pystr);

static PyValue
py_encode_basestring(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *pystr;

    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("encode_basestring", "argument", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("encode_basestring", "argument", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    pystr = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    return_value = py_encode_basestring_impl(PyValue_AsKnownObjectBorrow(module), pystr);

exit:
    return return_value;
}
/*[clinic end generated code: output=8b68448143ece6c9 input=a9049054013a1b77]*/
