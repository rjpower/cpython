/* Range object implementation */

#include "Python.h"
#include "pycore_abstract.h"      // _PyIndex_Check()
#include "pycore_ceval.h"         // _PyEval_GetBuiltin()
#include "pycore_freelist.h"
#include "pycore_long.h"          // _PyLong_GetZero()
#include "pycore_long_value.h"    // _PyValue_AsLong()
#include "pycore_modsupport.h"    // _PyArg_NoKwnames()
#include "pycore_range.h"
#include "pycore_sliceobject.h"   // _PySlice_GetLongIndices()
#include "pycore_tuple.h"         // _PyTuple_ITEMS()
#include "pycore_value_call.h"    // _PyValue_VectorcallThunk()
#include "pycore_value_slots.h"   // _Py_VALUE_THUNK_ITERNEXT
#include "pycore_value_ops.h"     // _PyValue_Add()
#include "pycore_value_api.h"     // _PyValue_FromObjectKnown()

typedef struct {
    PyObject_HEAD
    PyObject *start;
    PyObject *step;
    PyObject *len;
} longrangeiterobject;

/*[clinic input]
value_emission
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=eb7b17088bfd7215]*/

/*[clinic input]
class range_iterator "_PyRangeIterObject *" "&PyRangeIter_Type"
class longrange_iterator "longrangeiterobject *" "&PyLongRangeIter_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=c7d97a63d1cfa6b3]*/

#include "clinic/rangeobject.c.h"


/* Support objects whose length is > PY_SSIZE_T_MAX.

   This could be sped up for small PyLongs if they fit in a Py_ssize_t.
   This only matters on Win64.  Though we could use long long which
   would presumably help perf.
*/

/* start, stop, step and length are owned words of exact ints (value fields,
   V5): inline when they fit, so that range(n) allocates no int. */
typedef struct {
    PyObject_HEAD
    PyValue start;
    PyValue stop;
    PyValue step;
    PyValue length;
} rangeobject;

/* The C long of an exact int word; *overflow as PyLong_AsLongAndOverflow(). */
static long
range_long(PyValue v, int *overflow)
{
    if (PyValue_IsInlineInt(v)) {
        *overflow = 0;
        return (long)PyValue_InlineInt64(v);
    }
    return PyLong_AsLongAndOverflow(PyValue_AsKnownObjectBorrow(v), overflow);
}

static int
range_is_int_value(PyValue v, int64_t n)
{
    return PyValue_IsInlineInt(v) && PyValue_InlineInt64(v) == n;
}

/* Helper function for validating step.  Always returns an owned word or
   PyValue_NULL on error.  PyValue_NULL as the argument means no step was
   specified.
*/
static PyValue
validate_step(PyValue step_in)
{
    /* No step specified, use a step of 1. */
    if (PyValue_IsNull(step_in))
        return PyValue_FromInt64(1);

    PyValue step = _PyValue_IndexExact(step_in);
    if (!PyValue_IsNull(step)
        && (range_is_int_value(step, 0)
            || (PyValue_IsObject(step)
                && _PyLong_IsZero((PyLongObject *)PyValue_AsKnownObjectBorrow(step)))))
    {
        PyErr_SetString(PyExc_ValueError,
                        "range() arg 3 must not be zero");
        PyValue_CLOSE(step);
        return PyValue_NULL;
    }

    return step;
}

static PyValue
compute_range_length(PyValue start, PyValue stop,
                     PyValue step);

/* Takes the owners of start, stop and step on success only. */
static rangeobject *
make_range_object(PyTypeObject *type, PyValue start,
                  PyValue stop, PyValue step)
{
    PyValue length = compute_range_length(start, stop, step);
    if (PyValue_IsNull(length)) {
        return NULL;
    }
    rangeobject *obj = _Py_FREELIST_POP(rangeobject, ranges);
    if (obj == NULL) {
        obj = PyObject_New(rangeobject, type);
        if (obj == NULL) {
            PyValue_CLOSE(length);
            return NULL;
        }
    }
    obj->start = start;
    obj->stop = stop;
    obj->step = step;
    obj->length = length;
    return obj;
}

/* XXX(nnorwitz): should we error check if the user passes any empty ranges?
   range(-10)
   range(0, -5)
   range(0, 5, -1)
*/
static PyObject *
range_from_array(PyTypeObject *type, const PyValue *args, Py_ssize_t num_args)
{
    rangeobject *obj;
    PyValue start = PyValue_NULL, stop = PyValue_NULL, step = PyValue_NULL;
    PyValue step_in = PyValue_NULL;

    switch (num_args) {
        case 3:
            step_in = args[2];
            _Py_FALLTHROUGH;
        case 2:
            start = _PyValue_IndexExact(args[0]);
            if (PyValue_IsNull(start)) {
                return NULL;
            }
            stop = _PyValue_IndexExact(args[1]);
            if (PyValue_IsNull(stop)) {
                PyValue_CLOSE(start);
                return NULL;
            }
            step = validate_step(step_in);  /* Caution, this can clear exceptions */
            if (PyValue_IsNull(step)) {
                PyValue_CLOSE(start);
                PyValue_CLOSE(stop);
                return NULL;
            }
            break;
        case 1:
            stop = _PyValue_IndexExact(args[0]);
            if (PyValue_IsNull(stop)) {
                return NULL;
            }
            start = PyValue_FromInt64(0);
            step = PyValue_FromInt64(1);
            break;
        case 0:
            PyErr_SetString(PyExc_TypeError,
                            "range expected at least 1 argument, got 0");
            return NULL;
        default:
            PyErr_Format(PyExc_TypeError,
                         "range expected at most 3 arguments, got %zd",
                         num_args);
            return NULL;
    }
    obj = make_range_object(type, start, stop, step);
    if (obj != NULL) {
        return (PyObject *) obj;
    }

    /* Failed to create object, release attributes */
    PyValue_CLOSE(start);
    PyValue_CLOSE(stop);
    PyValue_CLOSE(step);
    return NULL;
}

static PyObject *
range_new(PyTypeObject *type, PyObject *args, PyObject *kw)
{
    if (!_PyArg_NoKeywords("range", kw))
        return NULL;

    return range_from_array(type, _PyTuple_ITEMS(args), PyTuple_GET_SIZE(args));
}

static PyValue
range_vectorcall(PyThreadState *tstate, PyObject *rangetype,
                 const PyValue *args, size_t nargsf, PyObject *kwnames)
{
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    if (!_PyArg_NoKwnames("range", kwnames)) {
        return PyValue_NULL;
    }
    return _PyValue_FromObjectResult(
        range_from_array((PyTypeObject *)rangetype, args, nargs));
}

PyDoc_STRVAR(range_doc,
"range(stop) -> range object\n\
range(start, stop[, step]) -> range object\n\
\n\
Return an object that produces a sequence of integers from start (inclusive)\n\
to stop (exclusive) by step.  range(i, j) produces i, i+1, i+2, ..., j-1.\n\
start defaults to 0, and stop is omitted!  range(4) produces 0, 1, 2, 3.\n\
These are exactly the valid indices for a list of 4 elements.\n\
When step is given, it specifies the increment (or decrement).");

static void
range_dealloc(PyObject *op)
{
    rangeobject *r = (rangeobject*)op;
    PyValue_CLOSE(r->start);
    PyValue_CLOSE(r->stop);
    PyValue_CLOSE(r->step);
    PyValue_CLOSE(r->length);
    _Py_FREELIST_FREE(ranges, r, PyObject_Free);
}

