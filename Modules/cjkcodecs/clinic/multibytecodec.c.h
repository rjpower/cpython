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
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywordsValue()
#include "pycore_unicodeobject.h" // _PyUnicode_AsUTF8NoNUL()
#include "pycore_value_call.h"    // METH_VALUE
#include "pycore_value_ops.h"     // _PyValue_IsTrue()

PyDoc_STRVAR(_multibytecodec_MultibyteCodec_encode__doc__,
"encode($self, /, input, errors=None)\n"
"--\n"
"\n"
"Return an encoded string version of \'input\'.\n"
"\n"
"\'errors\' may be given to set a different error handling scheme.\n"
"Default is \'strict\' meaning that encoding errors raise\n"
"a UnicodeEncodeError.  Other possible values are \'ignore\', \'replace\'\n"
"and \'xmlcharrefreplace\' as well as any other name registered with\n"
"codecs.register_error that can handle UnicodeEncodeErrors.");

#define _MULTIBYTECODEC_MULTIBYTECODEC_ENCODE_METHODDEF    \
    {"encode", _PyCFunction_CAST(_multibytecodec_MultibyteCodec_encode), METH_VALUE|METH_KEYWORDS, _multibytecodec_MultibyteCodec_encode__doc__},

static PyValue
_multibytecodec_MultibyteCodec_encode_impl(MultibyteCodecObject *self,
                                           PyValueBorrow input,
                                           const char *errors);

static PyValue
_multibytecodec_MultibyteCodec_encode(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(input)), _PyValue_STATIC_IMMORTAL(&_Py_ID(errors)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"input", "errors", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "encode",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    PyValueBorrow input;
    const char *errors = NULL;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    input = PyValue_Borrow(args[_idx[0]]);
    if (_idx[1] >= 0) {
        if (!PyValue_IsObject(PyValue_Borrow(args[_idx[1]]))) {
            _PyArg_BadArgumentValue("encode", "argument 'errors'", "str or None", PyValue_Borrow(args[_idx[1]]));
            goto exit;
        }
        if (PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])) == Py_None) {
            errors = NULL;
        }
        else if (PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])))) {
            errors = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])));
            if (errors == NULL) {
                goto exit;
            }
        }
        else {
            _PyArg_BadArgument("encode", "argument 'errors'", "str or None", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])));
            goto exit;
        }
    }
    return_value = _multibytecodec_MultibyteCodec_encode_impl((MultibyteCodecObject *)PyValue_AsKnownObjectBorrow(self), input, errors);

exit:
    return return_value;
}

PyDoc_STRVAR(_multibytecodec_MultibyteCodec_decode__doc__,
"decode($self, /, input, errors=None)\n"
"--\n"
"\n"
"Decodes \'input\'.\n"
"\n"
"\'errors\' may be given to set a different error handling scheme.\n"
"Default is \'strict\' meaning that encoding errors raise\n"
"a UnicodeDecodeError.  Other possible values are \'ignore\' and\n"
"\'replace\' as well as any other name registered with\n"
"codecs.register_error that is able to handle UnicodeDecodeErrors.\"");

#define _MULTIBYTECODEC_MULTIBYTECODEC_DECODE_METHODDEF    \
    {"decode", _PyCFunction_CAST(_multibytecodec_MultibyteCodec_decode), METH_VALUE|METH_KEYWORDS, _multibytecodec_MultibyteCodec_decode__doc__},

static PyValue
_multibytecodec_MultibyteCodec_decode_impl(MultibyteCodecObject *self,
                                           Py_buffer *input,
                                           const char *errors);

static PyValue
_multibytecodec_MultibyteCodec_decode(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(input)), _PyValue_STATIC_IMMORTAL(&_Py_ID(errors)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"input", "errors", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "decode",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    Py_buffer input = {NULL, NULL};
    const char *errors = NULL;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[_idx[0]]))) {
        PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[_idx[0]]))->tp_name);
        goto exit;
    }
    if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])), &input, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (_idx[1] >= 0) {
        if (!PyValue_IsObject(PyValue_Borrow(args[_idx[1]]))) {
            _PyArg_BadArgumentValue("decode", "argument 'errors'", "str or None", PyValue_Borrow(args[_idx[1]]));
            goto exit;
        }
        if (PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])) == Py_None) {
            errors = NULL;
        }
        else if (PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])))) {
            errors = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])));
            if (errors == NULL) {
                goto exit;
            }
        }
        else {
            _PyArg_BadArgument("decode", "argument 'errors'", "str or None", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])));
            goto exit;
        }
    }
    return_value = _multibytecodec_MultibyteCodec_decode_impl((MultibyteCodecObject *)PyValue_AsKnownObjectBorrow(self), &input, errors);

