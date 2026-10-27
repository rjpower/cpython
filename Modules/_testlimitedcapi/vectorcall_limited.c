/* Test Vectorcall in the limited API.  A vectorcall function takes values,
   which the limited API cannot express, so only calls out are tested. */

// Need limited C API version 3.12 for PyObject_Vectorcall()
#define Py_LIMITED_API 0x030c0000

#include "parts.h"
#include "clinic/vectorcall_limited.c.h"

/*[clinic input]
module _testlimitedcapi
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=2700057f9c1135ba]*/

/*[clinic input]
_testlimitedcapi.call_vectorcall

    callable: object
    /
[clinic start generated code]*/

static PyObject *
_testlimitedcapi_call_vectorcall(PyObject *module, PyObject *callable)
/*[clinic end generated code: output=9cbb7832263a8eef input=0743636c12dccb28]*/
{
    PyObject *args[3] = { NULL, NULL, NULL };
    PyObject *kwname = NULL, *kwnames = NULL, *result = NULL;

    args[1] = PyUnicode_FromString("foo");
    if (!args[1]) {
        goto leave;
    }

    args[2] = PyUnicode_FromString("bar");
    if (!args[2]) {
        goto leave;
    }

    kwname = PyUnicode_InternFromString("baz");
    if (!kwname) {
        goto leave;
    }

    kwnames = PyTuple_New(1);
    if (!kwnames) {
        goto leave;
    }

    if (PyTuple_SetItem(kwnames, 0, kwname)) {
        goto leave;
    }

    result = PyObject_Vectorcall(
        callable,
        args + 1,
        1 | PY_VECTORCALL_ARGUMENTS_OFFSET,
        kwnames
    );

leave:
    Py_XDECREF(args[1]);
    Py_XDECREF(args[2]);
    Py_XDECREF(kwnames);

    return result;
}

/*[clinic input]
_testlimitedcapi.call_vectorcall_method

    callable: object
    /
[clinic start generated code]*/

static PyObject *
_testlimitedcapi_call_vectorcall_method(PyObject *module, PyObject *callable)
/*[clinic end generated code: output=4558323a46cc09eb input=a736f7dbf15f1be5]*/
{
    PyObject *args[3] = { NULL, NULL, NULL };
    PyObject *name = NULL, *kwname = NULL,
             *kwnames = NULL, *result = NULL;

    name = PyUnicode_FromString("f");
    if (!name) {
        goto leave;
    }

    args[0] = callable;
    args[1] = PyUnicode_FromString("foo");
    if (!args[1]) {
        goto leave;
    }

    args[2] = PyUnicode_FromString("bar");
    if (!args[2]) {
        goto leave;
    }

    kwname = PyUnicode_InternFromString("baz");
    if (!kwname) {
        goto leave;
    }

    kwnames = PyTuple_New(1);
    if (!kwnames) {
        goto leave;
    }

    if (PyTuple_SetItem(kwnames, 0, kwname)) {
        goto leave;
    }


    result = PyObject_VectorcallMethod(
        name,
        args,
        2 | PY_VECTORCALL_ARGUMENTS_OFFSET,
        kwnames
    );

leave:
    Py_XDECREF(name);
    Py_XDECREF(args[1]);
    Py_XDECREF(args[2]);
    Py_XDECREF(kwnames);

    return result;
}

static PyMethodDef TestMethods[] = {
    _TESTLIMITEDCAPI_CALL_VECTORCALL_METHODDEF
    _TESTLIMITEDCAPI_CALL_VECTORCALL_METHOD_METHODDEF
    {NULL},
};

int
_PyTestLimitedCAPI_Init_VectorcallLimited(PyObject *m)
{
    if (PyModule_AddFunctions(m, TestMethods) < 0) {
        return -1;
    }

    return 0;
}
