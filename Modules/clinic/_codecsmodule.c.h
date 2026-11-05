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
#include "pycore_long_value.h"    // _PyValue_AsInt()
#include "pycore_modsupport.h"    // _PyArg_BadArgument()
#include "pycore_unicodeobject.h" // _PyUnicode_AsUTF8NoNUL()
#include "pycore_value_call.h"    // METH_VALUE
#include "pycore_value_ops.h"     // _PyValue_IsTrue()

PyDoc_STRVAR(_codecs_register__doc__,
"register($module, search_function, /)\n"
"--\n"
"\n"
"Register a codec search function.\n"
"\n"
"Search functions are expected to take one argument, the encoding\n"
"name in all lower case letters, and either return None, or a tuple\n"
"of functions (encoder, decoder, stream_reader, stream_writer) (or\n"
"a CodecInfo object).");

#define _CODECS_REGISTER_METHODDEF    \
    {"register", _PyCFunction_CAST(_codecs_register), METH_VALUE|METH_O, _codecs_register__doc__},

static PyValue
_codecs_register_impl(PyObject *module, PyValueBorrow search_function);

static PyValue
_codecs_register(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow search_function;

    search_function = PyValue_Borrow(args[0]);
    return_value = _codecs_register_impl(PyValue_AsKnownObjectBorrow(module), search_function);

    return return_value;
}

PyDoc_STRVAR(_codecs_unregister__doc__,
"unregister($module, search_function, /)\n"
"--\n"
"\n"
"Unregister a codec search function and clear the registry\'s cache.\n"
"\n"
"If the search function is not registered, do nothing.");

#define _CODECS_UNREGISTER_METHODDEF    \
    {"unregister", _PyCFunction_CAST(_codecs_unregister), METH_VALUE|METH_O, _codecs_unregister__doc__},

static PyValue
_codecs_unregister_impl(PyObject *module, PyValueBorrow search_function);

static PyValue
_codecs_unregister(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow search_function;

    search_function = PyValue_Borrow(args[0]);
    return_value = _codecs_unregister_impl(PyValue_AsKnownObjectBorrow(module), search_function);

    return return_value;
}

PyDoc_STRVAR(_codecs_lookup__doc__,
"lookup($module, encoding, /)\n"
"--\n"
"\n"
"Looks up a codec tuple in the Python codec registry and returns a CodecInfo object.");

#define _CODECS_LOOKUP_METHODDEF    \
    {"lookup", _PyCFunction_CAST(_codecs_lookup), METH_VALUE|METH_O, _codecs_lookup__doc__},

static PyValue
_codecs_lookup_impl(PyObject *module, const char *encoding);

static PyValue
_codecs_lookup(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    const char *encoding;

    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("lookup", "argument", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("lookup", "argument", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    encoding = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
    if (encoding == NULL) {
        goto exit;
    }
    return_value = _codecs_lookup_impl(PyValue_AsKnownObjectBorrow(module), encoding);

exit:
    return return_value;
}

PyDoc_STRVAR(_codecs_encode__doc__,
"encode($module, /, obj, encoding=\'utf-8\', errors=\'strict\')\n"
"--\n"
"\n"
"Encodes obj using the codec registered for encoding.\n"
"\n"
"The default encoding is \'utf-8\'.  errors may be given to set a\n"
"different error handling scheme.  Default is \'strict\' meaning that\n"
"encoding errors raise a ValueError.  Other possible values are \'ignore\',\n"
"\'replace\' and \'backslashreplace\' as well as any other name registered\n"
"with codecs.register_error that can handle ValueErrors.");

#define _CODECS_ENCODE_METHODDEF    \
    {"encode", _PyCFunction_CAST(_codecs_encode), METH_VALUE|METH_KEYWORDS, _codecs_encode__doc__},

static PyValue
_codecs_encode_impl(PyObject *module, PyValueBorrow obj,
                    const char *encoding, const char *errors);

static PyValue
_codecs_encode(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(obj)), _PyValue_STATIC_IMMORTAL(&_Py_ID(encoding)), _PyValue_STATIC_IMMORTAL(&_Py_ID(errors)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"obj", "encoding", "errors", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "encode",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    PyValueBorrow obj;
    const char *encoding = NULL;
    const char *errors = NULL;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, _idx, 3)) {
        goto exit;
    }
    obj = PyValue_Borrow(args[_idx[0]]);
    if (_idx[1] >= 0) {
        if (!PyValue_IsObject(PyValue_Borrow(args[_idx[1]]))) {
            _PyArg_BadArgumentValue("encode", "argument 'encoding'", "str", PyValue_Borrow(args[_idx[1]]));
            goto exit;
        }
        if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])))) {
            _PyArg_BadArgument("encode", "argument 'encoding'", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])));
            goto exit;
        }
        encoding = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])));
        if (encoding == NULL) {
            goto exit;
        }
    }
    if (_idx[2] >= 0) {
        if (!PyValue_IsObject(PyValue_Borrow(args[_idx[2]]))) {
            _PyArg_BadArgumentValue("encode", "argument 'errors'", "str", PyValue_Borrow(args[_idx[2]]));
            goto exit;
        }
        if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[2]])))) {
            _PyArg_BadArgument("encode", "argument 'errors'", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[2]])));
            goto exit;
        }
        errors = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[2]])));
        if (errors == NULL) {
            goto exit;
        }
    }
    return_value = _codecs_encode_impl(PyValue_AsKnownObjectBorrow(module), obj, encoding, errors);

exit:
    return return_value;
}

PyDoc_STRVAR(_codecs_decode__doc__,
"decode($module, /, obj, encoding=\'utf-8\', errors=\'strict\')\n"
"--\n"
"\n"
"Decodes obj using the codec registered for encoding.\n"
"\n"
"Default encoding is \'utf-8\'.  errors may be given to set a\n"
"different error handling scheme.  Default is \'strict\' meaning that\n"
"encoding errors raise a ValueError.  Other possible values are \'ignore\',\n"
"\'replace\' and \'backslashreplace\' as well as any other name registered\n"
"with codecs.register_error that can handle ValueErrors.");

#define _CODECS_DECODE_METHODDEF    \
    {"decode", _PyCFunction_CAST(_codecs_decode), METH_VALUE|METH_KEYWORDS, _codecs_decode__doc__},

static PyValue
_codecs_decode_impl(PyObject *module, PyValueBorrow obj,
                    const char *encoding, const char *errors);

static PyValue
_codecs_decode(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(obj)), _PyValue_STATIC_IMMORTAL(&_Py_ID(encoding)), _PyValue_STATIC_IMMORTAL(&_Py_ID(errors)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"obj", "encoding", "errors", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "decode",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    PyValueBorrow obj;
    const char *encoding = NULL;
    const char *errors = NULL;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, _idx, 3)) {
        goto exit;
    }
    obj = PyValue_Borrow(args[_idx[0]]);
    if (_idx[1] >= 0) {
        if (!PyValue_IsObject(PyValue_Borrow(args[_idx[1]]))) {
            _PyArg_BadArgumentValue("decode", "argument 'encoding'", "str", PyValue_Borrow(args[_idx[1]]));
            goto exit;
        }
        if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])))) {
            _PyArg_BadArgument("decode", "argument 'encoding'", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])));
            goto exit;
        }
        encoding = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])));
        if (encoding == NULL) {
            goto exit;
        }
    }
    if (_idx[2] >= 0) {
        if (!PyValue_IsObject(PyValue_Borrow(args[_idx[2]]))) {
            _PyArg_BadArgumentValue("decode", "argument 'errors'", "str", PyValue_Borrow(args[_idx[2]]));
            goto exit;
        }
        if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[2]])))) {
            _PyArg_BadArgument("decode", "argument 'errors'", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[2]])));
            goto exit;
        }
        errors = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[2]])));
        if (errors == NULL) {
            goto exit;
        }
    }
    return_value = _codecs_decode_impl(PyValue_AsKnownObjectBorrow(module), obj, encoding, errors);

exit:
    return return_value;
}

PyDoc_STRVAR(_codecs_escape_decode__doc__,
"escape_decode($module, data, errors=None, /)\n"
"--\n"
"\n");

#define _CODECS_ESCAPE_DECODE_METHODDEF    \
    {"escape_decode", _PyCFunction_CAST(_codecs_escape_decode), METH_VALUE, _codecs_escape_decode__doc__},

static PyValue
_codecs_escape_decode_impl(PyObject *module, Py_buffer *data,
                           const char *errors);