exit:
    /* Cleanup for input */
    if (input.obj) {
       PyBuffer_Release(&input);
    }

    return return_value;
}

PyDoc_STRVAR(_multibytecodec_MultibyteIncrementalEncoder_encode__doc__,
"encode($self, /, input, final=False)\n"
"--\n"
"\n");

#define _MULTIBYTECODEC_MULTIBYTEINCREMENTALENCODER_ENCODE_METHODDEF    \
    {"encode", _PyCFunction_CAST(_multibytecodec_MultibyteIncrementalEncoder_encode), METH_VALUE|METH_KEYWORDS, _multibytecodec_MultibyteIncrementalEncoder_encode__doc__},

static PyValue
_multibytecodec_MultibyteIncrementalEncoder_encode_impl(MultibyteIncrementalEncoderObject *self,
                                                        PyValueBorrow input,
                                                        int final);

static PyValue
_multibytecodec_MultibyteIncrementalEncoder_encode(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(input)), _PyValue_STATIC_IMMORTAL(&_Py_ID(final)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"input", "final", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "encode",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    PyValueBorrow input;
    int final = 0;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    input = PyValue_Borrow(args[_idx[0]]);
    if (_idx[1] >= 0) {
        final = _PyValue_IsTrue(PyValue_Borrow(args[_idx[1]]));
        if (final < 0) {
            goto exit;
        }
    }
    return_value = _multibytecodec_MultibyteIncrementalEncoder_encode_impl((MultibyteIncrementalEncoderObject *)PyValue_AsKnownObjectBorrow(self), input, final);

exit:
    return return_value;
}

PyDoc_STRVAR(_multibytecodec_MultibyteIncrementalEncoder_getstate__doc__,
"getstate($self, /)\n"
"--\n"
"\n");

#define _MULTIBYTECODEC_MULTIBYTEINCREMENTALENCODER_GETSTATE_METHODDEF    \
    {"getstate", _PyCFunction_CAST(_multibytecodec_MultibyteIncrementalEncoder_getstate), METH_VALUE|METH_NOARGS, _multibytecodec_MultibyteIncrementalEncoder_getstate__doc__},

static PyValue
_multibytecodec_MultibyteIncrementalEncoder_getstate_impl(MultibyteIncrementalEncoderObject *self);

static PyValue
_multibytecodec_MultibyteIncrementalEncoder_getstate(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _multibytecodec_MultibyteIncrementalEncoder_getstate_impl((MultibyteIncrementalEncoderObject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_multibytecodec_MultibyteIncrementalEncoder_setstate__doc__,
"setstate($self, state, /)\n"
"--\n"
"\n");

#define _MULTIBYTECODEC_MULTIBYTEINCREMENTALENCODER_SETSTATE_METHODDEF    \
    {"setstate", _PyCFunction_CAST(_multibytecodec_MultibyteIncrementalEncoder_setstate), METH_VALUE|METH_O, _multibytecodec_MultibyteIncrementalEncoder_setstate__doc__},

static PyValue
_multibytecodec_MultibyteIncrementalEncoder_setstate_impl(MultibyteIncrementalEncoderObject *self,
                                                          PyValueBorrow statelong);

static PyValue
_multibytecodec_MultibyteIncrementalEncoder_setstate(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow statelong;

    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[0])), &PyLong_Type)) {
        _PyArg_BadArgumentValue("setstate", "argument", "int", PyValue_Borrow(args[0]));
        goto exit;
    }
    statelong = PyValue_Borrow(args[0]);
    return_value = _multibytecodec_MultibyteIncrementalEncoder_setstate_impl((MultibyteIncrementalEncoderObject *)PyValue_AsKnownObjectBorrow(self), statelong);

exit:
    return return_value;
}

PyDoc_STRVAR(_multibytecodec_MultibyteIncrementalEncoder_reset__doc__,
"reset($self, /)\n"
"--\n"
"\n");

#define _MULTIBYTECODEC_MULTIBYTEINCREMENTALENCODER_RESET_METHODDEF    \
    {"reset", _PyCFunction_CAST(_multibytecodec_MultibyteIncrementalEncoder_reset), METH_VALUE|METH_NOARGS, _multibytecodec_MultibyteIncrementalEncoder_reset__doc__},

