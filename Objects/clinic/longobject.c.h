/*[clinic input]
preserve
[clinic start generated code]*/

#ifndef Py_VALUE_CORE
#  error "value-emission Clinic output requires Py_VALUE_CORE"
#endif

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#  include "pycore_value.h"       // _PyValue_STATIC_IMMORTAL()
#endif
#include "pycore_long_value.h"    // _PyValue_IntAsSsize_t()
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywordsValue()
#include "pycore_tuple.h"         // _PyTuple_ITEMS()
#include "pycore_value_call.h"    // METH_VALUE
#include "pycore_value_ops.h"     // _PyValue_Index()
#include "pycore_value_slots.h"   // _PyValue_THUNK_EXIT()

static PyValue
long_new_impl(PyTypeObject *type, PyValueBorrow x, PyValueBorrow obase);

static PyValue
long_new_value(PyTypeObject *type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(base)), },
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
    Py_ssize_t _idx[2];
    PyValueBorrow x = PyValue_Borrow(PyValue_NULL);
    PyValueBorrow obase = PyValue_Borrow(PyValue_NULL);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        x = PyValue_Borrow(args[_idx[0]]);
    }
    if (_idx[1] >= 0) {
        obase = PyValue_Borrow(args[_idx[1]]);
    }
    return_value = long_new_impl(type, x, obase);

exit:
    return return_value;
}

Py_VALUE_ENTRY static PyObject *
long_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _PyValue_THUNK_EXIT(long_new_value(type, _PyTuple_ITEMS(args), nargs, NULL), BOX_SITE_CALL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return NULL;
    }
    PyValue result = long_new_value(type, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return _PyValue_THUNK_EXIT(result, BOX_SITE_CALL);
}

PyDoc_STRVAR(int___getnewargs____doc__,
"__getnewargs__($self, /)\n"
"--\n"
"\n");

#define INT___GETNEWARGS___METHODDEF    \
    {"__getnewargs__", _PyCFunction_CAST(int___getnewargs__), METH_VALUE|METH_NOARGS, int___getnewargs____doc__},

static PyValue
int___getnewargs___impl(PyValueBorrow self);

static PyValue
int___getnewargs__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = int___getnewargs___impl(self);

    return return_value;
}

PyDoc_STRVAR(int___format____doc__,
"__format__($self, format_spec, /)\n"
"--\n"
"\n"
"Convert to a string according to format_spec.");

#define INT___FORMAT___METHODDEF    \
    {"__format__", _PyCFunction_CAST(int___format__), METH_VALUE|METH_O, int___format____doc__},

static PyValue
int___format___impl(PyValueBorrow self, PyObject *format_spec);

