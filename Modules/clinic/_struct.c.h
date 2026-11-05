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

PyDoc_STRVAR(Struct__doc__,
"Struct(format)\n"
"--\n"
"\n"
"Create a compiled struct object.\n"
"\n"
"Return a new Struct object which writes and reads binary data according\n"
"to the format string.  See help(struct) for more on format strings.");

static PyValue
Struct_impl(PyTypeObject *type, PyValueBorrow format);

static PyValue
Struct_value(PyTypeObject *type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(format)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"format", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "Struct",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    PyValueBorrow format;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    format = PyValue_Borrow(args[_idx[0]]);
    return_value = Struct_impl(type, format);

exit:
    return return_value;
}

Py_VALUE_ENTRY static PyObject *
Struct(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _PyValue_THUNK_EXIT(Struct_value(type, _PyTuple_ITEMS(args), nargs, NULL), BOX_SITE_CALL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return NULL;
    }
    PyValue result = Struct_value(type, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return _PyValue_THUNK_EXIT(result, BOX_SITE_CALL);
}

PyDoc_STRVAR(Struct___init____doc__,
"Struct(format)\n"
"--\n"
"\n"
"Create a compiled struct object.\n"
"\n"
"Return a new Struct object which writes and reads binary data according\n"
"to the format string.  See help(struct) for more on format strings.");

static int
Struct___init___impl(PyStructObject *self, PyValueBorrow format);

static int
Struct___init___value(PyObject *self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    int return_value = -1;
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(format)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"format", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "Struct",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    PyValueBorrow format;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    format = PyValue_Borrow(args[_idx[0]]);
    return_value = Struct___init___impl((PyStructObject *)self, format);

exit:
    return return_value;
}

static int
Struct___init__(PyObject *self, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return Struct___init___value(self, _PyTuple_ITEMS(args), nargs, NULL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return -1;
    }
    int result = Struct___init___value(self, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return result;
}

PyDoc_STRVAR(Struct_unpack__doc__,
"unpack($self, buffer, /)\n"
"--\n"
"\n"
"Return a tuple containing unpacked values.\n"
"\n"
"Unpack according to the struct format string.  The buffer\'s\n"
"size in bytes must be the struct size.  See help(struct) for more on\n"
"format strings.");

#define STRUCT_UNPACK_METHODDEF    \
    {"unpack", _PyCFunction_CAST(Struct_unpack), METH_VALUE|METH_O, Struct_unpack__doc__},

static PyValue
Struct_unpack_impl(PyStructObject *self, Py_buffer *buffer);

static PyValue
Struct_unpack(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_buffer buffer = {NULL, NULL};

    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[0]))->tp_name);
        goto exit;
    }
    if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])), &buffer, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    return_value = Struct_unpack_impl((PyStructObject *)PyValue_AsKnownObjectBorrow(self), &buffer);

exit:
    /* Cleanup for buffer */
    if (buffer.obj) {
       PyBuffer_Release(&buffer);
    }

    return return_value;
}

PyDoc_STRVAR(Struct_unpack_from__doc__,
"unpack_from($self, /, buffer, offset=0)\n"
"--\n"
"\n"
"Return a tuple containing unpacked values.\n"
"\n"
"Values are unpacked according to the struct format string.  The\n"
"buffer\'s size in bytes, starting at position offset, must be at\n"
"least the struct size.  A negative offset counts from the end of\n"
"the buffer.  See help(struct) for more on format strings.");

#define STRUCT_UNPACK_FROM_METHODDEF    \
    {"unpack_from", _PyCFunction_CAST(Struct_unpack_from), METH_VALUE|METH_KEYWORDS, Struct_unpack_from__doc__},

static PyValue
Struct_unpack_from_impl(PyStructObject *self, Py_buffer *buffer,
                        Py_ssize_t offset);

static PyValue
Struct_unpack_from(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(buffer)), _PyValue_STATIC_IMMORTAL(&_Py_ID(offset)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"buffer", "offset", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "unpack_from",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    Py_buffer buffer = {NULL, NULL};
    Py_ssize_t offset = 0;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[_idx[0]]))) {
        PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[_idx[0]]))->tp_name);
        goto exit;
    }
    if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])), &buffer, PyBUF_SIMPLE) != 0) {
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
            offset = ival;
        }
    }
    return_value = Struct_unpack_from_impl((PyStructObject *)PyValue_AsKnownObjectBorrow(self), &buffer, offset);

exit:
    /* Cleanup for buffer */
    if (buffer.obj) {
       PyBuffer_Release(&buffer);
    }

    return return_value;
}

