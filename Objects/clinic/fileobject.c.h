/*[clinic input]
preserve
[clinic start generated code]*/

#ifndef Py_VALUE_CORE
#  error "value-emission Clinic output requires Py_VALUE_CORE"
#endif

#include "pycore_modsupport.h"    // _PyArg_BadArgument()
#include "pycore_value_call.h"    // METH_VALUE

PyDoc_STRVAR(stdprinter_write__doc__,
"write($self, text, /)\n"
"--\n"
"\n"
"Write the text to the stream.");

#define STDPRINTER_WRITE_METHODDEF    \
    {"write", _PyCFunction_CAST(stdprinter_write), METH_VALUE|METH_O, stdprinter_write__doc__},

static PyValue
stdprinter_write_impl(PyStdPrinter_Object *self, PyObject *unicode);

static PyValue
stdprinter_write(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *unicode;

    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("write", "argument", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("write", "argument", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    unicode = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    return_value = stdprinter_write_impl((PyStdPrinter_Object *)PyValue_AsKnownObjectBorrow(self), unicode);

exit:
    return return_value;
}
/*[clinic end generated code: output=02a8016f9089fd45 input=a9049054013a1b77]*/
