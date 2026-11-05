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
#include "pycore_unicodeobject.h" // _PyUnicode_AsUTF8NoNUL()
#include "pycore_value_call.h"    // METH_VALUE
#include "pycore_value_ops.h"     // _PyValue_Index()

PyDoc_STRVAR(_hashlib_HASH_copy__doc__,
"copy($self, /)\n"
"--\n"
"\n"
"Return a copy of the hash object.");

#define _HASHLIB_HASH_COPY_METHODDEF    \
    {"copy", _PyCFunction_CAST(_hashlib_HASH_copy), METH_VALUE|METH_NOARGS, _hashlib_HASH_copy__doc__},

static PyValue
_hashlib_HASH_copy_impl(HASHobject *self);

static PyValue
_hashlib_HASH_copy(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _hashlib_HASH_copy_impl((HASHobject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_hashlib_HASH_digest__doc__,
"digest($self, /)\n"
"--\n"
"\n"
"Return the digest value as a bytes object.");

#define _HASHLIB_HASH_DIGEST_METHODDEF    \
    {"digest", _PyCFunction_CAST(_hashlib_HASH_digest), METH_VALUE|METH_NOARGS, _hashlib_HASH_digest__doc__},

static PyValue
_hashlib_HASH_digest_impl(HASHobject *self);

static PyValue
_hashlib_HASH_digest(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _hashlib_HASH_digest_impl((HASHobject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_hashlib_HASH_hexdigest__doc__,
"hexdigest($self, /)\n"
"--\n"
"\n"
"Return the digest value as a string of hexadecimal digits.");

#define _HASHLIB_HASH_HEXDIGEST_METHODDEF    \
    {"hexdigest", _PyCFunction_CAST(_hashlib_HASH_hexdigest), METH_VALUE|METH_NOARGS, _hashlib_HASH_hexdigest__doc__},

static PyValue
_hashlib_HASH_hexdigest_impl(HASHobject *self);

static PyValue
_hashlib_HASH_hexdigest(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _hashlib_HASH_hexdigest_impl((HASHobject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_hashlib_HASH_update__doc__,
"update($self, obj, /)\n"
"--\n"
"\n"
"Update this hash object\'s state with the provided string.");

#define _HASHLIB_HASH_UPDATE_METHODDEF    \
    {"update", _PyCFunction_CAST(_hashlib_HASH_update), METH_VALUE|METH_O, _hashlib_HASH_update__doc__},

static PyValue
_hashlib_HASH_update_impl(HASHobject *self, PyValueBorrow obj);

static PyValue
_hashlib_HASH_update(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow obj;

    obj = PyValue_Borrow(args[0]);
    return_value = _hashlib_HASH_update_impl((HASHobject *)PyValue_AsKnownObjectBorrow(self), obj);

    return return_value;
}

#if defined(PY_OPENSSL_HAS_SHAKE)

PyDoc_STRVAR(_hashlib_HASHXOF_digest__doc__,
"digest($self, /, length)\n"
"--\n"
"\n"
"Return the digest value as a bytes object.");

#define _HASHLIB_HASHXOF_DIGEST_METHODDEF    \
    {"digest", _PyCFunction_CAST(_hashlib_HASHXOF_digest), METH_VALUE|METH_KEYWORDS, _hashlib_HASHXOF_digest__doc__},

static PyValue
_hashlib_HASHXOF_digest_impl(HASHobject *self, Py_ssize_t length);

static PyValue
_hashlib_HASHXOF_digest(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
    return_value = _hashlib_HASHXOF_digest_impl((HASHobject *)PyValue_AsKnownObjectBorrow(self), length);

exit:
    return return_value;
}

#endif /* defined(PY_OPENSSL_HAS_SHAKE) */

#if defined(PY_OPENSSL_HAS_SHAKE)

PyDoc_STRVAR(_hashlib_HASHXOF_hexdigest__doc__,
"hexdigest($self, /, length)\n"
"--\n"
"\n"
"Return the digest value as a string of hexadecimal digits.");

#define _HASHLIB_HASHXOF_HEXDIGEST_METHODDEF    \
    {"hexdigest", _PyCFunction_CAST(_hashlib_HASHXOF_hexdigest), METH_VALUE|METH_KEYWORDS, _hashlib_HASHXOF_hexdigest__doc__},

static PyValue
_hashlib_HASHXOF_hexdigest_impl(HASHobject *self, Py_ssize_t length);

static PyValue
_hashlib_HASHXOF_hexdigest(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
    return_value = _hashlib_HASHXOF_hexdigest_impl((HASHobject *)PyValue_AsKnownObjectBorrow(self), length);

exit:
    return return_value;
}

#endif /* defined(PY_OPENSSL_HAS_SHAKE) */

PyDoc_STRVAR(_hashlib_HASH_new__doc__,
"new($module, /, name, data=b\'\', *, usedforsecurity=True, string=None)\n"
"--\n"
"\n"
"Return a new hash object using the named algorithm.\n"
"\n"
"An optional string argument may be provided and will be\n"
"automatically hashed.\n"
"\n"
"The MD5 and SHA1 algorithms are always supported.");

#define _HASHLIB_HASH_NEW_METHODDEF    \
    {"new", _PyCFunction_CAST(_hashlib_HASH_new), METH_VALUE|METH_KEYWORDS, _hashlib_HASH_new__doc__},

static PyValue
_hashlib_HASH_new_impl(PyObject *module, const char *name,
                       PyValueBorrow data, int usedforsecurity,
                       PyValueBorrow string);

static PyValue
_hashlib_HASH_new(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 4
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(name)), _PyValue_STATIC_IMMORTAL(&_Py_ID(data)), _PyValue_STATIC_IMMORTAL(&_Py_ID(usedforsecurity)), _PyValue_STATIC_IMMORTAL(&_Py_ID(string)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"name", "data", "usedforsecurity", "string", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "new",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[4];
    const char *name;
    PyValueBorrow data = PyValue_Borrow(PyValue_NULL);
    int usedforsecurity = 1;
    PyValueBorrow string = PyValue_Borrow(PyValue_NULL);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 4)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[_idx[0]]))) {
        _PyArg_BadArgumentValue("new", "argument 'name'", "str", PyValue_Borrow(args[_idx[0]]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])))) {
        _PyArg_BadArgument("new", "argument 'name'", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])));
        goto exit;
    }
    name = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])));
    if (name == NULL) {
        goto exit;
    }
    if (_idx[1] >= 0) {
        data = PyValue_Borrow(args[_idx[1]]);
    }
    if (_idx[2] >= 0) {
        usedforsecurity = _PyValue_IsTrue(PyValue_Borrow(args[_idx[2]]));
        if (usedforsecurity < 0) {
            goto exit;
        }
    }
    if (_idx[3] >= 0) {
        string = PyValue_Borrow(args[_idx[3]]);
    }
    return_value = _hashlib_HASH_new_impl(PyValue_AsKnownObjectBorrow(module), name, data, usedforsecurity, string);

