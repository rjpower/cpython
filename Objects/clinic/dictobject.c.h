/*[clinic input]
preserve
[clinic start generated code]*/

#ifndef Py_VALUE_CORE
#  error "value-emission Clinic output requires Py_VALUE_CORE"
#endif

#include "pycore_critical_section.h"// Py_BEGIN_CRITICAL_SECTION()
#include "pycore_modsupport.h"    // _PyArg_NoKwnamesValue()
#include "pycore_value_call.h"    // METH_VALUE

static PyObject *
dict_repr_impl(PyObject *self);

static PyObject *
dict_repr(PyObject *self)
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = dict_repr_impl(self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(dict_fromkeys__doc__,
"fromkeys($type, iterable, value=None, /)\n"
"--\n"
"\n"
"Create a new dictionary with keys from iterable and values set to value.");

#define DICT_FROMKEYS_METHODDEF    \
    {"fromkeys", _PyCFunction_CAST(dict_fromkeys), METH_VALUE|METH_CLASS, dict_fromkeys__doc__},

static PyValue
dict_fromkeys_impl(PyTypeObject *type, PyValueBorrow iterable,
                   PyValueBorrow value);

static PyValue
dict_fromkeys(PyValueBorrow type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow iterable;
    PyValueBorrow value = PyValue_Borrow(PyValue_None);

    if (!_PyArg_NoKwnamesValueMethod(&PyDict_Type, "fromkeys", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("fromkeys", nargs, 1, 2)) {
        goto exit;
    }
    iterable = PyValue_Borrow(args[0]);
    if (nargs < 2) {
        goto skip_optional;
    }
    value = PyValue_Borrow(args[1]);
skip_optional:
    return_value = dict_fromkeys_impl((PyTypeObject *)PyValue_AsKnownObjectBorrow(type), iterable, value);

exit:
    return return_value;
}

PyDoc_STRVAR(dict_copy__doc__,
"copy($self, /)\n"
"--\n"
"\n"
"Return a shallow copy of the dict.");

#define DICT_COPY_METHODDEF    \
    {"copy", _PyCFunction_CAST(dict_copy), METH_VALUE|METH_NOARGS, dict_copy__doc__},

static PyValue
dict_copy_impl(PyDictObject *self);

static PyValue
dict_copy(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = dict_copy_impl((PyDictObject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(dict___contains____doc__,
"__contains__($self, key, /)\n"
"--\n"
"\n"
"True if the dictionary has the specified key, else False.");

#define DICT___CONTAINS___METHODDEF    \
    {"__contains__", _PyCFunction_CAST(dict___contains__), METH_VALUE|METH_O|METH_COEXIST, dict___contains____doc__},

static PyValue
dict___contains___impl(PyDictObject *self, PyValueBorrow key);

static PyValue
dict___contains__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow key;

    key = PyValue_Borrow(args[0]);
    return_value = dict___contains___impl((PyDictObject *)PyValue_AsKnownObjectBorrow(self), key);

    return return_value;
}

PyDoc_STRVAR(dict_get__doc__,
"get($self, key, default=None, /)\n"
"--\n"
"\n"
"Return the value for key if key is in the dictionary, else default.");

#define DICT_GET_METHODDEF    \
    {"get", _PyCFunction_CAST(dict_get), METH_VALUE, dict_get__doc__},

static PyValue
dict_get_impl(PyDictObject *self, PyValueBorrow key,
              PyValueBorrow default_value);

static PyValue
dict_get(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow key;
    PyValueBorrow default_value = PyValue_Borrow(PyValue_None);

    if (!_PyArg_NoKwnamesValueMethod(&PyDict_Type, "get", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("get", nargs, 1, 2)) {
        goto exit;
    }
    key = PyValue_Borrow(args[0]);
    if (nargs < 2) {
        goto skip_optional;
    }
    default_value = PyValue_Borrow(args[1]);
skip_optional:
    return_value = dict_get_impl((PyDictObject *)PyValue_AsKnownObjectBorrow(self), key, default_value);

exit:
    return return_value;
}

PyDoc_STRVAR(dict_setdefault__doc__,
"setdefault($self, key, default=None, /)\n"
"--\n"
"\n"
"Insert key with a value of default if key is not in the dictionary.\n"
"\n"
"Return the value for key if key is in the dictionary, else default.");

#define DICT_SETDEFAULT_METHODDEF    \
    {"setdefault", _PyCFunction_CAST(dict_setdefault), METH_VALUE, dict_setdefault__doc__},

static PyValue
dict_setdefault_impl(PyDictObject *self, PyValueBorrow key,
                     PyValueBorrow default_value);

static PyValue
dict_setdefault(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow key;
    PyValueBorrow default_value = PyValue_Borrow(PyValue_None);

    if (!_PyArg_NoKwnamesValueMethod(&PyDict_Type, "setdefault", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("setdefault", nargs, 1, 2)) {
        goto exit;
    }
    key = PyValue_Borrow(args[0]);
    if (nargs < 2) {
        goto skip_optional;
    }
    default_value = PyValue_Borrow(args[1]);
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = dict_setdefault_impl((PyDictObject *)PyValue_AsKnownObjectBorrow(self), key, default_value);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(dict_clear__doc__,
"clear($self, /)\n"
"--\n"
"\n"
"Remove all items from the dict.");

#define DICT_CLEAR_METHODDEF    \
    {"clear", _PyCFunction_CAST(dict_clear), METH_VALUE|METH_NOARGS, dict_clear__doc__},

static PyValue
dict_clear_impl(PyDictObject *self);

static PyValue
dict_clear(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = dict_clear_impl((PyDictObject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(dict_pop__doc__,
"pop($self, key, default=<unrepresentable>, /)\n"
"--\n"
"\n"
"D.pop(k[,d]) -> v, remove specified key and return the corresponding value.\n"
"\n"
"If the key is not found, return the default if given; otherwise,\n"
"raise a KeyError.");

#define DICT_POP_METHODDEF    \
    {"pop", _PyCFunction_CAST(dict_pop), METH_VALUE, dict_pop__doc__},

static PyValue
dict_pop_impl(PyDictObject *self, PyValueBorrow key,
              PyValueBorrow default_value);

static PyValue
dict_pop(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow key;
    PyValueBorrow default_value = PyValue_Borrow(PyValue_NULL);

    if (!_PyArg_NoKwnamesValueMethod(&PyDict_Type, "pop", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("pop", nargs, 1, 2)) {
        goto exit;
    }
    key = PyValue_Borrow(args[0]);
    if (nargs < 2) {
        goto skip_optional;
    }
    default_value = PyValue_Borrow(args[1]);
skip_optional:
    return_value = dict_pop_impl((PyDictObject *)PyValue_AsKnownObjectBorrow(self), key, default_value);

exit:
    return return_value;
}

PyDoc_STRVAR(dict_popitem__doc__,
"popitem($self, /)\n"
"--\n"
"\n"
"Remove and return a (key, value) pair as a 2-tuple.\n"
"\n"
"Pairs are returned in LIFO (last-in, first-out) order.\n"
"Raises KeyError if the dict is empty.");

#define DICT_POPITEM_METHODDEF    \
    {"popitem", _PyCFunction_CAST(dict_popitem), METH_VALUE|METH_NOARGS, dict_popitem__doc__},

static PyValue
dict_popitem_impl(PyDictObject *self);

static PyValue
dict_popitem(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = dict_popitem_impl((PyDictObject *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(dict___sizeof____doc__,
"__sizeof__($self, /)\n"
"--\n"
"\n"
"Return the size of the dict in memory, in bytes.");

#define DICT___SIZEOF___METHODDEF    \
    {"__sizeof__", _PyCFunction_CAST(dict___sizeof__), METH_VALUE|METH_NOARGS, dict___sizeof____doc__},

static PyValue
dict___sizeof___impl(PyDictObject *self);

static PyValue
dict___sizeof__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = dict___sizeof___impl((PyDictObject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(dict___reversed____doc__,
"__reversed__($self, /)\n"
"--\n"
"\n"
"Return a reverse iterator over the dict keys.");

#define DICT___REVERSED___METHODDEF    \
    {"__reversed__", _PyCFunction_CAST(dict___reversed__), METH_VALUE|METH_NOARGS, dict___reversed____doc__},

static PyValue
dict___reversed___impl(PyDictObject *self);

static PyValue
dict___reversed__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = dict___reversed___impl((PyDictObject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(dict_keys__doc__,
"keys($self, /)\n"
"--\n"
"\n"
"Return a set-like object providing a view on the dict\'s keys.");

#define DICT_KEYS_METHODDEF    \
    {"keys", _PyCFunction_CAST(dict_keys), METH_VALUE|METH_NOARGS, dict_keys__doc__},

static PyValue
dict_keys_impl(PyDictObject *self);

static PyValue
dict_keys(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = dict_keys_impl((PyDictObject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(dict_items__doc__,
"items($self, /)\n"
"--\n"
"\n"
"Return a set-like object providing a view on the dict\'s items.");

#define DICT_ITEMS_METHODDEF    \
    {"items", _PyCFunction_CAST(dict_items), METH_VALUE|METH_NOARGS, dict_items__doc__},

static PyValue
dict_items_impl(PyDictObject *self);

static PyValue
dict_items(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = dict_items_impl((PyDictObject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(dict_values__doc__,
"values($self, /)\n"
"--\n"
"\n"
"Return an object providing a view on the dict\'s values.");

#define DICT_VALUES_METHODDEF    \
    {"values", _PyCFunction_CAST(dict_values), METH_VALUE|METH_NOARGS, dict_values__doc__},

static PyValue
dict_values_impl(PyDictObject *self);

static PyValue
dict_values(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = dict_values_impl((PyDictObject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(frozendict_copy__doc__,
"copy($self, /)\n"
"--\n"
"\n"
"Return a shallow copy of the frozendict.");

#define FROZENDICT_COPY_METHODDEF    \
    {"copy", _PyCFunction_CAST(frozendict_copy), METH_VALUE|METH_NOARGS, frozendict_copy__doc__},

static PyValue
frozendict_copy_impl(PyFrozenDictObject *self);

static PyValue
frozendict_copy(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = frozendict_copy_impl((PyFrozenDictObject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}
/*[clinic end generated code: output=b5978c8e401704dc input=a9049054013a1b77]*/
