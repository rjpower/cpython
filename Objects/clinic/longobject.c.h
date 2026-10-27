/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()
#include "pycore_tuple.h"         // _PyTuple_ITEMS()

static PyObject *
long_new_impl(PyTypeObject *type, PyObject *x, PyObject *obase);

static PyObject *
long_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
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
        .ob_item = { &_Py_ID(base), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "base", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "int",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyValue argsbuf[2];
    const PyValue *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 0;
    PyObject *x = NULL;
    PyObject *obase = NULL;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_ITEMS(args), nargs, kwargs, NULL, &_parser,
            /*minpos*/ 0, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional_posonly;
    }
    noptargs--;
    x = PyValue_Object(fastargs[0]);
    if (x == NULL) {
        goto exit;
    }
skip_optional_posonly:
    if (!noptargs) {
        goto skip_optional_pos;
    }
    obase = PyValue_Object(fastargs[1]);
    if (obase == NULL) {
        goto exit;
    }
skip_optional_pos:
    return_value = long_new_impl(type, x, obase);

exit:
    return return_value;
}

PyDoc_STRVAR(int___getnewargs____doc__,
"__getnewargs__($self, /)\n"
"--\n"
"\n");

#define INT___GETNEWARGS___METHODDEF    \
    {"__getnewargs__", (PyCFunction)int___getnewargs__, METH_NOARGS, int___getnewargs____doc__},

static PyObject *
int___getnewargs___impl(PyObject *self);

static PyObject *
int___getnewargs__(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return int___getnewargs___impl(self);
}

PyDoc_STRVAR(int___format____doc__,
"__format__($self, format_spec, /)\n"
"--\n"
"\n"
"Convert to a string according to format_spec.");

#define INT___FORMAT___METHODDEF    \
    {"__format__", _PyCFunction_CAST(int___format__), METH_O, int___format____doc__},

static PyObject *
int___format___impl(PyObject *self, PyObject *format_spec);

static PyValue
int___format__(PyObject *self, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyObject *format_spec;
    PyObject *_return_value;

    {
        PyObject *_arg = PyValue_Object(arg);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("__format__", "argument", "str", _arg);
            goto exit;
        }
        format_spec = _arg;
    }
    _return_value = int___format___impl(self, format_spec);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(int___round____doc__,
"__round__($self, ndigits=None, /)\n"
"--\n"
"\n"
"Rounding an Integral returns itself.\n"
"\n"
"Rounding with an ndigits argument also returns an integer.");

#define INT___ROUND___METHODDEF    \
    {"__round__", _PyCFunction_CAST(int___round__), METH_FASTCALL, int___round____doc__},

static PyObject *
int___round___impl(PyObject *self, PyObject *o_ndigits);

