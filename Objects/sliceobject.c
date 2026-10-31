/*
Written by Jim Hugunin and Chris Chase.

This includes both the singular ellipsis object and slice objects.

Guido, feel free to do whatever you want in the way of copyrights
for this file.
*/

/*
Py_Ellipsis encodes the '...' rubber index token. It is similar to
the Py_NoneStruct in that there is no way to create other objects of
this type and there is exactly one in existence.
*/

#include "Python.h"
#include "pycore_abstract.h"      // _PyIndex_Check()
#include "pycore_long.h"          // _PyLong_GetZero()
#include "pycore_modsupport.h"    // _PyArg_NoKeywords()
#include "pycore_object.h"        // _PyObject_GC_TRACK()
#include "pycore_sliceobject.h"   // _PySlice_FromValues()
#include "pycore_tuple.h"         // _PyTuple_FromValues()
#include "pycore_value.h"         // _PyValue_IsNone()


#define _PySlice_CAST(op) _Py_CAST(PySliceObject*, (op))

// The tuple (start, stop, step) is made from &start (slice_values()).
static_assert(offsetof(PySliceObject, stop) == offsetof(PySliceObject, start) + sizeof(PyValue)
              && offsetof(PySliceObject, step) == offsetof(PySliceObject, stop) + sizeof(PyValue),
              "PySliceObject's start, stop and step must be adjacent");

// A new tuple (start, stop, step) of the slice op.
static PyObject *
slice_values(PyObject *op)
{
    return _PyTuple_FromValues(&_PySlice_CAST(op)->start, 3);
}


static PyObject *
ellipsis_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    if (PyTuple_GET_SIZE(args) || (kwargs && PyDict_GET_SIZE(kwargs))) {
        PyErr_SetString(PyExc_TypeError, "EllipsisType takes no arguments");
        return NULL;
    }
    return Py_Ellipsis;
}

static void
ellipsis_dealloc(PyObject *ellipsis)
{
    /* This should never get called, but we also don't want to SEGV if
     * we accidentally decref Ellipsis out of existence. Instead,
     * since Ellipsis is an immortal object, re-set the reference count.
     */
    _Py_SetImmortal(ellipsis);
}

static PyObject *
ellipsis_repr(PyObject *op)
{
    return PyUnicode_FromString("Ellipsis");
}

static PyObject *
ellipsis_reduce(PyObject *op, PyObject *Py_UNUSED(ignored))
{
    return PyUnicode_FromString("Ellipsis");
}

static PyMethodDef ellipsis_methods[] = {
    {"__reduce__", ellipsis_reduce, METH_NOARGS, NULL},
    {NULL, NULL}
};

PyDoc_STRVAR(ellipsis_doc,
"ellipsis()\n"
"--\n\n"
"The type of the Ellipsis singleton.");

PyTypeObject PyEllipsis_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "ellipsis",                         /* tp_name */
    0,                                  /* tp_basicsize */
    0,                                  /* tp_itemsize */
    ellipsis_dealloc,                   /* tp_dealloc */
    0,                                  /* tp_vectorcall_offset */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_as_async */
    ellipsis_repr,                      /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    0,                                  /* tp_hash */
    0,                                  /* tp_call */
    0,                                  /* tp_str */
    PyObject_GenericGetAttr,            /* tp_getattro */
    0,                                  /* tp_setattro */
    0,                                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                 /* tp_flags */
    ellipsis_doc,                       /* tp_doc */
    0,                                  /* tp_traverse */
    0,                                  /* tp_clear */
    0,                                  /* tp_richcompare */
    0,                                  /* tp_weaklistoffset */
    0,                                  /* tp_iter */
    0,                                  /* tp_iternext */
    ellipsis_methods,                   /* tp_methods */
    0,                                  /* tp_members */
    0,                                  /* tp_getset */
    0,                                  /* tp_base */
    0,                                  /* tp_dict */
    0,                                  /* tp_descr_get */
    0,                                  /* tp_descr_set */
    0,                                  /* tp_dictoffset */
    0,                                  /* tp_init */
    0,                                  /* tp_alloc */
    ellipsis_new,                       /* tp_new */
};

