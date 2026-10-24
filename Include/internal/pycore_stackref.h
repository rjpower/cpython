#ifndef Py_INTERNAL_STACKREF_H
#define Py_INTERNAL_STACKREF_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_long.h"          // _PyLong_CheckExactAndCompact()
#include "pycore_object.h"        // Py_DECREF_MORTAL
#include "pycore_object_deferred.h" // _PyObject_HasDeferredRefcount()
#include "pycore_stats.h"         // OBJECT_STAT_INC()

#include <stdbool.h>              // bool


/*
  This file introduces a new API for handling references on the stack, called
  _PyStackRef. This API is inspired by HPy.

  There are 3 main operations, that convert _PyStackRef to PyObject* and
  vice versa:

    1. Borrow (discouraged)
    2. Steal
    3. New

  Borrow means that the reference is converted without any change in ownership.
  This is discouraged because it makes verification much harder. It also makes
  unboxed integers harder in the future.

  Steal means that ownership is transferred to something else. The total
  number of references to the object stays the same.  The old reference is no
  longer valid.

  New creates a new reference from the old reference. The old reference
  is still valid.

  All _PyStackRef must be operated on by the new reference operations:

    1. DUP
    2. CLOSE

   DUP is roughly equivalent to Py_NewRef. It creates a new reference from an old
   reference. The old reference remains unchanged.

   CLOSE is roughly equivalent to Py_DECREF. It destroys a reference.

   Note that it is unsafe to borrow a _PyStackRef and then do normal
   CPython refcounting operations on it!

  Encoding

  On 64-bit builds without Py_GIL_DISABLED (Py_STACKREF_INLINE_VALUES), a
  _PyStackRef also holds exact ints that fit in int32 and exact non-NaN floats
  without a heap object, and interpreter-internal indices. The bits are laid
  out as follows (boil/design/02-tagged-values.md, section 2.1):

    Top 16 bits     Low 48 bits         Meaning
    0x0000          0x000000000001      NULL
    0x0000          0x000000000002      ERROR
    0x0000          ptr, bit 0 = 0      owned reference
    0x0000          ptr, bit 0 = 1      borrowed, immortal or deferred reference
    0x0000          ptr | 2             PyStackRef_Wrap() pointer (debug builds)
    0x0001          int48               index (Py_INDEX_BASE)
    0x0002-0x7FF2   double + 2^49       inline float, +0.0 to +inf
    0x8002-0xFFF2   double + 2^49       inline float, -0.0 to -inf
    0xFFFE          0x0000 then int32   inline int (Py_INT_BASE)
    other                               unused: images of NaNs, and 0xFFFF

  Values below Py_PTR_LIMIT are pointer space; Py_TAG_REFCNT and
  Py_TAG_INVALID have meaning only there. Indices and inline values own
  nothing, so CLOSE, DUP and Borrow leave them unchanged. NaN results are
  always boxed, because some NaN images overlap the other regions.

  An index is the position of a virtual iterator in the FOR_ITER family
  (null_or_index) or the saved lasti during exception unwinding. It never
  reaches Python code and must not be converted to a PyObject *.

  An inline value must be boxed with PyStackRef_Box() before it is
  converted to a PyObject *. The cases generator does this for instruction
  inputs; see "Inline values" in
  Tools/cases_generator/interpreter_definition.md.

  Py_STACKREF_DEBUG builds use the same layout with table IDs, which stay
  below Py_PTR_LIMIT, in place of addresses. Free-threaded and 32-bit builds
  have no inline values: an index there is (i << Py_TAGGED_SHIFT) |
  Py_INDEX_TAG and the inline predicates are constant false.
*/

#define Py_TAG_REFCNT 1
#define Py_TAG_INVALID 2
#define Py_TAG_BITS 3

#define Py_TAGGED_SHIFT 2

#if !defined(Py_GIL_DISABLED) && defined(Py_STACKREF_DEBUG)
#  define _PyStackRef_BITS(REF) ((REF).index)
#else
#  define _PyStackRef_BITS(REF) ((REF).bits)
#endif

#if SIZEOF_VOID_P == 8 && !defined(Py_GIL_DISABLED)

#define Py_STACKREF_INLINE_VALUES 1

#define Py_PTR_LIMIT     ((uintptr_t)1 << 48)
#define Py_INDEX_BASE    ((uintptr_t)1 << 48)
#define Py_DOUBLE_OFFSET ((uintptr_t)1 << 49)
#define Py_INT_BASE      ((uintptr_t)0xFFFE << 48)
// A reference owns a count on its object only if all of these bits are 0.
#define Py_OWNED_MASK    (((uintptr_t)0xFFFF << 48) | Py_TAG_REFCNT)

static inline bool PyStackRef_IsPointerSpace(_PyStackRef r) { return _PyStackRef_BITS(r) < Py_PTR_LIMIT; }
static inline bool PyStackRef_IsTaggedIndex(_PyStackRef r) { return _PyStackRef_BITS(r) - Py_INDEX_BASE < Py_DOUBLE_OFFSET - Py_INDEX_BASE; }
static inline bool PyStackRef_IsInline(_PyStackRef r) { return _PyStackRef_BITS(r) >= Py_DOUBLE_OFFSET; }
static inline bool PyStackRef_IsInlineInt(_PyStackRef r) { return _PyStackRef_BITS(r) >= Py_INT_BASE; }
static inline bool PyStackRef_IsInlineFloat(_PyStackRef r) { return _PyStackRef_BITS(r) - Py_DOUBLE_OFFSET < Py_INT_BASE - Py_DOUBLE_OFFSET; }
// Relies on 0xFFFF being unused: only two inline ints have all of these bits set.
static inline bool PyStackRef_BothInlineInt(_PyStackRef a, _PyStackRef b) { return (_PyStackRef_BITS(a) & _PyStackRef_BITS(b)) >= Py_INT_BASE; }

/* The top 16 bits of *ref, which select the region. Reading them from memory
   rather than shifting a loaded value lets the compiler fold the load into a
   compare with an immediate ("cmpw $0, 6(ref)" on x86-64), so testing a stack
   slot or a local costs a compare and a branch. */
static inline uint16_t
_PyStackRef_TopBitsAt(const _PyStackRef *ref)
{
#if PY_LITTLE_ENDIAN
    uint16_t top;
    memcpy(&top, (const char *)ref + 6, sizeof(top));
    return top;
#else
    return (uint16_t)(_PyStackRef_BITS(*ref) >> 48);
#endif
}

static inline bool PyStackRef_IsPointerSpaceAt(const _PyStackRef *ref) { return _PyStackRef_TopBitsAt(ref) == 0; }
static inline bool PyStackRef_IsInlineAt(const _PyStackRef *ref) { return _PyStackRef_TopBitsAt(ref) >= (Py_DOUBLE_OFFSET >> 48); }

// The bit that PyStackRef_Borrow() sets: Py_TAG_REFCNT in pointer space only.
static inline uintptr_t
_PyStackRef_BorrowBitAt(const _PyStackRef *ref)
{
    return (uintptr_t)PyStackRef_IsPointerSpaceAt(ref);
}

