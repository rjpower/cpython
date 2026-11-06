/* Bisection algorithms. Drop in replacement for bisect.py

Converted to C by Dmitry Vasiliev (dima at hlabs.spb.ru).
*/

#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#define Py_VALUE_CORE 1
#include "Python.h"
#include "pycore_call.h"          // _PyObject_CallMethod()
#include "pycore_list.h"          // PyList_InsertValue()
#include "pycore_value.h"         // PyValue
#include "pycore_value_ops.h"     // _PyValue_IsTrue()
#include "pycore_value_slots.h"   // _PyType_VALUE_SLOT()

/*[clinic input]
value_emission
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=eb7b17088bfd7215]*/

/*[clinic input]
module _bisect
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=4d56a2b2033b462b]*/

#include "clinic/_bisectmodule.c.h"

typedef struct {
    PyObject *str_insert;
} bisect_state;

static inline bisect_state*
get_bisect_state(PyObject *module)
{
    void *state = PyModule_GetState(module);
    assert(state != NULL);
    return (bisect_state *)state;
}

static ssizeargfunc
get_sq_item_type(PyTypeObject *tp)
{
    // The parts of PySequence_GetItem that we only need to do once
    PySequenceMethods *m = tp->tp_as_sequence;
    if (m && m->sq_item) {
        return m->sq_item;
    }
    const char *msg;
    if (tp->tp_as_mapping && tp->tp_as_mapping->mp_subscript) {
        msg = "%.200s is not a sequence";
    }
    else {
        msg = "'%.200s' object does not support indexing";
    }
    PyErr_Format(PyExc_TypeError, msg, tp->tp_name);
    return NULL;
}

/* Box the three arguments of a bisect call for the object-form search below.
   On failure, any box made is released and an exception is set. */
static int
bisect_box_args(PyValueBorrow a, PyValueBorrow x, PyValueBorrow key,
                PyObject **a_obj, PyObject **x_obj, PyObject **key_obj)
{
    *a_obj = _PyValue_AsObjectAt(a, BOX_SITE_PARSER);
    *x_obj = _PyValue_AsObjectAt(x, BOX_SITE_PARSER);
    *key_obj = _PyValue_AsObjectAt(key, BOX_SITE_PARSER);
    if (*a_obj == NULL || *x_obj == NULL || *key_obj == NULL) {
        Py_CLEAR(*a_obj);
        Py_CLEAR(*x_obj);
        Py_CLEAR(*key_obj);
        return -1;
    }
    return 0;
}

