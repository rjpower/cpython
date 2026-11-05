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
#include "pycore_tuple.h"         // _PyTuple_FromValueArray()
#include "pycore_value_call.h"    // METH_VALUE
#include "pycore_value_ops.h"     // _PyValue_IsTrue()
#include "pycore_value_slots.h"   // _PyValue_THUNK_EXIT()

PyDoc_STRVAR(typevar_new__doc__,
"typevar(name, *constraints, bound=None, default=typing.NoDefault,\n"
"        covariant=False, contravariant=False, infer_variance=False)\n"
"--\n"
"\n"
"Create a TypeVar.");

static PyValue
typevar_new_impl(PyTypeObject *type, PyObject *name, PyObject *constraints,
                 PyValueBorrow bound, PyValueBorrow default_value,
                 int covariant, int contravariant, int infer_variance);

static PyValue
typevar_new_value(PyTypeObject *type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(name)), _PyValue_STATIC_IMMORTAL(&_Py_ID(bound)), _PyValue_STATIC_IMMORTAL(&_Py_ID(default)), _PyValue_STATIC_IMMORTAL(&_Py_ID(covariant)), _PyValue_STATIC_IMMORTAL(&_Py_ID(contravariant)), _PyValue_STATIC_IMMORTAL(&_Py_ID(infer_variance)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"name", "bound", "default", "covariant", "contravariant", "infer_variance", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "typevar",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[6];
    PyObject *name;
    PyObject *constraints = NULL;
    PyValueBorrow bound = PyValue_Borrow(PyValue_None);
    PyValueBorrow default_value = PyValue_FromObjectBorrow(&_Py_NoDefaultStruct);
    int covariant = 0;
    int contravariant = 0;
    int infer_variance = 0;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 1, _idx, 6)) {
        goto exit;
    }
    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[_idx[0]])), &PyUnicode_Type)) {
        _PyArg_BadArgumentValue("typevar", "argument 'name'", "str", PyValue_Borrow(args[_idx[0]]));
        goto exit;
    }
    name = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]]));
    if (_idx[1] >= 0) {
        bound = PyValue_Borrow(args[_idx[1]]);
    }
    if (_idx[2] >= 0) {
        default_value = PyValue_Borrow(args[_idx[2]]);
    }
    if (_idx[3] >= 0) {
        covariant = _PyValue_IsTrue(PyValue_Borrow(args[_idx[3]]));
        if (covariant < 0) {
            goto exit;
        }
    }
    if (_idx[4] >= 0) {
        contravariant = _PyValue_IsTrue(PyValue_Borrow(args[_idx[4]]));
        if (contravariant < 0) {
            goto exit;
        }
    }
    if (_idx[5] >= 0) {
        infer_variance = _PyValue_IsTrue(PyValue_Borrow(args[_idx[5]]));
        if (infer_variance < 0) {
            goto exit;
        }
    }
    constraints = _PyTuple_FromValueArray(args + 1,
                                          Py_MAX(nargs - 1, 0));
    if (constraints == NULL) {
        goto exit;
    }
    return_value = typevar_new_impl(type, name, constraints, bound, default_value, covariant, contravariant, infer_variance);

exit:
    /* Cleanup for constraints */
    Py_XDECREF(constraints);

    return return_value;
}

Py_VALUE_ENTRY static PyObject *
typevar_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _PyValue_THUNK_EXIT(typevar_new_value(type, _PyTuple_ITEMS(args), nargs, NULL), BOX_SITE_CALL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return NULL;
    }
    PyValue result = typevar_new_value(type, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return _PyValue_THUNK_EXIT(result, BOX_SITE_CALL);
}

PyDoc_STRVAR(typevar_typing_subst__doc__,
"__typing_subst__($self, arg, /)\n"
"--\n"
"\n");

#define TYPEVAR_TYPING_SUBST_METHODDEF    \
    {"__typing_subst__", _PyCFunction_CAST(typevar_typing_subst), METH_VALUE|METH_O, typevar_typing_subst__doc__},

static PyValue
typevar_typing_subst_impl(typevarobject *self, PyValueBorrow arg);

static PyValue
typevar_typing_subst(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow arg;

    arg = PyValue_Borrow(args[0]);
    return_value = typevar_typing_subst_impl((typevarobject *)PyValue_AsKnownObjectBorrow(self), arg);

    return return_value;
}

PyDoc_STRVAR(typevar_typing_prepare_subst__doc__,
"__typing_prepare_subst__($self, alias, args, /)\n"
"--\n"
"\n");

