#include "Python.h"

#include "pycore_gc_trace.h"      // _PyGC_LiveCollectHere()
#include "pycore_gc_walk.h"       // Py_GC_PRECISE
#include "pycore_long.h"          // _PyLong_IsCompact()
#include "pycore_object.h"
#include "pycore_pyerrors.h"      // _Py_FatalErrorFormat()
#include "pycore_value.h"


// The low three bits of an object pointer must be clear (Include/pyvalue.h).
static_assert(_Alignof(PyObject) >= 8, "PyObject must be 8-byte aligned");

#ifdef Py_STATS
/* Count a box made for the caller at pc, in an open-addressed table; when
   the table is full, the last entry counts the rest. */
static void
count_box_site(uintptr_t pc)
{
    PyStats *stats = _PyStats_GET();
    if (stats == NULL) {
        return;
    }
    ObjectStats *os = &stats->object_stats;
    size_t i = (pc >> 2) % _PY_BOX_SITES;
    for (int n = 0; n < _PY_BOX_SITES; n++) {
        if (os->box_site_pc[i] == pc || os->box_site_count[i] == 0) {
            os->box_site_pc[i] = pc;
            os->box_site_count[i]++;
            return;
        }
        i = (i + 1) % _PY_BOX_SITES;
    }
    os->box_site_count[_PY_BOX_SITES - 1]++;
}
#endif

static PyObject *
box(PyValue v)
{
    if (PyValue_IsInlineInt(v)) {
        OBJECT_STAT_INC(inline_int_boxes);
        return PyLong_FromLong(PyValue_Int32(v));
    }
    OBJECT_STAT_INC(inline_float_boxes);
    return PyFloat_FromDouble(PyValue_Double(v));
}

/* The box again after a collection, when the first attempt ran out of
   memory: a box is garbage soon, and so is much else, so the collector gets
   its chance before the caller sees the MemoryError.  Out of line, so that
   the first attempt stays a plain call. */
Py_GC_PRECISE static Py_NO_INLINE PyObject *
box_after_failure(PyValue v)
{
    assert(PyValue_IsInline(v));
    PyThreadState *tstate = _PyThreadState_GET();
    if (!_PyErr_ExceptionMatches(tstate, PyExc_MemoryError)) {
        return NULL;
    }
    // The MemoryError is held across the collection and raised again if
    // the collection cannot run.
    _PyValueRoot exc;
    _PyThreadState_PushValueRoot(tstate, &exc);
    exc.object = _PyErr_GetRaisedException(tstate);
    int collected = _PyGC_LiveCollectHere(tstate);
    if (!collected) {
        _PyErr_SetRaisedException(tstate, exc.object);
    }
    _PyThreadState_PopValueRoot(tstate, &exc);
    if (!collected) {
        return NULL;
    }
    return box(v);
}

static inline PyObject *
box_collecting(PyValue v)
{
    assert(PyValue_IsInline(v));
    PyObject *op = box(v);
    if (op == NULL) {
        return box_after_failure(v);
    }
    return op;
}

// Out of line, so that the test before each call is all that hot paths carry.
Py_GC_PRECISE Py_NO_INLINE PyObject *
_PyValue_BoxInline(PyValue v)
{
    assert(PyValue_IsInline(v));
#ifdef Py_STATS
    count_box_site((uintptr_t)__builtin_return_address(0));
#endif
    return box_collecting(v);
}

/* For an accessor that returns an object and has no way to report an
   error, such as PyTuple_GET_ITEM(): its callers take NULL for a missing
   item or never check, so a box that cannot be made aborts rather than
   pass for one. */
Py_GC_PRECISE Py_NO_INLINE PyObject *
_PyValue_BoxOrAbort(PyValue v, const char *accessor)
{
    assert(PyValue_IsInline(v));
#ifdef Py_STATS
    count_box_site((uintptr_t)__builtin_return_address(0));
#endif
    PyObject *op = box_collecting(v);
    if (op == NULL) {
        _Py_FatalErrorFormat(accessor,
                             "out of memory for the object of an inline %s",
                             PyValue_IsInlineInt(v) ? "int" : "float");
    }
    return op;
}

