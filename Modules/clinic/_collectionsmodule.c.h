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
#include "pycore_modsupport.h"    // _PyArg_CheckPositional()
#include "pycore_tuple.h"         // _PyTuple_ITEMS()
#include "pycore_value_call.h"    // METH_VALUE
#include "pycore_value_ops.h"     // _PyValue_Index()
#include "pycore_value_slots.h"   // _PyValue_THUNK_EXIT()

PyDoc_STRVAR(deque_pop__doc__,
"pop($self, /)\n"
"--\n"
"\n"
"Remove and return the rightmost element.");

#define DEQUE_POP_METHODDEF    \
    {"pop", _PyCFunction_CAST(deque_pop), METH_VALUE|METH_NOARGS, deque_pop__doc__},

static PyValue
deque_pop_impl(dequeobject *deque);

static PyValue
deque_pop(PyValueBorrow deque, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(deque));
    return_value = deque_pop_impl((dequeobject *)PyValue_AsKnownObjectBorrow(deque));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(deque_popleft__doc__,
"popleft($self, /)\n"
"--\n"
"\n"
"Remove and return the leftmost element.");

#define DEQUE_POPLEFT_METHODDEF    \
    {"popleft", _PyCFunction_CAST(deque_popleft), METH_VALUE|METH_NOARGS, deque_popleft__doc__},

static PyValue
deque_popleft_impl(dequeobject *deque);

static PyValue
deque_popleft(PyValueBorrow deque, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(deque));
    return_value = deque_popleft_impl((dequeobject *)PyValue_AsKnownObjectBorrow(deque));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(deque_append__doc__,
"append($self, item, /)\n"
"--\n"
"\n"
"Add an element to the right side of the deque.");

#define DEQUE_APPEND_METHODDEF    \
    {"append", _PyCFunction_CAST(deque_append), METH_VALUE|METH_O, deque_append__doc__},

static PyValue
deque_append_impl(dequeobject *deque, PyValueBorrow item);

static PyValue
deque_append(PyValueBorrow deque, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow item;

    item = PyValue_Borrow(args[0]);
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(deque));
    return_value = deque_append_impl((dequeobject *)PyValue_AsKnownObjectBorrow(deque), item);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(deque_appendleft__doc__,
"appendleft($self, item, /)\n"
"--\n"
"\n"
"Add an element to the left side of the deque.");

#define DEQUE_APPENDLEFT_METHODDEF    \
    {"appendleft", _PyCFunction_CAST(deque_appendleft), METH_VALUE|METH_O, deque_appendleft__doc__},

static PyValue
deque_appendleft_impl(dequeobject *deque, PyValueBorrow item);

static PyValue
deque_appendleft(PyValueBorrow deque, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow item;

    item = PyValue_Borrow(args[0]);
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(deque));
    return_value = deque_appendleft_impl((dequeobject *)PyValue_AsKnownObjectBorrow(deque), item);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(deque_extend__doc__,
"extend($self, iterable, /)\n"
"--\n"
"\n"
"Extend the right side of the deque with elements from the iterable.");

#define DEQUE_EXTEND_METHODDEF    \
    {"extend", _PyCFunction_CAST(deque_extend), METH_VALUE|METH_O, deque_extend__doc__},

static PyValue
deque_extend_impl(dequeobject *deque, PyValueBorrow iterable);

static PyValue
deque_extend(PyValueBorrow deque, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow iterable;

    iterable = PyValue_Borrow(args[0]);
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(deque));
    return_value = deque_extend_impl((dequeobject *)PyValue_AsKnownObjectBorrow(deque), iterable);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(deque_extendleft__doc__,
"extendleft($self, iterable, /)\n"
"--\n"
"\n"
"Extend the left side of the deque with elements from the iterable.");

#define DEQUE_EXTENDLEFT_METHODDEF    \
    {"extendleft", _PyCFunction_CAST(deque_extendleft), METH_VALUE|METH_O, deque_extendleft__doc__},

static PyValue
deque_extendleft_impl(dequeobject *deque, PyValueBorrow iterable);

static PyValue
deque_extendleft(PyValueBorrow deque, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow iterable;

    iterable = PyValue_Borrow(args[0]);
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(deque));
    return_value = deque_extendleft_impl((dequeobject *)PyValue_AsKnownObjectBorrow(deque), iterable);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(deque_copy__doc__,
"copy($self, /)\n"
"--\n"
"\n"
"Return a shallow copy of a deque.");

