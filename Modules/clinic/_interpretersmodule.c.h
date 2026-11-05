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
#include "pycore_modsupport.h"    // _PyArg_CheckPositional()
#include "pycore_unicodeobject.h" // _PyUnicode_AsUTF8NoNUL()
#include "pycore_value_call.h"    // METH_VALUE
#include "pycore_value_ops.h"     // _PyValue_IsTrue()

PyDoc_STRVAR(_interpreters_new_config__doc__,
"new_config($module, name=\'isolated\', /, **overrides)\n"
"--\n"
"\n"
"Return a representation of a new PyInterpreterConfig.\n"
"\n"
"The name determines the initial values of the config.  Supported named\n"
"configs are: default, isolated, legacy, and empty.\n"
"\n"
"Any keyword arguments are set on the corresponding config fields,\n"
"overriding the initial values.");

#define _INTERPRETERS_NEW_CONFIG_METHODDEF    \
    {"new_config", _PyCFunction_CAST(_interpreters_new_config), METH_VALUE|METH_KEYWORDS, _interpreters_new_config__doc__},

static PyValue
_interpreters_new_config_impl(PyObject *module, const char *name,
                              PyObject *overrides);

static PyValue
_interpreters_new_config(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    const char *name = NULL;
    PyObject *overrides = NULL;

    if (!_PyArg_CheckPositional("new_config", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("new_config", "argument 1", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("new_config", "argument 1", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    name = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
    if (name == NULL) {
        goto exit;
    }
skip_optional:
    overrides = kwnames == NULL ? PyDict_New()
        : _PyValue_StackAsDict(args + nargs, kwnames);
    if (overrides == NULL) {
        goto exit;
    }
    return_value = _interpreters_new_config_impl(PyValue_AsKnownObjectBorrow(module), name, overrides);

exit:
    /* Cleanup for overrides */
    Py_XDECREF(overrides);

    return return_value;
}

PyDoc_STRVAR(_interpreters_create__doc__,
"create($module, /, config=\'isolated\', *, reqrefs=False)\n"
"--\n"
"\n"
"Create a new interpreter and return a unique generated ID.\n"
"\n"
"The caller is responsible for destroying the interpreter before exiting,\n"
"typically by using _interpreters.destroy().  This can be managed\n"
"automatically by passing \"reqrefs=True\" and then using _incref() and\n"
"_decref() appropriately.\n"
"\n"
"\"config\" must be a valid interpreter config or the name of a\n"
"predefined config (\'isolated\' or \'legacy\').  The default\n"
"is \'isolated\'.");

#define _INTERPRETERS_CREATE_METHODDEF    \
    {"create", _PyCFunction_CAST(_interpreters_create), METH_VALUE|METH_KEYWORDS, _interpreters_create__doc__},

static PyValue
_interpreters_create_impl(PyObject *module, PyValueBorrow configobj,
                          int reqrefs);

static PyValue
_interpreters_create(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(config)), _PyValue_STATIC_IMMORTAL(&_Py_ID(reqrefs)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"config", "reqrefs", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "create",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    PyValueBorrow configobj = PyValue_Borrow(PyValue_NULL);
    int reqrefs = 0;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        configobj = PyValue_Borrow(args[_idx[0]]);
    }
    if (_idx[1] >= 0) {
        reqrefs = _PyValue_IsTrue(PyValue_Borrow(args[_idx[1]]));
        if (reqrefs < 0) {
            goto exit;
        }
    }
    return_value = _interpreters_create_impl(PyValue_AsKnownObjectBorrow(module), configobj, reqrefs);

exit:
    return return_value;
}

PyDoc_STRVAR(_interpreters_destroy__doc__,
"destroy($module, /, id, *, restrict=False)\n"
"--\n"
"\n"
"Destroy the identified interpreter.\n"
"\n"
"Attempting to destroy the current interpreter raises InterpreterError.\n"
"So does an unrecognized ID.");

#define _INTERPRETERS_DESTROY_METHODDEF    \
    {"destroy", _PyCFunction_CAST(_interpreters_destroy), METH_VALUE|METH_KEYWORDS, _interpreters_destroy__doc__},

static PyValue
_interpreters_destroy_impl(PyObject *module, PyValueBorrow id,
                           int restricted);

static PyValue
_interpreters_destroy(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(id)), _PyValue_STATIC_IMMORTAL(&_Py_ID(restrict)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"id", "restrict", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "destroy",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    PyValueBorrow id;
    int restricted = 0;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    id = PyValue_Borrow(args[_idx[0]]);
    if (_idx[1] >= 0) {
        restricted = _PyValue_IsTrue(PyValue_Borrow(args[_idx[1]]));
        if (restricted < 0) {
            goto exit;
        }
    }
    return_value = _interpreters_destroy_impl(PyValue_AsKnownObjectBorrow(module), id, restricted);

exit:
    return return_value;
}

PyDoc_STRVAR(_interpreters_list_all__doc__,
"list_all($module, /, *, require_ready=False)\n"
"--\n"
"\n"
"Return a list containing the ID of every existing interpreter.");

#define _INTERPRETERS_LIST_ALL_METHODDEF    \
    {"list_all", _PyCFunction_CAST(_interpreters_list_all), METH_VALUE|METH_KEYWORDS, _interpreters_list_all__doc__},

static PyValue
_interpreters_list_all_impl(PyObject *module, int reqready);

static PyValue
_interpreters_list_all(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(require_ready)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"require_ready", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "list_all",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    int reqready = 0;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 0, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        reqready = _PyValue_IsTrue(PyValue_Borrow(args[_idx[0]]));
        if (reqready < 0) {
            goto exit;
        }
    }
    return_value = _interpreters_list_all_impl(PyValue_AsKnownObjectBorrow(module), reqready);

exit:
    return return_value;
}

PyDoc_STRVAR(_interpreters_get_current__doc__,
"get_current($module, /)\n"
"--\n"
"\n"
"Return (ID, whence) of the current interpreter.");

#define _INTERPRETERS_GET_CURRENT_METHODDEF    \
    {"get_current", _PyCFunction_CAST(_interpreters_get_current), METH_VALUE|METH_NOARGS, _interpreters_get_current__doc__},

static PyValue
_interpreters_get_current_impl(PyObject *module);

static PyValue
_interpreters_get_current(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _interpreters_get_current_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(_interpreters_get_main__doc__,
"get_main($module, /)\n"
"--\n"
"\n"
"Return (ID, whence) of the main interpreter.");

#define _INTERPRETERS_GET_MAIN_METHODDEF    \
    {"get_main", _PyCFunction_CAST(_interpreters_get_main), METH_VALUE|METH_NOARGS, _interpreters_get_main__doc__},

static PyValue
_interpreters_get_main_impl(PyObject *module);

static PyValue
_interpreters_get_main(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _interpreters_get_main_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(_interpreters_set___main___attrs__doc__,
"set___main___attrs($module, /, id, updates, *, restrict=False)\n"
"--\n"
"\n"
"Bind the given attributes in the interpreter\'s __main__ module.");

#define _INTERPRETERS_SET___MAIN___ATTRS_METHODDEF    \
    {"set___main___attrs", _PyCFunction_CAST(_interpreters_set___main___attrs), METH_VALUE|METH_KEYWORDS, _interpreters_set___main___attrs__doc__},

static PyValue
_interpreters_set___main___attrs_impl(PyObject *module, PyValueBorrow id,
                                      PyObject *updates, int restricted);

static PyValue
_interpreters_set___main___attrs(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(id)), _PyValue_STATIC_IMMORTAL(&_Py_ID(updates)), _PyValue_STATIC_IMMORTAL(&_Py_ID(restrict)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"id", "updates", "restrict", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "set___main___attrs",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    PyValueBorrow id;
    PyObject *updates;
    int restricted = 0;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 3)) {
        goto exit;
    }
    id = PyValue_Borrow(args[_idx[0]]);
    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[_idx[1]])), &PyDict_Type)) {
        _PyArg_BadArgumentValue("set___main___attrs", "argument 'updates'", "dict", PyValue_Borrow(args[_idx[1]]));
        goto exit;
    }
    updates = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]]));
    if (_idx[2] >= 0) {
        restricted = _PyValue_IsTrue(PyValue_Borrow(args[_idx[2]]));
        if (restricted < 0) {
            goto exit;
        }
    }
    return_value = _interpreters_set___main___attrs_impl(PyValue_AsKnownObjectBorrow(module), id, updates, restricted);

