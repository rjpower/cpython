/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()
#include "pycore_unicodeobject.h" // _PyUnicode_AsUTF8NoNUL()

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
    {"encode", _PyCFunction_CAST(_multibytecodec_MultibyteCodec_encode), METH_FASTCALL|METH_KEYWORDS, _multibytecodec_MultibyteCodec_encode__doc__},

static PyObject *
_multibytecodec_MultibyteCodec_encode_impl(MultibyteCodecObject *self,
                                           PyObject *input,
                                           const char *errors);

static PyValue
_multibytecodec_MultibyteCodec_encode(PyObject *self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(input), &_Py_ID(errors), },
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
    PyValue argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *input;
    const char *errors = NULL;
    PyObject *_return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    input = PyValue_Object(args[0]);
    if (input == NULL) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    {
        PyObject *_arg = PyValue_Object(args[1]);
        if (_arg == NULL) {
            goto exit;
        }
        if (_arg == Py_None) {
            errors = NULL;
        }
        else if (PyUnicode_Check(_arg)) {
            errors = _PyUnicode_AsUTF8NoNUL(_arg);
            if (errors == NULL) {
                goto exit;
            }
        }
        else {
            _PyArg_BadArgument("encode", "argument 'errors'", "str or None", _arg);
            goto exit;
        }
    }
skip_optional_pos:
    _return_value = _multibytecodec_MultibyteCodec_encode_impl((MultibyteCodecObject *)self, input, errors);
    return_value = PyValue_FromResult(_return_value);

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
    {"decode", _PyCFunction_CAST(_multibytecodec_MultibyteCodec_decode), METH_FASTCALL|METH_KEYWORDS, _multibytecodec_MultibyteCodec_decode__doc__},

static PyObject *
_multibytecodec_MultibyteCodec_decode_impl(MultibyteCodecObject *self,
                                           Py_buffer *input,
                                           const char *errors);

static PyValue
_multibytecodec_MultibyteCodec_decode(PyObject *self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(input), &_Py_ID(errors), },
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
    PyValue argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    Py_buffer input = {NULL, NULL};
    const char *errors = NULL;
    PyObject *_return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    {
        PyObject *_arg = PyValue_Object(args[0]);
        if (_arg == NULL) {
            goto exit;
        }
        if (PyObject_GetBuffer(_arg, &input, PyBUF_SIMPLE) != 0) {
            goto exit;
        }
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    {
        PyObject *_arg = PyValue_Object(args[1]);
        if (_arg == NULL) {
            goto exit;
        }
        if (_arg == Py_None) {
            errors = NULL;
        }
        else if (PyUnicode_Check(_arg)) {
            errors = _PyUnicode_AsUTF8NoNUL(_arg);
            if (errors == NULL) {
                goto exit;
            }
        }
        else {
            _PyArg_BadArgument("decode", "argument 'errors'", "str or None", _arg);
            goto exit;
        }
    }
skip_optional_pos:
    _return_value = _multibytecodec_MultibyteCodec_decode_impl((MultibyteCodecObject *)self, &input, errors);
    return_value = PyValue_FromResult(_return_value);

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
    {"encode", _PyCFunction_CAST(_multibytecodec_MultibyteIncrementalEncoder_encode), METH_FASTCALL|METH_KEYWORDS, _multibytecodec_MultibyteIncrementalEncoder_encode__doc__},

static PyObject *
_multibytecodec_MultibyteIncrementalEncoder_encode_impl(MultibyteIncrementalEncoderObject *self,
                                                        PyObject *input,
                                                        int final);

static PyValue
_multibytecodec_MultibyteIncrementalEncoder_encode(PyObject *self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(input), &_Py_ID(final), },
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
    PyValue argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *input;
    int final = 0;
    PyObject *_return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    input = PyValue_Object(args[0]);
    if (input == NULL) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    final = PyObject_IsTrueValue(args[1]);
    if (final < 0) {
        goto exit;
    }
skip_optional_pos:
    _return_value = _multibytecodec_MultibyteIncrementalEncoder_encode_impl((MultibyteIncrementalEncoderObject *)self, input, final);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_multibytecodec_MultibyteIncrementalEncoder_getstate__doc__,
"getstate($self, /)\n"
"--\n"
"\n");

#define _MULTIBYTECODEC_MULTIBYTEINCREMENTALENCODER_GETSTATE_METHODDEF    \
    {"getstate", (PyCFunction)_multibytecodec_MultibyteIncrementalEncoder_getstate, METH_NOARGS, _multibytecodec_MultibyteIncrementalEncoder_getstate__doc__},

static PyObject *
_multibytecodec_MultibyteIncrementalEncoder_getstate_impl(MultibyteIncrementalEncoderObject *self);

