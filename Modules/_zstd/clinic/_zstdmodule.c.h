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
#include "pycore_modsupport.h"    // _PyArg_CheckPositional()
#include "pycore_value_call.h"    // METH_VALUE
#include "pycore_value_ops.h"     // _PyValue_Index()

PyDoc_STRVAR(_zstd_train_dict__doc__,
"train_dict($module, samples_bytes, samples_sizes, dict_size, /)\n"
"--\n"
"\n"
"Train a Zstandard dictionary on sample data.\n"
"\n"
"  samples_bytes\n"
"    Concatenation of samples.\n"
"  samples_sizes\n"
"    Tuple of samples\' sizes.\n"
"  dict_size\n"
"    The size of the dictionary.");

#define _ZSTD_TRAIN_DICT_METHODDEF    \
    {"train_dict", _PyCFunction_CAST(_zstd_train_dict), METH_VALUE, _zstd_train_dict__doc__},

static PyValue
_zstd_train_dict_impl(PyObject *module, PyBytesObject *samples_bytes,
                      PyObject *samples_sizes, Py_ssize_t dict_size);

static PyValue
_zstd_train_dict(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyBytesObject *samples_bytes;
    PyObject *samples_sizes;
    Py_ssize_t dict_size;

    if (!_PyArg_CheckPositional("train_dict", nargs, 3, 3)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("train_dict", "argument 1", "bytes", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyBytes_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("train_dict", "argument 1", "bytes", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    samples_bytes = (PyBytesObject *)PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[1])), &PyTuple_Type)) {
        _PyArg_BadArgumentValue("train_dict", "argument 2", "tuple", PyValue_Borrow(args[1]));
        goto exit;
    }
    samples_sizes = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1]));
    {
        Py_ssize_t ival = -1;
        PyValue iobj = _PyValue_Index(PyValue_Borrow(args[2]));
        if (!PyValue_IsNull(iobj)) {
            ival = _PyValue_IntAsSsize_t(PyValue_Borrow(iobj));
            PyValue_CLOSE(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        dict_size = ival;
    }
    return_value = _zstd_train_dict_impl(PyValue_AsKnownObjectBorrow(module), samples_bytes, samples_sizes, dict_size);

exit:
    return return_value;
}

PyDoc_STRVAR(_zstd_finalize_dict__doc__,
"finalize_dict($module, custom_dict_bytes, samples_bytes, samples_sizes,\n"
"              dict_size, compression_level, /)\n"
"--\n"
"\n"
"Finalize a Zstandard dictionary.\n"
"\n"
"  custom_dict_bytes\n"
"    Custom dictionary content.\n"
"  samples_bytes\n"
"    Concatenation of samples.\n"
"  samples_sizes\n"
"    Tuple of samples\' sizes.\n"
"  dict_size\n"
"    The size of the dictionary.\n"
"  compression_level\n"
"    Optimize for a specific Zstandard compression level, 0 means default.");

#define _ZSTD_FINALIZE_DICT_METHODDEF    \
    {"finalize_dict", _PyCFunction_CAST(_zstd_finalize_dict), METH_VALUE, _zstd_finalize_dict__doc__},

static PyValue
_zstd_finalize_dict_impl(PyObject *module, PyBytesObject *custom_dict_bytes,
                         PyBytesObject *samples_bytes,
                         PyObject *samples_sizes, Py_ssize_t dict_size,
                         int compression_level);

static PyValue
_zstd_finalize_dict(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyBytesObject *custom_dict_bytes;
    PyBytesObject *samples_bytes;
    PyObject *samples_sizes;
    Py_ssize_t dict_size;
    int compression_level;

    if (!_PyArg_CheckPositional("finalize_dict", nargs, 5, 5)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("finalize_dict", "argument 1", "bytes", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyBytes_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("finalize_dict", "argument 1", "bytes", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    custom_dict_bytes = (PyBytesObject *)PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    if (!PyValue_IsObject(PyValue_Borrow(args[1]))) {
        _PyArg_BadArgumentValue("finalize_dict", "argument 2", "bytes", PyValue_Borrow(args[1]));
        goto exit;
    }
    if (!PyBytes_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])))) {
        _PyArg_BadArgument("finalize_dict", "argument 2", "bytes", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        goto exit;
    }
    samples_bytes = (PyBytesObject *)PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1]));
    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[2])), &PyTuple_Type)) {
        _PyArg_BadArgumentValue("finalize_dict", "argument 3", "tuple", PyValue_Borrow(args[2]));
        goto exit;
    }
    samples_sizes = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[2]));
    {
        Py_ssize_t ival = -1;
        PyValue iobj = _PyValue_Index(PyValue_Borrow(args[3]));
        if (!PyValue_IsNull(iobj)) {
            ival = _PyValue_IntAsSsize_t(PyValue_Borrow(iobj));
            PyValue_CLOSE(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        dict_size = ival;
    }
    compression_level = _PyValue_AsInt(PyValue_Borrow(args[4]));
    if (compression_level == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _zstd_finalize_dict_impl(PyValue_AsKnownObjectBorrow(module), custom_dict_bytes, samples_bytes, samples_sizes, dict_size, compression_level);

exit:
    return return_value;
}

PyDoc_STRVAR(_zstd_get_param_bounds__doc__,
"get_param_bounds($module, /, parameter, is_compress)\n"
"--\n"
"\n"
"Get CompressionParameter/DecompressionParameter bounds.\n"
"\n"
"  parameter\n"
"    The parameter to get bounds.\n"
"  is_compress\n"
"    True for CompressionParameter, False for DecompressionParameter.");

#define _ZSTD_GET_PARAM_BOUNDS_METHODDEF    \
    {"get_param_bounds", _PyCFunction_CAST(_zstd_get_param_bounds), METH_VALUE|METH_KEYWORDS, _zstd_get_param_bounds__doc__},

static PyValue
_zstd_get_param_bounds_impl(PyObject *module, int parameter, int is_compress);

static PyValue
_zstd_get_param_bounds(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(parameter)), _PyValue_STATIC_IMMORTAL(&_Py_ID(is_compress)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"parameter", "is_compress", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "get_param_bounds",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    int parameter;
    int is_compress;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    parameter = _PyValue_AsInt(PyValue_Borrow(args[_idx[0]]));
    if (parameter == -1 && PyErr_Occurred()) {
        goto exit;
    }
    is_compress = _PyValue_IsTrue(PyValue_Borrow(args[_idx[1]]));
    if (is_compress < 0) {
        goto exit;
    }
    return_value = _zstd_get_param_bounds_impl(PyValue_AsKnownObjectBorrow(module), parameter, is_compress);

exit:
    return return_value;
}

PyDoc_STRVAR(_zstd_get_frame_size__doc__,
"get_frame_size($module, /, frame_buffer)\n"
"--\n"
"\n"
"Get the size of a Zstandard frame, including the header and optional checksum.\n"
"\n"
"  frame_buffer\n"
"    A bytes-like object, it should start from the beginning of a frame,\n"
"    and contains at least one complete frame.");

#define _ZSTD_GET_FRAME_SIZE_METHODDEF    \
    {"get_frame_size", _PyCFunction_CAST(_zstd_get_frame_size), METH_VALUE|METH_KEYWORDS, _zstd_get_frame_size__doc__},

static PyValue
_zstd_get_frame_size_impl(PyObject *module, Py_buffer *frame_buffer);

static PyValue
_zstd_get_frame_size(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(frame_buffer)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"frame_buffer", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "get_frame_size",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    Py_buffer frame_buffer = {NULL, NULL};

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[_idx[0]]))) {
        PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[_idx[0]]))->tp_name);
        goto exit;
    }
    if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])), &frame_buffer, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    return_value = _zstd_get_frame_size_impl(PyValue_AsKnownObjectBorrow(module), &frame_buffer);