exit:
    return return_value;
}

PyDoc_STRVAR(_interpreters_exec__doc__,
"exec($module, /, id, code, shared={}, *, restrict=False)\n"
"--\n"
"\n"
"Execute the provided code in the identified interpreter.\n"
"\n"
"This is equivalent to running the builtin exec() under the target\n"
"interpreter, using the __dict__ of its __main__ module as both\n"
"globals and locals.\n"
"\n"
"\"code\" may be a string containing the text of a Python script.\n"
"\n"
"Functions (and code objects) are also supported, with some restrictions.\n"
"The code/function must not take any arguments or be a closure\n"
"(i.e. have cell vars).  Methods and other callables are not supported.\n"
"\n"
"If a function is provided, its code object is used and all its state\n"
"is ignored, including its __globals__ dict.");

#define _INTERPRETERS_EXEC_METHODDEF    \
    {"exec", _PyCFunction_CAST(_interpreters_exec), METH_VALUE|METH_KEYWORDS, _interpreters_exec__doc__},

static PyValue
_interpreters_exec_impl(PyObject *module, PyValueBorrow id,
                        PyValueBorrow code, PyObject *shared, int restricted);

static PyValue
_interpreters_exec(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(id)), _PyValue_STATIC_IMMORTAL(&_Py_ID(code)), _PyValue_STATIC_IMMORTAL(&_Py_ID(shared)), _PyValue_STATIC_IMMORTAL(&_Py_ID(restrict)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"id", "code", "shared", "restrict", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "exec",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[4];
    PyValueBorrow id;
    PyValueBorrow code;
    PyObject *shared = NULL;
    int restricted = 0;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, _idx, 4)) {
        goto exit;
    }
    id = PyValue_Borrow(args[_idx[0]]);
    code = PyValue_Borrow(args[_idx[1]]);
    if (_idx[2] >= 0) {
        if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[_idx[2]])), &PyDict_Type)) {
            _PyArg_BadArgumentValue("exec", "argument 'shared'", "dict", PyValue_Borrow(args[_idx[2]]));
            goto exit;
        }
        shared = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[2]]));
    }
    if (_idx[3] >= 0) {
        restricted = _PyValue_IsTrue(PyValue_Borrow(args[_idx[3]]));
        if (restricted < 0) {
            goto exit;
        }
    }
    return_value = _interpreters_exec_impl(PyValue_AsKnownObjectBorrow(module), id, code, shared, restricted);

