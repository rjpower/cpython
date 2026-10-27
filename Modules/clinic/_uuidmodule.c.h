/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_critical_section.h"// Py_BEGIN_CRITICAL_SECTION()
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()
#include "pycore_tuple.h"         // _PyTuple_ITEMS()

PyDoc_STRVAR(_uuid_uuid4__doc__,
"uuid4($module, /)\n"
"--\n"
"\n"
"Generate a random UUID (version 4).");

#define _UUID_UUID4_METHODDEF    \
    {"uuid4", (PyCFunction)_uuid_uuid4, METH_NOARGS, _uuid_uuid4__doc__},

static PyObject *
_uuid_uuid4_impl(PyObject *module);

static PyObject *
_uuid_uuid4(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(module);
    return_value = _uuid_uuid4_impl(module);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_uuid_uuid7__doc__,
"uuid7($module, /)\n"
"--\n"
"\n"
"Generate a UUID from a Unix timestamp in milliseconds and random bits.\n"
"\n"
"UUIDv7 objects feature monotonicity within a millisecond.");

#define _UUID_UUID7_METHODDEF    \
    {"uuid7", (PyCFunction)_uuid_uuid7, METH_NOARGS, _uuid_uuid7__doc__},

static PyObject *
_uuid_uuid7_impl(PyObject *module);

static PyObject *
_uuid_uuid7(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(module);
    return_value = _uuid_uuid7_impl(module);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_uuid__install_c_hooks__doc__,
"_install_c_hooks($module, /, *, random_func, time_func)\n"
"--\n"
"\n");

#define _UUID__INSTALL_C_HOOKS_METHODDEF    \
    {"_install_c_hooks", _PyCFunction_CAST(_uuid__install_c_hooks), METH_FASTCALL|METH_KEYWORDS, _uuid__install_c_hooks__doc__},

static PyObject *
_uuid__install_c_hooks_impl(PyObject *module, PyObject *random_func,
                            PyObject *time_func);

static PyValue
_uuid__install_c_hooks(PyObject *module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(random_func), &_Py_ID(time_func), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"random_func", "time_func", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "_install_c_hooks",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyValue argsbuf[2];
    PyObject *random_func;
    PyObject *time_func;
    PyObject *_return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 0, /*minkw*/ 2, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    random_func = PyValue_Object(args[0]);
    if (random_func == NULL) {
        goto exit;
    }
    time_func = PyValue_Object(args[1]);
    if (time_func == NULL) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(module);
    _return_value = _uuid__install_c_hooks_impl(module, random_func, time_func);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_uuid_UUID___init____doc__,
"UUID(hex=None, bytes=None, bytes_le=None, fields=None, int=None,\n"
"     version=None, *, is_safe=<unrepresentable>)\n"
"--\n"
"\n"
"UUID is a fast base implementation type for uuid.UUID.");

static int
_uuid_UUID___init___impl(uuidobject *self, PyObject *hex, PyObject *bytes,
                         PyObject *bytes_le, PyObject *fields,
                         PyObject *int_value, PyObject *version,
                         PyObject *is_safe);

static int
_uuid_UUID___init__(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int return_value = -1;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 7
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(hex), &_Py_ID(bytes), &_Py_ID(bytes_le), &_Py_ID(fields), &_Py_ID(int), &_Py_ID(version), &_Py_ID(is_safe), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"hex", "bytes", "bytes_le", "fields", "int", "version", "is_safe", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "UUID",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyValue argsbuf[7];
    const PyValue *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 0;
    PyObject *hex = Py_None;
    PyObject *bytes = Py_None;
    PyObject *bytes_le = Py_None;
    PyObject *fields = Py_None;
    PyObject *int_value = Py_None;
    PyObject *version = Py_None;
    PyObject *is_safe = NULL;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_ITEMS(args), nargs, kwargs, NULL, &_parser,
            /*minpos*/ 0, /*maxpos*/ 6, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (!PyValue_IsNull(fastargs[0])) {
        hex = PyValue_Object(fastargs[0]);
        if (hex == NULL) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (!PyValue_IsNull(fastargs[1])) {
        bytes = PyValue_Object(fastargs[1]);
        if (bytes == NULL) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (!PyValue_IsNull(fastargs[2])) {
        bytes_le = PyValue_Object(fastargs[2]);
        if (bytes_le == NULL) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (!PyValue_IsNull(fastargs[3])) {
        fields = PyValue_Object(fastargs[3]);
        if (fields == NULL) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (!PyValue_IsNull(fastargs[4])) {
        int_value = PyValue_Object(fastargs[4]);
        if (int_value == NULL) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (!PyValue_IsNull(fastargs[5])) {
        version = PyValue_Object(fastargs[5]);
        if (version == NULL) {
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
    is_safe = PyValue_Object(fastargs[6]);
    if (is_safe == NULL) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = _uuid_UUID___init___impl((uuidobject *)self, hex, bytes, bytes_le, fields, int_value, version, is_safe);

exit:
    return return_value;
}

PyDoc_STRVAR(_uuid_UUID__from_int__doc__,
"_from_int($type, value, /)\n"
"--\n"
"\n"
"Create a UUID from an integer value. Internal use only.");

#define _UUID_UUID__FROM_INT_METHODDEF    \
    {"_from_int", _PyCFunction_CAST(_uuid_UUID__from_int), METH_O|METH_CLASS, _uuid_UUID__from_int__doc__},

static PyObject *
_uuid_UUID__from_int_impl(PyTypeObject *type, PyObject *value);

static PyValue
_uuid_UUID__from_int(PyObject *type, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyObject *value;
    PyObject *_return_value;

    value = PyValue_Object(arg);
    if (value == NULL) {
        goto exit;
    }
    _return_value = _uuid_UUID__from_int_impl((PyTypeObject *)type, value);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_uuid_UUID___getstate____doc__,
"__getstate__($self, /)\n"
"--\n"
"\n"
"Return the UUID\'s state for pickling.");

#define _UUID_UUID___GETSTATE___METHODDEF    \
    {"__getstate__", (PyCFunction)_uuid_UUID___getstate__, METH_NOARGS, _uuid_UUID___getstate____doc__},

static PyObject *
_uuid_UUID___getstate___impl(uuidobject *self);

static PyObject *
_uuid_UUID___getstate__(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _uuid_UUID___getstate___impl((uuidobject *)self);
}

PyDoc_STRVAR(_uuid_UUID___setstate____doc__,
"__setstate__($self, state, /)\n"
"--\n"
"\n"
"Restore the UUID\'s state from pickling.\n"
"\n"
"Expects a mapping with \'int\' and optionally \'is_safe\' keys.");

#define _UUID_UUID___SETSTATE___METHODDEF    \
    {"__setstate__", _PyCFunction_CAST(_uuid_UUID___setstate__), METH_O, _uuid_UUID___setstate____doc__},

static PyObject *
_uuid_UUID___setstate___impl(uuidobject *self, PyObject *state);

static PyValue
_uuid_UUID___setstate__(PyObject *self, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyObject *state;
    PyObject *_return_value;

    state = PyValue_Object(arg);
    if (state == NULL) {
        goto exit;
    }
    _return_value = _uuid_UUID___setstate___impl((uuidobject *)self, state);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}
/*[clinic end generated code: output=5499719cfaec602f input=a9049054013a1b77]*/
