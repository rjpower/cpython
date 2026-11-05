/* This file holds the hash dispatcher, so it is an adapter unit:
   Tools/build/check_value_api.py does not list its slot-field reads. */
#define Py_VALUE_ADAPTER 1

#include "Python.h"
#include "pycore_typeobject.h"    // _PyType_IsReady()
#include "pycore_value_ops.h"
#include "pycore_value_slots.h"   // _PyType_VALUE_FIELD_AT()

/* As upstream's PyObject_RichCompare() and PyObject_Repr(), these entry
   points take their status from the operation and leave any other pending
   exception alone. Releases need no save (the release contract in
   pycore_value.h).

   The mixed paths call the dispatchers (pyjit/design/proposals/
   value-api-v2a0.md, section 4.6) after their inline arms, which V2a-1a
   moves into the value slots of int and float. Their callers read the
   operands from containers, so each keeps an owner of an object operand
   across the dispatch, which can run Python code that removes it, as
   upstream's callers do with Py_INCREF(). An inline operand owns nothing. */

static inline PyObject *
hold(PyValueBorrow value)
{
    if (PyValue_IsNull(value) || PyValue_IsInline(value)) {
        return NULL;
    }
    return Py_NewRef(PyValue_AsKnownObjectBorrow(value));
}

/* The port of PyObject_Hash(), whose value is an object: an inline value is
   an exact int or float, which _PyValue_Hash()'s arms hash. */
static Py_hash_t
hash_object_value(PyValueBorrow value)
{
    PyTypeObject *tp = PyValue_TYPE(value);
    int row = _Py_VALUE_SLOT_INDEX_hash;
    if (tp->tp_hash == NULL) {
        /* To keep to the general practice that inheriting
         * solely from object in C code should work without
         * an explicit call to PyType_Ready, we implicitly call
         * PyType_Ready here and then check the tp_hash slot again
         */
        if (_PyType_IsReady(tp)) {
            /* Otherwise, the object can't be hashed */
            return _PyValue_HashNotImplemented(value);
        }
        if (PyType_Ready(tp) < 0) {
            return -1;
        }
        if (tp->tp_hash == NULL) {
            return _PyValue_HashNotImplemented(value);
        }
    }
    _Py_hashvaluefunc f = (_Py_hashvaluefunc)_PyType_VALUE_FIELD_AT(tp, row);
    if (f != NULL) {
        _PyType_CHECK_VALUE_ROW(tp, row);
        return f(value);
    }
    return tp->tp_hash(PyValue_AsKnownObjectBorrow(value));
}

Py_hash_t
(_PyValue_Hash)(PyValueBorrow value)
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
    PyObject *owner = hold(value);
    Py_hash_t hash = hash_object_value(value);
    Py_XDECREF(owner);
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
    PyObject *owner_left = hold(left);
    PyObject *owner_right = hold(right);
    PyValue res = (_PyValue_RichCompare)(left, right, op);
    int ok = PyValue_IsNull(res) ? -1 : (_PyValue_IsTrue)(PyValue_Borrow(res));
    PyValue_XCLOSE(res);
    Py_XDECREF(owner_right);
    Py_XDECREF(owner_left);
    return ok;
}

PyObject *
(_PyValue_RichCompareObject)(PyValueBorrow left, PyValueBorrow right, int op)
{
    if (PyValue_IsInline(left) && PyValue_IsInline(right)) {
        return PyBool_FromLong(compare_inline(left, right, op));
    }
    PyObject *owner_left = hold(left);
    PyObject *owner_right = hold(right);
    PyValue res = (_PyValue_RichCompare)(left, right, op);
    Py_XDECREF(owner_right);
    Py_XDECREF(owner_left);
    if (PyValue_IsNull(res)) {
        return NULL;
    }
    if (PyValue_IsInline(res)) {
        /* A comparison that returned an exact int or float, for the
           interpreter or tuple_richcompare(): a CORE box. */
        return _PyValue_AsObjectAt(res, BOX_SITE_COMPARE);
    }
    return PyValue_AsKnownObjectSteal(res);
}

int
(_PyValue_WriteRepr)(PyUnicodeWriter *writer, PyValueBorrow value)
{
    PyObject *owner = hold(value);
    PyObject *repr = (_PyValue_Repr)(value);
    Py_XDECREF(owner);
    if (repr == NULL) {
        return -1;
    }
    int result = PyUnicodeWriter_WriteStr(writer, repr);
    Py_DECREF(repr);
    return result;
}

Py_hash_t
_PyValue_HashNotImplemented(PyValueBorrow value)
{
    PyErr_Format(PyExc_TypeError, "unhashable type: '%.200s'",
                 PyValue_TYPE(value)->tp_name);
    return -1;
}

Py_hash_t
_PyValue_GenericHash(PyValueBorrow value)
{
    if (!PyValue_IsInline(value)) {
        return PyObject_GenericHash(PyValue_AsKnownObjectBorrow(value));
    }
    /* object.__hash__() of an exact int or float, whose own types hash by
       value: the identity hash that PyObject_GenericHash() gives any box of
       the value, without a box. */
    return Py_HashPointer((void *)_PyValue_Identity(value));
}