PyObject _Py_EllipsisObject = _PyObject_HEAD_INIT(&PyEllipsis_Type);


/* Slice object implementation */

/* start, stop, and step are values with None indicating no index is
   present.
*/

PyObject *
_PySlice_FromValues(PyValue start, PyValue stop, PyValue step)
{
    PySliceObject *obj = PyObject_GC_New(PySliceObject, &PySlice_Type);
    if (obj == NULL) {
        return NULL;
    }
    obj->start = PyValue_IsNull(start) ? _PyValue_None : start;
    obj->stop = PyValue_IsNull(stop) ? _PyValue_None : stop;
    obj->step = PyValue_IsNull(step) ? _PyValue_None : step;
    _PyObject_GC_TRACK(obj);
    return (PyObject *)obj;
}

static PyValue
value_or_null(PyObject *op)
{
    return op == NULL ? PyValue_NULL : PyValue_FromObject(op);
}

PyObject *
PySlice_New(PyObject *start, PyObject *stop, PyObject *step)
{
    return _PySlice_FromValues(value_or_null(start), value_or_null(stop),
                               value_or_null(step));
}

/* The value of an index, inline when it fits in int32. */
static PyValue
ssize_value(Py_ssize_t i)
{
    if (INT32_MIN <= i && i <= INT32_MAX) {
        return PyValue_FromInt32((int32_t)i);
    }
    PyObject *op = PyLong_FromSsize_t(i);
    return op == NULL ? PyValue_NULL : PyValue_FromObject(op);
}

PyObject *
_PySlice_FromIndices(Py_ssize_t istart, Py_ssize_t istop)
{
    PyValue start = ssize_value(istart);
    if (PyValue_IsNull(start)) {
        return NULL;
    }
    PyValue stop = ssize_value(istop);
    if (PyValue_IsNull(stop)) {
        return NULL;
    }
    return _PySlice_FromValues(start, stop, PyValue_NULL);
}

/* The int bound v as a Py_ssize_t for PySlice_GetIndices(): 0 if v is an
   int, with *pi possibly -1 and an exception set on overflow, or -1 if it
   is not an int. */
static int
long_bound(PyValue v, Py_ssize_t *pi)
{
    if (PyValue_IsInlineInt(v)) {
        *pi = PyValue_Int32(v);
        return 0;
    }
    if (PyValue_IsInline(v) || !PyLong_Check(PyValue_ObjectBorrow(v))) {
        return -1;
    }
    *pi = PyLong_AsSsize_t(PyValue_ObjectBorrow(v));
    return 0;
}

int
PySlice_GetIndices(PyObject *_r, Py_ssize_t length,
                   Py_ssize_t *start, Py_ssize_t *stop, Py_ssize_t *step)
{
    PySliceObject *r = (PySliceObject*)_r;
    /* XXX support long ints */
    if (_PyValue_IsNone(r->step)) {
        *step = 1;
    } else {
        if (long_bound(r->step, step) < 0) return -1;
    }
    if (_PyValue_IsNone(r->start)) {
        *start = *step < 0 ? length-1 : 0;
    } else {
        if (long_bound(r->start, start) < 0) return -1;
        if (*start < 0) *start += length;
    }
    if (_PyValue_IsNone(r->stop)) {
        *stop = *step < 0 ? -1 : length;
    } else {
        if (long_bound(r->stop, stop) < 0) return -1;
        if (*stop < 0) *stop += length;
    }
    if (*stop > length) return -1;
    if (*start >= length) return -1;
    if (*step == 0) return -1;
    return 0;
}

/* _PyEval_SliceIndexNotNone() of the value v. */
static int
slice_index(PyValue v, Py_ssize_t *pi)
{
    if (PyValue_IsInlineInt(v)) {
        *pi = PyValue_Int32(v);
        return 1;
    }
    if (PyValue_IsInlineFloat(v)) {
        PyErr_SetString(PyExc_TypeError,
                        "slice indices must be integers or "
                        "have an __index__ method");
        return 0;
    }
    return _PyEval_SliceIndexNotNone(PyValue_ObjectBorrow(v), pi);
}