#define TYPEVAR_TYPING_PREPARE_SUBST_METHODDEF    \
    {"__typing_prepare_subst__", _PyCFunction_CAST(typevar_typing_prepare_subst), METH_VALUE, typevar_typing_prepare_subst__doc__},

static PyValue
typevar_typing_prepare_subst_impl(typevarobject *self, PyValueBorrow alias,
                                  PyValueBorrow args);

static PyValue
typevar_typing_prepare_subst(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow alias;
    PyValueBorrow __clinic_args;

    if (!_PyArg_NoKwnamesValue("TypeVar.__typing_prepare_subst__()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("__typing_prepare_subst__", nargs, 2, 2)) {
        goto exit;
    }
    alias = PyValue_Borrow(args[0]);
    __clinic_args = PyValue_Borrow(args[1]);
    return_value = typevar_typing_prepare_subst_impl((typevarobject *)PyValue_AsKnownObjectBorrow(self), alias, __clinic_args);

exit:
    return return_value;
}

PyDoc_STRVAR(typevar_reduce__doc__,
"__reduce__($self, /)\n"
"--\n"
"\n");

#define TYPEVAR_REDUCE_METHODDEF    \
    {"__reduce__", _PyCFunction_CAST(typevar_reduce), METH_VALUE|METH_NOARGS, typevar_reduce__doc__},

static PyValue
typevar_reduce_impl(typevarobject *self);

static PyValue
typevar_reduce(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = typevar_reduce_impl((typevarobject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(typevar_has_default__doc__,
"has_default($self, /)\n"
"--\n"
"\n");

#define TYPEVAR_HAS_DEFAULT_METHODDEF    \
    {"has_default", _PyCFunction_CAST(typevar_has_default), METH_VALUE|METH_NOARGS, typevar_has_default__doc__},

static PyValue
typevar_has_default_impl(typevarobject *self);

static PyValue
typevar_has_default(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = typevar_has_default_impl((typevarobject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(paramspecargs_new__doc__,
"paramspecargs(origin)\n"
"--\n"
"\n"
"Create a ParamSpecArgs object.");

static PyValue
paramspecargs_new_impl(PyTypeObject *type, PyValueBorrow origin);

static PyValue
paramspecargs_new_value(PyTypeObject *type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(origin)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"origin", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "paramspecargs",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    PyValueBorrow origin;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    origin = PyValue_Borrow(args[_idx[0]]);
    return_value = paramspecargs_new_impl(type, origin);

exit:
    return return_value;
}

Py_VALUE_ENTRY static PyObject *
paramspecargs_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _PyValue_THUNK_EXIT(paramspecargs_new_value(type, _PyTuple_ITEMS(args), nargs, NULL), BOX_SITE_CALL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return NULL;
    }
    PyValue result = paramspecargs_new_value(type, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return _PyValue_THUNK_EXIT(result, BOX_SITE_CALL);
}

PyDoc_STRVAR(paramspeckwargs_new__doc__,
"paramspeckwargs(origin)\n"
"--\n"
"\n"
"Create a ParamSpecKwargs object.");

static PyValue
paramspeckwargs_new_impl(PyTypeObject *type, PyValueBorrow origin);

static PyValue
paramspeckwargs_new_value(PyTypeObject *type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(origin)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"origin", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "paramspeckwargs",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    PyValueBorrow origin;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    origin = PyValue_Borrow(args[_idx[0]]);
    return_value = paramspeckwargs_new_impl(type, origin);

exit:
    return return_value;
}

Py_VALUE_ENTRY static PyObject *
paramspeckwargs_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _PyValue_THUNK_EXIT(paramspeckwargs_new_value(type, _PyTuple_ITEMS(args), nargs, NULL), BOX_SITE_CALL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return NULL;
    }
    PyValue result = paramspeckwargs_new_value(type, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return _PyValue_THUNK_EXIT(result, BOX_SITE_CALL);
}

PyDoc_STRVAR(paramspec_new__doc__,
"paramspec(name, *, bound=None, default=typing.NoDefault,\n"
"          covariant=False, contravariant=False, infer_variance=False)\n"
"--\n"
"\n"
"Create a ParamSpec object.");

static PyValue
paramspec_new_impl(PyTypeObject *type, PyObject *name, PyValueBorrow bound,
                   PyValueBorrow default_value, int covariant,
                   int contravariant, int infer_variance);

static PyValue
paramspec_new_value(PyTypeObject *type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(name)), _PyValue_STATIC_IMMORTAL(&_Py_ID(bound)), _PyValue_STATIC_IMMORTAL(&_Py_ID(default)), _PyValue_STATIC_IMMORTAL(&_Py_ID(covariant)), _PyValue_STATIC_IMMORTAL(&_Py_ID(contravariant)), _PyValue_STATIC_IMMORTAL(&_Py_ID(infer_variance)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"name", "bound", "default", "covariant", "contravariant", "infer_variance", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "paramspec",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[6];
    PyObject *name;
    PyValueBorrow bound = PyValue_Borrow(PyValue_None);
    PyValueBorrow default_value = PyValue_FromObjectBorrow(&_Py_NoDefaultStruct);
    int covariant = 0;
    int contravariant = 0;
    int infer_variance = 0;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 6)) {
        goto exit;
    }
    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[_idx[0]])), &PyUnicode_Type)) {
        _PyArg_BadArgumentValue("paramspec", "argument 'name'", "str", PyValue_Borrow(args[_idx[0]]));
        goto exit;
    }
    name = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]]));
    if (_idx[1] >= 0) {
        bound = PyValue_Borrow(args[_idx[1]]);
    }
    if (_idx[2] >= 0) {
        default_value = PyValue_Borrow(args[_idx[2]]);
    }
    if (_idx[3] >= 0) {
        covariant = _PyValue_IsTrue(PyValue_Borrow(args[_idx[3]]));
        if (covariant < 0) {
            goto exit;
        }
    }
    if (_idx[4] >= 0) {
        contravariant = _PyValue_IsTrue(PyValue_Borrow(args[_idx[4]]));
        if (contravariant < 0) {
            goto exit;
        }
    }
    if (_idx[5] >= 0) {
        infer_variance = _PyValue_IsTrue(PyValue_Borrow(args[_idx[5]]));
        if (infer_variance < 0) {
            goto exit;
        }
    }
    return_value = paramspec_new_impl(type, name, bound, default_value, covariant, contravariant, infer_variance);

