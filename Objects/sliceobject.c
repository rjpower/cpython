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
#include "pycore_freelist.h"      // _Py_FREELIST_FREE(), _Py_FREELIST_POP()
#include "pycore_long.h"          // _PyLong_GetZero()
#include "pycore_modsupport.h"    // _PyArg_NoKeywords()
#include "pycore_object.h"        // _PyObject_GC_TRACK()
#include "pycore_sliceobject.h"   // PySliceObject
#include "pycore_value.h"         // PyValue_CLOSE()
#include "pycore_value_api.h"     // _PyValue_AsObjectAt()
#include "pycore_value_ops.h"     // _PyValue_Hash(), _PyValue_WriteRepr()


#define _PySlice_CAST(op) _Py_CAST(PySliceObject*, (op))


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

/* start, stop and step are value fields (Include/internal/pycore_sliceobject.h)
   with None indicating no index is present.
*/

/* A new slice holding start, stop and step, whose owners it takes, also on
   failure. Each is a language value. */
static PyObject *
slice_from_owned(PyValue start, PyValue stop, PyValue step)
{
    PySliceObject *obj = _Py_FREELIST_POP(PySliceObject, slices);
    if (obj == NULL) {
        obj = PyObject_GC_New(PySliceObject, &PySlice_Type);
        if (obj == NULL) {
            PyValue_CLOSE(start);
            PyValue_CLOSE(stop);
            PyValue_CLOSE(step);
            return NULL;
        }
    }

    obj->start = start;
    obj->stop = stop;
    obj->step = step;
    _PyValue_CheckHome(obj->start);
    _PyValue_CheckHome(obj->stop);
    _PyValue_CheckHome(obj->step);

    _PyObject_GC_TRACK(obj);
    return (PyObject *)obj;
}

// The owned value of a PySlice_New() argument: None if it is NULL.
static inline PyValue
object_bound(PyObject *obj)
{
    return obj == NULL ? PyValue_None : PyValue_DUP(PyValue_FromObjectBorrow(obj));
}

PyObject *
PySlice_New(PyObject *start, PyObject *stop, PyObject *step)
{
    return slice_from_owned(object_bound(start), object_bound(stop),
                            object_bound(step));
}

// The owned value of a _PySlice_FromValues() argument: None if it is NULL.
static inline PyValue
value_bound(PyValueBorrow value)
{
    if (PyValue_IsNull(value)) {
        return PyValue_None;
    }
    assert(PyValue_IsLanguageValue(value));
    /* A tagged object is immortal (H1 of register IR v2 step A5b), so that
       PyValue_DUP() makes an owner. Until A5b-3 lands, the tagging borrow
       of PyValue_FromObjectBorrow() breaks this for a mortal object. */
    assert(!PyValue_IsObject(value) || PyValue_RefcountOnObject(value) ||
           _Py_IsImmortal(PyValue_AsKnownObjectBorrow(value)));
    return PyValue_DUP(value);
}

PyObject *
(_PySlice_FromValues)(PyValueBorrow start, PyValueBorrow stop,
                      PyValueBorrow step)
{
    return slice_from_owned(value_bound(start), value_bound(stop),
                            value_bound(step));
}

PyObject *
_PySlice_FromIndices(Py_ssize_t istart, Py_ssize_t istop)
{
    PyValue start = PyValue_FromInt64(istart);
    if (PyValue_IsNull(start)) {
        return NULL;
    }
    PyValue stop = PyValue_FromInt64(istop);
    if (PyValue_IsNull(stop)) {
        PyValue_CLOSE(start);
        return NULL;
    }
    return slice_from_owned(start, stop, PyValue_None);
}

