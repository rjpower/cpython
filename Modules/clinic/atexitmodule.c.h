/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_modsupport.h"    // _PyArg_CheckPositional()
#include "pycore_value.h"         // _PyValue_RestoreException()

PyDoc_STRVAR(atexit_register__doc__,
"register($module, func, /, *args, **kwargs)\n"
"--\n"
"\n"
"Register a function to be executed upon normal program termination\n"
"\n"
"    func - function to be called at exit\n"
"    args - optional arguments to pass to func\n"
"    kwargs - optional keyword arguments to pass to func\n"
"\n"
"    func is returned to facilitate usage as a decorator.");

#define ATEXIT_REGISTER_METHODDEF    \
    {"register", _PyCFunction_CAST(atexit_register), METH_VARARGS|METH_KEYWORDS, atexit_register__doc__},

static PyObject *
atexit_register_impl(PyObject *module, PyObject *func, PyObject *args,
                     PyObject *kwargs);

static PyObject *
atexit_register(PyObject *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    PyObject *_clinic_receiver_owner = NULL;
    PyObject *_clinic_item_owner = NULL;
    PyObject *func;
    PyObject *__clinic_args = NULL;
    PyObject *__clinic_kwargs = NULL;

    if (PyErr_Occurred()) {
        goto exit;
    }
    _clinic_receiver_owner = Py_NewRef((PyObject *)module);
    if (!_PyArg_CheckPositional("register", PyTuple_GET_SIZE(args), 1, PY_SSIZE_T_MAX)) {
        goto exit;
    }
    _clinic_item_owner = PyTuple_GetItemRef(args, 0);
    if (_clinic_item_owner == NULL || PyErr_Occurred()) {
        goto exit;
    }
    func = _clinic_item_owner;
    __clinic_args = PyTuple_GetSlice(args, 1, PY_SSIZE_T_MAX);
    if (!__clinic_args) {
        goto exit;
    }
    if (PyErr_Occurred()) {
        goto exit;
    }
    if (kwargs == NULL) {
        __clinic_kwargs = PyDict_New();
        if (__clinic_kwargs == NULL) {
            goto exit;
        }
    }
    else {
        __clinic_kwargs = Py_NewRef(kwargs);
    }
    if (PyErr_Occurred()) {
        goto exit;
    }
    return_value = atexit_register_impl(module, func, __clinic_args, __clinic_kwargs);

exit:
    {
        PyObject *_clinic_detached_owner = __clinic_args;
        __clinic_args = NULL;
        PyObject *_clinic_saved_exception = PyErr_GetRaisedException();
        Py_XDECREF(_clinic_detached_owner);
        _PyValue_RestoreException(_clinic_saved_exception);
    }
    {
        PyObject *_clinic_detached_owner = __clinic_kwargs;
        __clinic_kwargs = NULL;
        PyObject *_clinic_saved_exception = PyErr_GetRaisedException();
        Py_XDECREF(_clinic_detached_owner);
        _PyValue_RestoreException(_clinic_saved_exception);
    }
    {
        PyObject *_clinic_detached_owner = _clinic_item_owner;
        _clinic_item_owner = NULL;
        PyObject *_clinic_saved_exception = PyErr_GetRaisedException();
        Py_XDECREF(_clinic_detached_owner);
        _PyValue_RestoreException(_clinic_saved_exception);
    }
    {
        PyObject *_clinic_detached_owner = _clinic_receiver_owner;
        _clinic_receiver_owner = NULL;
        PyObject *_clinic_saved_exception = PyErr_GetRaisedException();
        Py_XDECREF(_clinic_detached_owner);
        _PyValue_RestoreException(_clinic_saved_exception);
    }
    if (PyErr_Occurred()) {
        PyObject *_clinic_output = return_value;
        return_value = NULL;
        PyObject *_clinic_saved_exception = PyErr_GetRaisedException();
        Py_XDECREF(_clinic_output);
        _PyValue_RestoreException(_clinic_saved_exception);
        return NULL;
    }
    return return_value;
}
/*[clinic end generated code: output=a6a3ef41052bbf93 input=a9049054013a1b77]*/