static PyValue
_codecs_escape_decode(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_buffer data = {NULL, NULL};
    const char *errors = NULL;

    if (!_PyArg_CheckPositional("escape_decode", nargs, 1, 2)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[0]))->tp_name);
        goto exit;
    }
    if (PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        Py_ssize_t len;
        const char *ptr = PyUnicode_AsUTF8AndSize(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])), &len);
        if (ptr == NULL) {
            goto exit;
        }
        if (PyBuffer_FillInfo(&data, PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])), (void *)ptr, len, 1, PyBUF_SIMPLE) < 0) {
            goto exit;
        }
    }
    else { /* any bytes-like object */
        if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])), &data, PyBUF_SIMPLE) != 0) {
            goto exit;
        }
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[1]))) {
        _PyArg_BadArgumentValue("escape_decode", "argument 2", "str or None", PyValue_Borrow(args[1]));
        goto exit;
    }
    if (PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])) == Py_None) {
        errors = NULL;
    }
    else if (PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])))) {
        errors = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        if (errors == NULL) {
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("escape_decode", "argument 2", "str or None", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        goto exit;
    }
skip_optional:
    return_value = _codecs_escape_decode_impl(PyValue_AsKnownObjectBorrow(module), &data, errors);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(_codecs_escape_encode__doc__,
"escape_encode($module, data, errors=None, /)\n"
"--\n"
"\n");

#define _CODECS_ESCAPE_ENCODE_METHODDEF    \
    {"escape_encode", _PyCFunction_CAST(_codecs_escape_encode), METH_VALUE, _codecs_escape_encode__doc__},

static PyValue
_codecs_escape_encode_impl(PyObject *module, PyObject *data,
                           const char *errors);

static PyValue
_codecs_escape_encode(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *data;
    const char *errors = NULL;

    if (!_PyArg_CheckPositional("escape_encode", nargs, 1, 2)) {
        goto exit;
    }
    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[0])), &PyBytes_Type)) {
        _PyArg_BadArgumentValue("escape_encode", "argument 1", "bytes", PyValue_Borrow(args[0]));
        goto exit;
    }
    data = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    if (nargs < 2) {
        goto skip_optional;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[1]))) {
        _PyArg_BadArgumentValue("escape_encode", "argument 2", "str or None", PyValue_Borrow(args[1]));
        goto exit;
    }
    if (PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])) == Py_None) {
        errors = NULL;
    }
    else if (PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])))) {
        errors = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        if (errors == NULL) {
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("escape_encode", "argument 2", "str or None", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        goto exit;
    }
skip_optional:
    return_value = _codecs_escape_encode_impl(PyValue_AsKnownObjectBorrow(module), data, errors);

exit:
    return return_value;
}

PyDoc_STRVAR(_codecs_utf_7_decode__doc__,
"utf_7_decode($module, data, errors=None, final=False, /)\n"
"--\n"
"\n");

#define _CODECS_UTF_7_DECODE_METHODDEF    \
    {"utf_7_decode", _PyCFunction_CAST(_codecs_utf_7_decode), METH_VALUE, _codecs_utf_7_decode__doc__},

static PyValue
_codecs_utf_7_decode_impl(PyObject *module, Py_buffer *data,
                          const char *errors, int final);

static PyValue
_codecs_utf_7_decode(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_buffer data = {NULL, NULL};
    const char *errors = NULL;
    int final = 0;

    if (!_PyArg_CheckPositional("utf_7_decode", nargs, 1, 3)) {
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
    if (!PyValue_IsObject(PyValue_Borrow(args[1]))) {
        _PyArg_BadArgumentValue("utf_7_decode", "argument 2", "str or None", PyValue_Borrow(args[1]));
        goto exit;
    }
    if (PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])) == Py_None) {
        errors = NULL;
    }
    else if (PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])))) {
        errors = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        if (errors == NULL) {
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("utf_7_decode", "argument 2", "str or None", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    final = _PyValue_IsTrue(PyValue_Borrow(args[2]));
    if (final < 0) {
        goto exit;
    }
skip_optional:
    return_value = _codecs_utf_7_decode_impl(PyValue_AsKnownObjectBorrow(module), &data, errors, final);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(_codecs_utf_8_decode__doc__,
"utf_8_decode($module, data, errors=None, final=False, /)\n"
"--\n"
"\n");

#define _CODECS_UTF_8_DECODE_METHODDEF    \
    {"utf_8_decode", _PyCFunction_CAST(_codecs_utf_8_decode), METH_VALUE, _codecs_utf_8_decode__doc__},

static PyValue
_codecs_utf_8_decode_impl(PyObject *module, Py_buffer *data,
                          const char *errors, int final);

static PyValue
_codecs_utf_8_decode(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_buffer data = {NULL, NULL};
    const char *errors = NULL;
    int final = 0;

    if (!_PyArg_CheckPositional("utf_8_decode", nargs, 1, 3)) {
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
    if (!PyValue_IsObject(PyValue_Borrow(args[1]))) {
        _PyArg_BadArgumentValue("utf_8_decode", "argument 2", "str or None", PyValue_Borrow(args[1]));
        goto exit;
    }
    if (PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])) == Py_None) {
        errors = NULL;
    }
    else if (PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])))) {
        errors = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        if (errors == NULL) {
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("utf_8_decode", "argument 2", "str or None", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    final = _PyValue_IsTrue(PyValue_Borrow(args[2]));
    if (final < 0) {
        goto exit;
    }
skip_optional:
    return_value = _codecs_utf_8_decode_impl(PyValue_AsKnownObjectBorrow(module), &data, errors, final);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(_codecs_utf_16_decode__doc__,
"utf_16_decode($module, data, errors=None, final=False, /)\n"
"--\n"
"\n");

#define _CODECS_UTF_16_DECODE_METHODDEF    \
    {"utf_16_decode", _PyCFunction_CAST(_codecs_utf_16_decode), METH_VALUE, _codecs_utf_16_decode__doc__},

static PyValue
_codecs_utf_16_decode_impl(PyObject *module, Py_buffer *data,
                           const char *errors, int final);

static PyValue
_codecs_utf_16_decode(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_buffer data = {NULL, NULL};
    const char *errors = NULL;
    int final = 0;

    if (!_PyArg_CheckPositional("utf_16_decode", nargs, 1, 3)) {
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
    if (!PyValue_IsObject(PyValue_Borrow(args[1]))) {
        _PyArg_BadArgumentValue("utf_16_decode", "argument 2", "str or None", PyValue_Borrow(args[1]));
        goto exit;
    }
    if (PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])) == Py_None) {
        errors = NULL;
    }
    else if (PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])))) {
        errors = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        if (errors == NULL) {
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("utf_16_decode", "argument 2", "str or None", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    final = _PyValue_IsTrue(PyValue_Borrow(args[2]));
    if (final < 0) {
        goto exit;
    }
skip_optional:
    return_value = _codecs_utf_16_decode_impl(PyValue_AsKnownObjectBorrow(module), &data, errors, final);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(_codecs_utf_16_le_decode__doc__,
"utf_16_le_decode($module, data, errors=None, final=False, /)\n"
"--\n"
"\n");

#define _CODECS_UTF_16_LE_DECODE_METHODDEF    \
    {"utf_16_le_decode", _PyCFunction_CAST(_codecs_utf_16_le_decode), METH_VALUE, _codecs_utf_16_le_decode__doc__},

static PyValue
_codecs_utf_16_le_decode_impl(PyObject *module, Py_buffer *data,
                              const char *errors, int final);

static PyValue
_codecs_utf_16_le_decode(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_buffer data = {NULL, NULL};
    const char *errors = NULL;
    int final = 0;

    if (!_PyArg_CheckPositional("utf_16_le_decode", nargs, 1, 3)) {
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
    if (!PyValue_IsObject(PyValue_Borrow(args[1]))) {
        _PyArg_BadArgumentValue("utf_16_le_decode", "argument 2", "str or None", PyValue_Borrow(args[1]));
        goto exit;
    }
    if (PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])) == Py_None) {
        errors = NULL;
    }
    else if (PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])))) {
        errors = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        if (errors == NULL) {
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("utf_16_le_decode", "argument 2", "str or None", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    final = _PyValue_IsTrue(PyValue_Borrow(args[2]));
    if (final < 0) {
        goto exit;
    }
skip_optional:
    return_value = _codecs_utf_16_le_decode_impl(PyValue_AsKnownObjectBorrow(module), &data, errors, final);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(_codecs_utf_16_be_decode__doc__,
"utf_16_be_decode($module, data, errors=None, final=False, /)\n"
"--\n"
"\n");

#define _CODECS_UTF_16_BE_DECODE_METHODDEF    \
    {"utf_16_be_decode", _PyCFunction_CAST(_codecs_utf_16_be_decode), METH_VALUE, _codecs_utf_16_be_decode__doc__},

static PyValue
_codecs_utf_16_be_decode_impl(PyObject *module, Py_buffer *data,
                              const char *errors, int final);

static PyValue
_codecs_utf_16_be_decode(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_buffer data = {NULL, NULL};
    const char *errors = NULL;
    int final = 0;

    if (!_PyArg_CheckPositional("utf_16_be_decode", nargs, 1, 3)) {
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
    if (!PyValue_IsObject(PyValue_Borrow(args[1]))) {
        _PyArg_BadArgumentValue("utf_16_be_decode", "argument 2", "str or None", PyValue_Borrow(args[1]));
        goto exit;
    }
    if (PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])) == Py_None) {
        errors = NULL;
    }
    else if (PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])))) {
        errors = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        if (errors == NULL) {
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("utf_16_be_decode", "argument 2", "str or None", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    final = _PyValue_IsTrue(PyValue_Borrow(args[2]));
    if (final < 0) {
        goto exit;
    }
skip_optional:
    return_value = _codecs_utf_16_be_decode_impl(PyValue_AsKnownObjectBorrow(module), &data, errors, final);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(_codecs_utf_16_ex_decode__doc__,
"utf_16_ex_decode($module, data, errors=None, byteorder=0, final=False,\n"
"                 /)\n"
"--\n"
"\n");

#define _CODECS_UTF_16_EX_DECODE_METHODDEF    \
    {"utf_16_ex_decode", _PyCFunction_CAST(_codecs_utf_16_ex_decode), METH_VALUE, _codecs_utf_16_ex_decode__doc__},

static PyValue
_codecs_utf_16_ex_decode_impl(PyObject *module, Py_buffer *data,
                              const char *errors, int byteorder, int final);

static PyValue
_codecs_utf_16_ex_decode(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_buffer data = {NULL, NULL};
    const char *errors = NULL;
    int byteorder = 0;
    int final = 0;

    if (!_PyArg_CheckPositional("utf_16_ex_decode", nargs, 1, 4)) {
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
    if (!PyValue_IsObject(PyValue_Borrow(args[1]))) {
        _PyArg_BadArgumentValue("utf_16_ex_decode", "argument 2", "str or None", PyValue_Borrow(args[1]));
        goto exit;
    }
    if (PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])) == Py_None) {
        errors = NULL;
    }
    else if (PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])))) {
        errors = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        if (errors == NULL) {
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("utf_16_ex_decode", "argument 2", "str or None", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    byteorder = _PyValue_AsInt(PyValue_Borrow(args[2]));
    if (byteorder == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (nargs < 4) {
        goto skip_optional;
    }
    final = _PyValue_IsTrue(PyValue_Borrow(args[3]));
    if (final < 0) {
        goto exit;
    }
skip_optional:
    return_value = _codecs_utf_16_ex_decode_impl(PyValue_AsKnownObjectBorrow(module), &data, errors, byteorder, final);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(_codecs_utf_32_decode__doc__,
"utf_32_decode($module, data, errors=None, final=False, /)\n"
"--\n"
"\n");

#define _CODECS_UTF_32_DECODE_METHODDEF    \
    {"utf_32_decode", _PyCFunction_CAST(_codecs_utf_32_decode), METH_VALUE, _codecs_utf_32_decode__doc__},

static PyValue
_codecs_utf_32_decode_impl(PyObject *module, Py_buffer *data,
                           const char *errors, int final);

static PyValue
_codecs_utf_32_decode(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_buffer data = {NULL, NULL};
    const char *errors = NULL;
    int final = 0;

    if (!_PyArg_CheckPositional("utf_32_decode", nargs, 1, 3)) {
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
    if (!PyValue_IsObject(PyValue_Borrow(args[1]))) {
        _PyArg_BadArgumentValue("utf_32_decode", "argument 2", "str or None", PyValue_Borrow(args[1]));
        goto exit;
    }
    if (PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])) == Py_None) {
        errors = NULL;
    }
    else if (PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])))) {
        errors = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        if (errors == NULL) {
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("utf_32_decode", "argument 2", "str or None", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    final = _PyValue_IsTrue(PyValue_Borrow(args[2]));
    if (final < 0) {
        goto exit;
    }
skip_optional:
    return_value = _codecs_utf_32_decode_impl(PyValue_AsKnownObjectBorrow(module), &data, errors, final);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(_codecs_utf_32_le_decode__doc__,
"utf_32_le_decode($module, data, errors=None, final=False, /)\n"
"--\n"
"\n");

#define _CODECS_UTF_32_LE_DECODE_METHODDEF    \
    {"utf_32_le_decode", _PyCFunction_CAST(_codecs_utf_32_le_decode), METH_VALUE, _codecs_utf_32_le_decode__doc__},

static PyValue
_codecs_utf_32_le_decode_impl(PyObject *module, Py_buffer *data,
                              const char *errors, int final);

static PyValue
_codecs_utf_32_le_decode(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_buffer data = {NULL, NULL};
    const char *errors = NULL;
    int final = 0;

    if (!_PyArg_CheckPositional("utf_32_le_decode", nargs, 1, 3)) {
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
    if (!PyValue_IsObject(PyValue_Borrow(args[1]))) {
        _PyArg_BadArgumentValue("utf_32_le_decode", "argument 2", "str or None", PyValue_Borrow(args[1]));
        goto exit;
    }
    if (PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])) == Py_None) {
        errors = NULL;
    }
    else if (PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])))) {
        errors = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        if (errors == NULL) {
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("utf_32_le_decode", "argument 2", "str or None", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    final = _PyValue_IsTrue(PyValue_Borrow(args[2]));
    if (final < 0) {
        goto exit;
    }
skip_optional:
    return_value = _codecs_utf_32_le_decode_impl(PyValue_AsKnownObjectBorrow(module), &data, errors, final);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(_codecs_utf_32_be_decode__doc__,
"utf_32_be_decode($module, data, errors=None, final=False, /)\n"
"--\n"
"\n");

#define _CODECS_UTF_32_BE_DECODE_METHODDEF    \
    {"utf_32_be_decode", _PyCFunction_CAST(_codecs_utf_32_be_decode), METH_VALUE, _codecs_utf_32_be_decode__doc__},

static PyValue
_codecs_utf_32_be_decode_impl(PyObject *module, Py_buffer *data,
                              const char *errors, int final);

static PyValue
_codecs_utf_32_be_decode(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_buffer data = {NULL, NULL};
    const char *errors = NULL;
    int final = 0;

    if (!_PyArg_CheckPositional("utf_32_be_decode", nargs, 1, 3)) {
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
    if (!PyValue_IsObject(PyValue_Borrow(args[1]))) {
        _PyArg_BadArgumentValue("utf_32_be_decode", "argument 2", "str or None", PyValue_Borrow(args[1]));
        goto exit;
    }
    if (PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])) == Py_None) {
        errors = NULL;
    }
    else if (PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])))) {
        errors = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        if (errors == NULL) {
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("utf_32_be_decode", "argument 2", "str or None", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    final = _PyValue_IsTrue(PyValue_Borrow(args[2]));
    if (final < 0) {
        goto exit;
    }
skip_optional:
    return_value = _codecs_utf_32_be_decode_impl(PyValue_AsKnownObjectBorrow(module), &data, errors, final);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(_codecs_utf_32_ex_decode__doc__,
"utf_32_ex_decode($module, data, errors=None, byteorder=0, final=False,\n"
"                 /)\n"
"--\n"
"\n");

#define _CODECS_UTF_32_EX_DECODE_METHODDEF    \
    {"utf_32_ex_decode", _PyCFunction_CAST(_codecs_utf_32_ex_decode), METH_VALUE, _codecs_utf_32_ex_decode__doc__},

static PyValue
_codecs_utf_32_ex_decode_impl(PyObject *module, Py_buffer *data,
                              const char *errors, int byteorder, int final);

static PyValue
_codecs_utf_32_ex_decode(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_buffer data = {NULL, NULL};
    const char *errors = NULL;
    int byteorder = 0;
    int final = 0;

    if (!_PyArg_CheckPositional("utf_32_ex_decode", nargs, 1, 4)) {
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
    if (!PyValue_IsObject(PyValue_Borrow(args[1]))) {
        _PyArg_BadArgumentValue("utf_32_ex_decode", "argument 2", "str or None", PyValue_Borrow(args[1]));
        goto exit;
    }
    if (PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])) == Py_None) {
        errors = NULL;
    }
    else if (PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])))) {
        errors = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        if (errors == NULL) {
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("utf_32_ex_decode", "argument 2", "str or None", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    byteorder = _PyValue_AsInt(PyValue_Borrow(args[2]));
    if (byteorder == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (nargs < 4) {
        goto skip_optional;
    }
    final = _PyValue_IsTrue(PyValue_Borrow(args[3]));
    if (final < 0) {
        goto exit;
    }
skip_optional:
    return_value = _codecs_utf_32_ex_decode_impl(PyValue_AsKnownObjectBorrow(module), &data, errors, byteorder, final);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(_codecs_unicode_escape_decode__doc__,
"unicode_escape_decode($module, data, errors=None, final=True, /)\n"
"--\n"
"\n");

#define _CODECS_UNICODE_ESCAPE_DECODE_METHODDEF    \
    {"unicode_escape_decode", _PyCFunction_CAST(_codecs_unicode_escape_decode), METH_VALUE, _codecs_unicode_escape_decode__doc__},

static PyValue
_codecs_unicode_escape_decode_impl(PyObject *module, Py_buffer *data,
                                   const char *errors, int final);

static PyValue
_codecs_unicode_escape_decode(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_buffer data = {NULL, NULL};
    const char *errors = NULL;
    int final = 1;

    if (!_PyArg_CheckPositional("unicode_escape_decode", nargs, 1, 3)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[0]))->tp_name);
        goto exit;
    }
    if (PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        Py_ssize_t len;
        const char *ptr = PyUnicode_AsUTF8AndSize(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])), &len);
        if (ptr == NULL) {
            goto exit;
        }
        if (PyBuffer_FillInfo(&data, PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])), (void *)ptr, len, 1, PyBUF_SIMPLE) < 0) {
            goto exit;
        }
    }
    else { /* any bytes-like object */
        if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])), &data, PyBUF_SIMPLE) != 0) {
            goto exit;
        }
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[1]))) {
        _PyArg_BadArgumentValue("unicode_escape_decode", "argument 2", "str or None", PyValue_Borrow(args[1]));
        goto exit;
    }
    if (PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])) == Py_None) {
        errors = NULL;
    }
    else if (PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])))) {
        errors = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        if (errors == NULL) {
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("unicode_escape_decode", "argument 2", "str or None", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    final = _PyValue_IsTrue(PyValue_Borrow(args[2]));
    if (final < 0) {
        goto exit;
    }
skip_optional:
    return_value = _codecs_unicode_escape_decode_impl(PyValue_AsKnownObjectBorrow(module), &data, errors, final);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(_codecs_raw_unicode_escape_decode__doc__,
"raw_unicode_escape_decode($module, data, errors=None, final=True, /)\n"
"--\n"
"\n");

#define _CODECS_RAW_UNICODE_ESCAPE_DECODE_METHODDEF    \
    {"raw_unicode_escape_decode", _PyCFunction_CAST(_codecs_raw_unicode_escape_decode), METH_VALUE, _codecs_raw_unicode_escape_decode__doc__},

static PyValue
_codecs_raw_unicode_escape_decode_impl(PyObject *module, Py_buffer *data,
                                       const char *errors, int final);

static PyValue
_codecs_raw_unicode_escape_decode(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_buffer data = {NULL, NULL};
    const char *errors = NULL;
    int final = 1;

    if (!_PyArg_CheckPositional("raw_unicode_escape_decode", nargs, 1, 3)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[0]))->tp_name);
        goto exit;
    }
    if (PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        Py_ssize_t len;
        const char *ptr = PyUnicode_AsUTF8AndSize(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])), &len);
        if (ptr == NULL) {
            goto exit;
        }
        if (PyBuffer_FillInfo(&data, PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])), (void *)ptr, len, 1, PyBUF_SIMPLE) < 0) {
            goto exit;
        }
    }
    else { /* any bytes-like object */
        if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])), &data, PyBUF_SIMPLE) != 0) {
            goto exit;
        }
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[1]))) {
        _PyArg_BadArgumentValue("raw_unicode_escape_decode", "argument 2", "str or None", PyValue_Borrow(args[1]));
        goto exit;
    }
    if (PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])) == Py_None) {
        errors = NULL;
    }
    else if (PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])))) {
        errors = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        if (errors == NULL) {
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("raw_unicode_escape_decode", "argument 2", "str or None", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    final = _PyValue_IsTrue(PyValue_Borrow(args[2]));
    if (final < 0) {
        goto exit;
    }
skip_optional:
    return_value = _codecs_raw_unicode_escape_decode_impl(PyValue_AsKnownObjectBorrow(module), &data, errors, final);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(_codecs_latin_1_decode__doc__,
"latin_1_decode($module, data, errors=None, /)\n"
"--\n"
"\n");

#define _CODECS_LATIN_1_DECODE_METHODDEF    \
    {"latin_1_decode", _PyCFunction_CAST(_codecs_latin_1_decode), METH_VALUE, _codecs_latin_1_decode__doc__},

static PyValue
_codecs_latin_1_decode_impl(PyObject *module, Py_buffer *data,
                            const char *errors);

static PyValue
_codecs_latin_1_decode(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_buffer data = {NULL, NULL};
    const char *errors = NULL;

    if (!_PyArg_CheckPositional("latin_1_decode", nargs, 1, 2)) {
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
    if (!PyValue_IsObject(PyValue_Borrow(args[1]))) {
        _PyArg_BadArgumentValue("latin_1_decode", "argument 2", "str or None", PyValue_Borrow(args[1]));
        goto exit;
    }
    if (PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])) == Py_None) {
        errors = NULL;
    }
    else if (PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])))) {
        errors = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        if (errors == NULL) {
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("latin_1_decode", "argument 2", "str or None", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        goto exit;
    }
skip_optional:
    return_value = _codecs_latin_1_decode_impl(PyValue_AsKnownObjectBorrow(module), &data, errors);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(_codecs_ascii_decode__doc__,
"ascii_decode($module, data, errors=None, /)\n"
"--\n"
"\n");

#define _CODECS_ASCII_DECODE_METHODDEF    \
    {"ascii_decode", _PyCFunction_CAST(_codecs_ascii_decode), METH_VALUE, _codecs_ascii_decode__doc__},

static PyValue
_codecs_ascii_decode_impl(PyObject *module, Py_buffer *data,
                          const char *errors);

static PyValue
_codecs_ascii_decode(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_buffer data = {NULL, NULL};
    const char *errors = NULL;

    if (!_PyArg_CheckPositional("ascii_decode", nargs, 1, 2)) {
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
    if (!PyValue_IsObject(PyValue_Borrow(args[1]))) {
        _PyArg_BadArgumentValue("ascii_decode", "argument 2", "str or None", PyValue_Borrow(args[1]));
        goto exit;
    }
    if (PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])) == Py_None) {
        errors = NULL;
    }
    else if (PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])))) {
        errors = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        if (errors == NULL) {
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("ascii_decode", "argument 2", "str or None", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        goto exit;
    }
skip_optional:
    return_value = _codecs_ascii_decode_impl(PyValue_AsKnownObjectBorrow(module), &data, errors);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(_codecs_charmap_decode__doc__,
"charmap_decode($module, data, errors=None, mapping=None, /)\n"
"--\n"
"\n");

#define _CODECS_CHARMAP_DECODE_METHODDEF    \
    {"charmap_decode", _PyCFunction_CAST(_codecs_charmap_decode), METH_VALUE, _codecs_charmap_decode__doc__},

static PyValue
_codecs_charmap_decode_impl(PyObject *module, Py_buffer *data,
                            const char *errors, PyValueBorrow mapping);

static PyValue
_codecs_charmap_decode(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_buffer data = {NULL, NULL};
    const char *errors = NULL;
    PyValueBorrow mapping = PyValue_Borrow(PyValue_None);

    if (!_PyArg_CheckPositional("charmap_decode", nargs, 1, 3)) {
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
    if (!PyValue_IsObject(PyValue_Borrow(args[1]))) {
        _PyArg_BadArgumentValue("charmap_decode", "argument 2", "str or None", PyValue_Borrow(args[1]));
        goto exit;
    }
    if (PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])) == Py_None) {
        errors = NULL;
    }
    else if (PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])))) {
        errors = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        if (errors == NULL) {
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("charmap_decode", "argument 2", "str or None", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    mapping = PyValue_Borrow(args[2]);
skip_optional:
    return_value = _codecs_charmap_decode_impl(PyValue_AsKnownObjectBorrow(module), &data, errors, mapping);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

#if defined(MS_WINDOWS)

PyDoc_STRVAR(_codecs_mbcs_decode__doc__,
"mbcs_decode($module, data, errors=None, final=False, /)\n"
"--\n"
"\n");

#define _CODECS_MBCS_DECODE_METHODDEF    \
    {"mbcs_decode", _PyCFunction_CAST(_codecs_mbcs_decode), METH_VALUE, _codecs_mbcs_decode__doc__},

static PyValue
_codecs_mbcs_decode_impl(PyObject *module, Py_buffer *data,
                         const char *errors, int final);

static PyValue
_codecs_mbcs_decode(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_buffer data = {NULL, NULL};
    const char *errors = NULL;
    int final = 0;

    if (!_PyArg_CheckPositional("mbcs_decode", nargs, 1, 3)) {
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
    if (!PyValue_IsObject(PyValue_Borrow(args[1]))) {
        _PyArg_BadArgumentValue("mbcs_decode", "argument 2", "str or None", PyValue_Borrow(args[1]));
        goto exit;
    }
    if (PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])) == Py_None) {
        errors = NULL;
    }
    else if (PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])))) {
        errors = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        if (errors == NULL) {
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("mbcs_decode", "argument 2", "str or None", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    final = _PyValue_IsTrue(PyValue_Borrow(args[2]));
    if (final < 0) {
        goto exit;
    }
skip_optional:
    return_value = _codecs_mbcs_decode_impl(PyValue_AsKnownObjectBorrow(module), &data, errors, final);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

#endif /* defined(MS_WINDOWS) */

#if defined(MS_WINDOWS)

PyDoc_STRVAR(_codecs_oem_decode__doc__,
"oem_decode($module, data, errors=None, final=False, /)\n"
"--\n"
"\n");

#define _CODECS_OEM_DECODE_METHODDEF    \
    {"oem_decode", _PyCFunction_CAST(_codecs_oem_decode), METH_VALUE, _codecs_oem_decode__doc__},

static PyValue
_codecs_oem_decode_impl(PyObject *module, Py_buffer *data,
                        const char *errors, int final);

static PyValue
_codecs_oem_decode(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_buffer data = {NULL, NULL};
    const char *errors = NULL;
    int final = 0;

    if (!_PyArg_CheckPositional("oem_decode", nargs, 1, 3)) {
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
    if (!PyValue_IsObject(PyValue_Borrow(args[1]))) {
        _PyArg_BadArgumentValue("oem_decode", "argument 2", "str or None", PyValue_Borrow(args[1]));
        goto exit;
    }
    if (PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])) == Py_None) {
        errors = NULL;
    }
    else if (PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])))) {
        errors = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        if (errors == NULL) {
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("oem_decode", "argument 2", "str or None", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    final = _PyValue_IsTrue(PyValue_Borrow(args[2]));
    if (final < 0) {
        goto exit;
    }
skip_optional:
    return_value = _codecs_oem_decode_impl(PyValue_AsKnownObjectBorrow(module), &data, errors, final);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

#endif /* defined(MS_WINDOWS) */

#if defined(MS_WINDOWS)

PyDoc_STRVAR(_codecs_code_page_decode__doc__,
"code_page_decode($module, codepage, data, errors=None, final=False, /)\n"
"--\n"
"\n");

#define _CODECS_CODE_PAGE_DECODE_METHODDEF    \
    {"code_page_decode", _PyCFunction_CAST(_codecs_code_page_decode), METH_VALUE, _codecs_code_page_decode__doc__},

static PyValue
_codecs_code_page_decode_impl(PyObject *module, int codepage,
                              Py_buffer *data, const char *errors, int final);

static PyValue
_codecs_code_page_decode(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int codepage;
    Py_buffer data = {NULL, NULL};
    const char *errors = NULL;
    int final = 0;

    if (!_PyArg_CheckPositional("code_page_decode", nargs, 2, 4)) {
        goto exit;
    }
    codepage = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (codepage == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[1]))) {
        PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[1]))->tp_name);
        goto exit;
    }
    if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])), &data, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[2]))) {
        _PyArg_BadArgumentValue("code_page_decode", "argument 3", "str or None", PyValue_Borrow(args[2]));
        goto exit;
    }
    if (PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[2])) == Py_None) {
        errors = NULL;
    }
    else if (PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[2])))) {
        errors = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[2])));
        if (errors == NULL) {
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("code_page_decode", "argument 3", "str or None", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[2])));
        goto exit;
    }
    if (nargs < 4) {
        goto skip_optional;
    }
    final = _PyValue_IsTrue(PyValue_Borrow(args[3]));
    if (final < 0) {
        goto exit;
    }
