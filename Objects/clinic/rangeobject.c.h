/*[clinic input]
preserve
[clinic start generated code]*/

#ifndef Py_VALUE_CORE
#  error "value-emission Clinic output requires Py_VALUE_CORE"
#endif

#include "pycore_critical_section.h"// Py_BEGIN_CRITICAL_SECTION()
#include "pycore_modsupport.h"    // _PyArg_CheckArityValue()
#include "pycore_value_call.h"    // METH_VALUE

PyDoc_STRVAR(range_iterator___length_hint____doc__,
"__length_hint__($self, /)\n"
"--\n"
"\n"
"Private method returning an estimate of len(list(it)).");

#define RANGE_ITERATOR___LENGTH_HINT___METHODDEF    \
    {"__length_hint__", _PyCFunction_CAST(range_iterator___length_hint__), METH_VALUE, range_iterator___length_hint____doc__},

static PyValue
range_iterator___length_hint___impl(_PyRangeIterObject *r);

static PyValue
range_iterator___length_hint__(PyValueBorrow r, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValueMethod(&PyRangeIter_Type, "__length_hint__", nargs, kwnames, 0)) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(r));
    return_value = range_iterator___length_hint___impl((_PyRangeIterObject *)PyValue_AsKnownObjectBorrow(r));
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(range_iterator___reduce____doc__,
"__reduce__($self, /)\n"
"--\n"
"\n"
"Return state information for pickling.");

#define RANGE_ITERATOR___REDUCE___METHODDEF    \
    {"__reduce__", _PyCFunction_CAST(range_iterator___reduce__), METH_VALUE, range_iterator___reduce____doc__},

static PyValue
range_iterator___reduce___impl(_PyRangeIterObject *r);

static PyValue
range_iterator___reduce__(PyValueBorrow r, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValueMethod(&PyRangeIter_Type, "__reduce__", nargs, kwnames, 0)) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(r));
    return_value = range_iterator___reduce___impl((_PyRangeIterObject *)PyValue_AsKnownObjectBorrow(r));
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(range_iterator___setstate____doc__,
"__setstate__($self, state, /)\n"
"--\n"
"\n"
"Set state information for unpickling.");

#define RANGE_ITERATOR___SETSTATE___METHODDEF    \
    {"__setstate__", _PyCFunction_CAST(range_iterator___setstate__), METH_VALUE, range_iterator___setstate____doc__},

static PyValue
range_iterator___setstate___impl(_PyRangeIterObject *r, PyValueBorrow state);

static PyValue
range_iterator___setstate__(PyValueBorrow r, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow state;

    if (!_PyArg_CheckArityValueMethod(&PyRangeIter_Type, "__setstate__", nargs, kwnames, 1)) {
        goto exit;
    }
    state = PyValue_Borrow(args[0]);
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(r));
    return_value = range_iterator___setstate___impl((_PyRangeIterObject *)PyValue_AsKnownObjectBorrow(r), state);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(longrange_iterator___length_hint____doc__,
"__length_hint__($self, /)\n"
"--\n"
"\n"
"Private method returning an estimate of len(list(it)).");

#define LONGRANGE_ITERATOR___LENGTH_HINT___METHODDEF    \
    {"__length_hint__", _PyCFunction_CAST(longrange_iterator___length_hint__), METH_VALUE, longrange_iterator___length_hint____doc__},

static PyValue
longrange_iterator___length_hint___impl(longrangeiterobject *r);

static PyValue
longrange_iterator___length_hint__(PyValueBorrow r, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValueMethod(&PyLongRangeIter_Type, "__length_hint__", nargs, kwnames, 0)) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(r));
    return_value = longrange_iterator___length_hint___impl((longrangeiterobject *)PyValue_AsKnownObjectBorrow(r));
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(longrange_iterator___reduce____doc__,
"__reduce__($self, /)\n"
"--\n"
"\n"
"Return state information for pickling.");

#define LONGRANGE_ITERATOR___REDUCE___METHODDEF    \
    {"__reduce__", _PyCFunction_CAST(longrange_iterator___reduce__), METH_VALUE, longrange_iterator___reduce____doc__},

static PyValue
longrange_iterator___reduce___impl(longrangeiterobject *r);

static PyValue
longrange_iterator___reduce__(PyValueBorrow r, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValueMethod(&PyLongRangeIter_Type, "__reduce__", nargs, kwnames, 0)) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(r));
    return_value = longrange_iterator___reduce___impl((longrangeiterobject *)PyValue_AsKnownObjectBorrow(r));
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(longrange_iterator___setstate____doc__,
"__setstate__($self, state, /)\n"
"--\n"
"\n"
"Set state information for unpickling.");

#define LONGRANGE_ITERATOR___SETSTATE___METHODDEF    \
    {"__setstate__", _PyCFunction_CAST(longrange_iterator___setstate__), METH_VALUE, longrange_iterator___setstate____doc__},

static PyValue
longrange_iterator___setstate___impl(longrangeiterobject *r,
                                     PyValueBorrow state);

static PyValue
longrange_iterator___setstate__(PyValueBorrow r, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow state;

    if (!_PyArg_CheckArityValueMethod(&PyLongRangeIter_Type, "__setstate__", nargs, kwnames, 1)) {
        goto exit;
    }
    state = PyValue_Borrow(args[0]);
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(r));
    return_value = longrange_iterator___setstate___impl((longrangeiterobject *)PyValue_AsKnownObjectBorrow(r), state);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

static PyObject *
longrangeiter_next_impl(longrangeiterobject *self);

static PyObject *
longrangeiter_next(PyObject *self)
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = longrangeiter_next_impl((longrangeiterobject *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}
/*[clinic end generated code: output=8d7940d0239331db input=a9049054013a1b77]*/