int
PySlice_Unpack(PyObject *_r,
               Py_ssize_t *start, Py_ssize_t *stop, Py_ssize_t *step)
{
    PySliceObject *r = (PySliceObject*)_r;
    /* this is harder to get right than you might think */

    static_assert(PY_SSIZE_T_MIN + 1 <= -PY_SSIZE_T_MAX,
                  "-PY_SSIZE_T_MAX < PY_SSIZE_T_MIN + 1");

    if (_PyValue_IsNone(r->step)) {
        *step = 1;
    }
    else {
        if (!slice_index(r->step, step)) return -1;
        if (*step == 0) {
            PyErr_SetString(PyExc_ValueError,
                            "slice step cannot be zero");
            return -1;
        }
        /* Here *step might be -PY_SSIZE_T_MAX-1; in this case we replace it
         * with -PY_SSIZE_T_MAX.  This doesn't affect the semantics, and it
         * guards against later undefined behaviour resulting from code that
         * does "step = -step" as part of a slice reversal.
         */
        if (*step < -PY_SSIZE_T_MAX)
            *step = -PY_SSIZE_T_MAX;
    }

    if (_PyValue_IsNone(r->start)) {
        *start = *step < 0 ? PY_SSIZE_T_MAX : 0;
    }
    else {
        if (!slice_index(r->start, start)) return -1;
    }

    if (_PyValue_IsNone(r->stop)) {
        *stop = *step < 0 ? PY_SSIZE_T_MIN : PY_SSIZE_T_MAX;
    }
    else {
        if (!slice_index(r->stop, stop)) return -1;
    }

    return 0;
}

Py_ssize_t
PySlice_AdjustIndices(Py_ssize_t length,
                      Py_ssize_t *start, Py_ssize_t *stop, Py_ssize_t step)
{
    /* this is harder to get right than you might think */

    assert(step != 0);
    assert(step >= -PY_SSIZE_T_MAX);

    if (*start < 0) {
        *start += length;
        if (*start < 0) {
            *start = (step < 0) ? -1 : 0;
        }
    }
    else if (*start >= length) {
        *start = (step < 0) ? length - 1 : length;
    }

    if (*stop < 0) {
        *stop += length;
        if (*stop < 0) {
            *stop = (step < 0) ? -1 : 0;
        }
    }
    else if (*stop >= length) {
        *stop = (step < 0) ? length - 1 : length;
    }

    if (step < 0) {
        if (*stop < *start) {
            return (*start - *stop - 1) / (-step) + 1;
        }
    }
    else {
        if (*start < *stop) {
            return (*stop - *start - 1) / step + 1;
        }
    }
    return 0;
}

#undef PySlice_GetIndicesEx

int
PySlice_GetIndicesEx(PyObject *_r, Py_ssize_t length,
                     Py_ssize_t *start, Py_ssize_t *stop, Py_ssize_t *step,
                     Py_ssize_t *slicelength)
{
    if (PySlice_Unpack(_r, start, stop, step) < 0)
        return -1;
    *slicelength = PySlice_AdjustIndices(length, start, stop, *step);
    return 0;
}

static PyObject *
slice_new(PyTypeObject *type, PyObject *args, PyObject *kw)
{
    PyObject *start, *stop, *step;

    start = stop = step = NULL;

    if (!_PyArg_NoKeywords("slice", kw))
        return NULL;

    if (!PyArg_UnpackTuple(args, "slice", 1, 3, &start, &stop, &step))
        return NULL;

    /* This swapping of stop and start is to maintain similarity with
       range(). */
    if (stop == NULL) {
        stop = start;
        start = NULL;
    }
    return PySlice_New(start, stop, step);
}

PyDoc_STRVAR(slice_doc,
"slice(stop)\n\
slice(start, stop[, step])\n\
\n\
Create a slice object.\n\
\n\
This is used for extended slicing (e.g. a[0:10:2]).");

