/* Tuple object implementation */

#include "Python.h"
#include "pycore_abstract.h"      // _PyIndex_Check()
#include "pycore_ceval.h"         // _PyEval_GetBuiltin()
#include "pycore_gc.h"            // _PyObject_GC_IS_TRACKED()
#include "pycore_gc_walk.h"       // Py_GC_PRECISE
#include "pycore_list.h"          // _Py_memory_repeat()
#include "pycore_modsupport.h"    // _PyArg_NoKwnames()
#include "pycore_object.h"        // _PyObject_GC_TRACK()
#include "pycore_value.h"         // PyValue_ObjectBorrow()
#include "pycore_tuple.h"         // _PyTupleIterObject


/*[clinic input]
class tuple "PyTupleObject *" "&PyTuple_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=f051ba3cfdf9a189]*/

#include "clinic/tupleobject.c.h"


/* Allocate an uninitialized tuple object. Before making it public, following
   steps must be done:

   - Initialize its items.
   - Call _PyObject_GC_TRACK() on it.

   Because the empty tuple is always reused and it's already tracked by GC,
   this function must not be called with size == 0 (unless from PyTuple_New()
   which wraps this function).
*/
Py_GC_PRECISE static PyTupleObject *
tuple_alloc(Py_ssize_t size)
{
    if (size < 0) {
        PyErr_BadInternalCall();
        return NULL;
    }
    assert(size != 0);    // The empty tuple is statically allocated.
    /* Check for overflow */
    if ((size_t)size > ((size_t)PY_SSIZE_T_MAX - (sizeof(PyTupleObject) -
                sizeof(PyValue))) / sizeof(PyValue)) {
        return (PyTupleObject *)PyErr_NoMemory();
    }
    PyTupleObject *result = PyObject_GC_NewVar(PyTupleObject, &PyTuple_Type, size);
    if (result != NULL) {
        _PyTuple_RESET_HASH_CACHE(result);
        _PyGC_YoungAllocated((PyObject *)result);
    }
    return result;
}

// The empty tuple singleton is not tracked by the GC.
// It does not contain any Python object.
// Note that tuple subclasses have their own empty instances.

static inline PyObject *
tuple_get_empty(void)
{
    return (PyObject *)&_Py_SINGLETON(tuple_empty);
}

Py_GC_PRECISE PyObject *
PyTuple_New(Py_ssize_t size)
{
    PyTupleObject *op;
    if (size == 0) {
        return tuple_get_empty();
    }
    op = tuple_alloc(size);
    if (op == NULL) {
        return NULL;
    }
    for (Py_ssize_t i = 0; i < size; i++) {
        op->ob_item[i] = PyValue_NULL;
    }
    _PyObject_GC_TRACK(op);
    return (PyObject *) op;
}

Py_ssize_t
PyTuple_Size(PyObject *op)
{
    if (!PyTuple_Check(op)) {
        PyErr_BadInternalCall();
        return -1;
    }
    else
        return Py_SIZE(op);
}

