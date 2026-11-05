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
#include "pycore_long_value.h"    // _PyValue_AsNativeBytes()
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywordsValue()
#include "pycore_value_call.h"    // METH_VALUE
#include "pycore_value_ops.h"     // _PyValue_IsTrue()

PyDoc_STRVAR(binascii_a2b_uu__doc__,
"a2b_uu($module, data, /)\n"
"--\n"
"\n"
"Decode a line of uuencoded data.");

#define BINASCII_A2B_UU_METHODDEF    \
    {"a2b_uu", _PyCFunction_CAST(binascii_a2b_uu), METH_VALUE|METH_O, binascii_a2b_uu__doc__},

static PyValue
binascii_a2b_uu_impl(PyObject *module, Py_buffer *data);

static PyValue
binascii_a2b_uu(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_buffer data = {NULL, NULL};

    if (!ascii_buffer_converter(PyValue_Borrow(args[0]), &data)) {
        goto exit;
    }
    return_value = binascii_a2b_uu_impl(PyValue_AsKnownObjectBorrow(module), &data);

exit:
    /* Cleanup for data */
    if (data.obj)
       PyBuffer_Release(&data);

    return return_value;
}

PyDoc_STRVAR(binascii_b2a_uu__doc__,
"b2a_uu($module, data, /, *, backtick=False)\n"
"--\n"
"\n"
"Uuencode line of data.");

#define BINASCII_B2A_UU_METHODDEF    \
    {"b2a_uu", _PyCFunction_CAST(binascii_b2a_uu), METH_VALUE|METH_KEYWORDS, binascii_b2a_uu__doc__},

static PyValue
binascii_b2a_uu_impl(PyObject *module, Py_buffer *data, int backtick);

static PyValue
binascii_b2a_uu(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(backtick)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "backtick", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "b2a_uu",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    Py_buffer data = {NULL, NULL};
    int backtick = 0;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[_idx[0]]))) {
        PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[_idx[0]]))->tp_name);
        goto exit;
    }
    if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])), &data, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (_idx[1] >= 0) {
        backtick = _PyValue_IsTrue(PyValue_Borrow(args[_idx[1]]));
        if (backtick < 0) {
            goto exit;
        }
    }
    return_value = binascii_b2a_uu_impl(PyValue_AsKnownObjectBorrow(module), &data, backtick);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(binascii_a2b_base64__doc__,
"a2b_base64($module, data, /, *, strict_mode=<unrepresentable>,\n"
"           padded=True, alphabet=BASE64_ALPHABET,\n"
"           ignorechars=<unrepresentable>, canonical=False)\n"
"--\n"
"\n"
"Decode a line of base64 data.\n"
"\n"
"  strict_mode\n"
"    When set to true, bytes that are not part of the base64 standard are\n"
"    not allowed.  The same applies to excess data after padding (= / ==).\n"
"    Set to True by default if ignorechars is specified, False otherwise.\n"
"  padded\n"
"    When set to false, padding in input is not required.\n"
"  ignorechars\n"
"    A byte string containing characters to ignore from the input when\n"
"    strict_mode is true.\n"
"  canonical\n"
"    When set to true, reject non-zero padding bits per RFC 4648 section 3.5.");

#define BINASCII_A2B_BASE64_METHODDEF    \
    {"a2b_base64", _PyCFunction_CAST(binascii_a2b_base64), METH_VALUE|METH_KEYWORDS, binascii_a2b_base64__doc__},

static PyValue
binascii_a2b_base64_impl(PyObject *module, Py_buffer *data, int strict_mode,
                         int padded, PyBytesObject *alphabet,
                         Py_buffer *ignorechars, int canonical);

static PyValue
binascii_a2b_base64(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 5
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(strict_mode)), _PyValue_STATIC_IMMORTAL(&_Py_ID(padded)), _PyValue_STATIC_IMMORTAL(&_Py_ID(alphabet)), _PyValue_STATIC_IMMORTAL(&_Py_ID(ignorechars)), _PyValue_STATIC_IMMORTAL(&_Py_ID(canonical)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "strict_mode", "padded", "alphabet", "ignorechars", "canonical", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "a2b_base64",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[6];
    Py_buffer data = {NULL, NULL};
    int strict_mode = -1;
    int padded = 1;
    PyBytesObject *alphabet = NULL;
    Py_buffer ignorechars = {NULL, NULL};
    int canonical = 0;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 6)) {
        goto exit;
    }
    if (!ascii_buffer_converter(PyValue_Borrow(args[_idx[0]]), &data)) {
        goto exit;
    }
    if (_idx[1] >= 0) {
        strict_mode = _PyValue_IsTrue(PyValue_Borrow(args[_idx[1]]));
        if (strict_mode < 0) {
            goto exit;
        }
    }
    if (_idx[2] >= 0) {
        padded = _PyValue_IsTrue(PyValue_Borrow(args[_idx[2]]));
        if (padded < 0) {
            goto exit;
        }
    }
    if (_idx[3] >= 0) {
        if (!PyValue_IsObject(PyValue_Borrow(args[_idx[3]]))) {
            _PyArg_BadArgumentValue("a2b_base64", "argument 'alphabet'", "bytes", PyValue_Borrow(args[_idx[3]]));
            goto exit;
        }
        if (!PyBytes_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[3]])))) {
            _PyArg_BadArgument("a2b_base64", "argument 'alphabet'", "bytes", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[3]])));
            goto exit;
        }
        alphabet = (PyBytesObject *)PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[3]]));
    }
    if (_idx[4] >= 0) {
        if (!PyValue_IsObject(PyValue_Borrow(args[_idx[4]]))) {
            PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[_idx[4]]))->tp_name);
            goto exit;
        }
        if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[4]])), &ignorechars, PyBUF_SIMPLE) != 0) {
            goto exit;
        }
    }
    if (_idx[5] >= 0) {
        canonical = _PyValue_IsTrue(PyValue_Borrow(args[_idx[5]]));
        if (canonical < 0) {
            goto exit;
        }
    }
    return_value = binascii_a2b_base64_impl(PyValue_AsKnownObjectBorrow(module), &data, strict_mode, padded, alphabet, &ignorechars, canonical);