skip_optional:
    return_value = _codecs_code_page_decode_impl(PyValue_AsKnownObjectBorrow(module), codepage, &data, errors, final);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

#endif /* defined(MS_WINDOWS) */

#if defined(_Py_HAVE_ICONV)

PyDoc_STRVAR(_codecs_iconv_decode__doc__,
"iconv_decode($module, encoding, data, errors=None, final=False, /)\n"
"--\n"
"\n");

#define _CODECS_ICONV_DECODE_METHODDEF    \
    {"iconv_decode", _PyCFunction_CAST(_codecs_iconv_decode), METH_VALUE, _codecs_iconv_decode__doc__},

static PyValue
_codecs_iconv_decode_impl(PyObject *module, const char *encoding,
                          Py_buffer *data, const char *errors, int final);

static PyValue
_codecs_iconv_decode(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    const char *encoding;
    Py_buffer data = {NULL, NULL};
    const char *errors = NULL;
    int final = 0;

    if (!_PyArg_CheckPositional("iconv_decode", nargs, 2, 4)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("iconv_decode", "argument 1", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("iconv_decode", "argument 1", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    encoding = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
    if (encoding == NULL) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[1]))) {
        PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[1]))->tp_name);
        goto exit;
    }
    if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])), &data, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[2]))) {
        _PyArg_BadArgumentValue("iconv_decode", "argument 3", "str or None", PyValue_Borrow(args[2]));
        goto exit;
    }
    if (PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[2])) == Py_None) {
        errors = NULL;
    }
    else if (PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[2])))) {
        errors = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[2])));
        if (errors == NULL) {
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("iconv_decode", "argument 3", "str or None", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[2])));
        goto exit;
    }
    if (nargs < 4) {
        goto skip_optional;
    }
    final = _PyValue_IsTrue(PyValue_Borrow(args[3]));
    if (final < 0) {
        goto exit;
    }
