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
#include "pycore_value_slots.h"   // _PyValue_THUNK_EXIT()

PyDoc_STRVAR(function___annotate____doc__,
"Get the code object for a function.");

static PyObject *
function___annotate___get_impl(PyFunctionObject *self);

static PyObject *
function___annotate___get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = function___annotate___get_impl((PyFunctionObject *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

static int
function___annotate___set_impl(PyFunctionObject *self, PyObject *value);

static int
function___annotate___set(PyObject *self, PyObject *arg, void *Py_UNUSED(context))
{
    int return_value = -1;
    PyObject *value;

    if (arg == NULL) {
        PyErr_Format(PyExc_AttributeError,
                     "attribute '__annotate__' of '%.100s' objects cannot be deleted",
                     Py_TYPE(self)->tp_name);
        return -1;
    }
    value = arg;
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = function___annotate___set_impl((PyFunctionObject *)self, value);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(function___annotations____doc__,
"Dict of annotations in a function object.");

static PyObject *
function___annotations___get_impl(PyFunctionObject *self);

static PyObject *
function___annotations___get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = function___annotations___get_impl((PyFunctionObject *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

static int
function___annotations___set_impl(PyFunctionObject *self, PyObject *value);

static int
function___annotations___set(PyObject *self, PyObject *arg, void *Py_UNUSED(context))
{
    int return_value = -1;
    PyObject *value = NULL;

    if (arg != NULL) {
        value = arg;
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = function___annotations___set_impl((PyFunctionObject *)self, value);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(function___type_params____doc__,
"Get the declared type parameters for a function.");

static PyObject *
function___type_params___get_impl(PyFunctionObject *self);

static PyObject *
function___type_params___get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = function___type_params___get_impl((PyFunctionObject *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

static int
function___type_params___set_impl(PyFunctionObject *self, PyObject *value);

static int
function___type_params___set(PyObject *self, PyObject *arg, void *Py_UNUSED(context))
{
    int return_value = -1;
    PyObject *value;

    if (arg == NULL) {
        PyErr_Format(PyExc_AttributeError,
                     "attribute '__type_params__' of '%.100s' objects cannot be deleted",
                     Py_TYPE(self)->tp_name);
        return -1;
    }
    if (!PyTuple_Check(arg)) {
        PyErr_Format(PyExc_TypeError, "attribute '__type_params__' must be tuple, not %T", arg);
        goto exit;
    }
    value = arg;
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = function___type_params___set_impl((PyFunctionObject *)self, value);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(func_new__doc__,
"function(code, globals, name=None, argdefs=None, closure=None,\n"
"         kwdefaults=None)\n"
"--\n"
"\n"
"Create a function object.\n"
"\n"
"  code\n"
"    a code object\n"
"  globals\n"
"    the globals dictionary\n"
"  name\n"
"    a string that overrides the name from the code object\n"
"  argdefs\n"
"    a tuple that specifies the default argument values\n"
"  closure\n"
"    a tuple that supplies the bindings for free variables\n"
"  kwdefaults\n"
"    a dictionary that specifies the default keyword argument values");

static PyValue
func_new_impl(PyTypeObject *type, PyCodeObject *code, PyObject *globals,
              PyValueBorrow name, PyValueBorrow defaults,
              PyValueBorrow closure, PyValueBorrow kwdefaults);

static PyValue
func_new_value(PyTypeObject *type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(code)), _PyValue_STATIC_IMMORTAL(&_Py_ID(globals)), _PyValue_STATIC_IMMORTAL(&_Py_ID(name)), _PyValue_STATIC_IMMORTAL(&_Py_ID(argdefs)), _PyValue_STATIC_IMMORTAL(&_Py_ID(closure)), _PyValue_STATIC_IMMORTAL(&_Py_ID(kwdefaults)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"code", "globals", "name", "argdefs", "closure", "kwdefaults", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "function",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[6];
    PyCodeObject *code;
    PyObject *globals;
    PyValueBorrow name = PyValue_Borrow(PyValue_None);
    PyValueBorrow defaults = PyValue_Borrow(PyValue_None);
    PyValueBorrow closure = PyValue_Borrow(PyValue_None);
    PyValueBorrow kwdefaults = PyValue_Borrow(PyValue_None);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 6, /*minkw*/ 0, /*varpos*/ 0, _idx, 6)) {
        goto exit;
    }
    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[_idx[0]])), &PyCode_Type)) {
        _PyArg_BadArgumentValue("function", "argument 'code'", (&PyCode_Type)->tp_name, PyValue_Borrow(args[_idx[0]]));
        goto exit;
    }
    code = (PyCodeObject *)PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]]));
    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[_idx[1]])), &PyDict_Type)) {
        _PyArg_BadArgumentValue("function", "argument 'globals'", "dict", PyValue_Borrow(args[_idx[1]]));
        goto exit;
    }
    globals = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[1]]));
    if (_idx[2] >= 0) {
        name = PyValue_Borrow(args[_idx[2]]);
    }
    if (_idx[3] >= 0) {
        defaults = PyValue_Borrow(args[_idx[3]]);
    }
    if (_idx[4] >= 0) {
        closure = PyValue_Borrow(args[_idx[4]]);
    }
    if (_idx[5] >= 0) {
        kwdefaults = PyValue_Borrow(args[_idx[5]]);
    }
    return_value = func_new_impl(type, code, globals, name, defaults, closure, kwdefaults);

exit:
    return return_value;
}

Py_VALUE_ENTRY static PyObject *
func_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _PyValue_THUNK_EXIT(func_new_value(type, _PyTuple_ITEMS(args), nargs, NULL), BOX_SITE_CALL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return NULL;
    }
    PyValue result = func_new_value(type, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return _PyValue_THUNK_EXIT(result, BOX_SITE_CALL);
}
#define FUNCTION___ANNOTATE___GETSETDEF {"__annotate__", (getter)function___annotate___get, (setter)function___annotate___set, function___annotate____doc__},

#define FUNCTION___ANNOTATIONS___GETSETDEF {"__annotations__", (getter)function___annotations___get, (setter)function___annotations___set, function___annotations____doc__},

#define FUNCTION___TYPE_PARAMS___GETSETDEF {"__type_params__", (getter)function___type_params___get, (setter)function___type_params___set, function___type_params____doc__},

/*[clinic end generated code: output=24cee8c0ea2a2a5b input=a9049054013a1b77]*/