static unsigned long
get_len_of_range(long lo, long hi, long step);

/* Return the length as a long, -2 for an overflow and -1 for any other type of error
 *
 * In case of an overflow no error is set
 */
static long compute_range_length_long(PyValue start,
                PyValue stop, PyValue step) {
    int overflow = 0;

    long long_start = range_long(start, &overflow);
    if (overflow) {
        return -2;
    }
    if (long_start == -1 && PyErr_Occurred()) {
        return -1;
    }
    long long_stop = range_long(stop, &overflow);
    if (overflow) {
        return -2;
    }
    if (long_stop == -1 && PyErr_Occurred()) {
        return -1;
    }
    long long_step = range_long(step, &overflow);
    if (overflow) {
        return -2;
    }
    if (long_step == -1 && PyErr_Occurred()) {
        return -1;
    }

    unsigned long ulen = get_len_of_range(long_start, long_stop, long_step);
    if (ulen > (unsigned long)LONG_MAX) {
        /* length too large for a long */
        return -2;
    }
    else {
        return (long)ulen;
    }
}

/* Return number of items in range (lo, hi, step) as an owned int word,
 * when arguments are exact int words.  Return PyValue_NULL when there is an
 * error.
 */
static PyValue
compute_range_length(PyValue start, PyValue stop,
                     PyValue step)
{
    /* -------------------------------------------------------------
    Algorithm is equal to that of get_len_of_range(), but it operates
    on int words of any size.
    ---------------------------------------------------------------*/
    int cmp_result;
    PyValue lo, hi;
    PyValue nstep;      /* owned: |step| */
    PyValue tmp1 = PyValue_NULL, diff = PyValue_NULL, tmp2 = PyValue_NULL;
    PyValue result = PyValue_NULL;
                /* holds sub-expression evaluations */

    PyValue zero = PyValue_FromInt64(0);
    PyValue one = PyValue_FromInt64(1);

    /* fast path when all arguments fit into a long integer */
    long len = compute_range_length_long(start, stop, step);
    if (len >= 0) {
        return PyValue_FromInt64(len);
    }
    else if (len == -1) {
        /* unexpected error from compute_range_length_long, we propagate to the caller */
        return PyValue_NULL;
    }
    assert(len == -2);

    cmp_result = _PyValue_RichCompareBool(step, zero, Py_GT);
    if (cmp_result == -1)
        return PyValue_NULL;

    if (cmp_result == 1) {
        lo = start;
        hi = stop;
        nstep = PyValue_DUP(step);
    } else {
        lo = stop;
        hi = start;
        nstep = _PyValue_Negative(step);
        if (PyValue_IsNull(nstep))
            return PyValue_NULL;
    }

    /* if (lo >= hi), return length of 0. */
    cmp_result = _PyValue_RichCompareBool(lo, hi, Py_GE);
    if (cmp_result != 0) {
        PyValue_CLOSE(nstep);
        if (cmp_result < 0)
            return PyValue_NULL;
        return zero;
    }

    tmp1 = _PyValue_Subtract(hi, lo);
    if (PyValue_IsNull(tmp1))
        goto Fail;

    diff = _PyValue_Subtract(tmp1, one);
    if (PyValue_IsNull(diff))
        goto Fail;

    tmp2 = _PyValue_FloorDivide(diff, nstep);
    if (PyValue_IsNull(tmp2))
        goto Fail;

    result = _PyValue_Add(tmp2, one);
    if (PyValue_IsNull(result))
        goto Fail;

    PyValue_CLOSE(tmp2);
    PyValue_CLOSE(diff);
    PyValue_CLOSE(nstep);
    PyValue_CLOSE(tmp1);
    return result;

  Fail:
    PyValue_CLOSE(nstep);
    PyValue_XCLOSE(tmp2);
    PyValue_XCLOSE(diff);
    PyValue_XCLOSE(tmp1);
    return PyValue_NULL;
}

static Py_ssize_t
range_length(PyObject *op)
{
    rangeobject *r = (rangeobject*)op;
    return _PyValue_IntAsSsize_t(r->length);
}

/* start + i * step, for an int word i (borrowed); an owned word. */
static PyValue
compute_item(rangeobject *r, PyValue i)
{
    if (range_is_int_value(r->step, 1)) {
        return _PyValue_Add(r->start, i);
    }
    PyValue incr = _PyValue_Multiply(i, r->step);
    if (PyValue_IsNull(incr)) {
        return PyValue_NULL;
    }
    PyValue result = _PyValue_Add(r->start, incr);
    PyValue_CLOSE(incr);
    return result;
}

/* The item at the index arg, an int word (borrowed); an owned word. */
static PyValue
compute_range_item(rangeobject *r, PyValue arg)
{
    PyValue zero = PyValue_FromInt64(0);
    int cmp_result;
    PyValue i;

    /* PyLong equivalent to:
     *   if (arg < 0) {
     *     i = r->length + arg
     *   } else {
     *     i = arg
     *   }
     */
    cmp_result = _PyValue_RichCompareBool(arg, zero, Py_LT);
    if (cmp_result == -1) {
        return PyValue_NULL;
    }
    if (cmp_result == 1) {
        i = _PyValue_Add(r->length, arg);
        if (PyValue_IsNull(i)) {
            return PyValue_NULL;
        }
    } else {
        i = PyValue_DUP(arg);
    }

    /* PyLong equivalent to:
     *   if (i < 0 || i >= r->length) {
     *     <report index out of bounds>
     *   }
     */
    cmp_result = _PyValue_RichCompareBool(i, zero, Py_LT);
    if (cmp_result == 0) {
        cmp_result = _PyValue_RichCompareBool(i, r->length, Py_GE);
    }
    if (cmp_result == -1) {
       PyValue_CLOSE(i);
       return PyValue_NULL;
    }
    if (cmp_result == 1) {
        PyValue_CLOSE(i);
        PyErr_SetString(PyExc_IndexError,
                        "range object index out of range");
        return PyValue_NULL;
    }

    PyValue result = compute_item(r, i);
    PyValue_CLOSE(i);
    return result;
}

static PyObject *
range_item_object(PyObject *op, Py_ssize_t i)
{
    PyValue arg = PyValue_FromInt64(i);
    if (PyValue_IsNull(arg)) {
        return NULL;
    }
    PyValue res = compute_range_item((rangeobject *)op, arg);
    PyValue_CLOSE(arg);
    return PyValue_IsNull(res) ? NULL : _PyValue_AsObjectStealAt(res, BOX_SITE_SEQUENCE);
}

/* The value forms index and test membership in C integers when start, step
   and length all fit 32 bits (so i * step + start cannot overflow 64 bits);
   other ranges take the PyLong arithmetic of the object forms. */
static int
range_compact(rangeobject *r, int64_t *start, int64_t *step, int64_t *length)
{
    long v[3];
    PyValue fields[3] = {r->start, r->step, r->length};
    for (int k = 0; k < 3; k++) {
        int overflow;
        v[k] = range_long(fields[k], &overflow);
        if (overflow || v[k] < INT32_MIN || v[k] > INT32_MAX) {
            return 0;
        }
    }
    *start = v[0];
    *step = v[1];
    *length = v[2];
    return 1;
}

/* Returns 0 when the range is not compact. Otherwise stores the item, or
   PyValue_NULL with IndexError set, in *res. */