exit:
    return return_value;
}

PyDoc_STRVAR(_hashlib_openssl_md5__doc__,
"openssl_md5($module, /, data=b\'\', *, usedforsecurity=True, string=None)\n"
"--\n"
"\n"
"Returns a md5 hash object; optionally initialized with a string");

#define _HASHLIB_OPENSSL_MD5_METHODDEF    \
    {"openssl_md5", _PyCFunction_CAST(_hashlib_openssl_md5), METH_VALUE|METH_KEYWORDS, _hashlib_openssl_md5__doc__},

static PyValue
_hashlib_openssl_md5_impl(PyObject *module, PyValueBorrow data,
                          int usedforsecurity, PyValueBorrow string);

static PyValue
_hashlib_openssl_md5(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .fname = "openssl_md5",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    PyValueBorrow data = PyValue_Borrow(PyValue_NULL);
    int usedforsecurity = 1;
    PyValueBorrow string = PyValue_Borrow(PyValue_NULL);

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
        string = PyValue_Borrow(args[_idx[2]]);
    }
    return_value = _hashlib_openssl_md5_impl(PyValue_AsKnownObjectBorrow(module), data, usedforsecurity, string);

exit:
    return return_value;
}

PyDoc_STRVAR(_hashlib_openssl_sha1__doc__,
"openssl_sha1($module, /, data=b\'\', *, usedforsecurity=True, string=None)\n"
"--\n"
"\n"
"Returns a sha1 hash object; optionally initialized with a string");

#define _HASHLIB_OPENSSL_SHA1_METHODDEF    \
    {"openssl_sha1", _PyCFunction_CAST(_hashlib_openssl_sha1), METH_VALUE|METH_KEYWORDS, _hashlib_openssl_sha1__doc__},

static PyValue
_hashlib_openssl_sha1_impl(PyObject *module, PyValueBorrow data,
                           int usedforsecurity, PyValueBorrow string);

static PyValue
_hashlib_openssl_sha1(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .fname = "openssl_sha1",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    PyValueBorrow data = PyValue_Borrow(PyValue_NULL);
    int usedforsecurity = 1;
    PyValueBorrow string = PyValue_Borrow(PyValue_NULL);

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
        string = PyValue_Borrow(args[_idx[2]]);
    }
    return_value = _hashlib_openssl_sha1_impl(PyValue_AsKnownObjectBorrow(module), data, usedforsecurity, string);

exit:
    return return_value;
}

PyDoc_STRVAR(_hashlib_openssl_sha224__doc__,
"openssl_sha224($module, /, data=b\'\', *, usedforsecurity=True,\n"
"               string=None)\n"
"--\n"
"\n"
"Returns a sha224 hash object; optionally initialized with a string");