static PyObject *
_multibytecodec_MultibyteIncrementalEncoder_getstate(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _multibytecodec_MultibyteIncrementalEncoder_getstate_impl((MultibyteIncrementalEncoderObject *)self);
}

PyDoc_STRVAR(_multibytecodec_MultibyteIncrementalEncoder_setstate__doc__,
"setstate($self, state, /)\n"
"--\n"
"\n");

#define _MULTIBYTECODEC_MULTIBYTEINCREMENTALENCODER_SETSTATE_METHODDEF    \
    {"setstate", _PyCFunction_CAST(_multibytecodec_MultibyteIncrementalEncoder_setstate), METH_O, _multibytecodec_MultibyteIncrementalEncoder_setstate__doc__},

static PyObject *
_multibytecodec_MultibyteIncrementalEncoder_setstate_impl(MultibyteIncrementalEncoderObject *self,
                                                          PyLongObject *statelong);

static PyValue
_multibytecodec_MultibyteIncrementalEncoder_setstate(PyObject *self, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyLongObject *statelong;
    PyObject *_return_value;

    {
        PyObject *_arg = PyValue_Object(arg);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyLong_Check(_arg)) {
            _PyArg_BadArgument("setstate", "argument", "int", _arg);
            goto exit;
        }
        statelong = (PyLongObject *)_arg;
    }
    _return_value = _multibytecodec_MultibyteIncrementalEncoder_setstate_impl((MultibyteIncrementalEncoderObject *)self, statelong);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_multibytecodec_MultibyteIncrementalEncoder_reset__doc__,
"reset($self, /)\n"
"--\n"
"\n");

#define _MULTIBYTECODEC_MULTIBYTEINCREMENTALENCODER_RESET_METHODDEF    \
    {"reset", (PyCFunction)_multibytecodec_MultibyteIncrementalEncoder_reset, METH_NOARGS, _multibytecodec_MultibyteIncrementalEncoder_reset__doc__},

static PyObject *
_multibytecodec_MultibyteIncrementalEncoder_reset_impl(MultibyteIncrementalEncoderObject *self);

static PyObject *
_multibytecodec_MultibyteIncrementalEncoder_reset(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _multibytecodec_MultibyteIncrementalEncoder_reset_impl((MultibyteIncrementalEncoderObject *)self);
}

PyDoc_STRVAR(_multibytecodec_MultibyteIncrementalDecoder_decode__doc__,
"decode($self, /, input, final=False)\n"
"--\n"
"\n");

#define _MULTIBYTECODEC_MULTIBYTEINCREMENTALDECODER_DECODE_METHODDEF    \
    {"decode", _PyCFunction_CAST(_multibytecodec_MultibyteIncrementalDecoder_decode), METH_FASTCALL|METH_KEYWORDS, _multibytecodec_MultibyteIncrementalDecoder_decode__doc__},

static PyObject *
_multibytecodec_MultibyteIncrementalDecoder_decode_impl(MultibyteIncrementalDecoderObject *self,
                                                        Py_buffer *input,
                                                        int final);

static PyValue
_multibytecodec_MultibyteIncrementalDecoder_decode(PyObject *self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(input), &_Py_ID(final), },
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
    PyValue argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    Py_buffer input = {NULL, NULL};
    int final = 0;
    PyObject *_return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    {
        PyObject *_arg = PyValue_Object(args[0]);
        if (_arg == NULL) {
            goto exit;
        }
        if (PyObject_GetBuffer(_arg, &input, PyBUF_SIMPLE) != 0) {
            goto exit;
        }
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    final = PyObject_IsTrueValue(args[1]);
    if (final < 0) {
        goto exit;
    }
skip_optional_pos:
    _return_value = _multibytecodec_MultibyteIncrementalDecoder_decode_impl((MultibyteIncrementalDecoderObject *)self, &input, final);
    return_value = PyValue_FromResult(_return_value);

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
    {"getstate", (PyCFunction)_multibytecodec_MultibyteIncrementalDecoder_getstate, METH_NOARGS, _multibytecodec_MultibyteIncrementalDecoder_getstate__doc__},

static PyObject *
_multibytecodec_MultibyteIncrementalDecoder_getstate_impl(MultibyteIncrementalDecoderObject *self);

static PyObject *
_multibytecodec_MultibyteIncrementalDecoder_getstate(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _multibytecodec_MultibyteIncrementalDecoder_getstate_impl((MultibyteIncrementalDecoderObject *)self);
}

PyDoc_STRVAR(_multibytecodec_MultibyteIncrementalDecoder_setstate__doc__,
"setstate($self, state, /)\n"
"--\n"
"\n");

#define _MULTIBYTECODEC_MULTIBYTEINCREMENTALDECODER_SETSTATE_METHODDEF    \
    {"setstate", _PyCFunction_CAST(_multibytecodec_MultibyteIncrementalDecoder_setstate), METH_O, _multibytecodec_MultibyteIncrementalDecoder_setstate__doc__},

static PyObject *
_multibytecodec_MultibyteIncrementalDecoder_setstate_impl(MultibyteIncrementalDecoderObject *self,
                                                          PyObject *state);

static PyValue
_multibytecodec_MultibyteIncrementalDecoder_setstate(PyObject *self, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyObject *state;
    PyObject *_return_value;

    {
        PyObject *_arg = PyValue_Object(arg);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyTuple_Check(_arg)) {
            _PyArg_BadArgument("setstate", "argument", "tuple", _arg);
            goto exit;
        }
        state = _arg;
    }
    _return_value = _multibytecodec_MultibyteIncrementalDecoder_setstate_impl((MultibyteIncrementalDecoderObject *)self, state);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_multibytecodec_MultibyteIncrementalDecoder_reset__doc__,
"reset($self, /)\n"
"--\n"
"\n");