static int
range_item_compact(rangeobject *r, int64_t i, PyValue *res)
{
    int64_t start, step, length;
    if (!range_compact(r, &start, &step, &length)) {
        return 0;
    }
    if (i < 0) {
        i += length;
    }
    if (i < 0 || i >= length) {
        PyErr_SetString(PyExc_IndexError, "range object index out of range");
        *res = PyValue_NULL;
        return 1;
    }
    *res = PyValue_FromInt64(start + i * step);
    return 1;
}

static PyValue
range_item_value(PyObject *op, Py_ssize_t i)
{
    PyValue res;
    if (range_item_compact((rangeobject *)op, i, &res)) {
        return res;
    }
    return _PyValue_FromObjectResult(range_item_object(op, i));
}
_Py_VALUE_THUNK_ITEM(range_item, range_item_value)

/* r[slice] for a compact range and a slice of inline ints and None, in
   machine arithmetic (no int object is made). Returns 0 if the case does not
   apply; otherwise 1, with the new range or NULL with an exception set in
   *result. */
static int
compute_slice_compact(rangeobject *r, PySliceObject *slice, PyObject **result)
{
    int64_t start, step, length;
    if (!range_compact(r, &start, &step, &length)) {
        return 0;
    }
    PyValueBorrow lower = PyValue_Borrow(slice->start);
    PyValueBorrow upper = PyValue_Borrow(slice->stop);
    PyValueBorrow stride = PyValue_Borrow(slice->step);
    if ((!PyValue_IsNone(lower) && !PyValue_IsInlineInt(lower)) ||
        (!PyValue_IsNone(upper) && !PyValue_IsInlineInt(upper)) ||
        (!PyValue_IsNone(stride) && !PyValue_IsInlineInt(stride))) {
        return 0;
    }
    Py_ssize_t s, e, st;
    if (PySlice_Unpack(_PyObject_CAST(slice), &s, &e, &st) < 0) {
        *result = NULL;
        return 1;
    }
    PySlice_AdjustIndices(length, &s, &e, st);
    int64_t new_start = start + (int64_t)s * step;
    int64_t new_stop = start + (int64_t)e * step;
    int64_t new_step;
    if (__builtin_mul_overflow(step, (int64_t)st, &new_step)
        || !_PyValue_IntFitsInline(new_start)
        || !_PyValue_IntFitsInline(new_stop)
        || !_PyValue_IntFitsInline(new_step))
    {
        return 0;
    }
    *result = (PyObject *)make_range_object(
        Py_TYPE(r), PyValue_FromInt64(new_start), PyValue_FromInt64(new_stop),
        PyValue_FromInt64(new_step));
    return 1;
}

static PyObject *
compute_slice(rangeobject *r, PyObject *_slice)
{
    PySliceObject *slice = (PySliceObject *) _slice;
    PyObject *compact_result;
    if (compute_slice_compact(r, slice, &compact_result)) {
        return compact_result;
    }
    PyObject *start = NULL, *stop = NULL, *step = NULL;
    PyValue substart = PyValue_NULL, substop = PyValue_NULL, substep = PyValue_NULL;

    /* _PySlice_GetLongIndices() takes the length as an object. */
    PyObject *length = _PyValue_AsObjectAt(r->length, BOX_SITE_FIELD);
    if (length == NULL) {
        return NULL;
    }
    int error = _PySlice_GetLongIndices(slice, length, &start, &stop, &step);
    Py_DECREF(length);
    if (error == -1)
        return NULL;

    substep = _PyValue_Multiply(r->step, PyValue_FromObjectBorrow(step));
    if (PyValue_IsNull(substep)) goto fail;

    PyValue vstart = PyValue_DUP(PyValue_FromObjectBorrow(start));
    substart = compute_item(r, vstart);
    PyValue_CLOSE(vstart);
    if (PyValue_IsNull(substart)) goto fail;

    PyValue vstop = PyValue_DUP(PyValue_FromObjectBorrow(stop));
    substop = compute_item(r, vstop);
    PyValue_CLOSE(vstop);
    if (PyValue_IsNull(substop)) goto fail;

    rangeobject *result = make_range_object(Py_TYPE(r), substart, substop, substep);
    if (result != NULL) {
        Py_DECREF(start);
        Py_DECREF(stop);
        Py_DECREF(step);
        return (PyObject *) result;
    }
fail:
    Py_XDECREF(start);
    Py_XDECREF(stop);
    Py_XDECREF(step);
    PyValue_XCLOSE(substart);
    PyValue_XCLOSE(substop);
    PyValue_XCLOSE(substep);
    return NULL;
}

/* Assumes (PyLong_CheckExact(ob) || PyBool_Check(ob)) */
static int
range_contains_long(rangeobject *r, PyObject *ob)
{
    PyObject *zero = _PyLong_GetZero();  // borrowed reference
    int cmp1, cmp2, cmp3;
    PyObject *tmp1 = NULL;
    PyObject *tmp2 = NULL;
    int result = -1;

    /* Check if the value can possibly be in the range. */

    cmp1 = _PyValue_RichCompareBool(r->step, PyValue_FromObjectBorrow(zero), Py_GT);
    if (cmp1 == -1)
        goto end;
    if (cmp1 == 1) { /* positive steps: start <= ob < stop */
        cmp2 = _PyValue_RichCompareBool(r->start, PyValue_FromObjectBorrow(ob), Py_LE);
        cmp3 = _PyValue_RichCompareBool(PyValue_FromObjectBorrow(ob), r->stop, Py_LT);
    }
    else { /* negative steps: stop < ob <= start */
        cmp2 = _PyValue_RichCompareBool(PyValue_FromObjectBorrow(ob), r->start, Py_LE);
        cmp3 = _PyValue_RichCompareBool(r->stop, PyValue_FromObjectBorrow(ob), Py_LT);
    }

    if (cmp2 == -1 || cmp3 == -1) /* TypeError */
        goto end;
    if (cmp2 == 0 || cmp3 == 0) { /* ob outside of range */
        result = 0;
        goto end;
    }

    /* Check that the stride does not invalidate ob's membership. */
    PyValue tmp1_v = _PyValue_Subtract(PyValue_FromObjectBorrow(ob), r->start);
    tmp1 = PyValue_IsNull(tmp1_v) ? NULL
        : _PyValue_AsObjectStealAt(tmp1_v, BOX_SITE_NUMBER);
    if (tmp1 == NULL)
        goto end;
    PyValue tmp2_v = _PyValue_Remainder(PyValue_FromObjectBorrow(tmp1), r->step);
    tmp2 = PyValue_IsNull(tmp2_v) ? NULL
        : _PyValue_AsObjectStealAt(tmp2_v, BOX_SITE_NUMBER);
    if (tmp2 == NULL)
        goto end;
    /* result = ((int(ob) - start) % step) == 0 */
    result = _PyValue_RichCompareBool(PyValue_FromObjectBorrow(tmp2), PyValue_FromObjectBorrow(zero), Py_EQ);
  end:
    Py_XDECREF(tmp1);
    Py_XDECREF(tmp2);
    return result;
}

static int
range_contains_object(PyObject *self, PyObject *ob)
{
    rangeobject *r = (rangeobject*)self;
    if (PyLong_CheckExact(ob) || PyBool_Check(ob))
        return range_contains_long(r, ob);

    return (int)_PySequence_IterSearch((PyObject*)r, ob,
                                       PY_ITERSEARCH_CONTAINS);
}