#define _HASHLIB_OPENSSL_SHA224_METHODDEF    \
    {"openssl_sha224", _PyCFunction_CAST(_hashlib_openssl_sha224), METH_VALUE|METH_KEYWORDS, _hashlib_openssl_sha224__doc__},

static PyValue
_hashlib_openssl_sha224_impl(PyObject *module, PyValueBorrow data,
                             int usedforsecurity, PyValueBorrow string);

static PyValue
_hashlib_openssl_sha224(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .fname = "openssl_sha224",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    PyValueBorrow data = PyValue_Borrow(PyValue_NULL);
    int usedforsecurity = 1;
    PyValueBorrow string = PyValue_Borrow(PyValue_NULL);

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
        string = PyValue_Borrow(args[_idx[2]]);
    }
    return_value = _hashlib_openssl_sha224_impl(PyValue_AsKnownObjectBorrow(module), data, usedforsecurity, string);

exit:
    return return_value;
}

PyDoc_STRVAR(_hashlib_openssl_sha256__doc__,
"openssl_sha256($module, /, data=b\'\', *, usedforsecurity=True,\n"
"               string=None)\n"
"--\n"
"\n"
"Returns a sha256 hash object; optionally initialized with a string");

#define _HASHLIB_OPENSSL_SHA256_METHODDEF    \
    {"openssl_sha256", _PyCFunction_CAST(_hashlib_openssl_sha256), METH_VALUE|METH_KEYWORDS, _hashlib_openssl_sha256__doc__},

static PyValue
_hashlib_openssl_sha256_impl(PyObject *module, PyValueBorrow data,
                             int usedforsecurity, PyValueBorrow string);

static PyValue
_hashlib_openssl_sha256(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .fname = "openssl_sha256",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    PyValueBorrow data = PyValue_Borrow(PyValue_NULL);
    int usedforsecurity = 1;
    PyValueBorrow string = PyValue_Borrow(PyValue_NULL);

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
        string = PyValue_Borrow(args[_idx[2]]);
    }
    return_value = _hashlib_openssl_sha256_impl(PyValue_AsKnownObjectBorrow(module), data, usedforsecurity, string);

exit:
    return return_value;
}

PyDoc_STRVAR(_hashlib_openssl_sha384__doc__,
"openssl_sha384($module, /, data=b\'\', *, usedforsecurity=True,\n"
"               string=None)\n"
"--\n"
"\n"
"Returns a sha384 hash object; optionally initialized with a string");

#define _HASHLIB_OPENSSL_SHA384_METHODDEF    \
    {"openssl_sha384", _PyCFunction_CAST(_hashlib_openssl_sha384), METH_VALUE|METH_KEYWORDS, _hashlib_openssl_sha384__doc__},

static PyValue
_hashlib_openssl_sha384_impl(PyObject *module, PyValueBorrow data,
                             int usedforsecurity, PyValueBorrow string);

static PyValue
_hashlib_openssl_sha384(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .fname = "openssl_sha384",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    PyValueBorrow data = PyValue_Borrow(PyValue_NULL);
    int usedforsecurity = 1;
    PyValueBorrow string = PyValue_Borrow(PyValue_NULL);

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
        string = PyValue_Borrow(args[_idx[2]]);
    }
    return_value = _hashlib_openssl_sha384_impl(PyValue_AsKnownObjectBorrow(module), data, usedforsecurity, string);

exit:
    return return_value;
}

PyDoc_STRVAR(_hashlib_openssl_sha512__doc__,
"openssl_sha512($module, /, data=b\'\', *, usedforsecurity=True,\n"
"               string=None)\n"
"--\n"
"\n"
"Returns a sha512 hash object; optionally initialized with a string");

#define _HASHLIB_OPENSSL_SHA512_METHODDEF    \
    {"openssl_sha512", _PyCFunction_CAST(_hashlib_openssl_sha512), METH_VALUE|METH_KEYWORDS, _hashlib_openssl_sha512__doc__},

static PyValue
_hashlib_openssl_sha512_impl(PyObject *module, PyValueBorrow data,
                             int usedforsecurity, PyValueBorrow string);

static PyValue
_hashlib_openssl_sha512(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .fname = "openssl_sha512",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    PyValueBorrow data = PyValue_Borrow(PyValue_NULL);
    int usedforsecurity = 1;
    PyValueBorrow string = PyValue_Borrow(PyValue_NULL);

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
        string = PyValue_Borrow(args[_idx[2]]);
    }
    return_value = _hashlib_openssl_sha512_impl(PyValue_AsKnownObjectBorrow(module), data, usedforsecurity, string);

exit:
    return return_value;
}

#if defined(PY_OPENSSL_HAS_SHA3)

PyDoc_STRVAR(_hashlib_openssl_sha3_224__doc__,
"openssl_sha3_224($module, /, data=b\'\', *, usedforsecurity=True,\n"
"                 string=None)\n"
"--\n"
"\n"
"Returns a sha3-224 hash object; optionally initialized with a string");