exit:
    /* Cleanup for data */
    if (data.obj)
       PyBuffer_Release(&data);
    /* Cleanup for ignorechars */
    if (ignorechars.obj) {
       PyBuffer_Release(&ignorechars);
    }

    return return_value;
}

PyDoc_STRVAR(binascii_b2a_base64__doc__,
"b2a_base64($module, data, /, *, padded=True, wrapcol=0, newline=True,\n"
"           alphabet=BASE64_ALPHABET)\n"
"--\n"
"\n"
"Base64-code line of data.\n"
"\n"
"  padded\n"
"    When set to false, omit padding in the output.");

#define BINASCII_B2A_BASE64_METHODDEF    \
    {"b2a_base64", _PyCFunction_CAST(binascii_b2a_base64), METH_VALUE|METH_KEYWORDS, binascii_b2a_base64__doc__},

static PyValue
binascii_b2a_base64_impl(PyObject *module, Py_buffer *data, int padded,
                         size_t wrapcol, int newline, Py_buffer *alphabet);

static PyValue
binascii_b2a_base64(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 4
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(padded)), _PyValue_STATIC_IMMORTAL(&_Py_ID(wrapcol)), _PyValue_STATIC_IMMORTAL(&_Py_ID(newline)), _PyValue_STATIC_IMMORTAL(&_Py_ID(alphabet)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "padded", "wrapcol", "newline", "alphabet", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "b2a_base64",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[5];
    Py_buffer data = {NULL, NULL};
    int padded = 1;
    size_t wrapcol = 0;
    int newline = 1;
    Py_buffer alphabet = {NULL, NULL};

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 5)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[_idx[0]]))) {
        PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[_idx[0]]))->tp_name);
        goto exit;
    }
    if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])), &data, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (_idx[1] >= 0) {
        padded = _PyValue_IsTrue(PyValue_Borrow(args[_idx[1]]));
        if (padded < 0) {
            goto exit;
        }
    }
    if (_idx[2] >= 0) {
        {
            Py_ssize_t _bytes = _PyValue_AsNativeBytes(PyValue_Borrow(args[_idx[2]]), &wrapcol, sizeof(size_t),
                    Py_ASNATIVEBYTES_NATIVE_ENDIAN |
                    Py_ASNATIVEBYTES_ALLOW_INDEX |
                    Py_ASNATIVEBYTES_REJECT_NEGATIVE |
                    Py_ASNATIVEBYTES_UNSIGNED_BUFFER);
            if (_bytes < 0) {
                goto exit;
            }
            if ((size_t)_bytes > sizeof(size_t)) {
                PyErr_SetString(PyExc_OverflowError,
                            "Python int too large for C size_t");
                goto exit;
            }
        }
    }
    if (_idx[3] >= 0) {
        newline = _PyValue_IsTrue(PyValue_Borrow(args[_idx[3]]));
        if (newline < 0) {
            goto exit;
        }
    }
    if (_idx[4] >= 0) {
        if (!PyValue_IsObject(PyValue_Borrow(args[_idx[4]]))) {
            PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[_idx[4]]))->tp_name);
            goto exit;
        }
        if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[4]])), &alphabet, PyBUF_SIMPLE) != 0) {
            goto exit;
        }
    }
    return_value = binascii_b2a_base64_impl(PyValue_AsKnownObjectBorrow(module), &data, padded, wrapcol, newline, &alphabet);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }
    /* Cleanup for alphabet */
    if (alphabet.obj) {
       PyBuffer_Release(&alphabet);
    }

    return return_value;
}

PyDoc_STRVAR(binascii_a2b_ascii85__doc__,
"a2b_ascii85($module, data, /, *, foldspaces=False, adobe=False,\n"
"            ignorechars=b\'\', canonical=False)\n"
"--\n"
"\n"
"Decode Ascii85 data.\n"
"\n"
"  foldspaces\n"
"    Allow \'y\' as a short form encoding four spaces.\n"
"  adobe\n"
"    Expect data to be terminated with \'~>\' as in Adobe Ascii85, and\n"
"    optionally accept leading \'<~\'.\n"
"  ignorechars\n"
"    A byte string containing characters to ignore from the input.\n"
"  canonical\n"
"    When set to true, reject non-canonical encodings.");

#define BINASCII_A2B_ASCII85_METHODDEF    \
    {"a2b_ascii85", _PyCFunction_CAST(binascii_a2b_ascii85), METH_VALUE|METH_KEYWORDS, binascii_a2b_ascii85__doc__},

static PyValue
binascii_a2b_ascii85_impl(PyObject *module, Py_buffer *data, int foldspaces,
                          int adobe, Py_buffer *ignorechars, int canonical);