static void
slice_dealloc(PyObject *op)
{
    PySliceObject *r = _PySlice_CAST(op);
    PyObject_GC_UnTrack(r);
    PyObject_GC_Del(r);
}

static PyObject *
slice_repr(PyObject *op)
{
    PySliceObject *r = _PySlice_CAST(op);
    PyValue start = PyObject_ReprValue(r->start);
    if (PyValue_IsNull(start)) {
        return NULL;
    }
    PyValue stop = PyObject_ReprValue(r->stop);
    if (PyValue_IsNull(stop)) {
        return NULL;
    }
    PyValue step = PyObject_ReprValue(r->step);
    if (PyValue_IsNull(step)) {
        return NULL;
    }
    return PyUnicode_FromFormat("slice(%U, %U, %U)",
                                PyValue_ObjectBorrow(start),
                                PyValue_ObjectBorrow(stop),
                                PyValue_ObjectBorrow(step));
}

static PyMemberDef slice_members[] = {
    {"start", _Py_T_VALUE, offsetof(PySliceObject, start), Py_READONLY},
    {"stop", _Py_T_VALUE, offsetof(PySliceObject, stop), Py_READONLY},
    {"step", _Py_T_VALUE, offsetof(PySliceObject, step), Py_READONLY},
    {0}
};

/* Helper function to convert a slice argument to an int value, and raise
   TypeError with a suitable message on failure. */

static PyValue
evaluate_slice_index(PyValue value)
{
    if (PyValue_IsInlineInt(value)) {
        return value;
    }
    if (PyValue_IsObject(value) && _PyIndex_Check(PyValue_ObjectBorrow(value))) {
        return PyNumber_IndexValue(value);
    }
    else {
        PyErr_SetString(PyExc_TypeError,
                        "slice indices must be integers or "
                        "None or have an __index__ method");
        return PyValue_NULL;
    }
}

/* The sign of the int value v: -1, 0 or 1. */
static int
int_value_sign(PyValue v)
{
    if (PyValue_IsInlineInt(v)) {
        int32_t i = PyValue_Int32(v);
        return (i > 0) - (i < 0);
    }
    int sign;
    (void)PyLong_GetSign(PyValue_ObjectBorrow(v), &sign);
    return sign;
}

/* _PySlice_GetLongIndices() on values: the bounds and the length are int
   values, inline when they fit. */
static int
slice_long_indices(PySliceObject *self, PyValue length,
                   PyValue *start_ptr, PyValue *stop_ptr, PyValue *step_ptr)
{
    PyValue start, stop, step;
    PyValue upper, lower;
    int step_is_negative, cmp_result;

    *start_ptr = *stop_ptr = *step_ptr = PyValue_NULL;

    /* Convert step to an integer; raise for zero step. */
    if (_PyValue_IsNone(self->step)) {
        step = PyValue_FromInt32(1);
        step_is_negative = 0;
    }
    else {
        step = evaluate_slice_index(self->step);
        if (PyValue_IsNull(step)) {
            return -1;
        }
        assert(PyType_IsSubtype(PyValue_Type(step), &PyLong_Type));

        int step_sign = int_value_sign(step);
        if (step_sign == 0) {
            PyErr_SetString(PyExc_ValueError,
                            "slice step cannot be zero");
            return -1;
        }
        step_is_negative = step_sign < 0;
    }

    /* Find lower and upper bounds for start and stop. */
    if (step_is_negative) {
        lower = PyValue_FromInt32(-1);
        upper = PyNumber_AddValue(length, lower);
        if (PyValue_IsNull(upper))
            return -1;
    }
    else {
        lower = PyValue_FromInt32(0);
        upper = length;
    }

    /* Compute start. */
    if (_PyValue_IsNone(self->start)) {
        start = step_is_negative ? upper : lower;
    }
    else {
        start = evaluate_slice_index(self->start);
        if (PyValue_IsNull(start))
            return -1;

        if (int_value_sign(start) < 0) {
            /* start += length */
            start = PyNumber_AddValue(start, length);
            if (PyValue_IsNull(start))
                return -1;

            cmp_result = PyObject_RichCompareBoolValue(start, lower, Py_LT);
            if (cmp_result < 0)
                return -1;
            if (cmp_result) {
                start = lower;
            }
        }
        else {
            cmp_result = PyObject_RichCompareBoolValue(start, upper, Py_GT);
            if (cmp_result < 0)
                return -1;
            if (cmp_result) {
                start = upper;
            }
        }
    }

    /* Compute stop. */
    if (_PyValue_IsNone(self->stop)) {
        stop = step_is_negative ? lower : upper;
    }
    else {
        stop = evaluate_slice_index(self->stop);
        if (PyValue_IsNull(stop))
            return -1;

        if (int_value_sign(stop) < 0) {
            /* stop += length */
            stop = PyNumber_AddValue(stop, length);
            if (PyValue_IsNull(stop))
                return -1;

            cmp_result = PyObject_RichCompareBoolValue(stop, lower, Py_LT);
            if (cmp_result < 0)
                return -1;
            if (cmp_result) {
                stop = lower;
            }
        }
        else {
            cmp_result = PyObject_RichCompareBoolValue(stop, upper, Py_GT);
            if (cmp_result < 0)
                return -1;
            if (cmp_result) {
                stop = upper;
            }
        }
    }

    *start_ptr = start;
    *stop_ptr = stop;
    *step_ptr = step;
    return 0;
}

