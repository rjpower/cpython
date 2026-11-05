/*[clinic input]
preserve
[clinic start generated code]*/

#ifndef Py_VALUE_CORE
#  error "value-emission Clinic output requires Py_VALUE_CORE"
#endif

#include "pycore_modsupport.h"    // _PyArg_NoKwnamesValue()
#include "pycore_tuple.h"         // _PyTuple_ITEMS()
#include "pycore_value_call.h"    // METH_VALUE
#include "pycore_value_slots.h"   // _PyValue_THUNK_EXIT()

PyDoc_STRVAR(tuple_index__doc__,
"index($self, value, start=0, stop=sys.maxsize, /)\n"
"--\n"
"\n"
"Return first index of value.\n"
"\n"
"Raises ValueError if the value is not present.");

#define TUPLE_INDEX_METHODDEF    \
    {"index", _PyCFunction_CAST(tuple_index), METH_VALUE, tuple_index__doc__},

static PyValue
tuple_index_impl(PyTupleObject *self, PyValueBorrow value, Py_ssize_t start,
                 Py_ssize_t stop);

static PyValue
tuple_index(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow value;
    Py_ssize_t start = 0;
    Py_ssize_t stop = PY_SSIZE_T_MAX;

    if (!_PyArg_NoKwnamesValueMethod(&PyTuple_Type, "index", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("index", nargs, 1, 3)) {
        goto exit;
    }
    value = PyValue_Borrow(args[0]);
    if (nargs < 2) {
        goto skip_optional;
    }
    if (!_PyArg_SliceIndexNotNoneValue(PyValue_Borrow(args[1]), &start)) {
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    if (!_PyArg_SliceIndexNotNoneValue(PyValue_Borrow(args[2]), &stop)) {
        goto exit;
    }
skip_optional:
    return_value = tuple_index_impl((PyTupleObject *)PyValue_AsKnownObjectBorrow(self), value, start, stop);

exit:
    return return_value;
}

PyDoc_STRVAR(tuple_count__doc__,
"count($self, value, /)\n"
"--\n"
"\n"
"Return number of occurrences of value.");

#define TUPLE_COUNT_METHODDEF    \
    {"count", _PyCFunction_CAST(tuple_count), METH_VALUE|METH_O, tuple_count__doc__},

static PyValue
tuple_count_impl(PyTupleObject *self, PyValueBorrow value);

static PyValue
tuple_count(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow value;

    value = PyValue_Borrow(args[0]);
    return_value = tuple_count_impl((PyTupleObject *)PyValue_AsKnownObjectBorrow(self), value);

    return return_value;
}

PyDoc_STRVAR(tuple_new__doc__,
"tuple(iterable=(), /)\n"
"--\n"
"\n"
"Built-in immutable sequence.\n"
"\n"
"If no argument is given, the constructor returns an empty tuple.\n"
"If iterable is specified the tuple is initialized from iterable\'s items.\n"
"\n"
"If the argument is a tuple, the return value is the same object.");

static PyValue
tuple_new_impl(PyTypeObject *type, PyValueBorrow iterable);

static PyValue
tuple_new_value(PyTypeObject *type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyTypeObject *base_tp = &PyTuple_Type;
    PyValueBorrow iterable = PyValue_Borrow(PyValue_NULL);

    if ((type == base_tp || type->tp_init == base_tp->tp_init) &&
        !_PyArg_NoKwnames("tuple", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("tuple", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    iterable = PyValue_Borrow(args[0]);
skip_optional:
    return_value = tuple_new_impl(type, iterable);

exit:
    return return_value;
}

Py_VALUE_ENTRY static PyObject *
tuple_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _PyValue_THUNK_EXIT(tuple_new_value(type, _PyTuple_ITEMS(args), nargs, NULL), BOX_SITE_CALL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return NULL;
    }
    PyValue result = tuple_new_value(type, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return _PyValue_THUNK_EXIT(result, BOX_SITE_CALL);
}

static PyValue
tuple_vectorcall(PyThreadState *tstate, PyObject *type, const PyValue *args,
    size_t nargsf, PyObject *kwnames)
{
    assert(_PyType_CAST(type)->tp_new == tuple_new);
    /* Make sure the type object is immutable: the generated
     * vectorcall doesn't deal e.g. with users reassigning __init__. */
    assert(PyType_HasFeature(_PyType_CAST(type), Py_TPFLAGS_IMMUTABLETYPE));
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    return tuple_new_value(_PyType_CAST(type), args, nargs, kwnames);
}

PyDoc_STRVAR(tuple___getnewargs____doc__,
"__getnewargs__($self, /)\n"
"--\n"
"\n");

#define TUPLE___GETNEWARGS___METHODDEF    \
    {"__getnewargs__", _PyCFunction_CAST(tuple___getnewargs__), METH_VALUE|METH_NOARGS, tuple___getnewargs____doc__},

static PyValue
tuple___getnewargs___impl(PyTupleObject *self);

static PyValue
tuple___getnewargs__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = tuple___getnewargs___impl((PyTupleObject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}
/*[clinic end generated code: output=8bc3f238c6c6fc38 input=a9049054013a1b77]*/
