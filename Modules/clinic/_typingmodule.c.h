/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(_typing__idfunc__doc__,
"_idfunc($module, x, /)\n"
"--\n"
"\n");

#define _TYPING__IDFUNC_METHODDEF    \
    {"_idfunc", _PyCFunction_CAST(_typing__idfunc), METH_O, _typing__idfunc__doc__},

static PyObject *
_typing__idfunc_impl(PyObject *module, PyObject *x);

static PyValue
_typing__idfunc(PyObject *module, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyObject *x;
    PyObject *_return_value;

    x = PyValue_Object(arg);
    if (x == NULL) {
        goto exit;
    }
    _return_value = _typing__idfunc_impl(module, x);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}
/*[clinic end generated code: output=d8a90c0fbe328962 input=a9049054013a1b77]*/