skip_optional:
    return_value = _codecs_iconv_decode_impl(PyValue_AsKnownObjectBorrow(module), encoding, &data, errors, final);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

#endif /* defined(_Py_HAVE_ICONV) */

PyDoc_STRVAR(_codecs_readbuffer_encode__doc__,
"readbuffer_encode($module, data, errors=None, /)\n"
"--\n"
"\n");

#define _CODECS_READBUFFER_ENCODE_METHODDEF    \
    {"readbuffer_encode", _PyCFunction_CAST(_codecs_readbuffer_encode), METH_VALUE, _codecs_readbuffer_encode__doc__},

static PyValue
_codecs_readbuffer_encode_impl(PyObject *module, Py_buffer *data,
                               const char *errors);

static PyValue
_codecs_readbuffer_encode(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_buffer data = {NULL, NULL};
    const char *errors = NULL;

    if (!_PyArg_CheckPositional("readbuffer_encode", nargs, 1, 2)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[0]))->tp_name);
        goto exit;
    }
    if (PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        Py_ssize_t len;
        const char *ptr = PyUnicode_AsUTF8AndSize(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])), &len);
        if (ptr == NULL) {
            goto exit;
        }
        if (PyBuffer_FillInfo(&data, PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])), (void *)ptr, len, 1, PyBUF_SIMPLE) < 0) {
            goto exit;
        }
    }
    else { /* any bytes-like object */
        if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])), &data, PyBUF_SIMPLE) != 0) {
            goto exit;
        }
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[1]))) {
        _PyArg_BadArgumentValue("readbuffer_encode", "argument 2", "str or None", PyValue_Borrow(args[1]));
        goto exit;
    }
    if (PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])) == Py_None) {
        errors = NULL;
    }
    else if (PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])))) {
        errors = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        if (errors == NULL) {
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("readbuffer_encode", "argument 2", "str or None", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        goto exit;
    }
skip_optional:
    return_value = _codecs_readbuffer_encode_impl(PyValue_AsKnownObjectBorrow(module), &data, errors);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(_codecs_utf_7_encode__doc__,
"utf_7_encode($module, str, errors=None, /)\n"
"--\n"
"\n");

#define _CODECS_UTF_7_ENCODE_METHODDEF    \
    {"utf_7_encode", _PyCFunction_CAST(_codecs_utf_7_encode), METH_VALUE, _codecs_utf_7_encode__doc__},

static PyValue
_codecs_utf_7_encode_impl(PyObject *module, PyObject *str,
                          const char *errors);

static PyValue
_codecs_utf_7_encode(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *str;
    const char *errors = NULL;

    if (!_PyArg_CheckPositional("utf_7_encode", nargs, 1, 2)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("utf_7_encode", "argument 1", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("utf_7_encode", "argument 1", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    str = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    if (nargs < 2) {
        goto skip_optional;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[1]))) {
        _PyArg_BadArgumentValue("utf_7_encode", "argument 2", "str or None", PyValue_Borrow(args[1]));
        goto exit;
    }
    if (PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])) == Py_None) {
        errors = NULL;
    }
    else if (PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])))) {
        errors = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        if (errors == NULL) {
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("utf_7_encode", "argument 2", "str or None", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        goto exit;
    }
skip_optional:
    return_value = _codecs_utf_7_encode_impl(PyValue_AsKnownObjectBorrow(module), str, errors);

exit:
    return return_value;
}

PyDoc_STRVAR(_codecs_utf_8_encode__doc__,
"utf_8_encode($module, str, errors=None, /)\n"
"--\n"
"\n");

#define _CODECS_UTF_8_ENCODE_METHODDEF    \
    {"utf_8_encode", _PyCFunction_CAST(_codecs_utf_8_encode), METH_VALUE, _codecs_utf_8_encode__doc__},

static PyValue
_codecs_utf_8_encode_impl(PyObject *module, PyObject *str,
                          const char *errors);

static PyValue
_codecs_utf_8_encode(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *str;
    const char *errors = NULL;

    if (!_PyArg_CheckPositional("utf_8_encode", nargs, 1, 2)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("utf_8_encode", "argument 1", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("utf_8_encode", "argument 1", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    str = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    if (nargs < 2) {
        goto skip_optional;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[1]))) {
        _PyArg_BadArgumentValue("utf_8_encode", "argument 2", "str or None", PyValue_Borrow(args[1]));
        goto exit;
    }
    if (PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])) == Py_None) {
        errors = NULL;
    }
    else if (PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])))) {
        errors = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        if (errors == NULL) {
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("utf_8_encode", "argument 2", "str or None", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        goto exit;
    }
skip_optional:
    return_value = _codecs_utf_8_encode_impl(PyValue_AsKnownObjectBorrow(module), str, errors);

exit:
    return return_value;
}

PyDoc_STRVAR(_codecs_utf_16_encode__doc__,
"utf_16_encode($module, str, errors=None, byteorder=0, /)\n"
"--\n"
"\n");

#define _CODECS_UTF_16_ENCODE_METHODDEF    \
    {"utf_16_encode", _PyCFunction_CAST(_codecs_utf_16_encode), METH_VALUE, _codecs_utf_16_encode__doc__},

static PyValue
_codecs_utf_16_encode_impl(PyObject *module, PyObject *str,
                           const char *errors, int byteorder);

static PyValue
_codecs_utf_16_encode(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *str;
    const char *errors = NULL;
    int byteorder = 0;

    if (!_PyArg_CheckPositional("utf_16_encode", nargs, 1, 3)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("utf_16_encode", "argument 1", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("utf_16_encode", "argument 1", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    str = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    if (nargs < 2) {
        goto skip_optional;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[1]))) {
        _PyArg_BadArgumentValue("utf_16_encode", "argument 2", "str or None", PyValue_Borrow(args[1]));
        goto exit;
    }
    if (PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])) == Py_None) {
        errors = NULL;
    }
    else if (PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])))) {
        errors = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        if (errors == NULL) {
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("utf_16_encode", "argument 2", "str or None", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    byteorder = _PyValue_AsInt(PyValue_Borrow(args[2]));
    if (byteorder == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = _codecs_utf_16_encode_impl(PyValue_AsKnownObjectBorrow(module), str, errors, byteorder);

exit:
    return return_value;
}

PyDoc_STRVAR(_codecs_utf_16_le_encode__doc__,
"utf_16_le_encode($module, str, errors=None, /)\n"
"--\n"
"\n");

#define _CODECS_UTF_16_LE_ENCODE_METHODDEF    \
    {"utf_16_le_encode", _PyCFunction_CAST(_codecs_utf_16_le_encode), METH_VALUE, _codecs_utf_16_le_encode__doc__},

static PyValue
_codecs_utf_16_le_encode_impl(PyObject *module, PyObject *str,
                              const char *errors);

static PyValue
_codecs_utf_16_le_encode(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *str;
    const char *errors = NULL;

    if (!_PyArg_CheckPositional("utf_16_le_encode", nargs, 1, 2)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("utf_16_le_encode", "argument 1", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("utf_16_le_encode", "argument 1", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    str = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    if (nargs < 2) {
        goto skip_optional;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[1]))) {
        _PyArg_BadArgumentValue("utf_16_le_encode", "argument 2", "str or None", PyValue_Borrow(args[1]));
        goto exit;
    }
    if (PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])) == Py_None) {
        errors = NULL;
    }
    else if (PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])))) {
        errors = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        if (errors == NULL) {
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("utf_16_le_encode", "argument 2", "str or None", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        goto exit;
    }
skip_optional:
    return_value = _codecs_utf_16_le_encode_impl(PyValue_AsKnownObjectBorrow(module), str, errors);

exit:
    return return_value;
}

PyDoc_STRVAR(_codecs_utf_16_be_encode__doc__,
"utf_16_be_encode($module, str, errors=None, /)\n"
"--\n"
"\n");

#define _CODECS_UTF_16_BE_ENCODE_METHODDEF    \
    {"utf_16_be_encode", _PyCFunction_CAST(_codecs_utf_16_be_encode), METH_VALUE, _codecs_utf_16_be_encode__doc__},

static PyValue
_codecs_utf_16_be_encode_impl(PyObject *module, PyObject *str,
                              const char *errors);

static PyValue
_codecs_utf_16_be_encode(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *str;
    const char *errors = NULL;

    if (!_PyArg_CheckPositional("utf_16_be_encode", nargs, 1, 2)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("utf_16_be_encode", "argument 1", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("utf_16_be_encode", "argument 1", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    str = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    if (nargs < 2) {
        goto skip_optional;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[1]))) {
        _PyArg_BadArgumentValue("utf_16_be_encode", "argument 2", "str or None", PyValue_Borrow(args[1]));
        goto exit;
    }
    if (PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])) == Py_None) {
        errors = NULL;
    }
    else if (PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])))) {
        errors = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        if (errors == NULL) {
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("utf_16_be_encode", "argument 2", "str or None", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        goto exit;
    }
skip_optional:
    return_value = _codecs_utf_16_be_encode_impl(PyValue_AsKnownObjectBorrow(module), str, errors);

exit:
    return return_value;
}

PyDoc_STRVAR(_codecs_utf_32_encode__doc__,
"utf_32_encode($module, str, errors=None, byteorder=0, /)\n"
"--\n"
"\n");

#define _CODECS_UTF_32_ENCODE_METHODDEF    \
    {"utf_32_encode", _PyCFunction_CAST(_codecs_utf_32_encode), METH_VALUE, _codecs_utf_32_encode__doc__},

static PyValue
_codecs_utf_32_encode_impl(PyObject *module, PyObject *str,
                           const char *errors, int byteorder);

static PyValue
_codecs_utf_32_encode(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *str;
    const char *errors = NULL;
    int byteorder = 0;

    if (!_PyArg_CheckPositional("utf_32_encode", nargs, 1, 3)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("utf_32_encode", "argument 1", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("utf_32_encode", "argument 1", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    str = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    if (nargs < 2) {
        goto skip_optional;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[1]))) {
        _PyArg_BadArgumentValue("utf_32_encode", "argument 2", "str or None", PyValue_Borrow(args[1]));
        goto exit;
    }
    if (PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])) == Py_None) {
        errors = NULL;
    }
    else if (PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])))) {
        errors = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        if (errors == NULL) {
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("utf_32_encode", "argument 2", "str or None", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    byteorder = _PyValue_AsInt(PyValue_Borrow(args[2]));
    if (byteorder == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = _codecs_utf_32_encode_impl(PyValue_AsKnownObjectBorrow(module), str, errors, byteorder);

exit:
    return return_value;
}

PyDoc_STRVAR(_codecs_utf_32_le_encode__doc__,
"utf_32_le_encode($module, str, errors=None, /)\n"
"--\n"
"\n");

#define _CODECS_UTF_32_LE_ENCODE_METHODDEF    \
    {"utf_32_le_encode", _PyCFunction_CAST(_codecs_utf_32_le_encode), METH_VALUE, _codecs_utf_32_le_encode__doc__},

static PyValue
_codecs_utf_32_le_encode_impl(PyObject *module, PyObject *str,
                              const char *errors);

static PyValue
_codecs_utf_32_le_encode(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *str;
    const char *errors = NULL;

    if (!_PyArg_CheckPositional("utf_32_le_encode", nargs, 1, 2)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("utf_32_le_encode", "argument 1", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("utf_32_le_encode", "argument 1", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    str = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    if (nargs < 2) {
        goto skip_optional;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[1]))) {
        _PyArg_BadArgumentValue("utf_32_le_encode", "argument 2", "str or None", PyValue_Borrow(args[1]));
        goto exit;
    }
    if (PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])) == Py_None) {
        errors = NULL;
    }
    else if (PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])))) {
        errors = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        if (errors == NULL) {
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("utf_32_le_encode", "argument 2", "str or None", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        goto exit;
    }
skip_optional:
    return_value = _codecs_utf_32_le_encode_impl(PyValue_AsKnownObjectBorrow(module), str, errors);

exit:
    return return_value;
}

PyDoc_STRVAR(_codecs_utf_32_be_encode__doc__,
"utf_32_be_encode($module, str, errors=None, /)\n"
"--\n"
"\n");

#define _CODECS_UTF_32_BE_ENCODE_METHODDEF    \
    {"utf_32_be_encode", _PyCFunction_CAST(_codecs_utf_32_be_encode), METH_VALUE, _codecs_utf_32_be_encode__doc__},

static PyValue
_codecs_utf_32_be_encode_impl(PyObject *module, PyObject *str,
                              const char *errors);

static PyValue
_codecs_utf_32_be_encode(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *str;
    const char *errors = NULL;

    if (!_PyArg_CheckPositional("utf_32_be_encode", nargs, 1, 2)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("utf_32_be_encode", "argument 1", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("utf_32_be_encode", "argument 1", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    str = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    if (nargs < 2) {
        goto skip_optional;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[1]))) {
        _PyArg_BadArgumentValue("utf_32_be_encode", "argument 2", "str or None", PyValue_Borrow(args[1]));
        goto exit;
    }
    if (PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])) == Py_None) {
        errors = NULL;
    }
    else if (PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])))) {
        errors = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        if (errors == NULL) {
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("utf_32_be_encode", "argument 2", "str or None", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        goto exit;
    }
skip_optional:
    return_value = _codecs_utf_32_be_encode_impl(PyValue_AsKnownObjectBorrow(module), str, errors);

exit:
    return return_value;
}

PyDoc_STRVAR(_codecs_unicode_escape_encode__doc__,
"unicode_escape_encode($module, str, errors=None, /)\n"
"--\n"
"\n");

#define _CODECS_UNICODE_ESCAPE_ENCODE_METHODDEF    \
    {"unicode_escape_encode", _PyCFunction_CAST(_codecs_unicode_escape_encode), METH_VALUE, _codecs_unicode_escape_encode__doc__},

static PyValue
_codecs_unicode_escape_encode_impl(PyObject *module, PyObject *str,
                                   const char *errors);

static PyValue
_codecs_unicode_escape_encode(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *str;
    const char *errors = NULL;

    if (!_PyArg_CheckPositional("unicode_escape_encode", nargs, 1, 2)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("unicode_escape_encode", "argument 1", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("unicode_escape_encode", "argument 1", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    str = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    if (nargs < 2) {
        goto skip_optional;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[1]))) {
        _PyArg_BadArgumentValue("unicode_escape_encode", "argument 2", "str or None", PyValue_Borrow(args[1]));
        goto exit;
    }
    if (PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])) == Py_None) {
        errors = NULL;
    }
    else if (PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])))) {
        errors = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        if (errors == NULL) {
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("unicode_escape_encode", "argument 2", "str or None", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        goto exit;
    }
skip_optional:
    return_value = _codecs_unicode_escape_encode_impl(PyValue_AsKnownObjectBorrow(module), str, errors);

exit:
    return return_value;
}

PyDoc_STRVAR(_codecs_raw_unicode_escape_encode__doc__,
"raw_unicode_escape_encode($module, str, errors=None, /)\n"
"--\n"
"\n");

#define _CODECS_RAW_UNICODE_ESCAPE_ENCODE_METHODDEF    \
    {"raw_unicode_escape_encode", _PyCFunction_CAST(_codecs_raw_unicode_escape_encode), METH_VALUE, _codecs_raw_unicode_escape_encode__doc__},

static PyValue
_codecs_raw_unicode_escape_encode_impl(PyObject *module, PyObject *str,
                                       const char *errors);

static PyValue
_codecs_raw_unicode_escape_encode(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *str;
    const char *errors = NULL;

    if (!_PyArg_CheckPositional("raw_unicode_escape_encode", nargs, 1, 2)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("raw_unicode_escape_encode", "argument 1", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("raw_unicode_escape_encode", "argument 1", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    str = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    if (nargs < 2) {
        goto skip_optional;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[1]))) {
        _PyArg_BadArgumentValue("raw_unicode_escape_encode", "argument 2", "str or None", PyValue_Borrow(args[1]));
        goto exit;
    }
    if (PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])) == Py_None) {
        errors = NULL;
    }
    else if (PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])))) {
        errors = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        if (errors == NULL) {
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("raw_unicode_escape_encode", "argument 2", "str or None", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        goto exit;
    }
skip_optional:
    return_value = _codecs_raw_unicode_escape_encode_impl(PyValue_AsKnownObjectBorrow(module), str, errors);

exit:
    return return_value;
}

PyDoc_STRVAR(_codecs_latin_1_encode__doc__,
"latin_1_encode($module, str, errors=None, /)\n"
"--\n"
"\n");

#define _CODECS_LATIN_1_ENCODE_METHODDEF    \
    {"latin_1_encode", _PyCFunction_CAST(_codecs_latin_1_encode), METH_VALUE, _codecs_latin_1_encode__doc__},

static PyValue
_codecs_latin_1_encode_impl(PyObject *module, PyObject *str,
                            const char *errors);

static PyValue
_codecs_latin_1_encode(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *str;
    const char *errors = NULL;

    if (!_PyArg_CheckPositional("latin_1_encode", nargs, 1, 2)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("latin_1_encode", "argument 1", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("latin_1_encode", "argument 1", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    str = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    if (nargs < 2) {
        goto skip_optional;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[1]))) {
        _PyArg_BadArgumentValue("latin_1_encode", "argument 2", "str or None", PyValue_Borrow(args[1]));
        goto exit;
    }
    if (PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])) == Py_None) {
        errors = NULL;
    }
    else if (PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])))) {
        errors = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        if (errors == NULL) {
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("latin_1_encode", "argument 2", "str or None", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        goto exit;
    }
skip_optional:
    return_value = _codecs_latin_1_encode_impl(PyValue_AsKnownObjectBorrow(module), str, errors);

exit:
    return return_value;
}

PyDoc_STRVAR(_codecs_ascii_encode__doc__,
"ascii_encode($module, str, errors=None, /)\n"
"--\n"
"\n");

#define _CODECS_ASCII_ENCODE_METHODDEF    \
    {"ascii_encode", _PyCFunction_CAST(_codecs_ascii_encode), METH_VALUE, _codecs_ascii_encode__doc__},

static PyValue
_codecs_ascii_encode_impl(PyObject *module, PyObject *str,
                          const char *errors);

static PyValue
_codecs_ascii_encode(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *str;
    const char *errors = NULL;

    if (!_PyArg_CheckPositional("ascii_encode", nargs, 1, 2)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("ascii_encode", "argument 1", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("ascii_encode", "argument 1", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    str = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    if (nargs < 2) {
        goto skip_optional;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[1]))) {
        _PyArg_BadArgumentValue("ascii_encode", "argument 2", "str or None", PyValue_Borrow(args[1]));
        goto exit;
    }
    if (PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])) == Py_None) {
        errors = NULL;
    }
    else if (PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])))) {
        errors = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        if (errors == NULL) {
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("ascii_encode", "argument 2", "str or None", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        goto exit;
    }
skip_optional:
    return_value = _codecs_ascii_encode_impl(PyValue_AsKnownObjectBorrow(module), str, errors);

exit:
    return return_value;
}

PyDoc_STRVAR(_codecs_charmap_encode__doc__,
"charmap_encode($module, str, errors=None, mapping=None, /)\n"
"--\n"
"\n");

#define _CODECS_CHARMAP_ENCODE_METHODDEF    \
    {"charmap_encode", _PyCFunction_CAST(_codecs_charmap_encode), METH_VALUE, _codecs_charmap_encode__doc__},

static PyValue
_codecs_charmap_encode_impl(PyObject *module, PyObject *str,
                            const char *errors, PyValueBorrow mapping);

static PyValue
_codecs_charmap_encode(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *str;
    const char *errors = NULL;
    PyValueBorrow mapping = PyValue_Borrow(PyValue_None);

    if (!_PyArg_CheckPositional("charmap_encode", nargs, 1, 3)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("charmap_encode", "argument 1", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("charmap_encode", "argument 1", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    str = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    if (nargs < 2) {
        goto skip_optional;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[1]))) {
        _PyArg_BadArgumentValue("charmap_encode", "argument 2", "str or None", PyValue_Borrow(args[1]));
        goto exit;
    }
    if (PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])) == Py_None) {
        errors = NULL;
    }
    else if (PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])))) {
        errors = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        if (errors == NULL) {
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("charmap_encode", "argument 2", "str or None", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    mapping = PyValue_Borrow(args[2]);
skip_optional:
    return_value = _codecs_charmap_encode_impl(PyValue_AsKnownObjectBorrow(module), str, errors, mapping);

exit:
    return return_value;
}

PyDoc_STRVAR(_codecs_charmap_build__doc__,
"charmap_build($module, map, /)\n"
"--\n"
"\n");

#define _CODECS_CHARMAP_BUILD_METHODDEF    \
    {"charmap_build", _PyCFunction_CAST(_codecs_charmap_build), METH_VALUE|METH_O, _codecs_charmap_build__doc__},

static PyValue
_codecs_charmap_build_impl(PyObject *module, PyObject *map);

static PyValue
_codecs_charmap_build(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *map;

    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("charmap_build", "argument", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("charmap_build", "argument", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    map = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    return_value = _codecs_charmap_build_impl(PyValue_AsKnownObjectBorrow(module), map);

exit:
    return return_value;
}

#if defined(MS_WINDOWS)

PyDoc_STRVAR(_codecs_mbcs_encode__doc__,
"mbcs_encode($module, str, errors=None, /)\n"
"--\n"
"\n");

#define _CODECS_MBCS_ENCODE_METHODDEF    \
    {"mbcs_encode", _PyCFunction_CAST(_codecs_mbcs_encode), METH_VALUE, _codecs_mbcs_encode__doc__},

static PyValue
_codecs_mbcs_encode_impl(PyObject *module, PyObject *str, const char *errors);

static PyValue
_codecs_mbcs_encode(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *str;
    const char *errors = NULL;

    if (!_PyArg_CheckPositional("mbcs_encode", nargs, 1, 2)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("mbcs_encode", "argument 1", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("mbcs_encode", "argument 1", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    str = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    if (nargs < 2) {
        goto skip_optional;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[1]))) {
        _PyArg_BadArgumentValue("mbcs_encode", "argument 2", "str or None", PyValue_Borrow(args[1]));
        goto exit;
    }
    if (PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])) == Py_None) {
        errors = NULL;
    }
    else if (PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])))) {
        errors = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        if (errors == NULL) {
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("mbcs_encode", "argument 2", "str or None", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        goto exit;
    }
skip_optional:
    return_value = _codecs_mbcs_encode_impl(PyValue_AsKnownObjectBorrow(module), str, errors);

exit:
    return return_value;
}

#endif /* defined(MS_WINDOWS) */

#if defined(MS_WINDOWS)

PyDoc_STRVAR(_codecs_oem_encode__doc__,
"oem_encode($module, str, errors=None, /)\n"
"--\n"
"\n");

#define _CODECS_OEM_ENCODE_METHODDEF    \
    {"oem_encode", _PyCFunction_CAST(_codecs_oem_encode), METH_VALUE, _codecs_oem_encode__doc__},

static PyValue
_codecs_oem_encode_impl(PyObject *module, PyObject *str, const char *errors);

static PyValue
_codecs_oem_encode(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *str;
    const char *errors = NULL;

    if (!_PyArg_CheckPositional("oem_encode", nargs, 1, 2)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("oem_encode", "argument 1", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("oem_encode", "argument 1", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    str = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    if (nargs < 2) {
        goto skip_optional;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[1]))) {
        _PyArg_BadArgumentValue("oem_encode", "argument 2", "str or None", PyValue_Borrow(args[1]));
        goto exit;
    }
    if (PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])) == Py_None) {
        errors = NULL;
    }
    else if (PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])))) {
        errors = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        if (errors == NULL) {
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("oem_encode", "argument 2", "str or None", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        goto exit;
    }
skip_optional:
    return_value = _codecs_oem_encode_impl(PyValue_AsKnownObjectBorrow(module), str, errors);

exit:
    return return_value;
}

#endif /* defined(MS_WINDOWS) */

#if defined(MS_WINDOWS)

PyDoc_STRVAR(_codecs_code_page_encode__doc__,
"code_page_encode($module, code_page, str, errors=None, /)\n"
"--\n"
"\n");

#define _CODECS_CODE_PAGE_ENCODE_METHODDEF    \
    {"code_page_encode", _PyCFunction_CAST(_codecs_code_page_encode), METH_VALUE, _codecs_code_page_encode__doc__},

static PyValue
_codecs_code_page_encode_impl(PyObject *module, int code_page, PyObject *str,
                              const char *errors);

static PyValue
_codecs_code_page_encode(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int code_page;
    PyObject *str;
    const char *errors = NULL;

    if (!_PyArg_CheckPositional("code_page_encode", nargs, 2, 3)) {
        goto exit;
    }
    code_page = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (code_page == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[1]))) {
        _PyArg_BadArgumentValue("code_page_encode", "argument 2", "str", PyValue_Borrow(args[1]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])))) {
        _PyArg_BadArgument("code_page_encode", "argument 2", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        goto exit;
    }
    str = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1]));
    if (nargs < 3) {
        goto skip_optional;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[2]))) {
        _PyArg_BadArgumentValue("code_page_encode", "argument 3", "str or None", PyValue_Borrow(args[2]));
        goto exit;
    }
    if (PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[2])) == Py_None) {
        errors = NULL;
    }
    else if (PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[2])))) {
        errors = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[2])));
        if (errors == NULL) {
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("code_page_encode", "argument 3", "str or None", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[2])));
        goto exit;
    }