// The encoding of an inline int, which is also its identity (see "Value
// identity" below).
static inline uintptr_t
_PyStackRef_Int32Bits(int32_t i)
{
    return Py_INT_BASE | (uint32_t)i;
}

static inline _PyStackRef
PyStackRef_FromInt32(int32_t i)
{
    OBJECT_STAT_INC(inline_ints_created);
    return (_PyStackRef){ _PyStackRef_Int32Bits(i) };
}

static inline int32_t
PyStackRef_InlineInt(_PyStackRef r)
{
    assert(PyStackRef_IsInlineInt(r));
    return (int32_t)(uint32_t)_PyStackRef_BITS(r);
}

// The encoding of an inline float, which is also its identity.
static inline uintptr_t
_PyStackRef_DoubleBits(double d)
{
    assert(!isnan(d));
    uint64_t u;
    memcpy(&u, &d, sizeof(u));
    return u + Py_DOUBLE_OFFSET;
}

static inline _PyStackRef
PyStackRef_FromDoubleNotNaN(double d)
{
    return (_PyStackRef){ _PyStackRef_DoubleBits(d) };
}

static inline double
PyStackRef_InlineFloat(_PyStackRef r)
{
    assert(PyStackRef_IsInlineFloat(r));
    uint64_t u = _PyStackRef_BITS(r) - Py_DOUBLE_OFFSET;
    double d;
    memcpy(&d, &u, sizeof(d));
    return d;
}

// i is in -1 .. 2**47 - 1.
static inline _PyStackRef
PyStackRef_TagIndex(intptr_t i)
{
    assert(i >= -1 && i < ((intptr_t)1 << 47));
    return (_PyStackRef){ Py_INDEX_BASE | ((uintptr_t)i & (Py_PTR_LIMIT - 1)) };
}

static inline intptr_t
PyStackRef_UntagIndex(_PyStackRef r)
{
    assert(PyStackRef_IsTaggedIndex(r));
    return Py_ARITHMETIC_RIGHT_SHIFT(intptr_t, (intptr_t)(_PyStackRef_BITS(r) << 16), 16);
}

/* Increments a tagged index, but does not check for overflow */
static inline _PyStackRef
PyStackRef_IncrementTaggedIndexNoOverflow(_PyStackRef r)
{
    assert(PyStackRef_UntagIndex(r) >= 0);
    assert(PyStackRef_UntagIndex(r) < ((intptr_t)1 << 47) - 1);
    _PyStackRef_BITS(r)++;
    return r;
}

PyAPI_FUNC(_PyStackRef) _PyStackRef_BoxInlineValue(_PyStackRef ref);

/* If ref holds an inline value, return an owned reference to a new box with
   the same value; return any other ref unchanged. On failure, set an exception
   and return ref unchanged, so the result is still inline. Boxing runs no
   Python code and cannot start a collection.

   Callers pass and assign the value (x = PyStackRef_Box(x)) rather than its
   address, so that x can stay in a register. */
static inline _PyStackRef
PyStackRef_Box(_PyStackRef ref)
{
    return PyStackRef_IsInline(ref) ? _PyStackRef_BoxInlineValue(ref) : ref;
}

#else  // No inline values: free-threaded and 32-bit builds.

#define Py_INDEX_TAG 3
#define Py_OWNED_MASK Py_TAG_REFCNT

static inline bool PyStackRef_IsTaggedIndex(_PyStackRef r) { return (_PyStackRef_BITS(r) & Py_TAG_BITS) == Py_INDEX_TAG; }
static inline bool PyStackRef_IsPointerSpace(_PyStackRef r) { return !PyStackRef_IsTaggedIndex(r); }
static inline bool PyStackRef_IsInline(_PyStackRef r) { (void)r; return false; }
static inline bool PyStackRef_IsInlineInt(_PyStackRef r) { (void)r; return false; }
static inline bool PyStackRef_IsInlineFloat(_PyStackRef r) { (void)r; return false; }
static inline bool PyStackRef_BothInlineInt(_PyStackRef a, _PyStackRef b) { (void)a; (void)b; return false; }
static inline bool PyStackRef_IsPointerSpaceAt(const _PyStackRef *ref) { return PyStackRef_IsPointerSpace(*ref); }
static inline bool PyStackRef_IsInlineAt(const _PyStackRef *ref) { (void)ref; return false; }

// Nothing is inline, so code behind the tests above never calls these.
static inline int32_t PyStackRef_InlineInt(_PyStackRef r) { (void)r; Py_UNREACHABLE(); }
static inline double PyStackRef_InlineFloat(_PyStackRef r) { (void)r; Py_UNREACHABLE(); }
static inline _PyStackRef PyStackRef_FromDoubleNotNaN(double d) { (void)d; Py_UNREACHABLE(); }

// Indices already have Py_TAG_REFCNT set.
static inline uintptr_t
_PyStackRef_BorrowBitAt(const _PyStackRef *ref)
{
    (void)ref;
    return Py_TAG_REFCNT;
}

static inline _PyStackRef
PyStackRef_TagIndex(intptr_t i)
{
    assert(Py_ARITHMETIC_RIGHT_SHIFT(intptr_t, (intptr_t)(((uintptr_t)i) << Py_TAGGED_SHIFT),
                                     Py_TAGGED_SHIFT) == i);
    return (_PyStackRef){ .bits = ((((uintptr_t)i) << Py_TAGGED_SHIFT) | Py_INDEX_TAG) };
}

static inline intptr_t
PyStackRef_UntagIndex(_PyStackRef r)
{
    assert(PyStackRef_IsTaggedIndex(r));
    return Py_ARITHMETIC_RIGHT_SHIFT(intptr_t, (intptr_t)r.bits, Py_TAGGED_SHIFT);
}

/* Increments a tagged index, but does not check for overflow */
static inline _PyStackRef
PyStackRef_IncrementTaggedIndexNoOverflow(_PyStackRef r)
{
    assert(PyStackRef_IsTaggedIndex(r));
    assert((r.bits & (~Py_TAG_BITS)) != (INTPTR_MAX & (~Py_TAG_BITS))); // Isn't about to overflow
    return (_PyStackRef){ .bits = r.bits + (1 << Py_TAGGED_SHIFT) };
}

static inline _PyStackRef
PyStackRef_Box(_PyStackRef ref)
{
    return ref;
}

#endif  // SIZEOF_VOID_P == 8 && !defined(Py_GIL_DISABLED)