static PyValue
_multibytecodec_MultibyteIncrementalEncoder_reset_impl(MultibyteIncrementalEncoderObject *self);

static PyValue
_multibytecodec_MultibyteIncrementalEncoder_reset(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _multibytecodec_MultibyteIncrementalEncoder_reset_impl((MultibyteIncrementalEncoderObject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_multibytecodec_MultibyteIncrementalDecoder_decode__doc__,
"decode($self, /, input, final=False)\n"
"--\n"
"\n");

#define _MULTIBYTECODEC_MULTIBYTEINCREMENTALDECODER_DECODE_METHODDEF    \
    {"decode", _PyCFunction_CAST(_multibytecodec_MultibyteIncrementalDecoder_decode), METH_VALUE|METH_KEYWORDS, _multibytecodec_MultibyteIncrementalDecoder_decode__doc__},

static PyValue
_multibytecodec_MultibyteIncrementalDecoder_decode_impl(MultibyteIncrementalDecoderObject *self,
                                                        Py_buffer *input,
                                                        int final);

static PyValue
_multibytecodec_MultibyteIncrementalDecoder_decode(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(input)), _PyValue_STATIC_IMMORTAL(&_Py_ID(final)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"input", "final", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "decode",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    Py_buffer input = {NULL, NULL};
    int final = 0;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[_idx[0]]))) {
        PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[_idx[0]]))->tp_name);
        goto exit;
    }
    if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])), &input, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (_idx[1] >= 0) {
        final = _PyValue_IsTrue(PyValue_Borrow(args[_idx[1]]));
        if (final < 0) {
            goto exit;
        }
    }
    return_value = _multibytecodec_MultibyteIncrementalDecoder_decode_impl((MultibyteIncrementalDecoderObject *)PyValue_AsKnownObjectBorrow(self), &input, final);

exit:
    /* Cleanup for input */
    if (input.obj) {
       PyBuffer_Release(&input);
    }

    return return_value;
}

PyDoc_STRVAR(_multibytecodec_MultibyteIncrementalDecoder_getstate__doc__,
"getstate($self, /)\n"
"--\n"
"\n");

#define _MULTIBYTECODEC_MULTIBYTEINCREMENTALDECODER_GETSTATE_METHODDEF    \
    {"getstate", _PyCFunction_CAST(_multibytecodec_MultibyteIncrementalDecoder_getstate), METH_VALUE|METH_NOARGS, _multibytecodec_MultibyteIncrementalDecoder_getstate__doc__},

static PyValue
_multibytecodec_MultibyteIncrementalDecoder_getstate_impl(MultibyteIncrementalDecoderObject *self);

static PyValue
_multibytecodec_MultibyteIncrementalDecoder_getstate(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _multibytecodec_MultibyteIncrementalDecoder_getstate_impl((MultibyteIncrementalDecoderObject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_multibytecodec_MultibyteIncrementalDecoder_setstate__doc__,
"setstate($self, state, /)\n"
"--\n"
"\n");

#define _MULTIBYTECODEC_MULTIBYTEINCREMENTALDECODER_SETSTATE_METHODDEF    \
    {"setstate", _PyCFunction_CAST(_multibytecodec_MultibyteIncrementalDecoder_setstate), METH_VALUE|METH_O, _multibytecodec_MultibyteIncrementalDecoder_setstate__doc__},

static PyValue
_multibytecodec_MultibyteIncrementalDecoder_setstate_impl(MultibyteIncrementalDecoderObject *self,
                                                          PyObject *state);

static PyValue
_multibytecodec_MultibyteIncrementalDecoder_setstate(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *state;

    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[0])), &PyTuple_Type)) {
        _PyArg_BadArgumentValue("setstate", "argument", "tuple", PyValue_Borrow(args[0]));
        goto exit;
    }
    state = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    return_value = _multibytecodec_MultibyteIncrementalDecoder_setstate_impl((MultibyteIncrementalDecoderObject *)PyValue_AsKnownObjectBorrow(self), state);

exit:
    return return_value;
}

PyDoc_STRVAR(_multibytecodec_MultibyteIncrementalDecoder_reset__doc__,
"reset($self, /)\n"
"--\n"
"\n");

#define _MULTIBYTECODEC_MULTIBYTEINCREMENTALDECODER_RESET_METHODDEF    \
    {"reset", _PyCFunction_CAST(_multibytecodec_MultibyteIncrementalDecoder_reset), METH_VALUE|METH_NOARGS, _multibytecodec_MultibyteIncrementalDecoder_reset__doc__},

