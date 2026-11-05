/*[clinic input]
preserve
[clinic start generated code]*/

#ifndef Py_VALUE_CORE
#  error "value-emission Clinic output requires Py_VALUE_CORE"
#endif

#include "pycore_value_call.h"    // METH_VALUE

PyDoc_STRVAR(_typing__idfunc__doc__,
"_idfunc($module, x, /)\n"
"--\n"
"\n");

#define _TYPING__IDFUNC_METHODDEF    \
    {"_idfunc", _PyCFunction_CAST(_typing__idfunc), METH_VALUE|METH_O, _typing__idfunc__doc__},

static PyValue
_typing__idfunc_impl(PyObject *module, PyValueBorrow x);

static PyValue
_typing__idfunc(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow x;

    x = PyValue_Borrow(args[0]);
    return_value = _typing__idfunc_impl(PyValue_AsKnownObjectBorrow(module), x);

    return return_value;
}
/*[clinic end generated code: output=cd11e5348136673b input=a9049054013a1b77]*/