/* Value identity (boil/design/02-tagged-values.md, section 3)

   An exact int that fits in int32, or an exact float other than NaN, may be
   held inline in one place and boxed in others, and a value may be boxed
   more than once. The program must not be able to tell these apart, so such
   a value has a single identity: its inline encoding, _PyStackRef_Int32Bits()
   or _PyStackRef_DoubleBits(), whether it is held inline or boxed. Any other
   object's identity is its address. An encoding is at least Py_DOUBLE_OFFSET
   (2**49) and an address is below Py_PTR_LIMIT (2**48), so an identity names
   exactly one value or object.

   `is` (PyStackRef_Is(), hence IS_OP; operator.is_ and operator.is_not)
   compares identities and id() returns the identity, so id(x) == id(y)
   exactly when x is y, and id() of a value is the same before and after it
   is boxed. 0.0 and -0.0 differ in bits, so `0.0 is -0.0` is False. NaN is
   never inline and keeps object identity. Bools and int subclasses keep
   object identity. Py_Is() stays pointer equality: C code sees only boxes.

   Builds without inline values use object identity throughout. */

// The identity of op: its value encoding or its address.
PyAPI_FUNC(uintptr_t) _PyObject_ValueIdentity(PyObject *op);

#ifdef Py_STACKREF_INLINE_VALUES
// Do a and b, which are different references, hold the same value?
// Neither may be PyStackRef_NULL or PyStackRef_ERROR.
PyAPI_FUNC(int) _PyStackRef_IsSameValue(_PyStackRef a, _PyStackRef b);
#endif

#if !defined(Py_GIL_DISABLED) && defined(Py_STACKREF_DEBUG)

PyAPI_FUNC(PyObject *) _Py_stackref_get_object(_PyStackRef ref);
PyAPI_FUNC(PyObject *) _Py_stackref_close(_PyStackRef ref, const char *filename, int linenumber);
PyAPI_FUNC(_PyStackRef) _Py_stackref_create(PyObject *obj, uint16_t flags, const char *filename, int linenumber);
PyAPI_FUNC(void) _Py_stackref_record_borrow(_PyStackRef ref, const char *filename, int linenumber);
PyAPI_FUNC(_PyStackRef) _Py_stackref_get_borrowed_from(_PyStackRef ref, const char *filename, int linenumber);
PyAPI_FUNC(void) _Py_stackref_set_borrowed_from(_PyStackRef ref, _PyStackRef borrowed_from, const char *filename, int linenumber);
// Declare that borrows of `ref` may outlive its close, because another owner
// keeps the object alive (see framelocalsproxy_setitem()).
PyAPI_FUNC(void) _Py_stackref_allow_borrows_outlive_close(_PyStackRef ref);
extern void _Py_stackref_associate(PyInterpreterState *interp, PyObject *obj, _PyStackRef ref);

static const _PyStackRef PyStackRef_NULL = { .index = 0 };
static const _PyStackRef PyStackRef_ERROR = { .index = (1 << Py_TAGGED_SHIFT) };

#define PyStackRef_None ((_PyStackRef){ .index = (2 << Py_TAGGED_SHIFT) } )
#define _Py_STACKREF_FALSE_INDEX (3 << Py_TAGGED_SHIFT)
#define _Py_STACKREF_TRUE_INDEX (4 << Py_TAGGED_SHIFT)
#define PyStackRef_False ((_PyStackRef){ .index = _Py_STACKREF_FALSE_INDEX })
#define PyStackRef_True ((_PyStackRef){ .index = _Py_STACKREF_TRUE_INDEX })

#define INITIAL_STACKREF_INDEX (5 << Py_TAGGED_SHIFT)

static inline _PyStackRef
PyStackRef_Wrap(void *ptr)
{
    assert(ptr != NULL);
#ifdef Py_DEBUG
    assert(((uint64_t)ptr & Py_TAG_BITS) == 0);
    return (_PyStackRef){ .index = ((uint64_t)ptr) | Py_TAG_INVALID };
#else
    return (_PyStackRef){ .index = (uint64_t)ptr };
#endif
}

static inline void *
PyStackRef_Unwrap(_PyStackRef ref)
{
#ifdef Py_DEBUG
    assert ((ref.index & Py_TAG_BITS) == Py_TAG_INVALID);
    return (void *)(ref.index & ~Py_TAG_BITS);
#else
    return (void *)(ref.index);
#endif
}

static inline int
PyStackRef_RefcountOnObject(_PyStackRef ref)
{
    return (ref.index & Py_OWNED_MASK) == 0;
}

static inline int
PyStackRef_IsNull(_PyStackRef ref)
{
    return ref.index == 0;
}

static inline bool
PyStackRef_IsError(_PyStackRef ref)
{
    return ref.index == (1 << Py_TAGGED_SHIFT);
}

static inline bool
PyStackRef_IsValid(_PyStackRef ref)
{
    /* Invalid values are ERROR and NULL */
    return !PyStackRef_IsError(ref) && !PyStackRef_IsNull(ref);
}

static inline int
PyStackRef_IsTrue(_PyStackRef ref)
{
    return PyStackRef_IsPointerSpace(ref) && _Py_stackref_get_object(ref) == Py_True;
}

static inline int
PyStackRef_IsFalse(_PyStackRef ref)
{
    return PyStackRef_IsPointerSpace(ref) && _Py_stackref_get_object(ref) == Py_False;
}

static inline int
PyStackRef_IsNone(_PyStackRef ref)
{
    return PyStackRef_IsPointerSpace(ref) && _Py_stackref_get_object(ref) == Py_None;
}

// _Py_stackref_record_borrow() fails for an index or an inline value.
static inline PyObject *
_PyStackRef_AsPyObjectBorrow(_PyStackRef ref, const char *filename, int linenumber)
{
    assert(!PyStackRef_IsError(ref));
    _Py_stackref_record_borrow(ref, filename, linenumber);
    return _Py_stackref_get_object(ref);
}
#define PyStackRef_AsPyObjectBorrow(REF) _PyStackRef_AsPyObjectBorrow((REF), __FILE__, __LINE__)

// _Py_stackref_close() fails for an index or an inline value.
static inline PyObject *
_PyStackRef_AsPyObjectSteal(_PyStackRef ref, const char *filename, int linenumber)
{
    PyObject *obj = _Py_stackref_close(ref, filename, linenumber);
    if (PyStackRef_RefcountOnObject(ref)) {
        return obj;
    }
    return Py_NewRef(obj);
}
#define PyStackRef_AsPyObjectSteal(REF) _PyStackRef_AsPyObjectSteal((REF), __FILE__, __LINE__)

static inline _PyStackRef
_PyStackRef_FromPyObjectNew(PyObject *obj, const char *filename, int linenumber)
{
    assert(obj != NULL);
    uint16_t flags = 0;
    if (!_Py_IsImmortal(obj)) {
        _Py_INCREF_MORTAL(obj);
    } else {
        flags = Py_TAG_REFCNT;
    }
    return _Py_stackref_create(obj, flags, filename, linenumber);
}
#define PyStackRef_FromPyObjectNew(obj) _PyStackRef_FromPyObjectNew(_PyObject_CAST(obj), __FILE__, __LINE__)