exit:
    return return_value;
}

PyDoc_STRVAR(_interpreters_run_string__doc__,
"run_string($module, /, id, script, shared={}, *, restrict=False)\n"
"--\n"
"\n"
"Execute the provided string in the identified interpreter.\n"
"\n"
"(See _interpreters.exec().)");

#define _INTERPRETERS_RUN_STRING_METHODDEF    \
    {"run_string", _PyCFunction_CAST(_interpreters_run_string), METH_VALUE|METH_KEYWORDS, _interpreters_run_string__doc__},

static PyValue
_interpreters_run_string_impl(PyObject *module, PyValueBorrow id,
                              PyObject *script, PyObject *shared,
                              int restricted);

static PyValue
_interpreters_run_string(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(id)), _PyValue_STATIC_IMMORTAL(&_Py_ID(script)), _PyValue_STATIC_IMMORTAL(&_Py_ID(shared)), _PyValue_STATIC_IMMORTAL(&_Py_ID(restrict)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"id", "script", "shared", "restrict", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "run_string",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[4];
    PyValueBorrow id;
    PyObject *script;
    PyObject *shared = NULL;
    int restricted = 0;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, _idx, 4)) {
        goto exit;
    }
    id = PyValue_Borrow(args[_idx[0]]);
    if (!PyValue_IsObject(PyValue_Borrow(args[_idx[1]]))) {
        _PyArg_BadArgumentValue("run_string", "argument 'script'", "str", PyValue_Borrow(args[_idx[1]]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])))) {
        _PyArg_BadArgument("run_string", "argument 'script'", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]])));
        goto exit;
    }
    script = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]]));
    if (_idx[2] >= 0) {
        if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[_idx[2]])), &PyDict_Type)) {
            _PyArg_BadArgumentValue("run_string", "argument 'shared'", "dict", PyValue_Borrow(args[_idx[2]]));
            goto exit;
        }
        shared = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[2]]));
    }
    if (_idx[3] >= 0) {
        restricted = _PyValue_IsTrue(PyValue_Borrow(args[_idx[3]]));
        if (restricted < 0) {
            goto exit;
        }
    }
    return_value = _interpreters_run_string_impl(PyValue_AsKnownObjectBorrow(module), id, script, shared, restricted);

