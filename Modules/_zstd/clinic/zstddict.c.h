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
#include "pycore_tuple.h"         // _PyTuple_ITEMS()
#include "pycore_value_call.h"    // METH_VALUE
#include "pycore_value_ops.h"     // _PyValue_IsTrue()
#include "pycore_value_slots.h"   // _PyValue_THUNK_EXIT()

PyDoc_STRVAR(_zstd_ZstdDict_new__doc__,
"ZstdDict(dict_content, /, *, is_raw=False)\n"
"--\n"
"\n"
"Represents a Zstandard dictionary.\n"
"\n"
"  dict_content\n"
"    The content of a Zstandard dictionary as a bytes-like object.\n"
"  is_raw\n"
"    If true, perform no checks on *dict_content*, useful for some\n"
"    advanced cases. Otherwise, check that the content represents\n"
"    a Zstandard dictionary created by the zstd library or CLI.\n"
"\n"
"The dictionary can be used for compression or decompression, and can be\n"
"shared by multiple ZstdCompressor or ZstdDecompressor objects.");

static PyValue
_zstd_ZstdDict_new_impl(PyTypeObject *type, Py_buffer *dict_content,
                        int is_raw);

static PyValue
_zstd_ZstdDict_new_value(PyTypeObject *type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(is_raw)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "is_raw", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "ZstdDict",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    Py_buffer dict_content = {NULL, NULL};
    int is_raw = 0;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[_idx[0]]))) {
        PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[_idx[0]]))->tp_name);
        goto exit;
    }
    if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])), &dict_content, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (_idx[1] >= 0) {
        is_raw = _PyValue_IsTrue(PyValue_Borrow(args[_idx[1]]));
        if (is_raw < 0) {
            goto exit;
        }
    }
    return_value = _zstd_ZstdDict_new_impl(type, &dict_content, is_raw);

exit:
    /* Cleanup for dict_content */
    if (dict_content.obj) {
       PyBuffer_Release(&dict_content);
    }

    return return_value;
}

Py_VALUE_ENTRY static PyObject *
_zstd_ZstdDict_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _PyValue_THUNK_EXIT(_zstd_ZstdDict_new_value(type, _PyTuple_ITEMS(args), nargs, NULL), BOX_SITE_CALL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return NULL;
    }
    PyValue result = _zstd_ZstdDict_new_value(type, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return _PyValue_THUNK_EXIT(result, BOX_SITE_CALL);
}

PyDoc_STRVAR(_zstd_ZstdDict_dict_content__doc__,
"The content of a Zstandard dictionary, as a bytes object.");

static PyObject *
_zstd_ZstdDict_dict_content_get_impl(ZstdDict *self);

static PyObject *
_zstd_ZstdDict_dict_content_get(PyObject *self, void *Py_UNUSED(context))
{
    return _zstd_ZstdDict_dict_content_get_impl((ZstdDict *)self);
}

PyDoc_STRVAR(_zstd_ZstdDict_as_digested_dict__doc__,
"Load as a digested dictionary to compressor.\n"
"\n"
"Pass this attribute as zstd_dict argument:\n"
"compress(dat, zstd_dict=zd.as_digested_dict)\n"
"\n"
"1. Some advanced compression parameters of compressor may be\n"
"   overridden by parameters of digested dictionary.\n"
"2. ZstdDict has a digested dictionaries cache for each compression\n"
"   level.  It\'s faster when loading again a digested dictionary with\n"
"   the same compression level.\n"
"3. No need to use this for decompression.");

static PyObject *
_zstd_ZstdDict_as_digested_dict_get_impl(ZstdDict *self);

static PyObject *
_zstd_ZstdDict_as_digested_dict_get(PyObject *self, void *Py_UNUSED(context))
{
    return _zstd_ZstdDict_as_digested_dict_get_impl((ZstdDict *)self);
}

PyDoc_STRVAR(_zstd_ZstdDict_as_undigested_dict__doc__,
"Load as an undigested dictionary to compressor.\n"
"\n"
"Pass this attribute as zstd_dict argument:\n"
"compress(dat, zstd_dict=zd.as_undigested_dict)\n"
"\n"
"1. The advanced compression parameters of compressor will not be\n"
"   overridden.\n"
"2. Loading an undigested dictionary is costly. If load an undigested\n"
"   dictionary multiple times, consider reusing a compressor object.\n"
"3. No need to use this for decompression.");

static PyObject *
_zstd_ZstdDict_as_undigested_dict_get_impl(ZstdDict *self);

static PyObject *
_zstd_ZstdDict_as_undigested_dict_get(PyObject *self, void *Py_UNUSED(context))
{
    return _zstd_ZstdDict_as_undigested_dict_get_impl((ZstdDict *)self);
}

PyDoc_STRVAR(_zstd_ZstdDict_as_prefix__doc__,
"Load as a prefix to compressor/decompressor.\n"
"\n"
"Pass this attribute as zstd_dict argument:\n"
"compress(dat, zstd_dict=zd.as_prefix)\n"
"\n"
"1. Prefix is compatible with long distance matching, while\n"
"   dictionary is not.\n"
"2. It only works for the first frame, then the\n"
"   compressor/decompressor will return to no prefix state.\n"
"3. When decompressing, must use the same prefix as when compressing.");

static PyObject *
_zstd_ZstdDict_as_prefix_get_impl(ZstdDict *self);

static PyObject *
_zstd_ZstdDict_as_prefix_get(PyObject *self, void *Py_UNUSED(context))
{
    return _zstd_ZstdDict_as_prefix_get_impl((ZstdDict *)self);
}
#define _ZSTD_ZSTDDICT_DICT_CONTENT_GETSETDEF {"dict_content", (getter)_zstd_ZstdDict_dict_content_get, (setter)NULL, _zstd_ZstdDict_dict_content__doc__},

#define _ZSTD_ZSTDDICT_AS_DIGESTED_DICT_GETSETDEF {"as_digested_dict", (getter)_zstd_ZstdDict_as_digested_dict_get, (setter)NULL, _zstd_ZstdDict_as_digested_dict__doc__},

#define _ZSTD_ZSTDDICT_AS_UNDIGESTED_DICT_GETSETDEF {"as_undigested_dict", (getter)_zstd_ZstdDict_as_undigested_dict_get, (setter)NULL, _zstd_ZstdDict_as_undigested_dict__doc__},

#define _ZSTD_ZSTDDICT_AS_PREFIX_GETSETDEF {"as_prefix", (getter)_zstd_ZstdDict_as_prefix_get, (setter)NULL, _zstd_ZstdDict_as_prefix__doc__},

/*[clinic end generated code: output=bb30321bca8e9ec9 input=a9049054013a1b77]*/