PyDoc_STRVAR(Struct_iter_unpack__doc__,
"iter_unpack($self, buffer, /)\n"
"--\n"
"\n"
"Return an iterator yielding tuples.\n"
"\n"
"Tuples are unpacked from the given bytes source, like a repeated\n"
"invocation of unpack_from().  Requires that the bytes length be\n"
"a multiple of the struct size.");

#define STRUCT_ITER_UNPACK_METHODDEF    \
    {"iter_unpack", _PyCFunction_CAST(Struct_iter_unpack), METH_VALUE|METH_O, Struct_iter_unpack__doc__},

static PyValue
Struct_iter_unpack_impl(PyStructObject *self, PyValueBorrow buffer);

static PyValue
Struct_iter_unpack(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow buffer;

    buffer = PyValue_Borrow(args[0]);
    return_value = Struct_iter_unpack_impl((PyStructObject *)PyValue_AsKnownObjectBorrow(self), buffer);

    return return_value;
}

PyDoc_STRVAR(Struct_pack__doc__,
"pack($self, /, *values)\n"
"--\n"
"\n"
"Pack values and return the packed bytes.\n"
"\n"
"Return a bytes object containing the provided values packed\n"
"according to the struct format string.  See help(struct) for more on\n"
"format strings.");

#define STRUCT_PACK_METHODDEF    \
    {"pack", _PyCFunction_CAST(Struct_pack), METH_VALUE, Struct_pack__doc__},

static PyValue
Struct_pack_impl(PyStructObject *self, const PyValue *values,
                 Py_ssize_t values_length);

static PyValue
Struct_pack(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    const PyValue *values;
    Py_ssize_t values_length;

    values = args;
    values_length = nargs;
    return_value = Struct_pack_impl((PyStructObject *)PyValue_AsKnownObjectBorrow(self), values, values_length);

    return return_value;
}

PyDoc_STRVAR(Struct_pack_into__doc__,
"pack_into($self, buffer, offset, /, *values)\n"
"--\n"
"\n"
"Pack values and write the packed bytes into the buffer.\n"
"\n"
"Pack the provided values according to the struct format string\n"
"and write the packed bytes into the writable buffer starting at\n"
"offset.  Note that the offset is a required argument.  A negative\n"
"offset counts from the end of the buffer.  See help(struct) for\n"
"more on format strings.");

#define STRUCT_PACK_INTO_METHODDEF    \
    {"pack_into", _PyCFunction_CAST(Struct_pack_into), METH_VALUE, Struct_pack_into__doc__},

static PyValue
Struct_pack_into_impl(PyStructObject *self, Py_buffer *buffer,
                      Py_ssize_t offset, const PyValue *values,
                      Py_ssize_t values_length);

