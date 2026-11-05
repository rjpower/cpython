#include "Python.h"
#include "pycore_value_ops.h"

/* As upstream's PyObject_RichCompare() and PyObject_Repr(), these entry
   points take their status from the operation and leave any other pending
   exception alone. Releases need no save (the release contract in
   pycore_value.h). */

Py_hash_t
_PyValue_Hash(PyValue value)
{
    if (PyValue_IsInlineInt(value)) {
        /* As long_hash(): |value| < 2**50 < _PyHASH_MODULUS, so the hash is
           the value. */
        Py_hash_t hash = PyValue_InlineInt64(value);
        return hash == -1 ? -2 : hash;
    }
    if (PyValue_IsInlineFloat(value)) {
        /* Inline floats exclude NaN, the only case using the object identity. */
        return _Py_HashDouble(Py_None, PyValue_InlineFloat(value));
    }
    /* As PyObject_Hash() itself, with an owner in case __hash__ removes the
       value from the container that it was read from. */
    PyValue owner = PyValue_MakeHeapSafe(_PyValue_BorrowTagged(value));
    Py_hash_t hash = PyObject_Hash(PyValue_AsKnownObjectBorrow(owner));
    PyValue_CLOSE(owner);
    return hash;
}

static int
compare_inline(PyValueBorrow left, PyValueBorrow right, int op)
{
    /* Every int51 is represented exactly by a double, whose significand has
       53 bits. No boxed subclass or NaN enters this path, so ordinary C
       comparisons preserve dispatch. */
    double a = PyValue_IsInlineInt(left)
               ? (double)PyValue_InlineInt64(left) : PyValue_InlineFloat(left);
    double b = PyValue_IsInlineInt(right)
               ? (double)PyValue_InlineInt64(right) : PyValue_InlineFloat(right);
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

/* A box that succeeds runs no Python (the release contract), so neither
   argument can leave its container in between. A caller may have an
   exception pending, as upstream's comparisons allow (a dict lookup in a
   deallocator); a failed box replaces it, and releasing the old exception can
   run Python, but then neither box is used. */
static int
box_operands(PyValueBorrow left, PyValueBorrow right, PyObject **a, PyObject **b)
{
    *a = PyValue_AsObject(left);
    *b = *a == NULL ? NULL : PyValue_AsObject(right);
    if (*b == NULL) {
        Py_CLEAR(*a);
        return -1;
    }
    return 0;
}

int
(_PyValue_RichCompareBool)(PyValueBorrow left, PyValueBorrow right, int op)
{
    /* Python's containers consider an object equal to itself, including NaN. */
    if ((op == Py_EQ || op == Py_NE) && PyValue_Is(left, right)) {
        return op == Py_EQ;
    }
    if (PyValue_IsInline(left) && PyValue_IsInline(right)) {
        return compare_inline(left, right, op);
    }
    PyObject *a, *b;
    if (box_operands(left, right, &a, &b) < 0) {
        return -1;
    }
    int result = PyObject_RichCompareBool(a, b, op);
    Py_DECREF(b);
    Py_DECREF(a);
    return result;
}

PyObject *
(_PyValue_RichCompareObject)(PyValueBorrow left, PyValueBorrow right, int op)
{
    if (PyValue_IsInline(left) && PyValue_IsInline(right)) {
        return PyBool_FromLong(compare_inline(left, right, op));
    }
    PyObject *a, *b;
    if (box_operands(left, right, &a, &b) < 0) {
        return NULL;
    }
    PyObject *result = PyObject_RichCompare(a, b, op);
    Py_DECREF(b);
    Py_DECREF(a);
    return result;
}

int
(_PyValue_WriteRepr)(PyUnicodeWriter *writer, PyValueBorrow value)
{
    PyObject *obj = PyValue_AsObject(value);
    if (obj == NULL) {
        return -1;
    }
    int result = PyUnicodeWriter_WriteRepr(writer, obj);
    Py_DECREF(obj);
    return result;
}