static int
range_contains_value(PyObject *self, PyValueBorrow ob)
{
    rangeobject *r = (rangeobject *)self;
    int64_t start, step, length;
    if (PyValue_IsInlineInt(ob) && range_compact(r, &start, &step, &length)) {
        int64_t d = PyValue_InlineInt64(ob) - start;
        if (length == 0 || step == 0 || d % step != 0) {
            return 0;
        }
        int64_t q = d / step;
        return q >= 0 && q < length;
    }
    PyObject *obj = _PyValue_AsObjectAt(ob, BOX_SITE_SEQUENCE);
    if (obj == NULL) {
        return -1;
    }
    int result = range_contains_object(self, obj);
    Py_DECREF(obj);
    return result;
}
_Py_VALUE_THUNK_CONTAINS(range_contains, range_contains_value)

/* Compare two range objects.  Return 1 for equal, 0 for not equal
   and -1 on error.  The algorithm is roughly the C equivalent of

   if r0 is r1:
       return True
   if len(r0) != len(r1):
       return False
   if not len(r0):
       return True
   if r0.start != r1.start:
       return False
   if len(r0) == 1:
       return True
   return r0.step == r1.step
*/
static int
range_equals(rangeobject *r0, rangeobject *r1)
{
    int cmp_result;

    if (r0 == r1)
        return 1;
    cmp_result = _PyValue_RichCompareBool(r0->length, r1->length, Py_EQ);
    /* Return False or error to the caller. */
    if (cmp_result != 1)
        return cmp_result;
    cmp_result = _PyValue_Not(r0->length);
    /* Return True or error to the caller. */
    if (cmp_result != 0)
        return cmp_result;
    cmp_result = _PyValue_RichCompareBool(r0->start, r1->start, Py_EQ);
    /* Return False or error to the caller. */
    if (cmp_result != 1)
        return cmp_result;
    cmp_result = _PyValue_RichCompareBool(r0->length, PyValue_FromObjectBorrow(_PyLong_GetOne()), Py_EQ);
    /* Return True or error to the caller. */
    if (cmp_result != 0)
        return cmp_result;
    return _PyValue_RichCompareBool(r0->step, r1->step, Py_EQ);
}

static PyObject *
range_richcompare(PyObject *self, PyObject *other, int op)
{
    int result;

    if (!PyRange_Check(other))
        Py_RETURN_NOTIMPLEMENTED;
    switch (op) {
    case Py_NE:
    case Py_EQ:
        result = range_equals((rangeobject*)self, (rangeobject*)other);
        if (result == -1)
            return NULL;
        if (op == Py_NE)
            result = !result;
        if (result)
            Py_RETURN_TRUE;
        else
            Py_RETURN_FALSE;
    case Py_LE:
    case Py_GE:
    case Py_LT:
    case Py_GT:
        Py_RETURN_NOTIMPLEMENTED;
    default:
        PyErr_BadArgument();
        return NULL;
    }
}

/* Hash function for range objects.  Rough C equivalent of

   if not len(r):
       return hash((len(r), None, None))
   if len(r) == 1:
       return hash((len(r), r.start, None))
   return hash((len(r), r.start, r.step))
*/
static Py_hash_t
range_hash(PyObject *op)
{
    rangeobject *r = (rangeobject*)op;
    PyObject *t;
    Py_hash_t result = -1;
    int cmp_result;

    t = PyTuple_New(3);
    if (!t)
        return -1;
    PyTuple_SET_VALUE(t, 0, PyValue_DUP(r->length));
    cmp_result = _PyValue_Not(r->length);
    if (cmp_result == -1)
        goto end;
    if (cmp_result == 1) {
        PyTuple_SET_VALUE(t, 1, PyValue_DUP(_PyValue_FromObjectKnown(Py_None)));
        PyTuple_SET_VALUE(t, 2, PyValue_DUP(_PyValue_FromObjectKnown(Py_None)));
    }
    else {
        PyTuple_SET_VALUE(t, 1, PyValue_DUP(r->start));
        cmp_result = _PyValue_RichCompareBool(r->length, PyValue_FromObjectBorrow(_PyLong_GetOne()), Py_EQ);
        if (cmp_result == -1)
            goto end;
        if (cmp_result == 1) {
            PyTuple_SET_VALUE(t, 2, PyValue_DUP(_PyValue_FromObjectKnown(Py_None)));
        }
        else {
            PyTuple_SET_VALUE(t, 2, PyValue_DUP(r->step));
        }
    }
    result = _PyValue_Hash(PyValue_FromObjectBorrow(t));
  end:
    Py_DECREF(t);
    return result;
}

static PyObject *
range_count(PyObject *self, PyObject *ob)
{
    rangeobject *r = (rangeobject*)self;
    if (PyLong_CheckExact(ob) || PyBool_Check(ob)) {
        int result = range_contains_long(r, ob);
        if (result == -1)
            return NULL;
        return PyLong_FromLong(result);
    } else {
        Py_ssize_t count;
        count = _PySequence_IterSearch((PyObject*)r, ob, PY_ITERSEARCH_COUNT);
        if (count == -1)
            return NULL;
        return PyLong_FromSsize_t(count);
    }
}

static PyObject *
range_index(PyObject *self, PyObject *ob)
{
    rangeobject *r = (rangeobject*)self;
    int contains;

    if (!PyLong_CheckExact(ob) && !PyBool_Check(ob)) {
        Py_ssize_t index;
        index = _PySequence_IterSearch((PyObject*)r, ob, PY_ITERSEARCH_INDEX);
        if (index == -1)
            return NULL;
        return PyLong_FromSsize_t(index);
    }

    contains = range_contains_long(r, ob);
    if (contains == -1)
        return NULL;

    if (contains) {
        PyValue idx_v = _PyValue_Subtract(PyValue_FromObjectBorrow(ob), r->start);
        PyObject *idx = PyValue_IsNull(idx_v) ? NULL
            : _PyValue_AsObjectStealAt(idx_v, BOX_SITE_NUMBER);
        if (idx == NULL) {
            return NULL;
        }

        if (range_is_int_value(r->step, 1)) {
            return idx;
        }

        /* idx = (ob - r.start) // r.step */
        PyValue sidx_v = _PyValue_FloorDivide(PyValue_FromObjectBorrow(idx), r->step);
        PyObject *sidx = PyValue_IsNull(sidx_v) ? NULL
            : _PyValue_AsObjectStealAt(sidx_v, BOX_SITE_NUMBER);
        Py_DECREF(idx);
        return sidx;
    }

    /* object is not in the range */
    PyErr_SetString(PyExc_ValueError, "range.index(x): x not in range");
    return NULL;
}

static PySequenceMethods range_as_sequence = {
    range_length,               /* sq_length */
    0,                          /* sq_concat */
    0,                          /* sq_repeat */
    range_item,                 /* sq_item */
    0,                          /* sq_slice */
    0,                          /* sq_ass_item */
    0,                          /* sq_ass_slice */
    range_contains,             /* sq_contains */
};

