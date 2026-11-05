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
#include "pycore_critical_section.h"// Py_BEGIN_CRITICAL_SECTION()
#include "pycore_long_value.h"    // _PyValue_AsInt()
#include "pycore_modsupport.h"    // _PyArg_NoKwnames()
#include "pycore_tuple.h"         // _PyTuple_ITEMS()
#include "pycore_unicodeobject.h" // _PyUnicode_AsUTF8NoNUL()
#include "pycore_value_call.h"    // METH_VALUE
#include "pycore_value_ops.h"     // _PyValue_IsTrue()

PyDoc_STRVAR(_io__TextIOBase_detach__doc__,
"detach($self, /)\n"
"--\n"
"\n"
"Separate the underlying buffer from the TextIOBase and return it.\n"
"\n"
"After the underlying buffer has been detached, the TextIO is in\n"
"an unusable state.");

#define _IO__TEXTIOBASE_DETACH_METHODDEF    \
    {"detach", _PyCFunction_CAST(_io__TextIOBase_detach), METH_VALUE|METH_METHOD, _io__TextIOBase_detach__doc__},

static PyValue
_io__TextIOBase_detach_impl(PyObject *self, PyTypeObject *cls);

static PyValue
_io__TextIOBase_detach(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "detach() takes no arguments");
        goto exit;
    }
    return_value = _io__TextIOBase_detach_impl(PyValue_AsKnownObjectBorrow(self), cls);

exit:
    return return_value;
}

PyDoc_STRVAR(_io__TextIOBase_read__doc__,
"read($self, size=-1, /)\n"
"--\n"
"\n"
"Read at most size characters from stream.\n"
"\n"
"Read from underlying buffer until we have size characters or we hit\n"
"EOF.  If size is negative or omitted, read until EOF.");

#define _IO__TEXTIOBASE_READ_METHODDEF    \
    {"read", _PyCFunction_CAST(_io__TextIOBase_read), METH_VALUE|METH_METHOD, _io__TextIOBase_read__doc__},

static PyValue
_io__TextIOBase_read_impl(PyObject *self, PyTypeObject *cls,
                          int Py_UNUSED(size));