static PyValue
Struct_pack_into(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_buffer buffer = {NULL, NULL};
    Py_ssize_t offset;
    const PyValue *values;
    Py_ssize_t values_length;

    if (!_PyArg_CheckPositional("pack_into", nargs, 2, PY_SSIZE_T_MAX)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("pack_into", "argument 1", "read-write bytes-like object", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])), &buffer, PyBUF_WRITABLE) < 0) {
        _PyArg_BadArgument("pack_into", "argument 1", "read-write bytes-like object", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    {
        Py_ssize_t ival = -1;
        PyValue iobj = _PyValue_Index(PyValue_Borrow(args[1]));
        if (!PyValue_IsNull(iobj)) {
            ival = _PyValue_IntAsSsize_t(PyValue_Borrow(iobj));
            PyValue_CLOSE(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        offset = ival;
    }
    values = args + 2;
    values_length = nargs - 2;
    return_value = Struct_pack_into_impl((PyStructObject *)PyValue_AsKnownObjectBorrow(self), &buffer, offset, values, values_length);

exit:
    /* Cleanup for buffer */
    if (buffer.obj) {
       PyBuffer_Release(&buffer);
    }

    return return_value;
}

PyDoc_STRVAR(Struct___sizeof____doc__,
"__sizeof__($self, /)\n"
"--\n"
"\n");

#define STRUCT___SIZEOF___METHODDEF    \
    {"__sizeof__", _PyCFunction_CAST(Struct___sizeof__), METH_VALUE|METH_NOARGS, Struct___sizeof____doc__},

static PyValue
Struct___sizeof___impl(PyStructObject *self);

static PyValue
Struct___sizeof__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = Struct___sizeof___impl((PyStructObject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_clearcache__doc__,
"_clearcache($module, /)\n"
"--\n"
"\n"
"Clear the internal cache.");

#define _CLEARCACHE_METHODDEF    \
    {"_clearcache", _PyCFunction_CAST(_clearcache), METH_VALUE|METH_NOARGS, _clearcache__doc__},

static PyValue
_clearcache_impl(PyObject *module);

static PyValue
_clearcache(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _clearcache_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(calcsize__doc__,
"calcsize($module, format, /)\n"
"--\n"
"\n"
"Return size in bytes of the struct described by the format string.");

#define CALCSIZE_METHODDEF    \
    {"calcsize", _PyCFunction_CAST(calcsize), METH_VALUE|METH_O, calcsize__doc__},

static Py_ssize_t
calcsize_impl(PyObject *module, PyStructObject *s_object);

static PyValue
calcsize(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyStructObject *s_object = NULL;
    Py_ssize_t _return_value;

    if (!cache_struct_converter(module, PyValue_Borrow(args[0]), &s_object)) {
        goto exit;
    }
    _return_value = calcsize_impl(PyValue_AsKnownObjectBorrow(module), s_object);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyValue_FromInt64((int64_t)_return_value);

exit:
    /* Cleanup for s_object */
    Py_XDECREF(s_object);

    return return_value;
}

PyDoc_STRVAR(pack__doc__,
"pack($module, format, /, *values)\n"
"--\n"
"\n"
"Pack values and return the packed bytes.\n"
"\n"
"Return a bytes object containing the provided values packed according\n"
"to the format string.  See help(struct) for more on format strings.");

#define PACK_METHODDEF    \
    {"pack", _PyCFunction_CAST(pack), METH_VALUE, pack__doc__},

static PyValue
pack_impl(PyObject *module, PyStructObject *s_object, const PyValue *values,
          Py_ssize_t values_length);

static PyValue
pack(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyStructObject *s_object = NULL;
    const PyValue *values;
    Py_ssize_t values_length;

    if (!_PyArg_CheckPositional("pack", nargs, 1, PY_SSIZE_T_MAX)) {
        goto exit;
    }
    if (!cache_struct_converter(module, PyValue_Borrow(args[0]), &s_object)) {
        goto exit;
    }
    values = args + 1;
    values_length = nargs - 1;
    return_value = pack_impl(PyValue_AsKnownObjectBorrow(module), s_object, values, values_length);

exit:
    /* Cleanup for s_object */
    Py_XDECREF(s_object);

    return return_value;
}

PyDoc_STRVAR(pack_into__doc__,
"pack_into($module, format, buffer, offset, /, *values)\n"
"--\n"
"\n"
"Pack values and write the packed bytes into the buffer.\n"
"\n"
"Pack the provided values according to the format string and write the\n"
"packed bytes into the writable buffer starting at offset.  Note that the\n"
"offset is a required argument.  A negative offset counts from the end of\n"
"the buffer.  See help(struct) for more on format strings.");

#define PACK_INTO_METHODDEF    \
    {"pack_into", _PyCFunction_CAST(pack_into), METH_VALUE, pack_into__doc__},

static PyValue
pack_into_impl(PyObject *module, PyStructObject *s_object, Py_buffer *buffer,
               Py_ssize_t offset, const PyValue *values,
               Py_ssize_t values_length);

static PyValue
pack_into(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyStructObject *s_object = NULL;
    Py_buffer buffer = {NULL, NULL};
    Py_ssize_t offset;
    const PyValue *values;
    Py_ssize_t values_length;

    if (!_PyArg_CheckPositional("pack_into", nargs, 3, PY_SSIZE_T_MAX)) {
        goto exit;
    }
    if (!cache_struct_converter(module, PyValue_Borrow(args[0]), &s_object)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[1]))) {
        _PyArg_BadArgumentValue("pack_into", "argument 2", "read-write bytes-like object", PyValue_Borrow(args[1]));
        goto exit;
    }
    if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])), &buffer, PyBUF_WRITABLE) < 0) {
        _PyArg_BadArgument("pack_into", "argument 2", "read-write bytes-like object", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        goto exit;
    }
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
        offset = ival;
    }
    values = args + 3;
    values_length = nargs - 3;
    return_value = pack_into_impl(PyValue_AsKnownObjectBorrow(module), s_object, &buffer, offset, values, values_length);

exit:
    /* Cleanup for s_object */
    Py_XDECREF(s_object);
    /* Cleanup for buffer */
    if (buffer.obj) {
       PyBuffer_Release(&buffer);
    }

    return return_value;
}

