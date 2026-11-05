/*[clinic input]
preserve
[clinic start generated code]*/

#ifndef Py_VALUE_CORE
#  error "value-emission Clinic output requires Py_VALUE_CORE"
#endif

#include "pycore_modsupport.h"    // _PyArg_CheckArityValue()
#include "pycore_value_call.h"    // METH_VALUE

PyDoc_STRVAR(_contextvars_copy_context__doc__,
"copy_context($module, /)\n"
"--\n"
"\n");

#define _CONTEXTVARS_COPY_CONTEXT_METHODDEF    \
    {"copy_context", _PyCFunction_CAST(_contextvars_copy_context), METH_VALUE, _contextvars_copy_context__doc__},

static PyValue
_contextvars_copy_context_impl(PyObject *module);

static PyValue
_contextvars_copy_context(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValue("_contextvars.copy_context()", nargs, kwnames, 0)) {
        goto exit;
    }
    return_value = _contextvars_copy_context_impl(PyValue_AsKnownObjectBorrow(module));

exit:
    return return_value;
}
/*[clinic end generated code: output=bfa7f6f22f411e50 input=a9049054013a1b77]*/
