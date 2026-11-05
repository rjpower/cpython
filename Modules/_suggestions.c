#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif
#define Py_VALUE_CORE 1
#include "Python.h"
#include "pycore_pyerrors.h"
#include "pycore_value.h"         // PyValue_IsObject()
#include "pycore_value_slots.h"// _PyValue_FromObjectResult()
#include "clinic/_suggestions.c.h"

/*[clinic input]
value_emission
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=eb7b17088bfd7215]*/

/*[clinic input]
module _suggestions
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=e58d81fafad5637b]*/

/*[clinic input]
@critical_section candidates
_suggestions._generate_suggestions
    candidates: object
    item: unicode
    /
Returns the candidate in candidates that's closest to item
[clinic start generated code]*/

static PyValue
_suggestions__generate_suggestions_impl(PyObject *module,
                                        PyValueBorrow candidates,
                                        PyObject *item)
/*[clinic end generated code: output=3420532c307ed540 input=92861a6c9bd8f667]*/
{
   // Check if dir is a list
    if (!(PyValue_IsObject(candidates) && PyList_CheckExact(PyValue_AsKnownObjectBorrow(candidates)))) {
        PyErr_SetString(PyExc_TypeError, "candidates must be a list");
        return PyValue_NULL;
    }
    PyObject *list = PyValue_AsKnownObjectBorrow(candidates);

    // Check if all elements in the list are Unicode
    Py_ssize_t size = PyList_Size(list);
    for (Py_ssize_t i = 0; i < size; ++i) {
        PyValue elem = PyList_GetValue(list, i);
        if (PyValue_IsNull(elem)) {
            return PyValue_NULL;
        }
        int is_str = (PyValue_IsObject(elem)
                      && PyUnicode_Check(PyValue_AsKnownObjectBorrow(elem)));
        PyValue_CLOSE(elem);
        if (!is_str) {
            PyErr_SetString(PyExc_TypeError, "all elements in 'candidates' must be strings");
            return PyValue_NULL;
        }
    }

    PyObject* result =  _Py_CalculateSuggestions(list, item);
    if (!result && !PyErr_Occurred()) {
        return PyValue_None;
    }
    return _PyValue_FromObjectResult(result);
}


static PyMethodDef module_methods[] = {
    _SUGGESTIONS__GENERATE_SUGGESTIONS_METHODDEF
    {NULL, NULL, 0, NULL} // Sentinel
};

static PyModuleDef_Slot module_slots[] = {
    _Py_ABI_SLOT,
    {Py_mod_multiple_interpreters, Py_MOD_MULTIPLE_INTERPRETERS_NOT_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL},
};

static struct PyModuleDef suggestions_module = {
    PyModuleDef_HEAD_INIT,
    "_suggestions",
    NULL,
    0,
    module_methods,
    module_slots,
};

PyMODINIT_FUNC PyInit__suggestions(void) {
    return PyModuleDef_Init(&suggestions_module);
}