static inline _PyStackRef
_PyStackRef_FromPyObjectSteal(PyObject *obj, const char *filename, int linenumber)
{
    assert(obj != NULL);
    uint16_t flags = 0;
    if (_Py_IsImmortal(obj)) {
        flags = Py_TAG_REFCNT;
    }
    return _Py_stackref_create(obj, flags, filename, linenumber);
}
#define PyStackRef_FromPyObjectSteal(obj) _PyStackRef_FromPyObjectSteal(_PyObject_CAST(obj), __FILE__, __LINE__)

static inline _PyStackRef
_PyStackRef_FromPyObjectBorrow(PyObject *obj, const char *filename, int linenumber)
{
    return _Py_stackref_create(obj, Py_TAG_REFCNT, filename, linenumber);
}
#define PyStackRef_FromPyObjectBorrow(obj) _PyStackRef_FromPyObjectBorrow(_PyObject_CAST(obj), __FILE__, __LINE__)

static inline void
_PyStackRef_CLOSE(_PyStackRef ref, const char *filename, int linenumber)
{
    assert(!PyStackRef_IsError(ref));
    assert(!PyStackRef_IsNull(ref));
    if (!PyStackRef_IsPointerSpace(ref)) {
        return;
    }
    PyObject *obj = _Py_stackref_close(ref, filename, linenumber);
    assert(Py_REFCNT(obj) > 0);
    if (PyStackRef_RefcountOnObject(ref)) {
        Py_DECREF(obj);
    }
}
#define PyStackRef_CLOSE(REF) _PyStackRef_CLOSE((REF), __FILE__, __LINE__)

static inline void
_PyStackRef_XCLOSE(_PyStackRef ref, const char *filename, int linenumber)
{
    assert(!PyStackRef_IsError(ref));
    if (PyStackRef_IsNull(ref)) {
        return;
    }
    _PyStackRef_CLOSE(ref, filename, linenumber);
}
#define PyStackRef_XCLOSE(REF) _PyStackRef_XCLOSE((REF), __FILE__, __LINE__)

static inline _PyStackRef
_PyStackRef_DUP(_PyStackRef ref, const char *filename, int linenumber)
{
    assert(!PyStackRef_IsError(ref));
    assert(!PyStackRef_IsNull(ref));
    if (!PyStackRef_IsPointerSpace(ref)) {
        return ref;
    }
    PyObject *obj = _Py_stackref_get_object(ref);
    uint16_t flags = 0;
    if (PyStackRef_RefcountOnObject(ref)) {
        Py_INCREF(obj);
    } else {
        flags = Py_TAG_REFCNT;
    }
    _PyStackRef new_ref = _Py_stackref_create(obj, flags, filename, linenumber);
    if (flags == Py_TAG_REFCNT && !_Py_IsImmortal(obj)) {
        _PyStackRef borrowed_from = _Py_stackref_get_borrowed_from(ref, filename, linenumber);
        _Py_stackref_set_borrowed_from(new_ref, borrowed_from, filename, linenumber);
    }
    return new_ref;
}
#define PyStackRef_DUP(REF) _PyStackRef_DUP(REF, __FILE__, __LINE__)

static inline _PyStackRef
_PyStackRef_DupImmortal(_PyStackRef ref, const char *filename, int linenumber)
{
    assert(!PyStackRef_IsError(ref));
    assert(PyStackRef_IsPointerSpace(ref));
    assert(!PyStackRef_RefcountOnObject(ref));
    PyObject *obj = _Py_stackref_get_object(ref);
    assert(_Py_IsImmortal(obj));
    return _Py_stackref_create(obj, Py_TAG_REFCNT, filename, linenumber);
}
#define PyStackRef_DupImmortal(REF) _PyStackRef_DupImmortal((REF), __FILE__, __LINE__)

static inline void
_PyStackRef_CLOSE_SPECIALIZED(_PyStackRef ref, destructor destruct, const char *filename, int linenumber)
{
    assert(!PyStackRef_IsError(ref));
    assert(!PyStackRef_IsNull(ref));
    if (!PyStackRef_IsPointerSpace(ref)) {
        return;
    }
    PyObject *obj = _Py_stackref_close(ref, filename, linenumber);
    assert(Py_REFCNT(obj) > 0);
    if (PyStackRef_RefcountOnObject(ref)) {
        _Py_DECREF_SPECIALIZED(obj, destruct);
    }
}
#define PyStackRef_CLOSE_SPECIALIZED(REF, DESTRUCT) _PyStackRef_CLOSE_SPECIALIZED(REF, DESTRUCT, __FILE__, __LINE__)

static inline _PyStackRef
_PyStackRef_Borrow(_PyStackRef ref, const char *filename, int linenumber)
{
    assert(!PyStackRef_IsError(ref));
    assert(!PyStackRef_IsNull(ref));
    if (!PyStackRef_IsPointerSpace(ref)) {
        return ref;
    }
    PyObject *obj = _Py_stackref_get_object(ref);
    _PyStackRef new_ref = _Py_stackref_create(obj, Py_TAG_REFCNT, filename, linenumber);
    if (!_Py_IsImmortal(obj)) {
        _Py_stackref_set_borrowed_from(new_ref, ref, filename, linenumber);
    }
    return new_ref;
}
#define PyStackRef_Borrow(REF) _PyStackRef_Borrow((REF), __FILE__, __LINE__)
#define PyStackRef_BorrowAt(REF) _PyStackRef_Borrow(*(REF), __FILE__, __LINE__)

#define PyStackRef_CLEAR(REF) \
    do { \
        _PyStackRef *_tmp_op_ptr = &(REF); \
        _PyStackRef _tmp_old_op = (*_tmp_op_ptr); \
        *_tmp_op_ptr = PyStackRef_NULL; \
        PyStackRef_XCLOSE(_tmp_old_op); \
    } while (0)

static inline _PyStackRef
_PyStackRef_FromPyObjectStealMortal(PyObject *obj, const char *filename, int linenumber)
{
    assert(obj != NULL);
    assert(!_Py_IsImmortal(obj));
    return _Py_stackref_create(obj, 0, filename, linenumber);
}
#define PyStackRef_FromPyObjectStealMortal(obj) _PyStackRef_FromPyObjectStealMortal(_PyObject_CAST(obj), __FILE__, __LINE__)

static inline bool
PyStackRef_IsHeapSafe(_PyStackRef ref)
{
    if ((ref.index & Py_TAG_BITS) != Py_TAG_REFCNT || PyStackRef_IsNull(ref) ||
        !PyStackRef_IsPointerSpace(ref)) {
        // ERROR, indices and inline values are included.
        return true;
    }

    PyObject *obj = _Py_stackref_get_object(ref);
    return _Py_IsImmortal(obj);
}

