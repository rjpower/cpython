#ifndef Py_INTERNAL_PYVALUE_H
#define Py_INTERNAL_PYVALUE_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

/* One RC-aware value format on the supported private ABI. A borrow
 * (PyValueBorrow) owns no reference; a home receives an owner, which
 * PyValue_DUP() makes of a borrow. Inline values own no heap object. */
#if SIZEOF_VOID_P != 8 || defined(Py_GIL_DISABLED)
#  error "PyValue requires the 64-bit GIL private runtime"
#endif

/* The regions of the bits of a PyValue (see "Encoding" in
 * Include/internal/pycore_value.h). */
#define Py_PTR_LIMIT     ((uintptr_t)1 << 48)
#define Py_INDEX_BASE    ((uintptr_t)1 << 48)
#define Py_DOUBLE_OFFSET ((uintptr_t)1 << 49)
/* An inline int is int51: Py_INT_BASE | (v & (2**51 - 1)), where
   Py_INT_MIN <= v <= Py_INT_MAX. The top Py_INT_SHIFT bits of its word are
   set, and every word with them set is an inline int. */
#define Py_INT_SHIFT     13
#define Py_INT_BASE      (~(uintptr_t)0 << (64 - Py_INT_SHIFT))
#define Py_INT_MIN       (-((int64_t)1 << (63 - Py_INT_SHIFT)))
#define Py_INT_MAX       (((int64_t)1 << (63 - Py_INT_SHIFT)) - 1)

static_assert(Py_INT_BASE == (uintptr_t)0xFFF8 << 48, "int51 region");
// -inf, the largest float image, lies below the int region.
static_assert(((uintptr_t)0xFFF0 << 48) + Py_DOUBLE_OFFSET < Py_INT_BASE,
              "the float and int regions are disjoint");

/* Consume obj and return its canonical value, which owns what obj owned.
 * An exact int or float whose value is held inline is returned inline and
 * its box is released at once; that release runs no Python and leaves the
 * current exception unchanged ("Release contract" in
 * Include/internal/pycore_value.h). NULL gives PyValue_NULL. */
PyAPI_FUNC(PyValue) PyValue_PrepareOwned(PyObject *obj);

/* The view of an owner or a borrow, as a borrow: the same bits, at no cost.
 * Readers take a PyValueBorrow; each has a same-name macro that passes its
 * value arguments through PyValue_Borrow(), so that callers pass owners and
 * borrows alike, and nothing else (pyjit/design/proposals/
 * register-ir-v2-a5b.md, section 3.1). */
#ifdef __cplusplus
extern "C++" {
static inline PyValueBorrow
PyValue_Borrow(PyValue value)
{
    PyValueBorrow view = {value.bits};
    return view;
}

static inline PyValueBorrow
PyValue_Borrow(PyValueBorrow value)
{
    return value;
}
}
#else
static inline PyValueBorrow
_PyValue_BorrowOwned(PyValue value)
{
    return (PyValueBorrow){value.bits};
}

static inline PyValueBorrow
_PyValue_BorrowBorrowed(PyValueBorrow value)
{
    return value;
}

#define PyValue_Borrow(V) \
    _Generic((V), PyValue: _PyValue_BorrowOwned, \
                  PyValueBorrow: _PyValue_BorrowBorrowed)(V)
#endif

/* Whether the size bytes at address lie in the pointer space, so that a
 * word can hold the address of an object among them: the allocator's check
 * of the memory that it takes from the OS (Objects/obmalloc.c). */
static inline bool
_PyValue_InPointerSpace(const void *address, size_t size)
{
    return (uintptr_t)address + size <= Py_PTR_LIMIT;
}

static inline int
PyValue_IsObject(PyValueBorrow value)
{
    return value.bits > 2 && value.bits < Py_PTR_LIMIT &&
           (value.bits & 2) == 0;
}
#define PyValue_IsObject(V0) (PyValue_IsObject)(PyValue_Borrow(V0))

static inline int
PyValue_IsLanguageValue(PyValueBorrow value)
{
    if (value.bits < Py_DOUBLE_OFFSET) {
        return PyValue_IsObject(value);
    }
    if (value.bits >= Py_INT_BASE) {
        return 1;
    }
    uint64_t bits = value.bits - Py_DOUBLE_OFFSET;
    double number;
    memcpy(&number, &bits, sizeof(number));
    return !isnan(number);
}
#define PyValue_IsLanguageValue(V0) (PyValue_IsLanguageValue)(PyValue_Borrow(V0))

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_PYVALUE_H */
