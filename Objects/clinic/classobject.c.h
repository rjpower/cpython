/*[clinic input]
preserve
[clinic start generated code]*/

#ifndef Py_VALUE_CORE
#  error "value-emission Clinic output requires Py_VALUE_CORE"
#endif

#include "pycore_modsupport.h"    // _PyArg_CheckArityValue()
#include "pycore_tuple.h"         // _PyTuple_ITEMS()
#include "pycore_value_call.h"    // METH_VALUE
#include "pycore_value_slots.h"   // _PyValue_THUNK_EXIT()

PyDoc_STRVAR(method___reduce____doc__,
"__reduce__($self, /)\n"
"--\n"
"\n");

#define METHOD___REDUCE___METHODDEF    \
    {"__reduce__", _PyCFunction_CAST(method___reduce__), METH_VALUE, method___reduce____doc__},

static PyValue
method___reduce___impl(PyMethodObject *self);

static PyValue
method___reduce__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValueMethod(&PyMethod_Type, "__reduce__", nargs, kwnames, 0)) {
        goto exit;
    }
    return_value = method___reduce___impl((PyMethodObject *)PyValue_AsKnownObjectBorrow(self));

exit:
    return return_value;
}

PyDoc_STRVAR(method_new__doc__,
"method(function, instance, /)\n"
"--\n"
"\n"
"Create a bound instance method object.");

static PyValue
method_new_impl(PyTypeObject *type, PyValueBorrow function,
                PyValueBorrow instance);

static PyValue
method_new_value(PyTypeObject *type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyTypeObject *base_tp = &PyMethod_Type;
    PyValueBorrow function;
    PyValueBorrow instance;

    if ((type == base_tp || type->tp_init == base_tp->tp_init) &&
        !_PyArg_NoKwnames("method", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("method", nargs, 2, 2)) {
        goto exit;
    }
    function = PyValue_Borrow(args[0]);
    instance = PyValue_Borrow(args[1]);
    return_value = method_new_impl(type, function, instance);

exit:
    return return_value;
}

Py_VALUE_ENTRY static PyObject *
method_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _PyValue_THUNK_EXIT(method_new_value(type, _PyTuple_ITEMS(args), nargs, NULL), BOX_SITE_CALL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return NULL;
    }
    PyValue result = method_new_value(type, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return _PyValue_THUNK_EXIT(result, BOX_SITE_CALL);
}

PyDoc_STRVAR(instancemethod_new__doc__,
"instancemethod(function, /)\n"
"--\n"
"\n"
"Bind a function to a class.");

static PyValue
instancemethod_new_impl(PyTypeObject *type, PyValueBorrow function);

static PyValue
instancemethod_new_value(PyTypeObject *type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyTypeObject *base_tp = &PyInstanceMethod_Type;
    PyValueBorrow function;

    if ((type == base_tp || type->tp_init == base_tp->tp_init) &&
        !_PyArg_NoKwnames("instancemethod", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("instancemethod", nargs, 1, 1)) {
        goto exit;
    }
    function = PyValue_Borrow(args[0]);
    return_value = instancemethod_new_impl(type, function);

exit:
    return return_value;
}

Py_VALUE_ENTRY static PyObject *
instancemethod_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _PyValue_THUNK_EXIT(instancemethod_new_value(type, _PyTuple_ITEMS(args), nargs, NULL), BOX_SITE_CALL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return NULL;
    }
    PyValue result = instancemethod_new_value(type, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return _PyValue_THUNK_EXIT(result, BOX_SITE_CALL);
}
/*[clinic end generated code: output=c3aa9bd587ee8272 input=a9049054013a1b77]*/
