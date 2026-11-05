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
#include "pycore_modsupport.h"    // _PyArg_BadArgument()
#include "pycore_tuple.h"         // _PyTuple_ITEMS()
#include "pycore_value_call.h"    // METH_VALUE
#include "pycore_value_ops.h"     // _PyValue_Index()
#include "pycore_value_slots.h"   // _PyValue_THUNK_EXIT()

PyDoc_STRVAR(_lzma_LZMACompressor_compress__doc__,
"compress($self, data, /)\n"
"--\n"
"\n"
"Provide data to the compressor object.\n"
"\n"
"Returns a chunk of compressed data if possible, or b\'\' otherwise.\n"
"\n"
"When you have finished providing data to the compressor, call the\n"
"flush() method to finish the compression process.");

#define _LZMA_LZMACOMPRESSOR_COMPRESS_METHODDEF    \
    {"compress", _PyCFunction_CAST(_lzma_LZMACompressor_compress), METH_VALUE|METH_O, _lzma_LZMACompressor_compress__doc__},

static PyValue
_lzma_LZMACompressor_compress_impl(Compressor *self, Py_buffer *data);

static PyValue
_lzma_LZMACompressor_compress(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_buffer data = {NULL, NULL};

    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[0]))->tp_name);
        goto exit;
    }
    if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])), &data, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    return_value = _lzma_LZMACompressor_compress_impl((Compressor *)PyValue_AsKnownObjectBorrow(self), &data);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(_lzma_LZMACompressor_flush__doc__,
"flush($self, /)\n"
"--\n"
"\n"
"Finish the compression process.\n"
"\n"
"Returns the compressed data left in internal buffers.\n"
"\n"
"The compressor object may not be used after this method is called.");

#define _LZMA_LZMACOMPRESSOR_FLUSH_METHODDEF    \
    {"flush", _PyCFunction_CAST(_lzma_LZMACompressor_flush), METH_VALUE|METH_NOARGS, _lzma_LZMACompressor_flush__doc__},

static PyValue
_lzma_LZMACompressor_flush_impl(Compressor *self);

