/*[clinic input]
preserve
[clinic start generated code]*/

#ifndef Py_VALUE_CORE
#  error "value-emission Clinic output requires Py_VALUE_CORE"
#endif

#include "pycore_modsupport.h"    // _PyArg_NoKwnamesValue()
#include "pycore_value_call.h"    // METH_VALUE

PyDoc_STRVAR(hamt_set__doc__,
"set($self, key, val, /)\n"
"--\n"
"\n"
"Return a copy of the mapping with the key set to the value.");

#define HAMT_SET_METHODDEF    \
    {"set", _PyCFunction_CAST(hamt_set), METH_VALUE, hamt_set__doc__},

static PyValue
hamt_set_impl(PyHamtObject *self, PyValueBorrow key, PyValueBorrow val);

static PyValue
hamt_set(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow key;
    PyValueBorrow val;

    if (!_PyArg_NoKwnamesValueMethod(&_PyHamt_Type, "set", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("set", nargs, 2, 2)) {
        goto exit;
    }
    key = PyValue_Borrow(args[0]);
    val = PyValue_Borrow(args[1]);
    return_value = hamt_set_impl((PyHamtObject *)PyValue_AsKnownObjectBorrow(self), key, val);

exit:
    return return_value;
}

PyDoc_STRVAR(hamt_get__doc__,
"get($self, key, default=None, /)\n"
"--\n"
"\n"
"Return the value for the key, or the default if it is not found.");

#define HAMT_GET_METHODDEF    \
    {"get", _PyCFunction_CAST(hamt_get), METH_VALUE, hamt_get__doc__},

static PyValue
hamt_get_impl(PyHamtObject *self, PyValueBorrow key, PyValueBorrow def);

static PyValue
hamt_get(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow key;
    PyValueBorrow def = PyValue_Borrow(PyValue_None);

    if (!_PyArg_NoKwnamesValueMethod(&_PyHamt_Type, "get", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("get", nargs, 1, 2)) {
        goto exit;
    }
    key = PyValue_Borrow(args[0]);
    if (nargs < 2) {
        goto skip_optional;
    }
    def = PyValue_Borrow(args[1]);
skip_optional:
    return_value = hamt_get_impl((PyHamtObject *)PyValue_AsKnownObjectBorrow(self), key, def);

exit:
    return return_value;
}
/*[clinic end generated code: output=b2b1beefa345f1f0 input=a9049054013a1b77]*/
