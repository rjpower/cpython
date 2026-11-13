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
#include "pycore_value_call.h"    // METH_VALUE

PyDoc_STRVAR(_opcode_stack_effect__doc__,
"stack_effect($module, opcode, oparg=None, /, *, jump=None)\n"
"--\n"
"\n"
"Compute the stack effect of the opcode.");

#define _OPCODE_STACK_EFFECT_METHODDEF    \
    {"stack_effect", _PyCFunction_CAST(_opcode_stack_effect), METH_VALUE|METH_KEYWORDS, _opcode_stack_effect__doc__},

static int
_opcode_stack_effect_impl(PyObject *module, int opcode, PyValueBorrow oparg,
                          PyValueBorrow jump);

static PyValue
_opcode_stack_effect(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(jump)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "", "jump", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "stack_effect",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    int opcode;
    PyValueBorrow oparg = PyValue_Borrow(PyValue_None);
    PyValueBorrow jump = PyValue_Borrow(PyValue_None);
    int _return_value;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 3)) {
        goto exit;
    }
    opcode = _PyValue_AsInt(PyValue_Borrow(args[_idx[0]]));
    if (opcode == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (_idx[1] >= 0) {
        oparg = PyValue_Borrow(args[_idx[1]]);
    }
    if (_idx[2] >= 0) {
        jump = PyValue_Borrow(args[_idx[2]]);
    }
    _return_value = _opcode_stack_effect_impl(PyValue_AsKnownObjectBorrow(module), opcode, oparg, jump);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyValue_FromInt64((int64_t)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_opcode_is_valid__doc__,
"is_valid($module, /, opcode)\n"
"--\n"
"\n"
"Return True if opcode is valid, False otherwise.");

#define _OPCODE_IS_VALID_METHODDEF    \
    {"is_valid", _PyCFunction_CAST(_opcode_is_valid), METH_VALUE|METH_KEYWORDS, _opcode_is_valid__doc__},

static int
_opcode_is_valid_impl(PyObject *module, int opcode);

static PyValue
_opcode_is_valid(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(opcode)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"opcode", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "is_valid",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    int opcode;
    int _return_value;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    opcode = _PyValue_AsInt(PyValue_Borrow(args[_idx[0]]));
    if (opcode == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = _opcode_is_valid_impl(PyValue_AsKnownObjectBorrow(module), opcode);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _return_value ? PyValue_True : PyValue_False;

exit:
    return return_value;
}

PyDoc_STRVAR(_opcode_has_arg__doc__,
"has_arg($module, /, opcode)\n"
"--\n"
"\n"
"Return True if the opcode uses its oparg, False otherwise.");

#define _OPCODE_HAS_ARG_METHODDEF    \
    {"has_arg", _PyCFunction_CAST(_opcode_has_arg), METH_VALUE|METH_KEYWORDS, _opcode_has_arg__doc__},

static int
_opcode_has_arg_impl(PyObject *module, int opcode);

static PyValue
_opcode_has_arg(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(opcode)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"opcode", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "has_arg",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    int opcode;
    int _return_value;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    opcode = _PyValue_AsInt(PyValue_Borrow(args[_idx[0]]));
    if (opcode == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = _opcode_has_arg_impl(PyValue_AsKnownObjectBorrow(module), opcode);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _return_value ? PyValue_True : PyValue_False;

exit:
    return return_value;
}

PyDoc_STRVAR(_opcode_has_const__doc__,
"has_const($module, /, opcode)\n"
"--\n"
"\n"
"Return True if the opcode accesses a constant, False otherwise.");

#define _OPCODE_HAS_CONST_METHODDEF    \
    {"has_const", _PyCFunction_CAST(_opcode_has_const), METH_VALUE|METH_KEYWORDS, _opcode_has_const__doc__},

static int
_opcode_has_const_impl(PyObject *module, int opcode);

static PyValue
_opcode_has_const(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(opcode)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"opcode", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "has_const",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    int opcode;
    int _return_value;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    opcode = _PyValue_AsInt(PyValue_Borrow(args[_idx[0]]));
    if (opcode == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = _opcode_has_const_impl(PyValue_AsKnownObjectBorrow(module), opcode);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _return_value ? PyValue_True : PyValue_False;

exit:
    return return_value;
}

PyDoc_STRVAR(_opcode_has_name__doc__,
"has_name($module, /, opcode)\n"
"--\n"
"\n"
"Return True if the opcode accesses an attribute by name, False otherwise.");

#define _OPCODE_HAS_NAME_METHODDEF    \
    {"has_name", _PyCFunction_CAST(_opcode_has_name), METH_VALUE|METH_KEYWORDS, _opcode_has_name__doc__},

static int
_opcode_has_name_impl(PyObject *module, int opcode);

static PyValue
_opcode_has_name(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(opcode)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"opcode", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "has_name",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    int opcode;
    int _return_value;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    opcode = _PyValue_AsInt(PyValue_Borrow(args[_idx[0]]));
    if (opcode == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = _opcode_has_name_impl(PyValue_AsKnownObjectBorrow(module), opcode);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _return_value ? PyValue_True : PyValue_False;

exit:
    return return_value;
}

PyDoc_STRVAR(_opcode_has_jump__doc__,
"has_jump($module, /, opcode)\n"
"--\n"
"\n"
"Return True if the opcode has a jump target, False otherwise.");

#define _OPCODE_HAS_JUMP_METHODDEF    \
    {"has_jump", _PyCFunction_CAST(_opcode_has_jump), METH_VALUE|METH_KEYWORDS, _opcode_has_jump__doc__},

static int
_opcode_has_jump_impl(PyObject *module, int opcode);

static PyValue
_opcode_has_jump(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(opcode)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"opcode", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "has_jump",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    int opcode;
    int _return_value;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    opcode = _PyValue_AsInt(PyValue_Borrow(args[_idx[0]]));
    if (opcode == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = _opcode_has_jump_impl(PyValue_AsKnownObjectBorrow(module), opcode);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _return_value ? PyValue_True : PyValue_False;

exit:
    return return_value;
}

PyDoc_STRVAR(_opcode_has_free__doc__,
"has_free($module, /, opcode)\n"
"--\n"
"\n"
"Return True if the opcode accesses a free variable, False otherwise.\n"
"\n"
"Note that \'free\' in this context refers to names in the current scope\n"
"that are referenced by inner scopes or names in outer scopes that are\n"
"referenced from this scope. It does not include references to global\n"
"or builtin scopes.");

#define _OPCODE_HAS_FREE_METHODDEF    \
    {"has_free", _PyCFunction_CAST(_opcode_has_free), METH_VALUE|METH_KEYWORDS, _opcode_has_free__doc__},

static int
_opcode_has_free_impl(PyObject *module, int opcode);

static PyValue
_opcode_has_free(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(opcode)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"opcode", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "has_free",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    int opcode;
    int _return_value;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    opcode = _PyValue_AsInt(PyValue_Borrow(args[_idx[0]]));
    if (opcode == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = _opcode_has_free_impl(PyValue_AsKnownObjectBorrow(module), opcode);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _return_value ? PyValue_True : PyValue_False;

exit:
    return return_value;
}

PyDoc_STRVAR(_opcode_has_local__doc__,
"has_local($module, /, opcode)\n"
"--\n"
"\n"
"Return True if the opcode accesses a local variable, False otherwise.");

#define _OPCODE_HAS_LOCAL_METHODDEF    \
    {"has_local", _PyCFunction_CAST(_opcode_has_local), METH_VALUE|METH_KEYWORDS, _opcode_has_local__doc__},

static int
_opcode_has_local_impl(PyObject *module, int opcode);

static PyValue
_opcode_has_local(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(opcode)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"opcode", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "has_local",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    int opcode;
    int _return_value;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    opcode = _PyValue_AsInt(PyValue_Borrow(args[_idx[0]]));
    if (opcode == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = _opcode_has_local_impl(PyValue_AsKnownObjectBorrow(module), opcode);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _return_value ? PyValue_True : PyValue_False;

exit:
    return return_value;
}

PyDoc_STRVAR(_opcode_has_exc__doc__,
"has_exc($module, /, opcode)\n"
"--\n"
"\n"
"Return True if the opcode sets an exception handler, False otherwise.");

#define _OPCODE_HAS_EXC_METHODDEF    \
    {"has_exc", _PyCFunction_CAST(_opcode_has_exc), METH_VALUE|METH_KEYWORDS, _opcode_has_exc__doc__},

static int
_opcode_has_exc_impl(PyObject *module, int opcode);

static PyValue
_opcode_has_exc(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(opcode)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"opcode", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "has_exc",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    int opcode;
    int _return_value;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    opcode = _PyValue_AsInt(PyValue_Borrow(args[_idx[0]]));
    if (opcode == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = _opcode_has_exc_impl(PyValue_AsKnownObjectBorrow(module), opcode);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _return_value ? PyValue_True : PyValue_False;

exit:
    return return_value;
}

PyDoc_STRVAR(_opcode_get_specialization_stats__doc__,
"get_specialization_stats($module, /)\n"
"--\n"
"\n"
"Return the specialization stats");

#define _OPCODE_GET_SPECIALIZATION_STATS_METHODDEF    \
    {"get_specialization_stats", _PyCFunction_CAST(_opcode_get_specialization_stats), METH_VALUE|METH_NOARGS, _opcode_get_specialization_stats__doc__},

static PyValue
_opcode_get_specialization_stats_impl(PyObject *module);

static PyValue
_opcode_get_specialization_stats(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _opcode_get_specialization_stats_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(_opcode_get_nb_ops__doc__,
"get_nb_ops($module, /)\n"
"--\n"
"\n"
"Return array of symbols of binary ops.\n"
"\n"
"Indexed by the BINARY_OP oparg value.");

#define _OPCODE_GET_NB_OPS_METHODDEF    \
    {"get_nb_ops", _PyCFunction_CAST(_opcode_get_nb_ops), METH_VALUE|METH_NOARGS, _opcode_get_nb_ops__doc__},

static PyValue
_opcode_get_nb_ops_impl(PyObject *module);

static PyValue
_opcode_get_nb_ops(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _opcode_get_nb_ops_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(_opcode_get_intrinsic1_descs__doc__,
"get_intrinsic1_descs($module, /)\n"
"--\n"
"\n"
"Return a list of names of the unary intrinsics.");

#define _OPCODE_GET_INTRINSIC1_DESCS_METHODDEF    \
    {"get_intrinsic1_descs", _PyCFunction_CAST(_opcode_get_intrinsic1_descs), METH_VALUE|METH_NOARGS, _opcode_get_intrinsic1_descs__doc__},

static PyValue
_opcode_get_intrinsic1_descs_impl(PyObject *module);

static PyValue
_opcode_get_intrinsic1_descs(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _opcode_get_intrinsic1_descs_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(_opcode_get_intrinsic2_descs__doc__,
"get_intrinsic2_descs($module, /)\n"
"--\n"
"\n"
"Return a list of names of the binary intrinsics.");

#define _OPCODE_GET_INTRINSIC2_DESCS_METHODDEF    \
    {"get_intrinsic2_descs", _PyCFunction_CAST(_opcode_get_intrinsic2_descs), METH_VALUE|METH_NOARGS, _opcode_get_intrinsic2_descs__doc__},

static PyValue
_opcode_get_intrinsic2_descs_impl(PyObject *module);

static PyValue
_opcode_get_intrinsic2_descs(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _opcode_get_intrinsic2_descs_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(_opcode_get_special_method_names__doc__,
"get_special_method_names($module, /)\n"
"--\n"
"\n"
"Return a list of special method names.");

#define _OPCODE_GET_SPECIAL_METHOD_NAMES_METHODDEF    \
    {"get_special_method_names", _PyCFunction_CAST(_opcode_get_special_method_names), METH_VALUE|METH_NOARGS, _opcode_get_special_method_names__doc__},

static PyValue
_opcode_get_special_method_names_impl(PyObject *module);

static PyValue
_opcode_get_special_method_names(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _opcode_get_special_method_names_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(_opcode_get_ir_contract__doc__,
"get_ir_contract($module, /)\n"
"--\n"
"\n"
"Return the instruction contract of the handlers of the register IR.\n"
"\n"
"Item h describes handler h: None if there is none, else an IRContract,\n"
"whose fields are those of _PyIRFormat in Include/internal/pycore_ir.h,\n"
"which documents them, and base, the opcode that the handler stands for.\n"
"roles holds the roles of fields A to D; reads, writes and through are\n"
"(k, form) extents; dst is None for a format without one; twin\n"
"is a handler or None.");

#define _OPCODE_GET_IR_CONTRACT_METHODDEF    \
    {"get_ir_contract", _PyCFunction_CAST(_opcode_get_ir_contract), METH_VALUE|METH_NOARGS, _opcode_get_ir_contract__doc__},

static PyValue
_opcode_get_ir_contract_impl(PyObject *module);

static PyValue
_opcode_get_ir_contract(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _opcode_get_ir_contract_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(_opcode_get_exception_entries__doc__,
"get_exception_entries($module, code, /)\n"
"--\n"
"\n"
"Return admitted (start, end, target, slot, lasti) exception entries.");

#define _OPCODE_GET_EXCEPTION_ENTRIES_METHODDEF    \
    {"get_exception_entries", _PyCFunction_CAST(_opcode_get_exception_entries), METH_VALUE|METH_O, _opcode_get_exception_entries__doc__},

static PyValue
_opcode_get_exception_entries_impl(PyObject *module, PyCodeObject *code);

static PyValue
_opcode_get_exception_entries(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyCodeObject *code;

    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[0])), &PyCode_Type)) {
        _PyArg_BadArgumentValue("get_exception_entries", "argument", (&PyCode_Type)->tp_name, PyValue_Borrow(args[0]));
        goto exit;
    }
    code = (PyCodeObject *)PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    return_value = _opcode_get_exception_entries_impl(PyValue_AsKnownObjectBorrow(module), code);

exit:
    return return_value;
}

PyDoc_STRVAR(_opcode_get_recovery_entries__doc__,
"get_recovery_entries($module, code, /)\n"
"--\n"
"\n"
"Return semantic recovery rows.\n"
"\n"
"Each row contains (from, to, edge, handler, exception slot, lasti).");

#define _OPCODE_GET_RECOVERY_ENTRIES_METHODDEF    \
    {"get_recovery_entries", _PyCFunction_CAST(_opcode_get_recovery_entries), METH_VALUE|METH_O, _opcode_get_recovery_entries__doc__},

static PyValue
_opcode_get_recovery_entries_impl(PyObject *module, PyCodeObject *code);

static PyValue
_opcode_get_recovery_entries(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyCodeObject *code;

    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[0])), &PyCode_Type)) {
        _PyArg_BadArgumentValue("get_recovery_entries", "argument", (&PyCode_Type)->tp_name, PyValue_Borrow(args[0]));
        goto exit;
    }
    code = (PyCodeObject *)PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    return_value = _opcode_get_recovery_entries_impl(PyValue_AsKnownObjectBorrow(module), code);

exit:
    return return_value;
}
/*[clinic end generated code: output=6e605283b48d51c8 input=a9049054013a1b77]*/