/* (start, stop, step), a tuple that holds the values of the fields. */
PyObject *
_PySlice_AsTuple(PySliceObject *slice)
{
    PyObject *fields = PyTuple_New(3);
    if (fields == NULL) {
        return NULL;
    }
    PyTuple_SET_VALUE(fields, 0, PyValue_DUP(slice->start));
    PyTuple_SET_VALUE(fields, 1, PyValue_DUP(slice->stop));
    PyTuple_SET_VALUE(fields, 2, PyValue_DUP(slice->step));
    return fields;
}

/* Store in *pi the index that field, a slice field other than None, holds:
   an inline int is the index; any other value goes through
   _PyEval_SliceIndexNotNone(), which calls __index__ and clamps to
   Py_ssize_t. Return 1, or 0 with an exception set. */
static int
slice_index(PyValueBorrow field, Py_ssize_t *pi)
{
    if (PyValue_IsInlineInt(field)) {
        // |value| < 2**50, which fits Py_ssize_t.
        *pi = (Py_ssize_t)PyValue_InlineInt64(field);
        return 1;
    }
    /* Only an inline float, which has no __index__, boxes here: the box
       raises upstream's TypeError. */
    PyObject *obj = _PyValue_AsObjectAt(field, BOX_SITE_FIELD);
    if (obj == NULL) {
        return 0;
    }
    int ok = _PyEval_SliceIndexNotNone(obj, pi);
    Py_DECREF(obj);
    return ok;
}

/* PySlice_GetIndices() reads an int field with PyLong_AsSsize_t() and fails
   with no exception set for any other type. Return 1, or 0 for a field that
   is not an int. */
static int
slice_long_field(PyValueBorrow field, Py_ssize_t *pi)
{
    if (PyValue_IsInlineInt(field)) {
        *pi = (Py_ssize_t)PyValue_InlineInt64(field);
        return 1;
    }
    if (!PyValue_LongCheck(field)) {
        return 0;
    }
    *pi = PyLong_AsSsize_t(PyValue_AsKnownObjectBorrow(field));
    return 1;
}

int
PySlice_GetIndices(PyObject *_r, Py_ssize_t length,
                   Py_ssize_t *start, Py_ssize_t *stop, Py_ssize_t *step)
{
    PySliceObject *r = (PySliceObject*)_r;
    /* XXX support long ints */
    if (PyValue_IsNone(r->step)) {
        *step = 1;
    } else {
        if (!slice_long_field(PyValue_Borrow(r->step), step)) return -1;
    }
    if (PyValue_IsNone(r->start)) {
        *start = *step < 0 ? length-1 : 0;
    } else {
        if (!slice_long_field(PyValue_Borrow(r->start), start)) return -1;
        if (*start < 0) *start += length;
    }
    if (PyValue_IsNone(r->stop)) {
        *stop = *step < 0 ? -1 : length;
    } else {
        if (!slice_long_field(PyValue_Borrow(r->stop), stop)) return -1;
        if (*stop < 0) *stop += length;
    }
    if (*stop > length) return -1;
    if (*start >= length) return -1;
    if (*step == 0) return -1;
    return 0;
}

