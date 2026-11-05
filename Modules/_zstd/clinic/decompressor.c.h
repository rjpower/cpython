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

PyDoc_STRVAR(_zstd_ZstdDecompressor_new__doc__,
"ZstdDecompressor(zstd_dict=None, options=None)\n"
"--\n"
"\n"
"Create a decompressor object for decompressing data incrementally.\n"
"\n"
"  zstd_dict\n"
"    A ZstdDict object, a pre-trained Zstandard dictionary.\n"
"  options\n"
"    A dict object that contains advanced decompression parameters.\n"
"\n"
"Thread-safe at method level.  For one-shot decompression, use the\n"
"decompress() function instead.");

static PyValue
_zstd_ZstdDecompressor_new_impl(PyTypeObject *type, PyValueBorrow zstd_dict,
                                PyValueBorrow options);

static PyValue
_zstd_ZstdDecompressor_new_value(PyTypeObject *type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(zstd_dict)), _PyValue_STATIC_IMMORTAL(&_Py_ID(options)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"zstd_dict", "options", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "ZstdDecompressor",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    PyValueBorrow zstd_dict = PyValue_Borrow(PyValue_None);
    PyValueBorrow options = PyValue_Borrow(PyValue_None);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        zstd_dict = PyValue_Borrow(args[_idx[0]]);
    }
    if (_idx[1] >= 0) {
        options = PyValue_Borrow(args[_idx[1]]);
    }
    return_value = _zstd_ZstdDecompressor_new_impl(type, zstd_dict, options);

exit:
    return return_value;
}

Py_VALUE_ENTRY static PyObject *
_zstd_ZstdDecompressor_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _PyValue_THUNK_EXIT(_zstd_ZstdDecompressor_new_value(type, _PyTuple_ITEMS(args), nargs, NULL), BOX_SITE_CALL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return NULL;
    }
    PyValue result = _zstd_ZstdDecompressor_new_value(type, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return _PyValue_THUNK_EXIT(result, BOX_SITE_CALL);
}

PyDoc_STRVAR(_zstd_ZstdDecompressor_unused_data__doc__,
"A bytes object of un-consumed input data.\n"
"\n"
"When ZstdDecompressor object stops after a frame is\n"
"decompressed, unused input data after the frame.  Otherwise this\n"
"will be b\'\'.");

static PyObject *
_zstd_ZstdDecompressor_unused_data_get_impl(ZstdDecompressor *self);

static PyObject *
_zstd_ZstdDecompressor_unused_data_get(PyObject *self, void *Py_UNUSED(context))
{
    return _zstd_ZstdDecompressor_unused_data_get_impl((ZstdDecompressor *)self);
}

PyDoc_STRVAR(_zstd_ZstdDecompressor_decompress__doc__,
"decompress($self, /, data, max_length=-1)\n"
"--\n"
"\n"
"Decompress *data*, returning uncompressed bytes if possible, or b\'\' otherwise.\n"
"\n"
"  data\n"
"    A bytes-like object, Zstandard data to be decompressed.\n"
"  max_length\n"
"    Maximum size of returned data. When it is negative, the size of\n"
"    output buffer is unlimited. When it is nonnegative, returns at\n"
"    most max_length bytes of decompressed data.\n"
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
"Attempting to decompress data after the end of a frame is reached\n"
"raises an EOFError.  Any data found after the end of the frame is\n"
"ignored and saved in the self.unused_data attribute.");

#define _ZSTD_ZSTDDECOMPRESSOR_DECOMPRESS_METHODDEF    \
    {"decompress", _PyCFunction_CAST(_zstd_ZstdDecompressor_decompress), METH_VALUE|METH_KEYWORDS, _zstd_ZstdDecompressor_decompress__doc__},

static PyValue
_zstd_ZstdDecompressor_decompress_impl(ZstdDecompressor *self,
                                       Py_buffer *data,
                                       Py_ssize_t max_length);

static PyValue
_zstd_ZstdDecompressor_decompress(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
    return_value = _zstd_ZstdDecompressor_decompress_impl((ZstdDecompressor *)PyValue_AsKnownObjectBorrow(self), &data, max_length);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}
#define _ZSTD_ZSTDDECOMPRESSOR_UNUSED_DATA_GETSETDEF {"unused_data", (getter)_zstd_ZstdDecompressor_unused_data_get, (setter)NULL, _zstd_ZstdDecompressor_unused_data__doc__},

/*[clinic end generated code: output=fe9cb8b6e62853c6 input=a9049054013a1b77]*/
