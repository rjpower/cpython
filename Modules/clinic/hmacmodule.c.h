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
#include "pycore_value_call.h"    // METH_VALUE

PyDoc_STRVAR(_hmac_new__doc__,
"new($module, /, key, msg=None, digestmod=None)\n"
"--\n"
"\n"
"Return a new HMAC object.");

#define _HMAC_NEW_METHODDEF    \
    {"new", _PyCFunction_CAST(_hmac_new), METH_VALUE|METH_KEYWORDS, _hmac_new__doc__},

static PyValue
_hmac_new_impl(PyObject *module, PyValueBorrow keyobj, PyValueBorrow msgobj,
               PyValueBorrow hash_info_ref);

static PyValue
_hmac_new(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(key)), _PyValue_STATIC_IMMORTAL(&_Py_ID(msg)), _PyValue_STATIC_IMMORTAL(&_Py_ID(digestmod)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"key", "msg", "digestmod", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "new",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    PyValueBorrow keyobj;
    PyValueBorrow msgobj = PyValue_Borrow(PyValue_NULL);
    PyValueBorrow hash_info_ref = PyValue_Borrow(PyValue_NULL);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, _idx, 3)) {
        goto exit;
    }
    keyobj = PyValue_Borrow(args[_idx[0]]);
    if (_idx[1] >= 0) {
        msgobj = PyValue_Borrow(args[_idx[1]]);
    }
    if (_idx[2] >= 0) {
        hash_info_ref = PyValue_Borrow(args[_idx[2]]);
    }
    return_value = _hmac_new_impl(PyValue_AsKnownObjectBorrow(module), keyobj, msgobj, hash_info_ref);

exit:
    return return_value;
}

PyDoc_STRVAR(_hmac_HMAC_copy__doc__,
"copy($self, /)\n"
"--\n"
"\n"
"Return a copy (\"clone\") of the HMAC object.");

#define _HMAC_HMAC_COPY_METHODDEF    \
    {"copy", _PyCFunction_CAST(_hmac_HMAC_copy), METH_VALUE|METH_METHOD|METH_KEYWORDS, _hmac_HMAC_copy__doc__},

static PyValue
_hmac_HMAC_copy_impl(HMACObject *self, PyTypeObject *cls);

static PyValue
_hmac_HMAC_copy(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "copy() takes no arguments");
        goto exit;
    }
    return_value = _hmac_HMAC_copy_impl((HMACObject *)PyValue_AsKnownObjectBorrow(self), cls);

exit:
    return return_value;
}

PyDoc_STRVAR(_hmac_HMAC_update__doc__,
"update($self, /, msg)\n"
"--\n"
"\n"
"Update the HMAC object with the given message.");

#define _HMAC_HMAC_UPDATE_METHODDEF    \
    {"update", _PyCFunction_CAST(_hmac_HMAC_update), METH_VALUE|METH_KEYWORDS, _hmac_HMAC_update__doc__},

static PyValue
_hmac_HMAC_update_impl(HMACObject *self, PyValueBorrow msgobj);

static PyValue
_hmac_HMAC_update(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(msg)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"msg", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "update",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    PyValueBorrow msgobj;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    msgobj = PyValue_Borrow(args[_idx[0]]);
    return_value = _hmac_HMAC_update_impl((HMACObject *)PyValue_AsKnownObjectBorrow(self), msgobj);

exit:
    return return_value;
}

PyDoc_STRVAR(_hmac_HMAC_digest__doc__,
"digest($self, /)\n"
"--\n"
"\n"
"Return the digest of the bytes passed to the update() method so far.\n"
"\n"
"This method may raise a MemoryError.");

#define _HMAC_HMAC_DIGEST_METHODDEF    \
    {"digest", _PyCFunction_CAST(_hmac_HMAC_digest), METH_VALUE|METH_NOARGS, _hmac_HMAC_digest__doc__},

static PyValue
_hmac_HMAC_digest_impl(HMACObject *self);

