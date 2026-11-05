/*[clinic input]
preserve
[clinic start generated code]*/

#ifndef Py_VALUE_CORE
#  error "value-emission Clinic output requires Py_VALUE_CORE"
#endif

#include "pycore_critical_section.h"// Py_BEGIN_CRITICAL_SECTION()
#include "pycore_value_call.h"    // METH_VALUE

static PyObject *
set_repr_impl(PySetObject *so);

static PyObject *
set_repr(PyObject *so)
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(so);
    return_value = set_repr_impl((PySetObject *)so);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(set_pop__doc__,
"pop($self, /)\n"
"--\n"
"\n"
"Remove and return an arbitrary set element.\n"
"\n"
"Raises KeyError if the set is empty.");

#define SET_POP_METHODDEF    \
    {"pop", _PyCFunction_CAST(set_pop), METH_VALUE|METH_NOARGS, set_pop__doc__},

static PyValue
set_pop_impl(PySetObject *so);

static PyValue
set_pop(PyValueBorrow so, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(so));
    return_value = set_pop_impl((PySetObject *)PyValue_AsKnownObjectBorrow(so));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(set_update__doc__,
"update($self, /, *others)\n"
"--\n"
"\n"
"Update the set, adding elements from all others.");

#define SET_UPDATE_METHODDEF    \
    {"update", _PyCFunction_CAST(set_update), METH_VALUE, set_update__doc__},

static PyValue
set_update_impl(PySetObject *so, const PyValue *others,
                Py_ssize_t others_length);

static PyValue
set_update(PyValueBorrow so, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    const PyValue *others;
    Py_ssize_t others_length;

    others = args;
    others_length = nargs;
    return_value = set_update_impl((PySetObject *)PyValue_AsKnownObjectBorrow(so), others, others_length);

    return return_value;
}

PyDoc_STRVAR(set_copy__doc__,
"copy($self, /)\n"
"--\n"
"\n"
"Return a shallow copy of a set.");

#define SET_COPY_METHODDEF    \
    {"copy", _PyCFunction_CAST(set_copy), METH_VALUE|METH_NOARGS, set_copy__doc__},

static PyValue
set_copy_impl(PySetObject *so);

static PyValue
set_copy(PyValueBorrow so, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(so));
    return_value = set_copy_impl((PySetObject *)PyValue_AsKnownObjectBorrow(so));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(frozenset_copy__doc__,
"copy($self, /)\n"
"--\n"
"\n"
"Return a shallow copy of a set.");

#define FROZENSET_COPY_METHODDEF    \
    {"copy", _PyCFunction_CAST(frozenset_copy), METH_VALUE|METH_NOARGS, frozenset_copy__doc__},

static PyValue
frozenset_copy_impl(PySetObject *so);

static PyValue
frozenset_copy(PyValueBorrow so, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(so));
    return_value = frozenset_copy_impl((PySetObject *)PyValue_AsKnownObjectBorrow(so));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(set_clear__doc__,
"clear($self, /)\n"
"--\n"
"\n"
"Remove all elements from this set.");

#define SET_CLEAR_METHODDEF    \
    {"clear", _PyCFunction_CAST(set_clear), METH_VALUE|METH_NOARGS, set_clear__doc__},

static PyValue
set_clear_impl(PySetObject *so);

static PyValue
set_clear(PyValueBorrow so, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(so));
    return_value = set_clear_impl((PySetObject *)PyValue_AsKnownObjectBorrow(so));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(set_union__doc__,
"union($self, /, *others)\n"
"--\n"
"\n"
"Return a new set with elements from the set and all others.");

#define SET_UNION_METHODDEF    \
    {"union", _PyCFunction_CAST(set_union), METH_VALUE, set_union__doc__},

static PyValue
set_union_impl(PySetObject *so, const PyValue *others,
               Py_ssize_t others_length);

static PyValue
set_union(PyValueBorrow so, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    const PyValue *others;
    Py_ssize_t others_length;

    others = args;
    others_length = nargs;
    return_value = set_union_impl((PySetObject *)PyValue_AsKnownObjectBorrow(so), others, others_length);

    return return_value;
}

PyDoc_STRVAR(set_intersection_multi__doc__,
"intersection($self, /, *others)\n"
"--\n"
"\n"
"Return a new set with elements common to the set and all others.");