exit:
    return return_value;
}

Py_VALUE_ENTRY static PyObject *
paramspec_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _PyValue_THUNK_EXIT(paramspec_new_value(type, _PyTuple_ITEMS(args), nargs, NULL), BOX_SITE_CALL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return NULL;
    }
    PyValue result = paramspec_new_value(type, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return _PyValue_THUNK_EXIT(result, BOX_SITE_CALL);
}

PyDoc_STRVAR(paramspec_typing_subst__doc__,
"__typing_subst__($self, arg, /)\n"
"--\n"
"\n");

#define PARAMSPEC_TYPING_SUBST_METHODDEF    \
    {"__typing_subst__", _PyCFunction_CAST(paramspec_typing_subst), METH_VALUE|METH_O, paramspec_typing_subst__doc__},

static PyValue
paramspec_typing_subst_impl(paramspecobject *self, PyValueBorrow arg);

static PyValue
paramspec_typing_subst(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow arg;

    arg = PyValue_Borrow(args[0]);
    return_value = paramspec_typing_subst_impl((paramspecobject *)PyValue_AsKnownObjectBorrow(self), arg);

    return return_value;
}

PyDoc_STRVAR(paramspec_typing_prepare_subst__doc__,
"__typing_prepare_subst__($self, alias, args, /)\n"
"--\n"
"\n");

#define PARAMSPEC_TYPING_PREPARE_SUBST_METHODDEF    \
    {"__typing_prepare_subst__", _PyCFunction_CAST(paramspec_typing_prepare_subst), METH_VALUE, paramspec_typing_prepare_subst__doc__},

static PyValue
paramspec_typing_prepare_subst_impl(paramspecobject *self,
                                    PyValueBorrow alias, PyValueBorrow args);

