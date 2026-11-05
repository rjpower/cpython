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
#include "pycore_value_ops.h"     // _PyValue_Index()
#include "pycore_value_slots.h"   // _PyValue_THUNK_EXIT()

PyDoc_STRVAR(zlib_compress__doc__,
"compress($module, data, /, level=Z_DEFAULT_COMPRESSION, wbits=MAX_WBITS)\n"
"--\n"
"\n"
"Returns a bytes object containing compressed data.\n"
"\n"
"  data\n"
"    Binary data to be compressed.\n"
"  level\n"
"    Compression level, in 0-9 or -1.\n"
"  wbits\n"
"    The window buffer size and container format.");

#define ZLIB_COMPRESS_METHODDEF    \
    {"compress", _PyCFunction_CAST(zlib_compress), METH_VALUE|METH_KEYWORDS, zlib_compress__doc__},

static PyValue
zlib_compress_impl(PyObject *module, Py_buffer *data, int level, int wbits);

static PyValue
zlib_compress(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(level)), _PyValue_STATIC_IMMORTAL(&_Py_ID(wbits)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "level", "wbits", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "compress",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    Py_buffer data = {NULL, NULL};
    int level = Z_DEFAULT_COMPRESSION;
    int wbits = MAX_WBITS;

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
        level = _PyValue_AsInt(PyValue_Borrow(args[_idx[1]]));
        if (level == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (_idx[2] >= 0) {
        wbits = _PyValue_AsInt(PyValue_Borrow(args[_idx[2]]));
        if (wbits == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    return_value = zlib_compress_impl(PyValue_AsKnownObjectBorrow(module), &data, level, wbits);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(zlib_decompress__doc__,
"decompress($module, data, /, wbits=MAX_WBITS, bufsize=DEF_BUF_SIZE)\n"
"--\n"
"\n"
"Returns a bytes object containing the uncompressed data.\n"
"\n"
"  data\n"
"    Compressed data.\n"
"  wbits\n"
"    The window buffer size and container format.\n"
"  bufsize\n"
"    The initial output buffer size.");

#define ZLIB_DECOMPRESS_METHODDEF    \
    {"decompress", _PyCFunction_CAST(zlib_decompress), METH_VALUE|METH_KEYWORDS, zlib_decompress__doc__},

static PyValue
zlib_decompress_impl(PyObject *module, Py_buffer *data, int wbits,
                     Py_ssize_t bufsize);

static PyValue
zlib_decompress(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(wbits)), _PyValue_STATIC_IMMORTAL(&_Py_ID(bufsize)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "wbits", "bufsize", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "decompress",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    Py_buffer data = {NULL, NULL};
    int wbits = MAX_WBITS;
    Py_ssize_t bufsize = DEF_BUF_SIZE;

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
        wbits = _PyValue_AsInt(PyValue_Borrow(args[_idx[1]]));
        if (wbits == -1 && PyErr_Occurred()) {
            goto exit;
        }
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
            bufsize = ival;
            if (bufsize < 0) {
                PyErr_SetString(PyExc_ValueError,
                                "bufsize cannot be negative");
                goto exit;
            }
        }
    }
    return_value = zlib_decompress_impl(PyValue_AsKnownObjectBorrow(module), &data, wbits, bufsize);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(zlib_compressobj__doc__,
"compressobj($module, /, level=Z_DEFAULT_COMPRESSION, method=DEFLATED,\n"
"            wbits=MAX_WBITS, memLevel=DEF_MEM_LEVEL,\n"
"            strategy=Z_DEFAULT_STRATEGY, zdict=<unrepresentable>)\n"
"--\n"
"\n"
"Return a compressor object.\n"
"\n"
"  level\n"
"    The compression level (an integer in the range 0-9 or -1; default is\n"
"    currently equivalent to 6).  Higher compression levels are slower,\n"
"    but produce smaller results.\n"
"  method\n"
"    The compression algorithm.  If given, this must be DEFLATED.\n"
"  wbits\n"
"    +9 to +15: The base-two logarithm of the window size.  Include a zlib\n"
"        container.\n"
"    -9 to -15: Generate a raw stream.\n"
"    +25 to +31: Include a gzip container.\n"
"  memLevel\n"
"    Controls the amount of memory used for internal compression state.\n"
"    Valid values range from 1 to 9.  Higher values result in higher memory\n"
"    usage, faster compression, and smaller output.\n"
"  strategy\n"
"    Used to tune the compression algorithm.  Possible values are\n"
"    Z_DEFAULT_STRATEGY, Z_FILTERED, and Z_HUFFMAN_ONLY.\n"
"  zdict\n"
"    The predefined compression dictionary - a sequence of bytes\n"
"    containing subsequences that are likely to occur in the input data.");

#define ZLIB_COMPRESSOBJ_METHODDEF    \
    {"compressobj", _PyCFunction_CAST(zlib_compressobj), METH_VALUE|METH_KEYWORDS, zlib_compressobj__doc__},

static PyValue
zlib_compressobj_impl(PyObject *module, int level, int method, int wbits,
                      int memLevel, int strategy, Py_buffer *zdict);

static PyValue
zlib_compressobj(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(level)), _PyValue_STATIC_IMMORTAL(&_Py_ID(method)), _PyValue_STATIC_IMMORTAL(&_Py_ID(wbits)), _PyValue_STATIC_IMMORTAL(&_Py_ID(memLevel)), _PyValue_STATIC_IMMORTAL(&_Py_ID(strategy)), _PyValue_STATIC_IMMORTAL(&_Py_ID(zdict)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"level", "method", "wbits", "memLevel", "strategy", "zdict", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "compressobj",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[6];
    int level = Z_DEFAULT_COMPRESSION;
    int method = DEFLATED;
    int wbits = MAX_WBITS;
    int memLevel = DEF_MEM_LEVEL;
    int strategy = Z_DEFAULT_STRATEGY;
    Py_buffer zdict = {NULL, NULL};

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 6, /*minkw*/ 0, /*varpos*/ 0, _idx, 6)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        level = _PyValue_AsInt(PyValue_Borrow(args[_idx[0]]));
        if (level == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (_idx[1] >= 0) {
        method = _PyValue_AsInt(PyValue_Borrow(args[_idx[1]]));
        if (method == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (_idx[2] >= 0) {
        wbits = _PyValue_AsInt(PyValue_Borrow(args[_idx[2]]));
        if (wbits == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (_idx[3] >= 0) {
        memLevel = _PyValue_AsInt(PyValue_Borrow(args[_idx[3]]));
        if (memLevel == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (_idx[4] >= 0) {
        strategy = _PyValue_AsInt(PyValue_Borrow(args[_idx[4]]));
        if (strategy == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (_idx[5] >= 0) {
        if (!PyValue_IsObject(PyValue_Borrow(args[_idx[5]]))) {
            PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[_idx[5]]))->tp_name);
            goto exit;
        }
        if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[5]])), &zdict, PyBUF_SIMPLE) != 0) {
            goto exit;
        }
    }
    return_value = zlib_compressobj_impl(PyValue_AsKnownObjectBorrow(module), level, method, wbits, memLevel, strategy, &zdict);

exit:
    /* Cleanup for zdict */
    if (zdict.obj) {
       PyBuffer_Release(&zdict);
    }

    return return_value;
}

PyDoc_STRVAR(zlib_decompressobj__doc__,
"decompressobj($module, /, wbits=MAX_WBITS, zdict=b\'\')\n"
"--\n"
"\n"
"Return a decompressor object.\n"
"\n"
"  wbits\n"
"    The window buffer size and container format.\n"
"  zdict\n"
"    The predefined compression dictionary.  This must be the same\n"
"    dictionary as used by the compressor that produced the input data.");

#define ZLIB_DECOMPRESSOBJ_METHODDEF    \
    {"decompressobj", _PyCFunction_CAST(zlib_decompressobj), METH_VALUE|METH_KEYWORDS, zlib_decompressobj__doc__},

static PyValue
zlib_decompressobj_impl(PyObject *module, int wbits, PyValueBorrow zdict);

static PyValue
zlib_decompressobj(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(wbits)), _PyValue_STATIC_IMMORTAL(&_Py_ID(zdict)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"wbits", "zdict", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "decompressobj",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    int wbits = MAX_WBITS;
    PyValueBorrow zdict = PyValue_Borrow(PyValue_NULL);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        wbits = _PyValue_AsInt(PyValue_Borrow(args[_idx[0]]));
        if (wbits == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (_idx[1] >= 0) {
        zdict = PyValue_Borrow(args[_idx[1]]);
    }
    return_value = zlib_decompressobj_impl(PyValue_AsKnownObjectBorrow(module), wbits, zdict);

exit:
    return return_value;
}

PyDoc_STRVAR(zlib_Compress_compress__doc__,
"compress($self, data, /)\n"
"--\n"
"\n"
"Returns a bytes object containing compressed data.\n"
"\n"
"  data\n"
"    Binary data to be compressed.\n"
"\n"
"After calling this function, some of the input data may still\n"
"be stored in internal buffers for later processing.\n"
"Call the flush() method to clear these buffers.");

#define ZLIB_COMPRESS_COMPRESS_METHODDEF    \
    {"compress", _PyCFunction_CAST(zlib_Compress_compress), METH_VALUE|METH_METHOD|METH_KEYWORDS, zlib_Compress_compress__doc__},

static PyValue
zlib_Compress_compress_impl(compobject *self, PyTypeObject *cls,
                            Py_buffer *data);

static PyValue
zlib_Compress_compress(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_buffer data = {NULL, NULL};

    if (!_PyArg_NoKwnames("compress", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("compress", nargs, 1, 1)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[0]))->tp_name);
        goto exit;
    }
    if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])), &data, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    return_value = zlib_Compress_compress_impl((compobject *)PyValue_AsKnownObjectBorrow(self), cls, &data);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(zlib_Decompress_decompress__doc__,
"decompress($self, data, /, max_length=0)\n"
"--\n"
"\n"
"Return a bytes object containing the decompressed version of the data.\n"
"\n"
"  data\n"
"    The binary data to decompress.\n"
"  max_length\n"
"    The maximum allowable length of the decompressed data.\n"
"    Unconsumed input data will be stored in\n"
"    the unconsumed_tail attribute.\n"
"\n"
"After calling this function, some of the input data may still be\n"
"stored in internal buffers for later processing.\n"
"Call the flush() method to clear these buffers.");