static PyValue
int___round__(PyObject *self, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    PyObject *o_ndigits = Py_None;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("__round__", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    o_ndigits = PyValue_Object(args[0]);
    if (o_ndigits == NULL) {
        goto exit;
    }
skip_optional:
    _return_value = int___round___impl(self, o_ndigits);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(int___sizeof____doc__,
"__sizeof__($self, /)\n"
"--\n"
"\n"
"Returns size in memory, in bytes.");

#define INT___SIZEOF___METHODDEF    \
    {"__sizeof__", (PyCFunction)int___sizeof__, METH_NOARGS, int___sizeof____doc__},

static Py_ssize_t
int___sizeof___impl(PyObject *self);

static PyObject *
int___sizeof__(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;
    Py_ssize_t _return_value;

    _return_value = int___sizeof___impl(self);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromSsize_t(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(int_bit_length__doc__,
"bit_length($self, /)\n"
"--\n"
"\n"
"Number of bits necessary to represent self in binary.\n"
"\n"
">>> bin(37)\n"
"\'0b100101\'\n"
">>> (37).bit_length()\n"
"6");

#define INT_BIT_LENGTH_METHODDEF    \
    {"bit_length", (PyCFunction)int_bit_length, METH_NOARGS, int_bit_length__doc__},

static PyObject *
int_bit_length_impl(PyObject *self);

static PyObject *
int_bit_length(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return int_bit_length_impl(self);
}

PyDoc_STRVAR(int_bit_count__doc__,
"bit_count($self, /)\n"
"--\n"
"\n"
"Number of ones in the binary representation of the absolute value of self.\n"
"\n"
"Also known as the population count.\n"
"\n"
">>> bin(13)\n"
"\'0b1101\'\n"
">>> (13).bit_count()\n"
"3");

#define INT_BIT_COUNT_METHODDEF    \
    {"bit_count", (PyCFunction)int_bit_count, METH_NOARGS, int_bit_count__doc__},

static PyObject *
int_bit_count_impl(PyObject *self);

static PyObject *
int_bit_count(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return int_bit_count_impl(self);
}

PyDoc_STRVAR(int_as_integer_ratio__doc__,
"as_integer_ratio($self, /)\n"
"--\n"
"\n"
"Return a pair of integers, whose ratio is equal to the original int.\n"
"\n"
"The ratio is in lowest terms and has a positive denominator.\n"
"\n"
">>> (10).as_integer_ratio()\n"
"(10, 1)\n"
">>> (-10).as_integer_ratio()\n"
"(-10, 1)\n"
">>> (0).as_integer_ratio()\n"
"(0, 1)");

#define INT_AS_INTEGER_RATIO_METHODDEF    \
    {"as_integer_ratio", (PyCFunction)int_as_integer_ratio, METH_NOARGS, int_as_integer_ratio__doc__},

static PyObject *
int_as_integer_ratio_impl(PyObject *self);

static PyObject *
int_as_integer_ratio(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return int_as_integer_ratio_impl(self);
}

PyDoc_STRVAR(int_to_bytes__doc__,
"to_bytes($self, /, length=1, byteorder=\'big\', *, signed=False)\n"
"--\n"
"\n"
"Return an array of bytes representing an integer.\n"
"\n"
"  length\n"
"    Length of bytes object to use.  An OverflowError is raised if\n"
"    the integer is not representable with the given number of bytes.\n"
"    Default is length 1.\n"
"  byteorder\n"
"    The byte order used to represent the integer.  If byteorder is\n"
"    \'big\', the most significant byte is at the beginning of the byte\n"
"    array.  If byteorder is \'little\', the most significant byte is at\n"
"    the end of the byte array.  To request the native byte order of\n"
"    the host system, use sys.byteorder as the byte order value.\n"
"    Default is to use \'big\'.\n"
"  signed\n"
"    Determines whether two\'s complement is used to represent the\n"
"    integer.  If signed is False and a negative integer is given,\n"
"    an OverflowError is raised.");

#define INT_TO_BYTES_METHODDEF    \
    {"to_bytes", _PyCFunction_CAST(int_to_bytes), METH_FASTCALL|METH_KEYWORDS, int_to_bytes__doc__},

static PyObject *
int_to_bytes_impl(PyObject *self, Py_ssize_t length, PyObject *byteorder,
                  int is_signed);

static PyValue
int_to_bytes(PyObject *self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(length), &_Py_ID(byteorder), &_Py_ID(signed), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"length", "byteorder", "signed", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "to_bytes",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyValue argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    Py_ssize_t length = 1;
    PyObject *byteorder = NULL;
    int is_signed = 0;
    PyObject *_return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (!PyValue_IsNull(args[0])) {
        {
            Py_ssize_t ival = -1;
            PyValue ivalue = PyNumber_IndexValue(args[0]);
            if (!PyValue_IsNull(ivalue)) {
                ival = PyLong_AsSsize_tValue(ivalue);
            }
            if (ival == -1 && PyErr_Occurred()) {
                goto exit;
            }
            length = ival;
                if (length < 0) {
                    PyErr_SetString(PyExc_ValueError,
                                    "length cannot be negative");
                    goto exit;
                }
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (!PyValue_IsNull(args[1])) {
        {
            PyObject *_arg = PyValue_Object(args[1]);
            if (_arg == NULL) {
                goto exit;
            }
            if (!PyUnicode_Check(_arg)) {
                _PyArg_BadArgument("to_bytes", "argument 'byteorder'", "str", _arg);
                goto exit;
            }
            byteorder = _arg;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
skip_optional_pos:
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    is_signed = PyObject_IsTrueValue(args[2]);
    if (is_signed < 0) {
        goto exit;
    }
skip_optional_kwonly:
    _return_value = int_to_bytes_impl(self, length, byteorder, is_signed);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(int_from_bytes__doc__,
"from_bytes($type, /, bytes, byteorder=\'big\', *, signed=False)\n"
"--\n"
"\n"
"Return the integer represented by the given array of bytes.\n"
"\n"
"  bytes\n"
"    Holds the array of bytes to convert.  The argument must either\n"
"    support the buffer protocol or be an iterable object producing\n"
"    bytes.  Bytes and bytearray are examples of built-in objects that\n"
"    support the buffer protocol.\n"
"  byteorder\n"
"    The byte order used to represent the integer.  If byteorder is\n"
"    \'big\', the most significant byte is at the beginning of the byte\n"
"    array.  If byteorder is \'little\', the most significant byte is at\n"
"    the end of the byte array.  To request the native byte order of\n"
"    the host system, use sys.byteorder as the byte order value.\n"
"    Default is to use \'big\'.\n"
"  signed\n"
"    Indicates whether two\'s complement is used to represent the\n"
"    integer.");

#define INT_FROM_BYTES_METHODDEF    \
    {"from_bytes", _PyCFunction_CAST(int_from_bytes), METH_FASTCALL|METH_KEYWORDS|METH_CLASS, int_from_bytes__doc__},

static PyObject *
int_from_bytes_impl(PyTypeObject *type, PyObject *bytes_obj,
                    PyObject *byteorder, int is_signed);

static PyValue
int_from_bytes(PyObject *type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(bytes), &_Py_ID(byteorder), &_Py_ID(signed), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"bytes", "byteorder", "signed", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "from_bytes",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyValue argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *bytes_obj;
    PyObject *byteorder = NULL;
    int is_signed = 0;
    PyObject *_return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    bytes_obj = PyValue_Object(args[0]);
    if (bytes_obj == NULL) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (!PyValue_IsNull(args[1])) {
        {
            PyObject *_arg = PyValue_Object(args[1]);
            if (_arg == NULL) {
                goto exit;
            }
            if (!PyUnicode_Check(_arg)) {
                _PyArg_BadArgument("from_bytes", "argument 'byteorder'", "str", _arg);
                goto exit;
            }
            byteorder = _arg;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
skip_optional_pos:
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    is_signed = PyObject_IsTrueValue(args[2]);
    if (is_signed < 0) {
        goto exit;
    }
skip_optional_kwonly:
    _return_value = int_from_bytes_impl((PyTypeObject *)type, bytes_obj, byteorder, is_signed);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(int_is_integer__doc__,
"is_integer($self, /)\n"
"--\n"
"\n"
"Returns True. Exists for duck type compatibility with float.is_integer.");

#define INT_IS_INTEGER_METHODDEF    \
    {"is_integer", (PyCFunction)int_is_integer, METH_NOARGS, int_is_integer__doc__},

static PyObject *
int_is_integer_impl(PyObject *self);

static PyObject *
int_is_integer(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return int_is_integer_impl(self);
}
/*[clinic end generated code: output=e494fc026e0999dc input=a9049054013a1b77]*/