#define _HASHLIB_OPENSSL_SHA3_224_METHODDEF    \
    {"openssl_sha3_224", _PyCFunction_CAST(_hashlib_openssl_sha3_224), METH_VALUE|METH_KEYWORDS, _hashlib_openssl_sha3_224__doc__},

static PyValue
_hashlib_openssl_sha3_224_impl(PyObject *module, PyValueBorrow data,
                               int usedforsecurity, PyValueBorrow string);

static PyValue
_hashlib_openssl_sha3_224(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .fname = "openssl_sha3_224",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    PyValueBorrow data = PyValue_Borrow(PyValue_NULL);
    int usedforsecurity = 1;
    PyValueBorrow string = PyValue_Borrow(PyValue_NULL);

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
        string = PyValue_Borrow(args[_idx[2]]);
    }
    return_value = _hashlib_openssl_sha3_224_impl(PyValue_AsKnownObjectBorrow(module), data, usedforsecurity, string);

exit:
    return return_value;
}

#endif /* defined(PY_OPENSSL_HAS_SHA3) */

#if defined(PY_OPENSSL_HAS_SHA3)

PyDoc_STRVAR(_hashlib_openssl_sha3_256__doc__,
"openssl_sha3_256($module, /, data=b\'\', *, usedforsecurity=True,\n"
"                 string=None)\n"
"--\n"
"\n"
"Returns a sha3-256 hash object; optionally initialized with a string");

#define _HASHLIB_OPENSSL_SHA3_256_METHODDEF    \
    {"openssl_sha3_256", _PyCFunction_CAST(_hashlib_openssl_sha3_256), METH_VALUE|METH_KEYWORDS, _hashlib_openssl_sha3_256__doc__},

static PyValue
_hashlib_openssl_sha3_256_impl(PyObject *module, PyValueBorrow data,
                               int usedforsecurity, PyValueBorrow string);

static PyValue
_hashlib_openssl_sha3_256(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .fname = "openssl_sha3_256",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    PyValueBorrow data = PyValue_Borrow(PyValue_NULL);
    int usedforsecurity = 1;
    PyValueBorrow string = PyValue_Borrow(PyValue_NULL);

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
        string = PyValue_Borrow(args[_idx[2]]);
    }
    return_value = _hashlib_openssl_sha3_256_impl(PyValue_AsKnownObjectBorrow(module), data, usedforsecurity, string);

exit:
    return return_value;
}

#endif /* defined(PY_OPENSSL_HAS_SHA3) */

#if defined(PY_OPENSSL_HAS_SHA3)

PyDoc_STRVAR(_hashlib_openssl_sha3_384__doc__,
"openssl_sha3_384($module, /, data=b\'\', *, usedforsecurity=True,\n"
"                 string=None)\n"
"--\n"
"\n"
"Returns a sha3-384 hash object; optionally initialized with a string");

#define _HASHLIB_OPENSSL_SHA3_384_METHODDEF    \
    {"openssl_sha3_384", _PyCFunction_CAST(_hashlib_openssl_sha3_384), METH_VALUE|METH_KEYWORDS, _hashlib_openssl_sha3_384__doc__},

static PyValue
_hashlib_openssl_sha3_384_impl(PyObject *module, PyValueBorrow data,
                               int usedforsecurity, PyValueBorrow string);

static PyValue
_hashlib_openssl_sha3_384(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .fname = "openssl_sha3_384",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    PyValueBorrow data = PyValue_Borrow(PyValue_NULL);
    int usedforsecurity = 1;
    PyValueBorrow string = PyValue_Borrow(PyValue_NULL);

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
        string = PyValue_Borrow(args[_idx[2]]);
    }
    return_value = _hashlib_openssl_sha3_384_impl(PyValue_AsKnownObjectBorrow(module), data, usedforsecurity, string);

exit:
    return return_value;
}

#endif /* defined(PY_OPENSSL_HAS_SHA3) */

#if defined(PY_OPENSSL_HAS_SHA3)

PyDoc_STRVAR(_hashlib_openssl_sha3_512__doc__,
"openssl_sha3_512($module, /, data=b\'\', *, usedforsecurity=True,\n"
"                 string=None)\n"
"--\n"
"\n"
"Returns a sha3-512 hash object; optionally initialized with a string");

#define _HASHLIB_OPENSSL_SHA3_512_METHODDEF    \
    {"openssl_sha3_512", _PyCFunction_CAST(_hashlib_openssl_sha3_512), METH_VALUE|METH_KEYWORDS, _hashlib_openssl_sha3_512__doc__},

static PyValue
_hashlib_openssl_sha3_512_impl(PyObject *module, PyValueBorrow data,
                               int usedforsecurity, PyValueBorrow string);

static PyValue
_hashlib_openssl_sha3_512(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .fname = "openssl_sha3_512",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    PyValueBorrow data = PyValue_Borrow(PyValue_NULL);
    int usedforsecurity = 1;
    PyValueBorrow string = PyValue_Borrow(PyValue_NULL);

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
        string = PyValue_Borrow(args[_idx[2]]);
    }
    return_value = _hashlib_openssl_sha3_512_impl(PyValue_AsKnownObjectBorrow(module), data, usedforsecurity, string);