#define SET_INTERSECTION_MULTI_METHODDEF    \
    {"intersection", _PyCFunction_CAST(set_intersection_multi), METH_VALUE, set_intersection_multi__doc__},

static PyValue
set_intersection_multi_impl(PySetObject *so, const PyValue *others,
                            Py_ssize_t others_length);

static PyValue
set_intersection_multi(PyValueBorrow so, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    const PyValue *others;
    Py_ssize_t others_length;

    others = args;
    others_length = nargs;
    return_value = set_intersection_multi_impl((PySetObject *)PyValue_AsKnownObjectBorrow(so), others, others_length);

    return return_value;
}

PyDoc_STRVAR(set_intersection_update_multi__doc__,
"intersection_update($self, /, *others)\n"
"--\n"
"\n"
"Update the set, keeping only elements found in it and all others.");

#define SET_INTERSECTION_UPDATE_MULTI_METHODDEF    \
    {"intersection_update", _PyCFunction_CAST(set_intersection_update_multi), METH_VALUE, set_intersection_update_multi__doc__},

static PyValue
set_intersection_update_multi_impl(PySetObject *so, const PyValue *others,
                                   Py_ssize_t others_length);

static PyValue
set_intersection_update_multi(PyValueBorrow so, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    const PyValue *others;
    Py_ssize_t others_length;

    others = args;
    others_length = nargs;
    return_value = set_intersection_update_multi_impl((PySetObject *)PyValue_AsKnownObjectBorrow(so), others, others_length);

    return return_value;
}

PyDoc_STRVAR(set_isdisjoint__doc__,
"isdisjoint($self, other, /)\n"
"--\n"
"\n"
"Return True if two sets have a null intersection.");

#define SET_ISDISJOINT_METHODDEF    \
    {"isdisjoint", _PyCFunction_CAST(set_isdisjoint), METH_VALUE|METH_O, set_isdisjoint__doc__},

static PyValue
set_isdisjoint_impl(PySetObject *so, PyValueBorrow other);

static PyValue
set_isdisjoint(PyValueBorrow so, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow other;

    other = PyValue_Borrow(args[0]);
    Py_BEGIN_CRITICAL_SECTION2(PyValue_AsKnownObjectBorrow(so), PyValue_AsKnownObjectBorrow(other));
    return_value = set_isdisjoint_impl((PySetObject *)PyValue_AsKnownObjectBorrow(so), other);
    Py_END_CRITICAL_SECTION2();

    return return_value;
}

PyDoc_STRVAR(set_difference_update__doc__,
"difference_update($self, /, *others)\n"
"--\n"
"\n"
"Update the set, removing elements found in others.");

#define SET_DIFFERENCE_UPDATE_METHODDEF    \
    {"difference_update", _PyCFunction_CAST(set_difference_update), METH_VALUE, set_difference_update__doc__},

static PyValue
set_difference_update_impl(PySetObject *so, const PyValue *others,
                           Py_ssize_t others_length);

static PyValue
set_difference_update(PyValueBorrow so, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    const PyValue *others;
    Py_ssize_t others_length;

    others = args;
    others_length = nargs;
    return_value = set_difference_update_impl((PySetObject *)PyValue_AsKnownObjectBorrow(so), others, others_length);

    return return_value;
}

PyDoc_STRVAR(set_difference_multi__doc__,
"difference($self, /, *others)\n"
"--\n"
"\n"
"Return a new set with elements in the set that are not in the others.");

#define SET_DIFFERENCE_MULTI_METHODDEF    \
    {"difference", _PyCFunction_CAST(set_difference_multi), METH_VALUE, set_difference_multi__doc__},

static PyValue
set_difference_multi_impl(PySetObject *so, const PyValue *others,
                          Py_ssize_t others_length);

static PyValue
set_difference_multi(PyValueBorrow so, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    const PyValue *others;
    Py_ssize_t others_length;

    others = args;
    others_length = nargs;
    return_value = set_difference_multi_impl((PySetObject *)PyValue_AsKnownObjectBorrow(so), others, others_length);

    return return_value;
}

