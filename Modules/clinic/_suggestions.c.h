/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_critical_section.h"// Py_BEGIN_CRITICAL_SECTION()
#include "pycore_modsupport.h"    // _PyArg_CheckPositional()

PyDoc_STRVAR(_suggestions__generate_suggestions__doc__,
"_generate_suggestions($module, candidates, item, /)\n"
"--\n"
"\n"
"Returns the candidate in candidates that\'s closest to item");

#define _SUGGESTIONS__GENERATE_SUGGESTIONS_METHODDEF    \
    {"_generate_suggestions", _PyCFunction_CAST(_suggestions__generate_suggestions), METH_FASTCALL, _suggestions__generate_suggestions__doc__},

static PyObject *
_suggestions__generate_suggestions_impl(PyObject *module,
                                        PyObject *candidates, PyObject *item);

static PyValue
_suggestions__generate_suggestions(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    PyObject *candidates;
    PyObject *item;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("_generate_suggestions", nargs, 2, 2)) {
        goto exit;
    }
    candidates = PyValue_Object(args[0]);
    if (candidates == NULL) {
        goto exit;
    }
    {
        PyObject *_arg = PyValue_Object(args[1]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("_generate_suggestions", "argument 2", "str", _arg);
            goto exit;
        }
        item = _arg;
    }
    Py_BEGIN_CRITICAL_SECTION(candidates);
    _return_value = _suggestions__generate_suggestions_impl(module, candidates, item);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}
/*[clinic end generated code: output=0d7b44af1d09a9a4 input=a9049054013a1b77]*/