static PyValue
_io__TextIOBase_read(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int size = -1;

    if (!_PyArg_NoKwnames("read", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("read", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    size = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (size == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = _io__TextIOBase_read_impl(PyValue_AsKnownObjectBorrow(self), cls, size);

exit:
    return return_value;
}

PyDoc_STRVAR(_io__TextIOBase_readline__doc__,
"readline($self, size=-1, /)\n"
"--\n"
"\n"
"Read until newline or EOF.\n"
"\n"
"Return an empty string if EOF is hit immediately.\n"
"If size is specified, at most size characters will be read.");

#define _IO__TEXTIOBASE_READLINE_METHODDEF    \
    {"readline", _PyCFunction_CAST(_io__TextIOBase_readline), METH_VALUE|METH_METHOD, _io__TextIOBase_readline__doc__},

static PyValue
_io__TextIOBase_readline_impl(PyObject *self, PyTypeObject *cls,
                              int Py_UNUSED(size));

static PyValue
_io__TextIOBase_readline(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int size = -1;

    if (!_PyArg_NoKwnames("readline", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("readline", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    size = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (size == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = _io__TextIOBase_readline_impl(PyValue_AsKnownObjectBorrow(self), cls, size);

exit:
    return return_value;
}

PyDoc_STRVAR(_io__TextIOBase_write__doc__,
"write($self, s, /)\n"
"--\n"
"\n"
"Write string s to stream.\n"
"\n"
"Return the number of characters written\n"
"(which is always equal to the length of the string).");

#define _IO__TEXTIOBASE_WRITE_METHODDEF    \
    {"write", _PyCFunction_CAST(_io__TextIOBase_write), METH_VALUE|METH_METHOD, _io__TextIOBase_write__doc__},

static PyValue
_io__TextIOBase_write_impl(PyObject *self, PyTypeObject *cls,
                           const char *Py_UNUSED(s));

static PyValue
_io__TextIOBase_write(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    const char *s;

    if (!_PyArg_NoKwnames("write", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("write", nargs, 1, 1)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("write", "argument 1", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("write", "argument 1", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    s = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
    if (s == NULL) {
        goto exit;
    }
    return_value = _io__TextIOBase_write_impl(PyValue_AsKnownObjectBorrow(self), cls, s);

exit:
    return return_value;
}

PyDoc_STRVAR(_io__TextIOBase_encoding__doc__,
"Encoding of the text stream.\n"
"\n"
"Subclasses should override.");

static PyObject *
_io__TextIOBase_encoding_get_impl(PyObject *self);

static PyObject *
_io__TextIOBase_encoding_get(PyObject *self, void *Py_UNUSED(context))
{
    return _io__TextIOBase_encoding_get_impl(self);
}

PyDoc_STRVAR(_io__TextIOBase_newlines__doc__,
"Line endings translated so far.\n"
"\n"
"Only line endings translated during reading are considered.\n"
"\n"
"Subclasses should override.");

static PyObject *
_io__TextIOBase_newlines_get_impl(PyObject *self);

static PyObject *
_io__TextIOBase_newlines_get(PyObject *self, void *Py_UNUSED(context))
{
    return _io__TextIOBase_newlines_get_impl(self);
}

PyDoc_STRVAR(_io__TextIOBase_errors__doc__,
"The error setting of the decoder or encoder.\n"
"\n"
"Subclasses should override.");

static PyObject *
_io__TextIOBase_errors_get_impl(PyObject *self);

static PyObject *
_io__TextIOBase_errors_get(PyObject *self, void *Py_UNUSED(context))
{
    return _io__TextIOBase_errors_get_impl(self);
}

PyDoc_STRVAR(_io_IncrementalNewlineDecoder___init____doc__,
"IncrementalNewlineDecoder(decoder, translate, errors=\'strict\')\n"
"--\n"
"\n"
"Codec used when reading a file in universal newlines mode.\n"
"\n"
"It wraps another incremental decoder, translating \\r\\n and \\r into \\n.\n"
"It also records the types of newlines encountered.  When used with\n"
"translate=False, it ensures that the newline sequence is returned in\n"
"one piece. When used with decoder=None, it expects unicode strings as\n"
"decode input and translates newlines without first invoking an external\n"
"decoder.");

static int
_io_IncrementalNewlineDecoder___init___impl(nldecoder_object *self,
                                            PyValueBorrow decoder,
                                            int translate,
                                            PyValueBorrow errors);

static int
_io_IncrementalNewlineDecoder___init___value(PyObject *self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    int return_value = -1;
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(decoder)), _PyValue_STATIC_IMMORTAL(&_Py_ID(translate)), _PyValue_STATIC_IMMORTAL(&_Py_ID(errors)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"decoder", "translate", "errors", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "IncrementalNewlineDecoder",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    PyValueBorrow decoder;
    int translate;
    PyValueBorrow errors = PyValue_Borrow(PyValue_NULL);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, _idx, 3)) {
        goto exit;
    }
    decoder = PyValue_Borrow(args[_idx[0]]);
    translate = _PyValue_IsTrue(PyValue_Borrow(args[_idx[1]]));
    if (translate < 0) {
        goto exit;
    }
    if (_idx[2] >= 0) {
        errors = PyValue_Borrow(args[_idx[2]]);
    }
    return_value = _io_IncrementalNewlineDecoder___init___impl((nldecoder_object *)self, decoder, translate, errors);

exit:
    return return_value;
}

static int
_io_IncrementalNewlineDecoder___init__(PyObject *self, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _io_IncrementalNewlineDecoder___init___value(self, _PyTuple_ITEMS(args), nargs, NULL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return -1;
    }
    int result = _io_IncrementalNewlineDecoder___init___value(self, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return result;
}

PyDoc_STRVAR(_io_IncrementalNewlineDecoder_decode__doc__,
"decode($self, /, input, final=False)\n"
"--\n"
"\n");

#define _IO_INCREMENTALNEWLINEDECODER_DECODE_METHODDEF    \
    {"decode", _PyCFunction_CAST(_io_IncrementalNewlineDecoder_decode), METH_VALUE, _io_IncrementalNewlineDecoder_decode__doc__},

static PyValue
_io_IncrementalNewlineDecoder_decode_impl(nldecoder_object *self,
                                          PyValueBorrow input, int final);

static PyValue
_io_IncrementalNewlineDecoder_decode(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _io_IncrementalNewlineDecoder_decode_impl((nldecoder_object *)PyValue_AsKnownObjectBorrow(self), input, final);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_io_IncrementalNewlineDecoder_getstate__doc__,
"getstate($self, /)\n"
"--\n"
"\n");

#define _IO_INCREMENTALNEWLINEDECODER_GETSTATE_METHODDEF    \
    {"getstate", _PyCFunction_CAST(_io_IncrementalNewlineDecoder_getstate), METH_VALUE|METH_NOARGS, _io_IncrementalNewlineDecoder_getstate__doc__},

static PyValue
_io_IncrementalNewlineDecoder_getstate_impl(nldecoder_object *self);

static PyValue
_io_IncrementalNewlineDecoder_getstate(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _io_IncrementalNewlineDecoder_getstate_impl((nldecoder_object *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_io_IncrementalNewlineDecoder_setstate__doc__,
"setstate($self, state, /)\n"
"--\n"
"\n");

#define _IO_INCREMENTALNEWLINEDECODER_SETSTATE_METHODDEF    \
    {"setstate", _PyCFunction_CAST(_io_IncrementalNewlineDecoder_setstate), METH_VALUE|METH_O, _io_IncrementalNewlineDecoder_setstate__doc__},

static PyValue
_io_IncrementalNewlineDecoder_setstate_impl(nldecoder_object *self,
                                            PyValueBorrow state);

static PyValue
_io_IncrementalNewlineDecoder_setstate(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow state;

    state = PyValue_Borrow(args[0]);
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _io_IncrementalNewlineDecoder_setstate_impl((nldecoder_object *)PyValue_AsKnownObjectBorrow(self), state);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_io_IncrementalNewlineDecoder_reset__doc__,
"reset($self, /)\n"
"--\n"
"\n");

#define _IO_INCREMENTALNEWLINEDECODER_RESET_METHODDEF    \
    {"reset", _PyCFunction_CAST(_io_IncrementalNewlineDecoder_reset), METH_VALUE|METH_NOARGS, _io_IncrementalNewlineDecoder_reset__doc__},

static PyValue
_io_IncrementalNewlineDecoder_reset_impl(nldecoder_object *self);

static PyValue
_io_IncrementalNewlineDecoder_reset(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _io_IncrementalNewlineDecoder_reset_impl((nldecoder_object *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

static PyObject *
_io_IncrementalNewlineDecoder_newlines_get_impl(nldecoder_object *self);

static PyObject *
_io_IncrementalNewlineDecoder_newlines_get(PyObject *self, void *Py_UNUSED(context))
{
    return _io_IncrementalNewlineDecoder_newlines_get_impl((nldecoder_object *)self);
}

PyDoc_STRVAR(_io_TextIOWrapper___init____doc__,
"TextIOWrapper(buffer, encoding=None, errors=None, newline=None,\n"
"              line_buffering=False, write_through=False)\n"
"--\n"
"\n"
"Character and line based layer over a BufferedIOBase object, buffer.\n"
"\n"
"encoding gives the name of the encoding that the stream will be\n"
"decoded or encoded with. It defaults to locale.getencoding().\n"
"\n"
"errors determines the strictness of encoding and decoding (see\n"
"help(codecs.Codec) or the documentation for codecs.register) and\n"
"defaults to \"strict\".\n"
"\n"
"newline controls how line endings are handled. It can be None, \'\',\n"
"\'\\n\', \'\\r\', and \'\\r\\n\'.  It works as follows:\n"
"\n"
"* On input, if newline is None, universal newlines mode is\n"
"  enabled. Lines in the input can end in \'\\n\', \'\\r\', or \'\\r\\n\', and\n"
"  these are translated into \'\\n\' before being returned to the\n"
"  caller. If it is \'\', universal newline mode is enabled, but line\n"
"  endings are returned to the caller untranslated. If it has any of\n"
"  the other legal values, input lines are only terminated by the given\n"
"  string, and the line ending is returned to the caller untranslated.\n"
"\n"
"* On output, if newline is None, any \'\\n\' characters written are\n"
"  translated to the system default line separator, os.linesep. If\n"
"  newline is \'\' or \'\\n\', no translation takes place. If newline is any\n"
"  of the other legal values, any \'\\n\' characters written are translated\n"
"  to the given string.\n"
"\n"
"If line_buffering is True, a call to flush is implied when a call to\n"
"write contains a newline character.");

static int
_io_TextIOWrapper___init___impl(textio *self, PyValueBorrow buffer,
                                const char *encoding, PyValueBorrow errors,
                                const char *newline, int line_buffering,
                                int write_through);

static int
_io_TextIOWrapper___init___value(PyObject *self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    int return_value = -1;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 6
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(buffer)), _PyValue_STATIC_IMMORTAL(&_Py_ID(encoding)), _PyValue_STATIC_IMMORTAL(&_Py_ID(errors)), _PyValue_STATIC_IMMORTAL(&_Py_ID(newline)), _PyValue_STATIC_IMMORTAL(&_Py_ID(line_buffering)), _PyValue_STATIC_IMMORTAL(&_Py_ID(write_through)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"buffer", "encoding", "errors", "newline", "line_buffering", "write_through", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "TextIOWrapper",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[6];
    PyValueBorrow buffer;
    const char *encoding = NULL;
    PyValueBorrow errors = PyValue_Borrow(PyValue_None);
    const char *newline = NULL;
    int line_buffering = 0;
    int write_through = 0;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 6, /*minkw*/ 0, /*varpos*/ 0, _idx, 6)) {
        goto exit;
    }
    buffer = PyValue_Borrow(args[_idx[0]]);
    if (_idx[1] >= 0) {
        if (!PyValue_IsObject(PyValue_Borrow(args[_idx[1]]))) {
            _PyArg_BadArgumentValue("TextIOWrapper", "argument 'encoding'", "str or None", PyValue_Borrow(args[_idx[1]]));
            goto exit;
        }
        if (PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])) == Py_None) {
            encoding = NULL;
        }
        else if (PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])))) {
            encoding = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])));
            if (encoding == NULL) {
                goto exit;
            }
        }
        else {
            _PyArg_BadArgument("TextIOWrapper", "argument 'encoding'", "str or None", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])));
            goto exit;
        }
    }
    if (_idx[2] >= 0) {
        errors = PyValue_Borrow(args[_idx[2]]);
    }
    if (_idx[3] >= 0) {
        if (!PyValue_IsObject(PyValue_Borrow(args[_idx[3]]))) {
            _PyArg_BadArgumentValue("TextIOWrapper", "argument 'newline'", "str or None", PyValue_Borrow(args[_idx[3]]));
            goto exit;
        }
        if (PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[3]])) == Py_None) {
            newline = NULL;
        }
        else if (PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[3]])))) {
            newline = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[3]])));
            if (newline == NULL) {
                goto exit;
            }
        }
        else {
            _PyArg_BadArgument("TextIOWrapper", "argument 'newline'", "str or None", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[3]])));
            goto exit;
        }
    }
    if (_idx[4] >= 0) {
        line_buffering = _PyValue_IsTrue(PyValue_Borrow(args[_idx[4]]));
        if (line_buffering < 0) {
            goto exit;
        }
    }
    if (_idx[5] >= 0) {
        write_through = _PyValue_IsTrue(PyValue_Borrow(args[_idx[5]]));
        if (write_through < 0) {
            goto exit;
        }
    }
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _io_TextIOWrapper___init___impl((textio *)self, buffer, encoding, errors, newline, line_buffering, write_through);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

