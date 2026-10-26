#include "Python.h"

#include "pycore_long.h"          // _PyLong_IsCompact()
#include "pycore_object.h"
#include "pycore_stackref.h"


#ifdef Py_STACKREF_INLINE_VALUES

// The tag bits (Py_TAG_BITS) of an object pointer must be clear.
static_assert(_Alignof(PyObject) >= 8, "PyObject must be 8-byte aligned");

// Out of line, so that the test before each call is all that hot paths carry.
Py_NO_INLINE _PyStackRef
_PyStackRef_BoxInlineValue(_PyStackRef ref)
{
    assert(PyStackRef_IsInline(ref));
    PyObject *box;
    if (PyStackRef_IsInlineInt(ref)) {
        OBJECT_STAT_INC(inline_int_boxes);
        box = PyLong_FromLong(PyStackRef_InlineInt(ref));
    }
    else {
        OBJECT_STAT_INC(inline_float_boxes);
        box = PyFloat_FromDouble(PyStackRef_InlineFloat(ref));
    }
    if (box == NULL) {
        return ref;
    }
    return PyStackRef_FromPyObjectSteal(box);
}

// The identity of a reference that is not NULL or ERROR.
static uintptr_t
stackref_identity(_PyStackRef ref)
{
    if (!PyStackRef_IsPointerSpace(ref)) {
        return _PyStackRef_BITS(ref);
    }
    return _PyObject_ValueIdentity(BITS_TO_PTR_MASKED(ref));
}

int
_PyStackRef_IsSameValue(_PyStackRef a, _PyStackRef b)
{
    assert(PyStackRef_IsValid(a) && PyStackRef_IsValid(b));
    return stackref_identity(a) == stackref_identity(b);
}

#endif  // Py_STACKREF_INLINE_VALUES

uintptr_t
_PyObject_ValueIdentity(PyObject *op)
{
#ifdef Py_STACKREF_INLINE_VALUES
    if (PyLong_CheckExact(op)) {
        if (_PyLong_IsCompact((PyLongObject *)op)) {
            return _PyStackRef_Int32Bits((int32_t)_PyLong_CompactValue((PyLongObject *)op));
        }
        // Exact ints do not raise here.
        int overflow;
        long value = PyLong_AsLongAndOverflow(op, &overflow);
        if (!overflow && INT32_MIN <= value && value <= INT32_MAX) {
            return _PyStackRef_Int32Bits((int32_t)value);
        }
    }
    else if (PyFloat_CheckExact(op) && !isnan(PyFloat_AS_DOUBLE(op))) {
        return _PyStackRef_DoubleBits(PyFloat_AS_DOUBLE(op));
    }
#endif
    return (uintptr_t)op;
}

_PyStackRef
_PyStackRef_BoxInt64(int64_t value)
{
    PyObject *op = PyLong_FromLongLong(value);
    if (op == NULL) {
        return PyStackRef_NULL;
    }
    return PyStackRef_FromPyObjectSteal(op);
}

_PyStackRef
_PyStackRef_LshiftInt32(int32_t value, int32_t shift)
{
    assert(shift >= 0);
    PyObject *op = PyLong_FromLong(value);
    if (op == NULL) {
        return PyStackRef_NULL;
    }
    PyObject *res = _PyLong_Lshift(op, shift);
    Py_DECREF(op);
    if (res == NULL) {
        return PyStackRef_NULL;
    }
    return PyStackRef_FromPyObjectSteal(res);
}

_PyStackRef
_PyStackRef_BoxDouble(double value)
{
    PyObject *op = PyFloat_FromDouble(value);
    if (op == NULL) {
        return PyStackRef_NULL;
    }
    return PyStackRef_FromPyObjectSteal(op);
}