PyDoc_STRVAR(set_symmetric_difference_update__doc__,
"symmetric_difference_update($self, other, /)\n"
"--\n"
"\n"
"Update the set, keeping only elements found in either set, but not in both.");

#define SET_SYMMETRIC_DIFFERENCE_UPDATE_METHODDEF    \
    {"symmetric_difference_update", _PyCFunction_CAST(set_symmetric_difference_update), METH_VALUE|METH_O, set_symmetric_difference_update__doc__},

static PyValue
set_symmetric_difference_update_impl(PySetObject *so, PyValueBorrow other);

static PyValue
set_symmetric_difference_update(PyValueBorrow so, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow other;

    other = PyValue_Borrow(args[0]);
    return_value = set_symmetric_difference_update_impl((PySetObject *)PyValue_AsKnownObjectBorrow(so), other);

    return return_value;
}

PyDoc_STRVAR(set_symmetric_difference__doc__,
"symmetric_difference($self, other, /)\n"
"--\n"
"\n"
"Return a new set with elements in either the set or other but not both.");

#define SET_SYMMETRIC_DIFFERENCE_METHODDEF    \
    {"symmetric_difference", _PyCFunction_CAST(set_symmetric_difference), METH_VALUE|METH_O, set_symmetric_difference__doc__},

static PyValue
set_symmetric_difference_impl(PySetObject *so, PyValueBorrow other);

static PyValue
set_symmetric_difference(PyValueBorrow so, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow other;

    other = PyValue_Borrow(args[0]);
    Py_BEGIN_CRITICAL_SECTION2(PyValue_AsKnownObjectBorrow(so), PyValue_AsKnownObjectBorrow(other));
    return_value = set_symmetric_difference_impl((PySetObject *)PyValue_AsKnownObjectBorrow(so), other);
    Py_END_CRITICAL_SECTION2();

    return return_value;
}

PyDoc_STRVAR(set_issubset__doc__,
"issubset($self, other, /)\n"
"--\n"
"\n"
"Report whether another set contains this set.");

#define SET_ISSUBSET_METHODDEF    \
    {"issubset", _PyCFunction_CAST(set_issubset), METH_VALUE|METH_O, set_issubset__doc__},

static PyValue
set_issubset_impl(PySetObject *so, PyValueBorrow other);

static PyValue
set_issubset(PyValueBorrow so, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow other;

    other = PyValue_Borrow(args[0]);
    Py_BEGIN_CRITICAL_SECTION2(PyValue_AsKnownObjectBorrow(so), PyValue_AsKnownObjectBorrow(other));
    return_value = set_issubset_impl((PySetObject *)PyValue_AsKnownObjectBorrow(so), other);
    Py_END_CRITICAL_SECTION2();

    return return_value;
}

PyDoc_STRVAR(set_issuperset__doc__,
"issuperset($self, other, /)\n"
"--\n"
"\n"
"Report whether this set contains another set.");

#define SET_ISSUPERSET_METHODDEF    \
    {"issuperset", _PyCFunction_CAST(set_issuperset), METH_VALUE|METH_O, set_issuperset__doc__},

static PyValue
set_issuperset_impl(PySetObject *so, PyValueBorrow other);

static PyValue
set_issuperset(PyValueBorrow so, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow other;

    other = PyValue_Borrow(args[0]);
    Py_BEGIN_CRITICAL_SECTION2(PyValue_AsKnownObjectBorrow(so), PyValue_AsKnownObjectBorrow(other));
    return_value = set_issuperset_impl((PySetObject *)PyValue_AsKnownObjectBorrow(so), other);
    Py_END_CRITICAL_SECTION2();

    return return_value;
}

PyDoc_STRVAR(set_add__doc__,
"add($self, object, /)\n"
"--\n"
"\n"
"Add an element to a set.\n"
"\n"
"This has no effect if the element is already present.");

#define SET_ADD_METHODDEF    \
    {"add", _PyCFunction_CAST(set_add), METH_VALUE|METH_O, set_add__doc__},

static PyValue
set_add_impl(PySetObject *so, PyValueBorrow key);

static PyValue
set_add(PyValueBorrow so, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow key;

    key = PyValue_Borrow(args[0]);
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(so));
    return_value = set_add_impl((PySetObject *)PyValue_AsKnownObjectBorrow(so), key);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(set___contains____doc__,
"__contains__($self, object, /)\n"
"--\n"
"\n"
"x.__contains__(y) <==> y in x.");

