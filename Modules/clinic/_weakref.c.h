/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_modsupport.h"    // _PyArg_CheckPositional()

PyDoc_STRVAR(_weakref_getweakrefcount__doc__,
"getweakrefcount($module, object, /)\n"
"--\n"
"\n"
"Return the number of weak references to \'object\'.");

#define _WEAKREF_GETWEAKREFCOUNT_METHODDEF    \
    {"getweakrefcount", _PyCFunction_CAST(_weakref_getweakrefcount), METH_O, _weakref_getweakrefcount__doc__},

static Py_ssize_t
_weakref_getweakrefcount_impl(PyObject *module, PyObject *object);

static PyValue
_weakref_getweakrefcount(PyObject *module, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyObject *object;
    Py_ssize_t _return_value;

    object = PyValue_Object(arg);
    if (object == NULL) {
        goto exit;
    }
    _return_value = _weakref_getweakrefcount_impl(module, object);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyValue_FromLongLong((long long)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_weakref__remove_dead_weakref__doc__,
"_remove_dead_weakref($module, dct, key, /)\n"
"--\n"
"\n"
"Atomically remove key from dict if it points to a dead weakref.");

#define _WEAKREF__REMOVE_DEAD_WEAKREF_METHODDEF    \
    {"_remove_dead_weakref", _PyCFunction_CAST(_weakref__remove_dead_weakref), METH_FASTCALL, _weakref__remove_dead_weakref__doc__},

static PyObject *
_weakref__remove_dead_weakref_impl(PyObject *module, PyObject *dct,
                                   PyObject *key);

static PyValue
_weakref__remove_dead_weakref(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    PyObject *dct;
    PyObject *key;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("_remove_dead_weakref", nargs, 2, 2)) {
        goto exit;
    }
    {
        PyObject *_arg = PyValue_Object(args[0]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyDict_Check(_arg)) {
            _PyArg_BadArgument("_remove_dead_weakref", "argument 1", "dict", _arg);
            goto exit;
        }
        dct = _arg;
    }
    key = PyValue_Object(args[1]);
    if (key == NULL) {
        goto exit;
    }
    _return_value = _weakref__remove_dead_weakref_impl(module, dct, key);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_weakref_getweakrefs__doc__,
"getweakrefs($module, object, /)\n"
"--\n"
"\n"
"Return a list of all weak reference objects pointing to \'object\'.");

#define _WEAKREF_GETWEAKREFS_METHODDEF    \
    {"getweakrefs", _PyCFunction_CAST(_weakref_getweakrefs), METH_O, _weakref_getweakrefs__doc__},

static PyObject *
_weakref_getweakrefs_impl(PyObject *module, PyObject *object);

static PyValue
_weakref_getweakrefs(PyObject *module, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyObject *object;
    PyObject *_return_value;

    object = PyValue_Object(arg);
    if (object == NULL) {
        goto exit;
    }
    _return_value = _weakref_getweakrefs_impl(module, object);
    return_value = PyValue_FromResult(_return_value);

exit:
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
    {"proxy", _PyCFunction_CAST(_weakref_proxy), METH_FASTCALL, _weakref_proxy__doc__},

static PyObject *
_weakref_proxy_impl(PyObject *module, PyObject *object, PyObject *callback);

static PyValue
_weakref_proxy(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    PyObject *object;
    PyObject *callback = NULL;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("proxy", nargs, 1, 2)) {
        goto exit;
    }
    object = PyValue_Object(args[0]);
    if (object == NULL) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    callback = PyValue_Object(args[1]);
    if (callback == NULL) {
        goto exit;
    }
skip_optional:
    _return_value = _weakref_proxy_impl(module, object, callback);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}
/*[clinic end generated code: output=8b9bb4cb647ea6cc input=a9049054013a1b77]*/