static PyObject *
range_repr(PyObject *op)
{
    rangeobject *r = (rangeobject*)op;
    Py_ssize_t istep;

    /* Check for special case values for printing.  We don't always
       need the step value.  We don't care about overflow. */
    istep = _PyValue_AsSsize_t(r->step, NULL);
    if (istep == -1 && PyErr_Occurred()) {
        assert(!PyErr_ExceptionMatches(PyExc_OverflowError));
        return NULL;
    }

    PyObject *start = _PyValue_AsObjectAt(r->start, BOX_SITE_FIELD);
    PyObject *stop = _PyValue_AsObjectAt(r->stop, BOX_SITE_FIELD);
    PyObject *step = istep == 1 ? NULL
                                : _PyValue_AsObjectAt(r->step, BOX_SITE_FIELD);
    PyObject *res = NULL;
    if (start != NULL && stop != NULL && (istep == 1 || step != NULL)) {
        if (istep == 1)
            res = PyUnicode_FromFormat("range(%R, %R)", start, stop);
        else
            res = PyUnicode_FromFormat("range(%R, %R, %R)", start, stop, step);
    }
    Py_XDECREF(start);
    Py_XDECREF(stop);
    Py_XDECREF(step);
    return res;
}

/* Pickling support */
static PyObject *
range_reduce(PyObject *op, PyObject *args)
{
    rangeobject *r = (rangeobject*)op;
    PyObject *start = _PyValue_AsObjectAt(r->start, BOX_SITE_FIELD);
    PyObject *stop = _PyValue_AsObjectAt(r->stop, BOX_SITE_FIELD);
    PyObject *step = _PyValue_AsObjectAt(r->step, BOX_SITE_FIELD);
    PyObject *res = NULL;
    if (start != NULL && stop != NULL && step != NULL) {
        res = Py_BuildValue("(O(OOO))", Py_TYPE(r), start, stop, step);
    }
    Py_XDECREF(start);
    Py_XDECREF(stop);
    Py_XDECREF(step);
    return res;
}

static PyObject *
range_subscript_object(PyObject *op, PyObject *item)
{
    rangeobject *self = (rangeobject*)op;
    if (_PyIndex_Check(item)) {
        PyValue i = _PyValue_IndexExact(PyValue_FromObjectBorrow(item));
        if (PyValue_IsNull(i))
            return NULL;
        PyValue result = compute_range_item(self, i);
        PyValue_CLOSE(i);
        return PyValue_IsNull(result) ? NULL
            : _PyValue_AsObjectStealAt(result, BOX_SITE_SEQUENCE);
    }
    if (PySlice_Check(item)) {
        return compute_slice(self, item);
    }
    PyErr_Format(PyExc_TypeError,
                 "range indices must be integers or slices, not %.200s",
                 Py_TYPE(item)->tp_name);
    return NULL;
}

static PyValue
range_subscript_value(PyObject *op, PyValueBorrow key)
{
    PyValue res;
    if (PyValue_IsInlineInt(key) &&
        range_item_compact((rangeobject *)op, PyValue_InlineInt64(key), &res)) {
        return res;
    }
    PyObject *item = _PyValue_AsObjectAt(key, BOX_SITE_SEQUENCE);
    if (item == NULL) {
        return PyValue_NULL;
    }
    PyObject *result = range_subscript_object(op, item);
    Py_DECREF(item);
    return _PyValue_FromObjectResult(result);
}
_Py_VALUE_THUNK_SUBSCRIPT(range_subscript, range_subscript_value)


static PyMappingMethods range_as_mapping = {
        range_length,                /* mp_length */
        range_subscript,             /* mp_subscript */
        0,                           /* mp_ass_subscript */
};

static int
range_bool(PyObject *op)
{
    rangeobject *self = (rangeobject*)op;
    return _PyValue_IsTrue(self->length);
}

static PyNumberMethods range_as_number = {
    .nb_bool = range_bool,
};

static PyObject * range_iter(PyObject *seq);
static PyObject * range_reverse(PyObject *seq, PyObject *Py_UNUSED(ignored));

PyDoc_STRVAR(reverse_doc,
"Return a reverse iterator.");

PyDoc_STRVAR(count_doc,
"rangeobject.count(value) -> integer -- return number of occurrences of value");

PyDoc_STRVAR(index_doc,
"rangeobject.index(value) -> integer -- return index of value.\n"
"Raise ValueError if the value is not present.");

static PyMethodDef range_methods[] = {
    {"__reversed__",    range_reverse,              METH_NOARGS, reverse_doc},
    {"__reduce__",      range_reduce,               METH_NOARGS},
    {"count",           range_count,                METH_O,      count_doc},
    {"index",           range_index,                METH_O,      index_doc},
    {NULL,              NULL}           /* sentinel */
};

static PyMemberDef range_members[] = {
    {"start",   _Py_T_VALUE,       offsetof(rangeobject, start),   Py_READONLY},
    {"stop",    _Py_T_VALUE,       offsetof(rangeobject, stop),    Py_READONLY},
    {"step",    _Py_T_VALUE,       offsetof(rangeobject, step),    Py_READONLY},
    {0}
};

/* range is not a GC type: its members are ints, which cannot form cycles.
   The traverse function lets the tracing mark reach them. */
static int
range_traverse(PyObject *op, visitproc visit, void *arg)
{
    rangeobject *r = (rangeobject *)op;
    _Py_VISIT_VALUE(r->start);
    _Py_VISIT_VALUE(r->stop);
    _Py_VISIT_VALUE(r->step);
    _Py_VISIT_VALUE(r->length);
    return 0;
}

_Py_VALUE_NOTIMPL_RICHCMP(range_richcompare_value, range_richcompare)

PyTypeObject PyRange_Type = {
        PyVarObject_HEAD_INIT(&PyType_Type, 0)
        "range",                /* Name of this type */
        sizeof(rangeobject),    /* Basic object size */
        0,                      /* Item size for varobject */
        range_dealloc,          /* tp_dealloc */
        0,                      /* tp_vectorcall_offset */
        0,                      /* tp_getattr */
        0,                      /* tp_setattr */
        0,                      /* tp_as_async */
        range_repr,             /* tp_repr */
        &range_as_number,       /* tp_as_number */
        &range_as_sequence,     /* tp_as_sequence */
        &range_as_mapping,      /* tp_as_mapping */
        range_hash,             /* tp_hash */
        0,                      /* tp_call */
        0,                      /* tp_str */
        PyObject_GenericGetAttr,  /* tp_getattro */
        0,                      /* tp_setattro */
        0,                      /* tp_as_buffer */
        Py_TPFLAGS_DEFAULT | Py_TPFLAGS_SEQUENCE,  /* tp_flags */
        range_doc,              /* tp_doc */
        range_traverse,         /* tp_traverse */
        0,                      /* tp_clear */
        range_richcompare,      /* tp_richcompare */
        0,                      /* tp_weaklistoffset */
        range_iter,             /* tp_iter */
        0,                      /* tp_iternext */
        range_methods,          /* tp_methods */
        range_members,          /* tp_members */
        0,                      /* tp_getset */
        0,                      /* tp_base */
        0,                      /* tp_dict */
        0,                      /* tp_descr_get */
        0,                      /* tp_descr_set */
        0,                      /* tp_dictoffset */
        0,                      /* tp_init */
        0,                      /* tp_alloc */
        range_new,              /* tp_new */
        .tp_vectorcall = _PyValue_VectorcallThunk,
        .tp_value_vectorcall = (_Py_valuefunc)range_vectorcall,
    .tp_value = {
        .richcompare = _Py_VALUE_SLOT_INIT(richcompare, range_richcompare_value),
        .subscript = _Py_VALUE_SLOT_INIT(subscript, range_subscript_value),
        .item = _Py_VALUE_SLOT_INIT(item, range_item_value),
        .contains = _Py_VALUE_SLOT_INIT(contains, range_contains_value),
    },
};