static PyValue
binascii_a2b_ascii85(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 4
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(foldspaces)), _PyValue_STATIC_IMMORTAL(&_Py_ID(adobe)), _PyValue_STATIC_IMMORTAL(&_Py_ID(ignorechars)), _PyValue_STATIC_IMMORTAL(&_Py_ID(canonical)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "foldspaces", "adobe", "ignorechars", "canonical", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "a2b_ascii85",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[5];
    Py_buffer data = {NULL, NULL};
    int foldspaces = 0;
    int adobe = 0;
    Py_buffer ignorechars = {.buf = "", .obj = NULL, .len = 0};
    int canonical = 0;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 5)) {
        goto exit;
    }
    if (!ascii_buffer_converter(PyValue_Borrow(args[_idx[0]]), &data)) {
        goto exit;
    }
    if (_idx[1] >= 0) {
        foldspaces = _PyValue_IsTrue(PyValue_Borrow(args[_idx[1]]));
        if (foldspaces < 0) {
            goto exit;
        }
    }
    if (_idx[2] >= 0) {
        adobe = _PyValue_IsTrue(PyValue_Borrow(args[_idx[2]]));
        if (adobe < 0) {
            goto exit;
        }
    }
    if (_idx[3] >= 0) {
        if (!PyValue_IsObject(PyValue_Borrow(args[_idx[3]]))) {
            PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[_idx[3]]))->tp_name);
            goto exit;
        }
        if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[3]])), &ignorechars, PyBUF_SIMPLE) != 0) {
            goto exit;
        }
    }
    if (_idx[4] >= 0) {
        canonical = _PyValue_IsTrue(PyValue_Borrow(args[_idx[4]]));
        if (canonical < 0) {
            goto exit;
        }
    }
    return_value = binascii_a2b_ascii85_impl(PyValue_AsKnownObjectBorrow(module), &data, foldspaces, adobe, &ignorechars, canonical);

exit:
    /* Cleanup for data */
    if (data.obj)
       PyBuffer_Release(&data);
    /* Cleanup for ignorechars */
    if (ignorechars.obj) {
       PyBuffer_Release(&ignorechars);
    }

    return return_value;
}

PyDoc_STRVAR(binascii_b2a_ascii85__doc__,
"b2a_ascii85($module, data, /, *, foldspaces=False, wrapcol=0,\n"
"            pad=False, adobe=False)\n"
"--\n"
"\n"
"Ascii85-encode data.\n"
"\n"
"  foldspaces\n"
"    Emit \'y\' as a short form encoding four spaces.\n"
"  wrapcol\n"
"    Split result into lines of provided width.\n"
"  pad\n"
"    Retain zero-padding bytes at end of output.\n"
"  adobe\n"
"    Wrap result in \'<~\' and \'~>\' as in Adobe Ascii85.");

#define BINASCII_B2A_ASCII85_METHODDEF    \
    {"b2a_ascii85", _PyCFunction_CAST(binascii_b2a_ascii85), METH_VALUE|METH_KEYWORDS, binascii_b2a_ascii85__doc__},

static PyValue
binascii_b2a_ascii85_impl(PyObject *module, Py_buffer *data, int foldspaces,
                          size_t wrapcol, int pad, int adobe);

static PyValue
binascii_b2a_ascii85(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 4
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(foldspaces)), _PyValue_STATIC_IMMORTAL(&_Py_ID(wrapcol)), _PyValue_STATIC_IMMORTAL(&_Py_ID(pad)), _PyValue_STATIC_IMMORTAL(&_Py_ID(adobe)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "foldspaces", "wrapcol", "pad", "adobe", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "b2a_ascii85",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[5];
    Py_buffer data = {NULL, NULL};
    int foldspaces = 0;
    size_t wrapcol = 0;
    int pad = 0;
    int adobe = 0;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 5)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[_idx[0]]))) {
        PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[_idx[0]]))->tp_name);
        goto exit;
    }
    if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])), &data, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (_idx[1] >= 0) {
        foldspaces = _PyValue_IsTrue(PyValue_Borrow(args[_idx[1]]));
        if (foldspaces < 0) {
            goto exit;
        }
    }
    if (_idx[2] >= 0) {
        {
            Py_ssize_t _bytes = _PyValue_AsNativeBytes(PyValue_Borrow(args[_idx[2]]), &wrapcol, sizeof(size_t),
                    Py_ASNATIVEBYTES_NATIVE_ENDIAN |
                    Py_ASNATIVEBYTES_ALLOW_INDEX |
                    Py_ASNATIVEBYTES_REJECT_NEGATIVE |
                    Py_ASNATIVEBYTES_UNSIGNED_BUFFER);
            if (_bytes < 0) {
                goto exit;
            }
            if ((size_t)_bytes > sizeof(size_t)) {
                PyErr_SetString(PyExc_OverflowError,
                            "Python int too large for C size_t");
                goto exit;
            }
        }
    }
    if (_idx[3] >= 0) {
        pad = _PyValue_IsTrue(PyValue_Borrow(args[_idx[3]]));
        if (pad < 0) {
            goto exit;
        }
    }
    if (_idx[4] >= 0) {
        adobe = _PyValue_IsTrue(PyValue_Borrow(args[_idx[4]]));
        if (adobe < 0) {
            goto exit;
        }
    }
    return_value = binascii_b2a_ascii85_impl(PyValue_AsKnownObjectBorrow(module), &data, foldspaces, wrapcol, pad, adobe);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(binascii_a2b_base85__doc__,
"a2b_base85($module, data, /, *, alphabet=BASE85_ALPHABET,\n"
"           ignorechars=b\'\', canonical=False)\n"
"--\n"
"\n"
"Decode a line of Base85 data.\n"
"\n"
"  ignorechars\n"
"    A byte string containing characters to ignore from the input.\n"
"  canonical\n"
"    When set to true, reject non-canonical encodings.");

