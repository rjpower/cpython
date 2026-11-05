/*[clinic input]
preserve
[clinic start generated code]*/

#ifndef Py_VALUE_CORE
#  error "value-emission Clinic output requires Py_VALUE_CORE"
#endif

#include "pycore_modsupport.h"    // _PyArg_CheckPositional()
#include "pycore_tuple.h"         // _PyTuple_FromValueArray()
#include "pycore_value_call.h"    // METH_VALUE

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
    {"register", _PyCFunction_CAST(atexit_register), METH_VALUE|METH_KEYWORDS, atexit_register__doc__},

static PyValue
atexit_register_impl(PyObject *module, PyValueBorrow func, PyObject *args,
                     PyObject *kwargs);

static PyValue
atexit_register(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow func;
    PyObject *__clinic_args = NULL;
    PyObject *__clinic_kwargs = NULL;

    if (!_PyArg_CheckPositional("register", nargs, 1, PY_SSIZE_T_MAX)) {
        goto exit;
    }
    func = PyValue_Borrow(args[0]);
    __clinic_args = _PyTuple_FromValueArray(args + 1,
                                          Py_MAX(nargs - 1, 0));
    if (__clinic_args == NULL) {
        goto exit;
    }
    __clinic_kwargs = kwnames == NULL ? PyDict_New()
        : _PyValue_StackAsDict(args + nargs, kwnames);
    if (__clinic_kwargs == NULL) {
        goto exit;
    }
    return_value = atexit_register_impl(PyValue_AsKnownObjectBorrow(module), func, __clinic_args, __clinic_kwargs);

exit:
    /* Cleanup for args */
    Py_XDECREF(__clinic_args);
    /* Cleanup for kwargs */
    Py_XDECREF(__clinic_kwargs);

    return return_value;
}
/*[clinic end generated code: output=6b3dba32b3541e52 input=a9049054013a1b77]*/