/* Compute slice indices given a slice and length.  Return -1 on failure.  Used
   by slice.indices and rangeobject slicing.  Assumes that `len` is a
   nonnegative instance of PyLong. */

int
_PySlice_GetLongIndices(PySliceObject *self, PyObject *length,
                        PyObject **start_ptr, PyObject **stop_ptr,
                        PyObject **step_ptr)
{
    PyValue start, stop, step;
    *start_ptr = *stop_ptr = *step_ptr = NULL;
    if (slice_long_indices(self, PyValue_FromObject(length),
                           &start, &stop, &step) < 0) {
        return -1;
    }
    // The caller takes int objects: inline bounds are boxed.
    PyObject *start_o = PyValue_Object(start);
    PyObject *stop_o = PyValue_Object(stop);
    PyObject *step_o = PyValue_Object(step);
    if (start_o == NULL || stop_o == NULL || step_o == NULL) {
        return -1;
    }
    *start_ptr = start_o;
    *stop_ptr = stop_o;
    *step_ptr = step_o;
    return 0;
}

/* Implementation of slice.indices. */

static PyValue
slice_indices(PyObject *op, PyValue len)
{
    PySliceObject *self = _PySlice_CAST(op);
    PyValue indices[3];

    /* Convert length to an integer if necessary; raise for negative length. */
    PyValue length = PyNumber_IndexValue(len);
    if (PyValue_IsNull(length))
        return PyValue_NULL;

    if (int_value_sign(length) < 0) {
        PyErr_SetString(PyExc_ValueError,
                        "length should not be negative");
        return PyValue_NULL;
    }

    if (slice_long_indices(self, length,
                           &indices[0], &indices[1], &indices[2]) < 0) {
        return PyValue_NULL;
    }
    return PyValue_FromResult(_PyTuple_FromValues(indices, 3));
}

PyDoc_STRVAR(slice_indices_doc,
"S.indices(len) -> (start, stop, stride)\n\
\n\
Assuming a sequence of length len, calculate the start and stop\n\
indices, and the stride length of the extended slice described by\n\
S. Out of bounds indices are clipped in a manner consistent with the\n\
handling of normal slices.");

static PyObject *
slice_reduce(PyObject *op, PyObject *Py_UNUSED(ignored))
{
    PyObject *args = slice_values(op);
    if (args == NULL) {
        return NULL;
    }
    return Py_BuildValue("ON", Py_TYPE(op), args);
}

PyDoc_STRVAR(reduce_doc, "Return state information for pickling.");