#define _MULTIBYTECODEC_MULTIBYTEINCREMENTALDECODER_RESET_METHODDEF    \
    {"reset", (PyCFunction)_multibytecodec_MultibyteIncrementalDecoder_reset, METH_NOARGS, _multibytecodec_MultibyteIncrementalDecoder_reset__doc__},

static PyObject *
_multibytecodec_MultibyteIncrementalDecoder_reset_impl(MultibyteIncrementalDecoderObject *self);

static PyObject *
_multibytecodec_MultibyteIncrementalDecoder_reset(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _multibytecodec_MultibyteIncrementalDecoder_reset_impl((MultibyteIncrementalDecoderObject *)self);
}

PyDoc_STRVAR(_multibytecodec_MultibyteStreamReader_read__doc__,
"read($self, sizeobj=None, /)\n"
"--\n"
"\n");

#define _MULTIBYTECODEC_MULTIBYTESTREAMREADER_READ_METHODDEF    \
    {"read", _PyCFunction_CAST(_multibytecodec_MultibyteStreamReader_read), METH_FASTCALL, _multibytecodec_MultibyteStreamReader_read__doc__},

static PyObject *
_multibytecodec_MultibyteStreamReader_read_impl(MultibyteStreamReaderObject *self,
                                                PyObject *sizeobj);