static inline Py_ssize_t
internal_bisect_right(PyObject *list, PyValueBorrow item, Py_ssize_t lo,
                      Py_ssize_t hi, PyValueBorrow key)
{
    PyValue litem = PyValue_NULL;
    Py_ssize_t mid;
    int res;

    if (hi == -1) {
        hi = _PyValue_SequenceSize(PyValue_FromObjectBorrow(list));
        if (hi < 0)
            return -1;
    }
    PyTypeObject *list_type = Py_TYPE(list);
    ssizeargfunc sq_item = get_sq_item_type(list_type);
    if (sq_item == NULL) {
        return -1;
    }
    _Py_itemvaluefunc value_item = _PyType_VALUE_SLOT(list_type, item);
    if (value_item != NULL) {
        _PyType_CHECK_VALUE_ROW(list_type, _Py_VALUE_SLOT_INDEX_item);
    }
    if (Py_EnterRecursiveCall(" in _bisect.bisect_right")) {
        return -1;
    }
    PyTypeObject *tp = PyValue_TYPE(item);
    richcmpfunc compare = tp->tp_richcompare;
    PyObject *item_obj = PyValue_IsObject(item)
        ? PyValue_AsKnownObjectBorrow(item) : NULL;
    while (lo < hi) {
        /* The (size_t)cast ensures that the addition and subsequent division
           are performed as unsigned operations, avoiding difficulties from
           signed overflow.  (See issue 13496.) */
        mid = ((size_t)lo + hi) / 2;
        assert(mid >= 0);
        // PySequence_GetItem, but the two selected slot rows are cached.
        if (value_item != NULL) {
            litem = value_item(list, mid);
            if (PyValue_IsNull(litem)) {
                goto error;
            }
        }
        else {
            PyObject *litem_obj = sq_item(list, mid);
            assert((PyErr_Occurred() == NULL) ^ (litem_obj == NULL));
            if (litem_obj == NULL) {
                goto error;
            }
            litem = _PyValue_FromObjectResult(litem_obj);
        }
        if (!PyValue_IsNone(key)) {
            PyValue newitem = _PyValue_CallOneArg(_PyThreadState_GET(), key,
                                                  litem);
            if (PyValue_IsNull(newitem)) {
                PyValue_CLOSE(litem);
                litem = PyValue_NULL;
                goto error;
            }
            PyValue_CLOSE(litem);
            litem = newitem;
        }
        /* if item < key(list[mid]):
         *     hi = mid
         * else:
         *     lo = mid + 1
         */
        if (compare != NULL && item_obj != NULL && PyValue_IsObject(litem) &&
            Py_IS_TYPE(PyValue_AsKnownObjectBorrow(litem), tp)) {
            // A fast path for comparing objects of the same type
            PyObject *res_obj = compare(item_obj,
                                        PyValue_AsKnownObjectBorrow(litem),
                                        Py_LT);
            if (res_obj == Py_True) {
                Py_DECREF(res_obj);
                PyValue_CLOSE(litem);
                litem = PyValue_NULL;
                hi = mid;
                continue;
            }
            if (res_obj == Py_False) {
                Py_DECREF(res_obj);
                PyValue_CLOSE(litem);
                litem = PyValue_NULL;
                lo = mid + 1;
                continue;
            }
            if (res_obj == NULL) {
                goto error;
            }
            if (res_obj == Py_NotImplemented) {
                Py_DECREF(res_obj);
                compare = NULL;
                res = _PyValue_RichCompareBool(item, litem, Py_LT);
            }
            else {
                res = _PyValue_IsTrue(PyValue_FromObjectBorrow(res_obj));
                Py_DECREF(res_obj);
            }
        }
        else {
            // A default path for comparing arbitrary objects
            res = _PyValue_RichCompareBool(item, litem, Py_LT);
        }
        if (res < 0) {
            goto error;
        }
        PyValue_CLOSE(litem);
        litem = PyValue_NULL;
        if (res)
            hi = mid;
        else
            lo = mid + 1;
    }
    Py_LeaveRecursiveCall();
    return lo;
error:
    Py_LeaveRecursiveCall();
    PyValue_XCLOSE(litem);
    return -1;
}

static Py_ssize_t
bisect_right_values(PyValueBorrow a, PyValueBorrow x, Py_ssize_t lo,
                    Py_ssize_t hi, PyValueBorrow key)
{
    if (lo < 0) {
        PyErr_SetString(PyExc_ValueError, "lo must be non-negative");
        return -1;
    }
    PyObject *a_obj;
    if (PyValue_IsObject(a)) {
        a_obj = PyValue_AsKnownObjectBorrow(a);
    }
    else {
        if (hi == -1) {
            (void)_PyValue_SequenceSize(a);
            return -1;
        }
        if (get_sq_item_type(PyValue_TYPE(a)) == NULL) {
            return -1;
        }
        Py_UNREACHABLE();
    }
    return internal_bisect_right(a_obj, x, lo, hi, key);
}

/*[clinic input]
_bisect.bisect_right -> Py_ssize_t

    a: object
    x: object
    lo: Py_ssize_t(allow_negative=False) = 0
    hi: Py_ssize_t(c_default='-1', accept={int, NoneType}) = None
    *
    key: object = None

Return the index where to insert item x in list a, assuming a is sorted.

The return value i is such that all e in a[:i] have e <= x, and all e in
a[i:] have e > x.  So if x already appears in the list, a.insert(i, x)
will insert just after the rightmost x already there.

Optional args lo (default 0) and hi (default len(a)) bound the
slice of a to be searched.

A custom key function can be supplied to customize the sort order.
[clinic start generated code]*/

static Py_ssize_t
_bisect_bisect_right_impl(PyObject *module, PyValueBorrow a, PyValueBorrow x,
                          Py_ssize_t lo, Py_ssize_t hi, PyValueBorrow key)
/*[clinic end generated code: output=1897cf145fc23ae2 input=27717afe1a61bfaa]*/
{
    return bisect_right_values(a, x, lo, hi, key);
}

