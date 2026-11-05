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
get_sq_item(PyObject *s)
{
    // The parts of PySequence_GetItem that we only need to do once
    PyTypeObject *tp = Py_TYPE(s);
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
internal_bisect_right(PyObject *list, PyObject *item, Py_ssize_t lo, Py_ssize_t hi,
                      PyObject* key)
{
    PyObject *litem;
    Py_ssize_t mid;
    int res;

    if (hi == -1) {
        hi = PySequence_Size(list);
        if (hi < 0)
            return -1;
    }
    ssizeargfunc sq_item = get_sq_item(list);
    if (sq_item == NULL) {
        return -1;
    }
    if (Py_EnterRecursiveCall(" in _bisect.bisect_right")) {
        return -1;
    }
    PyTypeObject *tp = Py_TYPE(item);
    richcmpfunc compare = tp->tp_richcompare;
    while (lo < hi) {
        /* The (size_t)cast ensures that the addition and subsequent division
           are performed as unsigned operations, avoiding difficulties from
           signed overflow.  (See issue 13496.) */
        mid = ((size_t)lo + hi) / 2;
        assert(mid >= 0);
        // PySequence_GetItem, but we already checked the types.
        litem = sq_item(list, mid);
        assert((PyErr_Occurred() == NULL) ^ (litem == NULL));
        if (litem == NULL) {
            goto error;
        }
        if (key != Py_None) {
            PyObject *newitem = PyObject_CallOneArg(key, litem);
            if (newitem == NULL) {
                goto error;
            }
            Py_SETREF(litem, newitem);
        }
        /* if item < key(list[mid]):
         *     hi = mid
         * else:
         *     lo = mid + 1
         */
        if (compare != NULL && Py_IS_TYPE(litem, tp)) {
            // A fast path for comparing objects of the same type
            PyObject *res_obj = compare(item, litem, Py_LT);
            if (res_obj == Py_True) {
                Py_DECREF(res_obj);
                Py_DECREF(litem);
                hi = mid;
                continue;
            }
            if (res_obj == Py_False) {
                Py_DECREF(res_obj);
                Py_DECREF(litem);
                lo = mid + 1;
                continue;
            }
            if (res_obj == NULL) {
                goto error;
            }
            if (res_obj == Py_NotImplemented) {
                Py_DECREF(res_obj);
                compare = NULL;
                res = _PyValue_RichCompareBool(PyValue_FromObjectBorrow(item), PyValue_FromObjectBorrow(litem), Py_LT);
            }
            else {
                res = _PyValue_IsTrue(PyValue_FromObjectBorrow(res_obj));
                Py_DECREF(res_obj);
            }
        }
        else {
            // A default path for comparing arbitrary objects
            res = _PyValue_RichCompareBool(PyValue_FromObjectBorrow(item), PyValue_FromObjectBorrow(litem), Py_LT);
        }
        if (res < 0) {
            goto error;
        }
        Py_DECREF(litem);
        if (res)
            hi = mid;
        else
            lo = mid + 1;
    }
    Py_LeaveRecursiveCall();
    return lo;
error:
    Py_LeaveRecursiveCall();
    Py_XDECREF(litem);
    return -1;
}

static Py_ssize_t
bisect_right_values(PyValueBorrow a, PyValueBorrow x, Py_ssize_t lo,
                    Py_ssize_t hi, PyValueBorrow key)
{
    PyObject *a_obj, *x_obj, *key_obj;
    if (bisect_box_args(a, x, key, &a_obj, &x_obj, &key_obj) < 0) {
        return -1;
    }
    Py_ssize_t index = internal_bisect_right(a_obj, x_obj, lo, hi, key_obj);
    Py_DECREF(a_obj);
    Py_DECREF(x_obj);
    Py_DECREF(key_obj);
    return index;
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
    PyObject *result, *key_x;
    PyObject *a_obj, *x_obj, *key_obj;
    Py_ssize_t index;

    if (bisect_box_args(a, x, key, &a_obj, &x_obj, &key_obj) < 0) {
        return PyValue_NULL;
    }
    if (PyValue_IsNone(key)) {
        index = internal_bisect_right(a_obj, x_obj, lo, hi, key_obj);
    } else {
        key_x = PyObject_CallOneArg(key_obj, x_obj);
        if (key_x == NULL) {
            goto error;
        }
        index = internal_bisect_right(a_obj, key_x, lo, hi, key_obj);
        Py_DECREF(key_x);
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
internal_bisect_left(PyObject *list, PyObject *item, Py_ssize_t lo, Py_ssize_t hi,
                     PyObject *key)
{
    PyObject *litem;
    Py_ssize_t mid;
    int res;

    if (hi == -1) {
        hi = PySequence_Size(list);
        if (hi < 0)
            return -1;
    }
    ssizeargfunc sq_item = get_sq_item(list);
    if (sq_item == NULL) {
        return -1;
    }
    if (Py_EnterRecursiveCall(" in _bisect.bisect_left")) {
        return -1;
    }
    PyTypeObject *tp = Py_TYPE(item);
    richcmpfunc compare = tp->tp_richcompare;
    while (lo < hi) {
        /* The (size_t)cast ensures that the addition and subsequent division
           are performed as unsigned operations, avoiding difficulties from
           signed overflow.  (See issue 13496.) */
        mid = ((size_t)lo + hi) / 2;
        assert(mid >= 0);
        // PySequence_GetItem, but we already checked the types.
        litem = sq_item(list, mid);
        assert((PyErr_Occurred() == NULL) ^ (litem == NULL));
        if (litem == NULL) {
            goto error;
        }
        if (key != Py_None) {
            PyObject *newitem = PyObject_CallOneArg(key, litem);
            if (newitem == NULL) {
                goto error;
            }
            Py_SETREF(litem, newitem);
        }
        /* if key(list[mid]) < item:
         *     lo = mid + 1
         * else:
         *     hi = mid
         */
        if (compare != NULL && Py_IS_TYPE(litem, tp)) {
            // A fast path for comparing objects of the same type
            PyObject *res_obj = compare(litem, item, Py_LT);
            if (res_obj == Py_True) {
                Py_DECREF(res_obj);
                Py_DECREF(litem);
                lo = mid + 1;
                continue;
            }
            if (res_obj == Py_False) {
                Py_DECREF(res_obj);
                Py_DECREF(litem);
                hi = mid;
                continue;
            }
            if (res_obj == NULL) {
                goto error;
            }
            if (res_obj == Py_NotImplemented) {
                Py_DECREF(res_obj);
                compare = NULL;
                res = _PyValue_RichCompareBool(PyValue_FromObjectBorrow(litem), PyValue_FromObjectBorrow(item), Py_LT);
            }
            else {
                res = _PyValue_IsTrue(PyValue_FromObjectBorrow(res_obj));
                Py_DECREF(res_obj);
            }
        }
        else {
            // A default path for comparing arbitrary objects
            res = _PyValue_RichCompareBool(PyValue_FromObjectBorrow(litem), PyValue_FromObjectBorrow(item), Py_LT);
        }
        if (res < 0) {
            goto error;
        }
        Py_DECREF(litem);
        if (res)
            lo = mid + 1;
        else
            hi = mid;
    }
    Py_LeaveRecursiveCall();
    return lo;
error:
    Py_LeaveRecursiveCall();
    Py_XDECREF(litem);
    return -1;
}


static Py_ssize_t
bisect_left_values(PyValueBorrow a, PyValueBorrow x, Py_ssize_t lo,
                    Py_ssize_t hi, PyValueBorrow key)
{
    PyObject *a_obj, *x_obj, *key_obj;
    if (bisect_box_args(a, x, key, &a_obj, &x_obj, &key_obj) < 0) {
        return -1;
    }
    Py_ssize_t index = internal_bisect_left(a_obj, x_obj, lo, hi, key_obj);
    Py_DECREF(a_obj);
    Py_DECREF(x_obj);
    Py_DECREF(key_obj);
    return index;
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
    PyObject *result, *key_x;
    PyObject *a_obj, *x_obj, *key_obj;
    Py_ssize_t index;

    if (bisect_box_args(a, x, key, &a_obj, &x_obj, &key_obj) < 0) {
        return PyValue_NULL;
    }
    if (PyValue_IsNone(key)) {
        index = internal_bisect_left(a_obj, x_obj, lo, hi, key_obj);
    } else {
        key_x = PyObject_CallOneArg(key_obj, x_obj);
        if (key_x == NULL) {
            goto error;
        }
        index = internal_bisect_left(a_obj, key_x, lo, hi, key_obj);
        Py_DECREF(key_x);
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
