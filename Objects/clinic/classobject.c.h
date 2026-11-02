/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_modsupport.h"    // _PyArg_CheckPositional()
#include "pycore_value.h"         // _PyValue_RestoreException()

PyDoc_STRVAR(method___reduce____doc__,
"__reduce__($self, /)\n"
"--\n"
"\n");

#define METHOD___REDUCE___METHODDEF    \
    {"__reduce__", (PyCFunction)method___reduce__, METH_NOARGS, method___reduce____doc__},

static PyObject *
method___reduce___impl(PyMethodObject *self);

static PyObject *
method___reduce__(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return method___reduce___impl((PyMethodObject *)self);
}

PyDoc_STRVAR(method_new__doc__,
"method(function, instance, /)\n"
"--\n"
"\n"
"Create a bound instance method object.");

static PyObject *
method_new_impl(PyTypeObject *type, PyObject *function, PyObject *instance);

static PyObject *
method_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    PyTypeObject *base_tp = &PyMethod_Type;
    _PyArg_OwnerScope _args_scope = {0};
    PyObject *const *fastargs;
    PyObject *function;
    PyObject *instance;

    if ((type == base_tp || type->tp_init == base_tp->tp_init) &&
        !_PyArg_NoKeywords("method", kwargs)) {
        goto exit;
    }
    if (_PyArg_OwnerScope_BeginTuple(&_args_scope, args, 0) < 0) {
        goto exit;
    }
    fastargs = _PyArg_UnpackPositionalOwned(&_args_scope, "method",
                                          2, 2);
    if (fastargs == NULL) {
        goto exit;
    }
    function = fastargs[0];
    instance = fastargs[1];
    return_value = method_new_impl(type, function, instance);

exit:
    if (_PyArg_OwnerScope_End(&_args_scope) < 0) {
        PyObject *_saved = PyErr_GetRaisedException();
        Py_CLEAR(return_value);
        _PyValue_RestoreException(_saved);
    }
    return return_value;
}

PyDoc_STRVAR(instancemethod_new__doc__,
"instancemethod(function, /)\n"
"--\n"
"\n"
"Bind a function to a class.");

static PyObject *
instancemethod_new_impl(PyTypeObject *type, PyObject *function);

static PyObject *
instancemethod_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    PyTypeObject *base_tp = &PyInstanceMethod_Type;
    _PyArg_OwnerScope _args_scope = {0};
    PyObject *const *fastargs;
    PyObject *function;

    if ((type == base_tp || type->tp_init == base_tp->tp_init) &&
        !_PyArg_NoKeywords("instancemethod", kwargs)) {
        goto exit;
    }
    if (_PyArg_OwnerScope_BeginTuple(&_args_scope, args, 0) < 0) {
        goto exit;
    }
    fastargs = _PyArg_UnpackPositionalOwned(&_args_scope, "instancemethod",
                                          1, 1);
    if (fastargs == NULL) {
        goto exit;
    }
    function = fastargs[0];
    return_value = instancemethod_new_impl(type, function);

exit:
    if (_PyArg_OwnerScope_End(&_args_scope) < 0) {
        PyObject *_saved = PyErr_GetRaisedException();
        Py_CLEAR(return_value);
        _PyValue_RestoreException(_saved);
    }
    return return_value;
}
/*[clinic end generated code: output=62721b5270bcff0d input=a9049054013a1b77]*/