static PyValue
_multibytecodec_MultibyteStreamReader_read(PyObject *self, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    PyObject *sizeobj = Py_None;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("read", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    sizeobj = PyValue_Object(args[0]);
    if (sizeobj == NULL) {
        goto exit;
    }
skip_optional:
    _return_value = _multibytecodec_MultibyteStreamReader_read_impl((MultibyteStreamReaderObject *)self, sizeobj);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_multibytecodec_MultibyteStreamReader_readline__doc__,
"readline($self, sizeobj=None, /)\n"
"--\n"
"\n");

#define _MULTIBYTECODEC_MULTIBYTESTREAMREADER_READLINE_METHODDEF    \
    {"readline", _PyCFunction_CAST(_multibytecodec_MultibyteStreamReader_readline), METH_FASTCALL, _multibytecodec_MultibyteStreamReader_readline__doc__},

static PyObject *
_multibytecodec_MultibyteStreamReader_readline_impl(MultibyteStreamReaderObject *self,
                                                    PyObject *sizeobj);

static PyValue
_multibytecodec_MultibyteStreamReader_readline(PyObject *self, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    PyObject *sizeobj = Py_None;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("readline", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    sizeobj = PyValue_Object(args[0]);
    if (sizeobj == NULL) {
        goto exit;
    }
skip_optional:
    _return_value = _multibytecodec_MultibyteStreamReader_readline_impl((MultibyteStreamReaderObject *)self, sizeobj);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_multibytecodec_MultibyteStreamReader_readlines__doc__,
"readlines($self, sizehintobj=None, /)\n"
"--\n"
"\n");

#define _MULTIBYTECODEC_MULTIBYTESTREAMREADER_READLINES_METHODDEF    \
    {"readlines", _PyCFunction_CAST(_multibytecodec_MultibyteStreamReader_readlines), METH_FASTCALL, _multibytecodec_MultibyteStreamReader_readlines__doc__},

static PyObject *
_multibytecodec_MultibyteStreamReader_readlines_impl(MultibyteStreamReaderObject *self,
                                                     PyObject *sizehintobj);

static PyValue
_multibytecodec_MultibyteStreamReader_readlines(PyObject *self, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    PyObject *sizehintobj = Py_None;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("readlines", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    sizehintobj = PyValue_Object(args[0]);
    if (sizehintobj == NULL) {
        goto exit;
    }
skip_optional:
    _return_value = _multibytecodec_MultibyteStreamReader_readlines_impl((MultibyteStreamReaderObject *)self, sizehintobj);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_multibytecodec_MultibyteStreamReader_reset__doc__,
"reset($self, /)\n"
"--\n"
"\n");

#define _MULTIBYTECODEC_MULTIBYTESTREAMREADER_RESET_METHODDEF    \
    {"reset", (PyCFunction)_multibytecodec_MultibyteStreamReader_reset, METH_NOARGS, _multibytecodec_MultibyteStreamReader_reset__doc__},

static PyObject *
_multibytecodec_MultibyteStreamReader_reset_impl(MultibyteStreamReaderObject *self);

static PyObject *
_multibytecodec_MultibyteStreamReader_reset(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _multibytecodec_MultibyteStreamReader_reset_impl((MultibyteStreamReaderObject *)self);
}

PyDoc_STRVAR(_multibytecodec_MultibyteStreamWriter_write__doc__,
"write($self, strobj, /)\n"
"--\n"
"\n");

#define _MULTIBYTECODEC_MULTIBYTESTREAMWRITER_WRITE_METHODDEF    \
    {"write", _PyCFunction_CAST(_multibytecodec_MultibyteStreamWriter_write), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _multibytecodec_MultibyteStreamWriter_write__doc__},

static PyObject *
_multibytecodec_MultibyteStreamWriter_write_impl(MultibyteStreamWriterObject *self,
                                                 PyTypeObject *cls,
                                                 PyObject *strobj);

static PyValue
_multibytecodec_MultibyteStreamWriter_write(PyObject *self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *strobj;
    PyObject *_return_value;

    if (!_PyArg_NoKwnames("write", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("write", nargs, 1, 1)) {
        goto exit;
    }
    strobj = PyValue_Object(args[0]);
    if (strobj == NULL) {
        goto exit;
    }
    _return_value = _multibytecodec_MultibyteStreamWriter_write_impl((MultibyteStreamWriterObject *)self, cls, strobj);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_multibytecodec_MultibyteStreamWriter_writelines__doc__,
"writelines($self, lines, /)\n"
"--\n"
"\n");

#define _MULTIBYTECODEC_MULTIBYTESTREAMWRITER_WRITELINES_METHODDEF    \
    {"writelines", _PyCFunction_CAST(_multibytecodec_MultibyteStreamWriter_writelines), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _multibytecodec_MultibyteStreamWriter_writelines__doc__},

static PyObject *
_multibytecodec_MultibyteStreamWriter_writelines_impl(MultibyteStreamWriterObject *self,
                                                      PyTypeObject *cls,
                                                      PyObject *lines);

static PyValue
_multibytecodec_MultibyteStreamWriter_writelines(PyObject *self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *lines;
    PyObject *_return_value;

    if (!_PyArg_NoKwnames("writelines", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("writelines", nargs, 1, 1)) {
        goto exit;
    }
    lines = PyValue_Object(args[0]);
    if (lines == NULL) {
        goto exit;
    }
    _return_value = _multibytecodec_MultibyteStreamWriter_writelines_impl((MultibyteStreamWriterObject *)self, cls, lines);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_multibytecodec_MultibyteStreamWriter_reset__doc__,
"reset($self, /)\n"
"--\n"
"\n");

#define _MULTIBYTECODEC_MULTIBYTESTREAMWRITER_RESET_METHODDEF    \
    {"reset", _PyCFunction_CAST(_multibytecodec_MultibyteStreamWriter_reset), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _multibytecodec_MultibyteStreamWriter_reset__doc__},

static PyObject *
_multibytecodec_MultibyteStreamWriter_reset_impl(MultibyteStreamWriterObject *self,
                                                 PyTypeObject *cls);

static PyValue
_multibytecodec_MultibyteStreamWriter_reset(PyObject *self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "reset() takes no arguments");
        return PyValue_NULL;
    }
    return PyValue_FromResult(_multibytecodec_MultibyteStreamWriter_reset_impl((MultibyteStreamWriterObject *)self, cls));
}

PyDoc_STRVAR(_multibytecodec___create_codec__doc__,
"__create_codec($module, arg, /)\n"
"--\n"
"\n");

#define _MULTIBYTECODEC___CREATE_CODEC_METHODDEF    \
    {"__create_codec", _PyCFunction_CAST(_multibytecodec___create_codec), METH_O, _multibytecodec___create_codec__doc__},

static PyObject *
_multibytecodec___create_codec_impl(PyObject *module, PyObject *arg);

static PyValue
_multibytecodec___create_codec(PyObject *module, PyValue arg_)
{
    PyValue return_value = PyValue_NULL;
    PyObject *arg;
    PyObject *_return_value;

    arg = PyValue_Object(arg_);
    if (arg == NULL) {
        goto exit;
    }
    _return_value = _multibytecodec___create_codec_impl(module, arg);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}
/*[clinic end generated code: output=293acfa3fb6180a6 input=a9049054013a1b77]*/