skip_optional:
    return_value = _codecs_code_page_encode_impl(PyValue_AsKnownObjectBorrow(module), code_page, str, errors);

exit:
    return return_value;
}

#endif /* defined(MS_WINDOWS) */

#if defined(_Py_HAVE_ICONV)

PyDoc_STRVAR(_codecs_iconv_encode__doc__,
"iconv_encode($module, encoding, str, errors=None, /)\n"
"--\n"
"\n");

#define _CODECS_ICONV_ENCODE_METHODDEF    \
    {"iconv_encode", _PyCFunction_CAST(_codecs_iconv_encode), METH_VALUE, _codecs_iconv_encode__doc__},

static PyValue
_codecs_iconv_encode_impl(PyObject *module, const char *encoding,
                          PyObject *str, const char *errors);

static PyValue
_codecs_iconv_encode(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    const char *encoding;
    PyObject *str;
    const char *errors = NULL;

    if (!_PyArg_CheckPositional("iconv_encode", nargs, 2, 3)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("iconv_encode", "argument 1", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("iconv_encode", "argument 1", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    encoding = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
    if (encoding == NULL) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[1]))) {
        _PyArg_BadArgumentValue("iconv_encode", "argument 2", "str", PyValue_Borrow(args[1]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])))) {
        _PyArg_BadArgument("iconv_encode", "argument 2", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        goto exit;
    }
    str = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1]));
    if (nargs < 3) {
        goto skip_optional;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[2]))) {
        _PyArg_BadArgumentValue("iconv_encode", "argument 3", "str or None", PyValue_Borrow(args[2]));
        goto exit;
    }
    if (PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[2])) == Py_None) {
        errors = NULL;
    }
    else if (PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[2])))) {
        errors = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[2])));
        if (errors == NULL) {
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("iconv_encode", "argument 3", "str or None", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[2])));
        goto exit;
    }