PyValue
_PyValue_FromLongObject(PyObject *op)
{
    assert(PyLong_CheckExact(op) && !_PyLong_IsCompact((PyLongObject *)op));
    // An exact int does not raise here.
    int overflow;
    long value = PyLong_AsLongAndOverflow(op, &overflow);
    if (!overflow && INT32_MIN <= value && value <= INT32_MAX) {
        return PyValue_FromInt32((int32_t)value);
    }
    return _PyValue_FromBits((uintptr_t)op);
}


PyValue
_PyValue_BoxInt64(int64_t value)
{
    PyObject *op = PyLong_FromLongLong(value);
    if (op == NULL) {
        return PyValue_NULL;
    }
    return PyValue_FromObject(op);
}

PyValue
_PyValue_BoxUInt64(uint64_t value)
{
    PyObject *op = PyLong_FromUnsignedLongLong(value);
    if (op == NULL) {
        return PyValue_NULL;
    }
    return PyValue_FromObject(op);
}

PyValue
_PyValue_LshiftInt32(int32_t value, int32_t shift)
{
    assert(shift >= 0);
    PyObject *op = PyLong_FromLong(value);
    if (op == NULL) {
        return PyValue_NULL;
    }
    PyObject *res = _PyLong_Lshift(op, shift);
    Py_DECREF(op);
    if (res == NULL) {
        return PyValue_NULL;
    }
    return PyValue_FromObject(res);
}

PyValue
_PyValue_BoxDouble(double value)
{
    PyObject *op = PyFloat_FromDouble(value);
    if (op == NULL) {
        return PyValue_NULL;
    }
    return PyValue_FromObject(op);
}

/* An array of objects made for C code that takes PyObject *const *, from
   values some of which are inline (_PyValue_ObjectArray()).  It is a heap
   object so that nothing needs to free it: the caller holds a pointer into
   it, which keeps it alive while the caller runs, and it keeps its boxes
   alive. */
typedef struct {
    PyObject_VAR_HEAD
    PyObject *items[1];
} _PyObjectArrayObject;

static int
object_array_traverse(PyObject *self, visitproc visit, void *arg)
{
    _PyObjectArrayObject *a = (_PyObjectArrayObject *)self;
    for (Py_ssize_t i = 0; i < Py_SIZE(a); i++) {
        Py_VISIT(a->items[i]);
    }
    return 0;
}

PyTypeObject _PyObjectArray_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    .tp_name = "object_array",
    .tp_basicsize = sizeof(_PyObjectArrayObject) - sizeof(PyObject *),
    .tp_itemsize = sizeof(PyObject *),
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    .tp_traverse = object_array_traverse,
    .tp_free = PyObject_GC_Del,
};

PyObject **
_PyValue_ObjectArray(PyValue *items, Py_ssize_t n)
{
    Py_ssize_t i = 0;
    while (i < n && !PyValue_IsInline(items[i])) {
        i++;
    }
    if (i == n) {
        // A value that is not inline is the object's pointer, or NULL.  An
        // empty list has no item array, and NULL would be an error.
        static PyObject *empty[1];
        return n == 0 ? empty : (PyObject **)items;
    }
    _PyObjectArrayObject *a = PyObject_GC_NewVar(_PyObjectArrayObject,
                                                 &_PyObjectArray_Type, n);
    if (a == NULL) {
        return NULL;
    }
    for (i = 0; i < n; i++) {
        a->items[i] = NULL;
    }
    _PyObject_GC_TRACK(a);
    for (i = 0; i < n; i++) {
        if (PyValue_IsInline(items[i])) {
            a->items[i] = _PyValue_BoxInline(items[i]);
            if (a->items[i] == NULL) {
                return NULL;
            }
            VALUE_BOX_STAT_INC(_PY_BOX_CALL);
        }
        else {
            a->items[i] = (PyObject *)items[i].bits;
        }
    }
    return a->items;
}