#define SET___CONTAINS___METHODDEF    \
    {"__contains__", _PyCFunction_CAST(set___contains__), METH_VALUE|METH_O|METH_COEXIST, set___contains____doc__},

static PyValue
set___contains___impl(PySetObject *so, PyValueBorrow key);

static PyValue
set___contains__(PyValueBorrow so, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow key;

    key = PyValue_Borrow(args[0]);
    return_value = set___contains___impl((PySetObject *)PyValue_AsKnownObjectBorrow(so), key);

    return return_value;
}

PyDoc_STRVAR(frozenset___contains____doc__,
"__contains__($self, object, /)\n"
"--\n"
"\n"
"x.__contains__(y) <==> y in x.");

#define FROZENSET___CONTAINS___METHODDEF    \
    {"__contains__", _PyCFunction_CAST(frozenset___contains__), METH_VALUE|METH_O|METH_COEXIST, frozenset___contains____doc__},

static PyValue
frozenset___contains___impl(PySetObject *so, PyValueBorrow key);

static PyValue
frozenset___contains__(PyValueBorrow so, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow key;

    key = PyValue_Borrow(args[0]);
    return_value = frozenset___contains___impl((PySetObject *)PyValue_AsKnownObjectBorrow(so), key);

    return return_value;
}

PyDoc_STRVAR(set_remove__doc__,
"remove($self, object, /)\n"
"--\n"
"\n"
"Remove an element from a set; it must be a member.\n"
"\n"
"If the element is not a member, raise a KeyError.");

#define SET_REMOVE_METHODDEF    \
    {"remove", _PyCFunction_CAST(set_remove), METH_VALUE|METH_O, set_remove__doc__},

static PyValue
set_remove_impl(PySetObject *so, PyValueBorrow key);

static PyValue
set_remove(PyValueBorrow so, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow key;

    key = PyValue_Borrow(args[0]);
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(so));
    return_value = set_remove_impl((PySetObject *)PyValue_AsKnownObjectBorrow(so), key);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(set_discard__doc__,
"discard($self, object, /)\n"
"--\n"
"\n"
"Remove an element from a set if it is a member.\n"
"\n"
"Unlike set.remove(), the discard() method does not raise\n"
"an exception when an element is missing from the set.");

#define SET_DISCARD_METHODDEF    \
    {"discard", _PyCFunction_CAST(set_discard), METH_VALUE|METH_O, set_discard__doc__},

static PyValue
set_discard_impl(PySetObject *so, PyValueBorrow key);

static PyValue
set_discard(PyValueBorrow so, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow key;

    key = PyValue_Borrow(args[0]);
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(so));
    return_value = set_discard_impl((PySetObject *)PyValue_AsKnownObjectBorrow(so), key);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(set___reduce____doc__,
"__reduce__($self, /)\n"
"--\n"
"\n"
"Return state information for pickling.");

#define SET___REDUCE___METHODDEF    \
    {"__reduce__", _PyCFunction_CAST(set___reduce__), METH_VALUE|METH_NOARGS, set___reduce____doc__},

static PyValue
set___reduce___impl(PySetObject *so);

static PyValue
set___reduce__(PyValueBorrow so, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(so));
    return_value = set___reduce___impl((PySetObject *)PyValue_AsKnownObjectBorrow(so));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(set___sizeof____doc__,
"__sizeof__($self, /)\n"
"--\n"
"\n"
"S.__sizeof__() -> size of S in memory, in bytes.");

#define SET___SIZEOF___METHODDEF    \
    {"__sizeof__", _PyCFunction_CAST(set___sizeof__), METH_VALUE|METH_NOARGS, set___sizeof____doc__},

static PyValue
set___sizeof___impl(PySetObject *so);

static PyValue
set___sizeof__(PyValueBorrow so, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(so));
    return_value = set___sizeof___impl((PySetObject *)PyValue_AsKnownObjectBorrow(so));
    Py_END_CRITICAL_SECTION();

    return return_value;
}
/*[clinic end generated code: output=946a2e30483266a3 input=a9049054013a1b77]*/