static PyValue
_hmac_HMAC_digest(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _hmac_HMAC_digest_impl((HMACObject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_hmac_HMAC_hexdigest__doc__,
"hexdigest($self, /)\n"
"--\n"
"\n"
"Return hexadecimal digest of the bytes passed to the update() method so far.\n"
"\n"
"This may be used to exchange the value safely in email or other\n"
"non-binary environments.\n"
"\n"
"This method may raise a MemoryError.");

#define _HMAC_HMAC_HEXDIGEST_METHODDEF    \
    {"hexdigest", _PyCFunction_CAST(_hmac_HMAC_hexdigest), METH_VALUE|METH_NOARGS, _hmac_HMAC_hexdigest__doc__},

static PyValue
_hmac_HMAC_hexdigest_impl(HMACObject *self);

static PyValue
_hmac_HMAC_hexdigest(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _hmac_HMAC_hexdigest_impl((HMACObject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

static PyObject *
_hmac_HMAC_name_get_impl(HMACObject *self);

static PyObject *
_hmac_HMAC_name_get(PyObject *self, void *Py_UNUSED(context))
{
    return _hmac_HMAC_name_get_impl((HMACObject *)self);
}

static PyObject *
_hmac_HMAC_block_size_get_impl(HMACObject *self);

static PyObject *
_hmac_HMAC_block_size_get(PyObject *self, void *Py_UNUSED(context))
{
    return _hmac_HMAC_block_size_get_impl((HMACObject *)self);
}

static PyObject *
_hmac_HMAC_digest_size_get_impl(HMACObject *self);

static PyObject *
_hmac_HMAC_digest_size_get(PyObject *self, void *Py_UNUSED(context))
{
    return _hmac_HMAC_digest_size_get_impl((HMACObject *)self);
}

PyDoc_STRVAR(_hmac_compute_digest__doc__,
"compute_digest($module, /, key, msg, digest)\n"
"--\n"
"\n");

#define _HMAC_COMPUTE_DIGEST_METHODDEF    \
    {"compute_digest", _PyCFunction_CAST(_hmac_compute_digest), METH_VALUE|METH_KEYWORDS, _hmac_compute_digest__doc__},

static PyValue
_hmac_compute_digest_impl(PyObject *module, PyValueBorrow key,
                          PyValueBorrow msg, PyValueBorrow digest);

static PyValue
_hmac_compute_digest(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(key)), _PyValue_STATIC_IMMORTAL(&_Py_ID(msg)), _PyValue_STATIC_IMMORTAL(&_Py_ID(digest)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"key", "msg", "digest", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "compute_digest",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    PyValueBorrow key;
    PyValueBorrow msg;
    PyValueBorrow digest;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 3, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, _idx, 3)) {
        goto exit;
    }
    key = PyValue_Borrow(args[_idx[0]]);
    msg = PyValue_Borrow(args[_idx[1]]);
    digest = PyValue_Borrow(args[_idx[2]]);
    return_value = _hmac_compute_digest_impl(PyValue_AsKnownObjectBorrow(module), key, msg, digest);

exit:
    return return_value;
}

PyDoc_STRVAR(_hmac_compute_md5__doc__,
"compute_md5($module, key, msg, /)\n"
"--\n"
"\n");

#define _HMAC_COMPUTE_MD5_METHODDEF    \
    {"compute_md5", _PyCFunction_CAST(_hmac_compute_md5), METH_VALUE, _hmac_compute_md5__doc__},

static PyValue
_hmac_compute_md5_impl(PyObject *module, PyValueBorrow key,
                       PyValueBorrow msg);

static PyValue
_hmac_compute_md5(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow key;
    PyValueBorrow msg;

    if (!_PyArg_CheckPositional("compute_md5", nargs, 2, 2)) {
        goto exit;
    }
    key = PyValue_Borrow(args[0]);
    msg = PyValue_Borrow(args[1]);
    return_value = _hmac_compute_md5_impl(PyValue_AsKnownObjectBorrow(module), key, msg);

exit:
    return return_value;
}

PyDoc_STRVAR(_hmac_compute_sha1__doc__,
"compute_sha1($module, key, msg, /)\n"
"--\n"
"\n");

#define _HMAC_COMPUTE_SHA1_METHODDEF    \
    {"compute_sha1", _PyCFunction_CAST(_hmac_compute_sha1), METH_VALUE, _hmac_compute_sha1__doc__},

static PyValue
_hmac_compute_sha1_impl(PyObject *module, PyValueBorrow key,
                        PyValueBorrow msg);

static PyValue
_hmac_compute_sha1(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow key;
    PyValueBorrow msg;

    if (!_PyArg_CheckPositional("compute_sha1", nargs, 2, 2)) {
        goto exit;
    }
    key = PyValue_Borrow(args[0]);
    msg = PyValue_Borrow(args[1]);
    return_value = _hmac_compute_sha1_impl(PyValue_AsKnownObjectBorrow(module), key, msg);

exit:
    return return_value;
}

PyDoc_STRVAR(_hmac_compute_sha2_224__doc__,
"compute_sha224($module, key, msg, /)\n"
"--\n"
"\n");

#define _HMAC_COMPUTE_SHA2_224_METHODDEF    \
    {"compute_sha224", _PyCFunction_CAST(_hmac_compute_sha2_224), METH_VALUE, _hmac_compute_sha2_224__doc__},

static PyValue
_hmac_compute_sha2_224_impl(PyObject *module, PyValueBorrow key,
                            PyValueBorrow msg);

static PyValue
_hmac_compute_sha2_224(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow key;
    PyValueBorrow msg;

    if (!_PyArg_CheckPositional("compute_sha224", nargs, 2, 2)) {
        goto exit;
    }
    key = PyValue_Borrow(args[0]);
    msg = PyValue_Borrow(args[1]);
    return_value = _hmac_compute_sha2_224_impl(PyValue_AsKnownObjectBorrow(module), key, msg);

exit:
    return return_value;
}

PyDoc_STRVAR(_hmac_compute_sha2_256__doc__,
"compute_sha256($module, key, msg, /)\n"
"--\n"
"\n");

#define _HMAC_COMPUTE_SHA2_256_METHODDEF    \
    {"compute_sha256", _PyCFunction_CAST(_hmac_compute_sha2_256), METH_VALUE, _hmac_compute_sha2_256__doc__},

static PyValue
_hmac_compute_sha2_256_impl(PyObject *module, PyValueBorrow key,
                            PyValueBorrow msg);

static PyValue
_hmac_compute_sha2_256(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow key;
    PyValueBorrow msg;

    if (!_PyArg_CheckPositional("compute_sha256", nargs, 2, 2)) {
        goto exit;
    }
    key = PyValue_Borrow(args[0]);
    msg = PyValue_Borrow(args[1]);
    return_value = _hmac_compute_sha2_256_impl(PyValue_AsKnownObjectBorrow(module), key, msg);

exit:
    return return_value;
}

PyDoc_STRVAR(_hmac_compute_sha2_384__doc__,
"compute_sha384($module, key, msg, /)\n"
"--\n"
"\n");

#define _HMAC_COMPUTE_SHA2_384_METHODDEF    \
    {"compute_sha384", _PyCFunction_CAST(_hmac_compute_sha2_384), METH_VALUE, _hmac_compute_sha2_384__doc__},

static PyValue
_hmac_compute_sha2_384_impl(PyObject *module, PyValueBorrow key,
                            PyValueBorrow msg);

static PyValue
_hmac_compute_sha2_384(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow key;
    PyValueBorrow msg;

    if (!_PyArg_CheckPositional("compute_sha384", nargs, 2, 2)) {
        goto exit;
    }
    key = PyValue_Borrow(args[0]);
    msg = PyValue_Borrow(args[1]);
    return_value = _hmac_compute_sha2_384_impl(PyValue_AsKnownObjectBorrow(module), key, msg);

exit:
    return return_value;
}

PyDoc_STRVAR(_hmac_compute_sha2_512__doc__,
"compute_sha512($module, key, msg, /)\n"
"--\n"
"\n");

#define _HMAC_COMPUTE_SHA2_512_METHODDEF    \
    {"compute_sha512", _PyCFunction_CAST(_hmac_compute_sha2_512), METH_VALUE, _hmac_compute_sha2_512__doc__},

static PyValue
_hmac_compute_sha2_512_impl(PyObject *module, PyValueBorrow key,
                            PyValueBorrow msg);

static PyValue
_hmac_compute_sha2_512(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow key;
    PyValueBorrow msg;

    if (!_PyArg_CheckPositional("compute_sha512", nargs, 2, 2)) {
        goto exit;
    }
    key = PyValue_Borrow(args[0]);
    msg = PyValue_Borrow(args[1]);
    return_value = _hmac_compute_sha2_512_impl(PyValue_AsKnownObjectBorrow(module), key, msg);

exit:
    return return_value;
}

PyDoc_STRVAR(_hmac_compute_sha3_224__doc__,
"compute_sha3_224($module, key, msg, /)\n"
"--\n"
"\n");

#define _HMAC_COMPUTE_SHA3_224_METHODDEF    \
    {"compute_sha3_224", _PyCFunction_CAST(_hmac_compute_sha3_224), METH_VALUE, _hmac_compute_sha3_224__doc__},

static PyValue
_hmac_compute_sha3_224_impl(PyObject *module, PyValueBorrow key,
                            PyValueBorrow msg);

static PyValue
_hmac_compute_sha3_224(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow key;
    PyValueBorrow msg;

    if (!_PyArg_CheckPositional("compute_sha3_224", nargs, 2, 2)) {
        goto exit;
    }
    key = PyValue_Borrow(args[0]);
    msg = PyValue_Borrow(args[1]);
    return_value = _hmac_compute_sha3_224_impl(PyValue_AsKnownObjectBorrow(module), key, msg);

exit:
    return return_value;
}

PyDoc_STRVAR(_hmac_compute_sha3_256__doc__,
"compute_sha3_256($module, key, msg, /)\n"
"--\n"
"\n");

#define _HMAC_COMPUTE_SHA3_256_METHODDEF    \
    {"compute_sha3_256", _PyCFunction_CAST(_hmac_compute_sha3_256), METH_VALUE, _hmac_compute_sha3_256__doc__},

static PyValue
_hmac_compute_sha3_256_impl(PyObject *module, PyValueBorrow key,
                            PyValueBorrow msg);

static PyValue
_hmac_compute_sha3_256(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow key;
    PyValueBorrow msg;

    if (!_PyArg_CheckPositional("compute_sha3_256", nargs, 2, 2)) {
        goto exit;
    }
    key = PyValue_Borrow(args[0]);
    msg = PyValue_Borrow(args[1]);
    return_value = _hmac_compute_sha3_256_impl(PyValue_AsKnownObjectBorrow(module), key, msg);

exit:
    return return_value;
}

PyDoc_STRVAR(_hmac_compute_sha3_384__doc__,
"compute_sha3_384($module, key, msg, /)\n"
"--\n"
"\n");

#define _HMAC_COMPUTE_SHA3_384_METHODDEF    \
    {"compute_sha3_384", _PyCFunction_CAST(_hmac_compute_sha3_384), METH_VALUE, _hmac_compute_sha3_384__doc__},

static PyValue
_hmac_compute_sha3_384_impl(PyObject *module, PyValueBorrow key,
                            PyValueBorrow msg);

static PyValue
_hmac_compute_sha3_384(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow key;
    PyValueBorrow msg;

    if (!_PyArg_CheckPositional("compute_sha3_384", nargs, 2, 2)) {
        goto exit;
    }
    key = PyValue_Borrow(args[0]);
    msg = PyValue_Borrow(args[1]);
    return_value = _hmac_compute_sha3_384_impl(PyValue_AsKnownObjectBorrow(module), key, msg);

exit:
    return return_value;
}

PyDoc_STRVAR(_hmac_compute_sha3_512__doc__,
"compute_sha3_512($module, key, msg, /)\n"
"--\n"
"\n");

#define _HMAC_COMPUTE_SHA3_512_METHODDEF    \
    {"compute_sha3_512", _PyCFunction_CAST(_hmac_compute_sha3_512), METH_VALUE, _hmac_compute_sha3_512__doc__},

static PyValue
_hmac_compute_sha3_512_impl(PyObject *module, PyValueBorrow key,
                            PyValueBorrow msg);

static PyValue
_hmac_compute_sha3_512(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow key;
    PyValueBorrow msg;

    if (!_PyArg_CheckPositional("compute_sha3_512", nargs, 2, 2)) {
        goto exit;
    }
    key = PyValue_Borrow(args[0]);
    msg = PyValue_Borrow(args[1]);
    return_value = _hmac_compute_sha3_512_impl(PyValue_AsKnownObjectBorrow(module), key, msg);

exit:
    return return_value;
}

PyDoc_STRVAR(_hmac_compute_blake2s_32__doc__,
"compute_blake2s_32($module, key, msg, /)\n"
"--\n"
"\n");

#define _HMAC_COMPUTE_BLAKE2S_32_METHODDEF    \
    {"compute_blake2s_32", _PyCFunction_CAST(_hmac_compute_blake2s_32), METH_VALUE, _hmac_compute_blake2s_32__doc__},

static PyValue
_hmac_compute_blake2s_32_impl(PyObject *module, PyValueBorrow key,
                              PyValueBorrow msg);

static PyValue
_hmac_compute_blake2s_32(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow key;
    PyValueBorrow msg;

    if (!_PyArg_CheckPositional("compute_blake2s_32", nargs, 2, 2)) {
        goto exit;
    }
    key = PyValue_Borrow(args[0]);
    msg = PyValue_Borrow(args[1]);
    return_value = _hmac_compute_blake2s_32_impl(PyValue_AsKnownObjectBorrow(module), key, msg);

exit:
    return return_value;
}

PyDoc_STRVAR(_hmac_compute_blake2b_32__doc__,
"compute_blake2b_32($module, key, msg, /)\n"
"--\n"
"\n");

#define _HMAC_COMPUTE_BLAKE2B_32_METHODDEF    \
    {"compute_blake2b_32", _PyCFunction_CAST(_hmac_compute_blake2b_32), METH_VALUE, _hmac_compute_blake2b_32__doc__},

static PyValue
_hmac_compute_blake2b_32_impl(PyObject *module, PyValueBorrow key,
                              PyValueBorrow msg);

static PyValue
_hmac_compute_blake2b_32(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow key;
    PyValueBorrow msg;

    if (!_PyArg_CheckPositional("compute_blake2b_32", nargs, 2, 2)) {
        goto exit;
    }
    key = PyValue_Borrow(args[0]);
    msg = PyValue_Borrow(args[1]);
    return_value = _hmac_compute_blake2b_32_impl(PyValue_AsKnownObjectBorrow(module), key, msg);

exit:
    return return_value;
}
#define _HMAC_HMAC_NAME_GETSETDEF {"name", (getter)_hmac_HMAC_name_get, (setter)NULL, NULL},

#define _HMAC_HMAC_BLOCK_SIZE_GETSETDEF {"block_size", (getter)_hmac_HMAC_block_size_get, (setter)NULL, NULL},

#define _HMAC_HMAC_DIGEST_SIZE_GETSETDEF {"digest_size", (getter)_hmac_HMAC_digest_size_get, (setter)NULL, NULL},

/*[clinic end generated code: output=a5cb39373b60d1d8 input=a9049054013a1b77]*/