static int
_io_TextIOWrapper___init__(PyObject *self, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _io_TextIOWrapper___init___value(self, _PyTuple_ITEMS(args), nargs, NULL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return -1;
    }
    int result = _io_TextIOWrapper___init___value(self, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return result;
}

PyDoc_STRVAR(_io_TextIOWrapper_reconfigure__doc__,
"reconfigure($self, /, *, encoding=None, errors=None, newline=None,\n"
"            line_buffering=None, write_through=None)\n"
"--\n"
"\n"
"Reconfigure the text stream with new parameters.\n"
"\n"
"This also does an implicit stream flush.");

#define _IO_TEXTIOWRAPPER_RECONFIGURE_METHODDEF    \
    {"reconfigure", _PyCFunction_CAST(_io_TextIOWrapper_reconfigure), METH_VALUE, _io_TextIOWrapper_reconfigure__doc__},

static PyValue
_io_TextIOWrapper_reconfigure_impl(textio *self, PyValueBorrow encoding,
                                   PyValueBorrow errors,
                                   PyValueBorrow newline_obj,
                                   PyValueBorrow line_buffering_obj,
                                   PyValueBorrow write_through_obj);

static PyValue
_io_TextIOWrapper_reconfigure(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(encoding)), _PyValue_STATIC_IMMORTAL(&_Py_ID(errors)), _PyValue_STATIC_IMMORTAL(&_Py_ID(newline)), _PyValue_STATIC_IMMORTAL(&_Py_ID(line_buffering)), _PyValue_STATIC_IMMORTAL(&_Py_ID(write_through)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"encoding", "errors", "newline", "line_buffering", "write_through", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "reconfigure",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[5];
    PyValueBorrow encoding = PyValue_Borrow(PyValue_None);
    PyValueBorrow errors = PyValue_Borrow(PyValue_None);
    PyValueBorrow newline_obj = PyValue_Borrow(PyValue_NULL);
    PyValueBorrow line_buffering_obj = PyValue_Borrow(PyValue_None);
    PyValueBorrow write_through_obj = PyValue_Borrow(PyValue_None);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 0, /*minkw*/ 0, /*varpos*/ 0, _idx, 5)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        encoding = PyValue_Borrow(args[_idx[0]]);
    }
    if (_idx[1] >= 0) {
        errors = PyValue_Borrow(args[_idx[1]]);
    }
    if (_idx[2] >= 0) {
        newline_obj = PyValue_Borrow(args[_idx[2]]);
    }
    if (_idx[3] >= 0) {
        line_buffering_obj = PyValue_Borrow(args[_idx[3]]);
    }
    if (_idx[4] >= 0) {
        write_through_obj = PyValue_Borrow(args[_idx[4]]);
    }
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _io_TextIOWrapper_reconfigure_impl((textio *)PyValue_AsKnownObjectBorrow(self), encoding, errors, newline_obj, line_buffering_obj, write_through_obj);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_io_TextIOWrapper_detach__doc__,
"detach($self, /)\n"
"--\n"
"\n");