/*********************** range Iterator **************************/

/* There are 2 types of iterators, one for C longs, the other for
   Python ints (ie, PyObjects).  This should make iteration fast
   in the normal case, but possible for any numeric value.
*/

static _PyStatusValue
rangeiter_next_value(PyObject *op)
{
    _PyStatusValue ret = {PYGEN_RETURN, PyValue_NULL};
    Py_BEGIN_CRITICAL_SECTION(op);
    _PyRangeIterObject *r = (_PyRangeIterObject*)op;
    if (r->len > 0) {
        long result = r->start;
        r->start = result + r->step;
        r->len--;
        PyValue v = PyValue_FromInt64(result);
        ret = (_PyStatusValue){PyValue_IsNull(v) ? PYGEN_ERROR : PYGEN_NEXT, v};
    }
    Py_END_CRITICAL_SECTION();
    return ret;
}

_Py_VALUE_THUNK_ITERNEXT(rangeiter_next, rangeiter_next_value)

/*[clinic input]
@critical_section
range_iterator.__length_hint__
    self as r: self(type="_PyRangeIterObject *")

Private method returning an estimate of len(list(it)).
[clinic start generated code]*/

static PyValue
range_iterator___length_hint___impl(_PyRangeIterObject *r)
/*[clinic end generated code: output=28f31f84e6bfd668 input=e3eb311e99d76e43]*/
{
    return PyValue_FromInt64(r->len);
}

/*[clinic input]
@critical_section
range_iterator.__reduce__
    self as r: self(type="_PyRangeIterObject *")

Return state information for pickling.
[clinic start generated code]*/

static PyValue
range_iterator___reduce___impl(_PyRangeIterObject *r)
/*[clinic end generated code: output=012c49f64ed753ba input=75a25b7076dc2c54]*/
{
    PyValue start = PyValue_FromInt64(r->start);
    PyValue stop = PyValue_FromInt64(r->start + r->len * r->step);
    PyValue step = PyValue_FromInt64(r->step);

    /* create a range object for pickling */
    PyObject *range = (PyObject*)make_range_object(&PyRange_Type,
                                                   start, stop, step);
    if (range == NULL) {
        PyValue_CLOSE(start);
        PyValue_CLOSE(stop);
        PyValue_CLOSE(step);
        return PyValue_NULL;
    }
    /* return the result */
    return _PyValue_FromObjectResult(Py_BuildValue("N(N)O", _PyEval_GetBuiltin(&_Py_ID(iter)),
                         range, Py_None));
}

/*[clinic input]
@critical_section
range_iterator.__setstate__
    self as r: self(type="_PyRangeIterObject *")
    state: object
    /

Set state information for unpickling.
[clinic start generated code]*/

static PyValue
range_iterator___setstate___impl(_PyRangeIterObject *r, PyValueBorrow state)
/*[clinic end generated code: output=e3204e7bd8ee0e07 input=c8c84fab2519d200]*/
{
    long index = _PyValue_AsLong(state);
    if (index == -1 && PyErr_Occurred())
        return PyValue_NULL;
    /* silently clip the index value */
    if (index < 0)
        index = 0;
    else if (index > r->len)
        index = r->len; /* exhausted iterator */
    r->start += index * r->step;
    r->len -= index;
    return PyValue_None;
}

static void
rangeiter_dealloc(PyObject *self)
{
    _Py_FREELIST_FREE(range_iters, (_PyRangeIterObject *)self, PyObject_Free);
}

static PyMethodDef rangeiter_methods[] = {
    RANGE_ITERATOR___LENGTH_HINT___METHODDEF
    RANGE_ITERATOR___REDUCE___METHODDEF
    RANGE_ITERATOR___SETSTATE___METHODDEF
    {NULL,              NULL}           /* sentinel */
};

PyTypeObject PyRangeIter_Type = {
        PyVarObject_HEAD_INIT(&PyType_Type, 0)
        "range_iterator",                       /* tp_name */
        sizeof(_PyRangeIterObject),             /* tp_basicsize */
        0,                                      /* tp_itemsize */
        /* methods */
        rangeiter_dealloc,                      /* tp_dealloc */
        0,                                      /* tp_vectorcall_offset */
        0,                                      /* tp_getattr */
        0,                                      /* tp_setattr */
        0,                                      /* tp_as_async */
        0,                                      /* tp_repr */
        0,                                      /* tp_as_number */
        0,                                      /* tp_as_sequence */
        0,                                      /* tp_as_mapping */
        0,                                      /* tp_hash */
        0,                                      /* tp_call */
        0,                                      /* tp_str */
        PyObject_GenericGetAttr,                /* tp_getattro */
        0,                                      /* tp_setattro */
        0,                                      /* tp_as_buffer */
        Py_TPFLAGS_DEFAULT,                     /* tp_flags */
        0,                                      /* tp_doc */
        0,                                      /* tp_traverse */
        0,                                      /* tp_clear */
        0,                                      /* tp_richcompare */
        0,                                      /* tp_weaklistoffset */
        PyObject_SelfIter,                      /* tp_iter */
        rangeiter_next,                         /* tp_iternext */
        rangeiter_methods,                      /* tp_methods */
        0,                                      /* tp_members */
        .tp_value = {
            .iternext = _Py_VALUE_SLOT_INIT(iternext, rangeiter_next_value),
        },
};

/* Return number of items in range (lo, hi, step).  step != 0
 * required.  The result always fits in an unsigned long.
 */
static unsigned long
get_len_of_range(long lo, long hi, long step)
{
    /* -------------------------------------------------------------
    If step > 0 and lo >= hi, or step < 0 and lo <= hi, the range is empty.
    Else for step > 0, if n values are in the range, the last one is
    lo + (n-1)*step, which must be <= hi-1.  Rearranging,
    n <= (hi - lo - 1)/step + 1, so taking the floor of the RHS gives
    the proper value.  Since lo < hi in this case, hi-lo-1 >= 0, so
    the RHS is non-negative and so truncation is the same as the
    floor.  Letting M be the largest positive long, the worst case
    for the RHS numerator is hi=M, lo=-M-1, and then
    hi-lo-1 = M-(-M-1)-1 = 2*M.  Therefore unsigned long has enough
    precision to compute the RHS exactly.  The analysis for step < 0
    is similar.
    ---------------------------------------------------------------*/
    assert(step != 0);
    if (step > 0 && lo < hi)
        return 1UL + (hi - 1UL - lo) / step;
    else if (step < 0 && lo > hi)
        return 1UL + (lo - 1UL - hi) / (0UL - step);
    else
        return 0UL;
}

/* Initialize a rangeiter object.  If the length of the rangeiter object
   is not representable as a C long, OverflowError is raised. */

static PyObject *
fast_range_iter(long start, long stop, long step, long len)
{
    _PyRangeIterObject *it = _Py_FREELIST_POP(_PyRangeIterObject, range_iters);
    if (it == NULL) {
        it = PyObject_New(_PyRangeIterObject, &PyRangeIter_Type);
        if (it == NULL) {
            return NULL;
        }
    }
    assert(Py_IS_TYPE(it, &PyRangeIter_Type));
    it->start = start;
    it->step = step;
    it->len = len;
    return (PyObject *)it;
}