exit:
    return return_value;
}

PyDoc_STRVAR(_interpreters_run_func__doc__,
"run_func($module, /, id, func, shared={}, *, restrict=False)\n"
"--\n"
"\n"
"Execute the body of the provided function in the identified interpreter.\n"
"\n"
"Code objects are also supported.  In both cases, closures and args\n"
"are not supported.  Methods and other callables are not supported\n"
"either.\n"
"\n"
"(See _interpreters.exec().)");

#define _INTERPRETERS_RUN_FUNC_METHODDEF    \
    {"run_func", _PyCFunction_CAST(_interpreters_run_func), METH_VALUE|METH_KEYWORDS, _interpreters_run_func__doc__},

static PyValue
_interpreters_run_func_impl(PyObject *module, PyValueBorrow id,
                            PyValueBorrow func, PyObject *shared,
                            int restricted);

static PyValue
_interpreters_run_func(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(id)), _PyValue_STATIC_IMMORTAL(&_Py_ID(func)), _PyValue_STATIC_IMMORTAL(&_Py_ID(shared)), _PyValue_STATIC_IMMORTAL(&_Py_ID(restrict)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"id", "func", "shared", "restrict", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "run_func",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[4];
    PyValueBorrow id;
    PyValueBorrow func;
    PyObject *shared = NULL;
    int restricted = 0;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, _idx, 4)) {
        goto exit;
    }
    id = PyValue_Borrow(args[_idx[0]]);
    func = PyValue_Borrow(args[_idx[1]]);
    if (_idx[2] >= 0) {
        if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[_idx[2]])), &PyDict_Type)) {
            _PyArg_BadArgumentValue("run_func", "argument 'shared'", "dict", PyValue_Borrow(args[_idx[2]]));
            goto exit;
        }
        shared = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[2]]));
    }
    if (_idx[3] >= 0) {
        restricted = _PyValue_IsTrue(PyValue_Borrow(args[_idx[3]]));
        if (restricted < 0) {
            goto exit;
        }
    }
    return_value = _interpreters_run_func_impl(PyValue_AsKnownObjectBorrow(module), id, func, shared, restricted);

exit:
    return return_value;
}

PyDoc_STRVAR(_interpreters_call__doc__,
"call($module, /, id, callable, args=(), kwargs={}, *,\n"
"     preserve_exc=False, restrict=False)\n"
"--\n"
"\n"
"Call the provided object in the identified interpreter.\n"
"\n"
"Pass the given args and kwargs, if possible.");

#define _INTERPRETERS_CALL_METHODDEF    \
    {"call", _PyCFunction_CAST(_interpreters_call), METH_VALUE|METH_KEYWORDS, _interpreters_call__doc__},

static PyValue
_interpreters_call_impl(PyObject *module, PyValueBorrow id,
                        PyValueBorrow callable, PyObject *args,
                        PyObject *kwargs, int preserve_exc, int restricted);

static PyValue
_interpreters_call(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 6
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(id)), _PyValue_STATIC_IMMORTAL(&_Py_ID(callable)), _PyValue_STATIC_IMMORTAL(&_Py_ID(args)), _PyValue_STATIC_IMMORTAL(&_Py_ID(kwargs)), _PyValue_STATIC_IMMORTAL(&_Py_ID(preserve_exc)), _PyValue_STATIC_IMMORTAL(&_Py_ID(restrict)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"id", "callable", "args", "kwargs", "preserve_exc", "restrict", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "call",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[6];
    PyValueBorrow id;
    PyValueBorrow callable;
    PyObject *__clinic_args = NULL;
    PyObject *__clinic_kwargs = NULL;
    int preserve_exc = 0;
    int restricted = 0;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 4, /*minkw*/ 0, /*varpos*/ 0, _idx, 6)) {
        goto exit;
    }
    id = PyValue_Borrow(args[_idx[0]]);
    callable = PyValue_Borrow(args[_idx[1]]);
    if (_idx[2] >= 0) {
        if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[_idx[2]])), &PyTuple_Type)) {
            _PyArg_BadArgumentValue("call", "argument 'args'", "tuple", PyValue_Borrow(args[_idx[2]]));
            goto exit;
        }
        __clinic_args = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[2]]));
    }
    if (_idx[3] >= 0) {
        if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[_idx[3]])), &PyDict_Type)) {
            _PyArg_BadArgumentValue("call", "argument 'kwargs'", "dict", PyValue_Borrow(args[_idx[3]]));
            goto exit;
        }
        __clinic_kwargs = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[3]]));
    }
    if (_idx[4] >= 0) {
        preserve_exc = _PyValue_IsTrue(PyValue_Borrow(args[_idx[4]]));
        if (preserve_exc < 0) {
            goto exit;
        }
    }
    if (_idx[5] >= 0) {
        restricted = _PyValue_IsTrue(PyValue_Borrow(args[_idx[5]]));
        if (restricted < 0) {
            goto exit;
        }
    }
    return_value = _interpreters_call_impl(PyValue_AsKnownObjectBorrow(module), id, callable, __clinic_args, __clinic_kwargs, preserve_exc, restricted);

