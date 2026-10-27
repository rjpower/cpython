/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_abstract.h"      // _Py_convert_optional_to_ssize_t()
#include "pycore_critical_section.h"// Py_BEGIN_CRITICAL_SECTION()
#include "pycore_long.h"          // _PyLong_UnsignedShort_Converter()
#include "pycore_modsupport.h"    // _PyArg_CheckPositional()
#include "pycore_runtime.h"       // _Py_ID()
#include "pycore_tuple.h"         // _PyTuple_FromValues()
#include "pycore_unicodeobject.h" // _PyUnicode_AsUTF8NoNUL()

PyDoc_STRVAR(test_empty_function__doc__,
"test_empty_function($module, /)\n"
"--\n"
"\n");

#define TEST_EMPTY_FUNCTION_METHODDEF    \
    {"test_empty_function", (PyCFunction)test_empty_function, METH_NOARGS, test_empty_function__doc__},

static PyObject *
test_empty_function_impl(PyObject *module);

static PyObject *
test_empty_function(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return test_empty_function_impl(module);
}

PyDoc_STRVAR(objects_converter__doc__,
"objects_converter($module, a, b=<unrepresentable>, /)\n"
"--\n"
"\n");

#define OBJECTS_CONVERTER_METHODDEF    \
    {"objects_converter", _PyCFunction_CAST(objects_converter), METH_FASTCALL, objects_converter__doc__},

static PyObject *
objects_converter_impl(PyObject *module, PyObject *a, PyObject *b);

static PyValue
objects_converter(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    PyObject *a;
    PyObject *b = NULL;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("objects_converter", nargs, 1, 2)) {
        goto exit;
    }
    a = PyValue_Object(args[0]);
    if (a == NULL) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    b = PyValue_Object(args[1]);
    if (b == NULL) {
        goto exit;
    }
skip_optional:
    _return_value = objects_converter_impl(module, a, b);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(bytes_object_converter__doc__,
"bytes_object_converter($module, a, /)\n"
"--\n"
"\n");

#define BYTES_OBJECT_CONVERTER_METHODDEF    \
    {"bytes_object_converter", _PyCFunction_CAST(bytes_object_converter), METH_O, bytes_object_converter__doc__},

static PyObject *
bytes_object_converter_impl(PyObject *module, PyBytesObject *a);

static PyValue
bytes_object_converter(PyObject *module, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyBytesObject *a;
    PyObject *_return_value;

    {
        PyObject *_arg = PyValue_Object(arg);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyBytes_Check(_arg)) {
            _PyArg_BadArgument("bytes_object_converter", "argument", "bytes", _arg);
            goto exit;
        }
        a = (PyBytesObject *)_arg;
    }
    _return_value = bytes_object_converter_impl(module, a);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(byte_array_object_converter__doc__,
"byte_array_object_converter($module, a, /)\n"
"--\n"
"\n");

#define BYTE_ARRAY_OBJECT_CONVERTER_METHODDEF    \
    {"byte_array_object_converter", _PyCFunction_CAST(byte_array_object_converter), METH_O, byte_array_object_converter__doc__},

static PyObject *
byte_array_object_converter_impl(PyObject *module, PyByteArrayObject *a);

static PyValue
byte_array_object_converter(PyObject *module, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyByteArrayObject *a;
    PyObject *_return_value;

    {
        PyObject *_arg = PyValue_Object(arg);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyByteArray_Check(_arg)) {
            _PyArg_BadArgument("byte_array_object_converter", "argument", "bytearray", _arg);
            goto exit;
        }
        a = (PyByteArrayObject *)_arg;
    }
    _return_value = byte_array_object_converter_impl(module, a);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(unicode_converter__doc__,
"unicode_converter($module, a, /)\n"
"--\n"
"\n");

#define UNICODE_CONVERTER_METHODDEF    \
    {"unicode_converter", _PyCFunction_CAST(unicode_converter), METH_O, unicode_converter__doc__},

static PyObject *
unicode_converter_impl(PyObject *module, PyObject *a);

static PyValue
unicode_converter(PyObject *module, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyObject *a;
    PyObject *_return_value;

    {
        PyObject *_arg = PyValue_Object(arg);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("unicode_converter", "argument", "str", _arg);
            goto exit;
        }
        a = _arg;
    }
    _return_value = unicode_converter_impl(module, a);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(bool_converter__doc__,
"bool_converter($module, a=True, b=True, c=True, /)\n"
"--\n"
"\n");

#define BOOL_CONVERTER_METHODDEF    \
    {"bool_converter", _PyCFunction_CAST(bool_converter), METH_FASTCALL, bool_converter__doc__},

static PyObject *
bool_converter_impl(PyObject *module, int a, int b, int c);