exit:
    return return_value;
}

#endif /* defined(PY_OPENSSL_HAS_SHA3) */

#if defined(PY_OPENSSL_HAS_SHAKE)

PyDoc_STRVAR(_hashlib_openssl_shake_128__doc__,
"openssl_shake_128($module, /, data=b\'\', *, usedforsecurity=True,\n"
"                  string=None)\n"
"--\n"
"\n"
"Returns a shake-128 variable hash object; optionally initialized with a string");

#define _HASHLIB_OPENSSL_SHAKE_128_METHODDEF    \
    {"openssl_shake_128", _PyCFunction_CAST(_hashlib_openssl_shake_128), METH_VALUE|METH_KEYWORDS, _hashlib_openssl_shake_128__doc__},

static PyValue
_hashlib_openssl_shake_128_impl(PyObject *module, PyValueBorrow data,
                                int usedforsecurity, PyValueBorrow string);

static PyValue
_hashlib_openssl_shake_128(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .fname = "openssl_shake_128",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    PyValueBorrow data = PyValue_Borrow(PyValue_NULL);
    int usedforsecurity = 1;
    PyValueBorrow string = PyValue_Borrow(PyValue_NULL);

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
        string = PyValue_Borrow(args[_idx[2]]);
    }
    return_value = _hashlib_openssl_shake_128_impl(PyValue_AsKnownObjectBorrow(module), data, usedforsecurity, string);

exit:
    return return_value;
}

#endif /* defined(PY_OPENSSL_HAS_SHAKE) */

#if defined(PY_OPENSSL_HAS_SHAKE)

PyDoc_STRVAR(_hashlib_openssl_shake_256__doc__,
"openssl_shake_256($module, /, data=b\'\', *, usedforsecurity=True,\n"
"                  string=None)\n"
"--\n"
"\n"
"Returns a shake-256 variable hash object; optionally initialized with a string");

#define _HASHLIB_OPENSSL_SHAKE_256_METHODDEF    \
    {"openssl_shake_256", _PyCFunction_CAST(_hashlib_openssl_shake_256), METH_VALUE|METH_KEYWORDS, _hashlib_openssl_shake_256__doc__},

static PyValue
_hashlib_openssl_shake_256_impl(PyObject *module, PyValueBorrow data,
                                int usedforsecurity, PyValueBorrow string);

static PyValue
_hashlib_openssl_shake_256(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .fname = "openssl_shake_256",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    PyValueBorrow data = PyValue_Borrow(PyValue_NULL);
    int usedforsecurity = 1;
    PyValueBorrow string = PyValue_Borrow(PyValue_NULL);

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
        string = PyValue_Borrow(args[_idx[2]]);
    }
    return_value = _hashlib_openssl_shake_256_impl(PyValue_AsKnownObjectBorrow(module), data, usedforsecurity, string);

exit:
    return return_value;
}

#endif /* defined(PY_OPENSSL_HAS_SHAKE) */

PyDoc_STRVAR(pbkdf2_hmac__doc__,
"pbkdf2_hmac($module, /, hash_name, password, salt, iterations,\n"
"            dklen=None)\n"
"--\n"
"\n"
"Password based key derivation function 2 (PKCS #5 v2.0) with HMAC as pseudorandom function.");

#define PBKDF2_HMAC_METHODDEF    \
    {"pbkdf2_hmac", _PyCFunction_CAST(pbkdf2_hmac), METH_VALUE|METH_KEYWORDS, pbkdf2_hmac__doc__},

static PyValue
pbkdf2_hmac_impl(PyObject *module, const char *hash_name,
                 Py_buffer *password, Py_buffer *salt, long iterations,
                 PyValueBorrow dklen_obj);

static PyValue
pbkdf2_hmac(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 5
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(hash_name)), _PyValue_STATIC_IMMORTAL(&_Py_ID(password)), _PyValue_STATIC_IMMORTAL(&_Py_ID(salt)), _PyValue_STATIC_IMMORTAL(&_Py_ID(iterations)), _PyValue_STATIC_IMMORTAL(&_Py_ID(dklen)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"hash_name", "password", "salt", "iterations", "dklen", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "pbkdf2_hmac",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[5];
    const char *hash_name;
    Py_buffer password = {NULL, NULL};
    Py_buffer salt = {NULL, NULL};
    long iterations;
    PyValueBorrow dklen_obj = PyValue_Borrow(PyValue_None);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 4, /*maxpos*/ 5, /*minkw*/ 0, /*varpos*/ 0, _idx, 5)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[_idx[0]]))) {
        _PyArg_BadArgumentValue("pbkdf2_hmac", "argument 'hash_name'", "str", PyValue_Borrow(args[_idx[0]]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])))) {
        _PyArg_BadArgument("pbkdf2_hmac", "argument 'hash_name'", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])));
        goto exit;
    }
    hash_name = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])));
    if (hash_name == NULL) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[_idx[1]]))) {
        PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[_idx[1]]))->tp_name);
        goto exit;
    }
    if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])), &password, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[_idx[2]]))) {
        PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[_idx[2]]))->tp_name);
        goto exit;
    }
    if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[2]])), &salt, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    iterations = _PyValue_AsLong(PyValue_Borrow(args[_idx[3]]));
    if (iterations == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (_idx[4] >= 0) {
        dklen_obj = PyValue_Borrow(args[_idx[4]]);
    }
    return_value = pbkdf2_hmac_impl(PyValue_AsKnownObjectBorrow(module), hash_name, &password, &salt, iterations, dklen_obj);

exit:
    /* Cleanup for password */
    if (password.obj) {
       PyBuffer_Release(&password);
    }
    /* Cleanup for salt */
    if (salt.obj) {
       PyBuffer_Release(&salt);
    }

    return return_value;
}

