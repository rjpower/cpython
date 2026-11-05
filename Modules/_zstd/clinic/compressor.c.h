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
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywordsValue()
#include "pycore_tuple.h"         // _PyTuple_ITEMS()
#include "pycore_value_call.h"    // METH_VALUE
#include "pycore_value_slots.h"   // _PyValue_THUNK_EXIT()

PyDoc_STRVAR(_zstd_ZstdCompressor_new__doc__,
"ZstdCompressor(level=None, options=None, zstd_dict=None)\n"
"--\n"
"\n"
"Create a compressor object for compressing data incrementally.\n"
"\n"
"  level\n"
"    The compression level to use. Defaults to COMPRESSION_LEVEL_DEFAULT.\n"
"  options\n"
"    A dict object that contains advanced compression parameters.\n"
"  zstd_dict\n"
"    A ZstdDict object, a pre-trained Zstandard dictionary.\n"
"\n"
"Thread-safe at method level.  For one-shot compression, use the\n"
"compress() function instead.");

static PyValue
_zstd_ZstdCompressor_new_impl(PyTypeObject *type, PyValueBorrow level,
                              PyValueBorrow options, PyValueBorrow zstd_dict);

static PyValue
_zstd_ZstdCompressor_new_value(PyTypeObject *type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(level)), _PyValue_STATIC_IMMORTAL(&_Py_ID(options)), _PyValue_STATIC_IMMORTAL(&_Py_ID(zstd_dict)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"level", "options", "zstd_dict", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "ZstdCompressor",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    PyValueBorrow level = PyValue_Borrow(PyValue_None);
    PyValueBorrow options = PyValue_Borrow(PyValue_None);
    PyValueBorrow zstd_dict = PyValue_Borrow(PyValue_None);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, _idx, 3)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        level = PyValue_Borrow(args[_idx[0]]);
    }
    if (_idx[1] >= 0) {
        options = PyValue_Borrow(args[_idx[1]]);
    }
    if (_idx[2] >= 0) {
        zstd_dict = PyValue_Borrow(args[_idx[2]]);
    }
    return_value = _zstd_ZstdCompressor_new_impl(type, level, options, zstd_dict);

exit:
    return return_value;
}

Py_VALUE_ENTRY static PyObject *
_zstd_ZstdCompressor_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _PyValue_THUNK_EXIT(_zstd_ZstdCompressor_new_value(type, _PyTuple_ITEMS(args), nargs, NULL), BOX_SITE_CALL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return NULL;
    }
    PyValue result = _zstd_ZstdCompressor_new_value(type, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return _PyValue_THUNK_EXIT(result, BOX_SITE_CALL);
}

PyDoc_STRVAR(_zstd_ZstdCompressor_compress__doc__,
"compress($self, /, data, mode=ZstdCompressor.CONTINUE)\n"
"--\n"
"\n"
"Provide data to the compressor object.\n"
"\n"
"  mode\n"
"    Can be these 3 values ZstdCompressor.CONTINUE,\n"
"    ZstdCompressor.FLUSH_BLOCK, ZstdCompressor.FLUSH_FRAME\n"
"\n"
"Return a chunk of compressed data if possible, or b\'\' otherwise.\n"
"When you have finished providing data to the compressor, call the\n"
"flush() method to finish the compression process.");

#define _ZSTD_ZSTDCOMPRESSOR_COMPRESS_METHODDEF    \
    {"compress", _PyCFunction_CAST(_zstd_ZstdCompressor_compress), METH_VALUE|METH_KEYWORDS, _zstd_ZstdCompressor_compress__doc__},

static PyValue
_zstd_ZstdCompressor_compress_impl(ZstdCompressor *self, Py_buffer *data,
                                   int mode);

static PyValue
_zstd_ZstdCompressor_compress(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(data)), _PyValue_STATIC_IMMORTAL(&_Py_ID(mode)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"data", "mode", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "compress",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    Py_buffer data = {NULL, NULL};
    int mode = ZSTD_e_continue;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
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
        mode = _PyValue_AsInt(PyValue_Borrow(args[_idx[1]]));
        if (mode == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    return_value = _zstd_ZstdCompressor_compress_impl((ZstdCompressor *)PyValue_AsKnownObjectBorrow(self), &data, mode);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(_zstd_ZstdCompressor_flush__doc__,
"flush($self, /, mode=ZstdCompressor.FLUSH_FRAME)\n"
"--\n"
"\n"
"Finish the compression process.\n"
"\n"
"  mode\n"
"    Can be these 2 values ZstdCompressor.FLUSH_FRAME,\n"
"    ZstdCompressor.FLUSH_BLOCK\n"
"\n"
"Flush any remaining data left in internal buffers.  Since Zstandard\n"
"data consists of one or more independent frames, the compressor\n"
"object can still be used after this method is called.");

#define _ZSTD_ZSTDCOMPRESSOR_FLUSH_METHODDEF    \
    {"flush", _PyCFunction_CAST(_zstd_ZstdCompressor_flush), METH_VALUE|METH_KEYWORDS, _zstd_ZstdCompressor_flush__doc__},

static PyValue
_zstd_ZstdCompressor_flush_impl(ZstdCompressor *self, int mode);

static PyValue
_zstd_ZstdCompressor_flush(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(mode)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"mode", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "flush",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    int mode = ZSTD_e_end;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        mode = _PyValue_AsInt(PyValue_Borrow(args[_idx[0]]));
        if (mode == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    return_value = _zstd_ZstdCompressor_flush_impl((ZstdCompressor *)PyValue_AsKnownObjectBorrow(self), mode);

exit:
    return return_value;
}

PyDoc_STRVAR(_zstd_ZstdCompressor_set_pledged_input_size__doc__,
"set_pledged_input_size($self, size, /)\n"
"--\n"
"\n"
"Set the uncompressed content size to be written into the frame header.\n"
"\n"
"  size\n"
"    The size of the uncompressed data to be provided to the compressor.\n"
"\n"
"This method can be used to ensure the header of the frame about to\n"
"be written includes the size of the data, unless the\n"
"CompressionParameter.content_size_flag is set to False.\n"
"If last_mode != FLUSH_FRAME, then a RuntimeError is raised.\n"
"\n"
"It is important to ensure that the pledged data size matches the\n"
"actual data size.  If they do not match the compressed output data\n"
"may be corrupted and the final chunk written may be lost.");

#define _ZSTD_ZSTDCOMPRESSOR_SET_PLEDGED_INPUT_SIZE_METHODDEF    \
    {"set_pledged_input_size", _PyCFunction_CAST(_zstd_ZstdCompressor_set_pledged_input_size), METH_VALUE|METH_O, _zstd_ZstdCompressor_set_pledged_input_size__doc__},

static PyValue
_zstd_ZstdCompressor_set_pledged_input_size_impl(ZstdCompressor *self,
                                                 unsigned long long size);

static PyValue
_zstd_ZstdCompressor_set_pledged_input_size(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    unsigned long long size;

    if (!zstd_contentsize_converter(PyValue_Borrow(args[0]), &size)) {
        goto exit;
    }
    return_value = _zstd_ZstdCompressor_set_pledged_input_size_impl((ZstdCompressor *)PyValue_AsKnownObjectBorrow(self), size);

exit:
    return return_value;
}
/*[clinic end generated code: output=b2530afdf73c929a input=a9049054013a1b77]*/
