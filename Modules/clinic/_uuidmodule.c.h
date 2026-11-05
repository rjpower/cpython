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
#include "pycore_critical_section.h"// Py_BEGIN_CRITICAL_SECTION()
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywordsValue()
#include "pycore_tuple.h"         // _PyTuple_ITEMS()
#include "pycore_value_call.h"    // METH_VALUE

PyDoc_STRVAR(_uuid_uuid4__doc__,
"uuid4($module, /)\n"
"--\n"
"\n"
"Generate a random UUID (version 4).");

#define _UUID_UUID4_METHODDEF    \
    {"uuid4", _PyCFunction_CAST(_uuid_uuid4), METH_VALUE|METH_NOARGS, _uuid_uuid4__doc__},

static PyValue
_uuid_uuid4_impl(PyObject *module);

static PyValue
_uuid_uuid4(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(module));
    return_value = _uuid_uuid4_impl(PyValue_AsKnownObjectBorrow(module));
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
    {"uuid7", _PyCFunction_CAST(_uuid_uuid7), METH_VALUE|METH_NOARGS, _uuid_uuid7__doc__},

static PyValue
_uuid_uuid7_impl(PyObject *module);

static PyValue
_uuid_uuid7(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(module));
    return_value = _uuid_uuid7_impl(PyValue_AsKnownObjectBorrow(module));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_uuid__install_c_hooks__doc__,
"_install_c_hooks($module, /, *, random_func, time_func)\n"
"--\n"
"\n");

#define _UUID__INSTALL_C_HOOKS_METHODDEF    \
    {"_install_c_hooks", _PyCFunction_CAST(_uuid__install_c_hooks), METH_VALUE|METH_KEYWORDS, _uuid__install_c_hooks__doc__},

static PyValue
_uuid__install_c_hooks_impl(PyObject *module, PyValueBorrow random_func,
                            PyValueBorrow time_func);

static PyValue
_uuid__install_c_hooks(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(random_func)), _PyValue_STATIC_IMMORTAL(&_Py_ID(time_func)), },
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
    Py_ssize_t _idx[2];
    PyValueBorrow random_func;
    PyValueBorrow time_func;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 0, /*minkw*/ 2, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    random_func = PyValue_Borrow(args[_idx[0]]);
    time_func = PyValue_Borrow(args[_idx[1]]);
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(module));
    return_value = _uuid__install_c_hooks_impl(PyValue_AsKnownObjectBorrow(module), random_func, time_func);
    Py_END_CRITICAL_SECTION();

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
_uuid_UUID___init___impl(uuidobject *self, PyValueBorrow hex,
                         PyValueBorrow bytes, PyValueBorrow bytes_le,
                         PyValueBorrow fields, PyValueBorrow int_value,
                         PyValueBorrow version, PyValueBorrow is_safe);

static int
_uuid_UUID___init___value(PyObject *self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    int return_value = -1;
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(hex)), _PyValue_STATIC_IMMORTAL(&_Py_ID(bytes)), _PyValue_STATIC_IMMORTAL(&_Py_ID(bytes_le)), _PyValue_STATIC_IMMORTAL(&_Py_ID(fields)), _PyValue_STATIC_IMMORTAL(&_Py_ID(int)), _PyValue_STATIC_IMMORTAL(&_Py_ID(version)), _PyValue_STATIC_IMMORTAL(&_Py_ID(is_safe)), },
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
    Py_ssize_t _idx[7];
    PyValueBorrow hex = PyValue_Borrow(PyValue_None);
    PyValueBorrow bytes = PyValue_Borrow(PyValue_None);
    PyValueBorrow bytes_le = PyValue_Borrow(PyValue_None);
    PyValueBorrow fields = PyValue_Borrow(PyValue_None);
    PyValueBorrow int_value = PyValue_Borrow(PyValue_None);
    PyValueBorrow version = PyValue_Borrow(PyValue_None);
    PyValueBorrow is_safe = PyValue_Borrow(PyValue_NULL);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 6, /*minkw*/ 0, /*varpos*/ 0, _idx, 7)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        hex = PyValue_Borrow(args[_idx[0]]);
    }
    if (_idx[1] >= 0) {
        bytes = PyValue_Borrow(args[_idx[1]]);
    }
    if (_idx[2] >= 0) {
        bytes_le = PyValue_Borrow(args[_idx[2]]);
    }
    if (_idx[3] >= 0) {
        fields = PyValue_Borrow(args[_idx[3]]);
    }
    if (_idx[4] >= 0) {
        int_value = PyValue_Borrow(args[_idx[4]]);
    }
    if (_idx[5] >= 0) {
        version = PyValue_Borrow(args[_idx[5]]);
    }
    if (_idx[6] >= 0) {
        is_safe = PyValue_Borrow(args[_idx[6]]);
    }
    return_value = _uuid_UUID___init___impl((uuidobject *)self, hex, bytes, bytes_le, fields, int_value, version, is_safe);

exit:
    return return_value;
}

static int
_uuid_UUID___init__(PyObject *self, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _uuid_UUID___init___value(self, _PyTuple_ITEMS(args), nargs, NULL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return -1;
    }
    int result = _uuid_UUID___init___value(self, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return result;
}

PyDoc_STRVAR(_uuid_UUID__from_int__doc__,
"_from_int($type, value, /)\n"
"--\n"
"\n"
"Create a UUID from an integer value. Internal use only.");

#define _UUID_UUID__FROM_INT_METHODDEF    \
    {"_from_int", _PyCFunction_CAST(_uuid_UUID__from_int), METH_VALUE|METH_O|METH_CLASS, _uuid_UUID__from_int__doc__},

static PyValue
_uuid_UUID__from_int_impl(PyTypeObject *type, PyValueBorrow value);

static PyValue
_uuid_UUID__from_int(PyValueBorrow type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow value;

    value = PyValue_Borrow(args[0]);
    return_value = _uuid_UUID__from_int_impl((PyTypeObject *)PyValue_AsKnownObjectBorrow(type), value);

    return return_value;
}

PyDoc_STRVAR(_uuid_UUID___getstate____doc__,
"__getstate__($self, /)\n"
"--\n"
"\n"
"Return the UUID\'s state for pickling.");

#define _UUID_UUID___GETSTATE___METHODDEF    \
    {"__getstate__", _PyCFunction_CAST(_uuid_UUID___getstate__), METH_VALUE|METH_NOARGS, _uuid_UUID___getstate____doc__},

static PyValue
_uuid_UUID___getstate___impl(uuidobject *self);

static PyValue
_uuid_UUID___getstate__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _uuid_UUID___getstate___impl((uuidobject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_uuid_UUID___setstate____doc__,
"__setstate__($self, state, /)\n"
"--\n"
"\n"
"Restore the UUID\'s state from pickling.\n"
"\n"
"Expects a mapping with \'int\' and optionally \'is_safe\' keys.");

#define _UUID_UUID___SETSTATE___METHODDEF    \
    {"__setstate__", _PyCFunction_CAST(_uuid_UUID___setstate__), METH_VALUE|METH_O, _uuid_UUID___setstate____doc__},

static PyValue
_uuid_UUID___setstate___impl(uuidobject *self, PyValueBorrow state);

static PyValue
_uuid_UUID___setstate__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow state;

    state = PyValue_Borrow(args[0]);
    return_value = _uuid_UUID___setstate___impl((uuidobject *)PyValue_AsKnownObjectBorrow(self), state);

    return return_value;
}
/*[clinic end generated code: output=94e10d272add3683 input=a9049054013a1b77]*/