/*[clinic input]
@critical_section
longrange_iterator.__length_hint__
    self as r: self(type="longrangeiterobject *")

Private method returning an estimate of len(list(it)).
[clinic start generated code]*/

static PyValue
longrange_iterator___length_hint___impl(longrangeiterobject *r)
/*[clinic end generated code: output=e57dcdffeda56119 input=ba94b050d940411e]*/
{
    Py_INCREF(r->len);
    return _PyValue_FromObjectResult(r->len);
}

/*[clinic input]
@critical_section
longrange_iterator.__reduce__
    self as r: self(type="longrangeiterobject *")

Return state information for pickling.
[clinic start generated code]*/

static PyValue
longrange_iterator___reduce___impl(longrangeiterobject *r)
/*[clinic end generated code: output=8dd460d2a9186f04 input=2e8930e897ace086]*/
{
    /* create a range object for pickling.  Must calculate the "stop" value */
    PyValueBorrow start = PyValue_FromObjectBorrow(r->start);
    PyValueBorrow step = PyValue_FromObjectBorrow(r->step);
    PyValue product = _PyValue_Multiply(PyValue_FromObjectBorrow(r->len), step);
    if (PyValue_IsNull(product))
        return PyValue_NULL;
    PyValue stop = _PyValue_Add(start, product);
    PyValue_CLOSE(product);
    if (PyValue_IsNull(stop))
        return PyValue_NULL;
    PyValue start_owned = PyValue_DUP(start);
    PyValue step_owned = PyValue_DUP(step);
    PyObject *range = (PyObject*)make_range_object(&PyRange_Type,
                               start_owned, stop, step_owned);
    if (range == NULL) {
        PyValue_CLOSE(start_owned);
        PyValue_CLOSE(stop);
        PyValue_CLOSE(step_owned);
        return PyValue_NULL;
    }

    /* return the result */
    return _PyValue_FromObjectResult(Py_BuildValue("N(N)O", _PyEval_GetBuiltin(&_Py_ID(iter)),
                         range, Py_None));
}

/*[clinic input]
@critical_section
longrange_iterator.__setstate__
    self as r: self(type="longrangeiterobject *")
    state: object
    /

Set state information for unpickling.
[clinic start generated code]*/

static PyValue
longrange_iterator___setstate___impl(longrangeiterobject *r,
                                     PyValueBorrow state)
/*[clinic end generated code: output=b5d6e0b24a6e9982 input=8b116de3018de824]*/
{
    if (PyValue_TYPE(state) != &PyLong_Type) {
        PyErr_Format(PyExc_TypeError, "state must be an int, not %vT", state);
        return PyValue_NULL;
    }

    /* The clip and the arithmetic work on objects, so box an inline state once. */
    PyObject *index = _PyValue_AsObjectAt(state, BOX_SITE_PARSER);
    if (index == NULL)
        return PyValue_NULL;
    PyObject *zero = _PyLong_GetZero();  // borrowed reference
    int cmp;

    /* clip the value */
    cmp = _PyValue_RichCompareBool(PyValue_FromObjectBorrow(index), PyValue_FromObjectBorrow(zero), Py_LT);
    if (cmp < 0) {
        Py_DECREF(index);
        return PyValue_NULL;
    }
    if (cmp > 0) {
        Py_SETREF(index, Py_NewRef(zero));
    }
    else {
        cmp = _PyValue_RichCompareBool(PyValue_FromObjectBorrow(r->len), PyValue_FromObjectBorrow(index), Py_LT);
        if (cmp < 0) {
            Py_DECREF(index);
            return PyValue_NULL;
        }
        if (cmp > 0)
            Py_SETREF(index, Py_NewRef(r->len));
    }
    PyValue product_v = _PyValue_Multiply(PyValue_FromObjectBorrow(index), PyValue_FromObjectBorrow(r->step));
    PyObject *product = PyValue_IsNull(product_v) ? NULL
        : _PyValue_AsObjectStealAt(product_v, BOX_SITE_NUMBER);
    if (product == NULL) {
        Py_DECREF(index);
        return PyValue_NULL;
    }
    PyValue new_start_v = _PyValue_Add(PyValue_FromObjectBorrow(r->start), PyValue_FromObjectBorrow(product));
    PyObject *new_start = PyValue_IsNull(new_start_v) ? NULL
        : _PyValue_AsObjectStealAt(new_start_v, BOX_SITE_NUMBER);
    Py_DECREF(product);
    if (new_start == NULL) {
        Py_DECREF(index);
        return PyValue_NULL;
    }
    PyValue new_len_v = _PyValue_Subtract(PyValue_FromObjectBorrow(r->len), PyValue_FromObjectBorrow(index));
    PyObject *new_len = PyValue_IsNull(new_len_v) ? NULL
        : _PyValue_AsObjectStealAt(new_len_v, BOX_SITE_NUMBER);
    Py_DECREF(index);
    if (new_len == NULL) {
        Py_DECREF(new_start);
        return PyValue_NULL;
    }
    PyObject *tmp = r->start;
    r->start = new_start;
    Py_SETREF(r->len, new_len);
    Py_DECREF(tmp);
    return PyValue_None;
}

static PyMethodDef longrangeiter_methods[] = {
    LONGRANGE_ITERATOR___LENGTH_HINT___METHODDEF
    LONGRANGE_ITERATOR___REDUCE___METHODDEF
    LONGRANGE_ITERATOR___SETSTATE___METHODDEF
    {NULL,              NULL}           /* sentinel */
};

static void
longrangeiter_dealloc(PyObject *op)
{
    longrangeiterobject *r = (longrangeiterobject*)op;
    Py_XDECREF(r->start);
    Py_XDECREF(r->step);
    Py_XDECREF(r->len);
    PyObject_Free(r);
}

/* One step under the iterator's lock: the next int, or NULL with or without
   an exception set. */
static PyObject *
longrangeiter_step(longrangeiterobject *r)
{
    if (_PyValue_RichCompareBool(PyValue_FromObjectBorrow(r->len), PyValue_FromObjectBorrow(_PyLong_GetZero()), Py_GT) != 1)
        return NULL;

    PyValue new_start_v = _PyValue_Add(PyValue_FromObjectBorrow(r->start), PyValue_FromObjectBorrow(r->step));
    PyObject *new_start = PyValue_IsNull(new_start_v) ? NULL
        : _PyValue_AsObjectStealAt(new_start_v, BOX_SITE_NUMBER);
    if (new_start == NULL) {
        return NULL;
    }
    PyValue new_len_v = _PyValue_Subtract(PyValue_FromObjectBorrow(r->len), PyValue_FromObjectBorrow(_PyLong_GetOne()));
    PyObject *new_len = PyValue_IsNull(new_len_v) ? NULL
        : _PyValue_AsObjectStealAt(new_len_v, BOX_SITE_NUMBER);
    if (new_len == NULL) {
        Py_DECREF(new_start);
        return NULL;
    }
    PyObject *result = r->start;
    r->start = new_start;
    Py_SETREF(r->len, new_len);
    return result;
}

static _PyStatusValue
longrangeiter_next_value(PyObject *self)
{
    PyObject *result;
    Py_BEGIN_CRITICAL_SECTION(self);
    result = longrangeiter_step((longrangeiterobject *)self);
    Py_END_CRITICAL_SECTION();
    if (result != NULL) {
        return (_PyStatusValue){PYGEN_NEXT, PyValue_FromObjectSteal(result)};
    }
    return (_PyStatusValue){PyErr_Occurred() ? PYGEN_ERROR : PYGEN_RETURN,
                            PyValue_NULL};
}

