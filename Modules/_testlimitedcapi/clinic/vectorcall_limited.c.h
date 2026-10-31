/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(_testlimitedcapi_call_vectorcall__doc__,
"call_vectorcall($module, callable, /)\n"
"--\n"
"\n");

#define _TESTLIMITEDCAPI_CALL_VECTORCALL_METHODDEF    \
    {"call_vectorcall", _PyCFunction_CAST(_testlimitedcapi_call_vectorcall), METH_O, _testlimitedcapi_call_vectorcall__doc__},

static PyObject *
_testlimitedcapi_call_vectorcall_impl(PyObject *module, PyObject *callable);

static PyValue
_testlimitedcapi_call_vectorcall(PyObject *module, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyObject *callable;
    PyObject *_return_value;

    callable = PyValue_Object(arg);
    if (callable == NULL) {
        goto exit;
    }
    _return_value = _testlimitedcapi_call_vectorcall_impl(module, callable);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_testlimitedcapi_call_vectorcall_method__doc__,
"call_vectorcall_method($module, callable, /)\n"
"--\n"
"\n");

#define _TESTLIMITEDCAPI_CALL_VECTORCALL_METHOD_METHODDEF    \
    {"call_vectorcall_method", _PyCFunction_CAST(_testlimitedcapi_call_vectorcall_method), METH_O, _testlimitedcapi_call_vectorcall_method__doc__},

static PyObject *
_testlimitedcapi_call_vectorcall_method_impl(PyObject *module,
                                             PyObject *callable);

static PyValue
_testlimitedcapi_call_vectorcall_method(PyObject *module, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyObject *callable;
    PyObject *_return_value;

    callable = PyValue_Object(arg);
    if (callable == NULL) {
        goto exit;
    }
    _return_value = _testlimitedcapi_call_vectorcall_method_impl(module, callable);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}
/*[clinic end generated code: output=0a3abefe139f2daa input=a9049054013a1b77]*/