/*[clinic input]
_bisect.insort_right

    a: object
    x: object
    lo: Py_ssize_t(allow_negative=False) = 0
    hi: Py_ssize_t(c_default='-1', accept={int, NoneType}) = None
    *
    key: object = None

Insert item x in list a, and keep it sorted assuming a is sorted.

If x is already in a, insert it to the right of the rightmost x.

Optional args lo (default 0) and hi (default len(a)) bound the
slice of a to be searched.

A custom key function can be supplied to customize the sort order.
[clinic start generated code]*/

static PyValue
_bisect_insort_right_impl(PyObject *module, PyValueBorrow a, PyValueBorrow x,
                          Py_ssize_t lo, Py_ssize_t hi, PyValueBorrow key)
/*[clinic end generated code: output=a9d02209327ccd11 input=f2caa8abec0763e8]*/
{
    PyObject *result;
    PyObject *a_obj, *x_obj, *key_obj;
    Py_ssize_t index;

    if (bisect_box_args(a, x, key, &a_obj, &x_obj, &key_obj) < 0) {
        return PyValue_NULL;
    }
    if (PyValue_IsNone(key)) {
        index = internal_bisect_right(a_obj, PyValue_FromObjectBorrow(x_obj),
                                      lo, hi, PyValue_FromObjectBorrow(key_obj));
    } else {
        PyValue key_x = _PyValue_CallOneArg(_PyThreadState_GET(),
                                            PyValue_FromObjectBorrow(key_obj),
                                            PyValue_FromObjectBorrow(x_obj));
        if (PyValue_IsNull(key_x)) {
            goto error;
        }
        index = internal_bisect_right(a_obj, PyValue_Borrow(key_x),
                                      lo, hi, PyValue_FromObjectBorrow(key_obj));
        PyValue_CLOSE(key_x);
    }
    if (index < 0)
        goto error;
    if (PyList_CheckExact(a_obj)) {
        if (PyList_InsertValue(a_obj, index, PyValue_FromObjectBorrow(x_obj)) < 0)
            goto error;
    }
    else {
        bisect_state *state = get_bisect_state(module);
        result = _PyObject_CallMethod(a_obj, state->str_insert, "nO", index, x_obj);
        if (result == NULL)
            goto error;
        Py_DECREF(result);
    }

    Py_DECREF(a_obj);
    Py_DECREF(x_obj);
    Py_DECREF(key_obj);
    return PyValue_None;

error:
    Py_DECREF(a_obj);
    Py_DECREF(x_obj);
    Py_DECREF(key_obj);
    return PyValue_NULL;
}

