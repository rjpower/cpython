/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_modsupport.h"    // _PyArg_CheckPositional()

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
    {"scanstring", _PyCFunction_CAST(py_scanstring), METH_FASTCALL, py_scanstring__doc__},

static PyObject *
py_scanstring_impl(PyObject *module, PyObject *pystr, Py_ssize_t end,
                   int strict);

static PyValue
py_scanstring(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    PyObject *pystr;
    Py_ssize_t end;
    int strict = 1;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("scanstring", nargs, 2, 3)) {
        goto exit;
    }
    {
        PyObject *_arg = PyValue_Object(args[0]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("scanstring", "argument 1", "str", _arg);
            goto exit;
        }
        pystr = _arg;
    }
    {
        Py_ssize_t ival = -1;
        PyValue ivalue = PyNumber_IndexValue(args[1]);
        if (!PyValue_IsNull(ivalue)) {
            ival = PyLong_AsSsize_tValue(ivalue);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        end = ival;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    strict = PyObject_IsTrueValue(args[2]);
    if (strict < 0) {
        goto exit;
    }
skip_optional:
    _return_value = py_scanstring_impl(module, pystr, end, strict);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(py_encode_basestring_ascii__doc__,
"encode_basestring_ascii($module, pystr, /)\n"
"--\n"
"\n"
"Return an ASCII-only JSON representation of a Python string");

#define PY_ENCODE_BASESTRING_ASCII_METHODDEF    \
    {"encode_basestring_ascii", _PyCFunction_CAST(py_encode_basestring_ascii), METH_O, py_encode_basestring_ascii__doc__},

static PyObject *
py_encode_basestring_ascii_impl(PyObject *module, PyObject *pystr);

static PyValue
py_encode_basestring_ascii(PyObject *module, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyObject *pystr;
    PyObject *_return_value;

    {
        PyObject *_arg = PyValue_Object(arg);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("encode_basestring_ascii", "argument", "str", _arg);
            goto exit;
        }
        pystr = _arg;
    }
    _return_value = py_encode_basestring_ascii_impl(module, pystr);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(py_encode_basestring__doc__,
"encode_basestring($module, pystr, /)\n"
"--\n"
"\n"
"Return a JSON representation of a Python string");

#define PY_ENCODE_BASESTRING_METHODDEF    \
    {"encode_basestring", _PyCFunction_CAST(py_encode_basestring), METH_O, py_encode_basestring__doc__},

static PyObject *
py_encode_basestring_impl(PyObject *module, PyObject *pystr);

static PyValue
py_encode_basestring(PyObject *module, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyObject *pystr;
    PyObject *_return_value;

    {
        PyObject *_arg = PyValue_Object(arg);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("encode_basestring", "argument", "str", _arg);
            goto exit;
        }
        pystr = _arg;
    }
    _return_value = py_encode_basestring_impl(module, pystr);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}
/*[clinic end generated code: output=9b5749505d5a4715 input=a9049054013a1b77]*/