static inline _PyStackRef
_PyStackRef_MakeHeapSafe(_PyStackRef ref, const char *filename, int linenumber)
{
    // Special references that can't be closed, indices and inline values.
    if (ref.index < INITIAL_STACKREF_INDEX || !PyStackRef_IsPointerSpace(ref)) {
        return ref;
    }

    bool heap_safe = PyStackRef_IsHeapSafe(ref);
    PyObject *obj = _Py_stackref_close(ref, filename, linenumber);
    uint16_t flags = 0;
    if (heap_safe) {
        // Close old ref and create a new one with the same flags.
        // This is necessary for correct borrow checking.
        flags = ref.index & Py_TAG_BITS;
    } else {
        Py_INCREF(obj);
    }
    return _Py_stackref_create(obj, flags, filename, linenumber);
}
#define PyStackRef_MakeHeapSafe(REF) _PyStackRef_MakeHeapSafe(REF, __FILE__, __LINE__)

static inline _PyStackRef
_PyStackRef_FromPyObjectNewMortal(PyObject *obj, const char *filename, int linenumber)
{
    assert(obj != NULL);
    assert(!_Py_IsStaticImmortal(obj));
    Py_INCREF(obj);
    return _Py_stackref_create(obj, 0, filename, linenumber);
}
#define PyStackRef_FromPyObjectNewMortal(obj) _PyStackRef_FromPyObjectNewMortal(_PyObject_CAST(obj), __FILE__, __LINE__)

PyAPI_FUNC(int) PyStackRef_Is(_PyStackRef a, _PyStackRef b);

#else

// Only the Py_STACKREF_DEBUG checker tracks borrows.
#define _Py_stackref_allow_borrows_outlive_close(ref) ((void)(ref))

static const _PyStackRef PyStackRef_ERROR = { .bits = Py_TAG_INVALID };

/* Wrap a pointer in a stack ref.
 * The resulting stack reference is not safe and should only be used
 * in the interpreter to pass values from one uop to another.
 * The GC should never see one of these stack refs. */
static inline _PyStackRef
PyStackRef_Wrap(void *ptr)
{
    assert(ptr != NULL);
#ifdef Py_DEBUG
    assert(((uintptr_t)ptr & Py_TAG_BITS) == 0);
    return (_PyStackRef){ .bits = ((uintptr_t)ptr) | Py_TAG_INVALID };
#else
    return (_PyStackRef){ .bits = (uintptr_t)ptr };
#endif
}

static inline void *
PyStackRef_Unwrap(_PyStackRef ref)
{
#ifdef Py_DEBUG
    assert ((ref.bits & Py_TAG_BITS) == Py_TAG_INVALID);
    return (void *)(ref.bits & ~Py_TAG_BITS);
#else
    return (void *)(ref.bits);
#endif
}

static inline bool
PyStackRef_IsError(_PyStackRef ref)
{
    return ref.bits == Py_TAG_INVALID;
}

static inline bool
PyStackRef_IsValid(_PyStackRef ref)
{
    /* Invalid values are ERROR and NULL */
    return ref.bits > Py_TAG_INVALID;
}

/* References to immortal objects always have their tag bit set to Py_TAG_REFCNT
 * as they can (must) have their reclamation deferred */

#if _Py_IMMORTAL_FLAGS != Py_TAG_REFCNT
#  error "_Py_IMMORTAL_FLAGS != Py_TAG_REFCNT"
#endif

#define BITS_TO_PTR(REF) ((PyObject *)((REF).bits))
#define BITS_TO_PTR_MASKED(REF) ((PyObject *)(((REF).bits) & (~Py_TAG_REFCNT)))

#define PyStackRef_NULL_BITS Py_TAG_REFCNT
static const _PyStackRef PyStackRef_NULL = { .bits = PyStackRef_NULL_BITS };

#define PyStackRef_IsNull(ref) ((ref).bits == PyStackRef_NULL_BITS)
#define PyStackRef_True ((_PyStackRef){.bits = ((uintptr_t)&_Py_TrueStruct) | Py_TAG_REFCNT })
#define PyStackRef_False ((_PyStackRef){.bits = ((uintptr_t)&_Py_FalseStruct) | Py_TAG_REFCNT })
#define PyStackRef_None ((_PyStackRef){.bits = ((uintptr_t)&_Py_NoneStruct) | Py_TAG_REFCNT })

#ifdef Py_GIL_DISABLED
// Checks that mask out the deferred bit in the free threading build.
#define PyStackRef_IsNone(REF) (((REF).bits & ~Py_TAG_REFCNT) == (uintptr_t)&_Py_NoneStruct)
#define PyStackRef_IsTrue(REF) (((REF).bits & ~Py_TAG_REFCNT) == (uintptr_t)&_Py_TrueStruct)
#define PyStackRef_IsFalse(REF) (((REF).bits & ~Py_TAG_REFCNT) == (uintptr_t)&_Py_FalseStruct)
#else
#define PyStackRef_IsTrue(REF) ((REF).bits == (((uintptr_t)&_Py_TrueStruct) | Py_TAG_REFCNT))
#define PyStackRef_IsFalse(REF) ((REF).bits == (((uintptr_t)&_Py_FalseStruct) | Py_TAG_REFCNT))
#define PyStackRef_IsNone(REF) ((REF).bits == (((uintptr_t)&_Py_NoneStruct) | Py_TAG_REFCNT))
#endif

#if defined(Py_DEBUG) && !defined(Py_GIL_DISABLED)

static inline void
PyStackRef_CheckValid(_PyStackRef ref)
{
    assert(ref.bits != 0);
    assert(PyStackRef_IsPointerSpace(ref));
    int tag = ref.bits & Py_TAG_BITS;
    PyObject *obj = BITS_TO_PTR_MASKED(ref);
    switch (tag) {
        case 0:
            /* Can be immortal if object was made immortal after reference came into existence */
            assert(!_Py_IsStaticImmortal(obj));
            break;
        case Py_TAG_REFCNT:
            break;
        default:
            assert(0);
    }
}

#else

#define PyStackRef_CheckValid(REF) ((void)0)

#endif

#ifdef _WIN32
#define PyStackRef_RefcountOnObject(REF) (((REF).bits & Py_OWNED_MASK) == 0)
#define PyStackRef_AsPyObjectBorrow BITS_TO_PTR_MASKED
#define PyStackRef_Borrow(REF) (_PyStackRef){ .bits = ((REF).bits) | _PyStackRef_BorrowBitAt(&(REF))};
#define PyStackRef_BorrowAt(REF) (_PyStackRef){ .bits = ((REF)->bits) | _PyStackRef_BorrowBitAt(REF)};
#else
/* Does this ref own a count on its object? False for references to immortal
   objects, borrowed and deferred references, indices and inline values. */
static inline int
PyStackRef_RefcountOnObject(_PyStackRef ref)
{
    return (ref.bits & Py_OWNED_MASK) == 0;
}

static inline PyObject *
PyStackRef_AsPyObjectBorrow(_PyStackRef ref)
{
    assert(PyStackRef_IsPointerSpace(ref));
    return BITS_TO_PTR_MASKED(ref);
}

/* Borrow the reference in *ref, typically a local: see PyStackRef_Borrow().
   Passing the slot's address lets the pointer-space test read it in place. */