static PyValue
paramspec_typing_prepare_subst(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow alias;
    PyValueBorrow __clinic_args;

    if (!_PyArg_NoKwnamesValue("ParamSpec.__typing_prepare_subst__()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("__typing_prepare_subst__", nargs, 2, 2)) {
        goto exit;
    }
    alias = PyValue_Borrow(args[0]);
    __clinic_args = PyValue_Borrow(args[1]);
    return_value = paramspec_typing_prepare_subst_impl((paramspecobject *)PyValue_AsKnownObjectBorrow(self), alias, __clinic_args);

exit:
    return return_value;
}

PyDoc_STRVAR(paramspec_reduce__doc__,
"__reduce__($self, /)\n"
"--\n"
"\n");

#define PARAMSPEC_REDUCE_METHODDEF    \
    {"__reduce__", _PyCFunction_CAST(paramspec_reduce), METH_VALUE|METH_NOARGS, paramspec_reduce__doc__},

static PyValue
paramspec_reduce_impl(paramspecobject *self);

static PyValue
paramspec_reduce(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = paramspec_reduce_impl((paramspecobject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(paramspec_has_default__doc__,
"has_default($self, /)\n"
"--\n"
"\n");

#define PARAMSPEC_HAS_DEFAULT_METHODDEF    \
    {"has_default", _PyCFunction_CAST(paramspec_has_default), METH_VALUE|METH_NOARGS, paramspec_has_default__doc__},

static PyValue
paramspec_has_default_impl(paramspecobject *self);

static PyValue
paramspec_has_default(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = paramspec_has_default_impl((paramspecobject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(typevartuple__doc__,
"typevartuple(name, *, bound=None, covariant=False, contravariant=False,\n"
"             infer_variance=False, default=typing.NoDefault)\n"
"--\n"
"\n"
"Create a new TypeVarTuple with the given name.");

static PyValue
typevartuple_impl(PyTypeObject *type, PyObject *name, PyValueBorrow bound,
                  int covariant, int contravariant, int infer_variance,
                  PyValueBorrow default_value);

static PyValue
typevartuple_value(PyTypeObject *type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(name)), _PyValue_STATIC_IMMORTAL(&_Py_ID(bound)), _PyValue_STATIC_IMMORTAL(&_Py_ID(covariant)), _PyValue_STATIC_IMMORTAL(&_Py_ID(contravariant)), _PyValue_STATIC_IMMORTAL(&_Py_ID(infer_variance)), _PyValue_STATIC_IMMORTAL(&_Py_ID(default)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"name", "bound", "covariant", "contravariant", "infer_variance", "default", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "typevartuple",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[6];
    PyObject *name;
    PyValueBorrow bound = PyValue_Borrow(PyValue_None);
    int covariant = 0;
    int contravariant = 0;
    int infer_variance = 0;
    PyValueBorrow default_value = PyValue_FromObjectBorrow(&_Py_NoDefaultStruct);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 6)) {
        goto exit;
    }
    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[_idx[0]])), &PyUnicode_Type)) {
        _PyArg_BadArgumentValue("typevartuple", "argument 'name'", "str", PyValue_Borrow(args[_idx[0]]));
        goto exit;
    }
    name = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]]));
    if (_idx[1] >= 0) {
        bound = PyValue_Borrow(args[_idx[1]]);
    }
    if (_idx[2] >= 0) {
        covariant = _PyValue_IsTrue(PyValue_Borrow(args[_idx[2]]));
        if (covariant < 0) {
            goto exit;
        }
    }
    if (_idx[3] >= 0) {
        contravariant = _PyValue_IsTrue(PyValue_Borrow(args[_idx[3]]));
        if (contravariant < 0) {
            goto exit;
        }
    }
    if (_idx[4] >= 0) {
        infer_variance = _PyValue_IsTrue(PyValue_Borrow(args[_idx[4]]));
        if (infer_variance < 0) {
            goto exit;
        }
    }
    if (_idx[5] >= 0) {
        default_value = PyValue_Borrow(args[_idx[5]]);
    }
    return_value = typevartuple_impl(type, name, bound, covariant, contravariant, infer_variance, default_value);

exit:
    return return_value;
}

