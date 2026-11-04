#ifndef Py_PYVALUE_H
#define Py_PYVALUE_H
#ifdef __cplusplus
extern "C" {
#endif

/* One RC-aware value format on the supported private ABI. Borrowed pointer
 * values do not own a reference; storing one in a heap field requires an
 * explicit owning promotion. Inline values own no heap object. */
#if SIZEOF_VOID_P != 8 || defined(Py_GIL_DISABLED)
#  error "PyValue requires the 64-bit GIL private runtime"
#endif

/* The regions of the bits of a PyValue (see "Encoding" in
 * Include/internal/pycore_value.h). */
#define Py_PTR_LIMIT     ((uintptr_t)1 << 48)
#define Py_INDEX_BASE    ((uintptr_t)1 << 48)
#define Py_DOUBLE_OFFSET ((uintptr_t)1 << 49)
#define Py_INT_BASE      ((uintptr_t)0xFFFE << 48)

/* Consume obj and return its canonical value, which owns what obj owned.
 * An exact int or float whose value is held inline is returned inline and
 * its box is released at once; that release runs no Python and leaves the
 * current exception unchanged ("Release contract" in
 * Include/internal/pycore_value.h). NULL gives PyValue_NULL. */
PyAPI_FUNC(PyValue) PyValue_PrepareOwned(PyObject *obj);

/* Borrowed canonical input. The source owner must stay live until an explicit
 * owning promotion or the last use; an immediate does not retain that owner. */
PyAPI_FUNC(PyValue) PyValue_FromObjectBorrow(PyObject *obj);

/* Return a new object reference, boxing a scalar as needed. NULL denotes a
 * real error with an exception set. No borrowed scalar box is cached. */
PyAPI_FUNC(PyObject *) PyValue_AsObject(PyValue value);

static inline int
PyValue_IsObject(PyValue value)
{
    return value.bits > 2 && value.bits < Py_PTR_LIMIT &&
           (value.bits & 2) == 0;
}

/* Borrowing changes only object ownership: NULL, ERROR and scalar bits stay. */
static inline PyValue
PyValue_Borrow(PyValue value)
{
    value.bits |= (uintptr_t)PyValue_IsObject(value);
    return value;
}

/* NewRef creates an independent owner for a pointer value. A raw bit copy
 * merely aliases the existing owner. Scalars require no runtime call.
 * Immortal objects keep the no-count tag, so each has one encoding and
 * exact-bit tests such as PyValue_IsNone() hold for every owner. */
static inline PyValue
PyValue_NewRef(PyValue value)
{
    if (PyValue_IsObject(value)) {
        PyObject *obj = (PyObject *)(value.bits & ~(uintptr_t)1);
        if (_Py_IsImmortal(obj)) {
            value.bits = (uintptr_t)obj | 1;
        }
        else {
            Py_INCREF(obj);
            value.bits = (uintptr_t)obj;
        }
    }
    return value;
}

static inline void
PyValue_Close(PyValue value)
{
    if (PyValue_IsObject(value) && !(value.bits & 1)) {
        Py_DECREF((PyObject *)value.bits);
    }
}

static inline int
PyValue_IsLanguageValue(PyValue value)
{
    if (value.bits < Py_DOUBLE_OFFSET) {
        return PyValue_IsObject(value);
    }
    if ((value.bits & ~(uintptr_t)UINT32_MAX) == Py_INT_BASE) {
        return 1;
    }
    uint64_t bits = value.bits - Py_DOUBLE_OFFSET;
    double number;
    memcpy(&number, &bits, sizeof(number));
    return !isnan(number);
}

#ifdef __cplusplus
}
#endif
#endif /* Py_PYVALUE_H */