#define DEQUE_COPY_METHODDEF    \
    {"copy", _PyCFunction_CAST(deque_copy), METH_VALUE|METH_NOARGS, deque_copy__doc__},

static PyValue
deque_copy_impl(dequeobject *deque);

static PyValue
deque_copy(PyValueBorrow deque, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(deque));
    return_value = deque_copy_impl((dequeobject *)PyValue_AsKnownObjectBorrow(deque));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(deque___copy____doc__,
"__copy__($self, /)\n"
"--\n"
"\n"
"Return a shallow copy of a deque.");

#define DEQUE___COPY___METHODDEF    \
    {"__copy__", _PyCFunction_CAST(deque___copy__), METH_VALUE|METH_NOARGS, deque___copy____doc__},

static PyValue
deque___copy___impl(dequeobject *deque);

static PyValue
deque___copy__(PyValueBorrow deque, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(deque));
    return_value = deque___copy___impl((dequeobject *)PyValue_AsKnownObjectBorrow(deque));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

static PyObject *
deque_concat_impl(dequeobject *deque, PyObject *other);

static PyObject *
deque_concat(PyObject *deque, PyObject *other)
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(deque);
    return_value = deque_concat_impl((dequeobject *)deque, other);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(deque_clearmethod__doc__,
"clear($self, /)\n"
"--\n"
"\n"
"Remove all elements from the deque.");

#define DEQUE_CLEARMETHOD_METHODDEF    \
    {"clear", _PyCFunction_CAST(deque_clearmethod), METH_VALUE|METH_NOARGS, deque_clearmethod__doc__},

static PyValue
deque_clearmethod_impl(dequeobject *deque);

static PyValue
deque_clearmethod(PyValueBorrow deque, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(deque));
    return_value = deque_clearmethod_impl((dequeobject *)PyValue_AsKnownObjectBorrow(deque));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

static PyObject *
deque_inplace_repeat_impl(dequeobject *deque, Py_ssize_t n);

static PyObject *
deque_inplace_repeat(PyObject *deque, Py_ssize_t n)
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(deque);
    return_value = deque_inplace_repeat_impl((dequeobject *)deque, n);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(deque_rotate__doc__,
"rotate($self, n=1, /)\n"
"--\n"
"\n"
"Rotate the deque n steps to the right.  If n is negative, rotates left.");

#define DEQUE_ROTATE_METHODDEF    \
    {"rotate", _PyCFunction_CAST(deque_rotate), METH_VALUE, deque_rotate__doc__},

static PyValue
deque_rotate_impl(dequeobject *deque, Py_ssize_t n);

static PyValue
deque_rotate(PyValueBorrow deque, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_ssize_t n = 1;

    if (!_PyArg_CheckPositional("rotate", nargs, 0, 1)) {
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
        n = ival;
    }
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(deque));
    return_value = deque_rotate_impl((dequeobject *)PyValue_AsKnownObjectBorrow(deque), n);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(deque_reverse__doc__,
"reverse($self, /)\n"
"--\n"
"\n"
"Reverse *IN PLACE*.");

#define DEQUE_REVERSE_METHODDEF    \
    {"reverse", _PyCFunction_CAST(deque_reverse), METH_VALUE|METH_NOARGS, deque_reverse__doc__},

static PyValue
deque_reverse_impl(dequeobject *deque);

static PyValue
deque_reverse(PyValueBorrow deque, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(deque));
    return_value = deque_reverse_impl((dequeobject *)PyValue_AsKnownObjectBorrow(deque));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(deque_count__doc__,
"count($self, value, /)\n"
"--\n"
"\n"
"Return number of occurrences of value.");

#define DEQUE_COUNT_METHODDEF    \
    {"count", _PyCFunction_CAST(deque_count), METH_VALUE|METH_O, deque_count__doc__},

static PyValue
deque_count_impl(dequeobject *deque, PyValueBorrow v);

static PyValue
deque_count(PyValueBorrow deque, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow v;

    v = PyValue_Borrow(args[0]);
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(deque));
    return_value = deque_count_impl((dequeobject *)PyValue_AsKnownObjectBorrow(deque), v);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

