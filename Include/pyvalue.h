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

/* Pure preparation consumes obj without releasing it. For an inline scalar,
 * *retired receives that owner; otherwise the returned value owns it. Publish
 * the value and valid live state before releasing *retired. */
PyAPI_FUNC(PyValue) PyValue_PrepareOwned(PyObject *obj, PyObject **retired);

/* Borrowed canonical input. The source owner must stay live until an explicit
 * owning promotion or the last use; an immediate does not retain that owner. */
PyAPI_FUNC(PyValue) PyValue_FromObjectBorrow(PyObject *obj);

/* Return a new object reference, boxing a scalar as needed. NULL denotes a
 * real error with an exception set. No borrowed scalar box is cached. */
PyAPI_FUNC(PyObject *) PyValue_AsObject(PyValue value);

/* Borrowing changes only pointer ownership, never a scalar's bits. */
static inline PyValue
PyValue_Borrow(PyValue value)
{
    value.bits |= (uintptr_t)(value.bits < ((uintptr_t)1 << 48));
    return value;
}

static inline int
PyValue_IsObject(PyValue value)
{
    return value.bits > 2 && value.bits < ((uintptr_t)1 << 48) &&
           (value.bits & 2) == 0;
}

/* NewRef creates an independent owner for a pointer value. A raw bit copy
 * merely aliases the existing owner. Scalars require no runtime call. */
static inline PyValue
PyValue_NewRef(PyValue value)
{
    if (PyValue_IsObject(value)) {
        value.bits &= ~(uintptr_t)1;
        Py_INCREF((PyObject *)value.bits);
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
    if (value.bits < ((uintptr_t)1 << 49)) {
        return PyValue_IsObject(value);
    }
    if ((value.bits & ~(uintptr_t)UINT32_MAX) == ((uintptr_t)0xFFFE << 48)) {
        return 1;
    }
    uint64_t bits = value.bits - ((uintptr_t)1 << 49);
    double number;
    memcpy(&number, &bits, sizeof(number));
    return !isnan(number);
}

#ifdef __cplusplus
}
#endif
#endif /* Py_PYVALUE_H */