static inline _PyStackRef
PyStackRef_BorrowAt(const _PyStackRef *ref)
{
    return (_PyStackRef){ .bits = ref->bits | _PyStackRef_BorrowBitAt(ref) };
}

static inline _PyStackRef
PyStackRef_Borrow(_PyStackRef ref)
{
    return PyStackRef_BorrowAt(&ref);
}
#endif

static inline PyObject *
PyStackRef_AsPyObjectSteal(_PyStackRef ref)
{
    assert(!PyStackRef_IsNull(ref));
    assert(PyStackRef_IsPointerSpace(ref));
    if (PyStackRef_RefcountOnObject(ref)) {
        return BITS_TO_PTR(ref);
    }
    else {
        return Py_NewRef(BITS_TO_PTR_MASKED(ref));
    }
}

static inline _PyStackRef
PyStackRef_FromPyObjectSteal(PyObject *obj)
{
    assert(obj != NULL);
#ifdef Py_GIL_DISABLED
    return (_PyStackRef){ .bits = (uintptr_t)obj };
#else
#  if SIZEOF_VOID_P > 4
    unsigned int tag = obj->ob_flags & Py_TAG_REFCNT;
#  else
    unsigned int tag = _Py_IsImmortal(obj) ? Py_TAG_REFCNT : 0;
#  endif
    _PyStackRef ref = ((_PyStackRef){.bits = ((uintptr_t)(obj)) | tag});
    PyStackRef_CheckValid(ref);
    return ref;
#endif
}

static inline _PyStackRef
PyStackRef_FromPyObjectStealMortal(PyObject *obj)
{
    assert(obj != NULL);
    assert(!_Py_IsImmortal(obj));
    _PyStackRef ref = (_PyStackRef){ .bits = (uintptr_t)obj };
    PyStackRef_CheckValid(ref);
    return ref;
}

static inline _PyStackRef
_PyStackRef_FromPyObjectNew(PyObject *obj)
{
    assert(obj != NULL);
#ifdef Py_GIL_DISABLED
    if (_PyObject_HasDeferredRefcount(obj)) {
        return (_PyStackRef){ .bits = (uintptr_t)obj | Py_TAG_REFCNT };
    }
#else
    if (_Py_IsImmortal(obj)) {
        return (_PyStackRef){ .bits = (uintptr_t)obj | Py_TAG_REFCNT };
    }
#endif
    _Py_INCREF_MORTAL(obj);
    _PyStackRef ref = (_PyStackRef){ .bits = (uintptr_t)obj };
    PyStackRef_CheckValid(ref);
    return ref;
}
#define PyStackRef_FromPyObjectNew(obj) _PyStackRef_FromPyObjectNew(_PyObject_CAST(obj))

static inline _PyStackRef
_PyStackRef_FromPyObjectNewMortal(PyObject *obj)
{
    assert(obj != NULL);
    _Py_INCREF_MORTAL(obj);
    _PyStackRef ref = (_PyStackRef){ .bits = (uintptr_t)obj };
    PyStackRef_CheckValid(ref);
    return ref;
}
#define PyStackRef_FromPyObjectNewMortal(obj) _PyStackRef_FromPyObjectNewMortal(_PyObject_CAST(obj))

/* Create a new reference from an object with an embedded reference count */
static inline _PyStackRef
PyStackRef_FromPyObjectBorrow(PyObject *obj)
{
    assert(obj != NULL);
    return (_PyStackRef){ .bits = (uintptr_t)obj | Py_TAG_REFCNT};
}

/* WARNING: This macro evaluates its argument more than once */
#ifdef _WIN32
#define PyStackRef_DUP(REF) \
    (PyStackRef_RefcountOnObject(REF) ? (_Py_INCREF_MORTAL(BITS_TO_PTR(REF)), (REF)) : (REF))
#else
static inline _PyStackRef
PyStackRef_DUP(_PyStackRef ref)
{
    assert(!PyStackRef_IsNull(ref));
    if (PyStackRef_RefcountOnObject(ref)) {
        _Py_INCREF_MORTAL(BITS_TO_PTR(ref));
    }
    return ref;
}
#endif

static inline _PyStackRef
PyStackRef_DupImmortal(_PyStackRef ref)
{
    assert(!PyStackRef_IsNull(ref));
    assert(!PyStackRef_RefcountOnObject(ref));
    assert(_Py_IsImmortal(BITS_TO_PTR_MASKED(ref)));
    return ref;
}

static inline bool
PyStackRef_IsHeapSafe(_PyStackRef ref)
{
#ifdef Py_GIL_DISABLED
    if ((ref.bits & Py_TAG_BITS) != Py_TAG_REFCNT) {
        return true;
    }
    PyObject *obj = BITS_TO_PTR_MASKED(ref);
    return obj == NULL || _PyObject_HasDeferredRefcount(obj);
#else
    return (ref.bits & Py_TAG_BITS) != Py_TAG_REFCNT || ref.bits == PyStackRef_NULL_BITS ||
           !PyStackRef_IsPointerSpace(ref) || _Py_IsImmortal(BITS_TO_PTR_MASKED(ref));
#endif
}

static inline _PyStackRef
PyStackRef_MakeHeapSafe(_PyStackRef ref)
{
    if (PyStackRef_IsHeapSafe(ref)) {
        return ref;
    }
    PyObject *obj = BITS_TO_PTR_MASKED(ref);
    Py_INCREF(obj);
    ref.bits = (uintptr_t)obj;
    PyStackRef_CheckValid(ref);
    return ref;
}

// Convert a possibly deferred reference to a strong reference.
static inline _PyStackRef
PyStackRef_AsStrongReference(_PyStackRef stackref)
{
    return PyStackRef_FromPyObjectSteal(PyStackRef_AsPyObjectSteal(stackref));
}

#ifdef _WIN32
#define PyStackRef_CLOSE(REF) \
do { \
    _PyStackRef _temp = (REF); \
    if (PyStackRef_RefcountOnObject(_temp)) Py_DECREF_MORTAL(BITS_TO_PTR(_temp)); \
} while (0)
#else
static inline void
PyStackRef_CLOSE(_PyStackRef ref)
{
    assert(!PyStackRef_IsNull(ref));
    if (PyStackRef_RefcountOnObject(ref)) {
        Py_DECREF_MORTAL(BITS_TO_PTR(ref));
    }
}
#endif

static inline void
PyStackRef_CLOSE_SPECIALIZED(_PyStackRef ref, destructor destruct)
{
    assert(!PyStackRef_IsNull(ref));
    if (PyStackRef_RefcountOnObject(ref)) {
        Py_DECREF_MORTAL_SPECIALIZED(BITS_TO_PTR(ref), destruct);
    }
}

#ifdef _WIN32
#define PyStackRef_XCLOSE PyStackRef_CLOSE
#else
static inline void
PyStackRef_XCLOSE(_PyStackRef ref)
{
    assert(ref.bits != 0);
    if (PyStackRef_RefcountOnObject(ref)) {
        assert(!PyStackRef_IsNull(ref));
        Py_DECREF_MORTAL(BITS_TO_PTR(ref));
    }
}
#endif

