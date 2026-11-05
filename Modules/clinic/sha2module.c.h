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
#include "pycore_value_ops.h"     // _PyValue_IsTrue()

PyDoc_STRVAR(SHA256Type_copy__doc__,
"copy($self, /)\n"
"--\n"
"\n"
"Return a copy of the hash object.");

#define SHA256TYPE_COPY_METHODDEF    \
    {"copy", _PyCFunction_CAST(SHA256Type_copy), METH_VALUE|METH_METHOD|METH_KEYWORDS, SHA256Type_copy__doc__},

static PyValue
SHA256Type_copy_impl(SHA256object *self, PyTypeObject *cls);

static PyValue
SHA256Type_copy(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "copy() takes no arguments");
        goto exit;
    }
    return_value = SHA256Type_copy_impl((SHA256object *)PyValue_AsKnownObjectBorrow(self), cls);

exit:
    return return_value;
}

PyDoc_STRVAR(SHA512Type_copy__doc__,
"copy($self, /)\n"
"--\n"
"\n"
"Return a copy of the hash object.");

#define SHA512TYPE_COPY_METHODDEF    \
    {"copy", _PyCFunction_CAST(SHA512Type_copy), METH_VALUE|METH_METHOD|METH_KEYWORDS, SHA512Type_copy__doc__},

static PyValue
SHA512Type_copy_impl(SHA512object *self, PyTypeObject *cls);

static PyValue
SHA512Type_copy(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "copy() takes no arguments");
        goto exit;
    }
    return_value = SHA512Type_copy_impl((SHA512object *)PyValue_AsKnownObjectBorrow(self), cls);

exit:
    return return_value;
}

PyDoc_STRVAR(SHA256Type_digest__doc__,
"digest($self, /)\n"
"--\n"
"\n"
"Return the digest value as a bytes object.");

#define SHA256TYPE_DIGEST_METHODDEF    \
    {"digest", _PyCFunction_CAST(SHA256Type_digest), METH_VALUE|METH_NOARGS, SHA256Type_digest__doc__},

static PyValue
SHA256Type_digest_impl(SHA256object *self);