static int
deque_contains_impl(dequeobject *deque, PyObject *v);

static int
deque_contains(PyObject *deque, PyObject *v)
{
    int return_value = -1;

    Py_BEGIN_CRITICAL_SECTION(deque);
    return_value = deque_contains_impl((dequeobject *)deque, v);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(deque_index__doc__,
"index($self, value, [start, [stop]])\n"
"--\n"
"\n"
"Return first index of value.\n"
"\n"
"Raises ValueError if the value is not present.");

#define DEQUE_INDEX_METHODDEF    \
    {"index", _PyCFunction_CAST(deque_index), METH_VALUE, deque_index__doc__},

static PyValue
deque_index_impl(dequeobject *deque, PyValueBorrow v, Py_ssize_t start,
                 Py_ssize_t stop);

static PyValue
deque_index(PyValueBorrow deque, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow v;
    Py_ssize_t start = 0;
    Py_ssize_t stop = PY_SSIZE_T_MAX;

    if (!_PyArg_CheckPositional("index", nargs, 1, 3)) {
        goto exit;
    }
    v = PyValue_Borrow(args[0]);
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
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(deque));
    return_value = deque_index_impl((dequeobject *)PyValue_AsKnownObjectBorrow(deque), v, start, stop);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(deque_insert__doc__,
"insert($self, index, value, /)\n"
"--\n"
"\n"
"Insert value before index.");

#define DEQUE_INSERT_METHODDEF    \
    {"insert", _PyCFunction_CAST(deque_insert), METH_VALUE, deque_insert__doc__},

static PyValue
deque_insert_impl(dequeobject *deque, Py_ssize_t index, PyValueBorrow value);

static PyValue
deque_insert(PyValueBorrow deque, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_ssize_t index;
    PyValueBorrow value;

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
    value = PyValue_Borrow(args[1]);
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(deque));
    return_value = deque_insert_impl((dequeobject *)PyValue_AsKnownObjectBorrow(deque), index, value);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

static PyObject *
deque_item_impl(dequeobject *deque, Py_ssize_t i);

static PyObject *
deque_item(PyObject *deque, Py_ssize_t i)
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(deque);
    return_value = deque_item_impl((dequeobject *)deque, i);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(deque_remove__doc__,
"remove($self, value, /)\n"
"--\n"
"\n"
"Remove first occurrence of value.");

#define DEQUE_REMOVE_METHODDEF    \
    {"remove", _PyCFunction_CAST(deque_remove), METH_VALUE|METH_O, deque_remove__doc__},

static PyValue
deque_remove_impl(dequeobject *deque, PyValueBorrow value);

static PyValue
deque_remove(PyValueBorrow deque, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow value;

    value = PyValue_Borrow(args[0]);
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(deque));
    return_value = deque_remove_impl((dequeobject *)PyValue_AsKnownObjectBorrow(deque), value);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

static int
deque_ass_item_impl(dequeobject *deque, Py_ssize_t i, PyObject *v);

static int
deque_ass_item(PyObject *deque, Py_ssize_t i, PyObject *v)
{
    int return_value = -1;

    Py_BEGIN_CRITICAL_SECTION(deque);
    return_value = deque_ass_item_impl((dequeobject *)deque, i, v);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(deque___reduce____doc__,
"__reduce__($self, /)\n"
"--\n"
"\n"
"Return state information for pickling.");

#define DEQUE___REDUCE___METHODDEF    \
    {"__reduce__", _PyCFunction_CAST(deque___reduce__), METH_VALUE|METH_NOARGS, deque___reduce____doc__},

static PyValue
deque___reduce___impl(dequeobject *deque);

static PyValue
deque___reduce__(PyValueBorrow deque, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = deque___reduce___impl((dequeobject *)PyValue_AsKnownObjectBorrow(deque));

    return return_value;
}

PyDoc_STRVAR(deque_init__doc__,
"deque([iterable[, maxlen]])\n"
"--\n"
"\n"
"A list-like sequence optimized for data accesses near its endpoints.");

static int
deque_init_impl(dequeobject *deque, PyValueBorrow iterable,
                PyValueBorrow maxlenobj);

static int
deque_init_value(PyObject *deque, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    int return_value = -1;
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(iterable)), _PyValue_STATIC_IMMORTAL(&_Py_ID(maxlen)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"iterable", "maxlen", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "deque",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    PyValueBorrow iterable = PyValue_Borrow(PyValue_NULL);
    PyValueBorrow maxlenobj = PyValue_Borrow(PyValue_NULL);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        iterable = PyValue_Borrow(args[_idx[0]]);
    }
    if (_idx[1] >= 0) {
        maxlenobj = PyValue_Borrow(args[_idx[1]]);
    }
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(deque));
    return_value = deque_init_impl((dequeobject *)deque, iterable, maxlenobj);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

static int
deque_init(PyObject *deque, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return deque_init_value(deque, _PyTuple_ITEMS(args), nargs, NULL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return -1;
    }
    int result = deque_init_value(deque, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return result;
}

PyDoc_STRVAR(deque___sizeof____doc__,
"__sizeof__($self, /)\n"
"--\n"
"\n"
"Return the size of the deque in memory, in bytes.");

#define DEQUE___SIZEOF___METHODDEF    \
    {"__sizeof__", _PyCFunction_CAST(deque___sizeof__), METH_VALUE|METH_NOARGS, deque___sizeof____doc__},

static PyValue
deque___sizeof___impl(dequeobject *deque);

static PyValue
deque___sizeof__(PyValueBorrow deque, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(deque));
    return_value = deque___sizeof___impl((dequeobject *)PyValue_AsKnownObjectBorrow(deque));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(deque___reversed____doc__,
"__reversed__($self, /)\n"
"--\n"
"\n"
"Return a reverse iterator over the deque.");

#define DEQUE___REVERSED___METHODDEF    \
    {"__reversed__", _PyCFunction_CAST(deque___reversed__), METH_VALUE|METH_NOARGS, deque___reversed____doc__},

static PyValue
deque___reversed___impl(dequeobject *deque);

static PyValue
deque___reversed__(PyValueBorrow deque, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = deque___reversed___impl((dequeobject *)PyValue_AsKnownObjectBorrow(deque));

    return return_value;
}

PyDoc_STRVAR(_collections__count_elements__doc__,
"_count_elements($module, mapping, iterable, /)\n"
"--\n"
"\n"
"Count elements in the iterable, updating the mapping");

#define _COLLECTIONS__COUNT_ELEMENTS_METHODDEF    \
    {"_count_elements", _PyCFunction_CAST(_collections__count_elements), METH_VALUE, _collections__count_elements__doc__},

static PyValue
_collections__count_elements_impl(PyObject *module, PyValueBorrow mapping,
                                  PyValueBorrow iterable);

static PyValue
_collections__count_elements(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow mapping;
    PyValueBorrow iterable;

    if (!_PyArg_CheckPositional("_count_elements", nargs, 2, 2)) {
        goto exit;
    }
    mapping = PyValue_Borrow(args[0]);
    iterable = PyValue_Borrow(args[1]);
    return_value = _collections__count_elements_impl(PyValue_AsKnownObjectBorrow(module), mapping, iterable);

exit:
    return return_value;
}

static PyValue
tuplegetter_new_impl(PyTypeObject *type, Py_ssize_t index, PyValueBorrow doc);

static PyValue
tuplegetter_new_value(PyTypeObject *type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyTypeObject *base_tp = clinic_state()->tuplegetter_type;
    Py_ssize_t index;
    PyValueBorrow doc;

    if ((type == base_tp || type->tp_init == base_tp->tp_init) &&
        !_PyArg_NoKwnames("_tuplegetter", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("_tuplegetter", nargs, 2, 2)) {
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
    doc = PyValue_Borrow(args[1]);
    return_value = tuplegetter_new_impl(type, index, doc);

exit:
    return return_value;
}

Py_VALUE_ENTRY static PyObject *
tuplegetter_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _PyValue_THUNK_EXIT(tuplegetter_new_value(type, _PyTuple_ITEMS(args), nargs, NULL), BOX_SITE_CALL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return NULL;
    }
    PyValue result = tuplegetter_new_value(type, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return _PyValue_THUNK_EXIT(result, BOX_SITE_CALL);
}
/*[clinic end generated code: output=aaac4c61e8e1f0d2 input=a9049054013a1b77]*/