#define BINASCII_A2B_BASE85_METHODDEF    \
    {"a2b_base85", _PyCFunction_CAST(binascii_a2b_base85), METH_VALUE|METH_KEYWORDS, binascii_a2b_base85__doc__},

static PyValue
binascii_a2b_base85_impl(PyObject *module, Py_buffer *data,
                         PyBytesObject *alphabet, Py_buffer *ignorechars,
                         int canonical);

static PyValue
binascii_a2b_base85(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(alphabet)), _PyValue_STATIC_IMMORTAL(&_Py_ID(ignorechars)), _PyValue_STATIC_IMMORTAL(&_Py_ID(canonical)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "alphabet", "ignorechars", "canonical", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "a2b_base85",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[4];
    Py_buffer data = {NULL, NULL};
    PyBytesObject *alphabet = NULL;
    Py_buffer ignorechars = {.buf = "", .obj = NULL, .len = 0};
    int canonical = 0;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 4)) {
        goto exit;
    }
    if (!ascii_buffer_converter(PyValue_Borrow(args[_idx[0]]), &data)) {
        goto exit;
    }
    if (_idx[1] >= 0) {
        if (!PyValue_IsObject(PyValue_Borrow(args[_idx[1]]))) {
            _PyArg_BadArgumentValue("a2b_base85", "argument 'alphabet'", "bytes", PyValue_Borrow(args[_idx[1]]));
            goto exit;
        }
        if (!PyBytes_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])))) {
            _PyArg_BadArgument("a2b_base85", "argument 'alphabet'", "bytes", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])));
            goto exit;
        }
        alphabet = (PyBytesObject *)PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]]));
    }
    if (_idx[2] >= 0) {
        if (!PyValue_IsObject(PyValue_Borrow(args[_idx[2]]))) {
            PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[_idx[2]]))->tp_name);
            goto exit;
        }
        if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[2]])), &ignorechars, PyBUF_SIMPLE) != 0) {
            goto exit;
        }
    }
    if (_idx[3] >= 0) {
        canonical = _PyValue_IsTrue(PyValue_Borrow(args[_idx[3]]));
        if (canonical < 0) {
            goto exit;
        }
    }
    return_value = binascii_a2b_base85_impl(PyValue_AsKnownObjectBorrow(module), &data, alphabet, &ignorechars, canonical);

exit:
    /* Cleanup for data */
    if (data.obj)
       PyBuffer_Release(&data);
    /* Cleanup for ignorechars */
    if (ignorechars.obj) {
       PyBuffer_Release(&ignorechars);
    }

    return return_value;
}

PyDoc_STRVAR(binascii_b2a_base85__doc__,
"b2a_base85($module, data, /, *, pad=False, wrapcol=0,\n"
"           alphabet=BASE85_ALPHABET)\n"
"--\n"
"\n"
"Base85-code line of data.\n"
"\n"
"  pad\n"
"    Retain zero-padding bytes at end of output.");

#define BINASCII_B2A_BASE85_METHODDEF    \
    {"b2a_base85", _PyCFunction_CAST(binascii_b2a_base85), METH_VALUE|METH_KEYWORDS, binascii_b2a_base85__doc__},

static PyValue
binascii_b2a_base85_impl(PyObject *module, Py_buffer *data, int pad,
                         size_t wrapcol, Py_buffer *alphabet);

static PyValue
binascii_b2a_base85(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(pad)), _PyValue_STATIC_IMMORTAL(&_Py_ID(wrapcol)), _PyValue_STATIC_IMMORTAL(&_Py_ID(alphabet)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "pad", "wrapcol", "alphabet", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "b2a_base85",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[4];
    Py_buffer data = {NULL, NULL};
    int pad = 0;
    size_t wrapcol = 0;
    Py_buffer alphabet = {NULL, NULL};

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 4)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[_idx[0]]))) {
        PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[_idx[0]]))->tp_name);
        goto exit;
    }
    if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])), &data, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (_idx[1] >= 0) {
        pad = _PyValue_IsTrue(PyValue_Borrow(args[_idx[1]]));
        if (pad < 0) {
            goto exit;
        }
    }
    if (_idx[2] >= 0) {
        {
            Py_ssize_t _bytes = _PyValue_AsNativeBytes(PyValue_Borrow(args[_idx[2]]), &wrapcol, sizeof(size_t),
                    Py_ASNATIVEBYTES_NATIVE_ENDIAN |
                    Py_ASNATIVEBYTES_ALLOW_INDEX |
                    Py_ASNATIVEBYTES_REJECT_NEGATIVE |
                    Py_ASNATIVEBYTES_UNSIGNED_BUFFER);
            if (_bytes < 0) {
                goto exit;
            }
            if ((size_t)_bytes > sizeof(size_t)) {
                PyErr_SetString(PyExc_OverflowError,
                            "Python int too large for C size_t");
                goto exit;
            }
        }
    }
    if (_idx[3] >= 0) {
        if (!PyValue_IsObject(PyValue_Borrow(args[_idx[3]]))) {
            PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[_idx[3]]))->tp_name);
            goto exit;
        }
        if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[3]])), &alphabet, PyBUF_SIMPLE) != 0) {
            goto exit;
        }
    }
    return_value = binascii_b2a_base85_impl(PyValue_AsKnownObjectBorrow(module), &data, pad, wrapcol, &alphabet);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }
    /* Cleanup for alphabet */
    if (alphabet.obj) {
       PyBuffer_Release(&alphabet);
    }

    return return_value;
}