#define PyStackRef_CLEAR(REF) \
    do { \
        _PyStackRef *_tmp_op_ptr = &(REF); \
        _PyStackRef _tmp_old_op = (*_tmp_op_ptr); \
        *_tmp_op_ptr = PyStackRef_NULL; \
        PyStackRef_XCLOSE(_tmp_old_op); \
    } while (0)


// Python's `is`: the same reference, ignoring bit 0 of a pointer, or else
// the same value (see "Value identity" below), which only the out-of-line
// test can establish. Indices compare exactly.
// Note: this is a macro because MSVC (Windows) has trouble inlining it.
#ifdef Py_STACKREF_INLINE_VALUES
#define PyStackRef_Is(a, b) \
    (((((a).bits ^ (b).bits) & ~(uintptr_t)PyStackRef_IsPointerSpace(a)) == 0) \
     || _PyStackRef_IsSameValue((a), (b)))
#else
#define PyStackRef_Is(a, b) \
    ((((a).bits ^ (b).bits) & ~(uintptr_t)PyStackRef_IsPointerSpace(a)) == 0)
#endif


#endif // !defined(Py_GIL_DISABLED) && defined(Py_STACKREF_DEBUG)

/* Close, dup or steal a reference known to be in pointer space, which
   includes PyStackRef_NULL: the cases generator knows this of a value that
   passed a guard, was boxed or was built from an object (see "Inline values"
   in Tools/cases_generator/interpreter_definition.md). There a reference owns
   a count unless Py_TAG_REFCNT is set, so these test that bit alone instead
   of Py_OWNED_MASK. */
#if defined(Py_STACKREF_INLINE_VALUES) && !defined(Py_STACKREF_DEBUG)

static inline int
_PyStackRef_PointerOwnsCount(_PyStackRef ref)
{
    assert(PyStackRef_IsPointerSpace(ref));
    return (ref.bits & Py_TAG_REFCNT) == 0;
}

// PyStackRef_NULL owns no count, so this also serves as XCLOSE.
static inline void
_PyStackRef_XCLOSE_POINTER(_PyStackRef ref)
{
    if (_PyStackRef_PointerOwnsCount(ref)) {
        Py_DECREF_MORTAL(BITS_TO_PTR(ref));
    }
}

static inline void
_PyStackRef_CLOSE_POINTER(_PyStackRef ref)
{
    assert(!PyStackRef_IsNull(ref));
    _PyStackRef_XCLOSE_POINTER(ref);
}

static inline void
_PyStackRef_CLOSE_SPECIALIZED_POINTER(_PyStackRef ref, destructor destruct)
{
    assert(!PyStackRef_IsNull(ref));
    if (_PyStackRef_PointerOwnsCount(ref)) {
        Py_DECREF_MORTAL_SPECIALIZED(BITS_TO_PTR(ref), destruct);
    }
}

static inline _PyStackRef
_PyStackRef_DUP_POINTER(_PyStackRef ref)
{
    assert(!PyStackRef_IsNull(ref));
    if (_PyStackRef_PointerOwnsCount(ref)) {
        _Py_INCREF_MORTAL(BITS_TO_PTR(ref));
    }
    return ref;
}

static inline PyObject *
_PyStackRef_AsPyObjectStealPointer(_PyStackRef ref)
{
    assert(!PyStackRef_IsNull(ref));
    if (_PyStackRef_PointerOwnsCount(ref)) {
        return BITS_TO_PTR(ref);
    }
    return Py_NewRef(BITS_TO_PTR_MASKED(ref));
}

#else

#define _PyStackRef_CLOSE_POINTER PyStackRef_CLOSE
#define _PyStackRef_XCLOSE_POINTER PyStackRef_XCLOSE
#define _PyStackRef_CLOSE_SPECIALIZED_POINTER PyStackRef_CLOSE_SPECIALIZED
#define _PyStackRef_DUP_POINTER PyStackRef_DUP
#define _PyStackRef_AsPyObjectStealPointer PyStackRef_AsPyObjectSteal

#endif

static inline bool
PyStackRef_IsMalformed(_PyStackRef ref)
{
    return PyStackRef_IsPointerSpace(ref) && (_PyStackRef_BITS(ref) & Py_TAG_BITS) == Py_TAG_INVALID;
}

// Does ref refer to an object? False for NULL, ERROR, indices and inline values.
static inline bool
PyStackRef_IsObject(_PyStackRef ref)
{
    return PyStackRef_IsPointerSpace(ref) && PyStackRef_IsValid(ref);
}

static inline PyTypeObject *
PyStackRef_TYPE(_PyStackRef stackref) {
    if (PyStackRef_IsInline(stackref)) {
        return PyStackRef_IsInlineInt(stackref) ? &PyLong_Type : &PyFloat_Type;
    }
    return Py_TYPE(PyStackRef_AsPyObjectBorrow(stackref));
}

// Converts a PyStackRef back to a PyObject *, converting the
// stackref to a new reference.
#define PyStackRef_AsPyObjectNew(stackref) Py_NewRef(PyStackRef_AsPyObjectBorrow(stackref))

// StackRef type checks. Indices and inline values are not objects of these types.

#define STACKREF_CHECK_FUNC(T) \
    static inline bool \
    PyStackRef_ ## T ## Check(_PyStackRef stackref) { \
        return PyStackRef_IsPointerSpace(stackref) && \
               Py ## T ## _Check(PyStackRef_AsPyObjectBorrow(stackref)); \
    }

STACKREF_CHECK_FUNC(Gen)
STACKREF_CHECK_FUNC(Bool)
STACKREF_CHECK_FUNC(ExceptionInstance)
STACKREF_CHECK_FUNC(Code)
STACKREF_CHECK_FUNC(Function)

static inline bool
PyStackRef_LongCheck(_PyStackRef stackref)
{
    return PyStackRef_IsInlineInt(stackref) ||
           (PyStackRef_IsPointerSpace(stackref) && PyLong_Check(PyStackRef_AsPyObjectBorrow(stackref)));
}

static inline bool
PyStackRef_FloatCheck(_PyStackRef stackref)
{
    return PyStackRef_IsInlineFloat(stackref) ||
           (PyStackRef_IsPointerSpace(stackref) && PyFloat_Check(PyStackRef_AsPyObjectBorrow(stackref)));
}

static inline bool
PyStackRef_LongCheckExact(_PyStackRef stackref)
{
    return PyStackRef_IsInlineInt(stackref) ||
           (PyStackRef_IsPointerSpace(stackref) && PyLong_CheckExact(PyStackRef_AsPyObjectBorrow(stackref)));
}

/* Ints in specialized instructions. The int guards accept an inline int or
   a reference to an exact int with at most one digit; either way the value
   fits in int32. These accessors read both forms without converting the
   stackref to a PyObject *, so the cases generator does not box their
   argument (see "Inline values" in
   Tools/cases_generator/interpreter_definition.md). */