PyDoc_STRVAR(_hashlib_scrypt__doc__,
"scrypt($module, /, password, *, salt, n, r, p, maxmem=0, dklen=64)\n"
"--\n"
"\n"
"scrypt password-based key derivation function.");

#define _HASHLIB_SCRYPT_METHODDEF    \
    {"scrypt", _PyCFunction_CAST(_hashlib_scrypt), METH_VALUE|METH_KEYWORDS, _hashlib_scrypt__doc__},

static PyValue
_hashlib_scrypt_impl(PyObject *module, Py_buffer *password, Py_buffer *salt,
                     unsigned long n, unsigned long r, unsigned long p,
                     long maxmem, long dklen);

static PyValue
_hashlib_scrypt(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 7
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(password)), _PyValue_STATIC_IMMORTAL(&_Py_ID(salt)), _PyValue_STATIC_IMMORTAL(_Py_LATIN1_CHR('n')), _PyValue_STATIC_IMMORTAL(_Py_LATIN1_CHR('r')), _PyValue_STATIC_IMMORTAL(_Py_LATIN1_CHR('p')), _PyValue_STATIC_IMMORTAL(&_Py_ID(maxmem)), _PyValue_STATIC_IMMORTAL(&_Py_ID(dklen)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"password", "salt", "n", "r", "p", "maxmem", "dklen", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "scrypt",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[7];
    Py_buffer password = {NULL, NULL};
    Py_buffer salt = {NULL, NULL};
    unsigned long n;
    unsigned long r;
    unsigned long p;
    long maxmem = 0;
    long dklen = 64;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 4, /*varpos*/ 0, _idx, 7)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[_idx[0]]))) {
        PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[_idx[0]]))->tp_name);
        goto exit;
    }
    if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])), &password, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[_idx[1]]))) {
        PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[_idx[1]]))->tp_name);
        goto exit;
    }
    if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])), &salt, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    {
        Py_ssize_t _bytes = _PyValue_AsNativeBytes(PyValue_Borrow(args[_idx[2]]), &n, sizeof(unsigned long),
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
    {
        Py_ssize_t _bytes = _PyValue_AsNativeBytes(PyValue_Borrow(args[_idx[3]]), &r, sizeof(unsigned long),
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
    {
        Py_ssize_t _bytes = _PyValue_AsNativeBytes(PyValue_Borrow(args[_idx[4]]), &p, sizeof(unsigned long),
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
    if (_idx[5] >= 0) {
        maxmem = _PyValue_AsLong(PyValue_Borrow(args[_idx[5]]));
        if (maxmem == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (_idx[6] >= 0) {
        dklen = _PyValue_AsLong(PyValue_Borrow(args[_idx[6]]));
        if (dklen == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    return_value = _hashlib_scrypt_impl(PyValue_AsKnownObjectBorrow(module), &password, &salt, n, r, p, maxmem, dklen);

exit:
    /* Cleanup for password */
    if (password.obj) {
       PyBuffer_Release(&password);
    }
    /* Cleanup for salt */
    if (salt.obj) {
       PyBuffer_Release(&salt);
    }

    return return_value;
}

PyDoc_STRVAR(_hashlib_hmac_singleshot__doc__,
"hmac_digest($module, /, key, msg, digest)\n"
"--\n"
"\n"
"Single-shot HMAC.");

#define _HASHLIB_HMAC_SINGLESHOT_METHODDEF    \
    {"hmac_digest", _PyCFunction_CAST(_hashlib_hmac_singleshot), METH_VALUE|METH_KEYWORDS, _hashlib_hmac_singleshot__doc__},

static PyValue
_hashlib_hmac_singleshot_impl(PyObject *module, Py_buffer *key,
                              Py_buffer *msg, PyValueBorrow digest);

static PyValue
_hashlib_hmac_singleshot(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .fname = "hmac_digest",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    Py_buffer key = {NULL, NULL};
    Py_buffer msg = {NULL, NULL};
    PyValueBorrow digest;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 3, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, _idx, 3)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[_idx[0]]))) {
        PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[_idx[0]]))->tp_name);
        goto exit;
    }
    if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])), &key, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[_idx[1]]))) {
        PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[_idx[1]]))->tp_name);
        goto exit;
    }
    if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])), &msg, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    digest = PyValue_Borrow(args[_idx[2]]);
    return_value = _hashlib_hmac_singleshot_impl(PyValue_AsKnownObjectBorrow(module), &key, &msg, digest);

