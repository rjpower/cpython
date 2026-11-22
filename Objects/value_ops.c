/* This file holds the hash dispatcher, so it is an adapter unit:
   Tools/build/check_value_api.py does not list its slot-field reads. */
#define Py_VALUE_ADAPTER 1

#include "Python.h"
#include "pycore_long_value.h"    // _PyLong_ValueHash()
#include "pycore_object.h"        // _Py_SwappedOp
#include "pycore_pyhash.h"        // _Py_HashPointerInline()
#include "pycore_typeobject.h"    // _PyType_IsReady()
#include "pycore_value_ops.h"
#include "pycore_value_slots.h"   // _PyType_VALUE_FIELD_AT()

/* As upstream's PyObject_RichCompare() and PyObject_Repr(), these entry
   points take their status from the operation and leave any other pending
   exception alone. Releases need no save (the release contract in
   pycore_value.h). Their callers own the operands (the dispatcher contract
   in pycore_value_ops.h).

   Rule R-inline (pyjit/design/proposals/value-api-v2a1a.md, D13): an inline
   value's type is int or float, fixed by the encoding, so a dispatcher may
   call that type's value function directly for an inline operand, through
   the static inline definition that the type's tp_value also stores. No
   other inline arm exists here. */

/* The port of PyObject_Hash(), whose value is an object: an inline value is
   an exact int or float, which _PyValue_Hash() hashes by R-inline. */
static Py_hash_t
hash_object_value(PyObject *op, PyValueBorrow value)
{
    PyTypeObject *tp = Py_TYPE(op);
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
    return tp->tp_hash(op);
}

static _Py_COLD_FUNCTION Py_NO_INLINE Py_hash_t
hash_inline_float(PyValueBorrow value)
{
    if (PyValue_IsInlineFloat(value)) {
        return _PyFloat_ValueHash(value);
    }
    _PyValue_FatalNotObject(value);
}

Py_hash_t
(_PyValue_Hash)(PyValueBorrow value)
{
    if (PyValue_IsPointerSpace(value)) {
        PyObject *op = BITS_TO_PTR_MASKED(value);
        if (Py_TYPE(op)->tp_hash == PyObject_GenericHash) {
            /* The default hash, of a class instance, a type or a function,
               is the hash of the address: the dispatch below would reach
               _PyValue_GenericHash() and PyObject_GenericHash() for it. An
               exact int or float never has this hash, so the address is the
               identity. */
            return _Py_HashPointerInline(op);
        }
        return hash_object_value(op, value);
    }
    /* Inline int keys are common (ids, counters, indexes), so their hash
       stays inline; an inline float key is rare. */
    if (PyValue_IsInlineInt(value)) {
        return _PyLong_ValueHash(value);
    }
    return hash_inline_float(value);
}

PyObject *
(_PyValue_RichCompareObject)(PyValueBorrow left, PyValueBorrow right, int op)
{
    PyValue res = (_PyValue_RichCompare)(left, right, op);
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
    /* R-inline for an inline int or float, and upstream's arms
       (PyUnicodeWriter_WriteRepr()) for None and a heap exact int. */
    if (PyValue_IsInlineInt(value)) {
        return _PyLong_WriteDecimal((_PyUnicodeWriter *)writer, value);
    }
    PyObject *repr;
    if (PyValue_IsInlineFloat(value)) {
        repr = _PyFloat_ValueRepr(value);
    }
    else if (PyValue_IsNone(value)) {
        return PyUnicodeWriter_WriteASCII(writer, "None", 4);
    }
    else if (PyValue_LongCheckExact(value)) {
        return _PyLong_WriteDecimal((_PyUnicodeWriter *)writer, value);
    }
    else {
        repr = (_PyValue_Repr)(value);
    }
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
    PyObject *obj = PyValue_ObjectOrNull(value);
    if (obj != NULL) {
        return PyObject_GenericHash(obj);
    }
    /* object.__hash__() of an exact int or float, whose own types hash by
       value: the identity hash that PyObject_GenericHash() gives any box of
       the value, without a box. */
    return Py_HashPointer((void *)_PyValue_Identity(value));
}
