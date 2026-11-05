/*[clinic input]
preserve
[clinic start generated code]*/

#ifndef Py_VALUE_CORE
#  error "value-emission Clinic output requires Py_VALUE_CORE"
#endif

#include "pycore_long_value.h"    // _PyValue_AsInt()
#include "pycore_modsupport.h"    // _PyArg_CheckArityValue()
#include "pycore_value_call.h"    // METH_VALUE

PyDoc_STRVAR(type___instancecheck____doc__,
"__instancecheck__($self, instance, /)\n"
"--\n"
"\n"
"Check if an object is an instance.");

#define TYPE___INSTANCECHECK___METHODDEF    \
    {"__instancecheck__", _PyCFunction_CAST(type___instancecheck__), METH_VALUE, type___instancecheck____doc__},

static int
type___instancecheck___impl(PyTypeObject *self, PyValueBorrow instance);

static PyValue
type___instancecheck__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow instance;
    int _return_value;

    if (!_PyArg_CheckArityValueMethod(&PyType_Type, "__instancecheck__", nargs, kwnames, 1)) {
        goto exit;
    }
    instance = PyValue_Borrow(args[0]);
    _return_value = type___instancecheck___impl((PyTypeObject *)PyValue_AsKnownObjectBorrow(self), instance);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _return_value ? PyValue_True : PyValue_False;

exit:
    return return_value;
}

PyDoc_STRVAR(type___subclasscheck____doc__,
"__subclasscheck__($self, subclass, /)\n"
"--\n"
"\n"
"Check if a class is a subclass.");

#define TYPE___SUBCLASSCHECK___METHODDEF    \
    {"__subclasscheck__", _PyCFunction_CAST(type___subclasscheck__), METH_VALUE, type___subclasscheck____doc__},

static int
type___subclasscheck___impl(PyTypeObject *self, PyValueBorrow subclass);

static PyValue
type___subclasscheck__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow subclass;
    int _return_value;

    if (!_PyArg_CheckArityValueMethod(&PyType_Type, "__subclasscheck__", nargs, kwnames, 1)) {
        goto exit;
    }
    subclass = PyValue_Borrow(args[0]);
    _return_value = type___subclasscheck___impl((PyTypeObject *)PyValue_AsKnownObjectBorrow(self), subclass);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _return_value ? PyValue_True : PyValue_False;

exit:
    return return_value;
}

PyDoc_STRVAR(type_mro__doc__,
"mro($self, /)\n"
"--\n"
"\n"
"Return a type\'s method resolution order.");

#define TYPE_MRO_METHODDEF    \
    {"mro", _PyCFunction_CAST(type_mro), METH_VALUE, type_mro__doc__},

static PyValue
type_mro_impl(PyTypeObject *self);

static PyValue
type_mro(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValueMethod(&PyType_Type, "mro", nargs, kwnames, 0)) {
        goto exit;
    }
    return_value = type_mro_impl((PyTypeObject *)PyValue_AsKnownObjectBorrow(self));

exit:
    return return_value;
}

PyDoc_STRVAR(type___subclasses____doc__,
"__subclasses__($self, /)\n"
"--\n"
"\n"
"Return a list of immediate subclasses.");

#define TYPE___SUBCLASSES___METHODDEF    \
    {"__subclasses__", _PyCFunction_CAST(type___subclasses__), METH_VALUE, type___subclasses____doc__},

static PyValue
type___subclasses___impl(PyTypeObject *self);

static PyValue
type___subclasses__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValueMethod(&PyType_Type, "__subclasses__", nargs, kwnames, 0)) {
        goto exit;
    }
    return_value = type___subclasses___impl((PyTypeObject *)PyValue_AsKnownObjectBorrow(self));

exit:
    return return_value;
}

PyDoc_STRVAR(type___dir____doc__,
"__dir__($self, /)\n"
"--\n"
"\n"
"Specialized __dir__ implementation for types.");

#define TYPE___DIR___METHODDEF    \
    {"__dir__", _PyCFunction_CAST(type___dir__), METH_VALUE, type___dir____doc__},

static PyValue
type___dir___impl(PyTypeObject *self);

static PyValue
type___dir__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValueMethod(&PyType_Type, "__dir__", nargs, kwnames, 0)) {
        goto exit;
    }
    return_value = type___dir___impl((PyTypeObject *)PyValue_AsKnownObjectBorrow(self));

exit:
    return return_value;
}

PyDoc_STRVAR(type___sizeof____doc__,
"__sizeof__($self, /)\n"
"--\n"
"\n"
"Return memory consumption of the type object.");

#define TYPE___SIZEOF___METHODDEF    \
    {"__sizeof__", _PyCFunction_CAST(type___sizeof__), METH_VALUE, type___sizeof____doc__},