static PyValue
SHA256Type_digest(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = SHA256Type_digest_impl((SHA256object *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(SHA512Type_digest__doc__,
"digest($self, /)\n"
"--\n"
"\n"
"Return the digest value as a bytes object.");

#define SHA512TYPE_DIGEST_METHODDEF    \
    {"digest", _PyCFunction_CAST(SHA512Type_digest), METH_VALUE|METH_NOARGS, SHA512Type_digest__doc__},

static PyValue
SHA512Type_digest_impl(SHA512object *self);

static PyValue
SHA512Type_digest(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = SHA512Type_digest_impl((SHA512object *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(SHA256Type_hexdigest__doc__,
"hexdigest($self, /)\n"
"--\n"
"\n"
"Return the digest value as a string of hexadecimal digits.");

#define SHA256TYPE_HEXDIGEST_METHODDEF    \
    {"hexdigest", _PyCFunction_CAST(SHA256Type_hexdigest), METH_VALUE|METH_NOARGS, SHA256Type_hexdigest__doc__},

static PyValue
SHA256Type_hexdigest_impl(SHA256object *self);

static PyValue
SHA256Type_hexdigest(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = SHA256Type_hexdigest_impl((SHA256object *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(SHA512Type_hexdigest__doc__,
"hexdigest($self, /)\n"
"--\n"
"\n"
"Return the digest value as a string of hexadecimal digits.");

#define SHA512TYPE_HEXDIGEST_METHODDEF    \
    {"hexdigest", _PyCFunction_CAST(SHA512Type_hexdigest), METH_VALUE|METH_NOARGS, SHA512Type_hexdigest__doc__},

static PyValue
SHA512Type_hexdigest_impl(SHA512object *self);

static PyValue
SHA512Type_hexdigest(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = SHA512Type_hexdigest_impl((SHA512object *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(SHA256Type_update__doc__,
"update($self, obj, /)\n"
"--\n"
"\n"
"Update this hash object\'s state with the provided string.");

#define SHA256TYPE_UPDATE_METHODDEF    \
    {"update", _PyCFunction_CAST(SHA256Type_update), METH_VALUE|METH_O, SHA256Type_update__doc__},

static PyValue
SHA256Type_update_impl(SHA256object *self, PyValueBorrow obj);

static PyValue
SHA256Type_update(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow obj;

    obj = PyValue_Borrow(args[0]);
    return_value = SHA256Type_update_impl((SHA256object *)PyValue_AsKnownObjectBorrow(self), obj);

    return return_value;
}

PyDoc_STRVAR(SHA512Type_update__doc__,
"update($self, obj, /)\n"
"--\n"
"\n"
"Update this hash object\'s state with the provided string.");

#define SHA512TYPE_UPDATE_METHODDEF    \
    {"update", _PyCFunction_CAST(SHA512Type_update), METH_VALUE|METH_O, SHA512Type_update__doc__},

static PyValue
SHA512Type_update_impl(SHA512object *self, PyValueBorrow obj);

static PyValue
SHA512Type_update(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow obj;

    obj = PyValue_Borrow(args[0]);
    return_value = SHA512Type_update_impl((SHA512object *)PyValue_AsKnownObjectBorrow(self), obj);

    return return_value;
}

PyDoc_STRVAR(_sha2_sha256__doc__,
"sha256($module, /, data=b\'\', *, usedforsecurity=True, string=None)\n"
"--\n"
"\n"
"Return a new SHA-256 hash object; optionally initialized with a string.");

#define _SHA2_SHA256_METHODDEF    \
    {"sha256", _PyCFunction_CAST(_sha2_sha256), METH_VALUE|METH_KEYWORDS, _sha2_sha256__doc__},

static PyValue
_sha2_sha256_impl(PyObject *module, PyValueBorrow data, int usedforsecurity,
                  PyValueBorrow string_obj);

static PyValue
_sha2_sha256(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .fname = "sha256",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    PyValueBorrow data = PyValue_Borrow(PyValue_NULL);
    int usedforsecurity = 1;
    PyValueBorrow string_obj = PyValue_Borrow(PyValue_NULL);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 3)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        data = PyValue_Borrow(args[_idx[0]]);
    }
    if (_idx[1] >= 0) {
        usedforsecurity = _PyValue_IsTrue(PyValue_Borrow(args[_idx[1]]));
        if (usedforsecurity < 0) {
            goto exit;
        }
    }
    if (_idx[2] >= 0) {
        string_obj = PyValue_Borrow(args[_idx[2]]);
    }
    return_value = _sha2_sha256_impl(PyValue_AsKnownObjectBorrow(module), data, usedforsecurity, string_obj);

exit:
    return return_value;
}

PyDoc_STRVAR(_sha2_sha224__doc__,
"sha224($module, /, data=b\'\', *, usedforsecurity=True, string=None)\n"
"--\n"
"\n"
"Return a new SHA-224 hash object; optionally initialized with a string.");

#define _SHA2_SHA224_METHODDEF    \
    {"sha224", _PyCFunction_CAST(_sha2_sha224), METH_VALUE|METH_KEYWORDS, _sha2_sha224__doc__},

static PyValue
_sha2_sha224_impl(PyObject *module, PyValueBorrow data, int usedforsecurity,
                  PyValueBorrow string_obj);

static PyValue
_sha2_sha224(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .fname = "sha224",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    PyValueBorrow data = PyValue_Borrow(PyValue_NULL);
    int usedforsecurity = 1;
    PyValueBorrow string_obj = PyValue_Borrow(PyValue_NULL);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 3)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        data = PyValue_Borrow(args[_idx[0]]);
    }
    if (_idx[1] >= 0) {
        usedforsecurity = _PyValue_IsTrue(PyValue_Borrow(args[_idx[1]]));
        if (usedforsecurity < 0) {
            goto exit;
        }
    }
    if (_idx[2] >= 0) {
        string_obj = PyValue_Borrow(args[_idx[2]]);
    }
    return_value = _sha2_sha224_impl(PyValue_AsKnownObjectBorrow(module), data, usedforsecurity, string_obj);

exit:
    return return_value;
}

PyDoc_STRVAR(_sha2_sha512__doc__,
"sha512($module, /, data=b\'\', *, usedforsecurity=True, string=None)\n"
"--\n"
"\n"
"Return a new SHA-512 hash object; optionally initialized with a string.");

#define _SHA2_SHA512_METHODDEF    \
    {"sha512", _PyCFunction_CAST(_sha2_sha512), METH_VALUE|METH_KEYWORDS, _sha2_sha512__doc__},

static PyValue
_sha2_sha512_impl(PyObject *module, PyValueBorrow data, int usedforsecurity,
                  PyValueBorrow string_obj);

static PyValue
_sha2_sha512(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .fname = "sha512",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    PyValueBorrow data = PyValue_Borrow(PyValue_NULL);
    int usedforsecurity = 1;
    PyValueBorrow string_obj = PyValue_Borrow(PyValue_NULL);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 3)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        data = PyValue_Borrow(args[_idx[0]]);
    }
    if (_idx[1] >= 0) {
        usedforsecurity = _PyValue_IsTrue(PyValue_Borrow(args[_idx[1]]));
        if (usedforsecurity < 0) {
            goto exit;
        }
    }
    if (_idx[2] >= 0) {
        string_obj = PyValue_Borrow(args[_idx[2]]);
    }
    return_value = _sha2_sha512_impl(PyValue_AsKnownObjectBorrow(module), data, usedforsecurity, string_obj);

exit:
    return return_value;
}

PyDoc_STRVAR(_sha2_sha384__doc__,
"sha384($module, /, data=b\'\', *, usedforsecurity=True, string=None)\n"
"--\n"
"\n"
"Return a new SHA-384 hash object; optionally initialized with a string.");

#define _SHA2_SHA384_METHODDEF    \
    {"sha384", _PyCFunction_CAST(_sha2_sha384), METH_VALUE|METH_KEYWORDS, _sha2_sha384__doc__},

static PyValue
_sha2_sha384_impl(PyObject *module, PyValueBorrow data, int usedforsecurity,
                  PyValueBorrow string_obj);

static PyValue
_sha2_sha384(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .fname = "sha384",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    PyValueBorrow data = PyValue_Borrow(PyValue_NULL);
    int usedforsecurity = 1;
    PyValueBorrow string_obj = PyValue_Borrow(PyValue_NULL);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 3)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        data = PyValue_Borrow(args[_idx[0]]);
    }
    if (_idx[1] >= 0) {
        usedforsecurity = _PyValue_IsTrue(PyValue_Borrow(args[_idx[1]]));
        if (usedforsecurity < 0) {
            goto exit;
        }
    }
    if (_idx[2] >= 0) {
        string_obj = PyValue_Borrow(args[_idx[2]]);
    }
    return_value = _sha2_sha384_impl(PyValue_AsKnownObjectBorrow(module), data, usedforsecurity, string_obj);

exit:
    return return_value;
}
/*[clinic end generated code: output=cc9c41201bbf6e44 input=a9049054013a1b77]*/