#define ZLIB_DECOMPRESS_DECOMPRESS_METHODDEF    \
    {"decompress", _PyCFunction_CAST(zlib_Decompress_decompress), METH_VALUE|METH_METHOD|METH_KEYWORDS, zlib_Decompress_decompress__doc__},

static PyValue
zlib_Decompress_decompress_impl(compobject *self, PyTypeObject *cls,
                                Py_buffer *data, Py_ssize_t max_length);

static PyValue
zlib_Decompress_decompress(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(max_length)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "max_length", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "decompress",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    Py_buffer data = {NULL, NULL};
    Py_ssize_t max_length = 0;

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
            if (max_length < 0) {
                PyErr_SetString(PyExc_ValueError,
                                "max_length cannot be negative");
                goto exit;
            }
        }
    }
    return_value = zlib_Decompress_decompress_impl((compobject *)PyValue_AsKnownObjectBorrow(self), cls, &data, max_length);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(zlib_Compress_flush__doc__,
"flush($self, mode=zlib.Z_FINISH, /)\n"
"--\n"
"\n"
"Return a bytes object containing any remaining compressed data.\n"
"\n"
"  mode\n"
"    One of the constants Z_SYNC_FLUSH, Z_FULL_FLUSH, Z_FINISH.\n"
"    If mode == Z_FINISH, the compressor object can no longer be\n"
"    used after calling the flush() method.  Otherwise, more data\n"
"    can still be compressed.");

