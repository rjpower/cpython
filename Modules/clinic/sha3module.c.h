/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()
#include "pycore_tuple.h"         // _PyTuple_ITEMS()

PyDoc_STRVAR(py_sha3_new__doc__,
"sha3_224(data=b\'\', *, usedforsecurity=True, string=None)\n"
"--\n"
"\n"
"Return a new SHA3 hash object.");

static PyObject *
py_sha3_new_impl(PyTypeObject *type, PyObject *data_obj, int usedforsecurity,
                 PyObject *string);

static PyObject *
py_sha3_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(data), &_Py_ID(usedforsecurity), &_Py_ID(string), },
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
    PyValue argsbuf[3];
    const PyValue *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 0;
    PyObject *data_obj = NULL;
    int usedforsecurity = 1;
    PyObject *string = NULL;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_ITEMS(args), nargs, kwargs, NULL, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (!PyValue_IsNull(fastargs[0])) {
        data_obj = PyValue_Object(fastargs[0]);
        if (data_obj == NULL) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
skip_optional_pos:
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (!PyValue_IsNull(fastargs[1])) {
        usedforsecurity = PyObject_IsTrueValue(fastargs[1]);
        if (usedforsecurity < 0) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    string = PyValue_Object(fastargs[2]);
    if (string == NULL) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = py_sha3_new_impl(type, data_obj, usedforsecurity, string);

exit:
    return return_value;
}

PyDoc_STRVAR(_sha3_sha3_224_copy__doc__,
"copy($self, /)\n"
"--\n"
"\n"
"Return a copy of the hash object.");

#define _SHA3_SHA3_224_COPY_METHODDEF    \
    {"copy", _PyCFunction_CAST(_sha3_sha3_224_copy), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _sha3_sha3_224_copy__doc__},

static PyObject *
_sha3_sha3_224_copy_impl(SHA3object *self, PyTypeObject *cls);

static PyValue
_sha3_sha3_224_copy(PyObject *self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "copy() takes no arguments");
        return PyValue_NULL;
    }
    return PyValue_FromResult(_sha3_sha3_224_copy_impl((SHA3object *)self, cls));
}

PyDoc_STRVAR(_sha3_sha3_224_digest__doc__,
"digest($self, /)\n"
"--\n"
"\n"
"Return the digest value as a bytes object.");

#define _SHA3_SHA3_224_DIGEST_METHODDEF    \
    {"digest", (PyCFunction)_sha3_sha3_224_digest, METH_NOARGS, _sha3_sha3_224_digest__doc__},

static PyObject *
_sha3_sha3_224_digest_impl(SHA3object *self);

static PyObject *
_sha3_sha3_224_digest(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _sha3_sha3_224_digest_impl((SHA3object *)self);
}

PyDoc_STRVAR(_sha3_sha3_224_hexdigest__doc__,
"hexdigest($self, /)\n"
"--\n"
"\n"
"Return the digest value as a string of hexadecimal digits.");

#define _SHA3_SHA3_224_HEXDIGEST_METHODDEF    \
    {"hexdigest", (PyCFunction)_sha3_sha3_224_hexdigest, METH_NOARGS, _sha3_sha3_224_hexdigest__doc__},

static PyObject *
_sha3_sha3_224_hexdigest_impl(SHA3object *self);

static PyObject *
_sha3_sha3_224_hexdigest(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _sha3_sha3_224_hexdigest_impl((SHA3object *)self);
}

PyDoc_STRVAR(_sha3_sha3_224_update__doc__,
"update($self, data, /)\n"
"--\n"
"\n"
"Update this hash object\'s state with the provided bytes-like object.");

#define _SHA3_SHA3_224_UPDATE_METHODDEF    \
    {"update", _PyCFunction_CAST(_sha3_sha3_224_update), METH_O, _sha3_sha3_224_update__doc__},

static PyObject *
_sha3_sha3_224_update_impl(SHA3object *self, PyObject *data);

static PyValue
_sha3_sha3_224_update(PyObject *self, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyObject *data;
    PyObject *_return_value;

    data = PyValue_Object(arg);
    if (data == NULL) {
        goto exit;
    }
    _return_value = _sha3_sha3_224_update_impl((SHA3object *)self, data);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_sha3_shake_128_digest__doc__,
"digest($self, /, length)\n"
"--\n"
"\n"
"Return the digest value as a bytes object.");

#define _SHA3_SHAKE_128_DIGEST_METHODDEF    \
    {"digest", _PyCFunction_CAST(_sha3_shake_128_digest), METH_FASTCALL|METH_KEYWORDS, _sha3_shake_128_digest__doc__},

static PyObject *
_sha3_shake_128_digest_impl(SHA3object *self, Py_ssize_t length);

static PyValue
_sha3_shake_128_digest(PyObject *self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(length), },
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
    PyValue argsbuf[1];
    Py_ssize_t length;
    PyObject *_return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    {
        Py_ssize_t ival = -1;
        PyValue ivalue = PyNumber_IndexValue(args[0]);
        if (!PyValue_IsNull(ivalue)) {
            ival = PyLong_AsSsize_tValue(ivalue);
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
    _return_value = _sha3_shake_128_digest_impl((SHA3object *)self, length);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_sha3_shake_128_hexdigest__doc__,
"hexdigest($self, /, length)\n"
"--\n"
"\n"
"Return the digest value as a string of hexadecimal digits.");

#define _SHA3_SHAKE_128_HEXDIGEST_METHODDEF    \
    {"hexdigest", _PyCFunction_CAST(_sha3_shake_128_hexdigest), METH_FASTCALL|METH_KEYWORDS, _sha3_shake_128_hexdigest__doc__},

static PyObject *
_sha3_shake_128_hexdigest_impl(SHA3object *self, Py_ssize_t length);

static PyValue
_sha3_shake_128_hexdigest(PyObject *self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(length), },
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
    PyValue argsbuf[1];
    Py_ssize_t length;
    PyObject *_return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    {
        Py_ssize_t ival = -1;
        PyValue ivalue = PyNumber_IndexValue(args[0]);
        if (!PyValue_IsNull(ivalue)) {
            ival = PyLong_AsSsize_tValue(ivalue);
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
    _return_value = _sha3_shake_128_hexdigest_impl((SHA3object *)self, length);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}
/*[clinic end generated code: output=20788aae08b3ca60 input=a9049054013a1b77]*/