Py_VALUE_ENTRY static PyObject *
typevartuple(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _PyValue_THUNK_EXIT(typevartuple_value(type, _PyTuple_ITEMS(args), nargs, NULL), BOX_SITE_CALL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return NULL;
    }
    PyValue result = typevartuple_value(type, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return _PyValue_THUNK_EXIT(result, BOX_SITE_CALL);
}

PyDoc_STRVAR(typevartuple_typing_subst__doc__,
"__typing_subst__($self, arg, /)\n"
"--\n"
"\n");

#define TYPEVARTUPLE_TYPING_SUBST_METHODDEF    \
    {"__typing_subst__", _PyCFunction_CAST(typevartuple_typing_subst), METH_VALUE|METH_O, typevartuple_typing_subst__doc__},

static PyValue
typevartuple_typing_subst_impl(typevartupleobject *self, PyValueBorrow arg);

static PyValue
typevartuple_typing_subst(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow arg;

    arg = PyValue_Borrow(args[0]);
    return_value = typevartuple_typing_subst_impl((typevartupleobject *)PyValue_AsKnownObjectBorrow(self), arg);

    return return_value;
}

PyDoc_STRVAR(typevartuple_typing_prepare_subst__doc__,
"__typing_prepare_subst__($self, alias, args, /)\n"
"--\n"
"\n");

#define TYPEVARTUPLE_TYPING_PREPARE_SUBST_METHODDEF    \
    {"__typing_prepare_subst__", _PyCFunction_CAST(typevartuple_typing_prepare_subst), METH_VALUE, typevartuple_typing_prepare_subst__doc__},

static PyValue
typevartuple_typing_prepare_subst_impl(typevartupleobject *self,
                                       PyValueBorrow alias,
                                       PyValueBorrow args);

static PyValue
typevartuple_typing_prepare_subst(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow alias;
    PyValueBorrow __clinic_args;

    if (!_PyArg_NoKwnamesValue("TypeVarTuple.__typing_prepare_subst__()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("__typing_prepare_subst__", nargs, 2, 2)) {
        goto exit;
    }
    alias = PyValue_Borrow(args[0]);
    __clinic_args = PyValue_Borrow(args[1]);
    return_value = typevartuple_typing_prepare_subst_impl((typevartupleobject *)PyValue_AsKnownObjectBorrow(self), alias, __clinic_args);

exit:
    return return_value;
}

PyDoc_STRVAR(typevartuple_reduce__doc__,
"__reduce__($self, /)\n"
"--\n"
"\n");

#define TYPEVARTUPLE_REDUCE_METHODDEF    \
    {"__reduce__", _PyCFunction_CAST(typevartuple_reduce), METH_VALUE|METH_NOARGS, typevartuple_reduce__doc__},

static PyValue
typevartuple_reduce_impl(typevartupleobject *self);

static PyValue
typevartuple_reduce(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = typevartuple_reduce_impl((typevartupleobject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(typevartuple_has_default__doc__,
"has_default($self, /)\n"
"--\n"
"\n");

#define TYPEVARTUPLE_HAS_DEFAULT_METHODDEF    \
    {"has_default", _PyCFunction_CAST(typevartuple_has_default), METH_VALUE|METH_NOARGS, typevartuple_has_default__doc__},

static PyValue
typevartuple_has_default_impl(typevartupleobject *self);

static PyValue
typevartuple_has_default(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = typevartuple_has_default_impl((typevartupleobject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(typealias_reduce__doc__,
"__reduce__($self, /)\n"
"--\n"
"\n");

#define TYPEALIAS_REDUCE_METHODDEF    \
    {"__reduce__", _PyCFunction_CAST(typealias_reduce), METH_VALUE|METH_NOARGS, typealias_reduce__doc__},

static PyValue
typealias_reduce_impl(typealiasobject *self);

static PyValue
typealias_reduce(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = typealias_reduce_impl((typealiasobject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(typealias_new__doc__,
"typealias(name, value, *, type_params=(), qualname=None)\n"
"--\n"
"\n"
"Create a TypeAliasType.");

static PyValue
typealias_new_impl(PyTypeObject *type, PyObject *name, PyValueBorrow value,
                   PyValueBorrow type_params, PyValueBorrow qualname);

static PyValue
typealias_new_value(PyTypeObject *type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(name)), _PyValue_STATIC_IMMORTAL(&_Py_ID(value)), _PyValue_STATIC_IMMORTAL(&_Py_ID(type_params)), _PyValue_STATIC_IMMORTAL(&_Py_ID(qualname)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"name", "value", "type_params", "qualname", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "typealias",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[4];
    PyObject *name;
    PyValueBorrow value;
    PyValueBorrow type_params = PyValue_Borrow(PyValue_NULL);
    PyValueBorrow qualname = PyValue_Borrow(PyValue_NULL);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 4)) {
        goto exit;
    }
    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[_idx[0]])), &PyUnicode_Type)) {
        _PyArg_BadArgumentValue("typealias", "argument 'name'", "str", PyValue_Borrow(args[_idx[0]]));
        goto exit;
    }
    name = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]]));
    value = PyValue_Borrow(args[_idx[1]]);
    if (_idx[2] >= 0) {
        type_params = PyValue_Borrow(args[_idx[2]]);
    }
    if (_idx[3] >= 0) {
        qualname = PyValue_Borrow(args[_idx[3]]);
    }
    return_value = typealias_new_impl(type, name, value, type_params, qualname);

exit:
    return return_value;
}

Py_VALUE_ENTRY static PyObject *
typealias_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _PyValue_THUNK_EXIT(typealias_new_value(type, _PyTuple_ITEMS(args), nargs, NULL), BOX_SITE_CALL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return NULL;
    }
    PyValue result = typealias_new_value(type, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return _PyValue_THUNK_EXIT(result, BOX_SITE_CALL);
}
/*[clinic end generated code: output=2cf8cf4d8bd4101f input=a9049054013a1b77]*/