#define ZLIB_COMPRESS_FLUSH_METHODDEF    \
    {"flush", _PyCFunction_CAST(zlib_Compress_flush), METH_VALUE|METH_METHOD|METH_KEYWORDS, zlib_Compress_flush__doc__},

static PyValue
zlib_Compress_flush_impl(compobject *self, PyTypeObject *cls, int mode);

static PyValue
zlib_Compress_flush(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int mode = Z_FINISH;

    if (!_PyArg_NoKwnames("flush", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("flush", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    mode = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (mode == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = zlib_Compress_flush_impl((compobject *)PyValue_AsKnownObjectBorrow(self), cls, mode);

exit:
    return return_value;
}

#if defined(HAVE_ZLIB_COPY)

PyDoc_STRVAR(zlib_Compress_copy__doc__,
"copy($self, /)\n"
"--\n"
"\n"
"Return a copy of the compression object.");

#define ZLIB_COMPRESS_COPY_METHODDEF    \
    {"copy", _PyCFunction_CAST(zlib_Compress_copy), METH_VALUE|METH_METHOD|METH_KEYWORDS, zlib_Compress_copy__doc__},

static PyValue
zlib_Compress_copy_impl(compobject *self, PyTypeObject *cls);

static PyValue
zlib_Compress_copy(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "copy() takes no arguments");
        goto exit;
    }
    return_value = zlib_Compress_copy_impl((compobject *)PyValue_AsKnownObjectBorrow(self), cls);

exit:
    return return_value;
}

#endif /* defined(HAVE_ZLIB_COPY) */

#if defined(HAVE_ZLIB_COPY)

PyDoc_STRVAR(zlib_Compress___copy____doc__,
"__copy__($self, /)\n"
"--\n"
"\n");

#define ZLIB_COMPRESS___COPY___METHODDEF    \
    {"__copy__", _PyCFunction_CAST(zlib_Compress___copy__), METH_VALUE|METH_METHOD|METH_KEYWORDS, zlib_Compress___copy____doc__},

static PyValue
zlib_Compress___copy___impl(compobject *self, PyTypeObject *cls);

static PyValue
zlib_Compress___copy__(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "__copy__() takes no arguments");
        goto exit;
    }
    return_value = zlib_Compress___copy___impl((compobject *)PyValue_AsKnownObjectBorrow(self), cls);

exit:
    return return_value;
}

