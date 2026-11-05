/*[clinic input]
preserve
[clinic start generated code]*/

#ifndef Py_VALUE_CORE
#  error "value-emission Clinic output requires Py_VALUE_CORE"
#endif

#include "pycore_modsupport.h"    // _PyArg_CheckPositional()
#include "pycore_value_call.h"    // METH_VALUE

PyDoc_STRVAR(namespace___replace____doc__,
"__replace__($self, /, **changes)\n"
"--\n"
"\n"
"Return a copy with the specified attributes replaced.");

#define NAMESPACE___REPLACE___METHODDEF    \
    {"__replace__", _PyCFunction_CAST(namespace___replace__), METH_VALUE|METH_KEYWORDS, namespace___replace____doc__},

static PyValue
namespace___replace___impl(_PyNamespaceObject *self, PyObject *changes);

static PyValue
namespace___replace__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *changes = NULL;

    if (!_PyArg_CheckPositional("__replace__", nargs, 0, 0)) {
        goto exit;
    }
    changes = kwnames == NULL ? PyDict_New()
        : _PyValue_StackAsDict(args + nargs, kwnames);
    if (changes == NULL) {
        goto exit;
    }
    return_value = namespace___replace___impl((_PyNamespaceObject *)PyValue_AsKnownObjectBorrow(self), changes);

exit:
    /* Cleanup for changes */
    Py_XDECREF(changes);

    return return_value;
}
/*[clinic end generated code: output=731349243f42e3e0 input=a9049054013a1b77]*/
