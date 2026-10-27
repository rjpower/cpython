/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_modsupport.h"    // _PyArg_BadArgument()

PyDoc_STRVAR(stdprinter_write__doc__,
"write($self, text, /)\n"
"--\n"
"\n"
"Write the text to the stream.");

#define STDPRINTER_WRITE_METHODDEF    \
    {"write", _PyCFunction_CAST(stdprinter_write), METH_O, stdprinter_write__doc__},

static PyObject *
stdprinter_write_impl(PyStdPrinter_Object *self, PyObject *unicode);

static PyValue
stdprinter_write(PyObject *self, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyObject *unicode;
    PyObject *_return_value;

    {
        PyObject *_arg = PyValue_Object(arg);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("write", "argument", "str", _arg);
            goto exit;
        }
        unicode = _arg;
    }
    _return_value = stdprinter_write_impl((PyStdPrinter_Object *)self, unicode);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}
/*[clinic end generated code: output=ecd766dad66e5176 input=a9049054013a1b77]*/
