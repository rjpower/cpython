/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_modsupport.h"    // _PyArg_CheckPositional()

PyDoc_STRVAR(hamt_set__doc__,
"set($self, key, val, /)\n"
"--\n"
"\n"
"Return a copy of the mapping with the key set to the value.");

#define HAMT_SET_METHODDEF    \
    {"set", _PyCFunction_CAST(hamt_set), METH_FASTCALL, hamt_set__doc__},

static PyObject *
hamt_set_impl(PyHamtObject *self, PyObject *key, PyObject *val);

static PyValue
hamt_set(PyObject *self, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    PyObject *key;
    PyObject *val;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("set", nargs, 2, 2)) {
        goto exit;
    }
    key = PyValue_Object(args[0]);
    if (key == NULL) {
        goto exit;
    }
    val = PyValue_Object(args[1]);
    if (val == NULL) {
        goto exit;
    }
    _return_value = hamt_set_impl((PyHamtObject *)self, key, val);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(hamt_get__doc__,
"get($self, key, default=None, /)\n"
"--\n"
"\n"
"Return the value for the key, or the default if it is not found.");

#define HAMT_GET_METHODDEF    \
    {"get", _PyCFunction_CAST(hamt_get), METH_FASTCALL, hamt_get__doc__},

static PyObject *
hamt_get_impl(PyHamtObject *self, PyObject *key, PyObject *def);

static PyValue
hamt_get(PyObject *self, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    PyObject *key;
    PyObject *def = Py_None;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("get", nargs, 1, 2)) {
        goto exit;
    }
    key = PyValue_Object(args[0]);
    if (key == NULL) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    def = PyValue_Object(args[1]);
    if (def == NULL) {
        goto exit;
    }
skip_optional:
    _return_value = hamt_get_impl((PyHamtObject *)self, key, def);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}
/*[clinic end generated code: output=8b5b34721eab82dc input=a9049054013a1b77]*/