_Py_VALUE_THUNK_ITERNEXT(longrangeiter_next, longrangeiter_next_value)

static int
longrangeiter_traverse(PyObject *op, visitproc visit, void *arg)
{
    longrangeiterobject *r = (longrangeiterobject *)op;
    Py_VISIT(r->start);
    Py_VISIT(r->step);
    Py_VISIT(r->len);
    return 0;
}

PyTypeObject PyLongRangeIter_Type = {
        PyVarObject_HEAD_INIT(&PyType_Type, 0)
        "longrange_iterator",                   /* tp_name */
        sizeof(longrangeiterobject),            /* tp_basicsize */
        0,                                      /* tp_itemsize */
        /* methods */
        longrangeiter_dealloc,                  /* tp_dealloc */
        0,                                      /* tp_vectorcall_offset */
        0,                                      /* tp_getattr */
        0,                                      /* tp_setattr */
        0,                                      /* tp_as_async */
        0,                                      /* tp_repr */
        0,                                      /* tp_as_number */
        0,                                      /* tp_as_sequence */
        0,                                      /* tp_as_mapping */
        0,                                      /* tp_hash */
        0,                                      /* tp_call */
        0,                                      /* tp_str */
        PyObject_GenericGetAttr,                /* tp_getattro */
        0,                                      /* tp_setattro */
        0,                                      /* tp_as_buffer */
        Py_TPFLAGS_DEFAULT,                     /* tp_flags */
        0,                                      /* tp_doc */
        longrangeiter_traverse,                 /* tp_traverse */
        0,                                      /* tp_clear */
        0,                                      /* tp_richcompare */
        0,                                      /* tp_weaklistoffset */
        PyObject_SelfIter,                      /* tp_iter */
        longrangeiter_next,                     /* tp_iternext */
        longrangeiter_methods,                  /* tp_methods */
        0,
        .tp_value = {
            .iternext = _Py_VALUE_SLOT_INIT(iternext, longrangeiter_next_value),
        },
};

static PyObject *
range_iter(PyObject *seq)
{
    rangeobject *r = (rangeobject *)seq;
    longrangeiterobject *it;
    long lstart, lstop, lstep;
    unsigned long ulen;
    int overflow;

    assert(PyRange_Check(seq));

    /* If all three fields and the length convert to long, use the int
     * version */
    lstart = range_long(r->start, &overflow);
    if (overflow) {
        goto long_range;
    }
    lstop = range_long(r->stop, &overflow);
    if (overflow) {
        goto long_range;
    }
    lstep = range_long(r->step, &overflow);
    if (overflow) {
        goto long_range;
    }
    ulen = get_len_of_range(lstart, lstop, lstep);
    if (ulen > (unsigned long)LONG_MAX) {
        goto long_range;
    }
    /* check for potential overflow of lstart + ulen * lstep */
    if (ulen) {
        if (lstep > 0) {
            if (lstop > LONG_MAX - (lstep - 1))
                goto long_range;
        }
        else {
            if (lstop < LONG_MIN + (-1 - lstep))
                goto long_range;
        }
    }
    return fast_range_iter(lstart, lstop, lstep, (long)ulen);

  long_range:
    it = PyObject_New(longrangeiterobject, &PyLongRangeIter_Type);
    if (it == NULL)
        return NULL;

    /* The iterator holds objects: this path is for values beyond a C long. */
    it->start = _PyValue_AsObjectAt(r->start, BOX_SITE_FIELD);
    it->step = _PyValue_AsObjectAt(r->step, BOX_SITE_FIELD);
    it->len = _PyValue_AsObjectAt(r->length, BOX_SITE_FIELD);
    if (it->start == NULL || it->step == NULL || it->len == NULL) {
        Py_DECREF(it);
        return NULL;
    }
    return (PyObject *)it;
}

static PyObject *
range_reverse(PyObject *seq, PyObject *Py_UNUSED(ignored))
{
    rangeobject *range = (rangeobject*) seq;
    longrangeiterobject *it;
    long lstart, lstop, lstep, new_start, new_stop;
    unsigned long ulen;
    int overflow;

    assert(PyRange_Check(seq));

    /* reversed(range(start, stop, step)) can be expressed as
       range(start+(n-1)*step, start-step, -step), where n is the number of
       integers in the range.

       If each of start, stop, step, -step, start-step, and the length
       of the iterator is representable as a C long, use the int
       version.  This excludes some cases where the reversed range is
       representable as a range_iterator, but it's good enough for
       common cases and it makes the checks simple. */

    lstart = range_long(range->start, &overflow);
    if (overflow) {
        goto long_range;
    }
    lstop = range_long(range->stop, &overflow);
    if (overflow) {
        goto long_range;
    }
    lstep = range_long(range->step, &overflow);
    if (overflow) {
        goto long_range;
    }
    /* check for possible overflow of -lstep */
    if (lstep == LONG_MIN)
        goto long_range;

    /* check for overflow of lstart - lstep:

       for lstep > 0, need only check whether lstart - lstep < LONG_MIN.
       for lstep < 0, need only check whether lstart - lstep > LONG_MAX

       Rearrange these inequalities as:

           lstart - LONG_MIN < lstep  (lstep > 0)
           LONG_MAX - lstart < -lstep  (lstep < 0)

       and compute both sides as unsigned longs, to avoid the
       possibility of undefined behaviour due to signed overflow. */

    if (lstep > 0) {
         if ((unsigned long)lstart - LONG_MIN < (unsigned long)lstep)
            goto long_range;
    }
    else {
        if (LONG_MAX - (unsigned long)lstart < 0UL - lstep)
            goto long_range;
    }

    ulen = get_len_of_range(lstart, lstop, lstep);
    if (ulen > (unsigned long)LONG_MAX)
        goto long_range;

    new_stop = lstart - lstep;
    new_start = (long)(new_stop + ulen * lstep);
    return fast_range_iter(new_start, new_stop, -lstep, (long)ulen);

long_range:
    it = PyObject_New(longrangeiterobject, &PyLongRangeIter_Type);
    if (it == NULL)
        return NULL;
    it->start = it->step = it->len = NULL;

    /* start + (len - 1) * step, in words; the iterator holds objects. */
    PyValue one = PyValue_FromInt64(1);
    PyValue diff = _PyValue_Subtract(range->length, one);
    if (PyValue_IsNull(diff))
        goto create_failure;
    PyValue product = _PyValue_Multiply(diff, range->step);
    PyValue_CLOSE(diff);
    if (PyValue_IsNull(product))
        goto create_failure;
    PyValue sum = _PyValue_Add(range->start, product);
    PyValue_CLOSE(product);
    if (PyValue_IsNull(sum))
        goto create_failure;
    it->start = _PyValue_AsObjectStealAt(sum, BOX_SITE_FIELD);
    if (!it->start)
        goto create_failure;

    PyValue nstep = _PyValue_Negative(range->step);
    if (PyValue_IsNull(nstep))
        goto create_failure;
    it->step = _PyValue_AsObjectStealAt(nstep, BOX_SITE_FIELD);
    it->len = _PyValue_AsObjectAt(range->length, BOX_SITE_FIELD);
    if (it->step == NULL || it->len == NULL)
        goto create_failure;

    return (PyObject *)it;

create_failure:
    Py_DECREF(it);
    return NULL;
}
