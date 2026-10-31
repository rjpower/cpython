/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(_testcapi_call_long_compact_api__doc__,
"call_long_compact_api($module, arg, /)\n"
"--\n"
"\n");

#define _TESTCAPI_CALL_LONG_COMPACT_API_METHODDEF    \
    {"call_long_compact_api", _PyCFunction_CAST(_testcapi_call_long_compact_api), METH_O, _testcapi_call_long_compact_api__doc__},

static PyObject *
_testcapi_call_long_compact_api_impl(PyObject *module, PyObject *arg);

static PyValue
_testcapi_call_long_compact_api(PyObject *module, PyValue arg_)
{
    PyValue return_value = PyValue_NULL;
    PyObject *arg;
    PyObject *_return_value;

    arg = PyValue_Object(arg_);
    if (arg == NULL) {
        goto exit;
    }
    _return_value = _testcapi_call_long_compact_api_impl(module, arg);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}
/*[clinic end generated code: output=5137b699e2d38f10 input=a9049054013a1b77]*/
