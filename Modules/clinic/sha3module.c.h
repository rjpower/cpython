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
#include "pycore_value_ops.h"     // _PyValue_IsTrue()
#include "pycore_value_slots.h"   // _PyValue_THUNK_EXIT()

PyDoc_STRVAR(py_sha3_new__doc__,
"sha3_224(data=b\'\', *, usedforsecurity=True, string=None)\n"
"--\n"
"\n"
"Return a new SHA3 hash object.");

static PyValue
py_sha3_new_impl(PyTypeObject *type, PyValueBorrow data_obj,
                 int usedforsecurity, PyValueBorrow string);

static PyValue
py_sha3_new_value(PyTypeObject *type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(data)), _PyValue_STATIC_IMMORTAL(&_Py_ID(usedforsecurity)), _PyValue_STATIC_IMMORTAL(&_Py_ID(string)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"data", "usedforsecurity", "string", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "sha3_224",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    PyValueBorrow data_obj = PyValue_Borrow(PyValue_NULL);
    int usedforsecurity = 1;
    PyValueBorrow string = PyValue_Borrow(PyValue_NULL);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 3)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        data_obj = PyValue_Borrow(args[_idx[0]]);
    }
    if (_idx[1] >= 0) {
        usedforsecurity = _PyValue_IsTrue(PyValue_Borrow(args[_idx[1]]));
        if (usedforsecurity < 0) {
            goto exit;
        }
    }
    if (_idx[2] >= 0) {
        string = PyValue_Borrow(args[_idx[2]]);
    }
    return_value = py_sha3_new_impl(type, data_obj, usedforsecurity, string);

exit:
    return return_value;
}

Py_VALUE_ENTRY static PyObject *
py_sha3_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _PyValue_THUNK_EXIT(py_sha3_new_value(type, _PyTuple_ITEMS(args), nargs, NULL), BOX_SITE_CALL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return NULL;
    }
    PyValue result = py_sha3_new_value(type, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return _PyValue_THUNK_EXIT(result, BOX_SITE_CALL);
}

PyDoc_STRVAR(_sha3_sha3_224_copy__doc__,
"copy($self, /)\n"
"--\n"
"\n"
"Return a copy of the hash object.");

#define _SHA3_SHA3_224_COPY_METHODDEF    \
    {"copy", _PyCFunction_CAST(_sha3_sha3_224_copy), METH_VALUE|METH_METHOD|METH_KEYWORDS, _sha3_sha3_224_copy__doc__},

static PyValue
_sha3_sha3_224_copy_impl(SHA3object *self, PyTypeObject *cls);

static PyValue
_sha3_sha3_224_copy(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "copy() takes no arguments");
        goto exit;
    }
    return_value = _sha3_sha3_224_copy_impl((SHA3object *)PyValue_AsKnownObjectBorrow(self), cls);

exit:
    return return_value;
}

PyDoc_STRVAR(_sha3_sha3_224_digest__doc__,
"digest($self, /)\n"
"--\n"
"\n"
"Return the digest value as a bytes object.");

#define _SHA3_SHA3_224_DIGEST_METHODDEF    \
    {"digest", _PyCFunction_CAST(_sha3_sha3_224_digest), METH_VALUE|METH_NOARGS, _sha3_sha3_224_digest__doc__},

static PyValue
_sha3_sha3_224_digest_impl(SHA3object *self);

static PyValue
_sha3_sha3_224_digest(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _sha3_sha3_224_digest_impl((SHA3object *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_sha3_sha3_224_hexdigest__doc__,
"hexdigest($self, /)\n"
"--\n"
"\n"
"Return the digest value as a string of hexadecimal digits.");

#define _SHA3_SHA3_224_HEXDIGEST_METHODDEF    \
    {"hexdigest", _PyCFunction_CAST(_sha3_sha3_224_hexdigest), METH_VALUE|METH_NOARGS, _sha3_sha3_224_hexdigest__doc__},

static PyValue
_sha3_sha3_224_hexdigest_impl(SHA3object *self);

static PyValue
_sha3_sha3_224_hexdigest(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _sha3_sha3_224_hexdigest_impl((SHA3object *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_sha3_sha3_224_update__doc__,
"update($self, data, /)\n"
"--\n"
"\n"
"Update this hash object\'s state with the provided bytes-like object.");

#define _SHA3_SHA3_224_UPDATE_METHODDEF    \
    {"update", _PyCFunction_CAST(_sha3_sha3_224_update), METH_VALUE|METH_O, _sha3_sha3_224_update__doc__},

static PyValue
_sha3_sha3_224_update_impl(SHA3object *self, PyValueBorrow data);

static PyValue
_sha3_sha3_224_update(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow data;

    data = PyValue_Borrow(args[0]);
    return_value = _sha3_sha3_224_update_impl((SHA3object *)PyValue_AsKnownObjectBorrow(self), data);

    return return_value;
}

PyDoc_STRVAR(_sha3_shake_128_digest__doc__,
"digest($self, /, length)\n"
"--\n"
"\n"
"Return the digest value as a bytes object.");

#define _SHA3_SHAKE_128_DIGEST_METHODDEF    \
    {"digest", _PyCFunction_CAST(_sha3_shake_128_digest), METH_VALUE|METH_KEYWORDS, _sha3_shake_128_digest__doc__},

static PyValue
_sha3_shake_128_digest_impl(SHA3object *self, Py_ssize_t length);

static PyValue
_sha3_shake_128_digest(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(length)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"length", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "digest",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    Py_ssize_t length;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    {
        Py_ssize_t ival = -1;
        PyValue iobj = _PyValue_Index(PyValue_Borrow(args[_idx[0]]));
        if (!PyValue_IsNull(iobj)) {
            ival = _PyValue_IntAsSsize_t(PyValue_Borrow(iobj));
            PyValue_CLOSE(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        length = ival;
        if (length < 0) {
            PyErr_SetString(PyExc_ValueError,
                            "length cannot be negative");
            goto exit;
        }
    }
    return_value = _sha3_shake_128_digest_impl((SHA3object *)PyValue_AsKnownObjectBorrow(self), length);

exit:
    return return_value;
}

PyDoc_STRVAR(_sha3_shake_128_hexdigest__doc__,
"hexdigest($self, /, length)\n"
"--\n"
"\n"
"Return the digest value as a string of hexadecimal digits.");

#define _SHA3_SHAKE_128_HEXDIGEST_METHODDEF    \
    {"hexdigest", _PyCFunction_CAST(_sha3_shake_128_hexdigest), METH_VALUE|METH_KEYWORDS, _sha3_shake_128_hexdigest__doc__},

static PyValue
_sha3_shake_128_hexdigest_impl(SHA3object *self, Py_ssize_t length);

static PyValue
_sha3_shake_128_hexdigest(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(length)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"length", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "hexdigest",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    Py_ssize_t length;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    {
        Py_ssize_t ival = -1;
        PyValue iobj = _PyValue_Index(PyValue_Borrow(args[_idx[0]]));
        if (!PyValue_IsNull(iobj)) {
            ival = _PyValue_IntAsSsize_t(PyValue_Borrow(iobj));
            PyValue_CLOSE(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        length = ival;
        if (length < 0) {
            PyErr_SetString(PyExc_ValueError,
                            "length cannot be negative");
            goto exit;
        }
    }
    return_value = _sha3_shake_128_hexdigest_impl((SHA3object *)PyValue_AsKnownObjectBorrow(self), length);

exit:
    return return_value;
}
/*[clinic end generated code: output=213b26b45c308588 input=a9049054013a1b77]*/