PyDoc_STRVAR(binascii_a2b_base32__doc__,
"a2b_base32($module, data, /, *, padded=True, alphabet=BASE32_ALPHABET,\n"
"           ignorechars=b\'\', canonical=False)\n"
"--\n"
"\n"
"Decode a line of base32 data.\n"
"\n"
"  padded\n"
"    When set to false, padding in input is not required.\n"
"  ignorechars\n"
"    A byte string containing characters to ignore from the input.\n"
"  canonical\n"
"    When set to true, reject non-zero padding bits per RFC 4648 section 3.5.");

#define BINASCII_A2B_BASE32_METHODDEF    \
    {"a2b_base32", _PyCFunction_CAST(binascii_a2b_base32), METH_VALUE|METH_KEYWORDS, binascii_a2b_base32__doc__},

static PyValue
binascii_a2b_base32_impl(PyObject *module, Py_buffer *data, int padded,
                         PyBytesObject *alphabet, Py_buffer *ignorechars,
                         int canonical);

static PyValue
binascii_a2b_base32(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 4
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(padded)), _PyValue_STATIC_IMMORTAL(&_Py_ID(alphabet)), _PyValue_STATIC_IMMORTAL(&_Py_ID(ignorechars)), _PyValue_STATIC_IMMORTAL(&_Py_ID(canonical)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "padded", "alphabet", "ignorechars", "canonical", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "a2b_base32",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[5];
    Py_buffer data = {NULL, NULL};
    int padded = 1;
    PyBytesObject *alphabet = NULL;
    Py_buffer ignorechars = {.buf = "", .obj = NULL, .len = 0};
    int canonical = 0;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 5)) {
        goto exit;
    }
    if (!ascii_buffer_converter(PyValue_Borrow(args[_idx[0]]), &data)) {
        goto exit;
    }
    if (_idx[1] >= 0) {
        padded = _PyValue_IsTrue(PyValue_Borrow(args[_idx[1]]));
        if (padded < 0) {
            goto exit;
        }
    }
    if (_idx[2] >= 0) {
        if (!PyValue_IsObject(PyValue_Borrow(args[_idx[2]]))) {
            _PyArg_BadArgumentValue("a2b_base32", "argument 'alphabet'", "bytes", PyValue_Borrow(args[_idx[2]]));
            goto exit;
        }
        if (!PyBytes_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[2]])))) {
            _PyArg_BadArgument("a2b_base32", "argument 'alphabet'", "bytes", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[2]])));
            goto exit;
        }
        alphabet = (PyBytesObject *)PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[2]]));
    }
    if (_idx[3] >= 0) {
        if (!PyValue_IsObject(PyValue_Borrow(args[_idx[3]]))) {
            PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[_idx[3]]))->tp_name);
            goto exit;
        }
        if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[3]])), &ignorechars, PyBUF_SIMPLE) != 0) {
            goto exit;
        }
    }
    if (_idx[4] >= 0) {
        canonical = _PyValue_IsTrue(PyValue_Borrow(args[_idx[4]]));
        if (canonical < 0) {
            goto exit;
        }
    }
    return_value = binascii_a2b_base32_impl(PyValue_AsKnownObjectBorrow(module), &data, padded, alphabet, &ignorechars, canonical);

exit:
    /* Cleanup for data */
    if (data.obj)
       PyBuffer_Release(&data);
    /* Cleanup for ignorechars */
    if (ignorechars.obj) {
       PyBuffer_Release(&ignorechars);
    }

    return return_value;
}

PyDoc_STRVAR(binascii_b2a_base32__doc__,
"b2a_base32($module, data, /, *, padded=True, wrapcol=0,\n"
"           alphabet=BASE32_ALPHABET)\n"
"--\n"
"\n"
"Base32-code line of data.\n"
"\n"
"  padded\n"
"    When set to false, omit padding in the output.");

#define BINASCII_B2A_BASE32_METHODDEF    \
    {"b2a_base32", _PyCFunction_CAST(binascii_b2a_base32), METH_VALUE|METH_KEYWORDS, binascii_b2a_base32__doc__},

static PyValue
binascii_b2a_base32_impl(PyObject *module, Py_buffer *data, int padded,
                         size_t wrapcol, Py_buffer *alphabet);

static PyValue
binascii_b2a_base32(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(padded)), _PyValue_STATIC_IMMORTAL(&_Py_ID(wrapcol)), _PyValue_STATIC_IMMORTAL(&_Py_ID(alphabet)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "padded", "wrapcol", "alphabet", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "b2a_base32",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[4];
    Py_buffer data = {NULL, NULL};
    int padded = 1;
    size_t wrapcol = 0;
    Py_buffer alphabet = {NULL, NULL};

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 4)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[_idx[0]]))) {
        PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[_idx[0]]))->tp_name);
        goto exit;
    }
    if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])), &data, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (_idx[1] >= 0) {
        padded = _PyValue_IsTrue(PyValue_Borrow(args[_idx[1]]));
        if (padded < 0) {
            goto exit;
        }
    }
    if (_idx[2] >= 0) {
        {
            Py_ssize_t _bytes = _PyValue_AsNativeBytes(PyValue_Borrow(args[_idx[2]]), &wrapcol, sizeof(size_t),
                    Py_ASNATIVEBYTES_NATIVE_ENDIAN |
                    Py_ASNATIVEBYTES_ALLOW_INDEX |
                    Py_ASNATIVEBYTES_REJECT_NEGATIVE |
                    Py_ASNATIVEBYTES_UNSIGNED_BUFFER);
            if (_bytes < 0) {
                goto exit;
            }
            if ((size_t)_bytes > sizeof(size_t)) {
                PyErr_SetString(PyExc_OverflowError,
                            "Python int too large for C size_t");
                goto exit;
            }
        }
    }
    if (_idx[3] >= 0) {
        if (!PyValue_IsObject(PyValue_Borrow(args[_idx[3]]))) {
            PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[_idx[3]]))->tp_name);
            goto exit;
        }
        if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[3]])), &alphabet, PyBUF_SIMPLE) != 0) {
            goto exit;
        }
    }
    return_value = binascii_b2a_base32_impl(PyValue_AsKnownObjectBorrow(module), &data, padded, wrapcol, &alphabet);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }
    /* Cleanup for alphabet */
    if (alphabet.obj) {
       PyBuffer_Release(&alphabet);
    }

    return return_value;
}