#endif /* defined(HAVE_ZLIB_COPY) */

#if defined(HAVE_ZLIB_COPY)

PyDoc_STRVAR(zlib_Compress___deepcopy____doc__,
"__deepcopy__($self, memo, /)\n"
"--\n"
"\n");

#define ZLIB_COMPRESS___DEEPCOPY___METHODDEF    \
    {"__deepcopy__", _PyCFunction_CAST(zlib_Compress___deepcopy__), METH_VALUE|METH_METHOD|METH_KEYWORDS, zlib_Compress___deepcopy____doc__},

static PyValue
zlib_Compress___deepcopy___impl(compobject *self, PyTypeObject *cls,
                                PyValueBorrow memo);

static PyValue
zlib_Compress___deepcopy__(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow memo;

    if (!_PyArg_NoKwnames("__deepcopy__", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("__deepcopy__", nargs, 1, 1)) {
        goto exit;
    }
    memo = PyValue_Borrow(args[0]);
    return_value = zlib_Compress___deepcopy___impl((compobject *)PyValue_AsKnownObjectBorrow(self), cls, memo);

exit:
    return return_value;
}

#endif /* defined(HAVE_ZLIB_COPY) */

#if defined(HAVE_ZLIB_COPY)

PyDoc_STRVAR(zlib_Decompress_copy__doc__,
"copy($self, /)\n"
"--\n"
"\n"
"Return a copy of the decompression object.");

#define ZLIB_DECOMPRESS_COPY_METHODDEF    \
    {"copy", _PyCFunction_CAST(zlib_Decompress_copy), METH_VALUE|METH_METHOD|METH_KEYWORDS, zlib_Decompress_copy__doc__},

static PyValue
zlib_Decompress_copy_impl(compobject *self, PyTypeObject *cls);

static PyValue
zlib_Decompress_copy(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "copy() takes no arguments");
        goto exit;
    }
    return_value = zlib_Decompress_copy_impl((compobject *)PyValue_AsKnownObjectBorrow(self), cls);

exit:
    return return_value;
}

#endif /* defined(HAVE_ZLIB_COPY) */

#if defined(HAVE_ZLIB_COPY)

PyDoc_STRVAR(zlib_Decompress___copy____doc__,
"__copy__($self, /)\n"
"--\n"
"\n");

#define ZLIB_DECOMPRESS___COPY___METHODDEF    \
    {"__copy__", _PyCFunction_CAST(zlib_Decompress___copy__), METH_VALUE|METH_METHOD|METH_KEYWORDS, zlib_Decompress___copy____doc__},

static PyValue
zlib_Decompress___copy___impl(compobject *self, PyTypeObject *cls);

static PyValue
zlib_Decompress___copy__(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "__copy__() takes no arguments");
        goto exit;
    }
    return_value = zlib_Decompress___copy___impl((compobject *)PyValue_AsKnownObjectBorrow(self), cls);

exit:
    return return_value;
}

#endif /* defined(HAVE_ZLIB_COPY) */

#if defined(HAVE_ZLIB_COPY)

