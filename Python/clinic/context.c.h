/*[clinic input]
preserve
[clinic start generated code]*/

#ifndef Py_VALUE_CORE
#  error "value-emission Clinic output requires Py_VALUE_CORE"
#endif

#include "pycore_modsupport.h"    // _PyArg_CheckPositional()
#include "pycore_value_call.h"    // METH_VALUE

PyDoc_STRVAR(_contextvars_Context_get__doc__,
"get($self, key, default=None, /)\n"
"--\n"
"\n"
"Return the value for `key` if `key` has the value in the context object.\n"
"\n"
"If `key` does not exist, return `default`.  If `default` is not\n"
"given, return None.");

#define _CONTEXTVARS_CONTEXT_GET_METHODDEF    \
    {"get", _PyCFunction_CAST(_contextvars_Context_get), METH_VALUE, _contextvars_Context_get__doc__},

static PyValue
_contextvars_Context_get_impl(PyContext *self, PyValueBorrow key,
                              PyValueBorrow default_value);

static PyValue
_contextvars_Context_get(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow key;
    PyValueBorrow default_value = PyValue_Borrow(PyValue_None);

    if (!_PyArg_CheckPositional("get", nargs, 1, 2)) {
        goto exit;
    }
    key = PyValue_Borrow(args[0]);
    if (nargs < 2) {
        goto skip_optional;
    }
    default_value = PyValue_Borrow(args[1]);
skip_optional:
    return_value = _contextvars_Context_get_impl((PyContext *)PyValue_AsKnownObjectBorrow(self), key, default_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_contextvars_Context_items__doc__,
"items($self, /)\n"
"--\n"
"\n"
"Return all variables and their values in the context object.\n"
"\n"
"The result is returned as a list of 2-tuples (variable, value).");

#define _CONTEXTVARS_CONTEXT_ITEMS_METHODDEF    \
    {"items", _PyCFunction_CAST(_contextvars_Context_items), METH_VALUE|METH_NOARGS, _contextvars_Context_items__doc__},

static PyValue
_contextvars_Context_items_impl(PyContext *self);

static PyValue
_contextvars_Context_items(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _contextvars_Context_items_impl((PyContext *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_contextvars_Context_keys__doc__,
"keys($self, /)\n"
"--\n"
"\n"
"Return a list of all variables in the context object.");

#define _CONTEXTVARS_CONTEXT_KEYS_METHODDEF    \
    {"keys", _PyCFunction_CAST(_contextvars_Context_keys), METH_VALUE|METH_NOARGS, _contextvars_Context_keys__doc__},

static PyValue
_contextvars_Context_keys_impl(PyContext *self);

static PyValue
_contextvars_Context_keys(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _contextvars_Context_keys_impl((PyContext *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_contextvars_Context_values__doc__,
"values($self, /)\n"
"--\n"
"\n"
"Return a list of all variables\' values in the context object.");

#define _CONTEXTVARS_CONTEXT_VALUES_METHODDEF    \
    {"values", _PyCFunction_CAST(_contextvars_Context_values), METH_VALUE|METH_NOARGS, _contextvars_Context_values__doc__},

static PyValue
_contextvars_Context_values_impl(PyContext *self);

static PyValue
_contextvars_Context_values(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _contextvars_Context_values_impl((PyContext *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_contextvars_Context_copy__doc__,
"copy($self, /)\n"
"--\n"
"\n"
"Return a shallow copy of the context object.");

#define _CONTEXTVARS_CONTEXT_COPY_METHODDEF    \
    {"copy", _PyCFunction_CAST(_contextvars_Context_copy), METH_VALUE|METH_NOARGS, _contextvars_Context_copy__doc__},

static PyValue
_contextvars_Context_copy_impl(PyContext *self);

static PyValue
_contextvars_Context_copy(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _contextvars_Context_copy_impl((PyContext *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_contextvars_ContextVar_get__doc__,
"get($self, default=<unrepresentable>, /)\n"
"--\n"
"\n"
"Return a value for the context variable for the current context.\n"
"\n"
"If there is no value for the variable in the current context, the\n"
"method will:\n"
" * return the value of the default argument of the method, if\n"
"   provided; or\n"
" * return the default value for the context variable, if it was\n"
"   created with one; or\n"
" * raise a LookupError.");

#define _CONTEXTVARS_CONTEXTVAR_GET_METHODDEF    \
    {"get", _PyCFunction_CAST(_contextvars_ContextVar_get), METH_VALUE, _contextvars_ContextVar_get__doc__},

static PyValue
_contextvars_ContextVar_get_impl(PyContextVar *self,
                                 PyValueBorrow default_value);

static PyValue
_contextvars_ContextVar_get(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow default_value = PyValue_Borrow(PyValue_NULL);

    if (!_PyArg_CheckPositional("get", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    default_value = PyValue_Borrow(args[0]);
skip_optional:
    return_value = _contextvars_ContextVar_get_impl((PyContextVar *)PyValue_AsKnownObjectBorrow(self), default_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_contextvars_ContextVar_set__doc__,
"set($self, value, /)\n"
"--\n"
"\n"
"Call to set a new value for the context variable in the current context.\n"
"\n"
"The required value argument is the new value for the context\n"
"variable.\n"
"\n"
"Returns a Token object that can be used to restore the variable to\n"
"its previous value via the `ContextVar.reset()` method.");

#define _CONTEXTVARS_CONTEXTVAR_SET_METHODDEF    \
    {"set", _PyCFunction_CAST(_contextvars_ContextVar_set), METH_VALUE|METH_O, _contextvars_ContextVar_set__doc__},

static PyValue
_contextvars_ContextVar_set_impl(PyContextVar *self, PyValueBorrow value);

static PyValue
_contextvars_ContextVar_set(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow value;

    value = PyValue_Borrow(args[0]);
    return_value = _contextvars_ContextVar_set_impl((PyContextVar *)PyValue_AsKnownObjectBorrow(self), value);

    return return_value;
}

PyDoc_STRVAR(_contextvars_ContextVar_reset__doc__,
"reset($self, token, /)\n"
"--\n"
"\n"
"Reset the context variable.\n"
"\n"
"The variable is reset to the value it had before the\n"
"`ContextVar.set()` that created the token was used.");

#define _CONTEXTVARS_CONTEXTVAR_RESET_METHODDEF    \
    {"reset", _PyCFunction_CAST(_contextvars_ContextVar_reset), METH_VALUE|METH_O, _contextvars_ContextVar_reset__doc__},

static PyValue
_contextvars_ContextVar_reset_impl(PyContextVar *self, PyValueBorrow token);

static PyValue
_contextvars_ContextVar_reset(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow token;

    token = PyValue_Borrow(args[0]);
    return_value = _contextvars_ContextVar_reset_impl((PyContextVar *)PyValue_AsKnownObjectBorrow(self), token);

    return return_value;
}

PyDoc_STRVAR(token_enter__doc__,
"__enter__($self, /)\n"
"--\n"
"\n"
"Enter into Token context manager.");

#define TOKEN_ENTER_METHODDEF    \
    {"__enter__", _PyCFunction_CAST(token_enter), METH_VALUE|METH_NOARGS, token_enter__doc__},

static PyValue
token_enter_impl(PyContextToken *self);

static PyValue
token_enter(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = token_enter_impl((PyContextToken *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(token_exit__doc__,
"__exit__($self, type, val, tb, /)\n"
"--\n"
"\n"
"Exit from Token context manager, restore the linked ContextVar.");

#define TOKEN_EXIT_METHODDEF    \
    {"__exit__", _PyCFunction_CAST(token_exit), METH_VALUE, token_exit__doc__},

static PyValue
token_exit_impl(PyContextToken *self, PyValueBorrow type, PyValueBorrow val,
                PyValueBorrow tb);

static PyValue
token_exit(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow type;
    PyValueBorrow val;
    PyValueBorrow tb;

    if (!_PyArg_CheckPositional("__exit__", nargs, 3, 3)) {
        goto exit;
    }
    type = PyValue_Borrow(args[0]);
    val = PyValue_Borrow(args[1]);
    tb = PyValue_Borrow(args[2]);
    return_value = token_exit_impl((PyContextToken *)PyValue_AsKnownObjectBorrow(self), type, val, tb);

exit:
    return return_value;
}
/*[clinic end generated code: output=917a66deb851762a input=a9049054013a1b77]*/
