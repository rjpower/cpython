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
Py_NO_INLINE PyObject *
_PyValue_BoxInline(PyValue v)
{
    assert(PyValue_IsInline(v));
#ifdef Py_STATS
    count_box_site((uintptr_t)__builtin_return_address(0));
#endif
    if (PyValue_IsInlineInt(v)) {
        OBJECT_STAT_INC(inline_int_boxes);
        return PyLong_FromLong(PyValue_Int32(v));
    }
    OBJECT_STAT_INC(inline_float_boxes);
    return PyFloat_FromDouble(PyValue_Double(v));
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