#define _IO_TEXTIOWRAPPER_DETACH_METHODDEF    \
    {"detach", _PyCFunction_CAST(_io_TextIOWrapper_detach), METH_VALUE|METH_NOARGS, _io_TextIOWrapper_detach__doc__},

static PyValue
_io_TextIOWrapper_detach_impl(textio *self);

static PyValue
_io_TextIOWrapper_detach(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _io_TextIOWrapper_detach_impl((textio *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_io_TextIOWrapper_write__doc__,
"write($self, text, /)\n"
"--\n"
"\n");

#define _IO_TEXTIOWRAPPER_WRITE_METHODDEF    \
    {"write", _PyCFunction_CAST(_io_TextIOWrapper_write), METH_VALUE|METH_O, _io_TextIOWrapper_write__doc__},

static PyValue
_io_TextIOWrapper_write_impl(textio *self, PyObject *text);

static PyValue
_io_TextIOWrapper_write(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *text;

    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("write", "argument", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("write", "argument", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    text = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _io_TextIOWrapper_write_impl((textio *)PyValue_AsKnownObjectBorrow(self), text);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_io_TextIOWrapper_read__doc__,
"read($self, size=-1, /)\n"
"--\n"
"\n");

#define _IO_TEXTIOWRAPPER_READ_METHODDEF    \
    {"read", _PyCFunction_CAST(_io_TextIOWrapper_read), METH_VALUE, _io_TextIOWrapper_read__doc__},

static PyValue
_io_TextIOWrapper_read_impl(textio *self, Py_ssize_t n);

static PyValue
_io_TextIOWrapper_read(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_ssize_t n = -1;

    if (!_PyArg_NoKwnamesValue("TextIOWrapper.read()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("read", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    if (!_Py_convert_optional_to_ssize_t_value(PyValue_Borrow(args[0]), &n)) {
        goto exit;
    }
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _io_TextIOWrapper_read_impl((textio *)PyValue_AsKnownObjectBorrow(self), n);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_io_TextIOWrapper_readline__doc__,
"readline($self, size=-1, /)\n"
"--\n"
"\n");

#define _IO_TEXTIOWRAPPER_READLINE_METHODDEF    \
    {"readline", _PyCFunction_CAST(_io_TextIOWrapper_readline), METH_VALUE, _io_TextIOWrapper_readline__doc__},

static PyValue
_io_TextIOWrapper_readline_impl(textio *self, Py_ssize_t size);

static PyValue
_io_TextIOWrapper_readline(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_ssize_t size = -1;

    if (!_PyArg_NoKwnamesValue("TextIOWrapper.readline()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("readline", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    {
        Py_ssize_t ival = -1;
        PyValue iobj = _PyValue_Index(PyValue_Borrow(args[0]));
        if (!PyValue_IsNull(iobj)) {
            ival = _PyValue_IntAsSsize_t(PyValue_Borrow(iobj));
            PyValue_CLOSE(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        size = ival;
    }
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _io_TextIOWrapper_readline_impl((textio *)PyValue_AsKnownObjectBorrow(self), size);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_io_TextIOWrapper_seek__doc__,
"seek($self, cookie, whence=os.SEEK_SET, /)\n"
"--\n"
"\n"
"Set the stream position, and return the new stream position.\n"
"\n"
"  cookie\n"
"    Zero or an opaque number returned by tell().\n"
"  whence\n"
"    The relative position to seek from.\n"
"\n"
"Four operations are supported, given by the following argument\n"
"combinations:\n"
"\n"
"- seek(0, SEEK_SET): Rewind to the start of the stream.\n"
"- seek(cookie, SEEK_SET): Restore a previous position;\n"
"  \'cookie\' must be a number returned by tell().\n"
"- seek(0, SEEK_END): Fast-forward to the end of the stream.\n"
"- seek(0, SEEK_CUR): Leave the current stream position unchanged.\n"
"\n"
"Any other argument combinations are invalid,\n"
"and may raise exceptions.");

#define _IO_TEXTIOWRAPPER_SEEK_METHODDEF    \
    {"seek", _PyCFunction_CAST(_io_TextIOWrapper_seek), METH_VALUE, _io_TextIOWrapper_seek__doc__},

static PyValue
_io_TextIOWrapper_seek_impl(textio *self, PyValueBorrow cookieObj,
                            int whence);

static PyValue
_io_TextIOWrapper_seek(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow cookieObj;
    int whence = 0;

    if (!_PyArg_NoKwnamesValue("TextIOWrapper.seek()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("seek", nargs, 1, 2)) {
        goto exit;
    }
    cookieObj = PyValue_Borrow(args[0]);
    if (nargs < 2) {
        goto skip_optional;
    }
    whence = _PyValue_AsInt(PyValue_Borrow(args[1]));
    if (whence == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _io_TextIOWrapper_seek_impl((textio *)PyValue_AsKnownObjectBorrow(self), cookieObj, whence);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_io_TextIOWrapper_tell__doc__,
"tell($self, /)\n"
"--\n"
"\n"
"Return the stream position as an opaque number.\n"
"\n"
"The return value of tell() can be given as input to seek(), to\n"
"restore a previous stream position.");

#define _IO_TEXTIOWRAPPER_TELL_METHODDEF    \
    {"tell", _PyCFunction_CAST(_io_TextIOWrapper_tell), METH_VALUE|METH_NOARGS, _io_TextIOWrapper_tell__doc__},

static PyValue
_io_TextIOWrapper_tell_impl(textio *self);

static PyValue
_io_TextIOWrapper_tell(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _io_TextIOWrapper_tell_impl((textio *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_io_TextIOWrapper_truncate__doc__,
"truncate($self, pos=None, /)\n"
"--\n"
"\n");

#define _IO_TEXTIOWRAPPER_TRUNCATE_METHODDEF    \
    {"truncate", _PyCFunction_CAST(_io_TextIOWrapper_truncate), METH_VALUE, _io_TextIOWrapper_truncate__doc__},

static PyValue
_io_TextIOWrapper_truncate_impl(textio *self, PyValueBorrow pos);

static PyValue
_io_TextIOWrapper_truncate(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow pos = PyValue_Borrow(PyValue_None);

    if (!_PyArg_NoKwnamesValue("TextIOWrapper.truncate()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("truncate", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    pos = PyValue_Borrow(args[0]);
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _io_TextIOWrapper_truncate_impl((textio *)PyValue_AsKnownObjectBorrow(self), pos);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_io_TextIOWrapper_fileno__doc__,
"fileno($self, /)\n"
"--\n"
"\n");

#define _IO_TEXTIOWRAPPER_FILENO_METHODDEF    \
    {"fileno", _PyCFunction_CAST(_io_TextIOWrapper_fileno), METH_VALUE|METH_NOARGS, _io_TextIOWrapper_fileno__doc__},

static PyValue
_io_TextIOWrapper_fileno_impl(textio *self);

static PyValue
_io_TextIOWrapper_fileno(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _io_TextIOWrapper_fileno_impl((textio *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_io_TextIOWrapper_seekable__doc__,
"seekable($self, /)\n"
"--\n"
"\n");

#define _IO_TEXTIOWRAPPER_SEEKABLE_METHODDEF    \
    {"seekable", _PyCFunction_CAST(_io_TextIOWrapper_seekable), METH_VALUE|METH_NOARGS, _io_TextIOWrapper_seekable__doc__},

static PyValue
_io_TextIOWrapper_seekable_impl(textio *self);

static PyValue
_io_TextIOWrapper_seekable(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _io_TextIOWrapper_seekable_impl((textio *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_io_TextIOWrapper_readable__doc__,
"readable($self, /)\n"
"--\n"
"\n");

#define _IO_TEXTIOWRAPPER_READABLE_METHODDEF    \
    {"readable", _PyCFunction_CAST(_io_TextIOWrapper_readable), METH_VALUE|METH_NOARGS, _io_TextIOWrapper_readable__doc__},

static PyValue
_io_TextIOWrapper_readable_impl(textio *self);

static PyValue
_io_TextIOWrapper_readable(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _io_TextIOWrapper_readable_impl((textio *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_io_TextIOWrapper_writable__doc__,
"writable($self, /)\n"
"--\n"
"\n");

#define _IO_TEXTIOWRAPPER_WRITABLE_METHODDEF    \
    {"writable", _PyCFunction_CAST(_io_TextIOWrapper_writable), METH_VALUE|METH_NOARGS, _io_TextIOWrapper_writable__doc__},

static PyValue
_io_TextIOWrapper_writable_impl(textio *self);

static PyValue
_io_TextIOWrapper_writable(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _io_TextIOWrapper_writable_impl((textio *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_io_TextIOWrapper_isatty__doc__,
"isatty($self, /)\n"
"--\n"
"\n");

#define _IO_TEXTIOWRAPPER_ISATTY_METHODDEF    \
    {"isatty", _PyCFunction_CAST(_io_TextIOWrapper_isatty), METH_VALUE|METH_NOARGS, _io_TextIOWrapper_isatty__doc__},

static PyValue
_io_TextIOWrapper_isatty_impl(textio *self);

static PyValue
_io_TextIOWrapper_isatty(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _io_TextIOWrapper_isatty_impl((textio *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_io_TextIOWrapper_flush__doc__,
"flush($self, /)\n"
"--\n"
"\n");

#define _IO_TEXTIOWRAPPER_FLUSH_METHODDEF    \
    {"flush", _PyCFunction_CAST(_io_TextIOWrapper_flush), METH_VALUE|METH_NOARGS, _io_TextIOWrapper_flush__doc__},

static PyValue
_io_TextIOWrapper_flush_impl(textio *self);

static PyValue
_io_TextIOWrapper_flush(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _io_TextIOWrapper_flush_impl((textio *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_io_TextIOWrapper_close__doc__,
"close($self, /)\n"
"--\n"
"\n");

#define _IO_TEXTIOWRAPPER_CLOSE_METHODDEF    \
    {"close", _PyCFunction_CAST(_io_TextIOWrapper_close), METH_VALUE|METH_NOARGS, _io_TextIOWrapper_close__doc__},

static PyValue
_io_TextIOWrapper_close_impl(textio *self);

static PyValue
_io_TextIOWrapper_close(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _io_TextIOWrapper_close_impl((textio *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

static PyObject *
textiowrapper_iternext_impl(textio *self);

static PyObject *
textiowrapper_iternext(PyObject *self)
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = textiowrapper_iternext_impl((textio *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

static PyObject *
_io_TextIOWrapper_name_get_impl(textio *self);

static PyObject *
_io_TextIOWrapper_name_get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _io_TextIOWrapper_name_get_impl((textio *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

static PyObject *
_io_TextIOWrapper_closed_get_impl(textio *self);

static PyObject *
_io_TextIOWrapper_closed_get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _io_TextIOWrapper_closed_get_impl((textio *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

static PyObject *
_io_TextIOWrapper_newlines_get_impl(textio *self);

static PyObject *
_io_TextIOWrapper_newlines_get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _io_TextIOWrapper_newlines_get_impl((textio *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

static PyObject *
_io_TextIOWrapper_errors_get_impl(textio *self);

static PyObject *
_io_TextIOWrapper_errors_get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _io_TextIOWrapper_errors_get_impl((textio *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

static Py_ssize_t
_io_TextIOWrapper__CHUNK_SIZE_get_impl(textio *self);

static PyObject *
_io_TextIOWrapper__CHUNK_SIZE_get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;
    Py_ssize_t _return_value;

    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = _io_TextIOWrapper__CHUNK_SIZE_get_impl((textio *)self);
    Py_END_CRITICAL_SECTION();
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromSsize_t(_return_value);

exit:
    return return_value;
}

static int
_io_TextIOWrapper__CHUNK_SIZE_set_impl(textio *self, Py_ssize_t value);

static int
_io_TextIOWrapper__CHUNK_SIZE_set(PyObject *self, PyObject *arg, void *Py_UNUSED(context))
{
    int return_value = -1;
    Py_ssize_t value;

    if (arg == NULL) {
        PyErr_Format(PyExc_AttributeError,
                     "attribute '_CHUNK_SIZE' of '%.100s' objects cannot be deleted",
                     Py_TYPE(self)->tp_name);
        return -1;
    }
    {
        Py_ssize_t ival = -1;
        PyValue iobj = _PyValue_Index(PyValue_FromObjectBorrow(arg));
        if (!PyValue_IsNull(iobj)) {
            ival = _PyValue_IntAsSsize_t(PyValue_Borrow(iobj));
            PyValue_CLOSE(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        value = ival;
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _io_TextIOWrapper__CHUNK_SIZE_set_impl((textio *)self, value);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

static PyObject *
_io_TextIOWrapper_buffer_get_impl(textio *self);

static PyObject *
_io_TextIOWrapper_buffer_get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _io_TextIOWrapper_buffer_get_impl((textio *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}
#define _IO__TEXTIOBASE_ENCODING_GETSETDEF {"encoding", (getter)_io__TextIOBase_encoding_get, (setter)NULL, _io__TextIOBase_encoding__doc__},

#define _IO__TEXTIOBASE_NEWLINES_GETSETDEF {"newlines", (getter)_io__TextIOBase_newlines_get, (setter)NULL, _io__TextIOBase_newlines__doc__},

#define _IO__TEXTIOBASE_ERRORS_GETSETDEF {"errors", (getter)_io__TextIOBase_errors_get, (setter)NULL, _io__TextIOBase_errors__doc__},

#define _IO_INCREMENTALNEWLINEDECODER_NEWLINES_GETSETDEF {"newlines", (getter)_io_IncrementalNewlineDecoder_newlines_get, (setter)NULL, NULL},

#define _IO_TEXTIOWRAPPER_NAME_GETSETDEF {"name", (getter)_io_TextIOWrapper_name_get, (setter)NULL, NULL},

#define _IO_TEXTIOWRAPPER_CLOSED_GETSETDEF {"closed", (getter)_io_TextIOWrapper_closed_get, (setter)NULL, NULL},

#define _IO_TEXTIOWRAPPER_NEWLINES_GETSETDEF {"newlines", (getter)_io_TextIOWrapper_newlines_get, (setter)NULL, NULL},

#define _IO_TEXTIOWRAPPER_ERRORS_GETSETDEF {"errors", (getter)_io_TextIOWrapper_errors_get, (setter)NULL, NULL},

#define _IO_TEXTIOWRAPPER__CHUNK_SIZE_GETSETDEF {"_CHUNK_SIZE", (getter)_io_TextIOWrapper__CHUNK_SIZE_get, (setter)_io_TextIOWrapper__CHUNK_SIZE_set, NULL},

#define _IO_TEXTIOWRAPPER_BUFFER_GETSETDEF {"buffer", (getter)_io_TextIOWrapper_buffer_get, (setter)NULL, NULL},

/*[clinic end generated code: output=1e9725511fb7c6c4 input=a9049054013a1b77]*/
