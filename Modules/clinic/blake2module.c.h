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
#include "pycore_value_ops.h"     // _PyValue_IsTrue()
#include "pycore_value_slots.h"   // _PyValue_THUNK_EXIT()

PyDoc_STRVAR(py_blake2b_new__doc__,
"blake2b(data=b\'\', *, digest_size=_blake2.blake2b.MAX_DIGEST_SIZE,\n"
"        key=b\'\', salt=b\'\', person=b\'\', fanout=1, depth=1, leaf_size=0,\n"
"        node_offset=0, node_depth=0, inner_size=0, last_node=False,\n"
"        usedforsecurity=True, string=None)\n"
"--\n"
"\n"
"Return a new BLAKE2b hash object.");

static PyValue
py_blake2b_new_impl(PyTypeObject *type, PyValueBorrow data_obj,
                    int digest_size, Py_buffer *key, Py_buffer *salt,
                    Py_buffer *person, int fanout, int depth,
                    unsigned long leaf_size, unsigned long long node_offset,
                    int node_depth, int inner_size, int last_node,
                    int usedforsecurity, PyValueBorrow string);

static PyValue
py_blake2b_new_value(PyTypeObject *type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 14
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(data)), _PyValue_STATIC_IMMORTAL(&_Py_ID(digest_size)), _PyValue_STATIC_IMMORTAL(&_Py_ID(key)), _PyValue_STATIC_IMMORTAL(&_Py_ID(salt)), _PyValue_STATIC_IMMORTAL(&_Py_ID(person)), _PyValue_STATIC_IMMORTAL(&_Py_ID(fanout)), _PyValue_STATIC_IMMORTAL(&_Py_ID(depth)), _PyValue_STATIC_IMMORTAL(&_Py_ID(leaf_size)), _PyValue_STATIC_IMMORTAL(&_Py_ID(node_offset)), _PyValue_STATIC_IMMORTAL(&_Py_ID(node_depth)), _PyValue_STATIC_IMMORTAL(&_Py_ID(inner_size)), _PyValue_STATIC_IMMORTAL(&_Py_ID(last_node)), _PyValue_STATIC_IMMORTAL(&_Py_ID(usedforsecurity)), _PyValue_STATIC_IMMORTAL(&_Py_ID(string)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"data", "digest_size", "key", "salt", "person", "fanout", "depth", "leaf_size", "node_offset", "node_depth", "inner_size", "last_node", "usedforsecurity", "string", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "blake2b",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[14];
    PyValueBorrow data_obj = PyValue_Borrow(PyValue_NULL);
    int digest_size = HACL_HASH_BLAKE2B_OUT_BYTES;
    Py_buffer key = {.buf = "", .obj = NULL, .len = 0};
    Py_buffer salt = {.buf = "", .obj = NULL, .len = 0};
    Py_buffer person = {.buf = "", .obj = NULL, .len = 0};
    int fanout = 1;
    int depth = 1;
    unsigned long leaf_size = 0;
    unsigned long long node_offset = 0;
    int node_depth = 0;
    int inner_size = 0;
    int last_node = 0;
    int usedforsecurity = 1;
    PyValueBorrow string = PyValue_Borrow(PyValue_NULL);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 14)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        data_obj = PyValue_Borrow(args[_idx[0]]);
    }
    if (_idx[1] >= 0) {
        digest_size = _PyValue_AsInt(PyValue_Borrow(args[_idx[1]]));
        if (digest_size == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (_idx[2] >= 0) {
        if (!PyValue_IsObject(PyValue_Borrow(args[_idx[2]]))) {
            PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[_idx[2]]))->tp_name);
            goto exit;
        }
        if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[2]])), &key, PyBUF_SIMPLE) != 0) {
            goto exit;
        }
    }
    if (_idx[3] >= 0) {
        if (!PyValue_IsObject(PyValue_Borrow(args[_idx[3]]))) {
            PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[_idx[3]]))->tp_name);
            goto exit;
        }
        if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[3]])), &salt, PyBUF_SIMPLE) != 0) {
            goto exit;
        }
    }
    if (_idx[4] >= 0) {
        if (!PyValue_IsObject(PyValue_Borrow(args[_idx[4]]))) {
            PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[_idx[4]]))->tp_name);
            goto exit;
        }
        if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[4]])), &person, PyBUF_SIMPLE) != 0) {
            goto exit;
        }
    }
    if (_idx[5] >= 0) {
        fanout = _PyValue_AsInt(PyValue_Borrow(args[_idx[5]]));
        if (fanout == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (_idx[6] >= 0) {
        depth = _PyValue_AsInt(PyValue_Borrow(args[_idx[6]]));
        if (depth == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (_idx[7] >= 0) {
        {
            Py_ssize_t _bytes = _PyValue_AsNativeBytes(PyValue_Borrow(args[_idx[7]]), &leaf_size, sizeof(unsigned long),
                    Py_ASNATIVEBYTES_NATIVE_ENDIAN |
                    Py_ASNATIVEBYTES_ALLOW_INDEX |
                    Py_ASNATIVEBYTES_REJECT_NEGATIVE |
                    Py_ASNATIVEBYTES_UNSIGNED_BUFFER);
            if (_bytes < 0) {
                goto exit;
            }
            if ((size_t)_bytes > sizeof(unsigned long)) {
                PyErr_SetString(PyExc_OverflowError,
                            "Python int too large for C unsigned long");
                goto exit;
            }
        }
    }
    if (_idx[8] >= 0) {
        {
            Py_ssize_t _bytes = _PyValue_AsNativeBytes(PyValue_Borrow(args[_idx[8]]), &node_offset, sizeof(unsigned long long),
                    Py_ASNATIVEBYTES_NATIVE_ENDIAN |
                    Py_ASNATIVEBYTES_ALLOW_INDEX |
                    Py_ASNATIVEBYTES_REJECT_NEGATIVE |
                    Py_ASNATIVEBYTES_UNSIGNED_BUFFER);
            if (_bytes < 0) {
                goto exit;
            }
            if ((size_t)_bytes > sizeof(unsigned long long)) {
                PyErr_SetString(PyExc_OverflowError,
                            "Python int too large for C unsigned long long");
                goto exit;
            }
        }
    }
    if (_idx[9] >= 0) {
        node_depth = _PyValue_AsInt(PyValue_Borrow(args[_idx[9]]));
        if (node_depth == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (_idx[10] >= 0) {
        inner_size = _PyValue_AsInt(PyValue_Borrow(args[_idx[10]]));
        if (inner_size == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (_idx[11] >= 0) {
        last_node = _PyValue_IsTrue(PyValue_Borrow(args[_idx[11]]));
        if (last_node < 0) {
            goto exit;
        }
    }
    if (_idx[12] >= 0) {
        usedforsecurity = _PyValue_IsTrue(PyValue_Borrow(args[_idx[12]]));
        if (usedforsecurity < 0) {
            goto exit;
        }
    }
    if (_idx[13] >= 0) {
        string = PyValue_Borrow(args[_idx[13]]);
    }
    return_value = py_blake2b_new_impl(type, data_obj, digest_size, &key, &salt, &person, fanout, depth, leaf_size, node_offset, node_depth, inner_size, last_node, usedforsecurity, string);

exit:
    /* Cleanup for key */
    if (key.obj) {
       PyBuffer_Release(&key);
    }
    /* Cleanup for salt */
    if (salt.obj) {
       PyBuffer_Release(&salt);
    }
    /* Cleanup for person */
    if (person.obj) {
       PyBuffer_Release(&person);
    }

    return return_value;
}

Py_VALUE_ENTRY static PyObject *
py_blake2b_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _PyValue_THUNK_EXIT(py_blake2b_new_value(type, _PyTuple_ITEMS(args), nargs, NULL), BOX_SITE_CALL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return NULL;
    }
    PyValue result = py_blake2b_new_value(type, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return _PyValue_THUNK_EXIT(result, BOX_SITE_CALL);
}

PyDoc_STRVAR(py_blake2s_new__doc__,
"blake2s(data=b\'\', *, digest_size=_blake2.blake2s.MAX_DIGEST_SIZE,\n"
"        key=b\'\', salt=b\'\', person=b\'\', fanout=1, depth=1, leaf_size=0,\n"
"        node_offset=0, node_depth=0, inner_size=0, last_node=False,\n"
"        usedforsecurity=True, string=None)\n"
"--\n"
"\n"
"Return a new BLAKE2s hash object.");

static PyValue
py_blake2s_new_impl(PyTypeObject *type, PyValueBorrow data_obj,
                    int digest_size, Py_buffer *key, Py_buffer *salt,
                    Py_buffer *person, int fanout, int depth,
                    unsigned long leaf_size, unsigned long long node_offset,
                    int node_depth, int inner_size, int last_node,
                    int usedforsecurity, PyValueBorrow string);

static PyValue
py_blake2s_new_value(PyTypeObject *type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 14
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(data)), _PyValue_STATIC_IMMORTAL(&_Py_ID(digest_size)), _PyValue_STATIC_IMMORTAL(&_Py_ID(key)), _PyValue_STATIC_IMMORTAL(&_Py_ID(salt)), _PyValue_STATIC_IMMORTAL(&_Py_ID(person)), _PyValue_STATIC_IMMORTAL(&_Py_ID(fanout)), _PyValue_STATIC_IMMORTAL(&_Py_ID(depth)), _PyValue_STATIC_IMMORTAL(&_Py_ID(leaf_size)), _PyValue_STATIC_IMMORTAL(&_Py_ID(node_offset)), _PyValue_STATIC_IMMORTAL(&_Py_ID(node_depth)), _PyValue_STATIC_IMMORTAL(&_Py_ID(inner_size)), _PyValue_STATIC_IMMORTAL(&_Py_ID(last_node)), _PyValue_STATIC_IMMORTAL(&_Py_ID(usedforsecurity)), _PyValue_STATIC_IMMORTAL(&_Py_ID(string)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"data", "digest_size", "key", "salt", "person", "fanout", "depth", "leaf_size", "node_offset", "node_depth", "inner_size", "last_node", "usedforsecurity", "string", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "blake2s",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[14];
    PyValueBorrow data_obj = PyValue_Borrow(PyValue_NULL);
    int digest_size = HACL_HASH_BLAKE2S_OUT_BYTES;
    Py_buffer key = {.buf = "", .obj = NULL, .len = 0};
    Py_buffer salt = {.buf = "", .obj = NULL, .len = 0};
    Py_buffer person = {.buf = "", .obj = NULL, .len = 0};
    int fanout = 1;
    int depth = 1;
    unsigned long leaf_size = 0;
    unsigned long long node_offset = 0;
    int node_depth = 0;
    int inner_size = 0;
    int last_node = 0;
    int usedforsecurity = 1;
    PyValueBorrow string = PyValue_Borrow(PyValue_NULL);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 14)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        data_obj = PyValue_Borrow(args[_idx[0]]);
    }
    if (_idx[1] >= 0) {
        digest_size = _PyValue_AsInt(PyValue_Borrow(args[_idx[1]]));
        if (digest_size == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (_idx[2] >= 0) {
        if (!PyValue_IsObject(PyValue_Borrow(args[_idx[2]]))) {
            PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[_idx[2]]))->tp_name);
            goto exit;
        }
        if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[2]])), &key, PyBUF_SIMPLE) != 0) {
            goto exit;
        }
    }
    if (_idx[3] >= 0) {
        if (!PyValue_IsObject(PyValue_Borrow(args[_idx[3]]))) {
            PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[_idx[3]]))->tp_name);
            goto exit;
        }
        if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[3]])), &salt, PyBUF_SIMPLE) != 0) {
            goto exit;
        }
    }
    if (_idx[4] >= 0) {
        if (!PyValue_IsObject(PyValue_Borrow(args[_idx[4]]))) {
            PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[_idx[4]]))->tp_name);
            goto exit;
        }
        if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[4]])), &person, PyBUF_SIMPLE) != 0) {
            goto exit;
        }
    }
    if (_idx[5] >= 0) {
        fanout = _PyValue_AsInt(PyValue_Borrow(args[_idx[5]]));
        if (fanout == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (_idx[6] >= 0) {
        depth = _PyValue_AsInt(PyValue_Borrow(args[_idx[6]]));
        if (depth == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (_idx[7] >= 0) {
        {
            Py_ssize_t _bytes = _PyValue_AsNativeBytes(PyValue_Borrow(args[_idx[7]]), &leaf_size, sizeof(unsigned long),
                    Py_ASNATIVEBYTES_NATIVE_ENDIAN |
                    Py_ASNATIVEBYTES_ALLOW_INDEX |
                    Py_ASNATIVEBYTES_REJECT_NEGATIVE |
                    Py_ASNATIVEBYTES_UNSIGNED_BUFFER);
            if (_bytes < 0) {
                goto exit;
            }
            if ((size_t)_bytes > sizeof(unsigned long)) {
                PyErr_SetString(PyExc_OverflowError,
                            "Python int too large for C unsigned long");
                goto exit;
            }
        }
    }
    if (_idx[8] >= 0) {
        {
            Py_ssize_t _bytes = _PyValue_AsNativeBytes(PyValue_Borrow(args[_idx[8]]), &node_offset, sizeof(unsigned long long),
                    Py_ASNATIVEBYTES_NATIVE_ENDIAN |
                    Py_ASNATIVEBYTES_ALLOW_INDEX |
                    Py_ASNATIVEBYTES_REJECT_NEGATIVE |
                    Py_ASNATIVEBYTES_UNSIGNED_BUFFER);
            if (_bytes < 0) {
                goto exit;
            }
            if ((size_t)_bytes > sizeof(unsigned long long)) {
                PyErr_SetString(PyExc_OverflowError,
                            "Python int too large for C unsigned long long");
                goto exit;
            }
        }
    }
    if (_idx[9] >= 0) {
        node_depth = _PyValue_AsInt(PyValue_Borrow(args[_idx[9]]));
        if (node_depth == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (_idx[10] >= 0) {
        inner_size = _PyValue_AsInt(PyValue_Borrow(args[_idx[10]]));
        if (inner_size == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (_idx[11] >= 0) {
        last_node = _PyValue_IsTrue(PyValue_Borrow(args[_idx[11]]));
        if (last_node < 0) {
            goto exit;
        }
    }
    if (_idx[12] >= 0) {
        usedforsecurity = _PyValue_IsTrue(PyValue_Borrow(args[_idx[12]]));
        if (usedforsecurity < 0) {
            goto exit;
        }
    }
    if (_idx[13] >= 0) {
        string = PyValue_Borrow(args[_idx[13]]);
    }
    return_value = py_blake2s_new_impl(type, data_obj, digest_size, &key, &salt, &person, fanout, depth, leaf_size, node_offset, node_depth, inner_size, last_node, usedforsecurity, string);

exit:
    /* Cleanup for key */
    if (key.obj) {
       PyBuffer_Release(&key);
    }
    /* Cleanup for salt */
    if (salt.obj) {
       PyBuffer_Release(&salt);
    }
    /* Cleanup for person */
    if (person.obj) {
       PyBuffer_Release(&person);
    }

    return return_value;
}

Py_VALUE_ENTRY static PyObject *
py_blake2s_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _PyValue_THUNK_EXIT(py_blake2s_new_value(type, _PyTuple_ITEMS(args), nargs, NULL), BOX_SITE_CALL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return NULL;
    }
    PyValue result = py_blake2s_new_value(type, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return _PyValue_THUNK_EXIT(result, BOX_SITE_CALL);
}

PyDoc_STRVAR(_blake2_blake2b_copy__doc__,
"copy($self, /)\n"
"--\n"
"\n"
"Return a copy of the hash object.");

#define _BLAKE2_BLAKE2B_COPY_METHODDEF    \
    {"copy", _PyCFunction_CAST(_blake2_blake2b_copy), METH_VALUE|METH_METHOD|METH_KEYWORDS, _blake2_blake2b_copy__doc__},

static PyValue
_blake2_blake2b_copy_impl(Blake2Object *self, PyTypeObject *cls);

static PyValue
_blake2_blake2b_copy(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "copy() takes no arguments");
        goto exit;
    }
    return_value = _blake2_blake2b_copy_impl((Blake2Object *)PyValue_AsKnownObjectBorrow(self), cls);

exit:
    return return_value;
}

PyDoc_STRVAR(_blake2_blake2b_update__doc__,
"update($self, data, /)\n"
"--\n"
"\n"
"Update this hash object\'s state with the provided bytes-like object.");

#define _BLAKE2_BLAKE2B_UPDATE_METHODDEF    \
    {"update", _PyCFunction_CAST(_blake2_blake2b_update), METH_VALUE|METH_O, _blake2_blake2b_update__doc__},

static PyValue
_blake2_blake2b_update_impl(Blake2Object *self, PyValueBorrow data);

static PyValue
_blake2_blake2b_update(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow data;

    data = PyValue_Borrow(args[0]);
    return_value = _blake2_blake2b_update_impl((Blake2Object *)PyValue_AsKnownObjectBorrow(self), data);

    return return_value;
}

PyDoc_STRVAR(_blake2_blake2b_digest__doc__,
"digest($self, /)\n"
"--\n"
"\n"
"Return the digest value as a bytes object.");

#define _BLAKE2_BLAKE2B_DIGEST_METHODDEF    \
    {"digest", _PyCFunction_CAST(_blake2_blake2b_digest), METH_VALUE|METH_NOARGS, _blake2_blake2b_digest__doc__},

static PyValue
_blake2_blake2b_digest_impl(Blake2Object *self);

static PyValue
_blake2_blake2b_digest(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _blake2_blake2b_digest_impl((Blake2Object *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_blake2_blake2b_hexdigest__doc__,
"hexdigest($self, /)\n"
"--\n"
"\n"
"Return the digest value as a string of hexadecimal digits.");

#define _BLAKE2_BLAKE2B_HEXDIGEST_METHODDEF    \
    {"hexdigest", _PyCFunction_CAST(_blake2_blake2b_hexdigest), METH_VALUE|METH_NOARGS, _blake2_blake2b_hexdigest__doc__},

static PyValue
_blake2_blake2b_hexdigest_impl(Blake2Object *self);

static PyValue
_blake2_blake2b_hexdigest(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _blake2_blake2b_hexdigest_impl((Blake2Object *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}
/*[clinic end generated code: output=f9e8a46914bccf10 input=a9049054013a1b77]*/