static PyValue
int___format__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *format_spec;

    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("__format__", "argument", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("__format__", "argument", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    format_spec = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    return_value = int___format___impl(self, format_spec);

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
    {"__round__", _PyCFunction_CAST(int___round__), METH_VALUE, int___round____doc__},

static PyValue
int___round___impl(PyValueBorrow self, PyValueBorrow o_ndigits);

static PyValue
int___round__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow o_ndigits = PyValue_Borrow(PyValue_None);

    if (!_PyArg_CheckPositional("__round__", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    o_ndigits = PyValue_Borrow(args[0]);
skip_optional:
    return_value = int___round___impl(self, o_ndigits);

exit:
    return return_value;
}

PyDoc_STRVAR(int___sizeof____doc__,
"__sizeof__($self, /)\n"
"--\n"
"\n"
"Returns size in memory, in bytes.");

#define INT___SIZEOF___METHODDEF    \
    {"__sizeof__", _PyCFunction_CAST(int___sizeof__), METH_VALUE|METH_NOARGS, int___sizeof____doc__},

static Py_ssize_t
int___sizeof___impl(PyValueBorrow self);

static PyValue
int___sizeof__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_ssize_t _return_value;

    _return_value = int___sizeof___impl(self);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyValue_FromInt64((int64_t)_return_value);

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
    {"bit_length", _PyCFunction_CAST(int_bit_length), METH_VALUE|METH_NOARGS, int_bit_length__doc__},

static PyValue
int_bit_length_impl(PyValueBorrow self);

static PyValue
int_bit_length(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = int_bit_length_impl(self);

    return return_value;
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
    {"bit_count", _PyCFunction_CAST(int_bit_count), METH_VALUE|METH_NOARGS, int_bit_count__doc__},

static PyValue
int_bit_count_impl(PyValueBorrow self);

static PyValue
int_bit_count(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = int_bit_count_impl(self);

    return return_value;
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
    {"as_integer_ratio", _PyCFunction_CAST(int_as_integer_ratio), METH_VALUE|METH_NOARGS, int_as_integer_ratio__doc__},

static PyValue
int_as_integer_ratio_impl(PyValueBorrow self);

static PyValue
int_as_integer_ratio(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = int_as_integer_ratio_impl(self);

    return return_value;
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
    {"to_bytes", _PyCFunction_CAST(int_to_bytes), METH_VALUE|METH_KEYWORDS, int_to_bytes__doc__},

static PyValue
int_to_bytes_impl(PyValueBorrow self, Py_ssize_t length, PyObject *byteorder,
                  int is_signed);

static PyValue
int_to_bytes(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(length)), _PyValue_STATIC_IMMORTAL(&_Py_ID(byteorder)), _PyValue_STATIC_IMMORTAL(&_Py_ID(signed)), },
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
    Py_ssize_t _idx[3];
    Py_ssize_t length = 1;
    PyObject *byteorder = NULL;
    int is_signed = 0;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 3)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        {
            Py_ssize_t ival = -1;
            PyValue iobj = _PyValue_Index(PyValue_Borrow(args[_idx[0]]));
            if (!PyValue_IsNull(iobj)) {
                ival = _PyValue_IntAsSsize_t(PyValue_Borrow(iobj));
                PyValue_CLOSE(iobj);
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
    }
    if (_idx[1] >= 0) {
        if (!PyValue_IsObject(PyValue_Borrow(args[_idx[1]]))) {
            _PyArg_BadArgumentValue("to_bytes", "argument 'byteorder'", "str", PyValue_Borrow(args[_idx[1]]));
            goto exit;
        }
        if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])))) {
            _PyArg_BadArgument("to_bytes", "argument 'byteorder'", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])));
            goto exit;
        }
        byteorder = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]]));
    }
    if (_idx[2] >= 0) {
        is_signed = _PyValue_IsTrue(PyValue_Borrow(args[_idx[2]]));
        if (is_signed < 0) {
            goto exit;
        }
    }
    return_value = int_to_bytes_impl(self, length, byteorder, is_signed);

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
    {"from_bytes", _PyCFunction_CAST(int_from_bytes), METH_VALUE|METH_KEYWORDS|METH_CLASS, int_from_bytes__doc__},

static PyValue
int_from_bytes_impl(PyTypeObject *type, PyValueBorrow bytes_obj,
                    PyObject *byteorder, int is_signed);

static PyValue
int_from_bytes(PyValueBorrow type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(bytes)), _PyValue_STATIC_IMMORTAL(&_Py_ID(byteorder)), _PyValue_STATIC_IMMORTAL(&_Py_ID(signed)), },
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
    Py_ssize_t _idx[3];
    PyValueBorrow bytes_obj;
    PyObject *byteorder = NULL;
    int is_signed = 0;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 3)) {
        goto exit;
    }
    bytes_obj = PyValue_Borrow(args[_idx[0]]);
    if (_idx[1] >= 0) {
        if (!PyValue_IsObject(PyValue_Borrow(args[_idx[1]]))) {
            _PyArg_BadArgumentValue("from_bytes", "argument 'byteorder'", "str", PyValue_Borrow(args[_idx[1]]));
            goto exit;
        }
        if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])))) {
            _PyArg_BadArgument("from_bytes", "argument 'byteorder'", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])));
            goto exit;
        }
        byteorder = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]]));
    }
    if (_idx[2] >= 0) {
        is_signed = _PyValue_IsTrue(PyValue_Borrow(args[_idx[2]]));
        if (is_signed < 0) {
            goto exit;
        }
    }
    return_value = int_from_bytes_impl((PyTypeObject *)PyValue_AsKnownObjectBorrow(type), bytes_obj, byteorder, is_signed);

exit:
    return return_value;
}

PyDoc_STRVAR(int_is_integer__doc__,
"is_integer($self, /)\n"
"--\n"
"\n"
"Returns True. Exists for duck type compatibility with float.is_integer.");

#define INT_IS_INTEGER_METHODDEF    \
    {"is_integer", _PyCFunction_CAST(int_is_integer), METH_VALUE|METH_NOARGS, int_is_integer__doc__},

static PyValue
int_is_integer_impl(PyValueBorrow self);

static PyValue
int_is_integer(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = int_is_integer_impl(self);

    return return_value;
}
/*[clinic end generated code: output=6a15e07f546bcab6 input=a9049054013a1b77]*/