PyDoc_STRVAR(binascii_crc_hqx__doc__,
"crc_hqx($module, data, crc, /)\n"
"--\n"
"\n"
"Compute CRC-CCITT incrementally.");

#define BINASCII_CRC_HQX_METHODDEF    \
    {"crc_hqx", _PyCFunction_CAST(binascii_crc_hqx), METH_VALUE, binascii_crc_hqx__doc__},

static PyValue
binascii_crc_hqx_impl(PyObject *module, Py_buffer *data, unsigned int crc);

static PyValue
binascii_crc_hqx(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_buffer data = {NULL, NULL};
    unsigned int crc;

    if (!_PyArg_CheckPositional("crc_hqx", nargs, 2, 2)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[0]))->tp_name);
        goto exit;
    }
    if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])), &data, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    {
        Py_ssize_t _bytes = _PyValue_AsNativeBytes(PyValue_Borrow(args[1]), &crc, sizeof(unsigned int),
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
    return_value = binascii_crc_hqx_impl(PyValue_AsKnownObjectBorrow(module), &data, crc);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(binascii_crc32__doc__,
"crc32($module, data, crc=0, /)\n"
"--\n"
"\n"
"Compute CRC-32 incrementally.");

#define BINASCII_CRC32_METHODDEF    \
    {"crc32", _PyCFunction_CAST(binascii_crc32), METH_VALUE, binascii_crc32__doc__},

static unsigned int
binascii_crc32_impl(PyObject *module, Py_buffer *data, unsigned int crc);

static PyValue
binascii_crc32(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_buffer data = {NULL, NULL};
    unsigned int crc = 0;
    unsigned int _return_value;

    if (!_PyArg_CheckPositional("crc32", nargs, 1, 2)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[0]))->tp_name);
        goto exit;
    }
    if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])), &data, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    {
        Py_ssize_t _bytes = _PyValue_AsNativeBytes(PyValue_Borrow(args[1]), &crc, sizeof(unsigned int),
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
skip_optional:
    _return_value = binascii_crc32_impl(PyValue_AsKnownObjectBorrow(module), &data, crc);
    if ((_return_value == (unsigned int)-1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = (uint64_t)_return_value <= INT64_MAX
        ? PyValue_FromInt64((int64_t)_return_value)
        : _PyValue_FromObjectResult(PyLong_FromUnsignedLongLong((unsigned long long)_return_value));

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(binascii_b2a_hex__doc__,
"b2a_hex($module, /, data, sep=<unrepresentable>, bytes_per_sep=1)\n"
"--\n"
"\n"
"Hexadecimal representation of binary data.\n"
"\n"
"  sep\n"
"    An optional single character or byte to separate hex bytes.\n"
"  bytes_per_sep\n"
"    How many bytes between separators.  Positive values count from the\n"
"    right, negative values count from the left.\n"
"\n"
"The return value is a bytes object.  This function is also\n"
"available as \"hexlify()\".\n"
"\n"
"Example:\n"
">>> binascii.b2a_hex(b\'\\xb9\\x01\\xef\')\n"
"b\'b901ef\'\n"
">>> binascii.hexlify(b\'\\xb9\\x01\\xef\', \':\')\n"
"b\'b9:01:ef\'\n"
">>> binascii.b2a_hex(b\'\\xb9\\x01\\xef\', b\'_\', 2)\n"
"b\'b9_01ef\'");

#define BINASCII_B2A_HEX_METHODDEF    \
    {"b2a_hex", _PyCFunction_CAST(binascii_b2a_hex), METH_VALUE|METH_KEYWORDS, binascii_b2a_hex__doc__},

static PyValue
binascii_b2a_hex_impl(PyObject *module, Py_buffer *data, PyValueBorrow sep,
                      Py_ssize_t bytes_per_sep);

static PyValue
binascii_b2a_hex(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(data)), _PyValue_STATIC_IMMORTAL(&_Py_ID(sep)), _PyValue_STATIC_IMMORTAL(&_Py_ID(bytes_per_sep)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"data", "sep", "bytes_per_sep", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "b2a_hex",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    Py_buffer data = {NULL, NULL};
    PyValueBorrow sep = PyValue_Borrow(PyValue_NULL);
    Py_ssize_t bytes_per_sep = 1;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, _idx, 3)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[_idx[0]]))) {
        PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[_idx[0]]))->tp_name);
        goto exit;
    }
    if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])), &data, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (_idx[1] >= 0) {
        sep = PyValue_Borrow(args[_idx[1]]);
    }
    if (_idx[2] >= 0) {
        {
            Py_ssize_t ival = -1;
            PyValue iobj = _PyValue_Index(PyValue_Borrow(args[_idx[2]]));
            if (!PyValue_IsNull(iobj)) {
                ival = _PyValue_IntAsSsize_t(PyValue_Borrow(iobj));
                PyValue_CLOSE(iobj);
            }
            if (ival == -1 && PyErr_Occurred()) {
                goto exit;
            }
            bytes_per_sep = ival;
        }
    }
    return_value = binascii_b2a_hex_impl(PyValue_AsKnownObjectBorrow(module), &data, sep, bytes_per_sep);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(binascii_hexlify__doc__,
"hexlify($module, /, data, sep=<unrepresentable>, bytes_per_sep=1)\n"
"--\n"
"\n"
"Hexadecimal representation of binary data.\n"
"\n"
"  sep\n"
"    An optional single character or byte to separate hex bytes.\n"
"  bytes_per_sep\n"
"    How many bytes between separators.  Positive values count from the\n"
"    right, negative values count from the left.\n"
"\n"
"The return value is a bytes object.  This function is also\n"
"available as \"b2a_hex()\".");

