/*[clinic input]
preserve
[clinic start generated code]*/

#ifndef Py_VALUE_CORE
#  error "value-emission Clinic output requires Py_VALUE_CORE"
#endif

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#  include "pycore_value.h"       // _PyValue_STATIC_IMMORTAL()
#endif
#include "pycore_critical_section.h"// Py_BEGIN_CRITICAL_SECTION()
#include "pycore_long_value.h"    // _PyValue_IntAsSsize_t()
#include "pycore_modsupport.h"    // _PyArg_NoKwnamesValue()
#include "pycore_tuple.h"         // _PyTuple_ITEMS()
#include "pycore_value_call.h"    // METH_VALUE
#include "pycore_value_ops.h"     // _PyValue_Index()

static PyObject *
list_repr_impl(PyListObject *v);

static PyObject *
list_repr(PyObject *v)
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(v);
    return_value = list_repr_impl((PyListObject *)v);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

static PyObject *
list_repeat_impl(PyListObject *a, Py_ssize_t n);

static PyObject *
list_repeat(PyObject *a, Py_ssize_t n)
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(a);
    return_value = list_repeat_impl((PyListObject *)a, n);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

static PyObject *
list_inplace_repeat_impl(PyListObject *self, Py_ssize_t n);

static PyObject *
list_inplace_repeat(PyObject *self, Py_ssize_t n)
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = list_inplace_repeat_impl((PyListObject *)self, n);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

static int
list_ass_item_impl(PyListObject *a, Py_ssize_t i, PyObject *v);