static inline bool
PyStackRef_IsCompactInt(_PyStackRef ref)
{
    if (PyStackRef_IsInlineInt(ref)) {
        return true;
    }
    return PyStackRef_IsPointerSpace(ref)
           && _PyLong_CheckExactAndCompact(PyStackRef_AsPyObjectBorrow(ref));
}

static inline int32_t
PyStackRef_CompactIntValue(_PyStackRef ref)
{
    assert(PyStackRef_IsCompactInt(ref));
    if (PyStackRef_IsInlineInt(ref)) {
        return PyStackRef_InlineInt(ref);
    }
    return (int32_t)_PyLong_CompactValue((PyLongObject *)PyStackRef_AsPyObjectBorrow(ref));
}

#ifndef Py_STACKREF_INLINE_VALUES
/* Without inline values the result of int arithmetic is an int object:
   a new reference, or PyStackRef_NULL with an exception set. With them,
   PyStackRef_FromInt32() cannot fail and the callers' NULL tests fold away. */
static inline _PyStackRef
PyStackRef_FromInt32(int32_t i)
{
    PyObject *op = PyLong_FromLong(i);
    return op == NULL ? PyStackRef_NULL : PyStackRef_FromPyObjectSteal(op);
}
#endif

// A new int for a result outside int32 (the slow path of int arithmetic),
// or PyStackRef_NULL with an exception set.
PyAPI_FUNC(_PyStackRef) _PyStackRef_BoxInt64(int64_t value);

/* A new reference to the object that ref holds, a new box if ref holds an
   inline value, or NULL with an exception set if boxing fails. For C code
   that needs an object for a reference it does not own. */
static inline PyObject *
PyStackRef_AsPyObjectNewBoxed(_PyStackRef ref)
{
    if (PyStackRef_IsInline(ref)) {
        ref = PyStackRef_Box(ref);
        return PyStackRef_IsInline(ref) ? NULL : PyStackRef_AsPyObjectSteal(ref);
    }
    return PyStackRef_AsPyObjectNew(ref);
}

/* For each inline value src[i], store a new box in dst[i]; leave the other
   elements of dst alone. Return 0, or -1 with an exception set and no boxes
   left. For constructors that steal src only on success. */
static inline int
_PyStackRef_BoxInto(const _PyStackRef *src, PyObject **dst, Py_ssize_t n)
{
    for (Py_ssize_t i = 0; i < n; i++) {
        if (PyStackRef_IsInline(src[i])) {
            dst[i] = PyStackRef_AsPyObjectNewBoxed(src[i]);
            if (dst[i] == NULL) {
                while (--i >= 0) {
                    if (PyStackRef_IsInline(src[i])) {
                        Py_DECREF(dst[i]);
                    }
                }
                return -1;
            }
        }
    }
    return 0;
}

/* Box each inline value of refs[0..n) in place, for C code that converts
   the array to objects. Return 0, or -1 with an exception set if boxing
   fails; boxes made so far stay in the array, so callers close it as usual
   either way. */
static inline int
_PyStackRef_BoxArray(_PyStackRef *refs, Py_ssize_t n)
{
    for (Py_ssize_t i = 0; i < n; i++) {
        if (PyStackRef_IsInlineAt(&refs[i])) {
            refs[i] = PyStackRef_Box(refs[i]);
            if (PyStackRef_IsInline(refs[i])) {
                return -1;
            }
            OBJECT_STAT_INC(inline_array_boxes);
        }
    }
    return 0;
}

static inline void
_PyThreadState_PushCStackRef(PyThreadState *tstate, _PyCStackRef *ref)
{
#ifdef Py_GIL_DISABLED
    _PyThreadStateImpl *tstate_impl = (_PyThreadStateImpl *)tstate;
    ref->next = tstate_impl->c_stack_refs;
    tstate_impl->c_stack_refs = ref;
#endif
    ref->ref = PyStackRef_NULL;
}

static inline void
_PyThreadState_PushCStackRefNew(PyThreadState *tstate, _PyCStackRef *ref, PyObject *obj)
{
    _PyThreadState_PushCStackRef(tstate, ref);
    ref->ref = PyStackRef_FromPyObjectNew(obj);
}

static inline void
_PyThreadState_PopCStackRef(PyThreadState *tstate, _PyCStackRef *ref)
{
#ifdef Py_GIL_DISABLED
    _PyThreadStateImpl *tstate_impl = (_PyThreadStateImpl *)tstate;
    assert(tstate_impl->c_stack_refs == ref);
    tstate_impl->c_stack_refs = ref->next;
#endif
    PyStackRef_XCLOSE(ref->ref);
}

static inline _PyStackRef
_PyThreadState_PopCStackRefSteal(PyThreadState *tstate, _PyCStackRef *ref)
{
#ifdef Py_GIL_DISABLED
    _PyThreadStateImpl *tstate_impl = (_PyThreadStateImpl *)tstate;
    assert(tstate_impl->c_stack_refs == ref);
    tstate_impl->c_stack_refs = ref->next;
#endif
    return ref->ref;
}

#ifdef Py_GIL_DISABLED

static inline int
_Py_TryIncrefCompareStackRef(PyObject **src, PyObject *op, _PyStackRef *out)
{
    if (_PyObject_HasDeferredRefcount(op)) {
        *out = (_PyStackRef){ .bits = (uintptr_t)op | Py_TAG_REFCNT };
        return 1;
    }
    if (_Py_TryIncrefCompare(src, op)) {
        *out = PyStackRef_FromPyObjectSteal(op);
        return 1;
    }
    return 0;
}

static inline int
_Py_TryXGetStackRef(PyObject **src, _PyStackRef *out)
{
    PyObject *op = _PyObject_CAST(_Py_atomic_load_ptr_relaxed(src));
    if (op == NULL) {
        *out = PyStackRef_NULL;
        return 1;
    }
    return _Py_TryIncrefCompareStackRef(src, op, out);
}

#endif

#define PyStackRef_XSETREF(dst, src) \
    do { \
        _PyStackRef _tmp_dst_ref = (dst); \
        (dst) = (src); \
        PyStackRef_XCLOSE(_tmp_dst_ref); \
    } while(0)

// Like Py_VISIT but for _PyStackRef fields
#define _Py_VISIT_STACKREF(ref)                                         \
    do {                                                                \
        if (PyStackRef_IsObject(ref)) {                                 \
            int vret = _PyGC_VisitStackRef(&(ref), visit, arg);         \
            if (vret)                                                   \
                return vret;                                            \
        }                                                               \
    } while (0)

static inline void
_PyStackRef_CloseStack(_PyStackRef *arguments, int total_args)
{
    // arguments is a pointer into the GC visible stack,
    // so we must NULL out values as we clear them.
    for (int i = total_args-1; i >= 0; i--) {
        _PyStackRef tmp = arguments[i];
        arguments[i] = PyStackRef_NULL;
        PyStackRef_CLOSE(tmp);
    }
}

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_STACKREF_H */