exit:
    /* Cleanup for key */
    if (key.obj) {
       PyBuffer_Release(&key);
    }
    /* Cleanup for msg */
    if (msg.obj) {
       PyBuffer_Release(&msg);
    }

    return return_value;
}

PyDoc_STRVAR(_hashlib_hmac_new__doc__,
"hmac_new($module, /, key, msg=b\'\', digestmod=None)\n"
"--\n"
"\n"
"Return a new hmac object.");

#define _HASHLIB_HMAC_NEW_METHODDEF    \
    {"hmac_new", _PyCFunction_CAST(_hashlib_hmac_new), METH_VALUE|METH_KEYWORDS, _hashlib_hmac_new__doc__},

static PyValue
_hashlib_hmac_new_impl(PyObject *module, Py_buffer *key,
                       PyValueBorrow msg_obj, PyValueBorrow digestmod);

static PyValue
_hashlib_hmac_new(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .fname = "hmac_new",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    Py_buffer key = {NULL, NULL};
    PyValueBorrow msg_obj = PyValue_Borrow(PyValue_NULL);
    PyValueBorrow digestmod = PyValue_Borrow(PyValue_NULL);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, _idx, 3)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[_idx[0]]))) {
        PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[_idx[0]]))->tp_name);
        goto exit;
    }
    if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])), &key, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (_idx[1] >= 0) {
        msg_obj = PyValue_Borrow(args[_idx[1]]);
    }
    if (_idx[2] >= 0) {
        digestmod = PyValue_Borrow(args[_idx[2]]);
    }
    return_value = _hashlib_hmac_new_impl(PyValue_AsKnownObjectBorrow(module), &key, msg_obj, digestmod);

exit:
    /* Cleanup for key */
    if (key.obj) {
       PyBuffer_Release(&key);
    }

    return return_value;
}

PyDoc_STRVAR(_hashlib_HMAC_copy__doc__,
"copy($self, /)\n"
"--\n"
"\n"
"Return a copy (\"clone\") of the HMAC object.");

#define _HASHLIB_HMAC_COPY_METHODDEF    \
    {"copy", _PyCFunction_CAST(_hashlib_HMAC_copy), METH_VALUE|METH_NOARGS, _hashlib_HMAC_copy__doc__},

static PyValue
_hashlib_HMAC_copy_impl(HMACobject *self);