static PyValue
type___sizeof___impl(PyTypeObject *self);

static PyValue
type___sizeof__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValueMethod(&PyType_Type, "__sizeof__", nargs, kwnames, 0)) {
        goto exit;
    }
    return_value = type___sizeof___impl((PyTypeObject *)PyValue_AsKnownObjectBorrow(self));

exit:
    return return_value;
}

PyDoc_STRVAR(object___getstate____doc__,
"__getstate__($self, /)\n"
"--\n"
"\n"
"Helper for pickle.");

#define OBJECT___GETSTATE___METHODDEF    \
    {"__getstate__", _PyCFunction_CAST(object___getstate__), METH_VALUE, object___getstate____doc__},

static PyValue
object___getstate___impl(PyValueBorrow self);

static PyValue
object___getstate__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValueMethod(&PyBaseObject_Type, "__getstate__", nargs, kwnames, 0)) {
        goto exit;
    }
    return_value = object___getstate___impl(self);

exit:
    return return_value;
}

PyDoc_STRVAR(object___reduce____doc__,
"__reduce__($self, /)\n"
"--\n"
"\n"
"Helper for pickle.");

#define OBJECT___REDUCE___METHODDEF    \
    {"__reduce__", _PyCFunction_CAST(object___reduce__), METH_VALUE, object___reduce____doc__},

static PyValue
object___reduce___impl(PyValueBorrow self);

static PyValue
object___reduce__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValueMethod(&PyBaseObject_Type, "__reduce__", nargs, kwnames, 0)) {
        goto exit;
    }
    return_value = object___reduce___impl(self);

exit:
    return return_value;
}

PyDoc_STRVAR(object___reduce_ex____doc__,
"__reduce_ex__($self, protocol, /)\n"
"--\n"
"\n"
"Helper for pickle.");

#define OBJECT___REDUCE_EX___METHODDEF    \
    {"__reduce_ex__", _PyCFunction_CAST(object___reduce_ex__), METH_VALUE, object___reduce_ex____doc__},

static PyValue
object___reduce_ex___impl(PyValueBorrow self, int protocol);

static PyValue
object___reduce_ex__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int protocol;

    if (!_PyArg_CheckArityValueMethod(&PyBaseObject_Type, "__reduce_ex__", nargs, kwnames, 1)) {
        goto exit;
    }
    protocol = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (protocol == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = object___reduce_ex___impl(self, protocol);

exit:
    return return_value;
}

PyDoc_STRVAR(object___format____doc__,
"__format__($self, format_spec, /)\n"
"--\n"
"\n"
"Default object formatter.\n"
"\n"
"Return str(self) if format_spec is empty. Raise TypeError otherwise.");

#define OBJECT___FORMAT___METHODDEF    \
    {"__format__", _PyCFunction_CAST(object___format__), METH_VALUE, object___format____doc__},

static PyValue
object___format___impl(PyValueBorrow self, PyObject *format_spec);

static PyValue
object___format__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *format_spec;

    if (!_PyArg_CheckArityValueMethod(&PyBaseObject_Type, "__format__", nargs, kwnames, 1)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("__format__", "argument", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("__format__", "argument", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    format_spec = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    return_value = object___format___impl(self, format_spec);

exit:
    return return_value;
}

PyDoc_STRVAR(object___sizeof____doc__,
"__sizeof__($self, /)\n"
"--\n"
"\n"
"Size of object in memory, in bytes.");

#define OBJECT___SIZEOF___METHODDEF    \
    {"__sizeof__", _PyCFunction_CAST(object___sizeof__), METH_VALUE, object___sizeof____doc__},

static PyValue
object___sizeof___impl(PyValueBorrow self);

static PyValue
object___sizeof__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValueMethod(&PyBaseObject_Type, "__sizeof__", nargs, kwnames, 0)) {
        goto exit;
    }
    return_value = object___sizeof___impl(self);

exit:
    return return_value;
}

PyDoc_STRVAR(object___dir____doc__,
"__dir__($self, /)\n"
"--\n"
"\n"
"Default dir() implementation.");

#define OBJECT___DIR___METHODDEF    \
    {"__dir__", _PyCFunction_CAST(object___dir__), METH_VALUE, object___dir____doc__},

static PyValue
object___dir___impl(PyValueBorrow self);

static PyValue
object___dir__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValueMethod(&PyBaseObject_Type, "__dir__", nargs, kwnames, 0)) {
        goto exit;
    }
    return_value = object___dir___impl(self);

exit:
    return return_value;
}
/*[clinic end generated code: output=ce0ba69a5eacbc1b input=a9049054013a1b77]*/