skip_optional:
    return_value = _codecs_iconv_encode_impl(PyValue_AsKnownObjectBorrow(module), encoding, str, errors);

exit:
    return return_value;
}

#endif /* defined(_Py_HAVE_ICONV) */

PyDoc_STRVAR(_codecs_register_error__doc__,
"register_error($module, errors, handler, /)\n"
"--\n"
"\n"
"Register the specified error handler under the name errors.\n"
"\n"
"handler must be a callable object, that will be called with an exception\n"
"instance containing information about the location of the\n"
"encoding/decoding error and must return a (replacement, new position)\n"
"tuple.");

#define _CODECS_REGISTER_ERROR_METHODDEF    \
    {"register_error", _PyCFunction_CAST(_codecs_register_error), METH_VALUE, _codecs_register_error__doc__},

static PyValue
_codecs_register_error_impl(PyObject *module, const char *errors,
                            PyValueBorrow handler);

static PyValue
_codecs_register_error(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    const char *errors;
    PyValueBorrow handler;

    if (!_PyArg_CheckPositional("register_error", nargs, 2, 2)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("register_error", "argument 1", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("register_error", "argument 1", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    errors = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
    if (errors == NULL) {
        goto exit;
    }
    handler = PyValue_Borrow(args[1]);
    return_value = _codecs_register_error_impl(PyValue_AsKnownObjectBorrow(module), errors, handler);

exit:
    return return_value;
}

PyDoc_STRVAR(_codecs__unregister_error__doc__,
"_unregister_error($module, errors, /)\n"
"--\n"
"\n"
"Un-register the specified error handler for the error handling `errors\'.\n"
"\n"
"Only custom error handlers can be un-registered. An exception is raised\n"
"if the error handling is a built-in one (e.g., \'strict\'), or if an error\n"
"occurs.\n"
"\n"
"Otherwise, this returns True if a custom handler has been successfully\n"
"un-registered, and False if no custom handler for the specified error\n"
"handling exists.");

#define _CODECS__UNREGISTER_ERROR_METHODDEF    \
    {"_unregister_error", _PyCFunction_CAST(_codecs__unregister_error), METH_VALUE|METH_O, _codecs__unregister_error__doc__},

static int
_codecs__unregister_error_impl(PyObject *module, const char *errors);

static PyValue
_codecs__unregister_error(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    const char *errors;
    int _return_value;

    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("_unregister_error", "argument", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("_unregister_error", "argument", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    errors = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
    if (errors == NULL) {
        goto exit;
    }
    _return_value = _codecs__unregister_error_impl(PyValue_AsKnownObjectBorrow(module), errors);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _return_value ? PyValue_True : PyValue_False;

exit:
    return return_value;
}

PyDoc_STRVAR(_codecs_lookup_error__doc__,
"lookup_error($module, name, /)\n"
"--\n"
"\n"
"lookup_error(errors) -> handler\n"
"\n"
"Return the error handler for the specified error handling name or raise\n"
"a LookupError, if no handler exists under this name.");

#define _CODECS_LOOKUP_ERROR_METHODDEF    \
    {"lookup_error", _PyCFunction_CAST(_codecs_lookup_error), METH_VALUE|METH_O, _codecs_lookup_error__doc__},

static PyValue
_codecs_lookup_error_impl(PyObject *module, const char *name);

static PyValue
_codecs_lookup_error(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    const char *name;

    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("lookup_error", "argument", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("lookup_error", "argument", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    name = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
    if (name == NULL) {
        goto exit;
    }
    return_value = _codecs_lookup_error_impl(PyValue_AsKnownObjectBorrow(module), name);

exit:
    return return_value;
}

PyDoc_STRVAR(_codecs__normalize_encoding__doc__,
"_normalize_encoding($module, /, encoding)\n"
"--\n"
"\n"
"Normalize an encoding name *encoding*.\n"
"\n"
"Used for encodings.normalize_encoding. Does not convert to lower case.");

#define _CODECS__NORMALIZE_ENCODING_METHODDEF    \
    {"_normalize_encoding", _PyCFunction_CAST(_codecs__normalize_encoding), METH_VALUE|METH_KEYWORDS, _codecs__normalize_encoding__doc__},

static PyValue
_codecs__normalize_encoding_impl(PyObject *module, PyObject *encoding);

static PyValue
_codecs__normalize_encoding(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(encoding)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"encoding", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "_normalize_encoding",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    PyObject *encoding;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[_idx[0]]))) {
        _PyArg_BadArgumentValue("_normalize_encoding", "argument 'encoding'", "str", PyValue_Borrow(args[_idx[0]]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])))) {
        _PyArg_BadArgument("_normalize_encoding", "argument 'encoding'", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])));
        goto exit;
    }
    encoding = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]]));
    return_value = _codecs__normalize_encoding_impl(PyValue_AsKnownObjectBorrow(module), encoding);