static int
list_ass_item(PyObject *a, Py_ssize_t i, PyObject *v)
{
    int return_value = -1;

    Py_BEGIN_CRITICAL_SECTION(a);
    return_value = list_ass_item_impl((PyListObject *)a, i, v);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(list_insert__doc__,
"insert($self, index, object, /)\n"
"--\n"
"\n"
"Insert object before index.");

#define LIST_INSERT_METHODDEF    \
    {"insert", _PyCFunction_CAST(list_insert), METH_VALUE, list_insert__doc__},

static PyValue
list_insert_impl(PyListObject *self, Py_ssize_t index, PyValueBorrow object);

static PyValue
list_insert(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_ssize_t index;
    PyValueBorrow object;

    if (!_PyArg_NoKwnamesValueMethod(&PyList_Type, "insert", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("insert", nargs, 2, 2)) {
        goto exit;
    }
    {
        Py_ssize_t ival = -1;
        PyValue iobj = _PyValue_Index(PyValue_Borrow(args[0]));
        if (!PyValue_IsNull(iobj)) {
            ival = _PyValue_IntAsSsize_t(PyValue_Borrow(iobj));
            PyValue_CLOSE(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        index = ival;
    }
    object = PyValue_Borrow(args[1]);
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = list_insert_impl((PyListObject *)PyValue_AsKnownObjectBorrow(self), index, object);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(py_list_clear__doc__,
"clear($self, /)\n"
"--\n"
"\n"
"Remove all items from list.");

#define PY_LIST_CLEAR_METHODDEF    \
    {"clear", _PyCFunction_CAST(py_list_clear), METH_VALUE, py_list_clear__doc__},

static PyValue
py_list_clear_impl(PyListObject *self);

static PyValue
py_list_clear(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValueMethod(&PyList_Type, "clear", nargs, kwnames, 0)) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = py_list_clear_impl((PyListObject *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(list_copy__doc__,
"copy($self, /)\n"
"--\n"
"\n"
"Return a shallow copy of the list.");

#define LIST_COPY_METHODDEF    \
    {"copy", _PyCFunction_CAST(list_copy), METH_VALUE, list_copy__doc__},

static PyValue
list_copy_impl(PyListObject *self);

static PyValue
list_copy(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValueMethod(&PyList_Type, "copy", nargs, kwnames, 0)) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = list_copy_impl((PyListObject *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(list_append__doc__,
"append($self, object, /)\n"
"--\n"
"\n"
"Append object to the end of the list.");

#define LIST_APPEND_METHODDEF    \
    {"append", _PyCFunction_CAST(list_append), METH_VALUE, list_append__doc__},

static PyValue
list_append_impl(PyListObject *self, PyValueBorrow object);

static PyValue
list_append(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow object;

    if (!_PyArg_CheckArityValueMethod(&PyList_Type, "append", nargs, kwnames, 1)) {
        goto exit;
    }
    object = PyValue_Borrow(args[0]);
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = list_append_impl((PyListObject *)PyValue_AsKnownObjectBorrow(self), object);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(list_extend__doc__,
"extend($self, iterable, /)\n"
"--\n"
"\n"
"Extend list by appending elements from the iterable.");

#define LIST_EXTEND_METHODDEF    \
    {"extend", _PyCFunction_CAST(list_extend), METH_VALUE, list_extend__doc__},

static PyValue
list_extend_impl(PyListObject *self, PyValueBorrow iterable);

static PyValue
list_extend(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow iterable;

    if (!_PyArg_CheckArityValueMethod(&PyList_Type, "extend", nargs, kwnames, 1)) {
        goto exit;
    }
    iterable = PyValue_Borrow(args[0]);
    return_value = list_extend_impl((PyListObject *)PyValue_AsKnownObjectBorrow(self), iterable);

exit:
    return return_value;
}

PyDoc_STRVAR(list_pop__doc__,
"pop($self, index=-1, /)\n"
"--\n"
"\n"
"Remove and return item at index (default last).\n"
"\n"
"Raises IndexError if list is empty or index is out of range.");

#define LIST_POP_METHODDEF    \
    {"pop", _PyCFunction_CAST(list_pop), METH_VALUE, list_pop__doc__},

static PyValue
list_pop_impl(PyListObject *self, Py_ssize_t index);

static PyValue
list_pop(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_ssize_t index = -1;

    if (!_PyArg_NoKwnamesValueMethod(&PyList_Type, "pop", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("pop", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    {
        Py_ssize_t ival = -1;
        PyValue iobj = _PyValue_Index(PyValue_Borrow(args[0]));
        if (!PyValue_IsNull(iobj)) {
            ival = _PyValue_IntAsSsize_t(PyValue_Borrow(iobj));
            PyValue_CLOSE(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        index = ival;
    }
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = list_pop_impl((PyListObject *)PyValue_AsKnownObjectBorrow(self), index);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(list_sort__doc__,
"sort($self, /, *, key=None, reverse=False)\n"
"--\n"
"\n"
"Sort the list in ascending order and return None.\n"
"\n"
"The sort is in-place (i.e. the list itself is modified) and stable\n"
"(i.e. the order of two equal elements is maintained).\n"
"\n"
"If a key function is given, apply it once to each list item and sort\n"
"them, ascending or descending, according to their function values.\n"
"\n"
"The reverse flag can be set to sort in descending order.");

#define LIST_SORT_METHODDEF    \
    {"sort", _PyCFunction_CAST(list_sort), METH_VALUE, list_sort__doc__},

static PyValue
list_sort_impl(PyListObject *self, PyValueBorrow keyfunc, int reverse);

static PyValue
list_sort(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(key)), _PyValue_STATIC_IMMORTAL(&_Py_ID(reverse)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"key", "reverse", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "sort",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    PyValueBorrow keyfunc = PyValue_Borrow(PyValue_None);
    int reverse = 0;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 0, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        keyfunc = PyValue_Borrow(args[_idx[0]]);
    }
    if (_idx[1] >= 0) {
        reverse = _PyValue_IsTrue(PyValue_Borrow(args[_idx[1]]));
        if (reverse < 0) {
            goto exit;
        }
    }
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = list_sort_impl((PyListObject *)PyValue_AsKnownObjectBorrow(self), keyfunc, reverse);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(list_reverse__doc__,
"reverse($self, /)\n"
"--\n"
"\n"
"Reverse *IN PLACE*.");

#define LIST_REVERSE_METHODDEF    \
    {"reverse", _PyCFunction_CAST(list_reverse), METH_VALUE, list_reverse__doc__},

static PyValue
list_reverse_impl(PyListObject *self);

static PyValue
list_reverse(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValueMethod(&PyList_Type, "reverse", nargs, kwnames, 0)) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = list_reverse_impl((PyListObject *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(list_index__doc__,
"index($self, value, start=0, stop=sys.maxsize, /)\n"
"--\n"
"\n"
"Return first index of value.\n"
"\n"
"Raises ValueError if the value is not present.");

#define LIST_INDEX_METHODDEF    \
    {"index", _PyCFunction_CAST(list_index), METH_VALUE, list_index__doc__},

static PyValue
list_index_impl(PyListObject *self, PyValueBorrow value, Py_ssize_t start,
                Py_ssize_t stop);

static PyValue
list_index(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow value;
    Py_ssize_t start = 0;
    Py_ssize_t stop = PY_SSIZE_T_MAX;

    if (!_PyArg_NoKwnamesValueMethod(&PyList_Type, "index", kwnames)) {
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
    return_value = list_index_impl((PyListObject *)PyValue_AsKnownObjectBorrow(self), value, start, stop);

exit:
    return return_value;
}

PyDoc_STRVAR(list_count__doc__,
"count($self, value, /)\n"
"--\n"
"\n"
"Return number of occurrences of value.");

#define LIST_COUNT_METHODDEF    \
    {"count", _PyCFunction_CAST(list_count), METH_VALUE, list_count__doc__},

static PyValue
list_count_impl(PyListObject *self, PyValueBorrow value);

static PyValue
list_count(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow value;

    if (!_PyArg_CheckArityValueMethod(&PyList_Type, "count", nargs, kwnames, 1)) {
        goto exit;
    }
    value = PyValue_Borrow(args[0]);
    return_value = list_count_impl((PyListObject *)PyValue_AsKnownObjectBorrow(self), value);

exit:
    return return_value;
}

PyDoc_STRVAR(list_remove__doc__,
"remove($self, value, /)\n"
"--\n"
"\n"
"Remove first occurrence of value.\n"
"\n"
"Raises ValueError if the value is not present.");

#define LIST_REMOVE_METHODDEF    \
    {"remove", _PyCFunction_CAST(list_remove), METH_VALUE, list_remove__doc__},

static PyValue
list_remove_impl(PyListObject *self, PyValueBorrow value);

static PyValue
list_remove(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow value;

    if (!_PyArg_CheckArityValueMethod(&PyList_Type, "remove", nargs, kwnames, 1)) {
        goto exit;
    }
    value = PyValue_Borrow(args[0]);
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = list_remove_impl((PyListObject *)PyValue_AsKnownObjectBorrow(self), value);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(list___init____doc__,
"list(iterable=(), /)\n"
"--\n"
"\n"
"Built-in mutable sequence.\n"
"\n"
"If no argument is given, the constructor creates a new empty list.\n"
"The argument must be an iterable if specified.");

static int
list___init___impl(PyListObject *self, PyValueBorrow iterable);

static int
list___init___value(PyObject *self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    int return_value = -1;
    PyTypeObject *base_tp = &PyList_Type;
    PyValueBorrow iterable = PyValue_Borrow(PyValue_NULL);

    if ((Py_IS_TYPE(self, base_tp) ||
         Py_TYPE(self)->tp_new == base_tp->tp_new) &&
        !_PyArg_NoKwnames("list", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("list", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    iterable = PyValue_Borrow(args[0]);
skip_optional:
    return_value = list___init___impl((PyListObject *)self, iterable);

exit:
    return return_value;
}

static int
list___init__(PyObject *self, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return list___init___value(self, _PyTuple_ITEMS(args), nargs, NULL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return -1;
    }
    int result = list___init___value(self, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return result;
}

PyDoc_STRVAR(list___sizeof____doc__,
"__sizeof__($self, /)\n"
"--\n"
"\n"
"Return the size of the list in memory, in bytes.");

#define LIST___SIZEOF___METHODDEF    \
    {"__sizeof__", _PyCFunction_CAST(list___sizeof__), METH_VALUE, list___sizeof____doc__},

static PyValue
list___sizeof___impl(PyListObject *self);

static PyValue
list___sizeof__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValueMethod(&PyList_Type, "__sizeof__", nargs, kwnames, 0)) {
        goto exit;
    }
    return_value = list___sizeof___impl((PyListObject *)PyValue_AsKnownObjectBorrow(self));

exit:
    return return_value;
}

PyDoc_STRVAR(list___reversed____doc__,
"__reversed__($self, /)\n"
"--\n"
"\n"
"Return a reverse iterator over the list.");

#define LIST___REVERSED___METHODDEF    \
    {"__reversed__", _PyCFunction_CAST(list___reversed__), METH_VALUE, list___reversed____doc__},

static PyValue
list___reversed___impl(PyListObject *self);

static PyValue
list___reversed__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValueMethod(&PyList_Type, "__reversed__", nargs, kwnames, 0)) {
        goto exit;
    }
    return_value = list___reversed___impl((PyListObject *)PyValue_AsKnownObjectBorrow(self));

exit:
    return return_value;
}
/*[clinic end generated code: output=481f6095cb6adaf5 input=a9049054013a1b77]*/