exit:
    return return_value;
}

PyDoc_STRVAR(_interpreters_is_shareable__doc__,
"is_shareable($module, /, obj)\n"
"--\n"
"\n"
"Return True if the object\'s data may be shared between interpreters and False otherwise.");

#define _INTERPRETERS_IS_SHAREABLE_METHODDEF    \
    {"is_shareable", _PyCFunction_CAST(_interpreters_is_shareable), METH_VALUE|METH_KEYWORDS, _interpreters_is_shareable__doc__},

static PyValue
_interpreters_is_shareable_impl(PyObject *module, PyValueBorrow obj);

static PyValue
_interpreters_is_shareable(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(obj)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"obj", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "is_shareable",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    PyValueBorrow obj;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    obj = PyValue_Borrow(args[_idx[0]]);
    return_value = _interpreters_is_shareable_impl(PyValue_AsKnownObjectBorrow(module), obj);

exit:
    return return_value;
}

PyDoc_STRVAR(_interpreters_is_running__doc__,
"is_running($module, /, id, *, restrict=False)\n"
"--\n"
"\n"
"Return whether or not the identified interpreter is running.");

#define _INTERPRETERS_IS_RUNNING_METHODDEF    \
    {"is_running", _PyCFunction_CAST(_interpreters_is_running), METH_VALUE|METH_KEYWORDS, _interpreters_is_running__doc__},

static PyValue
_interpreters_is_running_impl(PyObject *module, PyValueBorrow id,
                              int restricted);

static PyValue
_interpreters_is_running(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(id)), _PyValue_STATIC_IMMORTAL(&_Py_ID(restrict)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"id", "restrict", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "is_running",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    PyValueBorrow id;
    int restricted = 0;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    id = PyValue_Borrow(args[_idx[0]]);
    if (_idx[1] >= 0) {
        restricted = _PyValue_IsTrue(PyValue_Borrow(args[_idx[1]]));
        if (restricted < 0) {
            goto exit;
        }
    }
    return_value = _interpreters_is_running_impl(PyValue_AsKnownObjectBorrow(module), id, restricted);

exit:
    return return_value;
}

PyDoc_STRVAR(_interpreters_get_config__doc__,
"get_config($module, /, id, *, restrict=False)\n"
"--\n"
"\n"
"Return a representation of the config used to initialize the interpreter.");

#define _INTERPRETERS_GET_CONFIG_METHODDEF    \
    {"get_config", _PyCFunction_CAST(_interpreters_get_config), METH_VALUE|METH_KEYWORDS, _interpreters_get_config__doc__},

static PyValue
_interpreters_get_config_impl(PyObject *module, PyValueBorrow id,
                              int restricted);

static PyValue
_interpreters_get_config(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(id)), _PyValue_STATIC_IMMORTAL(&_Py_ID(restrict)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"id", "restrict", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "get_config",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    PyValueBorrow id;
    int restricted = 0;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    id = PyValue_Borrow(args[_idx[0]]);
    if (_idx[1] >= 0) {
        restricted = _PyValue_IsTrue(PyValue_Borrow(args[_idx[1]]));
        if (restricted < 0) {
            goto exit;
        }
    }
    return_value = _interpreters_get_config_impl(PyValue_AsKnownObjectBorrow(module), id, restricted);

exit:
    return return_value;
}

PyDoc_STRVAR(_interpreters_whence__doc__,
"whence($module, /, id)\n"
"--\n"
"\n"
"Return an identifier for where the interpreter was created.");