static PyValue
_multibytecodec_MultibyteIncrementalDecoder_reset_impl(MultibyteIncrementalDecoderObject *self);

static PyValue
_multibytecodec_MultibyteIncrementalDecoder_reset(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _multibytecodec_MultibyteIncrementalDecoder_reset_impl((MultibyteIncrementalDecoderObject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_multibytecodec_MultibyteStreamReader_read__doc__,
"read($self, sizeobj=None, /)\n"
"--\n"
"\n");

#define _MULTIBYTECODEC_MULTIBYTESTREAMREADER_READ_METHODDEF    \
    {"read", _PyCFunction_CAST(_multibytecodec_MultibyteStreamReader_read), METH_VALUE, _multibytecodec_MultibyteStreamReader_read__doc__},

static PyValue
_multibytecodec_MultibyteStreamReader_read_impl(MultibyteStreamReaderObject *self,
                                                PyValueBorrow sizeobj);

static PyValue
_multibytecodec_MultibyteStreamReader_read(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow sizeobj = PyValue_Borrow(PyValue_None);

    if (!_PyArg_CheckPositional("read", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    sizeobj = PyValue_Borrow(args[0]);
skip_optional:
    return_value = _multibytecodec_MultibyteStreamReader_read_impl((MultibyteStreamReaderObject *)PyValue_AsKnownObjectBorrow(self), sizeobj);

exit:
    return return_value;
}

PyDoc_STRVAR(_multibytecodec_MultibyteStreamReader_readline__doc__,
"readline($self, sizeobj=None, /)\n"
"--\n"
"\n");

#define _MULTIBYTECODEC_MULTIBYTESTREAMREADER_READLINE_METHODDEF    \
    {"readline", _PyCFunction_CAST(_multibytecodec_MultibyteStreamReader_readline), METH_VALUE, _multibytecodec_MultibyteStreamReader_readline__doc__},

static PyValue
_multibytecodec_MultibyteStreamReader_readline_impl(MultibyteStreamReaderObject *self,
                                                    PyValueBorrow sizeobj);

static PyValue
_multibytecodec_MultibyteStreamReader_readline(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow sizeobj = PyValue_Borrow(PyValue_None);

    if (!_PyArg_CheckPositional("readline", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    sizeobj = PyValue_Borrow(args[0]);
skip_optional:
    return_value = _multibytecodec_MultibyteStreamReader_readline_impl((MultibyteStreamReaderObject *)PyValue_AsKnownObjectBorrow(self), sizeobj);

exit:
    return return_value;
}

PyDoc_STRVAR(_multibytecodec_MultibyteStreamReader_readlines__doc__,
"readlines($self, sizehintobj=None, /)\n"
"--\n"
"\n");

#define _MULTIBYTECODEC_MULTIBYTESTREAMREADER_READLINES_METHODDEF    \
    {"readlines", _PyCFunction_CAST(_multibytecodec_MultibyteStreamReader_readlines), METH_VALUE, _multibytecodec_MultibyteStreamReader_readlines__doc__},

static PyValue
_multibytecodec_MultibyteStreamReader_readlines_impl(MultibyteStreamReaderObject *self,
                                                     PyValueBorrow sizehintobj);

static PyValue
_multibytecodec_MultibyteStreamReader_readlines(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow sizehintobj = PyValue_Borrow(PyValue_None);

    if (!_PyArg_CheckPositional("readlines", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    sizehintobj = PyValue_Borrow(args[0]);
skip_optional:
    return_value = _multibytecodec_MultibyteStreamReader_readlines_impl((MultibyteStreamReaderObject *)PyValue_AsKnownObjectBorrow(self), sizehintobj);

exit:
    return return_value;
}

PyDoc_STRVAR(_multibytecodec_MultibyteStreamReader_reset__doc__,
"reset($self, /)\n"
"--\n"
"\n");

#define _MULTIBYTECODEC_MULTIBYTESTREAMREADER_RESET_METHODDEF    \
    {"reset", _PyCFunction_CAST(_multibytecodec_MultibyteStreamReader_reset), METH_VALUE|METH_NOARGS, _multibytecodec_MultibyteStreamReader_reset__doc__},

static PyValue
_multibytecodec_MultibyteStreamReader_reset_impl(MultibyteStreamReaderObject *self);

static PyValue
_multibytecodec_MultibyteStreamReader_reset(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _multibytecodec_MultibyteStreamReader_reset_impl((MultibyteStreamReaderObject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_multibytecodec_MultibyteStreamWriter_write__doc__,
"write($self, strobj, /)\n"
"--\n"
"\n");

#define _MULTIBYTECODEC_MULTIBYTESTREAMWRITER_WRITE_METHODDEF    \
    {"write", _PyCFunction_CAST(_multibytecodec_MultibyteStreamWriter_write), METH_VALUE|METH_METHOD|METH_KEYWORDS, _multibytecodec_MultibyteStreamWriter_write__doc__},

static PyValue
_multibytecodec_MultibyteStreamWriter_write_impl(MultibyteStreamWriterObject *self,
                                                 PyTypeObject *cls,
                                                 PyValueBorrow strobj);

static PyValue
_multibytecodec_MultibyteStreamWriter_write(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow strobj;

    if (!_PyArg_NoKwnames("write", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("write", nargs, 1, 1)) {
        goto exit;
    }
    strobj = PyValue_Borrow(args[0]);
    return_value = _multibytecodec_MultibyteStreamWriter_write_impl((MultibyteStreamWriterObject *)PyValue_AsKnownObjectBorrow(self), cls, strobj);

exit:
    return return_value;
}

PyDoc_STRVAR(_multibytecodec_MultibyteStreamWriter_writelines__doc__,
"writelines($self, lines, /)\n"
"--\n"
"\n");

#define _MULTIBYTECODEC_MULTIBYTESTREAMWRITER_WRITELINES_METHODDEF    \
    {"writelines", _PyCFunction_CAST(_multibytecodec_MultibyteStreamWriter_writelines), METH_VALUE|METH_METHOD|METH_KEYWORDS, _multibytecodec_MultibyteStreamWriter_writelines__doc__},

static PyValue
_multibytecodec_MultibyteStreamWriter_writelines_impl(MultibyteStreamWriterObject *self,
                                                      PyTypeObject *cls,
                                                      PyValueBorrow lines);

static PyValue
_multibytecodec_MultibyteStreamWriter_writelines(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow lines;

    if (!_PyArg_NoKwnames("writelines", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("writelines", nargs, 1, 1)) {
        goto exit;
    }
    lines = PyValue_Borrow(args[0]);
    return_value = _multibytecodec_MultibyteStreamWriter_writelines_impl((MultibyteStreamWriterObject *)PyValue_AsKnownObjectBorrow(self), cls, lines);

exit:
    return return_value;
}

PyDoc_STRVAR(_multibytecodec_MultibyteStreamWriter_reset__doc__,
"reset($self, /)\n"
"--\n"
"\n");

#define _MULTIBYTECODEC_MULTIBYTESTREAMWRITER_RESET_METHODDEF    \
    {"reset", _PyCFunction_CAST(_multibytecodec_MultibyteStreamWriter_reset), METH_VALUE|METH_METHOD|METH_KEYWORDS, _multibytecodec_MultibyteStreamWriter_reset__doc__},

static PyValue
_multibytecodec_MultibyteStreamWriter_reset_impl(MultibyteStreamWriterObject *self,
                                                 PyTypeObject *cls);

static PyValue
_multibytecodec_MultibyteStreamWriter_reset(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "reset() takes no arguments");
        goto exit;
    }
    return_value = _multibytecodec_MultibyteStreamWriter_reset_impl((MultibyteStreamWriterObject *)PyValue_AsKnownObjectBorrow(self), cls);

exit:
    return return_value;
}

PyDoc_STRVAR(_multibytecodec___create_codec__doc__,
"__create_codec($module, arg, /)\n"
"--\n"
"\n");

#define _MULTIBYTECODEC___CREATE_CODEC_METHODDEF    \
    {"__create_codec", _PyCFunction_CAST(_multibytecodec___create_codec), METH_VALUE|METH_O, _multibytecodec___create_codec__doc__},

static PyValue
_multibytecodec___create_codec_impl(PyObject *module, PyValueBorrow arg);

static PyValue
_multibytecodec___create_codec(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow arg;

    arg = PyValue_Borrow(args[0]);
    return_value = _multibytecodec___create_codec_impl(PyValue_AsKnownObjectBorrow(module), arg);

    return return_value;
}
/*[clinic end generated code: output=838c9be4a9b90f9d input=a9049054013a1b77]*/