static PyValue
_lzma_LZMACompressor_flush(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _lzma_LZMACompressor_flush_impl((Compressor *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_lzma_LZMADecompressor_decompress__doc__,
"decompress($self, /, data, max_length=-1)\n"
"--\n"
"\n"
"Decompress *data*, returning uncompressed data as bytes.\n"
"\n"
"If *max_length* is nonnegative, returns at most *max_length* bytes\n"
"of decompressed data. If this limit is reached and further output\n"
"can be produced, *self.needs_input* will be set to ``False``.  In\n"
"this case, the next call to *decompress()* may provide *data* as b\'\'\n"
"to obtain more of the output.\n"
"\n"
"If all of the input data was decompressed and returned (either\n"
"because this was less than *max_length* bytes, or because\n"
"*max_length* was negative), *self.needs_input* will be set to True.\n"
"\n"
"Attempting to decompress data after the end of stream is reached\n"
"raises an EOFError.  Any data found after the end of the stream is\n"
"ignored and saved in the unused_data attribute.");

#define _LZMA_LZMADECOMPRESSOR_DECOMPRESS_METHODDEF    \
    {"decompress", _PyCFunction_CAST(_lzma_LZMADecompressor_decompress), METH_VALUE|METH_KEYWORDS, _lzma_LZMADecompressor_decompress__doc__},

static PyValue
_lzma_LZMADecompressor_decompress_impl(Decompressor *self, Py_buffer *data,
                                       Py_ssize_t max_length);

static PyValue
_lzma_LZMADecompressor_decompress(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(data)), _PyValue_STATIC_IMMORTAL(&_Py_ID(max_length)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"data", "max_length", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "decompress",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    Py_buffer data = {NULL, NULL};
    Py_ssize_t max_length = -1;

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
        {
            Py_ssize_t ival = -1;
            PyValue iobj = _PyValue_Index(PyValue_Borrow(args[_idx[1]]));
            if (!PyValue_IsNull(iobj)) {
                ival = _PyValue_IntAsSsize_t(PyValue_Borrow(iobj));
                PyValue_CLOSE(iobj);
            }
            if (ival == -1 && PyErr_Occurred()) {
                goto exit;
            }
            max_length = ival;
        }
    }
    return_value = _lzma_LZMADecompressor_decompress_impl((Decompressor *)PyValue_AsKnownObjectBorrow(self), &data, max_length);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(_lzma_LZMADecompressor__doc__,
"LZMADecompressor(format=FORMAT_AUTO, memlimit=None, filters=None)\n"
"--\n"
"\n"
"Create a decompressor object for decompressing data incrementally.\n"
"\n"
"  format\n"
"    Specifies the container format of the input stream.  If this is\n"
"    FORMAT_AUTO (the default), the decompressor will automatically detect\n"
"    whether the input is FORMAT_XZ or FORMAT_ALONE.  Streams created with\n"
"    FORMAT_RAW cannot be autodetected.\n"
"  memlimit\n"
"    Limit the amount of memory used by the decompressor.  This will cause\n"
"    decompression to fail if the input cannot be decompressed within the\n"
"    given limit.\n"
"  filters\n"
"    A custom filter chain.  This argument is required for FORMAT_RAW, and\n"
"    not accepted with any other format.  When provided, this should be a\n"
"    sequence of dicts, each indicating the ID and options for a single\n"
"    filter.\n"
"\n"
"For one-shot decompression, use the decompress() function instead.");

static PyValue
_lzma_LZMADecompressor_impl(PyTypeObject *type, int format,
                            PyValueBorrow memlimit, PyValueBorrow filters);

static PyValue
_lzma_LZMADecompressor_value(PyTypeObject *type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(format)), _PyValue_STATIC_IMMORTAL(&_Py_ID(memlimit)), _PyValue_STATIC_IMMORTAL(&_Py_ID(filters)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"format", "memlimit", "filters", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "LZMADecompressor",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    int format = FORMAT_AUTO;
    PyValueBorrow memlimit = PyValue_Borrow(PyValue_None);
    PyValueBorrow filters = PyValue_Borrow(PyValue_None);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, _idx, 3)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        format = _PyValue_AsInt(PyValue_Borrow(args[_idx[0]]));
        if (format == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (_idx[1] >= 0) {
        memlimit = PyValue_Borrow(args[_idx[1]]);
    }
    if (_idx[2] >= 0) {
        filters = PyValue_Borrow(args[_idx[2]]);
    }
    return_value = _lzma_LZMADecompressor_impl(type, format, memlimit, filters);

exit:
    return return_value;
}

Py_VALUE_ENTRY static PyObject *
_lzma_LZMADecompressor(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _PyValue_THUNK_EXIT(_lzma_LZMADecompressor_value(type, _PyTuple_ITEMS(args), nargs, NULL), BOX_SITE_CALL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return NULL;
    }
    PyValue result = _lzma_LZMADecompressor_value(type, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return _PyValue_THUNK_EXIT(result, BOX_SITE_CALL);
}

PyDoc_STRVAR(_lzma_is_check_supported__doc__,
"is_check_supported($module, check_id, /)\n"
"--\n"
"\n"
"Test whether the given integrity check is supported.\n"
"\n"
"Always returns True for CHECK_NONE and CHECK_CRC32.");

#define _LZMA_IS_CHECK_SUPPORTED_METHODDEF    \
    {"is_check_supported", _PyCFunction_CAST(_lzma_is_check_supported), METH_VALUE|METH_O, _lzma_is_check_supported__doc__},

static PyValue
_lzma_is_check_supported_impl(PyObject *module, int check_id);

static PyValue
_lzma_is_check_supported(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int check_id;

    check_id = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (check_id == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _lzma_is_check_supported_impl(PyValue_AsKnownObjectBorrow(module), check_id);

exit:
    return return_value;
}

PyDoc_STRVAR(_lzma__decode_filter_properties__doc__,
"_decode_filter_properties($module, filter_id, encoded_props, /)\n"
"--\n"
"\n"
"Return a bytes object encoding the options (properties) of the filter specified by *filter* (a dict).\n"
"\n"
"The result does not include the filter ID itself, only the options.");

#define _LZMA__DECODE_FILTER_PROPERTIES_METHODDEF    \
    {"_decode_filter_properties", _PyCFunction_CAST(_lzma__decode_filter_properties), METH_VALUE, _lzma__decode_filter_properties__doc__},

static PyValue
_lzma__decode_filter_properties_impl(PyObject *module, lzma_vli filter_id,
                                     Py_buffer *encoded_props);

static PyValue
_lzma__decode_filter_properties(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    lzma_vli filter_id;
    Py_buffer encoded_props = {NULL, NULL};

    if (!_PyArg_CheckPositional("_decode_filter_properties", nargs, 2, 2)) {
        goto exit;
    }
    if (!lzma_vli_converter(PyValue_Borrow(args[0]), &filter_id)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[1]))) {
        PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[1]))->tp_name);
        goto exit;
    }
    if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])), &encoded_props, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    return_value = _lzma__decode_filter_properties_impl(PyValue_AsKnownObjectBorrow(module), filter_id, &encoded_props);

exit:
    /* Cleanup for encoded_props */
    if (encoded_props.obj) {
       PyBuffer_Release(&encoded_props);
    }

    return return_value;
}
/*[clinic end generated code: output=d728695910f44f95 input=a9049054013a1b77]*/
