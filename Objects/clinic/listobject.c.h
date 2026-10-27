/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_critical_section.h"// Py_BEGIN_CRITICAL_SECTION()
#include "pycore_modsupport.h"    // _PyArg_CheckPositional()

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
    {"insert", _PyCFunction_CAST(list_insert), METH_FASTCALL, list_insert__doc__},

static PyObject *
list_insert_impl(PyListObject *self, Py_ssize_t index, PyValue object);

static PyValue
list_insert(PyObject *self, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    Py_ssize_t index;
    PyValue object;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("insert", nargs, 2, 2)) {
        goto exit;
    }
    {
        Py_ssize_t ival = -1;
        PyValue ivalue = PyNumber_IndexValue(args[0]);
        if (!PyValue_IsNull(ivalue)) {
            ival = PyLong_AsSsize_tValue(ivalue);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        index = ival;
    }
    object = args[1];
    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = list_insert_impl((PyListObject *)self, index, object);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(py_list_clear__doc__,
"clear($self, /)\n"
"--\n"
"\n"
"Remove all items from list.");

#define PY_LIST_CLEAR_METHODDEF    \
    {"clear", (PyCFunction)py_list_clear, METH_NOARGS, py_list_clear__doc__},

static PyObject *
py_list_clear_impl(PyListObject *self);

static PyObject *
py_list_clear(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = py_list_clear_impl((PyListObject *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(list_copy__doc__,
"copy($self, /)\n"
"--\n"
"\n"
"Return a shallow copy of the list.");

#define LIST_COPY_METHODDEF    \
    {"copy", (PyCFunction)list_copy, METH_NOARGS, list_copy__doc__},

static PyObject *
list_copy_impl(PyListObject *self);

static PyObject *
list_copy(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = list_copy_impl((PyListObject *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(list_append__doc__,
"append($self, object, /)\n"
"--\n"
"\n"
"Append object to the end of the list.");

#define LIST_APPEND_METHODDEF    \
    {"append", _PyCFunction_CAST(list_append), METH_O, list_append__doc__},

static PyObject *
list_append_impl(PyListObject *self, PyValue object);

static PyValue
list_append(PyObject *self, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyValue object;
    PyObject *_return_value;

    object = arg;
    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = list_append_impl((PyListObject *)self, object);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

    return return_value;
}

PyDoc_STRVAR(list_extend__doc__,
"extend($self, iterable, /)\n"
"--\n"
"\n"
"Extend list by appending elements from the iterable.");

#define LIST_EXTEND_METHODDEF    \
    {"extend", _PyCFunction_CAST(list_extend), METH_O, list_extend__doc__},

static PyObject *
list_extend_impl(PyListObject *self, PyObject *iterable);

static PyValue
list_extend(PyObject *self, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyObject *iterable;
    PyObject *_return_value;

    iterable = PyValue_Object(arg);
    if (iterable == NULL) {
        goto exit;
    }
    _return_value = list_extend_impl((PyListObject *)self, iterable);
    return_value = PyValue_FromResult(_return_value);

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
    {"pop", _PyCFunction_CAST(list_pop), METH_FASTCALL, list_pop__doc__},

static PyValue
list_pop_impl(PyListObject *self, Py_ssize_t index);

static PyValue
list_pop(PyObject *self, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    Py_ssize_t index = -1;
    PyValue _return_value;

    if (!_PyArg_CheckPositional("pop", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    {
        Py_ssize_t ival = -1;
        PyValue ivalue = PyNumber_IndexValue(args[0]);
        if (!PyValue_IsNull(ivalue)) {
            ival = PyLong_AsSsize_tValue(ivalue);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        index = ival;
    }
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = list_pop_impl((PyListObject *)self, index);
    Py_END_CRITICAL_SECTION();
    return_value = _return_value;

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
    {"sort", _PyCFunction_CAST(list_sort), METH_FASTCALL|METH_KEYWORDS, list_sort__doc__},

static PyObject *
list_sort_impl(PyListObject *self, PyObject *keyfunc, int reverse);

static PyValue
list_sort(PyObject *self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(key), &_Py_ID(reverse), },
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
    PyValue argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *keyfunc = Py_None;
    int reverse = 0;
    PyObject *_return_value;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 0, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (!PyValue_IsNull(args[0])) {
        keyfunc = PyValue_Object(args[0]);
        if (keyfunc == NULL) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    reverse = PyObject_IsTrueValue(args[1]);
    if (reverse < 0) {
        goto exit;
    }
skip_optional_kwonly:
    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = list_sort_impl((PyListObject *)self, keyfunc, reverse);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(list_reverse__doc__,
"reverse($self, /)\n"
"--\n"
"\n"
"Reverse *IN PLACE*.");

#define LIST_REVERSE_METHODDEF    \
    {"reverse", (PyCFunction)list_reverse, METH_NOARGS, list_reverse__doc__},

static PyObject *
list_reverse_impl(PyListObject *self);

static PyObject *
list_reverse(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = list_reverse_impl((PyListObject *)self);
    Py_END_CRITICAL_SECTION();

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
    {"index", _PyCFunction_CAST(list_index), METH_FASTCALL, list_index__doc__},

static PyValue
list_index_impl(PyListObject *self, PyValue value, Py_ssize_t start,
                Py_ssize_t stop);

static PyValue
list_index(PyObject *self, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    PyValue value;
    Py_ssize_t start = 0;
    Py_ssize_t stop = PY_SSIZE_T_MAX;
    PyValue _return_value;

    if (!_PyArg_CheckPositional("index", nargs, 1, 3)) {
        goto exit;
    }
    value = args[0];
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
    _return_value = list_index_impl((PyListObject *)self, value, start, stop);
    return_value = _return_value;

exit:
    return return_value;
}

PyDoc_STRVAR(list_count__doc__,
"count($self, value, /)\n"
"--\n"
"\n"
"Return number of occurrences of value.");

#define LIST_COUNT_METHODDEF    \
    {"count", _PyCFunction_CAST(list_count), METH_O, list_count__doc__},

static PyValue
list_count_impl(PyListObject *self, PyValue value);

static PyValue
list_count(PyObject *self, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyValue value;
    PyValue _return_value;

    value = arg;
    _return_value = list_count_impl((PyListObject *)self, value);
    return_value = _return_value;

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
    {"remove", _PyCFunction_CAST(list_remove), METH_O, list_remove__doc__},

static PyObject *
list_remove_impl(PyListObject *self, PyValue value);

static PyValue
list_remove(PyObject *self, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyValue value;
    PyObject *_return_value;

    value = arg;
    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = list_remove_impl((PyListObject *)self, value);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

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
list___init___impl(PyListObject *self, PyObject *iterable);

static int
list___init__(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int return_value = -1;
    PyTypeObject *base_tp = &PyList_Type;
    PyObject *iterable = NULL;

    if ((Py_IS_TYPE(self, base_tp) ||
         Py_TYPE(self)->tp_new == base_tp->tp_new) &&
        !_PyArg_NoKeywords("list", kwargs)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("list", PyTuple_GET_SIZE(args), 0, 1)) {
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
    return_value = list___init___impl((PyListObject *)self, iterable);

exit:
    return return_value;
}

PyDoc_STRVAR(list___sizeof____doc__,
"__sizeof__($self, /)\n"
"--\n"
"\n"
"Return the size of the list in memory, in bytes.");

#define LIST___SIZEOF___METHODDEF    \
    {"__sizeof__", (PyCFunction)list___sizeof__, METH_NOARGS, list___sizeof____doc__},

static PyObject *
list___sizeof___impl(PyListObject *self);

static PyObject *
list___sizeof__(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return list___sizeof___impl((PyListObject *)self);
}

PyDoc_STRVAR(list___reversed____doc__,
"__reversed__($self, /)\n"
"--\n"
"\n"
"Return a reverse iterator over the list.");

#define LIST___REVERSED___METHODDEF    \
    {"__reversed__", (PyCFunction)list___reversed__, METH_NOARGS, list___reversed____doc__},

static PyObject *
list___reversed___impl(PyListObject *self);

static PyObject *
list___reversed__(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return list___reversed___impl((PyListObject *)self);
}
/*[clinic end generated code: output=7baec8aaa977defd input=a9049054013a1b77]*/