static PyMethodDef slice_methods[] = {
    {"indices", _PyCFunction_CAST(slice_indices), METH_O, slice_indices_doc},
    {"__reduce__", slice_reduce, METH_NOARGS, reduce_doc},
    {"__class_getitem__", _PyCFunction_CAST(Py_GenericAliasValue), METH_O|METH_CLASS,
     "slices are generic over the types of their start, end, and step values"},
    {NULL, NULL}
};

static PyObject *
slice_richcompare(PyObject *v, PyObject *w, int op)
{
    if (!PySlice_Check(v) || !PySlice_Check(w))
        Py_RETURN_NOTIMPLEMENTED;

    if (v == w) {
        PyObject *res;
        /* XXX Do we really need this shortcut?
           There's a unit test for it, but is that fair? */
        switch (op) {
        case Py_EQ:
        case Py_LE:
        case Py_GE:
            res = Py_True;
            break;
        default:
            res = Py_False;
            break;
        }
        return Py_NewRef(res);
    }


    PyObject *t1 = slice_values(v);
    if (t1 == NULL) {
        return NULL;
    }

    PyObject *t2 = slice_values(w);
    if (t2 == NULL) {
        Py_DECREF(t1);
        return NULL;
    }

    PyObject *res = PyObject_RichCompare(t1, t2, op);
    Py_DECREF(t1);
    Py_DECREF(t2);
    return res;
}

static int
slice_traverse(PyObject *op, visitproc visit, void *arg)
{
    PySliceObject *v = _PySlice_CAST(op);
    _Py_VISIT_VALUE(v->start);
    _Py_VISIT_VALUE(v->stop);
    _Py_VISIT_VALUE(v->step);
    return 0;
}

/* code based on tuplehash() of Objects/tupleobject.c */
#if SIZEOF_PY_UHASH_T > 4
#define _PyHASH_XXPRIME_1 ((Py_uhash_t)11400714785074694791ULL)
#define _PyHASH_XXPRIME_2 ((Py_uhash_t)14029467366897019727ULL)
#define _PyHASH_XXPRIME_5 ((Py_uhash_t)2870177450012600261ULL)
#define _PyHASH_XXROTATE(x) ((x << 31) | (x >> 33))  /* Rotate left 31 bits */
#else
#define _PyHASH_XXPRIME_1 ((Py_uhash_t)2654435761UL)
#define _PyHASH_XXPRIME_2 ((Py_uhash_t)2246822519UL)
#define _PyHASH_XXPRIME_5 ((Py_uhash_t)374761393UL)
#define _PyHASH_XXROTATE(x) ((x << 13) | (x >> 19))  /* Rotate left 13 bits */
#endif

static Py_hash_t
slice_hash(PyObject *op)
{
    PySliceObject *v = _PySlice_CAST(op);
    Py_uhash_t acc = _PyHASH_XXPRIME_5;
#define _PyHASH_SLICE_PART(com) { \
    Py_uhash_t lane = PyObject_HashValue(v->com); \
    if(lane == (Py_uhash_t)-1) { \
        return -1; \
    } \
    acc += lane * _PyHASH_XXPRIME_2; \
    acc = _PyHASH_XXROTATE(acc); \
    acc *= _PyHASH_XXPRIME_1; \
}
    _PyHASH_SLICE_PART(start);
    _PyHASH_SLICE_PART(stop);
    _PyHASH_SLICE_PART(step);
#undef _PyHASH_SLICE_PART
    if(acc == (Py_uhash_t)-1) {
        return 1546275796;
    }
    return acc;
}

PyTypeObject PySlice_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "slice",                    /* Name of this type */
    sizeof(PySliceObject),      /* Basic object size */
    0,                          /* Item size for varobject */
    slice_dealloc,                              /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    slice_repr,                                 /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    slice_hash,                                 /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,    /* tp_flags */
    slice_doc,                                  /* tp_doc */
    slice_traverse,                             /* tp_traverse */
    0,                                          /* tp_clear */
    slice_richcompare,                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    slice_methods,                              /* tp_methods */
    slice_members,                              /* tp_members */
    0,                                          /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    0,                                          /* tp_init */
    0,                                          /* tp_alloc */
    slice_new,                                  /* tp_new */
};