#define BINASCII_HEXLIFY_METHODDEF    \
    {"hexlify", _PyCFunction_CAST(binascii_hexlify), METH_VALUE|METH_KEYWORDS, binascii_hexlify__doc__},

static PyValue
binascii_hexlify_impl(PyObject *module, Py_buffer *data, PyValueBorrow sep,
                      Py_ssize_t bytes_per_sep);

static PyValue
binascii_hexlify(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(data)), _PyValue_STATIC_IMMORTAL(&_Py_ID(sep)), _PyValue_STATIC_IMMORTAL(&_Py_ID(bytes_per_sep)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"data", "sep", "bytes_per_sep", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "hexlify",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    Py_buffer data = {NULL, NULL};
    PyValueBorrow sep = PyValue_Borrow(PyValue_NULL);
    Py_ssize_t bytes_per_sep = 1;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, _idx, 3)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[_idx[0]]))) {
        PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[_idx[0]]))->tp_name);
        goto exit;
    }
    if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])), &data, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (_idx[1] >= 0) {
        sep = PyValue_Borrow(args[_idx[1]]);
    }
    if (_idx[2] >= 0) {
        {
            Py_ssize_t ival = -1;
            PyValue iobj = _PyValue_Index(PyValue_Borrow(args[_idx[2]]));
            if (!PyValue_IsNull(iobj)) {
                ival = _PyValue_IntAsSsize_t(PyValue_Borrow(iobj));
                PyValue_CLOSE(iobj);
            }
            if (ival == -1 && PyErr_Occurred()) {
                goto exit;
            }
            bytes_per_sep = ival;
        }
    }
    return_value = binascii_hexlify_impl(PyValue_AsKnownObjectBorrow(module), &data, sep, bytes_per_sep);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(binascii_a2b_hex__doc__,
"a2b_hex($module, hexstr, /, *, ignorechars=b\'\')\n"
"--\n"
"\n"
"Binary data of hexadecimal representation.\n"
"\n"
"  ignorechars\n"
"    A byte string containing characters to ignore from the input.\n"
"\n"
"hexstr must contain an even number of hex digits (upper or lower case).\n"
"This function is also available as \"unhexlify()\".");

#define BINASCII_A2B_HEX_METHODDEF    \
    {"a2b_hex", _PyCFunction_CAST(binascii_a2b_hex), METH_VALUE|METH_KEYWORDS, binascii_a2b_hex__doc__},

static PyValue
binascii_a2b_hex_impl(PyObject *module, Py_buffer *hexstr,
                      Py_buffer *ignorechars);

static PyValue
binascii_a2b_hex(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(ignorechars)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "ignorechars", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "a2b_hex",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    Py_buffer hexstr = {NULL, NULL};
    Py_buffer ignorechars = {.buf = "", .obj = NULL, .len = 0};

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    if (!ascii_buffer_converter(PyValue_Borrow(args[_idx[0]]), &hexstr)) {
        goto exit;
    }
    if (_idx[1] >= 0) {
        if (!PyValue_IsObject(PyValue_Borrow(args[_idx[1]]))) {
            PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[_idx[1]]))->tp_name);
            goto exit;
        }
        if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])), &ignorechars, PyBUF_SIMPLE) != 0) {
            goto exit;
        }
    }
    return_value = binascii_a2b_hex_impl(PyValue_AsKnownObjectBorrow(module), &hexstr, &ignorechars);

exit:
    /* Cleanup for hexstr */
    if (hexstr.obj)
       PyBuffer_Release(&hexstr);
    /* Cleanup for ignorechars */
    if (ignorechars.obj) {
       PyBuffer_Release(&ignorechars);
    }

    return return_value;
}

PyDoc_STRVAR(binascii_unhexlify__doc__,
"unhexlify($module, hexstr, /, *, ignorechars=b\'\')\n"
"--\n"
"\n"
"Binary data of hexadecimal representation.\n"
"\n"
"  ignorechars\n"
"    A byte string containing characters to ignore from the input.\n"
"\n"
"hexstr must contain an even number of hex digits (upper or lower case).");

#define BINASCII_UNHEXLIFY_METHODDEF    \
    {"unhexlify", _PyCFunction_CAST(binascii_unhexlify), METH_VALUE|METH_KEYWORDS, binascii_unhexlify__doc__},

