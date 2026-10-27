#include "Python.h"

#include "pycore_long.h"          // _PyLong_IsCompact()
#include "pycore_object.h"
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

// Out of line, so that the test before each call is all that hot paths carry.
Py_NO_INLINE PyValue
_PyValue_BoxInline(PyValue v)
{
    assert(PyValue_IsInline(v));
#ifdef Py_STATS
    count_box_site((uintptr_t)__builtin_return_address(0));
#endif
    PyObject *box;
    if (PyValue_IsInlineInt(v)) {
        OBJECT_STAT_INC(inline_int_boxes);
        box = PyLong_FromLong(PyValue_Int32(v));
    }
    else {
        OBJECT_STAT_INC(inline_float_boxes);
        box = PyFloat_FromDouble(PyValue_Double(v));
    }
    if (box == NULL) {
        return v;
    }
    return PyValue_FromObject(box);
}

// The identity of a value that is not NULL or ERROR.
static uintptr_t
value_identity(PyValue v)
{
    if (!_PyValue_IsPointerSpace(v)) {
        return v.bits;
    }
    return _PyObject_ValueIdentity(PyValue_ObjectBorrow(v));
}

int
_PyValue_IsSameValue(PyValue a, PyValue b)
{
    assert(_PyValue_IsValid(a) && _PyValue_IsValid(b));
    return value_identity(a) == value_identity(b);
}


uintptr_t
_PyObject_ValueIdentity(PyObject *op)
{
    if (PyLong_CheckExact(op)) {
        if (_PyLong_IsCompact((PyLongObject *)op)) {
            return _PyValue_Int32Bits((int32_t)_PyLong_CompactValue((PyLongObject *)op));
        }
        // Exact ints do not raise here.
        int overflow;
        long value = PyLong_AsLongAndOverflow(op, &overflow);
        if (!overflow && INT32_MIN <= value && value <= INT32_MAX) {
            return _PyValue_Int32Bits((int32_t)value);
        }
    }
    else if (PyFloat_CheckExact(op) && !isnan(PyFloat_AS_DOUBLE(op))) {
        return _PyValue_DoubleBits(PyFloat_AS_DOUBLE(op));
    }
    return (uintptr_t)op;
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
