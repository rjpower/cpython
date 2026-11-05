/*[clinic input]
preserve
[clinic start generated code]*/

#ifndef Py_VALUE_CORE
#  error "value-emission Clinic output requires Py_VALUE_CORE"
#endif

#include "pycore_critical_section.h"// Py_BEGIN_CRITICAL_SECTION()
#include "pycore_modsupport.h"    // _PyArg_CheckPositional()
#include "pycore_value_call.h"    // METH_VALUE

PyDoc_STRVAR(_suggestions__generate_suggestions__doc__,
"_generate_suggestions($module, candidates, item, /)\n"
"--\n"
"\n"
"Returns the candidate in candidates that\'s closest to item");

#define _SUGGESTIONS__GENERATE_SUGGESTIONS_METHODDEF    \
    {"_generate_suggestions", _PyCFunction_CAST(_suggestions__generate_suggestions), METH_VALUE, _suggestions__generate_suggestions__doc__},

static PyValue
_suggestions__generate_suggestions_impl(PyObject *module,
                                        PyValueBorrow candidates,
                                        PyObject *item);

static PyValue
_suggestions__generate_suggestions(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow candidates;
    PyObject *item;

    if (!_PyArg_CheckPositional("_generate_suggestions", nargs, 2, 2)) {
        goto exit;
    }
    candidates = PyValue_Borrow(args[0]);
    if (!PyValue_IsObject(PyValue_Borrow(args[1]))) {
        _PyArg_BadArgumentValue("_generate_suggestions", "argument 2", "str", PyValue_Borrow(args[1]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])))) {
        _PyArg_BadArgument("_generate_suggestions", "argument 2", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        goto exit;
    }
    item = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1]));
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(candidates));
    return_value = _suggestions__generate_suggestions_impl(PyValue_AsKnownObjectBorrow(module), candidates, item);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}
/*[clinic end generated code: output=cdcbcde74c55edba input=a9049054013a1b77]*/