#define _INTERPRETERS_WHENCE_METHODDEF    \
    {"whence", _PyCFunction_CAST(_interpreters_whence), METH_VALUE|METH_KEYWORDS, _interpreters_whence__doc__},

static PyValue
_interpreters_whence_impl(PyObject *module, PyValueBorrow id);

static PyValue
_interpreters_whence(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(id)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"id", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "whence",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    PyValueBorrow id;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    id = PyValue_Borrow(args[_idx[0]]);
    return_value = _interpreters_whence_impl(PyValue_AsKnownObjectBorrow(module), id);

exit:
    return return_value;
}

PyDoc_STRVAR(_interpreters_incref__doc__,
"incref($module, /, id, *, implieslink=False, restrict=False)\n"
"--\n"
"\n");

#define _INTERPRETERS_INCREF_METHODDEF    \
    {"incref", _PyCFunction_CAST(_interpreters_incref), METH_VALUE|METH_KEYWORDS, _interpreters_incref__doc__},

static PyValue
_interpreters_incref_impl(PyObject *module, PyValueBorrow id,
                          int implieslink, int restricted);

static PyValue
_interpreters_incref(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(id)), _PyValue_STATIC_IMMORTAL(&_Py_ID(implieslink)), _PyValue_STATIC_IMMORTAL(&_Py_ID(restrict)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"id", "implieslink", "restrict", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "incref",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    PyValueBorrow id;
    int implieslink = 0;
    int restricted = 0;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 3)) {
        goto exit;
    }
    id = PyValue_Borrow(args[_idx[0]]);
    if (_idx[1] >= 0) {
        implieslink = _PyValue_IsTrue(PyValue_Borrow(args[_idx[1]]));
        if (implieslink < 0) {
            goto exit;
        }
    }
    if (_idx[2] >= 0) {
        restricted = _PyValue_IsTrue(PyValue_Borrow(args[_idx[2]]));
        if (restricted < 0) {
            goto exit;
        }
    }
    return_value = _interpreters_incref_impl(PyValue_AsKnownObjectBorrow(module), id, implieslink, restricted);

exit:
    return return_value;
}

PyDoc_STRVAR(_interpreters_decref__doc__,
"decref($module, /, id, *, restrict=False)\n"
"--\n"
"\n");

#define _INTERPRETERS_DECREF_METHODDEF    \
    {"decref", _PyCFunction_CAST(_interpreters_decref), METH_VALUE|METH_KEYWORDS, _interpreters_decref__doc__},

static PyValue
_interpreters_decref_impl(PyObject *module, PyValueBorrow id, int restricted);

static PyValue
_interpreters_decref(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(id)), _PyValue_STATIC_IMMORTAL(&_Py_ID(restrict)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"id", "restrict", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "decref",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    PyValueBorrow id;
    int restricted = 0;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    id = PyValue_Borrow(args[_idx[0]]);
    if (_idx[1] >= 0) {
        restricted = _PyValue_IsTrue(PyValue_Borrow(args[_idx[1]]));
        if (restricted < 0) {
            goto exit;
        }
    }
    return_value = _interpreters_decref_impl(PyValue_AsKnownObjectBorrow(module), id, restricted);

exit:
    return return_value;
}

PyDoc_STRVAR(_interpreters_capture_exception__doc__,
"capture_exception($module, /, exc=None)\n"
"--\n"
"\n"
"Return a snapshot of an exception.\n"
"\n"
"If \"exc\" is None then the current exception, if any, is used (but not\n"
"cleared).  The returned snapshot is the same as what\n"
"_interpreters.exec() returns.");

#define _INTERPRETERS_CAPTURE_EXCEPTION_METHODDEF    \
    {"capture_exception", _PyCFunction_CAST(_interpreters_capture_exception), METH_VALUE|METH_KEYWORDS, _interpreters_capture_exception__doc__},

static PyValue
_interpreters_capture_exception_impl(PyObject *module, PyValueBorrow exc_arg);

static PyValue
_interpreters_capture_exception(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(exc)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"exc", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "capture_exception",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    PyValueBorrow exc_arg = PyValue_Borrow(PyValue_None);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        exc_arg = PyValue_Borrow(args[_idx[0]]);
    }
    return_value = _interpreters_capture_exception_impl(PyValue_AsKnownObjectBorrow(module), exc_arg);

exit:
    return return_value;
}
/*[clinic end generated code: output=29a118d73d7d2546 input=a9049054013a1b77]*/
