/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_modsupport.h"    // _PyArg_CheckPositional()

PyDoc_STRVAR(tuple_index__doc__,
"index($self, value, start=0, stop=sys.maxsize, /)\n"
"--\n"
"\n"
"Return first index of value.\n"
"\n"
"Raises ValueError if the value is not present.");

#define TUPLE_INDEX_METHODDEF    \
    {"index", _PyCFunction_CAST(tuple_index), METH_FASTCALL, tuple_index__doc__},

static PyObject *
tuple_index_impl(PyTupleObject *self, PyObject *value, Py_ssize_t start,
                 Py_ssize_t stop);

static PyValue
tuple_index(PyObject *self, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    PyObject *value;
    Py_ssize_t start = 0;
    Py_ssize_t stop = PY_SSIZE_T_MAX;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("index", nargs, 1, 3)) {
        goto exit;
    }
    value = PyValue_Object(args[0]);
    if (value == NULL) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    {
        PyObject *_arg = PyValue_Object(args[1]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!_PyEval_SliceIndexNotNone(_arg, &start)) {
            goto exit;
        }
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    {
        PyObject *_arg = PyValue_Object(args[2]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!_PyEval_SliceIndexNotNone(_arg, &stop)) {
            goto exit;
        }
    }
skip_optional:
    _return_value = tuple_index_impl((PyTupleObject *)self, value, start, stop);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(tuple_count__doc__,
"count($self, value, /)\n"
"--\n"
"\n"
"Return number of occurrences of value.");

#define TUPLE_COUNT_METHODDEF    \
    {"count", _PyCFunction_CAST(tuple_count), METH_O, tuple_count__doc__},

static PyObject *
tuple_count_impl(PyTupleObject *self, PyObject *value);

static PyValue
tuple_count(PyObject *self, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyObject *value;
    PyObject *_return_value;

    value = PyValue_Object(arg);
    if (value == NULL) {
        goto exit;
    }
    _return_value = tuple_count_impl((PyTupleObject *)self, value);
    return_value = PyValue_FromResult(_return_value);

exit:
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

static PyObject *
tuple_new_impl(PyTypeObject *type, PyObject *iterable);

static PyObject *
tuple_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    PyTypeObject *base_tp = &PyTuple_Type;
    PyObject *iterable = NULL;

    if ((type == base_tp || type->tp_init == base_tp->tp_init) &&
        !_PyArg_NoKeywords("tuple", kwargs)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("tuple", PyTuple_GET_SIZE(args), 0, 1)) {
        goto exit;
    }
    if (PyTuple_GET_SIZE(args) < 1) {
        goto skip_optional;
    }
    iterable = PyValue_Object(PyTuple_GET_VALUE(args, 0));
    if (iterable == NULL) {
        goto exit;
    }
skip_optional:
    return_value = tuple_new_impl(type, iterable);

exit:
    return return_value;
}

static PyValue
tuple_vectorcall(PyValue type_value, const PyValue *args,
    size_t nargsf, PyObject *kwnames)
{
    PyObject *type = PyValue_ObjectBorrow(type_value);
    PyObject *return_value = NULL;
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    PyObject *iterable = NULL;

    assert(_PyType_CAST(type)->tp_new == tuple_new);
    /* Make sure the type object is immutable: the generated
     * vectorcall doesn't deal e.g. with users reassigning __init__. */
    assert(PyType_HasFeature(_PyType_CAST(type), Py_TPFLAGS_IMMUTABLETYPE));
    if (!_PyArg_NoKwnames("tuple", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("tuple", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    iterable = PyValue_Object(args[0]);
    if (iterable == NULL) {
        goto exit;
    }
skip_optional:
    return_value = tuple_new_impl(_PyType_CAST(type), iterable);

exit:
    return PyValue_FromResult(return_value);
}

PyDoc_STRVAR(tuple___getnewargs____doc__,
"__getnewargs__($self, /)\n"
"--\n"
"\n");

#define TUPLE___GETNEWARGS___METHODDEF    \
    {"__getnewargs__", (PyCFunction)tuple___getnewargs__, METH_NOARGS, tuple___getnewargs____doc__},

static PyObject *
tuple___getnewargs___impl(PyTupleObject *self);

static PyObject *
tuple___getnewargs__(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return tuple___getnewargs___impl((PyTupleObject *)self);
}
/*[clinic end generated code: output=6fee3d891e4c4c81 input=a9049054013a1b77]*/