exit:
    return return_value;
}

#ifndef _CODECS_MBCS_DECODE_METHODDEF
    #define _CODECS_MBCS_DECODE_METHODDEF
#endif /* !defined(_CODECS_MBCS_DECODE_METHODDEF) */

#ifndef _CODECS_OEM_DECODE_METHODDEF
    #define _CODECS_OEM_DECODE_METHODDEF
#endif /* !defined(_CODECS_OEM_DECODE_METHODDEF) */

#ifndef _CODECS_CODE_PAGE_DECODE_METHODDEF
    #define _CODECS_CODE_PAGE_DECODE_METHODDEF
#endif /* !defined(_CODECS_CODE_PAGE_DECODE_METHODDEF) */

#ifndef _CODECS_ICONV_DECODE_METHODDEF
    #define _CODECS_ICONV_DECODE_METHODDEF
#endif /* !defined(_CODECS_ICONV_DECODE_METHODDEF) */

#ifndef _CODECS_MBCS_ENCODE_METHODDEF
    #define _CODECS_MBCS_ENCODE_METHODDEF
#endif /* !defined(_CODECS_MBCS_ENCODE_METHODDEF) */

#ifndef _CODECS_OEM_ENCODE_METHODDEF
    #define _CODECS_OEM_ENCODE_METHODDEF
#endif /* !defined(_CODECS_OEM_ENCODE_METHODDEF) */

#ifndef _CODECS_CODE_PAGE_ENCODE_METHODDEF
    #define _CODECS_CODE_PAGE_ENCODE_METHODDEF
#endif /* !defined(_CODECS_CODE_PAGE_ENCODE_METHODDEF) */

#ifndef _CODECS_ICONV_ENCODE_METHODDEF
    #define _CODECS_ICONV_ENCODE_METHODDEF
#endif /* !defined(_CODECS_ICONV_ENCODE_METHODDEF) */
/*[clinic end generated code: output=9a80b1440e1f5ce2 input=a9049054013a1b77]*/
