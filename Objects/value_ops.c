#include "Python.h"
#include "pycore_value_ops.h"

/* Allocator hooks may raise during release. Preserve the operation's error;
   callers inspect a newly raised cleanup error before restoring their entry
   exception. The latter can legitimately be set during a raw dict lookup. */
static void
close_object(PyObject *obj)
{
    PyObject *saved = PyErr_GetRaisedException();
    Py_XDECREF(obj);
    _PyValue_RestoreException(saved);
}

static void
close_value(PyValue value)
{
    PyObject *saved = PyErr_GetRaisedException();
    PyValue_CLOSE(value);
    _PyValue_RestoreException(saved);
}

Py_hash_t
_PyValue_Hash(PyValue value)
{
    if (PyValue_IsInlineInt(value)) {
        Py_hash_t hash = PyValue_InlineInt(value);
        return hash == -1 ? -2 : hash;
    }
    if (PyValue_IsInlineFloat(value)) {
        /* Inline floats exclude NaN, the only case using the object identity. */
        return _Py_HashDouble(Py_None, PyValue_InlineFloat(value));
    }
    PyObject *prior = PyErr_GetRaisedException();
    PyValue owner = PyValue_MakeHeapSafe(PyValue_Borrow(value));
    Py_hash_t hash = PyObject_Hash(PyValue_AsPyObjectBorrow(owner));
    close_value(owner);
    int failed = hash == -1 || PyErr_Occurred();
    _PyValue_RestoreException(prior);
    return failed ? -1 : hash;
}

static int
compare_inline(PyValue left, PyValue right, int op)
{
    /* Every int32 is represented exactly by a double. No boxed subclass or
       NaN enters this path, so ordinary C comparisons preserve dispatch. */
    double a = PyValue_IsInlineInt(left)
               ? (double)PyValue_InlineInt(left) : PyValue_InlineFloat(left);
    double b = PyValue_IsInlineInt(right)
               ? (double)PyValue_InlineInt(right) : PyValue_InlineFloat(right);
    switch (op) {
        case Py_LT: return a < b;
        case Py_LE: return a <= b;
        case Py_EQ: return a == b;
        case Py_NE: return a != b;
        case Py_GT: return a > b;
        case Py_GE: return a >= b;
        default: Py_UNREACHABLE();
    }
}

/* Both arguments must survive even if allocating the first box reenters and
   removes the second argument from its original mutable container. */
static int
box_operands(PyValue left, PyValue right, PyObject **a, PyObject **b)
{
    PyValue left_owner = PyValue_MakeHeapSafe(PyValue_Borrow(left));
    PyValue right_owner = PyValue_MakeHeapSafe(PyValue_Borrow(right));
    *a = PyValue_AsObject(left_owner);
    *b = *a == NULL || PyErr_Occurred()
         ? NULL : PyValue_AsObject(right_owner);
    close_value(right_owner);
    close_value(left_owner);
    if (*b == NULL || PyErr_Occurred()) {
        close_object(*b);
        close_object(*a);
        *a = NULL;
        *b = NULL;
        return -1;
    }
    return 0;
}

int
_PyValue_RichCompareBool(PyValue left, PyValue right, int op)
{
    /* Python's containers consider an object equal to itself, including NaN. */
    if ((op == Py_EQ || op == Py_NE) && PyValue_Is(left, right)) {
        return op == Py_EQ;
    }
    if (PyValue_IsInline(left) && PyValue_IsInline(right)) {
        return compare_inline(left, right, op);
    }
    PyObject *prior = PyErr_GetRaisedException();
    PyObject *a, *b;
    if (box_operands(left, right, &a, &b) < 0) {
        _PyValue_RestoreException(prior);
        return -1;
    }
    int result = PyObject_RichCompareBool(a, b, op);
    close_object(b);
    close_object(a);
    int failed = result < 0 || PyErr_Occurred();
    _PyValue_RestoreException(prior);
    return failed ? -1 : result;
}

PyObject *
_PyValue_RichCompareObject(PyValue left, PyValue right, int op)
{
    if (PyValue_IsInline(left) && PyValue_IsInline(right)) {
        return PyBool_FromLong(compare_inline(left, right, op));
    }
    PyObject *prior = PyErr_GetRaisedException();
    PyObject *a, *b;
    if (box_operands(left, right, &a, &b) < 0) {
        _PyValue_RestoreException(prior);
        return NULL;
    }
    PyObject *result = PyObject_RichCompare(a, b, op);
    close_object(b);
    close_object(a);
    if (PyErr_Occurred()) {
        close_object(result);
        result = NULL;
    }
    _PyValue_RestoreException(prior);
    return result;
}

int
_PyValue_WriteRepr(PyUnicodeWriter *writer, PyValue value)
{
    PyObject *prior = PyErr_GetRaisedException();
    PyObject *obj = PyValue_AsObject(value);
    if (obj == NULL || PyErr_Occurred()) {
        close_object(obj);
        _PyValue_RestoreException(prior);
        return -1;
    }
    int result = PyUnicodeWriter_WriteRepr(writer, obj);
    close_object(obj);
    int failed = result < 0 || PyErr_Occurred();
    _PyValue_RestoreException(prior);
    return failed ? -1 : result;
}