static inline Py_ssize_t
internal_bisect_left(PyObject *list, PyValueBorrow item, Py_ssize_t lo,
                     Py_ssize_t hi, PyValueBorrow key)
{
    PyValue litem = PyValue_NULL;
    Py_ssize_t mid;
    int res;

    if (hi == -1) {
        hi = _PyValue_SequenceSize(PyValue_FromObjectBorrow(list));
        if (hi < 0)
            return -1;
    }
    PyTypeObject *list_type = Py_TYPE(list);
    ssizeargfunc sq_item = get_sq_item_type(list_type);
    if (sq_item == NULL) {
        return -1;
    }
    _Py_itemvaluefunc value_item = _PyType_VALUE_SLOT(list_type, item);
    if (value_item != NULL) {
        _PyType_CHECK_VALUE_ROW(list_type, _Py_VALUE_SLOT_INDEX_item);
    }
    if (Py_EnterRecursiveCall(" in _bisect.bisect_left")) {
        return -1;
    }
    PyTypeObject *tp = PyValue_TYPE(item);
    richcmpfunc compare = tp->tp_richcompare;
    PyObject *item_obj = PyValue_IsObject(item)
        ? PyValue_AsKnownObjectBorrow(item) : NULL;
    while (lo < hi) {
        /* The (size_t)cast ensures that the addition and subsequent division
           are performed as unsigned operations, avoiding difficulties from
           signed overflow.  (See issue 13496.) */
        mid = ((size_t)lo + hi) / 2;
        assert(mid >= 0);
        // PySequence_GetItem, but the two selected slot rows are cached.
        if (value_item != NULL) {
            litem = value_item(list, mid);
            if (PyValue_IsNull(litem)) {
                goto error;
            }
        }
        else {
            PyObject *litem_obj = sq_item(list, mid);
            assert((PyErr_Occurred() == NULL) ^ (litem_obj == NULL));
            if (litem_obj == NULL) {
                goto error;
            }
            litem = _PyValue_FromObjectResult(litem_obj);
        }
        if (!PyValue_IsNone(key)) {
            PyValue newitem = _PyValue_CallOneArg(_PyThreadState_GET(), key,
                                                  litem);
            if (PyValue_IsNull(newitem)) {
                PyValue_CLOSE(litem);
                litem = PyValue_NULL;
                goto error;
            }
            PyValue_CLOSE(litem);
            litem = newitem;
        }
        /* if key(list[mid]) < item:
         *     lo = mid + 1
         * else:
         *     hi = mid
         */
        if (compare != NULL && item_obj != NULL && PyValue_IsObject(litem) &&
            Py_IS_TYPE(PyValue_AsKnownObjectBorrow(litem), tp)) {
            // A fast path for comparing objects of the same type
            PyObject *res_obj = compare(PyValue_AsKnownObjectBorrow(litem),
                                        item_obj, Py_LT);
            if (res_obj == Py_True) {
                Py_DECREF(res_obj);
                PyValue_CLOSE(litem);
                litem = PyValue_NULL;
                lo = mid + 1;
                continue;
            }
            if (res_obj == Py_False) {
                Py_DECREF(res_obj);
                PyValue_CLOSE(litem);
                litem = PyValue_NULL;
                hi = mid;
                continue;
            }
            if (res_obj == NULL) {
                goto error;
            }
            if (res_obj == Py_NotImplemented) {
                Py_DECREF(res_obj);
                compare = NULL;
                res = _PyValue_RichCompareBool(litem, item, Py_LT);
            }
            else {
                res = _PyValue_IsTrue(PyValue_FromObjectBorrow(res_obj));
                Py_DECREF(res_obj);
            }
        }
        else {
            // A default path for comparing arbitrary objects
            res = _PyValue_RichCompareBool(litem, item, Py_LT);
        }
        if (res < 0) {
            goto error;
        }
        PyValue_CLOSE(litem);
        litem = PyValue_NULL;
        if (res)
            lo = mid + 1;
        else
            hi = mid;
    }
    Py_LeaveRecursiveCall();
    return lo;
error:
    Py_LeaveRecursiveCall();
    PyValue_XCLOSE(litem);
    return -1;
}


static Py_ssize_t
bisect_left_values(PyValueBorrow a, PyValueBorrow x, Py_ssize_t lo,
                    Py_ssize_t hi, PyValueBorrow key)
{
    if (lo < 0) {
        PyErr_SetString(PyExc_ValueError, "lo must be non-negative");
        return -1;
    }
    PyObject *a_obj;
    if (PyValue_IsObject(a)) {
        a_obj = PyValue_AsKnownObjectBorrow(a);
    }
    else {
        if (hi == -1) {
            (void)_PyValue_SequenceSize(a);
            return -1;
        }
        if (get_sq_item_type(PyValue_TYPE(a)) == NULL) {
            return -1;
        }
        Py_UNREACHABLE();
    }
    return internal_bisect_left(a_obj, x, lo, hi, key);
}

/*[clinic input]
_bisect.bisect_left -> Py_ssize_t

    a: object
    x: object
    lo: Py_ssize_t(allow_negative=False) = 0
    hi: Py_ssize_t(c_default='-1', accept={int, NoneType}) = None
    *
    key: object = None

Return the index where to insert item x in list a, assuming a is sorted.

The return value i is such that all e in a[:i] have e < x, and all e in
a[i:] have e >= x.  So if x already appears in the list, a.insert(i, x)
will insert just before the leftmost x already there.

Optional args lo (default 0) and hi (default len(a)) bound the
slice of a to be searched.

A custom key function can be supplied to customize the sort order.
[clinic start generated code]*/

static Py_ssize_t
_bisect_bisect_left_impl(PyObject *module, PyValueBorrow a, PyValueBorrow x,
                         Py_ssize_t lo, Py_ssize_t hi, PyValueBorrow key)
/*[clinic end generated code: output=e6a0f7022af64621 input=259fedbe35e882e1]*/
{
    return bisect_left_values(a, x, lo, hi, key);
}