PyDoc_STRVAR(zlib_Decompress___deepcopy____doc__,
"__deepcopy__($self, memo, /)\n"
"--\n"
"\n");

#define ZLIB_DECOMPRESS___DEEPCOPY___METHODDEF    \
    {"__deepcopy__", _PyCFunction_CAST(zlib_Decompress___deepcopy__), METH_VALUE|METH_METHOD|METH_KEYWORDS, zlib_Decompress___deepcopy____doc__},

static PyValue
zlib_Decompress___deepcopy___impl(compobject *self, PyTypeObject *cls,
                                  PyValueBorrow memo);

static PyValue
zlib_Decompress___deepcopy__(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow memo;

    if (!_PyArg_NoKwnames("__deepcopy__", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("__deepcopy__", nargs, 1, 1)) {
        goto exit;
    }
    memo = PyValue_Borrow(args[0]);
    return_value = zlib_Decompress___deepcopy___impl((compobject *)PyValue_AsKnownObjectBorrow(self), cls, memo);

exit:
    return return_value;
}

#endif /* defined(HAVE_ZLIB_COPY) */

PyDoc_STRVAR(zlib_Decompress_flush__doc__,
"flush($self, length=zlib.DEF_BUF_SIZE, /)\n"
"--\n"
"\n"
"Return a bytes object containing any remaining decompressed data.\n"
"\n"
"  length\n"
"    the initial size of the output buffer.");

#define ZLIB_DECOMPRESS_FLUSH_METHODDEF    \
    {"flush", _PyCFunction_CAST(zlib_Decompress_flush), METH_VALUE|METH_METHOD|METH_KEYWORDS, zlib_Decompress_flush__doc__},

static PyValue
zlib_Decompress_flush_impl(compobject *self, PyTypeObject *cls,
                           Py_ssize_t length);

static PyValue
zlib_Decompress_flush(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_ssize_t length = DEF_BUF_SIZE;

    if (!_PyArg_NoKwnames("flush", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("flush", nargs, 0, 1)) {
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
        length = ival;
    }
skip_optional:
    return_value = zlib_Decompress_flush_impl((compobject *)PyValue_AsKnownObjectBorrow(self), cls, length);

exit:
    return return_value;
}

PyDoc_STRVAR(zlib__ZlibDecompressor_decompress__doc__,
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

#define ZLIB__ZLIBDECOMPRESSOR_DECOMPRESS_METHODDEF    \
    {"decompress", _PyCFunction_CAST(zlib__ZlibDecompressor_decompress), METH_VALUE|METH_KEYWORDS, zlib__ZlibDecompressor_decompress__doc__},

static PyValue
zlib__ZlibDecompressor_decompress_impl(ZlibDecompressor *self,
                                       Py_buffer *data,
                                       Py_ssize_t max_length);

static PyValue
zlib__ZlibDecompressor_decompress(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
    return_value = zlib__ZlibDecompressor_decompress_impl((ZlibDecompressor *)PyValue_AsKnownObjectBorrow(self), &data, max_length);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(zlib__ZlibDecompressor__doc__,
"_ZlibDecompressor(wbits=MAX_WBITS, zdict=b\'\')\n"
"--\n"
"\n"
"Create a decompressor object for decompressing data incrementally.\n"
"\n"
"  zdict\n"
"    The predefined compression dictionary. This is a sequence of bytes\n"
"    (such as a bytes object) containing subsequences that are expected\n"
"    to occur frequently in the data that is to be compressed. Those\n"
"    subsequences that are expected to be most common should come at the\n"
"    end of the dictionary. This must be the same dictionary as used by the\n"
"    compressor that produced the input data.");

static PyValue
zlib__ZlibDecompressor_impl(PyTypeObject *type, int wbits,
                            PyValueBorrow zdict);

static PyValue
zlib__ZlibDecompressor_value(PyTypeObject *type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(wbits)), _PyValue_STATIC_IMMORTAL(&_Py_ID(zdict)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"wbits", "zdict", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "_ZlibDecompressor",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    int wbits = MAX_WBITS;
    PyValueBorrow zdict = PyValue_Borrow(PyValue_NULL);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        wbits = _PyValue_AsInt(PyValue_Borrow(args[_idx[0]]));
        if (wbits == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (_idx[1] >= 0) {
        zdict = PyValue_Borrow(args[_idx[1]]);
    }
    return_value = zlib__ZlibDecompressor_impl(type, wbits, zdict);

exit:
    return return_value;
}

Py_VALUE_ENTRY static PyObject *
zlib__ZlibDecompressor(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _PyValue_THUNK_EXIT(zlib__ZlibDecompressor_value(type, _PyTuple_ITEMS(args), nargs, NULL), BOX_SITE_CALL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return NULL;
    }
    PyValue result = zlib__ZlibDecompressor_value(type, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return _PyValue_THUNK_EXIT(result, BOX_SITE_CALL);
}

PyDoc_STRVAR(zlib_adler32__doc__,
"adler32($module, data, value=1, /)\n"
"--\n"
"\n"
"Compute an Adler-32 checksum of data.\n"
"\n"
"  value\n"
"    Starting value of the checksum.\n"
"\n"
"The returned checksum is an integer.");

#define ZLIB_ADLER32_METHODDEF    \
    {"adler32", _PyCFunction_CAST(zlib_adler32), METH_VALUE, zlib_adler32__doc__},

static PyValue
zlib_adler32_impl(PyObject *module, Py_buffer *data, unsigned int value);

static PyValue
zlib_adler32(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_buffer data = {NULL, NULL};
    unsigned int value = 1;

    if (!_PyArg_CheckPositional("adler32", nargs, 1, 2)) {
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
        Py_ssize_t _bytes = _PyValue_AsNativeBytes(PyValue_Borrow(args[1]), &value, sizeof(unsigned int),
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
    return_value = zlib_adler32_impl(PyValue_AsKnownObjectBorrow(module), &data, value);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(zlib_adler32_combine__doc__,
"adler32_combine($module, adler1, adler2, len2, /)\n"
"--\n"
"\n"
"Combine two Adler-32 checksums into one.\n"
"\n"
"  adler1\n"
"    Adler-32 checksum for sequence A\n"
"  adler2\n"
"    Adler-32 checksum for sequence B\n"
"  len2\n"
"    Length of sequence B\n"
"\n"
"Given the Adler-32 checksum \'adler1\' of a sequence A and the\n"
"Adler-32 checksum \'adler2\' of a sequence B of length \'len2\',\n"
"return the Adler-32 checksum of A and B concatenated.");

#define ZLIB_ADLER32_COMBINE_METHODDEF    \
    {"adler32_combine", _PyCFunction_CAST(zlib_adler32_combine), METH_VALUE, zlib_adler32_combine__doc__},

static unsigned int
zlib_adler32_combine_impl(PyObject *module, unsigned int adler1,
                          unsigned int adler2, PyValueBorrow len2);

static PyValue
zlib_adler32_combine(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    unsigned int adler1;
    unsigned int adler2;
    PyValueBorrow len2;
    unsigned int _return_value;

    if (!_PyArg_CheckPositional("adler32_combine", nargs, 3, 3)) {
        goto exit;
    }
    {
        Py_ssize_t _bytes = _PyValue_AsNativeBytes(PyValue_Borrow(args[0]), &adler1, sizeof(unsigned int),
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
    {
        Py_ssize_t _bytes = _PyValue_AsNativeBytes(PyValue_Borrow(args[1]), &adler2, sizeof(unsigned int),
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
    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[2])), &PyLong_Type)) {
        _PyArg_BadArgumentValue("adler32_combine", "argument 3", "int", PyValue_Borrow(args[2]));
        goto exit;
    }
    len2 = PyValue_Borrow(args[2]);
    _return_value = zlib_adler32_combine_impl(PyValue_AsKnownObjectBorrow(module), adler1, adler2, len2);
    if ((_return_value == (unsigned int)-1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = (uint64_t)_return_value <= INT64_MAX
        ? PyValue_FromInt64((int64_t)_return_value)
        : _PyValue_FromObjectResult(PyLong_FromUnsignedLongLong((unsigned long long)_return_value));

exit:
    return return_value;
}

PyDoc_STRVAR(zlib_crc32__doc__,
"crc32($module, data, value=0, /)\n"
"--\n"
"\n"
"Compute a CRC-32 checksum of data.\n"
"\n"
"  value\n"
"    Starting value of the checksum.\n"
"\n"
"The returned checksum is an integer.");

#define ZLIB_CRC32_METHODDEF    \
    {"crc32", _PyCFunction_CAST(zlib_crc32), METH_VALUE, zlib_crc32__doc__},

static unsigned int
zlib_crc32_impl(PyObject *module, Py_buffer *data, unsigned int value);

static PyValue
zlib_crc32(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_buffer data = {NULL, NULL};
    unsigned int value = 0;
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
        Py_ssize_t _bytes = _PyValue_AsNativeBytes(PyValue_Borrow(args[1]), &value, sizeof(unsigned int),
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
    _return_value = zlib_crc32_impl(PyValue_AsKnownObjectBorrow(module), &data, value);
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

PyDoc_STRVAR(zlib_crc32_combine__doc__,
"crc32_combine($module, crc1, crc2, len2, /)\n"
"--\n"
"\n"
"Combine two CRC-32 checksums into one.\n"
"\n"
"  crc1\n"
"    CRC-32 checksum for sequence A\n"
"  crc2\n"
"    CRC-32 checksum for sequence B\n"
"  len2\n"
"    Length of sequence B\n"
"\n"
"Given the CRC-32 checksum \'crc1\' of a sequence A and the\n"
"CRC-32 checksum \'crc2\' of a sequence B of length \'len2\',\n"
"return the CRC-32 checksum of A and B concatenated.");

#define ZLIB_CRC32_COMBINE_METHODDEF    \
    {"crc32_combine", _PyCFunction_CAST(zlib_crc32_combine), METH_VALUE, zlib_crc32_combine__doc__},

static unsigned int
zlib_crc32_combine_impl(PyObject *module, unsigned int crc1,
                        unsigned int crc2, PyValueBorrow len2);

static PyValue
zlib_crc32_combine(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    unsigned int crc1;
    unsigned int crc2;
    PyValueBorrow len2;
    unsigned int _return_value;

    if (!_PyArg_CheckPositional("crc32_combine", nargs, 3, 3)) {
        goto exit;
    }
    {
        Py_ssize_t _bytes = _PyValue_AsNativeBytes(PyValue_Borrow(args[0]), &crc1, sizeof(unsigned int),
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
    {
        Py_ssize_t _bytes = _PyValue_AsNativeBytes(PyValue_Borrow(args[1]), &crc2, sizeof(unsigned int),
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
    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[2])), &PyLong_Type)) {
        _PyArg_BadArgumentValue("crc32_combine", "argument 3", "int", PyValue_Borrow(args[2]));
        goto exit;
    }
    len2 = PyValue_Borrow(args[2]);
    _return_value = zlib_crc32_combine_impl(PyValue_AsKnownObjectBorrow(module), crc1, crc2, len2);
    if ((_return_value == (unsigned int)-1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = (uint64_t)_return_value <= INT64_MAX
        ? PyValue_FromInt64((int64_t)_return_value)
        : _PyValue_FromObjectResult(PyLong_FromUnsignedLongLong((unsigned long long)_return_value));

exit:
    return return_value;
}

#ifndef ZLIB_COMPRESS_COPY_METHODDEF
    #define ZLIB_COMPRESS_COPY_METHODDEF
#endif /* !defined(ZLIB_COMPRESS_COPY_METHODDEF) */

#ifndef ZLIB_COMPRESS___COPY___METHODDEF
    #define ZLIB_COMPRESS___COPY___METHODDEF
#endif /* !defined(ZLIB_COMPRESS___COPY___METHODDEF) */

#ifndef ZLIB_COMPRESS___DEEPCOPY___METHODDEF
    #define ZLIB_COMPRESS___DEEPCOPY___METHODDEF
#endif /* !defined(ZLIB_COMPRESS___DEEPCOPY___METHODDEF) */

#ifndef ZLIB_DECOMPRESS_COPY_METHODDEF
    #define ZLIB_DECOMPRESS_COPY_METHODDEF
#endif /* !defined(ZLIB_DECOMPRESS_COPY_METHODDEF) */

#ifndef ZLIB_DECOMPRESS___COPY___METHODDEF
    #define ZLIB_DECOMPRESS___COPY___METHODDEF
#endif /* !defined(ZLIB_DECOMPRESS___COPY___METHODDEF) */

#ifndef ZLIB_DECOMPRESS___DEEPCOPY___METHODDEF
    #define ZLIB_DECOMPRESS___DEEPCOPY___METHODDEF
#endif /* !defined(ZLIB_DECOMPRESS___DEEPCOPY___METHODDEF) */
/*[clinic end generated code: output=dd3e7448aeab88ba input=a9049054013a1b77]*/
