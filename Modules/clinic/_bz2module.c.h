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
#include "pycore_tuple.h"         // _PyTuple_ITEMS()
#include "pycore_value_call.h"    // METH_VALUE
#include "pycore_value_ops.h"     // _PyValue_Index()
#include "pycore_value_slots.h"   // _PyValue_THUNK_EXIT()

PyDoc_STRVAR(_bz2_BZ2Compressor_compress__doc__,
"compress($self, data, /)\n"
"--\n"
"\n"
"Provide data to the compressor object.\n"
"\n"
"Returns a chunk of compressed data if possible, or b\'\' otherwise.\n"
"\n"
"When you have finished providing data to the compressor, call the\n"
"flush() method to finish the compression process.");

#define _BZ2_BZ2COMPRESSOR_COMPRESS_METHODDEF    \
    {"compress", _PyCFunction_CAST(_bz2_BZ2Compressor_compress), METH_VALUE|METH_O, _bz2_BZ2Compressor_compress__doc__},

static PyValue
_bz2_BZ2Compressor_compress_impl(BZ2Compressor *self, Py_buffer *data);

static PyValue
_bz2_BZ2Compressor_compress(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
    return_value = _bz2_BZ2Compressor_compress_impl((BZ2Compressor *)PyValue_AsKnownObjectBorrow(self), &data);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(_bz2_BZ2Compressor_flush__doc__,
"flush($self, /)\n"
"--\n"
"\n"
"Finish the compression process.\n"
"\n"
"Returns the compressed data left in internal buffers.\n"
"\n"
"The compressor object may not be used after this method is called.");

#define _BZ2_BZ2COMPRESSOR_FLUSH_METHODDEF    \
    {"flush", _PyCFunction_CAST(_bz2_BZ2Compressor_flush), METH_VALUE|METH_NOARGS, _bz2_BZ2Compressor_flush__doc__},

static PyValue
_bz2_BZ2Compressor_flush_impl(BZ2Compressor *self);

static PyValue
_bz2_BZ2Compressor_flush(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _bz2_BZ2Compressor_flush_impl((BZ2Compressor *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_bz2_BZ2Compressor__doc__,
"BZ2Compressor(compresslevel=9, /)\n"
"--\n"
"\n"
"Create a compressor object for compressing data incrementally.\n"
"\n"
"  compresslevel\n"
"    Compression level, as a number between 1 and 9.\n"
"\n"
"For one-shot compression, use the compress() function instead.");

static PyValue
_bz2_BZ2Compressor_impl(PyTypeObject *type, int compresslevel);

static PyValue
_bz2_BZ2Compressor_value(PyTypeObject *type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyTypeObject *base_tp = clinic_state()->bz2_compressor_type;
    int compresslevel = 9;

    if ((type == base_tp || type->tp_init == base_tp->tp_init) &&
        !_PyArg_NoKwnames("BZ2Compressor", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("BZ2Compressor", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    compresslevel = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (compresslevel == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = _bz2_BZ2Compressor_impl(type, compresslevel);

exit:
    return return_value;
}

Py_VALUE_ENTRY static PyObject *
_bz2_BZ2Compressor(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _PyValue_THUNK_EXIT(_bz2_BZ2Compressor_value(type, _PyTuple_ITEMS(args), nargs, NULL), BOX_SITE_CALL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return NULL;
    }
    PyValue result = _bz2_BZ2Compressor_value(type, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return _PyValue_THUNK_EXIT(result, BOX_SITE_CALL);
}

PyDoc_STRVAR(_bz2_BZ2Decompressor_decompress__doc__,
"decompress($self, /, data, max_length=-1)\n"
"--\n"
"\n"
"Decompress *data*, returning uncompressed data as bytes.\n"
"\n"
"If *max_length* is nonnegative, returns at most *max_length* bytes\n"
"of decompressed data.  If this limit is reached and further output\n"
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

#define _BZ2_BZ2DECOMPRESSOR_DECOMPRESS_METHODDEF    \
    {"decompress", _PyCFunction_CAST(_bz2_BZ2Decompressor_decompress), METH_VALUE|METH_KEYWORDS, _bz2_BZ2Decompressor_decompress__doc__},

static PyValue
_bz2_BZ2Decompressor_decompress_impl(BZ2Decompressor *self, Py_buffer *data,
                                     Py_ssize_t max_length);

static PyValue
_bz2_BZ2Decompressor_decompress(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
    return_value = _bz2_BZ2Decompressor_decompress_impl((BZ2Decompressor *)PyValue_AsKnownObjectBorrow(self), &data, max_length);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(_bz2_BZ2Decompressor__doc__,
"BZ2Decompressor()\n"
"--\n"
"\n"
"Create a decompressor object for decompressing data incrementally.\n"
"\n"
"For one-shot decompression, use the decompress() function instead.");

static PyValue
_bz2_BZ2Decompressor_impl(PyTypeObject *type);

static PyValue
_bz2_BZ2Decompressor_value(PyTypeObject *type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyTypeObject *base_tp = clinic_state()->bz2_decompressor_type;

    if ((type == base_tp || type->tp_init == base_tp->tp_init) &&
        !_PyArg_NoPositionalStack("BZ2Decompressor", nargs)) {
        goto exit;
    }
    if ((type == base_tp || type->tp_init == base_tp->tp_init) &&
        !_PyArg_NoKwnames("BZ2Decompressor", kwnames)) {
        goto exit;
    }
    return_value = _bz2_BZ2Decompressor_impl(type);

exit:
    return return_value;
}

Py_VALUE_ENTRY static PyObject *
_bz2_BZ2Decompressor(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _PyValue_THUNK_EXIT(_bz2_BZ2Decompressor_value(type, _PyTuple_ITEMS(args), nargs, NULL), BOX_SITE_CALL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return NULL;
    }
    PyValue result = _bz2_BZ2Decompressor_value(type, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return _PyValue_THUNK_EXIT(result, BOX_SITE_CALL);
}
/*[clinic end generated code: output=1691c531a20d966f input=a9049054013a1b77]*/