/*[clinic input]
_bisect.insort_left

    a: object
    x: object
    lo: Py_ssize_t(allow_negative=False) = 0
    hi: Py_ssize_t(c_default='-1', accept={int, NoneType}) = None
    *
    key: object = None

Insert item x in list a, and keep it sorted assuming a is sorted.

If x is already in a, insert it to the left of the leftmost x.

Optional args lo (default 0) and hi (default len(a)) bound the
slice of a to be searched.

A custom key function can be supplied to customize the sort order.
[clinic start generated code]*/

static PyValue
_bisect_insort_left_impl(PyObject *module, PyValueBorrow a, PyValueBorrow x,
                         Py_ssize_t lo, Py_ssize_t hi, PyValueBorrow key)
/*[clinic end generated code: output=bc0e9b70159ecab2 input=ff85a79826e22f31]*/
{
    PyObject *result;
    PyObject *a_obj, *x_obj, *key_obj;
    Py_ssize_t index;

    if (bisect_box_args(a, x, key, &a_obj, &x_obj, &key_obj) < 0) {
        return PyValue_NULL;
    }
    if (PyValue_IsNone(key)) {
        index = internal_bisect_left(a_obj, PyValue_FromObjectBorrow(x_obj),
                                     lo, hi, PyValue_FromObjectBorrow(key_obj));
    } else {
        PyValue key_x = _PyValue_CallOneArg(_PyThreadState_GET(),
                                            PyValue_FromObjectBorrow(key_obj),
                                            PyValue_FromObjectBorrow(x_obj));
        if (PyValue_IsNull(key_x)) {
            goto error;
        }
        index = internal_bisect_left(a_obj, PyValue_Borrow(key_x), lo, hi,
                                     PyValue_FromObjectBorrow(key_obj));
        PyValue_CLOSE(key_x);
    }
    if (index < 0)
        goto error;
    if (PyList_CheckExact(a_obj)) {
        if (PyList_InsertValue(a_obj, index, PyValue_FromObjectBorrow(x_obj)) < 0)
            goto error;
    }
    else {
        bisect_state *state = get_bisect_state(module);
        result = _PyObject_CallMethod(a_obj, state->str_insert, "nO", index, x_obj);
        if (result == NULL)
            goto error;
        Py_DECREF(result);
    }

    Py_DECREF(a_obj);
    Py_DECREF(x_obj);
    Py_DECREF(key_obj);
    return PyValue_None;

error:
    Py_DECREF(a_obj);
    Py_DECREF(x_obj);
    Py_DECREF(key_obj);
    return PyValue_NULL;
}

static PyMethodDef bisect_methods[] = {
    _BISECT_BISECT_RIGHT_METHODDEF
    _BISECT_INSORT_RIGHT_METHODDEF
    _BISECT_BISECT_LEFT_METHODDEF
    _BISECT_INSORT_LEFT_METHODDEF
    {NULL, NULL} /* sentinel */
};

PyDoc_STRVAR(module_doc,
"Bisection algorithms.\n\
\n\
This module provides support for maintaining a list in sorted order without\n\
having to sort the list after each insertion. For long lists of items with\n\
expensive comparison operations, this can be an improvement over the more\n\
common approach.\n");

static int
bisect_clear(PyObject *module)
{
    bisect_state *state = get_bisect_state(module);
    Py_CLEAR(state->str_insert);
    return 0;
}

static void
bisect_free(void *module)
{
    bisect_clear((PyObject *)module);
}

static int
bisect_modexec(PyObject *m)
{
    bisect_state *state = get_bisect_state(m);
    state->str_insert = PyUnicode_InternFromString("insert");
    if (state->str_insert == NULL) {
        return -1;
    }
    return 0;
}

static PyModuleDef_Slot bisect_slots[] = {
    _Py_ABI_SLOT,
    {Py_mod_exec, bisect_modexec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL}
};

static struct PyModuleDef _bisectmodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "_bisect",
    .m_size = sizeof(bisect_state),
    .m_doc = module_doc,
    .m_methods = bisect_methods,
    .m_slots = bisect_slots,
    .m_clear = bisect_clear,
    .m_free = bisect_free,
};

PyMODINIT_FUNC
PyInit__bisect(void)
{
    return PyModuleDef_Init(&_bisectmodule);
}
