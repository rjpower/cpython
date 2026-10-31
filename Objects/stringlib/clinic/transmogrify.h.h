/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()

PyDoc_STRVAR(stringlib_expandtabs__doc__,
"expandtabs($self, /, tabsize=8)\n"
"--\n"
"\n"
"Return a copy where all tab characters are expanded using spaces.\n"
"\n"
"If tabsize is not given, a tab size of 8 characters is assumed.");

#define STRINGLIB_EXPANDTABS_METHODDEF    \
    {"expandtabs", _PyCFunction_CAST(stringlib_expandtabs), METH_FASTCALL|METH_KEYWORDS, stringlib_expandtabs__doc__},

static PyObject *
stringlib_expandtabs_impl(PyObject *self, int tabsize);

static PyValue
stringlib_expandtabs(PyObject *self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(tabsize), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"tabsize", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "expandtabs",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyValue argsbuf[1];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    int tabsize = 8;
    PyObject *_return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    tabsize = PyLong_AsIntValue(args[0]);
    if (tabsize == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional_pos:
    _return_value = stringlib_expandtabs_impl(self, tabsize);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(stringlib_ljust__doc__,
"ljust($self, width, fillchar=b\' \', /)\n"
"--\n"
"\n"
"Return a left-justified string of length width.\n"
"\n"
"Padding is done using the specified fill character.");

#define STRINGLIB_LJUST_METHODDEF    \
    {"ljust", _PyCFunction_CAST(stringlib_ljust), METH_FASTCALL, stringlib_ljust__doc__},

static PyObject *
stringlib_ljust_impl(PyObject *self, Py_ssize_t width, char fillchar);

static PyValue
stringlib_ljust(PyObject *self, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    Py_ssize_t width;
    char fillchar = ' ';
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("ljust", nargs, 1, 2)) {
        goto exit;
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
        width = ival;
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
                    "ljust(): argument 2 must be a byte string of length 1, "
                    "not a bytes object of length %zd",
                    PyBytes_GET_SIZE(_arg));
                goto exit;
            }
            fillchar = PyBytes_AS_STRING(_arg)[0];
        }
        else if (PyByteArray_Check(_arg)) {
            if (PyByteArray_GET_SIZE(_arg) != 1) {
                PyErr_Format(PyExc_TypeError,
                    "ljust(): argument 2 must be a byte string of length 1, "
                    "not a bytearray object of length %zd",
                    PyByteArray_GET_SIZE(_arg));
                goto exit;
            }
            fillchar = PyByteArray_AS_STRING(_arg)[0];
        }
        else {
            _PyArg_BadArgument("ljust", "argument 2", "a byte string of length 1", _arg);
            goto exit;
        }
    }
skip_optional:
    _return_value = stringlib_ljust_impl(self, width, fillchar);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(stringlib_rjust__doc__,
"rjust($self, width, fillchar=b\' \', /)\n"
"--\n"
"\n"
"Return a right-justified string of length width.\n"
"\n"
"Padding is done using the specified fill character.");

#define STRINGLIB_RJUST_METHODDEF    \
    {"rjust", _PyCFunction_CAST(stringlib_rjust), METH_FASTCALL, stringlib_rjust__doc__},

static PyObject *
stringlib_rjust_impl(PyObject *self, Py_ssize_t width, char fillchar);

static PyValue
stringlib_rjust(PyObject *self, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    Py_ssize_t width;
    char fillchar = ' ';
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("rjust", nargs, 1, 2)) {
        goto exit;
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
        width = ival;
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
                    "rjust(): argument 2 must be a byte string of length 1, "
                    "not a bytes object of length %zd",
                    PyBytes_GET_SIZE(_arg));
                goto exit;
            }
            fillchar = PyBytes_AS_STRING(_arg)[0];
        }
        else if (PyByteArray_Check(_arg)) {
            if (PyByteArray_GET_SIZE(_arg) != 1) {
                PyErr_Format(PyExc_TypeError,
                    "rjust(): argument 2 must be a byte string of length 1, "
                    "not a bytearray object of length %zd",
                    PyByteArray_GET_SIZE(_arg));
                goto exit;
            }
            fillchar = PyByteArray_AS_STRING(_arg)[0];
        }
        else {
            _PyArg_BadArgument("rjust", "argument 2", "a byte string of length 1", _arg);
            goto exit;
        }
    }
skip_optional:
    _return_value = stringlib_rjust_impl(self, width, fillchar);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(stringlib_center__doc__,
"center($self, width, fillchar=b\' \', /)\n"
"--\n"
"\n"
"Return a centered string of length width.\n"
"\n"
"Padding is done using the specified fill character.");

#define STRINGLIB_CENTER_METHODDEF    \
    {"center", _PyCFunction_CAST(stringlib_center), METH_FASTCALL, stringlib_center__doc__},

static PyObject *
stringlib_center_impl(PyObject *self, Py_ssize_t width, char fillchar);

static PyValue
stringlib_center(PyObject *self, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    Py_ssize_t width;
    char fillchar = ' ';
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("center", nargs, 1, 2)) {
        goto exit;
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
        width = ival;
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
                    "center(): argument 2 must be a byte string of length 1, "
                    "not a bytes object of length %zd",
                    PyBytes_GET_SIZE(_arg));
                goto exit;
            }
            fillchar = PyBytes_AS_STRING(_arg)[0];
        }
        else if (PyByteArray_Check(_arg)) {
            if (PyByteArray_GET_SIZE(_arg) != 1) {
                PyErr_Format(PyExc_TypeError,
                    "center(): argument 2 must be a byte string of length 1, "
                    "not a bytearray object of length %zd",
                    PyByteArray_GET_SIZE(_arg));
                goto exit;
            }
            fillchar = PyByteArray_AS_STRING(_arg)[0];
        }
        else {
            _PyArg_BadArgument("center", "argument 2", "a byte string of length 1", _arg);
            goto exit;
        }
    }
skip_optional:
    _return_value = stringlib_center_impl(self, width, fillchar);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(stringlib_zfill__doc__,
"zfill($self, width, /)\n"
"--\n"
"\n"
"Pad a numeric string with zeros on the left, to fill a field of the given width.\n"
"\n"
"The original string is never truncated.");

#define STRINGLIB_ZFILL_METHODDEF    \
    {"zfill", _PyCFunction_CAST(stringlib_zfill), METH_O, stringlib_zfill__doc__},

static PyObject *
stringlib_zfill_impl(PyObject *self, Py_ssize_t width);

static PyValue
stringlib_zfill(PyObject *self, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    Py_ssize_t width;
    PyObject *_return_value;

    {
        Py_ssize_t ival = -1;
        PyValue ivalue = PyNumber_IndexValue(arg);
        if (!PyValue_IsNull(ivalue)) {
            ival = PyLong_AsSsize_tValue(ivalue);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        width = ival;
    }
    _return_value = stringlib_zfill_impl(self, width);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}
/*[clinic end generated code: output=c5c74deae66c1061 input=a9049054013a1b77]*/
