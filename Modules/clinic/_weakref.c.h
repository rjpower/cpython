/*[clinic input]
preserve
[clinic start generated code]*/

#ifndef Py_VALUE_CORE
#  error "value-emission Clinic output requires Py_VALUE_CORE"
#endif

#include "pycore_modsupport.h"    // _PyArg_CheckPositional()
#include "pycore_value_call.h"    // METH_VALUE

PyDoc_STRVAR(_weakref_getweakrefcount__doc__,
"getweakrefcount($module, object, /)\n"
"--\n"
"\n"
"Return the number of weak references to \'object\'.");

#define _WEAKREF_GETWEAKREFCOUNT_METHODDEF    \
    {"getweakrefcount", _PyCFunction_CAST(_weakref_getweakrefcount), METH_VALUE|METH_O, _weakref_getweakrefcount__doc__},

static Py_ssize_t
_weakref_getweakrefcount_impl(PyObject *module, PyValueBorrow object);

static PyValue
_weakref_getweakrefcount(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow object;
    Py_ssize_t _return_value;

    object = PyValue_Borrow(args[0]);
    _return_value = _weakref_getweakrefcount_impl(PyValue_AsKnownObjectBorrow(module), object);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyValue_FromInt64((int64_t)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_weakref__remove_dead_weakref__doc__,
"_remove_dead_weakref($module, dct, key, /)\n"
"--\n"
"\n"
"Atomically remove key from dict if it points to a dead weakref.");

#define _WEAKREF__REMOVE_DEAD_WEAKREF_METHODDEF    \
    {"_remove_dead_weakref", _PyCFunction_CAST(_weakref__remove_dead_weakref), METH_VALUE, _weakref__remove_dead_weakref__doc__},

static PyValue
_weakref__remove_dead_weakref_impl(PyObject *module, PyObject *dct,
                                   PyValueBorrow key);

static PyValue
_weakref__remove_dead_weakref(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *dct;
    PyValueBorrow key;

    if (!_PyArg_CheckPositional("_remove_dead_weakref", nargs, 2, 2)) {
        goto exit;
    }
    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[0])), &PyDict_Type)) {
        _PyArg_BadArgumentValue("_remove_dead_weakref", "argument 1", "dict", PyValue_Borrow(args[0]));
        goto exit;
    }
    dct = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    key = PyValue_Borrow(args[1]);
    return_value = _weakref__remove_dead_weakref_impl(PyValue_AsKnownObjectBorrow(module), dct, key);

exit:
    return return_value;
}

PyDoc_STRVAR(_weakref_getweakrefs__doc__,
"getweakrefs($module, object, /)\n"
"--\n"
"\n"
"Return a list of all weak reference objects pointing to \'object\'.");

#define _WEAKREF_GETWEAKREFS_METHODDEF    \
    {"getweakrefs", _PyCFunction_CAST(_weakref_getweakrefs), METH_VALUE|METH_O, _weakref_getweakrefs__doc__},

static PyValue
_weakref_getweakrefs_impl(PyObject *module, PyValueBorrow object);

static PyValue
_weakref_getweakrefs(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow object;

    object = PyValue_Borrow(args[0]);
    return_value = _weakref_getweakrefs_impl(PyValue_AsKnownObjectBorrow(module), object);

    return return_value;
}

PyDoc_STRVAR(_weakref_proxy__doc__,
"proxy($module, object, callback=None, /)\n"
"--\n"
"\n"
"Create a proxy object that weakly references \'object\'.\n"
"\n"
"\'callback\', if given, is called with a reference to the\n"
"proxy when \'object\' is about to be finalized.");

#define _WEAKREF_PROXY_METHODDEF    \
    {"proxy", _PyCFunction_CAST(_weakref_proxy), METH_VALUE, _weakref_proxy__doc__},

static PyValue
_weakref_proxy_impl(PyObject *module, PyValueBorrow object,
                    PyValueBorrow callback);

static PyValue
_weakref_proxy(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow object;
    PyValueBorrow callback = PyValue_Borrow(PyValue_NULL);

    if (!_PyArg_CheckPositional("proxy", nargs, 1, 2)) {
        goto exit;
    }
    object = PyValue_Borrow(args[0]);
    if (nargs < 2) {
        goto skip_optional;
    }
    callback = PyValue_Borrow(args[1]);
skip_optional:
    return_value = _weakref_proxy_impl(PyValue_AsKnownObjectBorrow(module), object, callback);

exit:
    return return_value;
}
/*[clinic end generated code: output=2e1ca427d24fb7ad input=a9049054013a1b77]*/