static PyValue
bool_converter(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    int a = 1;
    int b = 1;
    int c = 1;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("bool_converter", nargs, 0, 3)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    a = PyObject_IsTrueValue(args[0]);
    if (a < 0) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    b = PyObject_IsTrueValue(args[1]);
    if (b < 0) {
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    c = PyLong_AsIntValue(args[2]);
    if (c == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    _return_value = bool_converter_impl(module, a, b, c);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(bool_converter_c_default__doc__,
"bool_converter_c_default($module, a=True, b=False, c=True, d=x, /)\n"
"--\n"
"\n");

#define BOOL_CONVERTER_C_DEFAULT_METHODDEF    \
    {"bool_converter_c_default", _PyCFunction_CAST(bool_converter_c_default), METH_FASTCALL, bool_converter_c_default__doc__},

static PyObject *
bool_converter_c_default_impl(PyObject *module, int a, int b, int c, int d);

static PyValue
bool_converter_c_default(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    int a = 1;
    int b = 0;
    int c = -2;
    int d = -3;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("bool_converter_c_default", nargs, 0, 4)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    a = PyObject_IsTrueValue(args[0]);
    if (a < 0) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    b = PyObject_IsTrueValue(args[1]);
    if (b < 0) {
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    c = PyObject_IsTrueValue(args[2]);
    if (c < 0) {
        goto exit;
    }
    if (nargs < 4) {
        goto skip_optional;
    }
    d = PyObject_IsTrueValue(args[3]);
    if (d < 0) {
        goto exit;
    }
skip_optional:
    _return_value = bool_converter_c_default_impl(module, a, b, c, d);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(char_converter__doc__,
"char_converter($module, a=b\'A\', b=b\'\\x07\', c=b\'\\x08\', d=b\'\\t\', e=b\'\\n\',\n"
"               f=b\'\\x0b\', g=b\'\\x0c\', h=b\'\\r\', i=b\'\"\', j=b\"\'\", k=b\'?\',\n"
"               l=b\'\\\\\', m=b\'\\x00\', n=b\'\\xff\', /)\n"
"--\n"
"\n");

#define CHAR_CONVERTER_METHODDEF    \
    {"char_converter", _PyCFunction_CAST(char_converter), METH_FASTCALL, char_converter__doc__},

static PyObject *
char_converter_impl(PyObject *module, char a, char b, char c, char d, char e,
                    char f, char g, char h, char i, char j, char k, char l,
                    char m, char n);

static PyValue
char_converter(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    char a = 'A';
    char b = '\a';
    char c = '\b';
    char d = '\t';
    char e = '\n';
    char f = '\v';
    char g = '\f';
    char h = '\r';
    char i = '"';
    char j = '\'';
    char k = '?';
    char l = '\\';
    char m = '\0';
    char n = '\377';
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("char_converter", nargs, 0, 14)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    {
        PyObject *_arg = PyValue_Object(args[0]);
        if (_arg == NULL) {
            goto exit;
        }
        if (PyBytes_Check(_arg)) {
            if (PyBytes_GET_SIZE(_arg) != 1) {
                PyErr_Format(PyExc_TypeError,
                    "char_converter(): argument 1 must be a byte string of length 1, "
                    "not a bytes object of length %zd",
                    PyBytes_GET_SIZE(_arg));
                goto exit;
            }
            a = PyBytes_AS_STRING(_arg)[0];
        }
        else if (PyByteArray_Check(_arg)) {
            if (PyByteArray_GET_SIZE(_arg) != 1) {
                PyErr_Format(PyExc_TypeError,
                    "char_converter(): argument 1 must be a byte string of length 1, "
                    "not a bytearray object of length %zd",
                    PyByteArray_GET_SIZE(_arg));
                goto exit;
            }
            a = PyByteArray_AS_STRING(_arg)[0];
        }
        else {
            _PyArg_BadArgument("char_converter", "argument 1", "a byte string of length 1", _arg);
            goto exit;
        }
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    {
        PyObject *_arg = PyValue_Object(args[1]);
        if (_arg == NULL) {
            goto exit;
        }
        if (PyBytes_Check(_arg)) {
            if (PyBytes_GET_SIZE(_arg) != 1) {
                PyErr_Format(PyExc_TypeError,
                    "char_converter(): argument 2 must be a byte string of length 1, "
                    "not a bytes object of length %zd",
                    PyBytes_GET_SIZE(_arg));
                goto exit;
            }
            b = PyBytes_AS_STRING(_arg)[0];
        }
        else if (PyByteArray_Check(_arg)) {
            if (PyByteArray_GET_SIZE(_arg) != 1) {
                PyErr_Format(PyExc_TypeError,
                    "char_converter(): argument 2 must be a byte string of length 1, "
                    "not a bytearray object of length %zd",
                    PyByteArray_GET_SIZE(_arg));
                goto exit;
            }
            b = PyByteArray_AS_STRING(_arg)[0];
        }
        else {
            _PyArg_BadArgument("char_converter", "argument 2", "a byte string of length 1", _arg);
            goto exit;
        }
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    {
        PyObject *_arg = PyValue_Object(args[2]);
        if (_arg == NULL) {
            goto exit;
        }
        if (PyBytes_Check(_arg)) {
            if (PyBytes_GET_SIZE(_arg) != 1) {
                PyErr_Format(PyExc_TypeError,
                    "char_converter(): argument 3 must be a byte string of length 1, "
                    "not a bytes object of length %zd",
                    PyBytes_GET_SIZE(_arg));
                goto exit;
            }
            c = PyBytes_AS_STRING(_arg)[0];
        }
        else if (PyByteArray_Check(_arg)) {
            if (PyByteArray_GET_SIZE(_arg) != 1) {
                PyErr_Format(PyExc_TypeError,
                    "char_converter(): argument 3 must be a byte string of length 1, "
                    "not a bytearray object of length %zd",
                    PyByteArray_GET_SIZE(_arg));
                goto exit;
            }
            c = PyByteArray_AS_STRING(_arg)[0];
        }
        else {
            _PyArg_BadArgument("char_converter", "argument 3", "a byte string of length 1", _arg);
            goto exit;
        }
    }
    if (nargs < 4) {
        goto skip_optional;
    }
    {
        PyObject *_arg = PyValue_Object(args[3]);
        if (_arg == NULL) {
            goto exit;
        }
        if (PyBytes_Check(_arg)) {
            if (PyBytes_GET_SIZE(_arg) != 1) {
                PyErr_Format(PyExc_TypeError,
                    "char_converter(): argument 4 must be a byte string of length 1, "
                    "not a bytes object of length %zd",
                    PyBytes_GET_SIZE(_arg));
                goto exit;
            }
            d = PyBytes_AS_STRING(_arg)[0];
        }
        else if (PyByteArray_Check(_arg)) {
            if (PyByteArray_GET_SIZE(_arg) != 1) {
                PyErr_Format(PyExc_TypeError,
                    "char_converter(): argument 4 must be a byte string of length 1, "
                    "not a bytearray object of length %zd",
                    PyByteArray_GET_SIZE(_arg));
                goto exit;
            }
            d = PyByteArray_AS_STRING(_arg)[0];
        }
        else {
            _PyArg_BadArgument("char_converter", "argument 4", "a byte string of length 1", _arg);
            goto exit;
        }
    }
    if (nargs < 5) {
        goto skip_optional;
    }
    {
        PyObject *_arg = PyValue_Object(args[4]);
        if (_arg == NULL) {
            goto exit;
        }
        if (PyBytes_Check(_arg)) {
            if (PyBytes_GET_SIZE(_arg) != 1) {
                PyErr_Format(PyExc_TypeError,
                    "char_converter(): argument 5 must be a byte string of length 1, "
                    "not a bytes object of length %zd",
                    PyBytes_GET_SIZE(_arg));
                goto exit;
            }
            e = PyBytes_AS_STRING(_arg)[0];
        }
        else if (PyByteArray_Check(_arg)) {
            if (PyByteArray_GET_SIZE(_arg) != 1) {
                PyErr_Format(PyExc_TypeError,
                    "char_converter(): argument 5 must be a byte string of length 1, "
                    "not a bytearray object of length %zd",
                    PyByteArray_GET_SIZE(_arg));
                goto exit;
            }
            e = PyByteArray_AS_STRING(_arg)[0];
        }
        else {
            _PyArg_BadArgument("char_converter", "argument 5", "a byte string of length 1", _arg);
            goto exit;
        }
    }
    if (nargs < 6) {
        goto skip_optional;
    }
    {
        PyObject *_arg = PyValue_Object(args[5]);
        if (_arg == NULL) {
            goto exit;
        }
        if (PyBytes_Check(_arg)) {
            if (PyBytes_GET_SIZE(_arg) != 1) {
                PyErr_Format(PyExc_TypeError,
                    "char_converter(): argument 6 must be a byte string of length 1, "
                    "not a bytes object of length %zd",
                    PyBytes_GET_SIZE(_arg));
                goto exit;
            }
            f = PyBytes_AS_STRING(_arg)[0];
        }
        else if (PyByteArray_Check(_arg)) {
            if (PyByteArray_GET_SIZE(_arg) != 1) {
                PyErr_Format(PyExc_TypeError,
                    "char_converter(): argument 6 must be a byte string of length 1, "
                    "not a bytearray object of length %zd",
                    PyByteArray_GET_SIZE(_arg));
                goto exit;
            }
            f = PyByteArray_AS_STRING(_arg)[0];
        }
        else {
            _PyArg_BadArgument("char_converter", "argument 6", "a byte string of length 1", _arg);
            goto exit;
        }
    }
    if (nargs < 7) {
        goto skip_optional;
    }
    {
        PyObject *_arg = PyValue_Object(args[6]);
        if (_arg == NULL) {
            goto exit;
        }
        if (PyBytes_Check(_arg)) {
            if (PyBytes_GET_SIZE(_arg) != 1) {
                PyErr_Format(PyExc_TypeError,
                    "char_converter(): argument 7 must be a byte string of length 1, "
                    "not a bytes object of length %zd",
                    PyBytes_GET_SIZE(_arg));
                goto exit;
            }
            g = PyBytes_AS_STRING(_arg)[0];
        }
        else if (PyByteArray_Check(_arg)) {
            if (PyByteArray_GET_SIZE(_arg) != 1) {
                PyErr_Format(PyExc_TypeError,
                    "char_converter(): argument 7 must be a byte string of length 1, "
                    "not a bytearray object of length %zd",
                    PyByteArray_GET_SIZE(_arg));
                goto exit;
            }
            g = PyByteArray_AS_STRING(_arg)[0];
        }
        else {
            _PyArg_BadArgument("char_converter", "argument 7", "a byte string of length 1", _arg);
            goto exit;
        }
    }
    if (nargs < 8) {
        goto skip_optional;
    }
    {
        PyObject *_arg = PyValue_Object(args[7]);
        if (_arg == NULL) {
            goto exit;
        }
        if (PyBytes_Check(_arg)) {
            if (PyBytes_GET_SIZE(_arg) != 1) {
                PyErr_Format(PyExc_TypeError,
                    "char_converter(): argument 8 must be a byte string of length 1, "
                    "not a bytes object of length %zd",
                    PyBytes_GET_SIZE(_arg));
                goto exit;
            }
            h = PyBytes_AS_STRING(_arg)[0];
        }
        else if (PyByteArray_Check(_arg)) {
            if (PyByteArray_GET_SIZE(_arg) != 1) {
                PyErr_Format(PyExc_TypeError,
                    "char_converter(): argument 8 must be a byte string of length 1, "
                    "not a bytearray object of length %zd",
                    PyByteArray_GET_SIZE(_arg));
                goto exit;
            }
            h = PyByteArray_AS_STRING(_arg)[0];
        }
        else {
            _PyArg_BadArgument("char_converter", "argument 8", "a byte string of length 1", _arg);
            goto exit;
        }
    }
    if (nargs < 9) {
        goto skip_optional;
    }
    {
        PyObject *_arg = PyValue_Object(args[8]);
        if (_arg == NULL) {
            goto exit;
        }
        if (PyBytes_Check(_arg)) {
            if (PyBytes_GET_SIZE(_arg) != 1) {
                PyErr_Format(PyExc_TypeError,
                    "char_converter(): argument 9 must be a byte string of length 1, "
                    "not a bytes object of length %zd",
                    PyBytes_GET_SIZE(_arg));
                goto exit;
            }
            i = PyBytes_AS_STRING(_arg)[0];
        }
        else if (PyByteArray_Check(_arg)) {
            if (PyByteArray_GET_SIZE(_arg) != 1) {
                PyErr_Format(PyExc_TypeError,
                    "char_converter(): argument 9 must be a byte string of length 1, "
                    "not a bytearray object of length %zd",
                    PyByteArray_GET_SIZE(_arg));
                goto exit;
            }
            i = PyByteArray_AS_STRING(_arg)[0];
        }
        else {
            _PyArg_BadArgument("char_converter", "argument 9", "a byte string of length 1", _arg);
            goto exit;
        }
    }
    if (nargs < 10) {
        goto skip_optional;
    }
    {
        PyObject *_arg = PyValue_Object(args[9]);
        if (_arg == NULL) {
            goto exit;
        }
        if (PyBytes_Check(_arg)) {
            if (PyBytes_GET_SIZE(_arg) != 1) {
                PyErr_Format(PyExc_TypeError,
                    "char_converter(): argument 10 must be a byte string of length 1, "
                    "not a bytes object of length %zd",
                    PyBytes_GET_SIZE(_arg));
                goto exit;
            }
            j = PyBytes_AS_STRING(_arg)[0];
        }
        else if (PyByteArray_Check(_arg)) {
            if (PyByteArray_GET_SIZE(_arg) != 1) {
                PyErr_Format(PyExc_TypeError,
                    "char_converter(): argument 10 must be a byte string of length 1, "
                    "not a bytearray object of length %zd",
                    PyByteArray_GET_SIZE(_arg));
                goto exit;
            }
            j = PyByteArray_AS_STRING(_arg)[0];
        }
        else {
            _PyArg_BadArgument("char_converter", "argument 10", "a byte string of length 1", _arg);
            goto exit;
        }
    }
    if (nargs < 11) {
        goto skip_optional;
    }
    {
        PyObject *_arg = PyValue_Object(args[10]);
        if (_arg == NULL) {
            goto exit;
        }
        if (PyBytes_Check(_arg)) {
            if (PyBytes_GET_SIZE(_arg) != 1) {
                PyErr_Format(PyExc_TypeError,
                    "char_converter(): argument 11 must be a byte string of length 1, "
                    "not a bytes object of length %zd",
                    PyBytes_GET_SIZE(_arg));
                goto exit;
            }
            k = PyBytes_AS_STRING(_arg)[0];
        }
        else if (PyByteArray_Check(_arg)) {
            if (PyByteArray_GET_SIZE(_arg) != 1) {
                PyErr_Format(PyExc_TypeError,
                    "char_converter(): argument 11 must be a byte string of length 1, "
                    "not a bytearray object of length %zd",
                    PyByteArray_GET_SIZE(_arg));
                goto exit;
            }
            k = PyByteArray_AS_STRING(_arg)[0];
        }
        else {
            _PyArg_BadArgument("char_converter", "argument 11", "a byte string of length 1", _arg);
            goto exit;
        }
    }
    if (nargs < 12) {
        goto skip_optional;
    }
    {
        PyObject *_arg = PyValue_Object(args[11]);
        if (_arg == NULL) {
            goto exit;
        }
        if (PyBytes_Check(_arg)) {
            if (PyBytes_GET_SIZE(_arg) != 1) {
                PyErr_Format(PyExc_TypeError,
                    "char_converter(): argument 12 must be a byte string of length 1, "
                    "not a bytes object of length %zd",
                    PyBytes_GET_SIZE(_arg));
                goto exit;
            }
            l = PyBytes_AS_STRING(_arg)[0];
        }
        else if (PyByteArray_Check(_arg)) {
            if (PyByteArray_GET_SIZE(_arg) != 1) {
                PyErr_Format(PyExc_TypeError,
                    "char_converter(): argument 12 must be a byte string of length 1, "
                    "not a bytearray object of length %zd",
                    PyByteArray_GET_SIZE(_arg));
                goto exit;
            }
            l = PyByteArray_AS_STRING(_arg)[0];
        }
        else {
            _PyArg_BadArgument("char_converter", "argument 12", "a byte string of length 1", _arg);
            goto exit;
        }
    }
    if (nargs < 13) {
        goto skip_optional;
    }
    {
        PyObject *_arg = PyValue_Object(args[12]);
        if (_arg == NULL) {
            goto exit;
        }
        if (PyBytes_Check(_arg)) {
            if (PyBytes_GET_SIZE(_arg) != 1) {
                PyErr_Format(PyExc_TypeError,
                    "char_converter(): argument 13 must be a byte string of length 1, "
                    "not a bytes object of length %zd",
                    PyBytes_GET_SIZE(_arg));
                goto exit;
            }
            m = PyBytes_AS_STRING(_arg)[0];
        }
        else if (PyByteArray_Check(_arg)) {
            if (PyByteArray_GET_SIZE(_arg) != 1) {
                PyErr_Format(PyExc_TypeError,
                    "char_converter(): argument 13 must be a byte string of length 1, "
                    "not a bytearray object of length %zd",
                    PyByteArray_GET_SIZE(_arg));
                goto exit;
            }
            m = PyByteArray_AS_STRING(_arg)[0];
        }
        else {
            _PyArg_BadArgument("char_converter", "argument 13", "a byte string of length 1", _arg);
            goto exit;
        }
    }
    if (nargs < 14) {
        goto skip_optional;
    }
    {
        PyObject *_arg = PyValue_Object(args[13]);
        if (_arg == NULL) {
            goto exit;
        }
        if (PyBytes_Check(_arg)) {
            if (PyBytes_GET_SIZE(_arg) != 1) {
                PyErr_Format(PyExc_TypeError,
                    "char_converter(): argument 14 must be a byte string of length 1, "
                    "not a bytes object of length %zd",
                    PyBytes_GET_SIZE(_arg));
                goto exit;
            }
            n = PyBytes_AS_STRING(_arg)[0];
        }
        else if (PyByteArray_Check(_arg)) {
            if (PyByteArray_GET_SIZE(_arg) != 1) {
                PyErr_Format(PyExc_TypeError,
                    "char_converter(): argument 14 must be a byte string of length 1, "
                    "not a bytearray object of length %zd",
                    PyByteArray_GET_SIZE(_arg));
                goto exit;
            }
            n = PyByteArray_AS_STRING(_arg)[0];
        }
        else {
            _PyArg_BadArgument("char_converter", "argument 14", "a byte string of length 1", _arg);
            goto exit;
        }
    }
skip_optional:
    _return_value = char_converter_impl(module, a, b, c, d, e, f, g, h, i, j, k, l, m, n);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(unsigned_char_converter__doc__,
"unsigned_char_converter($module, a=12, b=34, c=56, /)\n"
"--\n"
"\n");

#define UNSIGNED_CHAR_CONVERTER_METHODDEF    \
    {"unsigned_char_converter", _PyCFunction_CAST(unsigned_char_converter), METH_FASTCALL, unsigned_char_converter__doc__},

static PyObject *
unsigned_char_converter_impl(PyObject *module, unsigned char a,
                             unsigned char b, unsigned char c);

static PyValue
unsigned_char_converter(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    unsigned char a = 12;
    unsigned char b = 34;
    unsigned char c = 56;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("unsigned_char_converter", nargs, 0, 3)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    {
        PyObject *_arg = PyValue_Object(args[0]);
        if (_arg == NULL) {
            goto exit;
        }
        {
            long ival = PyLong_AsLong(_arg);
            if (ival == -1 && PyErr_Occurred()) {
                goto exit;
            }
            else if (ival < 0) {
                PyErr_SetString(PyExc_OverflowError,
                                "unsigned byte integer is less than minimum");
                goto exit;
            }
            else if (ival > UCHAR_MAX) {
                PyErr_SetString(PyExc_OverflowError,
                                "unsigned byte integer is greater than maximum");
                goto exit;
            }
            else {
                a = (unsigned char) ival;
            }
        }
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    {
        PyObject *_arg = PyValue_Object(args[1]);
        if (_arg == NULL) {
            goto exit;
        }
        {
            long ival = PyLong_AsLong(_arg);
            if (ival == -1 && PyErr_Occurred()) {
                goto exit;
            }
            else if (ival < 0) {
                PyErr_SetString(PyExc_OverflowError,
                                "unsigned byte integer is less than minimum");
                goto exit;
            }
            else if (ival > UCHAR_MAX) {
                PyErr_SetString(PyExc_OverflowError,
                                "unsigned byte integer is greater than maximum");
                goto exit;
            }
            else {
                b = (unsigned char) ival;
            }
        }
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    {
        PyObject *_arg = PyValue_Object(args[2]);
        if (_arg == NULL) {
            goto exit;
        }
        {
            Py_ssize_t _bytes = PyLong_AsNativeBytes(_arg, &c, sizeof(unsigned char),
                    Py_ASNATIVEBYTES_NATIVE_ENDIAN |
                    Py_ASNATIVEBYTES_ALLOW_INDEX |
                    Py_ASNATIVEBYTES_UNSIGNED_BUFFER);
            if (_bytes < 0) {
                goto exit;
            }
            if ((size_t)_bytes > sizeof(unsigned char)) {
                if (PyErr_WarnEx(PyExc_DeprecationWarning,
                    "integer value out of range", 1) < 0)
                {
                    goto exit;
                }
            }
        }
    }
skip_optional:
    _return_value = unsigned_char_converter_impl(module, a, b, c);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(short_converter__doc__,
"short_converter($module, a=12, /)\n"
"--\n"
"\n");

#define SHORT_CONVERTER_METHODDEF    \
    {"short_converter", _PyCFunction_CAST(short_converter), METH_FASTCALL, short_converter__doc__},

static PyObject *
short_converter_impl(PyObject *module, short a);

static PyValue
short_converter(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    short a = 12;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("short_converter", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    {
        PyObject *_arg = PyValue_Object(args[0]);
        if (_arg == NULL) {
            goto exit;
        }
        {
            long ival = PyLong_AsLong(_arg);
            if (ival == -1 && PyErr_Occurred()) {
                goto exit;
            }
            else if (ival < SHRT_MIN) {
                PyErr_SetString(PyExc_OverflowError,
                                "signed short integer is less than minimum");
                goto exit;
            }
            else if (ival > SHRT_MAX) {
                PyErr_SetString(PyExc_OverflowError,
                                "signed short integer is greater than maximum");
                goto exit;
            }
            else {
                a = (short) ival;
            }
        }
    }
skip_optional:
    _return_value = short_converter_impl(module, a);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(unsigned_short_converter__doc__,
"unsigned_short_converter($module, a=12, b=34, c=56, /)\n"
"--\n"
"\n");

#define UNSIGNED_SHORT_CONVERTER_METHODDEF    \
    {"unsigned_short_converter", _PyCFunction_CAST(unsigned_short_converter), METH_FASTCALL, unsigned_short_converter__doc__},

static PyObject *
unsigned_short_converter_impl(PyObject *module, unsigned short a,
                              unsigned short b, unsigned short c);

static PyValue
unsigned_short_converter(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    unsigned short a = 12;
    unsigned short b = 34;
    unsigned short c = 56;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("unsigned_short_converter", nargs, 0, 3)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    {
        PyObject *_arg = PyValue_Object(args[0]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!_PyLong_UnsignedShort_Converter(_arg, &a)) {
            goto exit;
        }
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    {
        PyObject *_arg = PyValue_Object(args[1]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!_PyLong_UnsignedShort_Converter(_arg, &b)) {
            goto exit;
        }
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    {
        PyObject *_arg = PyValue_Object(args[2]);
        if (_arg == NULL) {
            goto exit;
        }
        {
            Py_ssize_t _bytes = PyLong_AsNativeBytes(_arg, &c, sizeof(unsigned short),
                    Py_ASNATIVEBYTES_NATIVE_ENDIAN |
                    Py_ASNATIVEBYTES_ALLOW_INDEX |
                    Py_ASNATIVEBYTES_UNSIGNED_BUFFER);
            if (_bytes < 0) {
                goto exit;
            }
            if ((size_t)_bytes > sizeof(unsigned short)) {
                if (PyErr_WarnEx(PyExc_DeprecationWarning,
                    "integer value out of range", 1) < 0)
                {
                    goto exit;
                }
            }
        }
    }
skip_optional:
    _return_value = unsigned_short_converter_impl(module, a, b, c);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(int_converter__doc__,
"int_converter($module, a=12, b=34, c=\'-\', /)\n"
"--\n"
"\n");

#define INT_CONVERTER_METHODDEF    \
    {"int_converter", _PyCFunction_CAST(int_converter), METH_FASTCALL, int_converter__doc__},

static PyObject *
int_converter_impl(PyObject *module, int a, int b, int c);

static PyValue
int_converter(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    int a = 12;
    int b = 34;
    int c = '-';
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("int_converter", nargs, 0, 3)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    a = PyLong_AsIntValue(args[0]);
    if (a == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    b = PyLong_AsIntValue(args[1]);
    if (b == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    {
        PyObject *_arg = PyValue_Object(args[2]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("int_converter", "argument 3", "a unicode character", _arg);
            goto exit;
        }
        if (PyUnicode_GET_LENGTH(_arg) != 1) {
            PyErr_Format(PyExc_TypeError,
                "int_converter(): argument 3 must be a unicode character, "
                "not a string of length %zd",
                PyUnicode_GET_LENGTH(_arg));
            goto exit;
        }
        c = PyUnicode_READ_CHAR(_arg, 0);
    }
skip_optional:
    _return_value = int_converter_impl(module, a, b, c);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(unsigned_int_converter__doc__,
"unsigned_int_converter($module, a=12, b=34, c=56, /)\n"
"--\n"
"\n");

#define UNSIGNED_INT_CONVERTER_METHODDEF    \
    {"unsigned_int_converter", _PyCFunction_CAST(unsigned_int_converter), METH_FASTCALL, unsigned_int_converter__doc__},

static PyObject *
unsigned_int_converter_impl(PyObject *module, unsigned int a, unsigned int b,
                            unsigned int c);

static PyValue
unsigned_int_converter(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    unsigned int a = 12;
    unsigned int b = 34;
    unsigned int c = 56;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("unsigned_int_converter", nargs, 0, 3)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    {
        PyObject *_arg = PyValue_Object(args[0]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!_PyLong_UnsignedInt_Converter(_arg, &a)) {
            goto exit;
        }
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    {
        PyObject *_arg = PyValue_Object(args[1]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!_PyLong_UnsignedInt_Converter(_arg, &b)) {
            goto exit;
        }
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    {
        PyObject *_arg = PyValue_Object(args[2]);
        if (_arg == NULL) {
            goto exit;
        }
        {
            Py_ssize_t _bytes = PyLong_AsNativeBytes(_arg, &c, sizeof(unsigned int),
                    Py_ASNATIVEBYTES_NATIVE_ENDIAN |
                    Py_ASNATIVEBYTES_ALLOW_INDEX |
                    Py_ASNATIVEBYTES_UNSIGNED_BUFFER);
            if (_bytes < 0) {
                goto exit;
            }
            if ((size_t)_bytes > sizeof(unsigned int)) {
                if (PyErr_WarnEx(PyExc_DeprecationWarning,
                    "integer value out of range", 1) < 0)
                {
                    goto exit;
                }
            }
        }
    }
skip_optional:
    _return_value = unsigned_int_converter_impl(module, a, b, c);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(long_converter__doc__,
"long_converter($module, a=12, /)\n"
"--\n"
"\n");

#define LONG_CONVERTER_METHODDEF    \
    {"long_converter", _PyCFunction_CAST(long_converter), METH_FASTCALL, long_converter__doc__},

static PyObject *
long_converter_impl(PyObject *module, long a);

static PyValue
long_converter(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    long a = 12;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("long_converter", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    a = PyLong_AsLongValue(args[0]);
    if (a == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    _return_value = long_converter_impl(module, a);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(unsigned_long_converter__doc__,
"unsigned_long_converter($module, a=12, b=34, c=56, /)\n"
"--\n"
"\n");

#define UNSIGNED_LONG_CONVERTER_METHODDEF    \
    {"unsigned_long_converter", _PyCFunction_CAST(unsigned_long_converter), METH_FASTCALL, unsigned_long_converter__doc__},

static PyObject *
unsigned_long_converter_impl(PyObject *module, unsigned long a,
                             unsigned long b, unsigned long c);

static PyValue
unsigned_long_converter(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    unsigned long a = 12;
    unsigned long b = 34;
    unsigned long c = 56;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("unsigned_long_converter", nargs, 0, 3)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    {
        PyObject *_arg = PyValue_Object(args[0]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!_PyLong_UnsignedLong_Converter(_arg, &a)) {
            goto exit;
        }
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    {
        PyObject *_arg = PyValue_Object(args[1]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!_PyLong_UnsignedLong_Converter(_arg, &b)) {
            goto exit;
        }
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    {
        PyObject *_arg = PyValue_Object(args[2]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyIndex_Check(_arg)) {
            _PyArg_BadArgument("unsigned_long_converter", "argument 3", "int", _arg);
            goto exit;
        }
        {
            Py_ssize_t _bytes = PyLong_AsNativeBytes(_arg, &c, sizeof(unsigned long),
                    Py_ASNATIVEBYTES_NATIVE_ENDIAN |
                    Py_ASNATIVEBYTES_ALLOW_INDEX |
                    Py_ASNATIVEBYTES_UNSIGNED_BUFFER);
            if (_bytes < 0) {
                goto exit;
            }
            if ((size_t)_bytes > sizeof(unsigned long)) {
                if (PyErr_WarnEx(PyExc_DeprecationWarning,
                    "integer value out of range", 1) < 0)
                {
                    goto exit;
                }
            }
        }
    }
skip_optional:
    _return_value = unsigned_long_converter_impl(module, a, b, c);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(long_long_converter__doc__,
"long_long_converter($module, a=12, /)\n"
"--\n"
"\n");

#define LONG_LONG_CONVERTER_METHODDEF    \
    {"long_long_converter", _PyCFunction_CAST(long_long_converter), METH_FASTCALL, long_long_converter__doc__},

static PyObject *
long_long_converter_impl(PyObject *module, long long a);

static PyValue
long_long_converter(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    long long a = 12;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("long_long_converter", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    a = PyLong_AsLongLongValue(args[0]);
    if (a == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    _return_value = long_long_converter_impl(module, a);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(unsigned_long_long_converter__doc__,
"unsigned_long_long_converter($module, a=12, b=34, c=56, /)\n"
"--\n"
"\n");

#define UNSIGNED_LONG_LONG_CONVERTER_METHODDEF    \
    {"unsigned_long_long_converter", _PyCFunction_CAST(unsigned_long_long_converter), METH_FASTCALL, unsigned_long_long_converter__doc__},

static PyObject *
unsigned_long_long_converter_impl(PyObject *module, unsigned long long a,
                                  unsigned long long b, unsigned long long c);

static PyValue
unsigned_long_long_converter(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    unsigned long long a = 12;
    unsigned long long b = 34;
    unsigned long long c = 56;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("unsigned_long_long_converter", nargs, 0, 3)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    {
        PyObject *_arg = PyValue_Object(args[0]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!_PyLong_UnsignedLongLong_Converter(_arg, &a)) {
            goto exit;
        }
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    {
        PyObject *_arg = PyValue_Object(args[1]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!_PyLong_UnsignedLongLong_Converter(_arg, &b)) {
            goto exit;
        }
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    {
        PyObject *_arg = PyValue_Object(args[2]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyIndex_Check(_arg)) {
            _PyArg_BadArgument("unsigned_long_long_converter", "argument 3", "int", _arg);
            goto exit;
        }
        {
            Py_ssize_t _bytes = PyLong_AsNativeBytes(_arg, &c, sizeof(unsigned long long),
                    Py_ASNATIVEBYTES_NATIVE_ENDIAN |
                    Py_ASNATIVEBYTES_ALLOW_INDEX |
                    Py_ASNATIVEBYTES_UNSIGNED_BUFFER);
            if (_bytes < 0) {
                goto exit;
            }
            if ((size_t)_bytes > sizeof(unsigned long long)) {
                if (PyErr_WarnEx(PyExc_DeprecationWarning,
                    "integer value out of range", 1) < 0)
                {
                    goto exit;
                }
            }
        }
    }
skip_optional:
    _return_value = unsigned_long_long_converter_impl(module, a, b, c);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(py_ssize_t_converter__doc__,
"py_ssize_t_converter($module, a=12, b=34, c=56, d=78, e=90, f=-12,\n"
"                     g=-34, /)\n"
"--\n"
"\n");

#define PY_SSIZE_T_CONVERTER_METHODDEF    \
    {"py_ssize_t_converter", _PyCFunction_CAST(py_ssize_t_converter), METH_FASTCALL, py_ssize_t_converter__doc__},

static PyObject *
py_ssize_t_converter_impl(PyObject *module, Py_ssize_t a, Py_ssize_t b,
                          Py_ssize_t c, Py_ssize_t d, Py_ssize_t e,
                          Py_ssize_t f, Py_ssize_t g);

static PyValue
py_ssize_t_converter(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    Py_ssize_t a = 12;
    Py_ssize_t b = 34;
    Py_ssize_t c = 56;
    Py_ssize_t d = 78;
    Py_ssize_t e = 90;
    Py_ssize_t f = -12;
    Py_ssize_t g = -34;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("py_ssize_t_converter", nargs, 0, 7)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    {
        Py_ssize_t ival = -1;
        PyValue ivalue = PyNumber_IndexValue(args[0]);
        if (!PyValue_IsNull(ivalue)) {
            ival = PyLong_AsSsize_tValue(ivalue);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        a = ival;
    }
    if (nargs < 2) {
        goto skip_optional;
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
        b = ival;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    {
        PyObject *_arg = PyValue_Object(args[2]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!_Py_convert_optional_to_ssize_t(_arg, &c)) {
            goto exit;
        }
    }
    if (nargs < 4) {
        goto skip_optional;
    }
    {
        Py_ssize_t ival = -1;
        PyValue ivalue = PyNumber_IndexValue(args[3]);
        if (!PyValue_IsNull(ivalue)) {
            ival = PyLong_AsSsize_tValue(ivalue);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        d = ival;
            if (d < 0) {
                PyErr_SetString(PyExc_ValueError,
                                "d cannot be negative");
                goto exit;
            }
    }
    if (nargs < 5) {
        goto skip_optional;
    }
    {
        PyObject *_arg = PyValue_Object(args[4]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!_Py_convert_optional_to_non_negative_ssize_t(_arg, &e)) {
            goto exit;
        }
    }
    if (nargs < 6) {
        goto skip_optional;
    }
    {
        Py_ssize_t ival = -1;
        PyValue ivalue = PyNumber_IndexValue(args[5]);
        if (!PyValue_IsNull(ivalue)) {
            ival = PyLong_AsSsize_tValue(ivalue);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        f = ival;
            if (f < 0) {
                PyErr_SetString(PyExc_ValueError,
                                "f cannot be negative");
                goto exit;
            }
    }
    if (nargs < 7) {
        goto skip_optional;
    }
    {
        PyObject *_arg = PyValue_Object(args[6]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!_Py_convert_optional_to_non_negative_ssize_t(_arg, &g)) {
            goto exit;
        }
    }
skip_optional:
    _return_value = py_ssize_t_converter_impl(module, a, b, c, d, e, f, g);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(slice_index_converter__doc__,
"slice_index_converter($module, a=12, b=34, c=56, /)\n"
"--\n"
"\n");

#define SLICE_INDEX_CONVERTER_METHODDEF    \
    {"slice_index_converter", _PyCFunction_CAST(slice_index_converter), METH_FASTCALL, slice_index_converter__doc__},

static PyObject *
slice_index_converter_impl(PyObject *module, Py_ssize_t a, Py_ssize_t b,
                           Py_ssize_t c);

static PyValue
slice_index_converter(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    Py_ssize_t a = 12;
    Py_ssize_t b = 34;
    Py_ssize_t c = 56;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("slice_index_converter", nargs, 0, 3)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    {
        PyObject *_arg = PyValue_Object(args[0]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!_PyEval_SliceIndex(_arg, &a)) {
            goto exit;
        }
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    {
        PyObject *_arg = PyValue_Object(args[1]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!_PyEval_SliceIndexNotNone(_arg, &b)) {
            goto exit;
        }
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    {
        PyObject *_arg = PyValue_Object(args[2]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!_PyEval_SliceIndex(_arg, &c)) {
            goto exit;
        }
    }
skip_optional:
    _return_value = slice_index_converter_impl(module, a, b, c);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(size_t_converter__doc__,
"size_t_converter($module, a=12, /)\n"
"--\n"
"\n");

#define SIZE_T_CONVERTER_METHODDEF    \
    {"size_t_converter", _PyCFunction_CAST(size_t_converter), METH_FASTCALL, size_t_converter__doc__},

static PyObject *
size_t_converter_impl(PyObject *module, size_t a);

static PyValue
size_t_converter(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    size_t a = 12;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("size_t_converter", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    {
        PyObject *_arg = PyValue_Object(args[0]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!_PyLong_Size_t_Converter(_arg, &a)) {
            goto exit;
        }
    }
skip_optional:
    _return_value = size_t_converter_impl(module, a);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(float_converter__doc__,
"float_converter($module, a=12.5, /)\n"
"--\n"
"\n");

#define FLOAT_CONVERTER_METHODDEF    \
    {"float_converter", _PyCFunction_CAST(float_converter), METH_FASTCALL, float_converter__doc__},

static PyObject *
float_converter_impl(PyObject *module, float a);

static PyValue
float_converter(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    float a = 12.5;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("float_converter", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    a = (float) PyFloat_AsDoubleValue(args[0]);
    if (a == -1.0 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    _return_value = float_converter_impl(module, a);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(double_converter__doc__,
"double_converter($module, a=12.5, /)\n"
"--\n"
"\n");

#define DOUBLE_CONVERTER_METHODDEF    \
    {"double_converter", _PyCFunction_CAST(double_converter), METH_FASTCALL, double_converter__doc__},

static PyObject *
double_converter_impl(PyObject *module, double a);

static PyValue
double_converter(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    double a = 12.5;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("double_converter", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    a = PyFloat_AsDoubleValue(args[0]);
    if (a == -1.0 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    _return_value = double_converter_impl(module, a);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(py_complex_converter__doc__,
"py_complex_converter($module, a, /)\n"
"--\n"
"\n");

#define PY_COMPLEX_CONVERTER_METHODDEF    \
    {"py_complex_converter", _PyCFunction_CAST(py_complex_converter), METH_O, py_complex_converter__doc__},

static PyObject *
py_complex_converter_impl(PyObject *module, Py_complex a);

static PyValue
py_complex_converter(PyObject *module, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    Py_complex a;
    PyObject *_return_value;

    {
        PyObject *_arg = PyValue_Object(arg);
        if (_arg == NULL) {
            goto exit;
        }
        a = PyComplex_AsCComplex(_arg);
        if (PyErr_Occurred()) {
            goto exit;
        }
    }
    _return_value = py_complex_converter_impl(module, a);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(str_converter__doc__,
"str_converter($module, a=\'a\', b=\'b\', c=\'c\', /)\n"
"--\n"
"\n");

#define STR_CONVERTER_METHODDEF    \
    {"str_converter", _PyCFunction_CAST(str_converter), METH_FASTCALL, str_converter__doc__},

static PyObject *
str_converter_impl(PyObject *module, const char *a, const char *b,
                   const char *c, Py_ssize_t c_length);

static PyValue
str_converter(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    const char *a = "a";
    const char *b = "b";
    const char *c = "c";
    Py_ssize_t c_length;
    PyObject *_return_value;

    if (!PyArg_ParseArray(args, nargs, "|sys#:str_converter",
        &a, &b, &c, &c_length)) {
        goto exit;
    }
    _return_value = str_converter_impl(module, a, b, c, c_length);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(str_converter_encoding__doc__,
"str_converter_encoding($module, a, b, c, /)\n"
"--\n"
"\n");

#define STR_CONVERTER_ENCODING_METHODDEF    \
    {"str_converter_encoding", _PyCFunction_CAST(str_converter_encoding), METH_FASTCALL, str_converter_encoding__doc__},

static PyObject *
str_converter_encoding_impl(PyObject *module, char *a, char *b, char *c,
                            Py_ssize_t c_length);

static PyValue
str_converter_encoding(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    char *a = NULL;
    char *b = NULL;
    char *c = NULL;
    Py_ssize_t c_length;
    PyObject *_return_value;

    if (!PyArg_ParseArray(args, nargs, "esetet#:str_converter_encoding",
        "idna", &a, "idna", &b, "idna", &c, &c_length)) {
        goto exit;
    }
    _return_value = str_converter_encoding_impl(module, a, b, c, c_length);
    return_value = PyValue_FromResult(_return_value);
    /* Post parse cleanup for a */
    PyMem_FREE(a);
    /* Post parse cleanup for b */
    PyMem_FREE(b);
    /* Post parse cleanup for c */
    PyMem_FREE(c);

exit:
    return return_value;
}

PyDoc_STRVAR(py_buffer_converter__doc__,
"py_buffer_converter($module, a, b, /)\n"
"--\n"
"\n");

#define PY_BUFFER_CONVERTER_METHODDEF    \
    {"py_buffer_converter", _PyCFunction_CAST(py_buffer_converter), METH_FASTCALL, py_buffer_converter__doc__},

static PyObject *
py_buffer_converter_impl(PyObject *module, Py_buffer *a, Py_buffer *b);

static PyValue
py_buffer_converter(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    Py_buffer a = {NULL, NULL};
    Py_buffer b = {NULL, NULL};
    PyObject *_return_value;

    if (!PyArg_ParseArray(args, nargs, "z*w*:py_buffer_converter",
        &a, &b)) {
        goto exit;
    }
    _return_value = py_buffer_converter_impl(module, &a, &b);
    return_value = PyValue_FromResult(_return_value);

exit:
    /* Cleanup for a */
    if (a.obj) {
       PyBuffer_Release(&a);
    }
    /* Cleanup for b */
    if (b.obj) {
       PyBuffer_Release(&b);
    }

    return return_value;
}

PyDoc_STRVAR(keywords__doc__,
"keywords($module, /, a, b)\n"
"--\n"
"\n");

#define KEYWORDS_METHODDEF    \
    {"keywords", _PyCFunction_CAST(keywords), METH_FASTCALL|METH_KEYWORDS, keywords__doc__},

static PyObject *
keywords_impl(PyObject *module, PyObject *a, PyObject *b);

static PyValue
keywords(PyObject *module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _Py_LATIN1_CHR('a'), _Py_LATIN1_CHR('b'), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"a", "b", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "keywords",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyValue argsbuf[2];
    PyObject *a;
    PyObject *b;
    PyObject *_return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    a = PyValue_Object(args[0]);
    if (a == NULL) {
        goto exit;
    }
    b = PyValue_Object(args[1]);
    if (b == NULL) {
        goto exit;
    }
    _return_value = keywords_impl(module, a, b);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(keywords_kwonly__doc__,
"keywords_kwonly($module, /, a, *, b)\n"
"--\n"
"\n");

#define KEYWORDS_KWONLY_METHODDEF    \
    {"keywords_kwonly", _PyCFunction_CAST(keywords_kwonly), METH_FASTCALL|METH_KEYWORDS, keywords_kwonly__doc__},

static PyObject *
keywords_kwonly_impl(PyObject *module, PyObject *a, PyObject *b);

static PyValue
keywords_kwonly(PyObject *module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _Py_LATIN1_CHR('a'), _Py_LATIN1_CHR('b'), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"a", "b", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "keywords_kwonly",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyValue argsbuf[2];
    PyObject *a;
    PyObject *b;
    PyObject *_return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 1, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    a = PyValue_Object(args[0]);
    if (a == NULL) {
        goto exit;
    }
    b = PyValue_Object(args[1]);
    if (b == NULL) {
        goto exit;
    }
    _return_value = keywords_kwonly_impl(module, a, b);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(keywords_opt__doc__,
"keywords_opt($module, /, a, b=None, c=None)\n"
"--\n"
"\n");

#define KEYWORDS_OPT_METHODDEF    \
    {"keywords_opt", _PyCFunction_CAST(keywords_opt), METH_FASTCALL|METH_KEYWORDS, keywords_opt__doc__},

static PyObject *
keywords_opt_impl(PyObject *module, PyObject *a, PyObject *b, PyObject *c);

static PyValue
keywords_opt(PyObject *module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _Py_LATIN1_CHR('a'), _Py_LATIN1_CHR('b'), _Py_LATIN1_CHR('c'), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"a", "b", "c", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "keywords_opt",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyValue argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *a;
    PyObject *b = Py_None;
    PyObject *c = Py_None;
    PyObject *_return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    a = PyValue_Object(args[0]);
    if (a == NULL) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (!PyValue_IsNull(args[1])) {
        b = PyValue_Object(args[1]);
        if (b == NULL) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    c = PyValue_Object(args[2]);
    if (c == NULL) {
        goto exit;
    }
skip_optional_pos:
    _return_value = keywords_opt_impl(module, a, b, c);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(keywords_opt_kwonly__doc__,
"keywords_opt_kwonly($module, /, a, b=None, *, c=None, d=None)\n"
"--\n"
"\n");

#define KEYWORDS_OPT_KWONLY_METHODDEF    \
    {"keywords_opt_kwonly", _PyCFunction_CAST(keywords_opt_kwonly), METH_FASTCALL|METH_KEYWORDS, keywords_opt_kwonly__doc__},

static PyObject *
keywords_opt_kwonly_impl(PyObject *module, PyObject *a, PyObject *b,
                         PyObject *c, PyObject *d);

static PyValue
keywords_opt_kwonly(PyObject *module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 4
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _Py_LATIN1_CHR('a'), _Py_LATIN1_CHR('b'), _Py_LATIN1_CHR('c'), _Py_LATIN1_CHR('d'), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"a", "b", "c", "d", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "keywords_opt_kwonly",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyValue argsbuf[4];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *a;
    PyObject *b = Py_None;
    PyObject *c = Py_None;
    PyObject *d = Py_None;
    PyObject *_return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    a = PyValue_Object(args[0]);
    if (a == NULL) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (!PyValue_IsNull(args[1])) {
        b = PyValue_Object(args[1]);
        if (b == NULL) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
skip_optional_pos:
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (!PyValue_IsNull(args[2])) {
        c = PyValue_Object(args[2]);
        if (c == NULL) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    d = PyValue_Object(args[3]);
    if (d == NULL) {
        goto exit;
    }
skip_optional_kwonly:
    _return_value = keywords_opt_kwonly_impl(module, a, b, c, d);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(keywords_kwonly_opt__doc__,
"keywords_kwonly_opt($module, /, a, *, b=None, c=None)\n"
"--\n"
"\n");

#define KEYWORDS_KWONLY_OPT_METHODDEF    \
    {"keywords_kwonly_opt", _PyCFunction_CAST(keywords_kwonly_opt), METH_FASTCALL|METH_KEYWORDS, keywords_kwonly_opt__doc__},

static PyObject *
keywords_kwonly_opt_impl(PyObject *module, PyObject *a, PyObject *b,
                         PyObject *c);

static PyValue
keywords_kwonly_opt(PyObject *module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _Py_LATIN1_CHR('a'), _Py_LATIN1_CHR('b'), _Py_LATIN1_CHR('c'), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"a", "b", "c", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "keywords_kwonly_opt",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyValue argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *a;
    PyObject *b = Py_None;
    PyObject *c = Py_None;
    PyObject *_return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    a = PyValue_Object(args[0]);
    if (a == NULL) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (!PyValue_IsNull(args[1])) {
        b = PyValue_Object(args[1]);
        if (b == NULL) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    c = PyValue_Object(args[2]);
    if (c == NULL) {
        goto exit;
    }
skip_optional_kwonly:
    _return_value = keywords_kwonly_opt_impl(module, a, b, c);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(posonly_keywords__doc__,
"posonly_keywords($module, a, /, b)\n"
"--\n"
"\n");

#define POSONLY_KEYWORDS_METHODDEF    \
    {"posonly_keywords", _PyCFunction_CAST(posonly_keywords), METH_FASTCALL|METH_KEYWORDS, posonly_keywords__doc__},

static PyObject *
posonly_keywords_impl(PyObject *module, PyObject *a, PyObject *b);

static PyValue
posonly_keywords(PyObject *module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _Py_LATIN1_CHR('b'), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "b", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "posonly_keywords",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyValue argsbuf[2];
    PyObject *a;
    PyObject *b;
    PyObject *_return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    a = PyValue_Object(args[0]);
    if (a == NULL) {
        goto exit;
    }
    b = PyValue_Object(args[1]);
    if (b == NULL) {
        goto exit;
    }
    _return_value = posonly_keywords_impl(module, a, b);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(posonly_kwonly__doc__,
"posonly_kwonly($module, a, /, *, b)\n"
"--\n"
"\n");

#define POSONLY_KWONLY_METHODDEF    \
    {"posonly_kwonly", _PyCFunction_CAST(posonly_kwonly), METH_FASTCALL|METH_KEYWORDS, posonly_kwonly__doc__},

static PyObject *
posonly_kwonly_impl(PyObject *module, PyObject *a, PyObject *b);

static PyValue
posonly_kwonly(PyObject *module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _Py_LATIN1_CHR('b'), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "b", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "posonly_kwonly",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyValue argsbuf[2];
    PyObject *a;
    PyObject *b;
    PyObject *_return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 1, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    a = PyValue_Object(args[0]);
    if (a == NULL) {
        goto exit;
    }
    b = PyValue_Object(args[1]);
    if (b == NULL) {
        goto exit;
    }
    _return_value = posonly_kwonly_impl(module, a, b);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(posonly_keywords_kwonly__doc__,
"posonly_keywords_kwonly($module, a, /, b, *, c)\n"
"--\n"
"\n");

#define POSONLY_KEYWORDS_KWONLY_METHODDEF    \
    {"posonly_keywords_kwonly", _PyCFunction_CAST(posonly_keywords_kwonly), METH_FASTCALL|METH_KEYWORDS, posonly_keywords_kwonly__doc__},

static PyObject *
posonly_keywords_kwonly_impl(PyObject *module, PyObject *a, PyObject *b,
                             PyObject *c);

static PyValue
posonly_keywords_kwonly(PyObject *module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _Py_LATIN1_CHR('b'), _Py_LATIN1_CHR('c'), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "b", "c", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "posonly_keywords_kwonly",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyValue argsbuf[3];
    PyObject *a;
    PyObject *b;
    PyObject *c;
    PyObject *_return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 2, /*minkw*/ 1, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    a = PyValue_Object(args[0]);
    if (a == NULL) {
        goto exit;
    }
    b = PyValue_Object(args[1]);
    if (b == NULL) {
        goto exit;
    }
    c = PyValue_Object(args[2]);
    if (c == NULL) {
        goto exit;
    }
    _return_value = posonly_keywords_kwonly_impl(module, a, b, c);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(posonly_keywords_opt__doc__,
"posonly_keywords_opt($module, a, /, b, c=None, d=None)\n"
"--\n"
"\n");

#define POSONLY_KEYWORDS_OPT_METHODDEF    \
    {"posonly_keywords_opt", _PyCFunction_CAST(posonly_keywords_opt), METH_FASTCALL|METH_KEYWORDS, posonly_keywords_opt__doc__},

static PyObject *
posonly_keywords_opt_impl(PyObject *module, PyObject *a, PyObject *b,
                          PyObject *c, PyObject *d);

static PyValue
posonly_keywords_opt(PyObject *module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _Py_LATIN1_CHR('b'), _Py_LATIN1_CHR('c'), _Py_LATIN1_CHR('d'), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "b", "c", "d", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "posonly_keywords_opt",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyValue argsbuf[4];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 2;
    PyObject *a;
    PyObject *b;
    PyObject *c = Py_None;
    PyObject *d = Py_None;
    PyObject *_return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 4, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    a = PyValue_Object(args[0]);
    if (a == NULL) {
        goto exit;
    }
    b = PyValue_Object(args[1]);
    if (b == NULL) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (!PyValue_IsNull(args[2])) {
        c = PyValue_Object(args[2]);
        if (c == NULL) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    d = PyValue_Object(args[3]);
    if (d == NULL) {
        goto exit;
    }
skip_optional_pos:
    _return_value = posonly_keywords_opt_impl(module, a, b, c, d);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(posonly_opt_keywords_opt__doc__,
"posonly_opt_keywords_opt($module, a, b=None, /, c=None, d=None)\n"
"--\n"
"\n");

#define POSONLY_OPT_KEYWORDS_OPT_METHODDEF    \
    {"posonly_opt_keywords_opt", _PyCFunction_CAST(posonly_opt_keywords_opt), METH_FASTCALL|METH_KEYWORDS, posonly_opt_keywords_opt__doc__},

static PyObject *
posonly_opt_keywords_opt_impl(PyObject *module, PyObject *a, PyObject *b,
                              PyObject *c, PyObject *d);

static PyValue
posonly_opt_keywords_opt(PyObject *module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _Py_LATIN1_CHR('c'), _Py_LATIN1_CHR('d'), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "", "c", "d", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "posonly_opt_keywords_opt",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyValue argsbuf[4];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *a;
    PyObject *b = Py_None;
    PyObject *c = Py_None;
    PyObject *d = Py_None;
    PyObject *_return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 4, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    a = PyValue_Object(args[0]);
    if (a == NULL) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional_posonly;
    }
    noptargs--;
    b = PyValue_Object(args[1]);
    if (b == NULL) {
        goto exit;
    }
skip_optional_posonly:
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (!PyValue_IsNull(args[2])) {
        c = PyValue_Object(args[2]);
        if (c == NULL) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    d = PyValue_Object(args[3]);
    if (d == NULL) {
        goto exit;
    }
skip_optional_pos:
    _return_value = posonly_opt_keywords_opt_impl(module, a, b, c, d);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(posonly_kwonly_opt__doc__,
"posonly_kwonly_opt($module, a, /, *, b, c=None, d=None)\n"
"--\n"
"\n");

#define POSONLY_KWONLY_OPT_METHODDEF    \
    {"posonly_kwonly_opt", _PyCFunction_CAST(posonly_kwonly_opt), METH_FASTCALL|METH_KEYWORDS, posonly_kwonly_opt__doc__},

static PyObject *
posonly_kwonly_opt_impl(PyObject *module, PyObject *a, PyObject *b,
                        PyObject *c, PyObject *d);

static PyValue
posonly_kwonly_opt(PyObject *module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _Py_LATIN1_CHR('b'), _Py_LATIN1_CHR('c'), _Py_LATIN1_CHR('d'), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "b", "c", "d", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "posonly_kwonly_opt",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyValue argsbuf[4];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 2;
    PyObject *a;
    PyObject *b;
    PyObject *c = Py_None;
    PyObject *d = Py_None;
    PyObject *_return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 1, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    a = PyValue_Object(args[0]);
    if (a == NULL) {
        goto exit;
    }
    b = PyValue_Object(args[1]);
    if (b == NULL) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (!PyValue_IsNull(args[2])) {
        c = PyValue_Object(args[2]);
        if (c == NULL) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    d = PyValue_Object(args[3]);
    if (d == NULL) {
        goto exit;
    }
skip_optional_kwonly:
    _return_value = posonly_kwonly_opt_impl(module, a, b, c, d);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(posonly_opt_kwonly_opt__doc__,
"posonly_opt_kwonly_opt($module, a, b=None, /, *, c=None, d=None)\n"
"--\n"
"\n");

#define POSONLY_OPT_KWONLY_OPT_METHODDEF    \
    {"posonly_opt_kwonly_opt", _PyCFunction_CAST(posonly_opt_kwonly_opt), METH_FASTCALL|METH_KEYWORDS, posonly_opt_kwonly_opt__doc__},

static PyObject *
posonly_opt_kwonly_opt_impl(PyObject *module, PyObject *a, PyObject *b,
                            PyObject *c, PyObject *d);

static PyValue
posonly_opt_kwonly_opt(PyObject *module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _Py_LATIN1_CHR('c'), _Py_LATIN1_CHR('d'), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "", "c", "d", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "posonly_opt_kwonly_opt",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyValue argsbuf[4];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *a;
    PyObject *b = Py_None;
    PyObject *c = Py_None;
    PyObject *d = Py_None;
    PyObject *_return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    a = PyValue_Object(args[0]);
    if (a == NULL) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional_posonly;
    }
    noptargs--;
    b = PyValue_Object(args[1]);
    if (b == NULL) {
        goto exit;
    }
skip_optional_posonly:
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (!PyValue_IsNull(args[2])) {
        c = PyValue_Object(args[2]);
        if (c == NULL) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    d = PyValue_Object(args[3]);
    if (d == NULL) {
        goto exit;
    }
skip_optional_kwonly:
    _return_value = posonly_opt_kwonly_opt_impl(module, a, b, c, d);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(posonly_keywords_kwonly_opt__doc__,
"posonly_keywords_kwonly_opt($module, a, /, b, *, c, d=None, e=None)\n"
"--\n"
"\n");

#define POSONLY_KEYWORDS_KWONLY_OPT_METHODDEF    \
    {"posonly_keywords_kwonly_opt", _PyCFunction_CAST(posonly_keywords_kwonly_opt), METH_FASTCALL|METH_KEYWORDS, posonly_keywords_kwonly_opt__doc__},

static PyObject *
posonly_keywords_kwonly_opt_impl(PyObject *module, PyObject *a, PyObject *b,
                                 PyObject *c, PyObject *d, PyObject *e);

static PyValue
posonly_keywords_kwonly_opt(PyObject *module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 4
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _Py_LATIN1_CHR('b'), _Py_LATIN1_CHR('c'), _Py_LATIN1_CHR('d'), _Py_LATIN1_CHR('e'), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "b", "c", "d", "e", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "posonly_keywords_kwonly_opt",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyValue argsbuf[5];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 3;
    PyObject *a;
    PyObject *b;
    PyObject *c;
    PyObject *d = Py_None;
    PyObject *e = Py_None;
    PyObject *_return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 2, /*minkw*/ 1, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    a = PyValue_Object(args[0]);
    if (a == NULL) {
        goto exit;
    }
    b = PyValue_Object(args[1]);
    if (b == NULL) {
        goto exit;
    }
    c = PyValue_Object(args[2]);
    if (c == NULL) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (!PyValue_IsNull(args[3])) {
        d = PyValue_Object(args[3]);
        if (d == NULL) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    e = PyValue_Object(args[4]);
    if (e == NULL) {
        goto exit;
    }
skip_optional_kwonly:
    _return_value = posonly_keywords_kwonly_opt_impl(module, a, b, c, d, e);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(posonly_keywords_opt_kwonly_opt__doc__,
"posonly_keywords_opt_kwonly_opt($module, a, /, b, c=None, *, d=None,\n"
"                                e=None)\n"
"--\n"
"\n");

#define POSONLY_KEYWORDS_OPT_KWONLY_OPT_METHODDEF    \
    {"posonly_keywords_opt_kwonly_opt", _PyCFunction_CAST(posonly_keywords_opt_kwonly_opt), METH_FASTCALL|METH_KEYWORDS, posonly_keywords_opt_kwonly_opt__doc__},

static PyObject *
posonly_keywords_opt_kwonly_opt_impl(PyObject *module, PyObject *a,
                                     PyObject *b, PyObject *c, PyObject *d,
                                     PyObject *e);

static PyValue
posonly_keywords_opt_kwonly_opt(PyObject *module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 4
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _Py_LATIN1_CHR('b'), _Py_LATIN1_CHR('c'), _Py_LATIN1_CHR('d'), _Py_LATIN1_CHR('e'), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "b", "c", "d", "e", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "posonly_keywords_opt_kwonly_opt",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyValue argsbuf[5];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 2;
    PyObject *a;
    PyObject *b;
    PyObject *c = Py_None;
    PyObject *d = Py_None;
    PyObject *e = Py_None;
    PyObject *_return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    a = PyValue_Object(args[0]);
    if (a == NULL) {
        goto exit;
    }
    b = PyValue_Object(args[1]);
    if (b == NULL) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (!PyValue_IsNull(args[2])) {
        c = PyValue_Object(args[2]);
        if (c == NULL) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
skip_optional_pos:
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (!PyValue_IsNull(args[3])) {
        d = PyValue_Object(args[3]);
        if (d == NULL) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    e = PyValue_Object(args[4]);
    if (e == NULL) {
        goto exit;
    }
skip_optional_kwonly:
    _return_value = posonly_keywords_opt_kwonly_opt_impl(module, a, b, c, d, e);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(posonly_opt_keywords_opt_kwonly_opt__doc__,
"posonly_opt_keywords_opt_kwonly_opt($module, a, b=None, /, c=None, *,\n"
"                                    d=None)\n"
"--\n"
"\n");

#define POSONLY_OPT_KEYWORDS_OPT_KWONLY_OPT_METHODDEF    \
    {"posonly_opt_keywords_opt_kwonly_opt", _PyCFunction_CAST(posonly_opt_keywords_opt_kwonly_opt), METH_FASTCALL|METH_KEYWORDS, posonly_opt_keywords_opt_kwonly_opt__doc__},

static PyObject *
posonly_opt_keywords_opt_kwonly_opt_impl(PyObject *module, PyObject *a,
                                         PyObject *b, PyObject *c,
                                         PyObject *d);

static PyValue
posonly_opt_keywords_opt_kwonly_opt(PyObject *module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _Py_LATIN1_CHR('c'), _Py_LATIN1_CHR('d'), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "", "c", "d", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "posonly_opt_keywords_opt_kwonly_opt",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyValue argsbuf[4];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *a;
    PyObject *b = Py_None;
    PyObject *c = Py_None;
    PyObject *d = Py_None;
    PyObject *_return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    a = PyValue_Object(args[0]);
    if (a == NULL) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional_posonly;
    }
    noptargs--;
    b = PyValue_Object(args[1]);
    if (b == NULL) {
        goto exit;
    }
skip_optional_posonly:
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (!PyValue_IsNull(args[2])) {
        c = PyValue_Object(args[2]);
        if (c == NULL) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
skip_optional_pos:
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    d = PyValue_Object(args[3]);
    if (d == NULL) {
        goto exit;
    }
skip_optional_kwonly:
    _return_value = posonly_opt_keywords_opt_kwonly_opt_impl(module, a, b, c, d);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(keyword_only_parameter__doc__,
"keyword_only_parameter($module, /, *, a)\n"
"--\n"
"\n");

#define KEYWORD_ONLY_PARAMETER_METHODDEF    \
    {"keyword_only_parameter", _PyCFunction_CAST(keyword_only_parameter), METH_FASTCALL|METH_KEYWORDS, keyword_only_parameter__doc__},

static PyObject *
keyword_only_parameter_impl(PyObject *module, PyObject *a);

static PyValue
keyword_only_parameter(PyObject *module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _Py_LATIN1_CHR('a'), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"a", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "keyword_only_parameter",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyValue argsbuf[1];
    PyObject *a;
    PyObject *_return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 0, /*minkw*/ 1, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    a = PyValue_Object(args[0]);
    if (a == NULL) {
        goto exit;
    }
    _return_value = keyword_only_parameter_impl(module, a);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(varpos__doc__,
"varpos($module, /, *args)\n"
"--\n"
"\n");

#define VARPOS_METHODDEF    \
    {"varpos", _PyCFunction_CAST(varpos), METH_FASTCALL, varpos__doc__},

static PyObject *
varpos_impl(PyObject *module, PyObject *args);

static PyValue
varpos(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    PyObject *__clinic_args = NULL;
    PyObject *_return_value;

    __clinic_args = _PyTuple_FromValues(args, nargs);
    if (__clinic_args == NULL) {
        goto exit;
    }
    _return_value = varpos_impl(module, __clinic_args);
    return_value = PyValue_FromResult(_return_value);

exit:
    /* Cleanup for args */
    Py_XDECREF(__clinic_args);

    return return_value;
}

PyDoc_STRVAR(posonly_varpos__doc__,
"posonly_varpos($module, a, b, /, *args)\n"
"--\n"
"\n");

#define POSONLY_VARPOS_METHODDEF    \
    {"posonly_varpos", _PyCFunction_CAST(posonly_varpos), METH_FASTCALL, posonly_varpos__doc__},

static PyObject *
posonly_varpos_impl(PyObject *module, PyObject *a, PyObject *b,
                    PyObject *args);

static PyValue
posonly_varpos(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    PyObject *a;
    PyObject *b;
    PyObject *__clinic_args = NULL;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("posonly_varpos", nargs, 2, PY_SSIZE_T_MAX)) {
        goto exit;
    }
    a = PyValue_Object(args[0]);
    if (a == NULL) {
        goto exit;
    }
    b = PyValue_Object(args[1]);
    if (b == NULL) {
        goto exit;
    }
    __clinic_args = _PyTuple_FromValues(args + 2, nargs - 2);
    if (__clinic_args == NULL) {
        goto exit;
    }
    _return_value = posonly_varpos_impl(module, a, b, __clinic_args);
    return_value = PyValue_FromResult(_return_value);

exit:
    /* Cleanup for args */
    Py_XDECREF(__clinic_args);

    return return_value;
}

PyDoc_STRVAR(posonly_req_opt_varpos__doc__,
"posonly_req_opt_varpos($module, a, b=False, /, *args)\n"
"--\n"
"\n");

#define POSONLY_REQ_OPT_VARPOS_METHODDEF    \
    {"posonly_req_opt_varpos", _PyCFunction_CAST(posonly_req_opt_varpos), METH_FASTCALL, posonly_req_opt_varpos__doc__},

static PyObject *
posonly_req_opt_varpos_impl(PyObject *module, PyObject *a, PyObject *b,
                            PyObject *args);

static PyValue
posonly_req_opt_varpos(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    PyObject *a;
    PyObject *b = Py_False;
    PyObject *__clinic_args = NULL;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("posonly_req_opt_varpos", nargs, 1, PY_SSIZE_T_MAX)) {
        goto exit;
    }
    a = PyValue_Object(args[0]);
    if (a == NULL) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    b = PyValue_Object(args[1]);
    if (b == NULL) {
        goto exit;
    }
skip_optional:
    __clinic_args = nargs > 2
        ? _PyTuple_FromValues(args + 2, nargs - 2)
        : PyTuple_New(0);
    if (__clinic_args == NULL) {
        goto exit;
    }
    _return_value = posonly_req_opt_varpos_impl(module, a, b, __clinic_args);
    return_value = PyValue_FromResult(_return_value);

exit:
    /* Cleanup for args */
    Py_XDECREF(__clinic_args);

    return return_value;
}

PyDoc_STRVAR(posonly_poskw_varpos__doc__,
"posonly_poskw_varpos($module, a, /, b, *args)\n"
"--\n"
"\n");

#define POSONLY_POSKW_VARPOS_METHODDEF    \
    {"posonly_poskw_varpos", _PyCFunction_CAST(posonly_poskw_varpos), METH_FASTCALL|METH_KEYWORDS, posonly_poskw_varpos__doc__},

static PyObject *
posonly_poskw_varpos_impl(PyObject *module, PyObject *a, PyObject *b,
                          PyObject *args);

static PyValue
posonly_poskw_varpos(PyObject *module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _Py_LATIN1_CHR('b'), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "b", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "posonly_poskw_varpos",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyValue argsbuf[2];
    const PyValue *fastargs;
    PyObject *a;
    PyObject *b;
    PyObject *__clinic_args = NULL;
    PyObject *_return_value;

    fastargs = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 1, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    a = PyValue_Object(fastargs[0]);
    if (a == NULL) {
        goto exit;
    }
    b = PyValue_Object(fastargs[1]);
    if (b == NULL) {
        goto exit;
    }
    __clinic_args = nargs > 2
        ? _PyTuple_FromValues(args + 2, nargs - 2)
        : PyTuple_New(0);
    if (__clinic_args == NULL) {
        goto exit;
    }
    _return_value = posonly_poskw_varpos_impl(module, a, b, __clinic_args);
    return_value = PyValue_FromResult(_return_value);

exit:
    /* Cleanup for args */
    Py_XDECREF(__clinic_args);

    return return_value;
}

PyDoc_STRVAR(poskw_varpos__doc__,
"poskw_varpos($module, /, a, *args)\n"
"--\n"
"\n");

#define POSKW_VARPOS_METHODDEF    \
    {"poskw_varpos", _PyCFunction_CAST(poskw_varpos), METH_FASTCALL|METH_KEYWORDS, poskw_varpos__doc__},

static PyObject *
poskw_varpos_impl(PyObject *module, PyObject *a, PyObject *args);

static PyValue
poskw_varpos(PyObject *module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _Py_LATIN1_CHR('a'), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"a", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "poskw_varpos",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyValue argsbuf[1];
    const PyValue *fastargs;
    PyObject *a;
    PyObject *__clinic_args = NULL;
    PyObject *_return_value;

    fastargs = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 1, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    a = PyValue_Object(fastargs[0]);
    if (a == NULL) {
        goto exit;
    }
    __clinic_args = nargs > 1
        ? _PyTuple_FromValues(args + 1, nargs - 1)
        : PyTuple_New(0);
    if (__clinic_args == NULL) {
        goto exit;
    }
    _return_value = poskw_varpos_impl(module, a, __clinic_args);
    return_value = PyValue_FromResult(_return_value);

exit:
    /* Cleanup for args */
    Py_XDECREF(__clinic_args);

    return return_value;
}

PyDoc_STRVAR(poskw_varpos_kwonly_opt__doc__,
"poskw_varpos_kwonly_opt($module, /, a, *args, b=False)\n"
"--\n"
"\n");

#define POSKW_VARPOS_KWONLY_OPT_METHODDEF    \
    {"poskw_varpos_kwonly_opt", _PyCFunction_CAST(poskw_varpos_kwonly_opt), METH_FASTCALL|METH_KEYWORDS, poskw_varpos_kwonly_opt__doc__},

static PyObject *
poskw_varpos_kwonly_opt_impl(PyObject *module, PyObject *a, PyObject *args,
                             int b);

static PyValue
poskw_varpos_kwonly_opt(PyObject *module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _Py_LATIN1_CHR('a'), _Py_LATIN1_CHR('b'), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"a", "b", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "poskw_varpos_kwonly_opt",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyValue argsbuf[2];
    const PyValue *fastargs;
    Py_ssize_t noptargs = Py_MIN(nargs, 1) + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *a;
    PyObject *__clinic_args = NULL;
    int b = 0;
    PyObject *_return_value;

    fastargs = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 1, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    a = PyValue_Object(fastargs[0]);
    if (a == NULL) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    b = PyObject_IsTrueValue(fastargs[1]);
    if (b < 0) {
        goto exit;
    }
skip_optional_kwonly:
    __clinic_args = nargs > 1
        ? _PyTuple_FromValues(args + 1, nargs - 1)
        : PyTuple_New(0);
    if (__clinic_args == NULL) {
        goto exit;
    }
    _return_value = poskw_varpos_kwonly_opt_impl(module, a, __clinic_args, b);
    return_value = PyValue_FromResult(_return_value);

exit:
    /* Cleanup for args */
    Py_XDECREF(__clinic_args);

    return return_value;
}

PyDoc_STRVAR(poskw_varpos_kwonly_opt2__doc__,
"poskw_varpos_kwonly_opt2($module, /, a, *args, b=False, c=False)\n"
"--\n"
"\n");

#define POSKW_VARPOS_KWONLY_OPT2_METHODDEF    \
    {"poskw_varpos_kwonly_opt2", _PyCFunction_CAST(poskw_varpos_kwonly_opt2), METH_FASTCALL|METH_KEYWORDS, poskw_varpos_kwonly_opt2__doc__},

static PyObject *
poskw_varpos_kwonly_opt2_impl(PyObject *module, PyObject *a, PyObject *args,
                              PyObject *b, PyObject *c);

static PyValue
poskw_varpos_kwonly_opt2(PyObject *module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _Py_LATIN1_CHR('a'), _Py_LATIN1_CHR('b'), _Py_LATIN1_CHR('c'), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"a", "b", "c", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "poskw_varpos_kwonly_opt2",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyValue argsbuf[3];
    const PyValue *fastargs;
    Py_ssize_t noptargs = Py_MIN(nargs, 1) + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *a;
    PyObject *__clinic_args = NULL;
    PyObject *b = Py_False;
    PyObject *c = Py_False;
    PyObject *_return_value;

    fastargs = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 1, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    a = PyValue_Object(fastargs[0]);
    if (a == NULL) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (!PyValue_IsNull(fastargs[1])) {
        b = PyValue_Object(fastargs[1]);
        if (b == NULL) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    c = PyValue_Object(fastargs[2]);
    if (c == NULL) {
        goto exit;
    }
skip_optional_kwonly:
    __clinic_args = nargs > 1
        ? _PyTuple_FromValues(args + 1, nargs - 1)
        : PyTuple_New(0);
    if (__clinic_args == NULL) {
        goto exit;
    }
    _return_value = poskw_varpos_kwonly_opt2_impl(module, a, __clinic_args, b, c);
    return_value = PyValue_FromResult(_return_value);

exit:
    /* Cleanup for args */
    Py_XDECREF(__clinic_args);

    return return_value;
}

PyDoc_STRVAR(varpos_kwonly_opt__doc__,
"varpos_kwonly_opt($module, /, *args, b=False)\n"
"--\n"
"\n");

#define VARPOS_KWONLY_OPT_METHODDEF    \
    {"varpos_kwonly_opt", _PyCFunction_CAST(varpos_kwonly_opt), METH_FASTCALL|METH_KEYWORDS, varpos_kwonly_opt__doc__},

static PyObject *
varpos_kwonly_opt_impl(PyObject *module, PyObject *args, PyObject *b);

static PyValue
varpos_kwonly_opt(PyObject *module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _Py_LATIN1_CHR('b'), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"b", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "varpos_kwonly_opt",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyValue argsbuf[1];
    const PyValue *fastargs;
    Py_ssize_t noptargs = 0 + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *__clinic_args = NULL;
    PyObject *b = Py_False;
    PyObject *_return_value;

    fastargs = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 0, /*minkw*/ 0, /*varpos*/ 1, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    b = PyValue_Object(fastargs[0]);
    if (b == NULL) {
        goto exit;
    }
skip_optional_kwonly:
    __clinic_args = _PyTuple_FromValues(args, nargs);
    if (__clinic_args == NULL) {
        goto exit;
    }
    _return_value = varpos_kwonly_opt_impl(module, __clinic_args, b);
    return_value = PyValue_FromResult(_return_value);

exit:
    /* Cleanup for args */
    Py_XDECREF(__clinic_args);

    return return_value;
}

PyDoc_STRVAR(varpos_kwonly_req_opt__doc__,
"varpos_kwonly_req_opt($module, /, *args, a, b=False, c=False)\n"
"--\n"
"\n");

#define VARPOS_KWONLY_REQ_OPT_METHODDEF    \
    {"varpos_kwonly_req_opt", _PyCFunction_CAST(varpos_kwonly_req_opt), METH_FASTCALL|METH_KEYWORDS, varpos_kwonly_req_opt__doc__},

static PyObject *
varpos_kwonly_req_opt_impl(PyObject *module, PyObject *args, PyObject *a,
                           PyObject *b, PyObject *c);

static PyValue
varpos_kwonly_req_opt(PyObject *module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _Py_LATIN1_CHR('a'), _Py_LATIN1_CHR('b'), _Py_LATIN1_CHR('c'), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"a", "b", "c", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "varpos_kwonly_req_opt",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyValue argsbuf[3];
    const PyValue *fastargs;
    Py_ssize_t noptargs = 0 + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *__clinic_args = NULL;
    PyObject *a;
    PyObject *b = Py_False;
    PyObject *c = Py_False;
    PyObject *_return_value;

    fastargs = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 0, /*minkw*/ 1, /*varpos*/ 1, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    a = PyValue_Object(fastargs[0]);
    if (a == NULL) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (!PyValue_IsNull(fastargs[1])) {
        b = PyValue_Object(fastargs[1]);
        if (b == NULL) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    c = PyValue_Object(fastargs[2]);
    if (c == NULL) {
        goto exit;
    }
skip_optional_kwonly:
    __clinic_args = _PyTuple_FromValues(args, nargs);
    if (__clinic_args == NULL) {
        goto exit;
    }
    _return_value = varpos_kwonly_req_opt_impl(module, __clinic_args, a, b, c);
    return_value = PyValue_FromResult(_return_value);

exit:
    /* Cleanup for args */
    Py_XDECREF(__clinic_args);

    return return_value;
}

PyDoc_STRVAR(varpos_array__doc__,
"varpos_array($module, /, *args)\n"
"--\n"
"\n");

#define VARPOS_ARRAY_METHODDEF    \
    {"varpos_array", _PyCFunction_CAST(varpos_array), METH_FASTCALL, varpos_array__doc__},

static PyObject *
varpos_array_impl(PyObject *module, const PyValue *args,
                  Py_ssize_t args_length);

static PyValue
varpos_array(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    const PyValue *__clinic_args;
    Py_ssize_t args_length;
    PyObject *_return_value;

    __clinic_args = args;
    args_length = nargs;
    _return_value = varpos_array_impl(module, __clinic_args, args_length);
    return_value = PyValue_FromResult(_return_value);

    return return_value;
}

PyDoc_STRVAR(posonly_varpos_array__doc__,
"posonly_varpos_array($module, a, b, /, *args)\n"
"--\n"
"\n");

#define POSONLY_VARPOS_ARRAY_METHODDEF    \
    {"posonly_varpos_array", _PyCFunction_CAST(posonly_varpos_array), METH_FASTCALL, posonly_varpos_array__doc__},

static PyObject *
posonly_varpos_array_impl(PyObject *module, PyObject *a, PyObject *b,
                          const PyValue *args, Py_ssize_t args_length);

static PyValue
posonly_varpos_array(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    PyObject *a;
    PyObject *b;
    const PyValue *__clinic_args;
    Py_ssize_t args_length;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("posonly_varpos_array", nargs, 2, PY_SSIZE_T_MAX)) {
        goto exit;
    }
    a = PyValue_Object(args[0]);
    if (a == NULL) {
        goto exit;
    }
    b = PyValue_Object(args[1]);
    if (b == NULL) {
        goto exit;
    }
    __clinic_args = args + 2;
    args_length = nargs - 2;
    _return_value = posonly_varpos_array_impl(module, a, b, __clinic_args, args_length);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(posonly_req_opt_varpos_array__doc__,
"posonly_req_opt_varpos_array($module, a, b=False, /, *args)\n"
"--\n"
"\n");

#define POSONLY_REQ_OPT_VARPOS_ARRAY_METHODDEF    \
    {"posonly_req_opt_varpos_array", _PyCFunction_CAST(posonly_req_opt_varpos_array), METH_FASTCALL, posonly_req_opt_varpos_array__doc__},

static PyObject *
posonly_req_opt_varpos_array_impl(PyObject *module, PyObject *a, PyObject *b,
                                  const PyValue *args,
                                  Py_ssize_t args_length);

static PyValue
posonly_req_opt_varpos_array(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    PyObject *a;
    PyObject *b = Py_False;
    const PyValue *__clinic_args;
    Py_ssize_t args_length;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("posonly_req_opt_varpos_array", nargs, 1, PY_SSIZE_T_MAX)) {
        goto exit;
    }
    a = PyValue_Object(args[0]);
    if (a == NULL) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    b = PyValue_Object(args[1]);
    if (b == NULL) {
        goto exit;
    }
skip_optional:
    __clinic_args = nargs > 2 ? args + 2 : args;
    args_length = Py_MAX(0, nargs - 2);
    _return_value = posonly_req_opt_varpos_array_impl(module, a, b, __clinic_args, args_length);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(posonly_poskw_varpos_array__doc__,
"posonly_poskw_varpos_array($module, a, /, b, *args)\n"
"--\n"
"\n");

#define POSONLY_POSKW_VARPOS_ARRAY_METHODDEF    \
    {"posonly_poskw_varpos_array", _PyCFunction_CAST(posonly_poskw_varpos_array), METH_FASTCALL|METH_KEYWORDS, posonly_poskw_varpos_array__doc__},

static PyObject *
posonly_poskw_varpos_array_impl(PyObject *module, PyObject *a, PyObject *b,
                                const PyValue *args, Py_ssize_t args_length);

static PyValue
posonly_poskw_varpos_array(PyObject *module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _Py_LATIN1_CHR('b'), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "b", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "posonly_poskw_varpos_array",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyValue argsbuf[2];
    const PyValue *fastargs;
    PyObject *a;
    PyObject *b;
    const PyValue *__clinic_args;
    Py_ssize_t args_length;
    PyObject *_return_value;

    fastargs = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 1, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    a = PyValue_Object(fastargs[0]);
    if (a == NULL) {
        goto exit;
    }
    b = PyValue_Object(fastargs[1]);
    if (b == NULL) {
        goto exit;
    }
    __clinic_args = nargs > 2 ? args + 2 : args;
    args_length = Py_MAX(0, nargs - 2);
    _return_value = posonly_poskw_varpos_array_impl(module, a, b, __clinic_args, args_length);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(only_group__doc__,
"only_group([a])");

#define ONLY_GROUP_METHODDEF    \
    {"only_group", _PyCFunction_CAST(only_group), METH_FASTCALL, only_group__doc__},

static PyObject *
only_group_impl(PyObject *module, int group_right_1, PyObject *a);

static PyValue
only_group(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    int group_right_1 = 0;
    PyObject *a = NULL;
    PyObject *_return_value;

    if (nargs > 1) {
        PyErr_SetString(PyExc_TypeError, "only_group requires 0 to 1 arguments");
        goto exit;
    }
    if (nargs >= 1) {
        a = PyValue_Object(args[0]);
        if (a == NULL) {
            goto exit;
        }
        group_right_1 = 1;
    }
    _return_value = only_group_impl(module, group_right_1, a);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(group_and_opt__doc__,
"group_and_opt([a, b,] c=None)");

#define GROUP_AND_OPT_METHODDEF    \
    {"group_and_opt", _PyCFunction_CAST(group_and_opt), METH_FASTCALL, group_and_opt__doc__},

static PyObject *
group_and_opt_impl(PyObject *module, int group_left_1, PyObject *a,
                   PyObject *b, PyObject *c);

static PyValue
group_and_opt(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    int group_left_1 = 0;
    PyObject *a = NULL;
    PyObject *b = NULL;
    PyObject *c = Py_None;
    PyObject *_return_value;

    Py_ssize_t offset = 0;
    if (nargs > 3) {
        PyErr_SetString(PyExc_TypeError, "group_and_opt requires 0 to 3 arguments");
        goto exit;
    }
    if (nargs >= 2) {
        a = PyValue_Object(args[0]);
        if (a == NULL) {
            goto exit;
        }
        b = PyValue_Object(args[1]);
        if (b == NULL) {
            goto exit;
        }
        offset += 2;
        group_left_1 = 1;
    }
    if (nargs <= offset) {
        goto skip_optional;
    }
    c = PyValue_Object(args[offset]);
    if (c == NULL) {
        goto exit;
    }
skip_optional:
    _return_value = group_and_opt_impl(module, group_left_1, a, b, c);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(two_groups_on_left__doc__,
"two_groups_on_left([a, b,] [c,] d)");

#define TWO_GROUPS_ON_LEFT_METHODDEF    \
    {"two_groups_on_left", _PyCFunction_CAST(two_groups_on_left), METH_FASTCALL, two_groups_on_left__doc__},

static PyObject *
two_groups_on_left_impl(PyObject *module, int group_left_1, PyObject *a,
                        PyObject *b, int group_left_2, PyObject *c,
                        PyObject *d);

static PyValue
two_groups_on_left(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    int group_left_1 = 0;
    PyObject *a = NULL;
    PyObject *b = NULL;
    int group_left_2 = 0;
    PyObject *c = NULL;
    PyObject *d;
    PyObject *_return_value;

    if (nargs < 1 || nargs > 4) {
        PyErr_SetString(PyExc_TypeError, "two_groups_on_left requires 1 to 4 arguments");
        goto exit;
    }
    if (nargs >= 3) {
        a = PyValue_Object(args[0]);
        if (a == NULL) {
            goto exit;
        }
        b = PyValue_Object(args[1]);
        if (b == NULL) {
            goto exit;
        }
        group_left_1 = 1;
    }
    if (nargs == 2 || nargs == 4) {
        c = PyValue_Object(args[nargs - 2]);
        if (c == NULL) {
            goto exit;
        }
        group_left_2 = 1;
    }
    d = PyValue_Object(args[nargs - 1]);
    if (d == NULL) {
        goto exit;
    }
    _return_value = two_groups_on_left_impl(module, group_left_1, a, b, group_left_2, c, d);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(two_groups_on_right__doc__,
"two_groups_on_right(a, [b,] [c, d])");

#define TWO_GROUPS_ON_RIGHT_METHODDEF    \
    {"two_groups_on_right", _PyCFunction_CAST(two_groups_on_right), METH_FASTCALL, two_groups_on_right__doc__},

static PyObject *
two_groups_on_right_impl(PyObject *module, PyObject *a, int group_right_1,
                         PyObject *b, int group_right_2, PyObject *c,
                         PyObject *d);

static PyValue
two_groups_on_right(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    PyObject *a;
    int group_right_1 = 0;
    PyObject *b = NULL;
    int group_right_2 = 0;
    PyObject *c = NULL;
    PyObject *d = NULL;
    PyObject *_return_value;

    if (nargs < 1 || nargs > 4) {
        PyErr_SetString(PyExc_TypeError, "two_groups_on_right requires 1 to 4 arguments");
        goto exit;
    }
    a = PyValue_Object(args[0]);
    if (a == NULL) {
        goto exit;
    }
    if (nargs == 2 || nargs == 4) {
        b = PyValue_Object(args[1]);
        if (b == NULL) {
            goto exit;
        }
        group_right_1 = 1;
    }
    if (nargs >= 3) {
        c = PyValue_Object(args[nargs - 2]);
        if (c == NULL) {
            goto exit;
        }
        d = PyValue_Object(args[nargs - 1]);
        if (d == NULL) {
            goto exit;
        }
        group_right_2 = 1;
    }
    _return_value = two_groups_on_right_impl(module, a, group_right_1, b, group_right_2, c, d);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(group_and_two_opt__doc__,
"group_and_two_opt([a, b, c,] d=None, e=None)");

#define GROUP_AND_TWO_OPT_METHODDEF    \
    {"group_and_two_opt", _PyCFunction_CAST(group_and_two_opt), METH_FASTCALL, group_and_two_opt__doc__},

static PyObject *
group_and_two_opt_impl(PyObject *module, int group_left_1, PyObject *a,
                       PyObject *b, PyObject *c, PyObject *d, PyObject *e);

static PyValue
group_and_two_opt(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    int group_left_1 = 0;
    PyObject *a = NULL;
    PyObject *b = NULL;
    PyObject *c = NULL;
    PyObject *d = Py_None;
    PyObject *e = Py_None;
    PyObject *_return_value;

    Py_ssize_t offset = 0;
    if (nargs > 5) {
        PyErr_SetString(PyExc_TypeError, "group_and_two_opt requires 0 to 5 arguments");
        goto exit;
    }
    if (nargs >= 3) {
        a = PyValue_Object(args[0]);
        if (a == NULL) {
            goto exit;
        }
        b = PyValue_Object(args[1]);
        if (b == NULL) {
            goto exit;
        }
        c = PyValue_Object(args[2]);
        if (c == NULL) {
            goto exit;
        }
        offset += 3;
        group_left_1 = 1;
    }
    if (nargs <= offset) {
        goto skip_optional;
    }
    d = PyValue_Object(args[offset]);
    if (d == NULL) {
        goto exit;
    }
    if (nargs <= offset + 1) {
        goto skip_optional;
    }
    e = PyValue_Object(args[offset + 1]);
    if (e == NULL) {
        goto exit;
    }
skip_optional:
    _return_value = group_and_two_opt_impl(module, group_left_1, a, b, c, d, e);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(gh_32092_oob__doc__,
"gh_32092_oob($module, /, pos1, pos2, *varargs, kw1=None, kw2=None)\n"
"--\n"
"\n"
"Proof-of-concept of GH-32092 OOB bug.");

#define GH_32092_OOB_METHODDEF    \
    {"gh_32092_oob", _PyCFunction_CAST(gh_32092_oob), METH_FASTCALL|METH_KEYWORDS, gh_32092_oob__doc__},

static PyObject *
gh_32092_oob_impl(PyObject *module, PyObject *pos1, PyObject *pos2,
                  PyObject *varargs, PyObject *kw1, PyObject *kw2);

static PyValue
gh_32092_oob(PyObject *module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 4
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(pos1), &_Py_ID(pos2), &_Py_ID(kw1), &_Py_ID(kw2), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"pos1", "pos2", "kw1", "kw2", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "gh_32092_oob",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyValue argsbuf[4];
    const PyValue *fastargs;
    Py_ssize_t noptargs = Py_MIN(nargs, 2) + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 2;
    PyObject *pos1;
    PyObject *pos2;
    PyObject *varargs = NULL;
    PyObject *kw1 = Py_None;
    PyObject *kw2 = Py_None;
    PyObject *_return_value;

    fastargs = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 1, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    pos1 = PyValue_Object(fastargs[0]);
    if (pos1 == NULL) {
        goto exit;
    }
    pos2 = PyValue_Object(fastargs[1]);
    if (pos2 == NULL) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (!PyValue_IsNull(fastargs[2])) {
        kw1 = PyValue_Object(fastargs[2]);
        if (kw1 == NULL) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    kw2 = PyValue_Object(fastargs[3]);
    if (kw2 == NULL) {
        goto exit;
    }
skip_optional_kwonly:
    varargs = nargs > 2
        ? _PyTuple_FromValues(args + 2, nargs - 2)
        : PyTuple_New(0);
    if (varargs == NULL) {
        goto exit;
    }
    _return_value = gh_32092_oob_impl(module, pos1, pos2, varargs, kw1, kw2);
    return_value = PyValue_FromResult(_return_value);

exit:
    /* Cleanup for varargs */
    Py_XDECREF(varargs);

    return return_value;
}

PyDoc_STRVAR(gh_32092_kw_pass__doc__,
"gh_32092_kw_pass($module, /, pos, *args, kw=None)\n"
"--\n"
"\n"
"Proof-of-concept of GH-32092 keyword args passing bug.");

#define GH_32092_KW_PASS_METHODDEF    \
    {"gh_32092_kw_pass", _PyCFunction_CAST(gh_32092_kw_pass), METH_FASTCALL|METH_KEYWORDS, gh_32092_kw_pass__doc__},

static PyObject *
gh_32092_kw_pass_impl(PyObject *module, PyObject *pos, PyObject *args,
                      PyObject *kw);

static PyValue
gh_32092_kw_pass(PyObject *module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(pos), &_Py_ID(kw), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"pos", "kw", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "gh_32092_kw_pass",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyValue argsbuf[2];
    const PyValue *fastargs;
    Py_ssize_t noptargs = Py_MIN(nargs, 1) + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *pos;
    PyObject *__clinic_args = NULL;
    PyObject *kw = Py_None;
    PyObject *_return_value;

    fastargs = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 1, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    pos = PyValue_Object(fastargs[0]);
    if (pos == NULL) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    kw = PyValue_Object(fastargs[1]);
    if (kw == NULL) {
        goto exit;
    }
skip_optional_kwonly:
    __clinic_args = nargs > 1
        ? _PyTuple_FromValues(args + 1, nargs - 1)
        : PyTuple_New(0);
    if (__clinic_args == NULL) {
        goto exit;
    }
    _return_value = gh_32092_kw_pass_impl(module, pos, __clinic_args, kw);
    return_value = PyValue_FromResult(_return_value);

exit:
    /* Cleanup for args */
    Py_XDECREF(__clinic_args);

    return return_value;
}

PyDoc_STRVAR(gh_99233_refcount__doc__,
"gh_99233_refcount($module, /, *args)\n"
"--\n"
"\n"
"Proof-of-concept of GH-99233 refcount error bug.");

#define GH_99233_REFCOUNT_METHODDEF    \
    {"gh_99233_refcount", _PyCFunction_CAST(gh_99233_refcount), METH_FASTCALL, gh_99233_refcount__doc__},

static PyObject *
gh_99233_refcount_impl(PyObject *module, PyObject *args);

static PyValue
gh_99233_refcount(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    PyObject *__clinic_args = NULL;
    PyObject *_return_value;

    __clinic_args = _PyTuple_FromValues(args, nargs);
    if (__clinic_args == NULL) {
        goto exit;
    }
    _return_value = gh_99233_refcount_impl(module, __clinic_args);
    return_value = PyValue_FromResult(_return_value);

exit:
    /* Cleanup for args */
    Py_XDECREF(__clinic_args);

    return return_value;
}

PyDoc_STRVAR(gh_99240_double_free__doc__,
"gh_99240_double_free($module, a, b, /)\n"
"--\n"
"\n"
"Proof-of-concept of GH-99240 double-free bug.");

#define GH_99240_DOUBLE_FREE_METHODDEF    \
    {"gh_99240_double_free", _PyCFunction_CAST(gh_99240_double_free), METH_FASTCALL, gh_99240_double_free__doc__},

static PyObject *
gh_99240_double_free_impl(PyObject *module, char *a, char *b);

static PyValue
gh_99240_double_free(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    char *a = NULL;
    char *b = NULL;
    PyObject *_return_value;

    if (!PyArg_ParseArray(args, nargs, "eses:gh_99240_double_free",
        "idna", &a, "idna", &b)) {
        goto exit;
    }
    _return_value = gh_99240_double_free_impl(module, a, b);
    return_value = PyValue_FromResult(_return_value);
    /* Post parse cleanup for a */
    PyMem_FREE(a);
    /* Post parse cleanup for b */
    PyMem_FREE(b);

exit:
    return return_value;
}

PyDoc_STRVAR(null_or_tuple_for_varargs__doc__,
"null_or_tuple_for_varargs($module, /, name, *constraints,\n"
"                          covariant=False)\n"
"--\n"
"\n"
"See https://github.com/python/cpython/issues/110864");

#define NULL_OR_TUPLE_FOR_VARARGS_METHODDEF    \
    {"null_or_tuple_for_varargs", _PyCFunction_CAST(null_or_tuple_for_varargs), METH_FASTCALL|METH_KEYWORDS, null_or_tuple_for_varargs__doc__},

static PyObject *
null_or_tuple_for_varargs_impl(PyObject *module, PyObject *name,
                               PyObject *constraints, int covariant);

static PyValue
null_or_tuple_for_varargs(PyObject *module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(name), &_Py_ID(covariant), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"name", "covariant", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "null_or_tuple_for_varargs",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyValue argsbuf[2];
    const PyValue *fastargs;
    Py_ssize_t noptargs = Py_MIN(nargs, 1) + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *name;
    PyObject *constraints = NULL;
    int covariant = 0;
    PyObject *_return_value;

    fastargs = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 1, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    name = PyValue_Object(fastargs[0]);
    if (name == NULL) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    covariant = PyObject_IsTrueValue(fastargs[1]);
    if (covariant < 0) {
        goto exit;
    }
skip_optional_kwonly:
    constraints = nargs > 1
        ? _PyTuple_FromValues(args + 1, nargs - 1)
        : PyTuple_New(0);
    if (constraints == NULL) {
        goto exit;
    }
    _return_value = null_or_tuple_for_varargs_impl(module, name, constraints, covariant);
    return_value = PyValue_FromResult(_return_value);

exit:
    /* Cleanup for constraints */
    Py_XDECREF(constraints);

    return return_value;
}

PyDoc_STRVAR(clone_f1__doc__,
"clone_f1($module, /, path)\n"
"--\n"
"\n");

#define CLONE_F1_METHODDEF    \
    {"clone_f1", _PyCFunction_CAST(clone_f1), METH_FASTCALL|METH_KEYWORDS, clone_f1__doc__},

static PyObject *
clone_f1_impl(PyObject *module, const char *path);

static PyValue
clone_f1(PyObject *module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(path), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"path", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "clone_f1",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyValue argsbuf[1];
    const char *path;
    PyObject *_return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    {
        PyObject *_arg = PyValue_Object(args[0]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("clone_f1", "argument 'path'", "str", _arg);
            goto exit;
        }
        path = _PyUnicode_AsUTF8NoNUL(_arg);
        if (path == NULL) {
            goto exit;
        }
    }
    _return_value = clone_f1_impl(module, path);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(clone_f2__doc__,
"clone_f2($module, /, path)\n"
"--\n"
"\n");

#define CLONE_F2_METHODDEF    \
    {"clone_f2", _PyCFunction_CAST(clone_f2), METH_FASTCALL|METH_KEYWORDS, clone_f2__doc__},

static PyObject *
clone_f2_impl(PyObject *module, const char *path);

static PyValue
clone_f2(PyObject *module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(path), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"path", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "clone_f2",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyValue argsbuf[1];
    const char *path;
    PyObject *_return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    {
        PyObject *_arg = PyValue_Object(args[0]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("clone_f2", "argument 'path'", "str", _arg);
            goto exit;
        }
        path = _PyUnicode_AsUTF8NoNUL(_arg);
        if (path == NULL) {
            goto exit;
        }
    }
    _return_value = clone_f2_impl(module, path);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(clone_with_conv_f1__doc__,
"clone_with_conv_f1($module, /, path=None)\n"
"--\n"
"\n");

#define CLONE_WITH_CONV_F1_METHODDEF    \
    {"clone_with_conv_f1", _PyCFunction_CAST(clone_with_conv_f1), METH_FASTCALL|METH_KEYWORDS, clone_with_conv_f1__doc__},

static PyObject *
clone_with_conv_f1_impl(PyObject *module, custom_t path);

static PyValue
clone_with_conv_f1(PyObject *module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(path), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"path", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "clone_with_conv_f1",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyValue argsbuf[1];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    custom_t path = {
                .name = "clone_with_conv_f1",
            };
    PyObject *_return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    {
        PyObject *_arg = PyValue_Object(args[0]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!custom_converter(_arg, &path)) {
            goto exit;
        }
    }
skip_optional_pos:
    _return_value = clone_with_conv_f1_impl(module, path);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(clone_with_conv_f2__doc__,
"clone_with_conv_f2($module, /, path=None)\n"
"--\n"
"\n");

#define CLONE_WITH_CONV_F2_METHODDEF    \
    {"clone_with_conv_f2", _PyCFunction_CAST(clone_with_conv_f2), METH_FASTCALL|METH_KEYWORDS, clone_with_conv_f2__doc__},

static PyObject *
clone_with_conv_f2_impl(PyObject *module, custom_t path);

static PyValue
clone_with_conv_f2(PyObject *module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(path), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"path", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "clone_with_conv_f2",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyValue argsbuf[1];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    custom_t path = {
                .name = "clone_with_conv_f2",
            };
    PyObject *_return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    {
        PyObject *_arg = PyValue_Object(args[0]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!custom_converter(_arg, &path)) {
            goto exit;
        }
    }
skip_optional_pos:
    _return_value = clone_with_conv_f2_impl(module, path);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(alias_pos__doc__,
"alias_pos($module, /, a=None)\n"
"--\n"
"\n");

#define ALIAS_POS_METHODDEF    \
    {"alias_pos", _PyCFunction_CAST(alias_pos), METH_FASTCALL|METH_KEYWORDS, alias_pos__doc__},

static PyObject *
alias_pos_impl(PyObject *module, PyObject *a);

static PyValue
alias_pos(PyObject *module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _Py_LATIN1_CHR('a'), _Py_LATIN1_CHR('b'), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"a", "b", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "alias_pos",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyValue argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *a = Py_None;
    PyObject *_return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (!PyValue_IsNull(args[0])) {
        a = PyValue_Object(args[0]);
        if (a == NULL) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
skip_optional_pos:
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (!PyValue_IsNull(args[0])) {
        PyErr_Format(PyExc_TypeError,
                "argument for alias_pos() given by "
                "name ('b') and %s", 0 < nargs ? "position (1)" : "name ('a')");
        goto exit;
    }
    a = PyValue_Object(args[1]);
    if (a == NULL) {
        goto exit;
    }
skip_optional_kwonly:
    _return_value = alias_pos_impl(module, a);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(alias_kwonly__doc__,
"alias_kwonly($module, /, *, a=None)\n"
"--\n"
"\n");

#define ALIAS_KWONLY_METHODDEF    \
    {"alias_kwonly", _PyCFunction_CAST(alias_kwonly), METH_FASTCALL|METH_KEYWORDS, alias_kwonly__doc__},

static PyObject *
alias_kwonly_impl(PyObject *module, PyObject *a);

static PyValue
alias_kwonly(PyObject *module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _Py_LATIN1_CHR('a'), _Py_LATIN1_CHR('b'), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"a", "b", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "alias_kwonly",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyValue argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *a = Py_None;
    PyObject *_return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 0, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (!PyValue_IsNull(args[0])) {
        a = PyValue_Object(args[0]);
        if (a == NULL) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (!PyValue_IsNull(args[0])) {
        PyErr_Format(PyExc_TypeError,
                "argument for alias_kwonly() given by "
                "name ('b') and name ('a')");
        goto exit;
    }
    a = PyValue_Object(args[1]);
    if (a == NULL) {
        goto exit;
    }
skip_optional_kwonly:
    _return_value = alias_kwonly_impl(module, a);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_testclinic_TestClass_get_defining_class__doc__,
"get_defining_class($self, /)\n"
"--\n"
"\n");

#define _TESTCLINIC_TESTCLASS_GET_DEFINING_CLASS_METHODDEF    \
    {"get_defining_class", _PyCFunction_CAST(_testclinic_TestClass_get_defining_class), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _testclinic_TestClass_get_defining_class__doc__},

static PyObject *
_testclinic_TestClass_get_defining_class_impl(PyObject *self,
                                              PyTypeObject *cls);

static PyValue
_testclinic_TestClass_get_defining_class(PyObject *self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "get_defining_class() takes no arguments");
        return PyValue_NULL;
    }
    return PyValue_FromResult(_testclinic_TestClass_get_defining_class_impl(self, cls));
}

PyDoc_STRVAR(_testclinic_TestClass_get_defining_class_arg__doc__,
"get_defining_class_arg($self, /, arg)\n"
"--\n"
"\n");

#define _TESTCLINIC_TESTCLASS_GET_DEFINING_CLASS_ARG_METHODDEF    \
    {"get_defining_class_arg", _PyCFunction_CAST(_testclinic_TestClass_get_defining_class_arg), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _testclinic_TestClass_get_defining_class_arg__doc__},

static PyObject *
_testclinic_TestClass_get_defining_class_arg_impl(PyObject *self,
                                                  PyTypeObject *cls,
                                                  PyObject *arg);

static PyValue
_testclinic_TestClass_get_defining_class_arg(PyObject *self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(arg), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"arg", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "get_defining_class_arg",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyValue argsbuf[1];
    PyObject *arg;
    PyObject *_return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    arg = PyValue_Object(args[0]);
    if (arg == NULL) {
        goto exit;
    }
    _return_value = _testclinic_TestClass_get_defining_class_arg_impl(self, cls, arg);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_testclinic_TestClass_defclass_varpos__doc__,
"defclass_varpos($self, /, *args)\n"
"--\n"
"\n");

#define _TESTCLINIC_TESTCLASS_DEFCLASS_VARPOS_METHODDEF    \
    {"defclass_varpos", _PyCFunction_CAST(_testclinic_TestClass_defclass_varpos), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _testclinic_TestClass_defclass_varpos__doc__},

static PyObject *
_testclinic_TestClass_defclass_varpos_impl(PyObject *self, PyTypeObject *cls,
                                           PyObject *args);

static PyValue
_testclinic_TestClass_defclass_varpos(PyObject *self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *__clinic_args = NULL;
    PyObject *_return_value;

    if (!_PyArg_NoKwnames("defclass_varpos", kwnames)) {
        goto exit;
    }
    __clinic_args = _PyTuple_FromValues(args, nargs);
    if (__clinic_args == NULL) {
        goto exit;
    }
    _return_value = _testclinic_TestClass_defclass_varpos_impl(self, cls, __clinic_args);
    return_value = PyValue_FromResult(_return_value);

exit:
    /* Cleanup for args */
    Py_XDECREF(__clinic_args);

    return return_value;
}

PyDoc_STRVAR(_testclinic_TestClass_defclass_posonly_varpos__doc__,
"defclass_posonly_varpos($self, a, b, /, *args)\n"
"--\n"
"\n");

#define _TESTCLINIC_TESTCLASS_DEFCLASS_POSONLY_VARPOS_METHODDEF    \
    {"defclass_posonly_varpos", _PyCFunction_CAST(_testclinic_TestClass_defclass_posonly_varpos), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _testclinic_TestClass_defclass_posonly_varpos__doc__},

static PyObject *
_testclinic_TestClass_defclass_posonly_varpos_impl(PyObject *self,
                                                   PyTypeObject *cls,
                                                   PyObject *a, PyObject *b,
                                                   PyObject *args);

static PyValue
_testclinic_TestClass_defclass_posonly_varpos(PyObject *self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *a;
    PyObject *b;
    PyObject *__clinic_args = NULL;
    PyObject *_return_value;

    if (!_PyArg_NoKwnames("defclass_posonly_varpos", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("defclass_posonly_varpos", nargs, 2, PY_SSIZE_T_MAX)) {
        goto exit;
    }
    a = PyValue_Object(args[0]);
    if (a == NULL) {
        goto exit;
    }
    b = PyValue_Object(args[1]);
    if (b == NULL) {
        goto exit;
    }
    __clinic_args = _PyTuple_FromValues(args + 2, nargs - 2);
    if (__clinic_args == NULL) {
        goto exit;
    }
    _return_value = _testclinic_TestClass_defclass_posonly_varpos_impl(self, cls, a, b, __clinic_args);
    return_value = PyValue_FromResult(_return_value);

exit:
    /* Cleanup for args */
    Py_XDECREF(__clinic_args);

    return return_value;
}

PyDoc_STRVAR(_testclinic_TestClass_varpos_no_fastcall__doc__,
"varpos_no_fastcall($type, /, *args)\n"
"--\n"
"\n");

#define _TESTCLINIC_TESTCLASS_VARPOS_NO_FASTCALL_METHODDEF    \
    {"varpos_no_fastcall", (PyCFunction)_testclinic_TestClass_varpos_no_fastcall, METH_VARARGS|METH_CLASS, _testclinic_TestClass_varpos_no_fastcall__doc__},

static PyObject *
_testclinic_TestClass_varpos_no_fastcall_impl(PyTypeObject *type,
                                              PyObject *args);

static PyObject *
_testclinic_TestClass_varpos_no_fastcall(PyObject *type, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *__clinic_args = NULL;

    __clinic_args = Py_NewRef(args);
    return_value = _testclinic_TestClass_varpos_no_fastcall_impl((PyTypeObject *)type, __clinic_args);

    /* Cleanup for args */
    Py_XDECREF(__clinic_args);

    return return_value;
}

PyDoc_STRVAR(_testclinic_TestClass_posonly_varpos_no_fastcall__doc__,
"posonly_varpos_no_fastcall($type, a, b, /, *args)\n"
"--\n"
"\n");

#define _TESTCLINIC_TESTCLASS_POSONLY_VARPOS_NO_FASTCALL_METHODDEF    \
    {"posonly_varpos_no_fastcall", (PyCFunction)_testclinic_TestClass_posonly_varpos_no_fastcall, METH_VARARGS|METH_CLASS, _testclinic_TestClass_posonly_varpos_no_fastcall__doc__},

static PyObject *
_testclinic_TestClass_posonly_varpos_no_fastcall_impl(PyTypeObject *type,
                                                      PyObject *a,
                                                      PyObject *b,
                                                      PyObject *args);

static PyObject *
_testclinic_TestClass_posonly_varpos_no_fastcall(PyObject *type, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *a;
    PyObject *b;
    PyObject *__clinic_args = NULL;

    if (!_PyArg_CheckPositional("posonly_varpos_no_fastcall", PyTuple_GET_SIZE(args), 2, PY_SSIZE_T_MAX)) {
        goto exit;
    }
    a = PyValue_Object(PyTuple_GET_VALUE(args, 0));
    if (a == NULL) {
        goto exit;
    }
    b = PyValue_Object(PyTuple_GET_VALUE(args, 1));
    if (b == NULL) {
        goto exit;
    }
    __clinic_args = PyTuple_GetSlice(args, 2, PY_SSIZE_T_MAX);
    if (!__clinic_args) {
        goto exit;
    }
    return_value = _testclinic_TestClass_posonly_varpos_no_fastcall_impl((PyTypeObject *)type, a, b, __clinic_args);

exit:
    /* Cleanup for args */
    Py_XDECREF(__clinic_args);

    return return_value;
}

PyDoc_STRVAR(_testclinic_TestClass_posonly_req_opt_varpos_no_fastcall__doc__,
"posonly_req_opt_varpos_no_fastcall($type, a, b=False, /, *args)\n"
"--\n"
"\n");

#define _TESTCLINIC_TESTCLASS_POSONLY_REQ_OPT_VARPOS_NO_FASTCALL_METHODDEF    \
    {"posonly_req_opt_varpos_no_fastcall", (PyCFunction)_testclinic_TestClass_posonly_req_opt_varpos_no_fastcall, METH_VARARGS|METH_CLASS, _testclinic_TestClass_posonly_req_opt_varpos_no_fastcall__doc__},

static PyObject *
_testclinic_TestClass_posonly_req_opt_varpos_no_fastcall_impl(PyTypeObject *type,
                                                              PyObject *a,
                                                              PyObject *b,
                                                              PyObject *args);

static PyObject *
_testclinic_TestClass_posonly_req_opt_varpos_no_fastcall(PyObject *type, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *a;
    PyObject *b = Py_False;
    PyObject *__clinic_args = NULL;

    if (!_PyArg_CheckPositional("posonly_req_opt_varpos_no_fastcall", PyTuple_GET_SIZE(args), 1, PY_SSIZE_T_MAX)) {
        goto exit;
    }
    a = PyValue_Object(PyTuple_GET_VALUE(args, 0));
    if (a == NULL) {
        goto exit;
    }
    if (PyTuple_GET_SIZE(args) < 2) {
        goto skip_optional;
    }
    b = PyValue_Object(PyTuple_GET_VALUE(args, 1));
    if (b == NULL) {
        goto exit;
    }
skip_optional:
    __clinic_args = PyTuple_GetSlice(args, 2, PY_SSIZE_T_MAX);
    if (!__clinic_args) {
        goto exit;
    }
    return_value = _testclinic_TestClass_posonly_req_opt_varpos_no_fastcall_impl((PyTypeObject *)type, a, b, __clinic_args);

exit:
    /* Cleanup for args */
    Py_XDECREF(__clinic_args);

    return return_value;
}

PyDoc_STRVAR(_testclinic_TestClass_posonly_poskw_varpos_no_fastcall__doc__,
"posonly_poskw_varpos_no_fastcall($type, a, /, b, *args)\n"
"--\n"
"\n");

#define _TESTCLINIC_TESTCLASS_POSONLY_POSKW_VARPOS_NO_FASTCALL_METHODDEF    \
    {"posonly_poskw_varpos_no_fastcall", _PyCFunction_CAST(_testclinic_TestClass_posonly_poskw_varpos_no_fastcall), METH_VARARGS|METH_KEYWORDS|METH_CLASS, _testclinic_TestClass_posonly_poskw_varpos_no_fastcall__doc__},

static PyObject *
_testclinic_TestClass_posonly_poskw_varpos_no_fastcall_impl(PyTypeObject *type,
                                                            PyObject *a,
                                                            PyObject *b,
                                                            PyObject *args);

static PyObject *
_testclinic_TestClass_posonly_poskw_varpos_no_fastcall(PyObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _Py_LATIN1_CHR('b'), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "b", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "posonly_poskw_varpos_no_fastcall",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyValue argsbuf[2];
    const PyValue *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    PyObject *a;
    PyObject *b;
    PyObject *__clinic_args = NULL;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_ITEMS(args), nargs, kwargs, NULL, &_parser,
            /*minpos*/ 2, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 1, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    a = PyValue_Object(fastargs[0]);
    if (a == NULL) {
        goto exit;
    }
    b = PyValue_Object(fastargs[1]);
    if (b == NULL) {
        goto exit;
    }
    __clinic_args = PyTuple_GetSlice(args, 2, PY_SSIZE_T_MAX);
    if (!__clinic_args) {
        goto exit;
    }
    return_value = _testclinic_TestClass_posonly_poskw_varpos_no_fastcall_impl((PyTypeObject *)type, a, b, __clinic_args);

exit:
    /* Cleanup for args */
    Py_XDECREF(__clinic_args);

    return return_value;
}

PyDoc_STRVAR(_testclinic_TestClass_varpos_array_no_fastcall__doc__,
"varpos_array_no_fastcall($type, /, *args)\n"
"--\n"
"\n");

#define _TESTCLINIC_TESTCLASS_VARPOS_ARRAY_NO_FASTCALL_METHODDEF    \
    {"varpos_array_no_fastcall", (PyCFunction)_testclinic_TestClass_varpos_array_no_fastcall, METH_VARARGS|METH_CLASS, _testclinic_TestClass_varpos_array_no_fastcall__doc__},

static PyObject *
_testclinic_TestClass_varpos_array_no_fastcall_impl(PyTypeObject *type,
                                                    const PyValue *args,
                                                    Py_ssize_t args_length);

static PyObject *
_testclinic_TestClass_varpos_array_no_fastcall(PyObject *type, PyObject *args)
{
    PyObject *return_value = NULL;
    const PyValue *__clinic_args;
    Py_ssize_t args_length;

    __clinic_args = _PyTuple_ITEMS(args);
    args_length = PyTuple_GET_SIZE(args);
    return_value = _testclinic_TestClass_varpos_array_no_fastcall_impl((PyTypeObject *)type, __clinic_args, args_length);

    return return_value;
}

PyDoc_STRVAR(_testclinic_TestClass_posonly_varpos_array_no_fastcall__doc__,
"posonly_varpos_array_no_fastcall($type, a, b, /, *args)\n"
"--\n"
"\n");

#define _TESTCLINIC_TESTCLASS_POSONLY_VARPOS_ARRAY_NO_FASTCALL_METHODDEF    \
    {"posonly_varpos_array_no_fastcall", (PyCFunction)_testclinic_TestClass_posonly_varpos_array_no_fastcall, METH_VARARGS|METH_CLASS, _testclinic_TestClass_posonly_varpos_array_no_fastcall__doc__},

static PyObject *
_testclinic_TestClass_posonly_varpos_array_no_fastcall_impl(PyTypeObject *type,
                                                            PyObject *a,
                                                            PyObject *b,
                                                            const PyValue *args,
                                                            Py_ssize_t args_length);

static PyObject *
_testclinic_TestClass_posonly_varpos_array_no_fastcall(PyObject *type, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *a;
    PyObject *b;
    const PyValue *__clinic_args;
    Py_ssize_t args_length;

    if (!_PyArg_CheckPositional("posonly_varpos_array_no_fastcall", PyTuple_GET_SIZE(args), 2, PY_SSIZE_T_MAX)) {
        goto exit;
    }
    a = PyValue_Object(PyTuple_GET_VALUE(args, 0));
    if (a == NULL) {
        goto exit;
    }
    b = PyValue_Object(PyTuple_GET_VALUE(args, 1));
    if (b == NULL) {
        goto exit;
    }
    __clinic_args = _PyTuple_ITEMS(args) + 2;
    args_length = PyTuple_GET_SIZE(args) - 2;
    return_value = _testclinic_TestClass_posonly_varpos_array_no_fastcall_impl((PyTypeObject *)type, a, b, __clinic_args, args_length);

exit:
    return return_value;
}

PyDoc_STRVAR(_testclinic_TestClass_posonly_req_opt_varpos_array_no_fastcall__doc__,
"posonly_req_opt_varpos_array_no_fastcall($type, a, b=False, /, *args)\n"
"--\n"
"\n");

#define _TESTCLINIC_TESTCLASS_POSONLY_REQ_OPT_VARPOS_ARRAY_NO_FASTCALL_METHODDEF    \
    {"posonly_req_opt_varpos_array_no_fastcall", (PyCFunction)_testclinic_TestClass_posonly_req_opt_varpos_array_no_fastcall, METH_VARARGS|METH_CLASS, _testclinic_TestClass_posonly_req_opt_varpos_array_no_fastcall__doc__},

static PyObject *
_testclinic_TestClass_posonly_req_opt_varpos_array_no_fastcall_impl(PyTypeObject *type,
                                                                    PyObject *a,
                                                                    PyObject *b,
                                                                    const PyValue *args,
                                                                    Py_ssize_t args_length);

static PyObject *
_testclinic_TestClass_posonly_req_opt_varpos_array_no_fastcall(PyObject *type, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *a;
    PyObject *b = Py_False;
    const PyValue *__clinic_args;
    Py_ssize_t args_length;

    if (!_PyArg_CheckPositional("posonly_req_opt_varpos_array_no_fastcall", PyTuple_GET_SIZE(args), 1, PY_SSIZE_T_MAX)) {
        goto exit;
    }
    a = PyValue_Object(PyTuple_GET_VALUE(args, 0));
    if (a == NULL) {
        goto exit;
    }
    if (PyTuple_GET_SIZE(args) < 2) {
        goto skip_optional;
    }
    b = PyValue_Object(PyTuple_GET_VALUE(args, 1));
    if (b == NULL) {
        goto exit;
    }
skip_optional:
    __clinic_args = PyTuple_GET_SIZE(args) > 2 ? _PyTuple_ITEMS(args) + 2 : _PyTuple_ITEMS(args);
    args_length = Py_MAX(0, PyTuple_GET_SIZE(args) - 2);
    return_value = _testclinic_TestClass_posonly_req_opt_varpos_array_no_fastcall_impl((PyTypeObject *)type, a, b, __clinic_args, args_length);

exit:
    return return_value;
}

PyDoc_STRVAR(_testclinic_TestClass_posonly_poskw_varpos_array_no_fastcall__doc__,
"posonly_poskw_varpos_array_no_fastcall($type, a, /, b, *args)\n"
"--\n"
"\n");

#define _TESTCLINIC_TESTCLASS_POSONLY_POSKW_VARPOS_ARRAY_NO_FASTCALL_METHODDEF    \
    {"posonly_poskw_varpos_array_no_fastcall", _PyCFunction_CAST(_testclinic_TestClass_posonly_poskw_varpos_array_no_fastcall), METH_VARARGS|METH_KEYWORDS|METH_CLASS, _testclinic_TestClass_posonly_poskw_varpos_array_no_fastcall__doc__},

static PyObject *
_testclinic_TestClass_posonly_poskw_varpos_array_no_fastcall_impl(PyTypeObject *type,
                                                                  PyObject *a,
                                                                  PyObject *b,
                                                                  const PyValue *args,
                                                                  Py_ssize_t args_length);

static PyObject *
_testclinic_TestClass_posonly_poskw_varpos_array_no_fastcall(PyObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _Py_LATIN1_CHR('b'), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "b", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "posonly_poskw_varpos_array_no_fastcall",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyValue argsbuf[2];
    const PyValue *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    PyObject *a;
    PyObject *b;
    const PyValue *__clinic_args;
    Py_ssize_t args_length;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_ITEMS(args), nargs, kwargs, NULL, &_parser,
            /*minpos*/ 2, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 1, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    a = PyValue_Object(fastargs[0]);
    if (a == NULL) {
        goto exit;
    }
    b = PyValue_Object(fastargs[1]);
    if (b == NULL) {
        goto exit;
    }
    __clinic_args = PyTuple_GET_SIZE(args) > 2 ? _PyTuple_ITEMS(args) + 2 : _PyTuple_ITEMS(args);
    args_length = Py_MAX(0, PyTuple_GET_SIZE(args) - 2);
    return_value = _testclinic_TestClass_posonly_poskw_varpos_array_no_fastcall_impl((PyTypeObject *)type, a, b, __clinic_args, args_length);

exit:
    return return_value;
}

static PyObject *
vc_plain_new_impl(PyTypeObject *type, PyObject *a);

static PyObject *
vc_plain_new_helper(PyTypeObject *type, const PyValue *args,
    Py_ssize_t nargs, Py_ssize_t nkw, PyObject *kwargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _Py_LATIN1_CHR('a'), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"a", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "VcNew",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyValue argsbuf[1];
    const PyValue *fastargs;
    Py_ssize_t noptargs = nargs + nkw - 0;
    PyObject *a = Py_None;

    fastargs = _PyArg_UnpackKeywords(args, nargs, kwargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    a = PyValue_Object(fastargs[0]);
    if (a == NULL) {
        goto exit;
    }
skip_optional_pos:
    return_value = vc_plain_new_impl(type, a);

exit:
    return return_value;
}

static PyObject *
vc_plain_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    return vc_plain_new_helper(type, _PyTuple_ITEMS(args),
        PyTuple_GET_SIZE(args),
        kwargs ? PyDict_GET_SIZE(kwargs) : 0,
        kwargs, NULL);
}

static PyValue
vc_plain_vectorcall(PyValue type_value, const PyValue *args,
    size_t nargsf, PyObject *kwnames)
{
    PyObject *type = PyValue_ObjectBorrow(type_value);
    PyObject *return_value = NULL;
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    PyObject *a = Py_None;

    assert(_PyType_CAST(type)->tp_new == vc_plain_new);
    /* Make sure the type object is immutable: the generated
     * vectorcall doesn't deal e.g. with users reassigning __init__. */
    assert(PyType_HasFeature(_PyType_CAST(type), Py_TPFLAGS_IMMUTABLETYPE));
    if (kwnames != NULL || nargs > 1) {
        return PyValue_FromResult(vc_plain_new_helper(_PyType_CAST(type),
            args, nargs,
            kwnames ? PyTuple_GET_SIZE(kwnames) : 0,
            NULL, kwnames));
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    a = PyValue_Object(args[0]);
    if (a == NULL) {
        goto exit;
    }
skip_optional:
    return_value = vc_plain_new_impl(_PyType_CAST(type), a);

exit:
    return PyValue_FromResult(return_value);
}

static int
vc_posorkw_init_impl(PyObject *self, PyObject *a, PyObject *b);

static int
vc_posorkw_init_helper(PyObject *self, const PyValue *args,
    Py_ssize_t nargs, Py_ssize_t nkw, PyObject *kwargs, PyObject *kwnames)
{
    int return_value = -1;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _Py_LATIN1_CHR('b'), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "b", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "VcInit",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyValue argsbuf[2];
    const PyValue *fastargs;
    Py_ssize_t noptargs = nargs + nkw - 1;
    PyObject *a;
    PyObject *b = Py_None;

    fastargs = _PyArg_UnpackKeywords(args, nargs, kwargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    a = PyValue_Object(fastargs[0]);
    if (a == NULL) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    b = PyValue_Object(fastargs[1]);
    if (b == NULL) {
        goto exit;
    }
skip_optional_pos:
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = vc_posorkw_init_impl(self, a, b);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

static int
vc_posorkw_init(PyObject *self, PyObject *args, PyObject *kwargs)
{
    return vc_posorkw_init_helper(self, _PyTuple_ITEMS(args),
        PyTuple_GET_SIZE(args),
        kwargs ? PyDict_GET_SIZE(kwargs) : 0,
        kwargs, NULL);
}

static PyValue
vc_posorkw_vectorcall(PyValue type_value, const PyValue *args,
    size_t nargsf, PyObject *kwnames)
{
    PyObject *type = PyValue_ObjectBorrow(type_value);
    PyObject *return_value = NULL;
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    PyObject *self;
    int _result;
    PyObject *a;
    PyObject *b = Py_None;

    assert(_PyType_CAST(type)->tp_init == vc_posorkw_init);
    /* Make sure the type object is immutable: the generated
     * vectorcall doesn't deal e.g. with users reassigning __init__. */
    assert(PyType_HasFeature(_PyType_CAST(type), Py_TPFLAGS_IMMUTABLETYPE));
    if (kwnames != NULL || nargs < 1 || nargs > 2) {
        self = _PyType_CAST(type)->tp_new(_PyType_CAST(type),
            (PyObject *)&_Py_SINGLETON(tuple_empty), NULL);
        if (self == NULL) {
            return PyValue_NULL;
        }
        _result = vc_posorkw_init_helper(self, args, nargs,
            kwnames ? PyTuple_GET_SIZE(kwnames) : 0,
            NULL, kwnames);
        if (_result != 0) {
            Py_DECREF(self);
            return PyValue_NULL;
        }
        return PyValue_FromObject(self);
    }
    a = PyValue_Object(args[0]);
    if (a == NULL) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    b = PyValue_Object(args[1]);
    if (b == NULL) {
        goto exit;
    }
skip_optional:
    self = _PyType_CAST(type)->tp_new(_PyType_CAST(type),
        (PyObject *)&_Py_SINGLETON(tuple_empty), NULL);
    if (self == NULL) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    _result = vc_posorkw_init_impl((PyObject *)self, a, b);
    Py_END_CRITICAL_SECTION();
    if (_result != 0) {
        Py_DECREF(self);
        goto exit;
    }
    return_value = self;

exit:
    return PyValue_FromResult(return_value);
}

static PyObject *
vc_base_new_impl(PyTypeObject *type, PyObject *a, PyObject *b);

static PyObject *
vc_base_new_helper(PyTypeObject *type, const PyValue *args,
    Py_ssize_t nargs, Py_ssize_t nkw, PyObject *kwargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _Py_LATIN1_CHR('b'), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "b", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "VcNewBase",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyValue argsbuf[2];
    const PyValue *fastargs;
    Py_ssize_t noptargs = nargs + nkw - 1;
    PyObject *a;
    PyObject *b = Py_None;

    fastargs = _PyArg_UnpackKeywords(args, nargs, kwargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    a = PyValue_Object(fastargs[0]);
    if (a == NULL) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    b = PyValue_Object(fastargs[1]);
    if (b == NULL) {
        goto exit;
    }
skip_optional_pos:
    return_value = vc_base_new_impl(type, a, b);

exit:
    return return_value;
}

static PyObject *
vc_base_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    return vc_base_new_helper(type, _PyTuple_ITEMS(args),
        PyTuple_GET_SIZE(args),
        kwargs ? PyDict_GET_SIZE(kwargs) : 0,
        kwargs, NULL);
}

static PyValue
vc_base_vectorcall(PyValue type_value, const PyValue *args,
    size_t nargsf, PyObject *kwnames)
{
    PyObject *type = PyValue_ObjectBorrow(type_value);
    PyObject *return_value = NULL;
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    PyObject *a;
    PyObject *b = Py_None;

    assert(_PyType_CAST(type)->tp_new == vc_base_new);
    /* Make sure the type object is immutable: the generated
     * vectorcall doesn't deal e.g. with users reassigning __init__. */
    assert(PyType_HasFeature(_PyType_CAST(type), Py_TPFLAGS_IMMUTABLETYPE));
    if (kwnames != NULL || nargs < 1 || nargs > 2) {
        return PyValue_FromResult(vc_base_new_helper(_PyType_CAST(type),
            args, nargs,
            kwnames ? PyTuple_GET_SIZE(kwnames) : 0,
            NULL, kwnames));
    }
    a = PyValue_Object(args[0]);
    if (a == NULL) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    b = PyValue_Object(args[1]);
    if (b == NULL) {
        goto exit;
    }
skip_optional:
    return_value = vc_base_new_impl(_PyType_CAST(type), a, b);

exit:
    return PyValue_FromResult(return_value);
}

static PyObject *
vc_kwonly_new_impl(PyTypeObject *type, PyObject *a, PyObject *b);

static PyObject *
vc_kwonly_new_helper(PyTypeObject *type, const PyValue *args,
    Py_ssize_t nargs, Py_ssize_t nkw, PyObject *kwargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _Py_LATIN1_CHR('a'), _Py_LATIN1_CHR('b'), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"a", "b", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "VcKwOnly",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyValue argsbuf[2];
    const PyValue *fastargs;
    Py_ssize_t noptargs = nargs + nkw - 1;
    PyObject *a;
    PyObject *b = Py_None;

    fastargs = _PyArg_UnpackKeywords(args, nargs, kwargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    a = PyValue_Object(fastargs[0]);
    if (a == NULL) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    b = PyValue_Object(fastargs[1]);
    if (b == NULL) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = vc_kwonly_new_impl(type, a, b);

exit:
    return return_value;
}

static PyObject *
vc_kwonly_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    return vc_kwonly_new_helper(type, _PyTuple_ITEMS(args),
        PyTuple_GET_SIZE(args),
        kwargs ? PyDict_GET_SIZE(kwargs) : 0,
        kwargs, NULL);
}

static PyValue
vc_kwonly_vectorcall(PyValue type_value, const PyValue *args,
    size_t nargsf, PyObject *kwnames)
{
    PyObject *type = PyValue_ObjectBorrow(type_value);
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);

    assert(_PyType_CAST(type)->tp_new == vc_kwonly_new);
    /* Make sure the type object is immutable: the generated
     * vectorcall doesn't deal e.g. with users reassigning __init__. */
    assert(PyType_HasFeature(_PyType_CAST(type), Py_TPFLAGS_IMMUTABLETYPE));
    return PyValue_FromResult(vc_kwonly_new_helper(_PyType_CAST(type),
        args, nargs,
        kwnames ? PyTuple_GET_SIZE(kwnames) : 0,
        NULL, kwnames));
}
/*[clinic end generated code: output=5546b54075c25b89 input=a9049054013a1b77]*/