static PyValue
binascii_unhexlify_impl(PyObject *module, Py_buffer *hexstr,
                        Py_buffer *ignorechars);

static PyValue
binascii_unhexlify(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(ignorechars)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "ignorechars", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "unhexlify",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    Py_buffer hexstr = {NULL, NULL};
    Py_buffer ignorechars = {.buf = "", .obj = NULL, .len = 0};

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    if (!ascii_buffer_converter(PyValue_Borrow(args[_idx[0]]), &hexstr)) {
        goto exit;
    }
    if (_idx[1] >= 0) {
        if (!PyValue_IsObject(PyValue_Borrow(args[_idx[1]]))) {
            PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[_idx[1]]))->tp_name);
            goto exit;
        }
        if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])), &ignorechars, PyBUF_SIMPLE) != 0) {
            goto exit;
        }
    }
    return_value = binascii_unhexlify_impl(PyValue_AsKnownObjectBorrow(module), &hexstr, &ignorechars);

exit:
    /* Cleanup for hexstr */
    if (hexstr.obj)
       PyBuffer_Release(&hexstr);
    /* Cleanup for ignorechars */
    if (ignorechars.obj) {
       PyBuffer_Release(&ignorechars);
    }

    return return_value;
}

PyDoc_STRVAR(binascii_a2b_qp__doc__,
"a2b_qp($module, /, data, header=False, *, strip_ws=False)\n"
"--\n"
"\n"
"Decode a string of qp-encoded data.\n"
"\n"
"If strip_ws is true, whitespace at the end of a line is stripped, as\n"
"required by RFC 2045 when decoding a quoted-printable body.");

#define BINASCII_A2B_QP_METHODDEF    \
    {"a2b_qp", _PyCFunction_CAST(binascii_a2b_qp), METH_VALUE|METH_KEYWORDS, binascii_a2b_qp__doc__},

static PyValue
binascii_a2b_qp_impl(PyObject *module, Py_buffer *data, int header,
                     int strip_ws);

static PyValue
binascii_a2b_qp(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(data)), _PyValue_STATIC_IMMORTAL(&_Py_ID(header)), _PyValue_STATIC_IMMORTAL(&_Py_ID(strip_ws)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"data", "header", "strip_ws", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "a2b_qp",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    Py_buffer data = {NULL, NULL};
    int header = 0;
    int strip_ws = 0;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 3)) {
        goto exit;
    }
    if (!ascii_buffer_converter(PyValue_Borrow(args[_idx[0]]), &data)) {
        goto exit;
    }
    if (_idx[1] >= 0) {
        header = _PyValue_IsTrue(PyValue_Borrow(args[_idx[1]]));
        if (header < 0) {
            goto exit;
        }
    }
    if (_idx[2] >= 0) {
        strip_ws = _PyValue_IsTrue(PyValue_Borrow(args[_idx[2]]));
        if (strip_ws < 0) {
            goto exit;
        }
    }
    return_value = binascii_a2b_qp_impl(PyValue_AsKnownObjectBorrow(module), &data, header, strip_ws);

exit:
    /* Cleanup for data */
    if (data.obj)
       PyBuffer_Release(&data);

    return return_value;
}

PyDoc_STRVAR(binascii_b2a_qp__doc__,
"b2a_qp($module, /, data, quotetabs=False, istext=True, header=False)\n"
"--\n"
"\n"
"Encode a string using quoted-printable encoding.\n"
"\n"
"On encoding, when istext is set, newlines are not encoded, and white\n"
"space at end of lines is.  When istext is not set, \\r and \\n (CR/LF)\n"
"are both encoded.  When quotetabs is set, space and tabs are encoded.");

#define BINASCII_B2A_QP_METHODDEF    \
    {"b2a_qp", _PyCFunction_CAST(binascii_b2a_qp), METH_VALUE|METH_KEYWORDS, binascii_b2a_qp__doc__},

static PyValue
binascii_b2a_qp_impl(PyObject *module, Py_buffer *data, int quotetabs,
                     int istext, int header);

static PyValue
binascii_b2a_qp(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 4
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(data)), _PyValue_STATIC_IMMORTAL(&_Py_ID(quotetabs)), _PyValue_STATIC_IMMORTAL(&_Py_ID(istext)), _PyValue_STATIC_IMMORTAL(&_Py_ID(header)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"data", "quotetabs", "istext", "header", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "b2a_qp",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[4];
    Py_buffer data = {NULL, NULL};
    int quotetabs = 0;
    int istext = 1;
    int header = 0;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 4, /*minkw*/ 0, /*varpos*/ 0, _idx, 4)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[_idx[0]]))) {
        PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[_idx[0]]))->tp_name);
        goto exit;
    }
    if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])), &data, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (_idx[1] >= 0) {
        quotetabs = _PyValue_IsTrue(PyValue_Borrow(args[_idx[1]]));
        if (quotetabs < 0) {
            goto exit;
        }
    }
    if (_idx[2] >= 0) {
        istext = _PyValue_IsTrue(PyValue_Borrow(args[_idx[2]]));
        if (istext < 0) {
            goto exit;
        }
    }
    if (_idx[3] >= 0) {
        header = _PyValue_IsTrue(PyValue_Borrow(args[_idx[3]]));
        if (header < 0) {
            goto exit;
        }
    }
    return_value = binascii_b2a_qp_impl(PyValue_AsKnownObjectBorrow(module), &data, quotetabs, istext, header);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}
/*[clinic end generated code: output=0feeca2fd5a1ba32 input=a9049054013a1b77]*/