PyObject *
PyTuple_GetItem(PyObject *op, Py_ssize_t i)
{
    if (!PyTuple_Check(op)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    if (i < 0 || i >= Py_SIZE(op)) {
        PyErr_SetString(PyExc_IndexError, "tuple index out of range");
        return NULL;
    }
    return PyValue_Object(((PyTupleObject *)op)->ob_item[i]);
}

int
PyTuple_SetItem(PyObject *op, Py_ssize_t i, PyObject *newitem)
{
    // Uniqueness cannot be checked without counts; it is the caller's
    // contract, as for PyTuple_SET_ITEM().
    if (!PyTuple_Check(op)) {
        Py_XDECREF(newitem);
        PyErr_BadInternalCall();
        return -1;
    }
    if (i < 0 || i >= Py_SIZE(op)) {
        Py_XDECREF(newitem);
        PyErr_SetString(PyExc_IndexError,
                        "tuple assignment index out of range");
        return -1;
    }
    PyTuple_SET_ITEM(op, i, newitem);
    return 0;
}

void
_PyTuple_MaybeUntrack(PyObject *op)
{
    PyTupleObject *t;
    Py_ssize_t i, n;

    if (!PyTuple_CheckExact(op) || !_PyObject_GC_IS_TRACKED(op))
        return;
    t = (PyTupleObject *) op;
    n = Py_SIZE(t);
    for (i = 0; i < n; i++) {
        PyValue elt = t->ob_item[i];
        /* Tuple with NULL elements aren't
           fully constructed, don't untrack
           them yet. */
        if (PyValue_IsNull(elt) || _PyValue_MayBeTracked(elt))
            return;
    }
    _PyObject_GC_UNTRACK(op);
}

/* Fast, but conservative check if an object maybe tracked
   May return true for an object that is not tracked,
   Will always return true for an object that is tracked.
   This is a temporary workaround until _PyObject_GC_IS_TRACKED
   becomes fast and safe to call on non-GC objects.
*/
static bool
maybe_tracked(PyValue v)
{
    return PyValue_IsObject(v) && _PyType_IS_GC(Py_TYPE(PyValue_ObjectBorrow(v)));
}

PyObject *
PyTuple_Pack(Py_ssize_t n, ...)
{
    Py_ssize_t i;
    PyObject *o;
    PyValue *items;
    va_list vargs;
    bool track = false;

    if (n == 0) {
        return tuple_get_empty();
    }

    va_start(vargs, n);
    PyTupleObject *result = tuple_alloc(n);
    if (result == NULL) {
        va_end(vargs);
        return NULL;
    }
    items = result->ob_item;
    for (i = 0; i < n; i++) {
        o = va_arg(vargs, PyObject *);
        items[i] = PyValue_FromObject(o);
        if (!track && maybe_tracked(items[i])) {
            track = true;
        }
    }
    va_end(vargs);
    if (track) {
        _PyObject_GC_TRACK(result);
    }
    return (PyObject *)result;
}

PyObject *
_PyTuple_FromPair(PyObject *first, PyObject *second)
{
    assert(first != NULL);
    assert(second != NULL);

    return _PyTuple_FromPairSteal(Py_NewRef(first), Py_NewRef(second));
}

PyObject *
_PyTuple_FromPairSteal(PyObject *first, PyObject *second)
{
    assert(first != NULL);
    assert(second != NULL);

    PyTupleObject *op = tuple_alloc(2);
    if (op == NULL) {
        Py_DECREF(first);
        Py_DECREF(second);
        return NULL;
    }
    PyValue *items = op->ob_item;
    items[0] = PyValue_FromObject(first);
    items[1] = PyValue_FromObject(second);
    if (maybe_tracked(items[0]) || maybe_tracked(items[1])) {
        _PyObject_GC_TRACK(op);
    }
    return (PyObject *)op;
}

/* Methods */

/*
 Free of a tuple where all contents have been stolen and
 is now untracked by GC. This operation is thus non-escaping.
 */
void
_PyStolenTuple_Free(PyObject *obj)
{
    assert(PyTuple_CheckExact(obj));
    PyTupleObject *op = _PyTuple_CAST(obj);
    assert(Py_SIZE(op) != 0);
    assert(!_PyObject_GC_IS_TRACKED(obj));
    PyTuple_Type.tp_free((PyObject *)op);
}

static void
tuple_dealloc(PyObject *self)
{
    PyTupleObject *op = _PyTuple_CAST(self);
    if (Py_SIZE(op) == 0) {
        /* The empty tuple is statically allocated. */
        if (op == &_Py_SINGLETON(tuple_empty)) {
#ifdef Py_DEBUG
            _Py_FatalRefcountError("deallocating the empty tuple singleton");
#else
            return;
#endif
        }
#ifdef Py_DEBUG
        /* tuple subclasses have their own empty instances. */
        assert(!PyTuple_CheckExact(op));
#endif
    }

    PyObject_GC_UnTrack(op);

    Py_TYPE(op)->tp_free((PyObject *)op);
}

static PyObject *
tuple_repr(PyObject *self)
{
    PyTupleObject *v = _PyTuple_CAST(self);
    Py_ssize_t n = PyTuple_GET_SIZE(v);
    if (n == 0) {
        return PyUnicode_FromString("()");
    }

    /* While not mutable, it is still possible to end up with a cycle in a
       tuple through an object that stores itself within a tuple (and thus
       infinitely asks for the repr of itself). This should only be
       possible within a type. */
    int res = Py_ReprEnter((PyObject *)v);
    if (res != 0) {
        return res > 0 ? PyUnicode_FromString("(...)") : NULL;
    }

    Py_ssize_t prealloc;
    if (n > 1) {
        // "(" + "1" + ", 2" * (len - 1) + ")"
        prealloc = 1 + 1 + (2 + 1) * (n - 1) + 1;
    }
    else {
        // "(1,)"
        prealloc = 4;
    }
    PyUnicodeWriter *writer = PyUnicodeWriter_Create(prealloc);
    if (writer == NULL) {
        goto error;
    }

    if (PyUnicodeWriter_WriteChar(writer, '(') < 0) {
        goto error;
    }

    /* Do repr() on each element. */
    for (Py_ssize_t i = 0; i < n; ++i) {
        if (i > 0) {
            if (PyUnicodeWriter_WriteChar(writer, ',') < 0) {
                goto error;
            }
            if (PyUnicodeWriter_WriteChar(writer, ' ') < 0) {
                goto error;
            }
        }

        // An uninitialized item is NULL, written as <NULL>.
        PyObject *item = PyValue_Object(v->ob_item[i]);
        if ((item == NULL && !PyValue_IsNull(v->ob_item[i]))
            || PyUnicodeWriter_WriteRepr(writer, item) < 0) {
            goto error;
        }
    }

    if (n == 1) {
        if (PyUnicodeWriter_WriteChar(writer, ',') < 0) {
            goto error;
        }
    }
    if (PyUnicodeWriter_WriteChar(writer, ')') < 0) {
        goto error;
    }

    Py_ReprLeave((PyObject *)v);
    return PyUnicodeWriter_Finish(writer);

error:
    PyUnicodeWriter_Discard(writer);
    Py_ReprLeave((PyObject *)v);
    return NULL;
}


/* Hash for tuples. This is a slightly simplified version of the xxHash
   non-cryptographic hash:
   - we do not use any parallelism, there is only 1 accumulator.
   - we drop the final mixing since this is just a permutation of the
     output space: it does not help against collisions.
   - at the end, we mangle the length with a single constant.
   For the xxHash specification, see
   https://github.com/Cyan4973/xxHash/blob/master/doc/xxhash_spec.md

   The constants for the hash function are defined in pycore_tuple.h.

   If you update this code, update also frozendict_pair_hash() which copied
   this code.
*/

static Py_hash_t
tuple_hash(PyObject *op)
{
    PyTupleObject *v = _PyTuple_CAST(op);

    Py_uhash_t acc = FT_ATOMIC_LOAD_SSIZE_RELAXED(v->ob_hash);
    if (acc != (Py_uhash_t)-1) {
        return acc;
    }

    Py_ssize_t len = Py_SIZE(v);
    PyValue *item = v->ob_item;

    /* Hashing an item recurses back into tuple_hash() when that item is
       itself a tuple, so guard the descent to raise RecursionError rather
       than overflow the C stack on a deeply nested tuple. */
    if (_Py_EnterRecursiveCall(" while hashing a tuple")) {
        return -1;
    }

    acc = _PyTuple_HASH_XXPRIME_5;
    for (Py_ssize_t i = 0; i < len; i++) {
        Py_uhash_t lane = PyObject_HashValue(item[i]);
        if (lane == (Py_uhash_t)-1) {
            _Py_LeaveRecursiveCall();
            return -1;
        }
        acc += lane * _PyTuple_HASH_XXPRIME_2;
        acc = _PyTuple_HASH_XXROTATE(acc);
        acc *= _PyTuple_HASH_XXPRIME_1;
    }
    _Py_LeaveRecursiveCall();

    /* Add input length, mangled to keep the historical value of hash(()). */
    acc += len ^ (_PyTuple_HASH_XXPRIME_5 ^ 3527539UL);

    if (acc == (Py_uhash_t)-1) {
        acc = 1546275796;
    }

    FT_ATOMIC_STORE_SSIZE_RELAXED(v->ob_hash, acc);

    return acc;
}

static Py_ssize_t
tuple_length(PyObject *self)
{
    PyTupleObject *a = _PyTuple_CAST(self);
    return Py_SIZE(a);
}

static int
tuple_contains(PyObject *self, PyObject *el)
{
    return _PyTuple_ContainsValue(self, PyValue_FromObject(el));
}

int
_PyTuple_ContainsValue(PyObject *self, PyValue v)
{
    PyTupleObject *a = _PyTuple_CAST(self);
    int cmp = 0;
    for (Py_ssize_t i = 0; cmp == 0 && i < Py_SIZE(a); ++i) {
        cmp = PyObject_RichCompareBoolValue(a->ob_item[i], v, Py_EQ);
    }
    return cmp;
}

static PyObject *
tuple_item(PyObject *op, Py_ssize_t i)
{
    PyTupleObject *a = _PyTuple_CAST(op);
    if (i < 0 || i >= Py_SIZE(a)) {
        PyErr_SetString(PyExc_IndexError, "tuple index out of range");
        return NULL;
    }
    return PyValue_Object(a->ob_item[i]);
}

PyObject *
PyTuple_FromArray(PyObject *const *src, Py_ssize_t n)
{
    if (n == 0) {
        return tuple_get_empty();
    }

    PyTupleObject *tuple = tuple_alloc(n);
    if (tuple == NULL) {
        return NULL;
    }
    PyValue *dst = tuple->ob_item;
    bool track = false;
    for (Py_ssize_t i = 0; i < n; i++) {
        dst[i] = PyValue_FromObject(src[i]);
        if (!track && maybe_tracked(dst[i])) {
            track = true;
        }
    }
    if (track) {
        _PyObject_GC_TRACK(tuple);
    }
    return (PyObject *)tuple;
}

PyObject *const *
_PyTuple_Objects(PyObject *op)
{
    PyTupleObject *t = _PyTuple_CAST(op);
    return (PyObject *const *)_PyValue_ObjectArray(t->ob_item, Py_SIZE(t));
}

Py_GC_PRECISE PyObject *
_PyTuple_FromValues(const PyValue *src, Py_ssize_t n)
{
    if (n == 0) {
        return tuple_get_empty();
    }
    PyTupleObject *tuple = tuple_alloc(n);
    if (tuple == NULL) {
        return NULL;
    }
    PyValue *dst = tuple->ob_item;
    bool track = false;
    for (Py_ssize_t i = 0; i < n; i++) {
        dst[i] = src[i];
        if (!track && maybe_tracked(src[i])) {
            track = true;
        }
    }
    if (track) {
        _PyObject_GC_TRACK(tuple);
    }
    return (PyObject *)tuple;
}

PyObject *
_PyTuple_FromArraySteal(PyObject *const *src, Py_ssize_t n)
{
    if (n == 0) {
        return tuple_get_empty();
    }
    PyTupleObject *tuple = tuple_alloc(n);
    if (tuple == NULL) {
        for (Py_ssize_t i = 0; i < n; i++) {
            Py_DECREF(src[i]);
        }
        return NULL;
    }
    _PyValue_FromObjectArray(tuple->ob_item, src, n);
    _PyObject_GC_TRACK(tuple);
    return (PyObject *)tuple;
}

static PyObject *
tuple_slice(PyTupleObject *a, Py_ssize_t ilow,
           Py_ssize_t ihigh)
{
    if (ilow < 0)
        ilow = 0;
    if (ihigh > Py_SIZE(a))
        ihigh = Py_SIZE(a);
    if (ihigh < ilow)
        ihigh = ilow;
    if (ilow == 0 && ihigh == Py_SIZE(a) && PyTuple_CheckExact(a)) {
        return Py_NewRef(a);
    }
    return _PyTuple_FromValues(a->ob_item + ilow, ihigh - ilow);
}

PyObject *
_PyTuple_BinarySlice(PyObject *container, PyObject *start, PyObject *stop)
{
    assert(PyTuple_CheckExact(container));
    Py_ssize_t len = Py_SIZE(container);
    Py_ssize_t istart, istop;
    if (!_PyEval_UnpackIndices(start, stop, len, &istart, &istop)) {
        return NULL;
    }
    if (istart == 0 && istop == len) {
        return Py_NewRef(container);
    }
    if (istop < istart) {
        istop = istart;
    }
    return _PyTuple_FromValues(((PyTupleObject *)container)->ob_item + istart,
                               istop - istart);
}

PyObject *
PyTuple_GetSlice(PyObject *op, Py_ssize_t i, Py_ssize_t j)
{
    if (op == NULL || !PyTuple_Check(op)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    return tuple_slice((PyTupleObject *)op, i, j);
}

PyObject *
_PyTuple_Concat(PyObject *aa, PyObject *bb)
{
    PyTupleObject *a = _PyTuple_CAST(aa);
    if (Py_SIZE(a) == 0 && PyTuple_CheckExact(bb)) {
        return Py_NewRef(bb);
    }
    if (!PyTuple_Check(bb)) {
        PyErr_Format(PyExc_TypeError,
             "can only concatenate tuple (not \"%.200s\") to tuple",
                 Py_TYPE(bb)->tp_name);
        return NULL;
    }
    PyTupleObject *b = (PyTupleObject *)bb;

    if (Py_SIZE(b) == 0 && PyTuple_CheckExact(a)) {
        return Py_NewRef(a);
    }
    assert((size_t)Py_SIZE(a) + (size_t)Py_SIZE(b) < PY_SSIZE_T_MAX);
    Py_ssize_t size = Py_SIZE(a) + Py_SIZE(b);
    if (size == 0) {
        return tuple_get_empty();
    }

    PyTupleObject *np = tuple_alloc(size);
    if (np == NULL) {
        return NULL;
    }

    memcpy(np->ob_item, a->ob_item, Py_SIZE(a) * sizeof(PyValue));
    memcpy(np->ob_item + Py_SIZE(a), b->ob_item, Py_SIZE(b) * sizeof(PyValue));

    _PyObject_GC_TRACK(np);
    return (PyObject *)np;
}

PyObject *
_PyTuple_Repeat(PyObject *self, Py_ssize_t n)
{
    PyTupleObject *a = _PyTuple_CAST(self);
    const Py_ssize_t input_size = Py_SIZE(a);
    if (input_size == 0 || n == 1) {
        if (PyTuple_CheckExact(a)) {
            /* Since tuples are immutable, we can return a shared
               copy in this case */
            return Py_NewRef(a);
        }
    }
    if (input_size == 0 || n <= 0) {
        return tuple_get_empty();
    }
    assert(n>0);

    if (input_size > PY_SSIZE_T_MAX / n)
        return PyErr_NoMemory();
    Py_ssize_t output_size = input_size * n;

    PyTupleObject *np = tuple_alloc(output_size);
    if (np == NULL)
        return NULL;

    PyValue *dest = np->ob_item;
    if (input_size == 1) {
        PyValue elem = a->ob_item[0];
        PyValue *dest_end = dest + output_size;
        while (dest < dest_end) {
            *dest++ = elem;
        }
    }
    else {
        memcpy(dest, a->ob_item, input_size * sizeof(PyValue));
        _Py_memory_repeat((char *)np->ob_item, sizeof(PyValue)*output_size,
                          sizeof(PyValue)*input_size);
    }
    _PyObject_GC_TRACK(np);
    return (PyObject *) np;
}

/*[clinic input]
tuple.index -> value

    value: value
    start: slice_index(accept={int}) = 0
    stop: slice_index(accept={int}, c_default="PY_SSIZE_T_MAX") = sys.maxsize
    /

Return first index of value.

Raises ValueError if the value is not present.
[clinic start generated code]*/

static PyValue
tuple_index_impl(PyTupleObject *self, PyValue value, Py_ssize_t start,
                 Py_ssize_t stop)
/*[clinic end generated code: output=648bb8dd6c38dda2 input=a8f75f15daf94751]*/
{
    Py_ssize_t i;

    if (start < 0) {
        start += Py_SIZE(self);
        if (start < 0)
            start = 0;
    }
    if (stop < 0) {
        stop += Py_SIZE(self);
    }
    else if (stop > Py_SIZE(self)) {
        stop = Py_SIZE(self);
    }
    for (i = start; i < stop; i++) {
        int cmp = PyObject_RichCompareBoolValue(self->ob_item[i], value, Py_EQ);
        if (cmp > 0)
            return PyValue_FromLongLong(i);
        else if (cmp < 0)
            return PyValue_NULL;
    }
    PyErr_SetString(PyExc_ValueError, "tuple.index(x): x not in tuple");
    return PyValue_NULL;
}

/*[clinic input]
tuple.count -> value

     value: value
     /

Return number of occurrences of value.
[clinic start generated code]*/

static PyValue
tuple_count_impl(PyTupleObject *self, PyValue value)
/*[clinic end generated code: output=c95da72a6cf07e00 input=961d6feec0a6264b]*/
{
    Py_ssize_t count = 0;
    Py_ssize_t i;

    for (i = 0; i < Py_SIZE(self); i++) {
        int cmp = PyObject_RichCompareBoolValue(self->ob_item[i], value, Py_EQ);
        if (cmp > 0)
            count++;
        else if (cmp < 0)
            return PyValue_NULL;
    }
    return PyValue_FromLongLong(count);
}

static int
tuple_traverse(PyObject *self, visitproc visit, void *arg)
{
    PyTupleObject *o = _PyTuple_CAST(self);
    _PyGCValueRange values = _PyTuple_GCValues(o);
    for (Py_ssize_t i = values.n; --i >= 0; ) {
        _Py_VISIT_VALUE(values.base[i]);
    }
    return 0;
}

static PyObject *
tuple_richcompare(PyObject *v, PyObject *w, int op)
{
    PyTupleObject *vt, *wt;
    Py_ssize_t i;
    Py_ssize_t vlen, wlen;

    if (!PyTuple_Check(v) || !PyTuple_Check(w))
        Py_RETURN_NOTIMPLEMENTED;

    vt = (PyTupleObject *)v;
    wt = (PyTupleObject *)w;

    vlen = Py_SIZE(vt);
    wlen = Py_SIZE(wt);

    /* Note:  the corresponding code for lists has an "early out" test
     * here when op is EQ or NE and the lengths differ.  That pays there,
     * but Tim was unable to find any real code where EQ/NE tuple
     * compares don't have the same length, so testing for it here would
     * have cost without benefit.
     */

    /* Search for the first index where items are different.
     * Note that because tuples are immutable, it's safe to reuse
     * vlen and wlen across the comparison calls.
     */
    for (i = 0; i < vlen && i < wlen; i++) {
        int k = PyObject_RichCompareBoolValue(vt->ob_item[i],
                                         wt->ob_item[i], Py_EQ);
        if (k < 0)
            return NULL;
        if (!k)
            break;
    }

    if (i >= vlen || i >= wlen) {
        /* No more items to compare -- compare sizes */
        Py_RETURN_RICHCOMPARE(vlen, wlen, op);
    }

    /* We have an item that differs -- shortcuts for EQ/NE */
    if (op == Py_EQ) {
        Py_RETURN_FALSE;
    }
    if (op == Py_NE) {
        Py_RETURN_TRUE;
    }

    /* Compare the final item again using the proper operator */
    return PyValue_Object(PyObject_RichCompareValue(vt->ob_item[i], wt->ob_item[i], op));
}

static PyObject *
tuple_subtype_new(PyTypeObject *type, PyObject *iterable);

/*[clinic input]
@vectorcall
@classmethod
tuple.__new__ as tuple_new
    iterable: object(c_default="NULL") = ()
    /

Built-in immutable sequence.

If no argument is given, the constructor returns an empty tuple.
If iterable is specified the tuple is initialized from iterable's items.

If the argument is a tuple, the return value is the same object.
[clinic start generated code]*/

static PyObject *
tuple_new_impl(PyTypeObject *type, PyObject *iterable)
/*[clinic end generated code: output=4546d9f0d469bce7 input=8fdda913493ebe48]*/
{
    if (type != &PyTuple_Type)
        return tuple_subtype_new(type, iterable);

    if (iterable == NULL) {
        return tuple_get_empty();
    }
    else {
        return PySequence_Tuple(iterable);
    }
}

static PyObject *
tuple_subtype_new(PyTypeObject *type, PyObject *iterable)
{
    PyObject *tmp, *newobj;
    Py_ssize_t i, n;

    assert(PyType_IsSubtype(type, &PyTuple_Type));
    // tuple subclasses must implement the GC protocol
    assert(_PyType_IS_GC(type));

    tmp = tuple_new_impl(&PyTuple_Type, iterable);
    if (tmp == NULL)
        return NULL;
    assert(PyTuple_Check(tmp));
    /* This may allocate an empty tuple that is not the global one. */
    newobj = type->tp_alloc(type, n = PyTuple_GET_SIZE(tmp));
    if (newobj == NULL) {
        Py_DECREF(tmp);
        return NULL;
    }
    for (i = 0; i < n; i++) {
        PyTuple_SET_VALUE(newobj, i, PyTuple_GET_VALUE(tmp, i));
    }
    Py_DECREF(tmp);

    _PyTuple_RESET_HASH_CACHE(newobj);

    // Don't track if a subclass tp_alloc is PyType_GenericAlloc()
    if (!_PyObject_GC_IS_TRACKED(newobj)) {
        _PyObject_GC_TRACK(newobj);
    }
    return newobj;
}

static PySequenceMethods tuple_as_sequence = {
    tuple_length,                               /* sq_length */
    _PyTuple_Concat,                            /* sq_concat */
    _PyTuple_Repeat,                            /* sq_repeat */
    tuple_item,                                 /* sq_item */
    0,                                          /* sq_slice */
    0,                                          /* sq_ass_item */
    0,                                          /* sq_ass_slice */
    tuple_contains,                             /* sq_contains */
};

static _PyValueIndexPair
tuple_iteritem(PyObject *obj, Py_ssize_t index)
{
    if (index >= PyTuple_GET_SIZE(obj)) {
        return (_PyValueIndexPair) { .value = PyValue_NULL, .index = index };
    }
    return (_PyValueIndexPair) { .value = PyTuple_GET_VALUE(obj, index),
                                 .index = index + 1 };
}

static PyObject*
tuple_subscript(PyObject *op, PyObject* item)
{
    PyTupleObject *self = _PyTuple_CAST(op);
    if (_PyIndex_Check(item)) {
        Py_ssize_t i = PyNumber_AsSsize_t(item, PyExc_IndexError);
        if (i == -1 && PyErr_Occurred())
            return NULL;
        if (i < 0)
            i += PyTuple_GET_SIZE(self);
        return tuple_item(op, i);
    }
    else if (PySlice_Check(item)) {
        Py_ssize_t start, stop, step, slicelength, i;
        size_t cur;
        PyValue *src, *dest;

        if (PySlice_Unpack(item, &start, &stop, &step) < 0) {
            return NULL;
        }
        slicelength = PySlice_AdjustIndices(PyTuple_GET_SIZE(self), &start,
                                            &stop, step);

        if (slicelength <= 0) {
            return tuple_get_empty();
        }
        else if (start == 0 && step == 1 &&
                 slicelength == PyTuple_GET_SIZE(self) &&
                 PyTuple_CheckExact(self)) {
            return Py_NewRef(self);
        }
        else {
            PyTupleObject* result = tuple_alloc(slicelength);
            if (!result) return NULL;

            src = self->ob_item;
            dest = result->ob_item;
            for (cur = start, i = 0; i < slicelength;
                 cur += step, i++) {
                dest[i] = src[cur];
            }

            _PyObject_GC_TRACK(result);
            return (PyObject *)result;
        }
    }
    else {
        PyErr_Format(PyExc_TypeError,
                     "tuple indices must be integers or slices, not %.200s",
                     Py_TYPE(item)->tp_name);
        return NULL;
    }
}

/*[clinic input]
tuple.__getnewargs__
[clinic start generated code]*/

static PyObject *
tuple___getnewargs___impl(PyTupleObject *self)
/*[clinic end generated code: output=25e06e3ee56027e2 input=1aeb4b286a21639a]*/
{
    return Py_BuildValue("(N)", tuple_slice(self, 0, Py_SIZE(self)));
}


PyDoc_STRVAR(tuple_class_getitem_doc,
"Tuples are generic over the types of their contents.\n\n\
For example, use ``tuple[int, str]`` for a pair whose first element\n\
is an int and second element is a string.\n\n\
Tuples also support the form ``tuple[T, ...]`` to indicate\n\
an arbitrary length tuple of elements of type T.");

static PyMethodDef tuple_methods[] = {
    TUPLE___GETNEWARGS___METHODDEF
    TUPLE_INDEX_METHODDEF
    TUPLE_COUNT_METHODDEF
    {"__class_getitem__", _PyCFunction_CAST(Py_GenericAliasValue), METH_O|METH_CLASS, tuple_class_getitem_doc},
    {NULL,              NULL}           /* sentinel */
};

static PyMappingMethods tuple_as_mapping = {
    tuple_length,
    tuple_subscript,
    0
};

static PyObject *tuple_iter(PyObject *seq);

PyTypeObject PyTuple_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "tuple",
    sizeof(PyTupleObject) - sizeof(PyValue),
    sizeof(PyValue),
    tuple_dealloc,                              /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    tuple_repr,                                 /* tp_repr */
    0,                                          /* tp_as_number */
    &tuple_as_sequence,                         /* tp_as_sequence */
    &tuple_as_mapping,                          /* tp_as_mapping */
    tuple_hash,                                 /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
        Py_TPFLAGS_BASETYPE | Py_TPFLAGS_TUPLE_SUBCLASS |
        _Py_TPFLAGS_MATCH_SELF | Py_TPFLAGS_SEQUENCE,  /* tp_flags */
    tuple_new__doc__,                           /* tp_doc */
    tuple_traverse,                             /* tp_traverse */
    0,                                          /* tp_clear */
    tuple_richcompare,                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    tuple_iter,                                 /* tp_iter */
    0,                                          /* tp_iternext */
    tuple_methods,                              /* tp_methods */
    0,                                          /* tp_members */
    0,                                          /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    0,                                          /* tp_init */
    0,                                          /* tp_alloc */
    tuple_new,                                  /* tp_new */
    PyObject_GC_Del,                            /* tp_free */
    .tp_vectorcall = tuple_vectorcall,
    .tp_version_tag = _Py_TYPE_VERSION_TUPLE,
    ._tp_iteritem = tuple_iteritem,
};

/* The following function breaks the notion that tuples are immutable:
   it changes the size of a tuple.  We get away with this only if there
   is only one module referencing the object.  You can also think of it
   as creating a new tuple object and destroying the old one, only more
   efficiently.  In any case, don't use this if the tuple may already be
   known to some other part of the code. */

int
_PyTuple_Resize(PyObject **pv, Py_ssize_t newsize)
{
    PyTupleObject *v;
    PyTupleObject *sv;
    Py_ssize_t i;
    Py_ssize_t oldsize;

    v = (PyTupleObject *) *pv;
    // As in PyTuple_SetItem(), uniqueness is the caller's contract.
    if (v == NULL || !Py_IS_TYPE(v, &PyTuple_Type) || newsize < 0) {
        *pv = 0;
        Py_XDECREF(v);
        PyErr_BadInternalCall();
        return -1;
    }

    oldsize = Py_SIZE(v);
    if (oldsize == newsize) {
        return 0;
    }
    if (newsize == 0) {
        Py_DECREF(v);
        *pv = tuple_get_empty();
        return 0;
    }
    if (oldsize == 0) {
#ifdef Py_DEBUG
        assert(v == &_Py_SINGLETON(tuple_empty));
#endif
        /* The empty tuple is statically allocated so we never
           resize it in-place. */
        Py_DECREF(v);
        *pv = PyTuple_New(newsize);
        return *pv == NULL ? -1 : 0;
    }
    /* Check for overflow */
    if ((size_t)newsize > ((size_t)PY_SSIZE_T_MAX - (sizeof(PyTupleObject) -
                sizeof(PyValue))) / sizeof(PyValue)) {
        *pv = NULL;
        Py_DECREF(v);
        PyErr_NoMemory();
        return -1;
    }

    if (_PyObject_GC_IS_TRACKED(v)) {
        _PyObject_GC_UNTRACK(v);
    }
    /* DECREF items deleted by shrinkage */
    for (i = newsize; i < oldsize; i++) {
        v->ob_item[i] = PyValue_NULL;
    }
    _PyReftracerTrack((PyObject *)v, PyRefTracer_DESTROY);
    sv = PyObject_GC_Resize(PyTupleObject, v, newsize);
    if (sv == NULL) {
        *pv = NULL;
        PyObject_GC_Del(v);
        return -1;
    }
    _Py_NewReferenceNoTotal((PyObject *) sv);
    /* Zero out items added by growing */
    if (newsize > oldsize)
        memset(&sv->ob_item[oldsize], 0,
               sizeof(*sv->ob_item) * (newsize - oldsize));
    *pv = (PyObject *) sv;
    _PyObject_GC_TRACK(sv);
    return 0;
}

/*********************** Tuple Iterator **************************/

#define _PyTupleIterObject_CAST(op) ((_PyTupleIterObject *)(op))

static void
tupleiter_dealloc(PyObject *self)
{
    _PyTupleIterObject *it = _PyTupleIterObject_CAST(self);
    _PyObject_GC_UNTRACK(it);
    Py_XDECREF(it->it_seq);
    assert(Py_IS_TYPE(self, &PyTupleIter_Type));
    PyObject_GC_Del(it);
}

static int
tupleiter_traverse(PyObject *self, visitproc visit, void *arg)
{
    _PyTupleIterObject *it = _PyTupleIterObject_CAST(self);
    Py_VISIT(it->it_seq);
    return 0;
}

static PyValue
tupleiter_next(PyObject *self)
{
    _PyTupleIterObject *it = _PyTupleIterObject_CAST(self);
    PyTupleObject *seq = it->it_seq;
    if (seq == NULL) {
        return PyValue_NULL;
    }
    assert(PyTuple_Check(seq));

    Py_ssize_t index = FT_ATOMIC_LOAD_SSIZE_RELAXED(it->it_index);
    if (index < PyTuple_GET_SIZE(seq)) {
        FT_ATOMIC_STORE_SSIZE_RELAXED(it->it_index, index + 1);
        return PyTuple_GET_VALUE(seq, index);
    }

    it->it_seq = NULL;
    return PyValue_NULL;
}

static PyObject *
tupleiter_len(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    _PyTupleIterObject *it = _PyTupleIterObject_CAST(self);
    Py_ssize_t len = 0;
    if (it->it_seq)
        len = PyTuple_GET_SIZE(it->it_seq) - it->it_index;
    return PyLong_FromSsize_t(len);
}

PyDoc_STRVAR(length_hint_doc, "Private method returning an estimate of len(list(it)).");

static PyObject *
tupleiter_reduce(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *iter = _PyEval_GetBuiltin(&_Py_ID(iter));

    /* _PyEval_GetBuiltin can invoke arbitrary code,
     * call must be before access of iterator pointers.
     * see issue #101765 */
    _PyTupleIterObject *it = _PyTupleIterObject_CAST(self);

    if (it->it_seq)
        return Py_BuildValue("N(O)n", iter, it->it_seq, it->it_index);
    return Py_BuildValue("N(())", iter);
}

static PyValue
tupleiter_setstate(PyObject *self, PyValue state_value)
{
    _PyTupleIterObject *it = _PyTupleIterObject_CAST(self);
    Py_ssize_t index = PyLong_AsSsize_tValue(state_value);
    if (index == -1 && PyErr_Occurred())
        return PyValue_NULL;
    if (it->it_seq != NULL) {
        if (index < 0)
            index = 0;
        else if (index > PyTuple_GET_SIZE(it->it_seq))
            index = PyTuple_GET_SIZE(it->it_seq); /* exhausted iterator */
        FT_ATOMIC_STORE_SSIZE_RELAXED(it->it_index, index);
    }
    return PyValue_FromObject(Py_None);
}

PyDoc_STRVAR(reduce_doc, "Return state information for pickling.");
PyDoc_STRVAR(setstate_doc, "Set state information for unpickling.");

static PyMethodDef tupleiter_methods[] = {
    {"__length_hint__", tupleiter_len, METH_NOARGS, length_hint_doc},
    {"__reduce__", tupleiter_reduce, METH_NOARGS, reduce_doc},
    {"__setstate__", _PyCFunction_CAST(tupleiter_setstate), METH_O, setstate_doc},
    {NULL, NULL, 0, NULL} /* sentinel */
};

PyTypeObject PyTupleIter_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "tuple_iterator",                           /* tp_name */
    sizeof(_PyTupleIterObject),                 /* tp_basicsize */
    0,                                          /* tp_itemsize */
    /* methods */
    tupleiter_dealloc,                          /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    0,                                          /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,/* tp_flags */
    0,                                          /* tp_doc */
    tupleiter_traverse,                         /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    PyObject_SelfIter,                          /* tp_iter */
    tupleiter_next,                             /* tp_iternext */
    tupleiter_methods,                          /* tp_methods */
    0,
};

static PyObject *
tuple_iter(PyObject *seq)
{
    if (!PyTuple_Check(seq)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    _PyTupleIterObject *it = PyObject_GC_New(_PyTupleIterObject, &PyTupleIter_Type);
    if (it == NULL)
        return NULL;
    it->it_index = 0;
    it->it_seq = (PyTupleObject *)Py_NewRef(seq);
    _PyObject_GC_TRACK(it);
    return (PyObject *)it;
}