static PyValue
_hashlib_HMAC_copy(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _hashlib_HMAC_copy_impl((HMACobject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_hashlib_HMAC_update__doc__,
"update($self, /, msg)\n"
"--\n"
"\n"
"Update the HMAC object with msg.");

#define _HASHLIB_HMAC_UPDATE_METHODDEF    \
    {"update", _PyCFunction_CAST(_hashlib_HMAC_update), METH_VALUE|METH_KEYWORDS, _hashlib_HMAC_update__doc__},

static PyValue
_hashlib_HMAC_update_impl(HMACobject *self, PyValueBorrow msg);

static PyValue
_hashlib_HMAC_update(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
    PyValueBorrow msg;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    msg = PyValue_Borrow(args[_idx[0]]);
    return_value = _hashlib_HMAC_update_impl((HMACobject *)PyValue_AsKnownObjectBorrow(self), msg);

exit:
    return return_value;
}

PyDoc_STRVAR(_hashlib_HMAC_digest__doc__,
"digest($self, /)\n"
"--\n"
"\n"
"Return the digest of the bytes passed to the update() method so far.");

#define _HASHLIB_HMAC_DIGEST_METHODDEF    \
    {"digest", _PyCFunction_CAST(_hashlib_HMAC_digest), METH_VALUE|METH_NOARGS, _hashlib_HMAC_digest__doc__},

static PyValue
_hashlib_HMAC_digest_impl(HMACobject *self);

static PyValue
_hashlib_HMAC_digest(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _hashlib_HMAC_digest_impl((HMACobject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_hashlib_HMAC_hexdigest__doc__,
"hexdigest($self, /)\n"
"--\n"
"\n"
"Return hexadecimal digest of the bytes passed to the update() method so far.\n"
"\n"
"This may be used to exchange the value safely in email or other\n"
"non-binary environments.");

#define _HASHLIB_HMAC_HEXDIGEST_METHODDEF    \
    {"hexdigest", _PyCFunction_CAST(_hashlib_HMAC_hexdigest), METH_VALUE|METH_NOARGS, _hashlib_HMAC_hexdigest__doc__},

static PyValue
_hashlib_HMAC_hexdigest_impl(HMACobject *self);

static PyValue
_hashlib_HMAC_hexdigest(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _hashlib_HMAC_hexdigest_impl((HMACobject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_hashlib_get_fips_mode__doc__,
"get_fips_mode($module, /)\n"
"--\n"
"\n"
"Determine the OpenSSL FIPS mode of operation.\n"
"\n"
"For OpenSSL 3.0.0 and newer it returns the state of the default provider\n"
"in the default OSSL context. It\'s not quite the same as FIPS_mode() but\n"
"good enough for unittests.\n"
"\n"
"Effectively any non-zero return value indicates FIPS mode;\n"
"values other than 1 may have additional significance.");

#define _HASHLIB_GET_FIPS_MODE_METHODDEF    \
    {"get_fips_mode", _PyCFunction_CAST(_hashlib_get_fips_mode), METH_VALUE|METH_NOARGS, _hashlib_get_fips_mode__doc__},

static int
_hashlib_get_fips_mode_impl(PyObject *module);

static PyValue
_hashlib_get_fips_mode(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int _return_value;

    _return_value = _hashlib_get_fips_mode_impl(PyValue_AsKnownObjectBorrow(module));
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyValue_FromInt64((int64_t)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_hashlib_compare_digest__doc__,
"compare_digest($module, a, b, /)\n"
"--\n"
"\n"
"Return \'a == b\'.\n"
"\n"
"This function uses an approach designed to prevent\n"
"timing analysis, making it appropriate for cryptography.\n"
"\n"
"a and b must both be of the same type: either str (ASCII only),\n"
"or any bytes-like object.\n"
"\n"
"Note: If a and b are of different lengths, or if an error occurs,\n"
"a timing attack could theoretically reveal information about the\n"
"types and lengths of a and b--but not their values.");

#define _HASHLIB_COMPARE_DIGEST_METHODDEF    \
    {"compare_digest", _PyCFunction_CAST(_hashlib_compare_digest), METH_VALUE, _hashlib_compare_digest__doc__},

static PyValue
_hashlib_compare_digest_impl(PyObject *module, PyValueBorrow a,
                             PyValueBorrow b);

static PyValue
_hashlib_compare_digest(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow a;
    PyValueBorrow b;

    if (!_PyArg_CheckPositional("compare_digest", nargs, 2, 2)) {
        goto exit;
    }
    a = PyValue_Borrow(args[0]);
    b = PyValue_Borrow(args[1]);
    return_value = _hashlib_compare_digest_impl(PyValue_AsKnownObjectBorrow(module), a, b);

exit:
    return return_value;
}

#ifndef _HASHLIB_HASHXOF_DIGEST_METHODDEF
    #define _HASHLIB_HASHXOF_DIGEST_METHODDEF
#endif /* !defined(_HASHLIB_HASHXOF_DIGEST_METHODDEF) */

#ifndef _HASHLIB_HASHXOF_HEXDIGEST_METHODDEF
    #define _HASHLIB_HASHXOF_HEXDIGEST_METHODDEF
#endif /* !defined(_HASHLIB_HASHXOF_HEXDIGEST_METHODDEF) */

#ifndef _HASHLIB_OPENSSL_SHA3_224_METHODDEF
    #define _HASHLIB_OPENSSL_SHA3_224_METHODDEF
#endif /* !defined(_HASHLIB_OPENSSL_SHA3_224_METHODDEF) */

#ifndef _HASHLIB_OPENSSL_SHA3_256_METHODDEF
    #define _HASHLIB_OPENSSL_SHA3_256_METHODDEF
#endif /* !defined(_HASHLIB_OPENSSL_SHA3_256_METHODDEF) */

#ifndef _HASHLIB_OPENSSL_SHA3_384_METHODDEF
    #define _HASHLIB_OPENSSL_SHA3_384_METHODDEF
#endif /* !defined(_HASHLIB_OPENSSL_SHA3_384_METHODDEF) */

#ifndef _HASHLIB_OPENSSL_SHA3_512_METHODDEF
    #define _HASHLIB_OPENSSL_SHA3_512_METHODDEF
#endif /* !defined(_HASHLIB_OPENSSL_SHA3_512_METHODDEF) */

#ifndef _HASHLIB_OPENSSL_SHAKE_128_METHODDEF
    #define _HASHLIB_OPENSSL_SHAKE_128_METHODDEF
#endif /* !defined(_HASHLIB_OPENSSL_SHAKE_128_METHODDEF) */

#ifndef _HASHLIB_OPENSSL_SHAKE_256_METHODDEF
    #define _HASHLIB_OPENSSL_SHAKE_256_METHODDEF
#endif /* !defined(_HASHLIB_OPENSSL_SHAKE_256_METHODDEF) */
/*[clinic end generated code: output=6231110504e4f79a input=a9049054013a1b77]*/