exit:
    /* Cleanup for frame_buffer */
    if (frame_buffer.obj) {
       PyBuffer_Release(&frame_buffer);
    }

    return return_value;
}

PyDoc_STRVAR(_zstd_get_frame_info__doc__,
"get_frame_info($module, /, frame_buffer)\n"
"--\n"
"\n"
"Get Zstandard frame infomation from a frame header.\n"
"\n"
"  frame_buffer\n"
"    A bytes-like object, containing the header of a Zstandard frame.");

#define _ZSTD_GET_FRAME_INFO_METHODDEF    \
    {"get_frame_info", _PyCFunction_CAST(_zstd_get_frame_info), METH_VALUE|METH_KEYWORDS, _zstd_get_frame_info__doc__},

static PyValue
_zstd_get_frame_info_impl(PyObject *module, Py_buffer *frame_buffer);

static PyValue
_zstd_get_frame_info(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(frame_buffer)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"frame_buffer", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "get_frame_info",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    Py_buffer frame_buffer = {NULL, NULL};

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[_idx[0]]))) {
        PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[_idx[0]]))->tp_name);
        goto exit;
    }
    if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])), &frame_buffer, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    return_value = _zstd_get_frame_info_impl(PyValue_AsKnownObjectBorrow(module), &frame_buffer);

exit:
    /* Cleanup for frame_buffer */
    if (frame_buffer.obj) {
       PyBuffer_Release(&frame_buffer);
    }

    return return_value;
}

PyDoc_STRVAR(_zstd_set_parameter_types__doc__,
"set_parameter_types($module, /, c_parameter_type, d_parameter_type)\n"
"--\n"
"\n"
"Set CompressionParameter and DecompressionParameter types for validity check.\n"
"\n"
"  c_parameter_type\n"
"    CompressionParameter IntEnum type object\n"
"  d_parameter_type\n"
"    DecompressionParameter IntEnum type object");

#define _ZSTD_SET_PARAMETER_TYPES_METHODDEF    \
    {"set_parameter_types", _PyCFunction_CAST(_zstd_set_parameter_types), METH_VALUE|METH_KEYWORDS, _zstd_set_parameter_types__doc__},

static PyValue
_zstd_set_parameter_types_impl(PyObject *module, PyObject *c_parameter_type,
                               PyObject *d_parameter_type);

static PyValue
_zstd_set_parameter_types(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(c_parameter_type)), _PyValue_STATIC_IMMORTAL(&_Py_ID(d_parameter_type)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"c_parameter_type", "d_parameter_type", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "set_parameter_types",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    PyObject *c_parameter_type;
    PyObject *d_parameter_type;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[_idx[0]])), &PyType_Type)) {
        _PyArg_BadArgumentValue("set_parameter_types", "argument 'c_parameter_type'", (&PyType_Type)->tp_name, PyValue_Borrow(args[_idx[0]]));
        goto exit;
    }
    c_parameter_type = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]]));
    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[_idx[1]])), &PyType_Type)) {
        _PyArg_BadArgumentValue("set_parameter_types", "argument 'd_parameter_type'", (&PyType_Type)->tp_name, PyValue_Borrow(args[_idx[1]]));
        goto exit;
    }
    d_parameter_type = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]]));
    return_value = _zstd_set_parameter_types_impl(PyValue_AsKnownObjectBorrow(module), c_parameter_type, d_parameter_type);

exit:
    return return_value;
}
/*[clinic end generated code: output=9704ca1cd18f2da2 input=a9049054013a1b77]*/