int
PySlice_Unpack(PyObject *_r,
               Py_ssize_t *start, Py_ssize_t *stop, Py_ssize_t *step)
{
    PySliceObject *r = (PySliceObject*)_r;
    /* this is harder to get right than you might think */

    static_assert(PY_SSIZE_T_MIN + 1 <= -PY_SSIZE_T_MAX,
                  "-PY_SSIZE_T_MAX < PY_SSIZE_T_MIN + 1");

    if (PyValue_IsNone(r->step)) {
        *step = 1;
    }
    else {
        if (!slice_index(PyValue_Borrow(r->step), step)) return -1;
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

    if (PyValue_IsNone(r->start)) {
        *start = *step < 0 ? PY_SSIZE_T_MAX : 0;
    }
    else {
        if (!slice_index(PyValue_Borrow(r->start), start)) return -1;
    }

    if (PyValue_IsNone(r->stop)) {
        *stop = *step < 0 ? PY_SSIZE_T_MIN : PY_SSIZE_T_MAX;
    }
    else {
        if (!slice_index(PyValue_Borrow(r->stop), stop)) return -1;
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
    PyValue_CLOSE(r->step);
    PyValue_CLOSE(r->start);
    PyValue_CLOSE(r->stop);
    _Py_FREELIST_FREE(slices, r, PyObject_GC_Del);
}

static PyObject *
slice_repr(PyObject *op)
{
    PySliceObject *r = _PySlice_CAST(op);
    PyUnicodeWriter *writer = PyUnicodeWriter_Create(0);
    if (writer == NULL) {
        return NULL;
    }
    if (PyUnicodeWriter_WriteASCII(writer, "slice(", 6) < 0 ||
        _PyValue_WriteRepr(writer, r->start) < 0 ||
        PyUnicodeWriter_WriteASCII(writer, ", ", 2) < 0 ||
        _PyValue_WriteRepr(writer, r->stop) < 0 ||
        PyUnicodeWriter_WriteASCII(writer, ", ", 2) < 0 ||
        _PyValue_WriteRepr(writer, r->step) < 0 ||
        PyUnicodeWriter_WriteChar(writer, ')') < 0)
    {
        PyUnicodeWriter_Discard(writer);
        return NULL;
    }
    return PyUnicodeWriter_Finish(writer);
}

static PyMemberDef slice_members[] = {
    {"start", _Py_T_VALUE, offsetof(PySliceObject, start), Py_READONLY},
    {"stop", _Py_T_VALUE, offsetof(PySliceObject, stop), Py_READONLY},
    {"step", _Py_T_VALUE, offsetof(PySliceObject, step), Py_READONLY},
    {0}
};

/* Helper function to convert a slice field other than None to a PyLong, and
   raise TypeError with a suitable message on failure. Only the object path
   of slice.indices() and range slicing comes here, so an inline field
   becomes an object, counted as a field box. */

static PyObject*
evaluate_slice_index(PyValueBorrow field)
{
    PyObject *v = _PyValue_AsObjectAt(field, BOX_SITE_FIELD);
    if (v == NULL) {
        return NULL;
    }
    if (_PyIndex_Check(v)) {
        Py_SETREF(v, PyNumber_Index(v));
        return v;
    }
    else {
        Py_DECREF(v);
        PyErr_SetString(PyExc_TypeError,
                        "slice indices must be integers or "
                        "None or have an __index__ method");
        return NULL;
    }
}

static inline bool
is_inline_bound(PyValueBorrow field)
{
    return PyValue_IsInlineInt(field) || PyValue_IsNone(field);
}

/* The fast path of slice indices for a length: if each field is an inline
   int or None and length fits Py_ssize_t, store the clipped start and stop
   and the step, as the object path below computes them, and return 1.
   PySlice_Unpack() and PySlice_AdjustIndices() clip as that path does: with
   a positive step to 0 .. length, with a negative one to -1 .. length - 1.
   Return 0 for the object path, or -1 with an exception set (a zero step).
   length is a nonnegative int. */
static int
inline_indices(PySliceObject *self, PyObject *length, Py_ssize_t *start,
               Py_ssize_t *stop, Py_ssize_t *step)
{
    if (!is_inline_bound(PyValue_Borrow(self->start)) ||
        !is_inline_bound(PyValue_Borrow(self->stop)) ||
        !is_inline_bound(PyValue_Borrow(self->step)))
    {
        return 0;
    }
    int overflow;
    long long ilength = PyLong_AsLongLongAndOverflow(length, &overflow);
    if (overflow) {
        return 0;
    }
    assert(ilength >= 0 && ilength <= PY_SSIZE_T_MAX);
    if (PySlice_Unpack((PyObject *)self, start, stop, step) < 0) {
        return -1;
    }
    PySlice_AdjustIndices((Py_ssize_t)ilength, start, stop, *step);
    return 1;
}

static int
long_indices(PySliceObject *self, PyObject *length,
             PyObject **start_ptr, PyObject **stop_ptr, PyObject **step_ptr);

/* Compute slice indices given a slice and length.  Return -1 on failure.  Used
   by slice.indices and rangeobject slicing.  Assumes that `len` is a
   nonnegative instance of PyLong. */

int
_PySlice_GetLongIndices(PySliceObject *self, PyObject *length,
                        PyObject **start_ptr, PyObject **stop_ptr,
                        PyObject **step_ptr)
{
    Py_ssize_t start, stop, step;
    int fast = inline_indices(self, length, &start, &stop, &step);
    if (fast == 0) {
        return long_indices(self, length, start_ptr, stop_ptr, step_ptr);
    }
    *start_ptr = *stop_ptr = *step_ptr = NULL;
    if (fast < 0) {
        return -1;
    }
    *start_ptr = PyLong_FromSsize_t(start);
    *stop_ptr = PyLong_FromSsize_t(stop);
    *step_ptr = PyLong_FromSsize_t(step);
    if (*start_ptr == NULL || *stop_ptr == NULL || *step_ptr == NULL) {
        Py_CLEAR(*start_ptr);
        Py_CLEAR(*stop_ptr);
        Py_CLEAR(*step_ptr);
        return -1;
    }
    return 0;
}

/* The object path of _PySlice_GetLongIndices(), for a field that is not an
   inline int or None, or a length beyond Py_ssize_t. */
static int
long_indices(PySliceObject *self, PyObject *length,
             PyObject **start_ptr, PyObject **stop_ptr, PyObject **step_ptr)
{
    PyObject *start=NULL, *stop=NULL, *step=NULL;
    PyObject *upper=NULL, *lower=NULL;
    int step_is_negative, cmp_result;

    /* Convert step to an integer; raise for zero step. */
    if (PyValue_IsNone(self->step)) {
        step = _PyLong_GetOne();
        step_is_negative = 0;
    }
    else {
        step = evaluate_slice_index(PyValue_Borrow(self->step));
        if (step == NULL) {
            goto error;
        }
        assert(PyLong_Check(step));

        int step_sign;
        (void)PyLong_GetSign(step, &step_sign);
        if (step_sign == 0) {
            PyErr_SetString(PyExc_ValueError,
                            "slice step cannot be zero");
            goto error;
        }
        step_is_negative = step_sign < 0;
    }

    /* Find lower and upper bounds for start and stop. */
    if (step_is_negative) {
        lower = PyLong_FromLong(-1L);
        if (lower == NULL)
            goto error;

        upper = PyNumber_Add(length, lower);
        if (upper == NULL)
            goto error;
    }
    else {
        lower = _PyLong_GetZero();
        upper = Py_NewRef(length);
    }

    /* Compute start. */
    if (PyValue_IsNone(self->start)) {
        start = Py_NewRef(step_is_negative ? upper : lower);
    }
    else {
        start = evaluate_slice_index(PyValue_Borrow(self->start));
        if (start == NULL)
            goto error;

        if (_PyLong_IsNegative((PyLongObject *)start)) {
            /* start += length */
            PyObject *tmp = PyNumber_Add(start, length);
            Py_SETREF(start, tmp);
            if (start == NULL)
                goto error;

            cmp_result = PyObject_RichCompareBool(start, lower, Py_LT);
            if (cmp_result < 0)
                goto error;
            if (cmp_result) {
                Py_SETREF(start, Py_NewRef(lower));
            }
        }
        else {
            cmp_result = PyObject_RichCompareBool(start, upper, Py_GT);
            if (cmp_result < 0)
                goto error;
            if (cmp_result) {
                Py_SETREF(start, Py_NewRef(upper));
            }
        }
    }

    /* Compute stop. */
    if (PyValue_IsNone(self->stop)) {
        stop = Py_NewRef(step_is_negative ? lower : upper);
    }
    else {
        stop = evaluate_slice_index(PyValue_Borrow(self->stop));
        if (stop == NULL)
            goto error;

        if (_PyLong_IsNegative((PyLongObject *)stop)) {
            /* stop += length */
            PyObject *tmp = PyNumber_Add(stop, length);
            Py_SETREF(stop, tmp);
            if (stop == NULL)
                goto error;

            cmp_result = PyObject_RichCompareBool(stop, lower, Py_LT);
            if (cmp_result < 0)
                goto error;
            if (cmp_result) {
                Py_SETREF(stop, Py_NewRef(lower));
            }
        }
        else {
            cmp_result = PyObject_RichCompareBool(stop, upper, Py_GT);
            if (cmp_result < 0)
                goto error;
            if (cmp_result) {
                Py_SETREF(stop, Py_NewRef(upper));
            }
        }
    }

    *start_ptr = start;
    *stop_ptr = stop;
    *step_ptr = step;
    Py_DECREF(upper);
    Py_DECREF(lower);
    return 0;

  error:
    *start_ptr = *stop_ptr = *step_ptr = NULL;
    Py_XDECREF(start);
    Py_XDECREF(stop);
    Py_XDECREF(step);
    Py_XDECREF(upper);
    Py_XDECREF(lower);
    return -1;
}

/* (start, stop, step) as a tuple of values: ints, inline when they fit. */
static PyObject *
indices_tuple(Py_ssize_t start, Py_ssize_t stop, Py_ssize_t step)
{
    Py_ssize_t indices[3] = {start, stop, step};
    PyObject *result = PyTuple_New(3);
    if (result == NULL) {
        return NULL;
    }
    for (int i = 0; i < 3; i++) {
        PyValue index = PyValue_FromInt64(indices[i]);
        if (PyValue_IsNull(index)) {
            Py_DECREF(result);
            return NULL;
        }
        PyTuple_SET_VALUE(result, i, index);
    }
    return result;
}

/* Implementation of slice.indices. */

static PyObject*
slice_indices(PyObject *op, PyObject* len)
{
    PySliceObject *self = _PySlice_CAST(op);
    PyObject *start, *stop, *step;
    PyObject *length;
    int error;
    Py_ssize_t istart, istop, istep;

    /* Convert length to an integer if necessary; raise for negative length. */
    length = PyNumber_Index(len);
    if (length == NULL)
        return NULL;

    if (_PyLong_IsNegative((PyLongObject *)length)) {
        PyErr_SetString(PyExc_ValueError,
                        "length should not be negative");
        Py_DECREF(length);
        return NULL;
    }

    int fast = inline_indices(self, length, &istart, &istop, &istep);
    if (fast != 0) {
        Py_DECREF(length);
        return fast < 0 ? NULL : indices_tuple(istart, istop, istep);
    }
    error = long_indices(self, length, &start, &stop, &step);
    Py_DECREF(length);
    if (error == -1)
        return NULL;
    else
        return Py_BuildValue("(NNN)", start, stop, step);
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
    PySliceObject *self = _PySlice_CAST(op);
    PyObject *fields = _PySlice_AsTuple(self);
    if (fields == NULL) {
        return NULL;
    }
    PyObject *result = PyTuple_Pack(2, Py_TYPE(self), fields);
    Py_DECREF(fields);
    return result;
}

PyDoc_STRVAR(reduce_doc, "Return state information for pickling.");

static PyMethodDef slice_methods[] = {
    {"indices", slice_indices, METH_O, slice_indices_doc},
    {"__reduce__", slice_reduce, METH_NOARGS, reduce_doc},
    {"__class_getitem__", Py_GenericAlias, METH_O|METH_CLASS,
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


    PyObject *t1 = _PySlice_AsTuple(_PySlice_CAST(v));
    if (t1 == NULL) {
        return NULL;
    }

    PyObject *t2 = _PySlice_AsTuple(_PySlice_CAST(w));
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
    Py_uhash_t lane = _PyValue_Hash(v->com); \
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