PyDoc_STRVAR(unpack__doc__,
"unpack($module, format, buffer, /)\n"
"--\n"
"\n"
"Return a tuple containing values unpacked according to the format string.\n"
"\n"
"The buffer\'s size in bytes must be calcsize(format).  See help(struct)\n"
"for more on format strings.");

#define UNPACK_METHODDEF    \
    {"unpack", _PyCFunction_CAST(unpack), METH_VALUE, unpack__doc__},

static PyValue
unpack_impl(PyObject *module, PyStructObject *s_object, Py_buffer *buffer);

static PyValue
unpack(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyStructObject *s_object = NULL;
    Py_buffer buffer = {NULL, NULL};

    if (!_PyArg_CheckPositional("unpack", nargs, 2, 2)) {
        goto exit;
    }
    if (!cache_struct_converter(module, PyValue_Borrow(args[0]), &s_object)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[1]))) {
        PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[1]))->tp_name);
        goto exit;
    }
    if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])), &buffer, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    return_value = unpack_impl(PyValue_AsKnownObjectBorrow(module), s_object, &buffer);

exit:
    /* Cleanup for s_object */
    Py_XDECREF(s_object);
    /* Cleanup for buffer */
    if (buffer.obj) {
       PyBuffer_Release(&buffer);
    }

    return return_value;
}

PyDoc_STRVAR(unpack_from__doc__,
"unpack_from($module, format, /, buffer, offset=0)\n"
"--\n"
"\n"
"Return a tuple containing values unpacked according to the format string.\n"
"\n"
"The buffer must contain at least calcsize(format) bytes starting at\n"
"offset.  A negative offset counts from the end of the buffer.  See\n"
"help(struct) for more on format strings.");

#define UNPACK_FROM_METHODDEF    \
    {"unpack_from", _PyCFunction_CAST(unpack_from), METH_VALUE|METH_KEYWORDS, unpack_from__doc__},

static PyValue
unpack_from_impl(PyObject *module, PyStructObject *s_object,
                 Py_buffer *buffer, Py_ssize_t offset);

static PyValue
unpack_from(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(buffer)), _PyValue_STATIC_IMMORTAL(&_Py_ID(offset)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "buffer", "offset", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "unpack_from",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    PyStructObject *s_object = NULL;
    Py_buffer buffer = {NULL, NULL};
    Py_ssize_t offset = 0;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, _idx, 3)) {
        goto exit;
    }
    if (!cache_struct_converter(module, PyValue_Borrow(args[_idx[0]]), &s_object)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[_idx[1]]))) {
        PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[_idx[1]]))->tp_name);
        goto exit;
    }
    if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])), &buffer, PyBUF_SIMPLE) != 0) {
        goto exit;
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
            offset = ival;
        }
    }
    return_value = unpack_from_impl(PyValue_AsKnownObjectBorrow(module), s_object, &buffer, offset);

exit:
    /* Cleanup for s_object */
    Py_XDECREF(s_object);
    /* Cleanup for buffer */
    if (buffer.obj) {
       PyBuffer_Release(&buffer);
    }

    return return_value;
}

PyDoc_STRVAR(iter_unpack__doc__,
"iter_unpack($module, format, buffer, /)\n"
"--\n"
"\n"
"Return an iterator yielding tuples unpacked from the given bytes.\n"
"\n"
"The bytes are unpacked according to the format string, like a repeated\n"
"invocation of unpack_from().  Requires that the bytes length be\n"
"a multiple of calcsize(format).");

#define ITER_UNPACK_METHODDEF    \
    {"iter_unpack", _PyCFunction_CAST(iter_unpack), METH_VALUE, iter_unpack__doc__},

static PyValue
iter_unpack_impl(PyObject *module, PyStructObject *s_object,
                 PyValueBorrow buffer);

static PyValue
iter_unpack(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyStructObject *s_object = NULL;
    PyValueBorrow buffer;

    if (!_PyArg_CheckPositional("iter_unpack", nargs, 2, 2)) {
        goto exit;
    }
    if (!cache_struct_converter(module, PyValue_Borrow(args[0]), &s_object)) {
        goto exit;
    }
    buffer = PyValue_Borrow(args[1]);
    return_value = iter_unpack_impl(PyValue_AsKnownObjectBorrow(module), s_object, buffer);

exit:
    /* Cleanup for s_object */
    Py_XDECREF(s_object);

    return return_value;
}
/*[clinic end generated code: output=e4ecb60c0138321f input=a9049054013a1b77]*/
