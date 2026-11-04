#ifndef Py_INTERNAL_VALUE_H
#define Py_INTERNAL_VALUE_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_long.h"          // _PyLong_CheckExactAndCompact()
#include "pycore_object.h"        // Py_DECREF_MORTAL
#include "pycore_object_deferred.h" // _PyObject_HasDeferredRefcount()
#include "pycore_pyvalue.h"       // PyValue_IsObject()
#include "pycore_stats.h"         // OBJECT_STAT_INC()

#include <stdbool.h>              // bool

/* The canonical form of source, with the same ownership: an exact int or
 * float whose value can be held inline becomes that inline value, and a box
 * that source owned is released at once (see "Release contract" below).
 * Any other value is returned unchanged. */
PyAPI_FUNC(PyValue) _PyValue_Canonicalize(PyValue source);

/* A handler's boxes of inline inputs, which it passes to C code that takes
 * objects; it releases them when it finishes. The generated bound counts its
 * fixed scalar input occurrences; variable windows use owned object arrays.
 * Each box is an exact int or float, so releasing it runs no Python. */
typedef struct _PyValueTemps {
    PyObject **objects;
    Py_ssize_t count;
    Py_ssize_t capacity;
} _PyValueTemps;

/* Can releasing owner deallocate its object? Releasing a scalar, a borrow,
 * an immortal or an owner that is not the last cannot run Python. */
static inline int
_PyValue_MayDealloc(PyValue owner)
{
    if (PyValue_IsObject(owner) && (owner.bits & 1) == 0) {
        return Py_REFCNT((PyObject *)owner.bits) == 1;
    }
    return 0;
}

PyAPI_FUNC(void) _PyValueTemps_CloseNonempty(_PyValueTemps *temps);
PyAPI_FUNC(PyObject *) _PyValueTemps_Box(_PyValueTemps *temps, PyValue value);

static inline PyObject *
_PyValueTemps_Object(_PyValueTemps *temps, PyValue value)
{
    if (PyValue_IsObject(value)) {
        return (PyObject *)(value.bits & ~(uintptr_t)1);
    }
    return _PyValueTemps_Box(temps, value);
}

static inline void
_PyValueTemps_Close(_PyValueTemps *temps)
{
    if (temps != NULL && temps->count != 0) {
        _PyValueTemps_CloseNonempty(temps);
    }
}

/* The canonical form of *source, for a home.  If *source owns a box whose
 * value is inline, the box is released at once and *source becomes that
 * inline value too.  A borrowed box stays in *source: its owner (an input
 * box in _PyValueTemps, for example) keeps it alive for the C code that
 * still reads it as an object. */
static inline PyValue
_PyValue_Publish(PyValue *source)
{
    if (PyValue_IsObject(*source)) {
        PyObject *obj = (PyObject *)(source->bits & ~(uintptr_t)1);
        // A static immortal always carries the no-count tag.
        assert((source->bits & 1) || !_Py_IsStaticImmortal(obj));
        if (Py_IS_TYPE(obj, &PyLong_Type) || Py_IS_TYPE(obj, &PyFloat_Type)) {
            if (source->bits & 1) {  // A borrow: the no-count tag
                return _PyValue_Canonicalize(*source);
            }
            *source = _PyValue_Canonicalize(*source);
        }
    }
    return *source;
}


/*
  This file introduces a new API for handling references on the stack, called
  PyValue. This API is inspired by HPy.

  There are 3 main operations, that convert PyValue to PyObject* and
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

  All PyValue must be operated on by the new reference operations:

    1. DUP
    2. CLOSE

   DUP is roughly equivalent to Py_NewRef. It creates a new reference from an old
   reference. The old reference remains unchanged.

   CLOSE is roughly equivalent to Py_DECREF. It destroys a reference.

   Note that it is unsafe to borrow a PyValue and then do normal
   CPython refcounting operations on it!

  Encoding

  A PyValue also holds exact ints that fit in int32 and exact non-NaN floats
  without a heap object, and interpreter-internal indices. The bits are laid
  out as follows (boil/design/02-tagged-values.md, section 2.1):

    Top 16 bits     Low 48 bits         Meaning
    0x0000          0x000000000000      NULL
    0x0000          0x000000000002      ERROR
    0x0000          ptr, bit 0 = 0      owned reference
    0x0000          ptr, bit 0 = 1      borrowed, immortal or deferred reference
    0x0000          ptr | 2             PyValue_Wrap() pointer (debug builds)
    0x0001          int48               index (Py_INDEX_BASE)
    0x0002-0x7FF2   double + 2^49       inline float, +0.0 to +inf
    0x8002-0xFFF2   double + 2^49       inline float, -0.0 to -inf
    0xFFFE          0x0000 then int32   inline int (Py_INT_BASE)
    other                               unused: images of NaNs, and 0xFFFF

  Values below Py_PTR_LIMIT are pointer space; Py_TAG_REFCNT and
  Py_TAG_INVALID have meaning only there. Indices and inline values own
  nothing, so CLOSE, DUP and Borrow leave them unchanged. NaN results are
  always boxed, because some NaN images overlap the other regions.

  NULL is zero, so zero-filled memory holds NULL values: memory from a
  zeroing allocator needs no further initialization. The bits 1 and 3 are
  never produced.

  An index is the position of a virtual iterator in the FOR_ITER family
  (null_or_index) or the saved lasti during exception unwinding. It never
  reaches Python code and must not be converted to a PyObject *.

  An inline value must be boxed with PyValue_Box() before it is
  converted to a PyObject *. The cases generator does this for instruction
  inputs; see "Inline values" in
  Tools/cases_generator/interpreter_definition.md.

  Ordinary debug and release builds use this same payload. Other word
  sizes and free-threaded builds are outside the supported private ABI.
*/

#define Py_TAG_REFCNT 1
#define Py_TAG_INVALID 2
#define Py_TAG_BITS 3

#define Py_TAGGED_SHIFT 2

#  define _PyValue_BITS(REF) ((REF).bits)

// A reference owns a count on its object only if all of these bits are 0
// and it is not PyValue_NULL.
#define Py_OWNED_MASK    (((uintptr_t)0xFFFF << 48) | Py_TAG_BITS)

#define PyValue_NULL_BITS 0
static const PyValue PyValue_NULL = { .bits = PyValue_NULL_BITS };

#define PyValue_IsNull(ref) ((ref).bits == PyValue_NULL_BITS)

static inline bool PyValue_IsPointerSpace(PyValue r) { return _PyValue_BITS(r) < Py_PTR_LIMIT; }
static inline bool PyValue_IsTaggedIndex(PyValue r) { return _PyValue_BITS(r) - Py_INDEX_BASE < Py_DOUBLE_OFFSET - Py_INDEX_BASE; }
static inline bool PyValue_IsInline(PyValue r) { return _PyValue_BITS(r) >= Py_DOUBLE_OFFSET; }
static inline bool PyValue_IsInlineInt(PyValue r) { return _PyValue_BITS(r) >= Py_INT_BASE; }
static inline bool PyValue_IsInlineFloat(PyValue r) { return _PyValue_BITS(r) - Py_DOUBLE_OFFSET < Py_INT_BASE - Py_DOUBLE_OFFSET; }
// Relies on 0xFFFF being unused: only two inline ints have all of these bits set.
static inline bool PyValue_BothInlineInt(PyValue a, PyValue b) { return (_PyValue_BITS(a) & _PyValue_BITS(b)) >= Py_INT_BASE; }

/* The top 16 bits of *ref, which select the region. Reading them from memory
   rather than shifting a loaded value lets the compiler fold the load into a
   compare with an immediate ("cmpw $0, 6(ref)" on x86-64), so testing a stack
   slot or a local costs a compare and a branch. */
static inline uint16_t
_PyValue_TopBitsAt(const PyValue *ref)
{
#if PY_LITTLE_ENDIAN
    uint16_t top;
    memcpy(&top, (const char *)ref + 6, sizeof(top));
    return top;
#else
    return (uint16_t)(_PyValue_BITS(*ref) >> 48);
#endif
}

static inline bool PyValue_IsPointerSpaceAt(const PyValue *ref) { return _PyValue_TopBitsAt(ref) == 0; }
static inline bool PyValue_IsInlineAt(const PyValue *ref) { return _PyValue_TopBitsAt(ref) >= (Py_DOUBLE_OFFSET >> 48); }

// The bit that PyValue_Borrow() sets: Py_TAG_REFCNT on object values only.
// PyValue_NULL, debug-wrap pointers (Py_TAG_INVALID) and non-pointer values
// stay unchanged.
static inline uintptr_t
_PyValue_BorrowBitAt(const PyValue *ref)
{
    return (uintptr_t)(PyValue_IsPointerSpaceAt(ref) && !PyValue_IsNull(*ref)
                       && (ref->bits & Py_TAG_INVALID) == 0);
}

// The encoding of an inline int, which is also its identity (see "Value
// identity" below).
static inline uintptr_t
_PyValue_Int32Bits(int32_t i)
{
    return Py_INT_BASE | (uint32_t)i;
}

static inline PyValue
PyValue_FromInt32(int32_t i)
{
    OBJECT_STAT_INC(inline_ints_created);
    return (PyValue){ _PyValue_Int32Bits(i) };
}

static inline int32_t
PyValue_InlineInt(PyValue r)
{
    assert(PyValue_IsInlineInt(r));
    return (int32_t)(uint32_t)_PyValue_BITS(r);
}

// The encoding of an inline float, which is also its identity.
static inline uintptr_t
_PyValue_DoubleBits(double d)
{
    assert(!isnan(d));
    uint64_t u;
    memcpy(&u, &d, sizeof(u));
    return u + Py_DOUBLE_OFFSET;
}

static inline PyValue
PyValue_FromDoubleNotNaN(double d)
{
    OBJECT_STAT_INC(inline_floats_created);
    return (PyValue){ _PyValue_DoubleBits(d) };
}

static inline double
PyValue_InlineFloat(PyValue r)
{
    assert(PyValue_IsInlineFloat(r));
    uint64_t u = _PyValue_BITS(r) - Py_DOUBLE_OFFSET;
    double d;
    memcpy(&d, &u, sizeof(d));
    return d;
}

// i is in -1 .. 2**47 - 1.
static inline PyValue
PyValue_TagIndex(intptr_t i)
{
    assert(i >= -1 && i < ((intptr_t)1 << 47));
    return (PyValue){ Py_INDEX_BASE | ((uintptr_t)i & (Py_PTR_LIMIT - 1)) };
}

static inline intptr_t
PyValue_UntagIndex(PyValue r)
{
    assert(PyValue_IsTaggedIndex(r));
    return Py_ARITHMETIC_RIGHT_SHIFT(intptr_t, (intptr_t)(_PyValue_BITS(r) << 16), 16);
}

/* Increments a tagged index, but does not check for overflow */
static inline PyValue
PyValue_IncrementTaggedIndexNoOverflow(PyValue r)
{
    assert(PyValue_UntagIndex(r) >= 0);
    assert(PyValue_UntagIndex(r) < ((intptr_t)1 << 47) - 1);
    _PyValue_BITS(r)++;
    return r;
}

PyAPI_FUNC(PyValue) _PyValue_BoxInlineValue(PyValue ref);

/* If ref holds an inline value, return an owned reference to a new box with
   the same value; return any other ref unchanged. On failure, set an exception
   and return ref unchanged, so the result is still inline. Scalar
   constructors, and the allocator hooks under them, run no Python (the
   release contract below), unless they fail with an exception pending: the
   MemoryError replaces that exception, and releasing it can run Python
   (__del__ of a frame local its traceback held), as an upstream allocation
   failure does.

   Callers pass and assign the value (x = PyValue_Box(x)) rather than its
   address, so that x can stay in a register. */
static inline PyValue
PyValue_Box(PyValue ref)
{
    return PyValue_IsInline(ref) ? _PyValue_BoxInlineValue(ref) : ref;
}

/* Value identity (boil/design/02-tagged-values.md, section 3)

   An exact int that fits in int32, or an exact float other than NaN, may be
   held inline in one place and boxed in others, and a value may be boxed
   more than once. The program must not be able to tell these apart, so such
   a value has a single identity: its inline encoding, _PyValue_Int32Bits()
   or _PyValue_DoubleBits(), whether it is held inline or boxed. Any other
   object's identity is its address. An encoding is at least Py_DOUBLE_OFFSET
   (2**49) and an address is below Py_PTR_LIMIT (2**48), so an identity names
   exactly one value or object.

   `is` (PyValue_Is(), hence IS_OP; operator.is_ and operator.is_not)
   compares identities and id() returns the identity, so id(x) == id(y)
   exactly when x is y, and id() of a value is the same before and after it
   is boxed. 0.0 and -0.0 differ in bits, so `0.0 is -0.0` is False. NaN is
   never inline and keeps object identity. Bools and int subclasses keep
   object identity. Py_Is() stays pointer equality: C code sees only boxes.

   Release contract. Freeing an exact int or float box never runs Python and
   never changes the current exception. Such a box has no finalizer, and
   allocator hooks (PyMem_SetAllocator(), for allocation as well as free) and
   reference tracers (PyRefTracer_SetTracer()) never run Python, never raise
   and never change the current exception. So a box that becomes an inline
   value is freed at once (_PyValue_Canonicalize()), with no exception save
   and restore around the release. Debug builds check the contract around
   every allocator and tracer call (_PyReleaseCheck in pycore_object.h).
   Releasing any other object can run Python (__del__, weakref callbacks,
   dict watchers); its deallocator leaves the current exception as it found
   it, as Objects/object.c checks (gh-89373).
 */

// The identity of op: its value encoding or its address.
PyAPI_FUNC(uintptr_t) _PyObject_ValueIdentity(PyObject *op);

// Can v be held inline as an int? Every test of the inline int range uses
// this, so that the range is defined in one place.
static inline bool
_PyValue_IntFitsInline(int64_t v)
{
    return INT32_MIN <= v && v <= INT32_MAX;
}

// Do a and b, which are different references, hold the same value?
// Neither may be PyValue_NULL.
PyAPI_FUNC(int) _PyValue_IsSameValue(PyValue a, PyValue b);


// Ownership discipline is explicit; no separate debug payload format exists.
#define _Py_value_allow_borrows_outlive_close(ref) ((void)(ref))

/* Wrap a pointer in a stack ref.
 * The resulting stack reference is not safe and should only be used
 * in the interpreter to pass values from one uop to another.
 * The GC should never see one of these stack refs. */
static inline PyValue
PyValue_Wrap(void *ptr)
{
    assert(ptr != NULL);
#ifdef Py_DEBUG
    assert(((uintptr_t)ptr & Py_TAG_BITS) == 0);
    return (PyValue){ .bits = ((uintptr_t)ptr) | Py_TAG_INVALID };
#else
    return (PyValue){ .bits = (uintptr_t)ptr };
#endif
}

static inline void *
PyValue_Unwrap(PyValue ref)
{
#ifdef Py_DEBUG
    assert ((ref.bits & Py_TAG_BITS) == Py_TAG_INVALID);
    return (void *)(ref.bits & ~Py_TAG_BITS);
#else
    return (void *)(ref.bits);
#endif
}

static inline bool
PyValue_IsValid(PyValue ref)
{
    /* NULL is invalid, and so are the bits Py_TAG_INVALID, which no value has */
    return ref.bits > Py_TAG_INVALID;
}

/* References to immortal objects always have their tag bit set to Py_TAG_REFCNT
 * as they can (must) have their reclamation deferred */

#if _Py_IMMORTAL_FLAGS != Py_TAG_REFCNT
#  error "_Py_IMMORTAL_FLAGS != Py_TAG_REFCNT"
#endif

#define BITS_TO_PTR(REF) ((PyObject *)((REF).bits))
#define BITS_TO_PTR_MASKED(REF) ((PyObject *)(((REF).bits) & (~Py_TAG_REFCNT)))

#define PyValue_True ((PyValue){.bits = ((uintptr_t)&_Py_TrueStruct) | Py_TAG_REFCNT })
#define PyValue_False ((PyValue){.bits = ((uintptr_t)&_Py_FalseStruct) | Py_TAG_REFCNT })
#define PyValue_None ((PyValue){.bits = ((uintptr_t)&_Py_NoneStruct) | Py_TAG_REFCNT })

/* Initializer for the canonical value of a statically allocated immortal
   object, e.g. a keyword in a static tuple.  C accepts an address plus a
   constant in a static initializer but not an address OR-ed with one; the
   object is aligned, so the sum equals the tagged bits. */
#define _PyValue_STATIC_IMMORTAL(op) { .bits = (uintptr_t)(op) + Py_TAG_REFCNT }

#define PyValue_IsTrue(REF) ((REF).bits == (((uintptr_t)&_Py_TrueStruct) | Py_TAG_REFCNT))
#define PyValue_IsFalse(REF) ((REF).bits == (((uintptr_t)&_Py_FalseStruct) | Py_TAG_REFCNT))
#define PyValue_IsNone(REF) ((REF).bits == (((uintptr_t)&_Py_NoneStruct) | Py_TAG_REFCNT))

#ifdef Py_DEBUG

static inline void
PyValue_CheckValid(PyValue ref)
{
    if (PyValue_IsNull(ref)) {
        return;
    }
    // The tag bits alone, without an address: never produced.
    assert(ref.bits != Py_TAG_REFCNT && ref.bits != Py_TAG_BITS);
    assert(PyValue_IsPointerSpace(ref));
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

#define PyValue_CheckValid(REF) ((void)0)

#endif

#ifdef Py_DEBUG
/* Canonical form of a value held in a home (a frame slot, container element,
   field, cell, dict key or value, or set key): an exact int that fits inline
   and an exact non-NaN float are never held as pointers, and a pointer
   without the no-count tag is never a static immortal. This only reports:
   it counts each violation by call site and kind, readable through
   _testinternalcapi.pyvalue_home_violations(). Debug builds only.

   _PyValue_CheckHome() checks a store, and the count is of stores.
   _PyValue_RecheckHome() checks a value already stored (the frame check, a
   GC visit), which it may see many times: it counts apart, as a backstop
   for a writer that has no store check. site is "file:line". */
PyAPI_FUNC(void) _PyValue_ReportHome(PyValue ref, const char *site,
                                     int recheck);

static inline bool
_PyValue_IsCanonicalHome(PyValue ref)
{
    if (PyValue_IsNull(ref)) {
        return true;
    }
    if (!PyValue_IsLanguageValue(ref)) {
        return false;
    }
    if (!PyValue_IsObject(ref)) {
        return true;
    }
    PyObject *obj = BITS_TO_PTR_MASKED(ref);
    // A static immortal always carries the no-count tag.
    if ((ref.bits & Py_TAG_BITS) == 0 && _Py_IsStaticImmortal(obj)) {
        return false;
    }
    return _PyObject_ValueIdentity(obj) == (uintptr_t)obj;
}

#define _PyValue_CHECK_HOME(REF, RECHECK) \
    do { \
        PyValue _home_ref = (REF); \
        if (!_PyValue_IsCanonicalHome(_home_ref)) { \
            _PyValue_ReportHome(_home_ref, \
                                __FILE__ ":" Py_STRINGIFY(__LINE__), \
                                (RECHECK)); \
        } \
    } while (0)
#define _PyValue_CheckHome(REF) _PyValue_CHECK_HOME(REF, 0)
#define _PyValue_RecheckHome(REF) _PyValue_CHECK_HOME(REF, 1)
/* A store to a frame temporary, which may also hold an index. */
#define _PyValue_CheckTemp(REF) \
    do { \
        PyValue _temp_ref = (REF); \
        if (!PyValue_IsTaggedIndex(_temp_ref)) { \
            _PyValue_CheckHome(_temp_ref); \
        } \
    } while (0)
#else
#define _PyValue_CheckHome(REF) ((void)0)
#define _PyValue_RecheckHome(REF) ((void)0)
#define _PyValue_CheckTemp(REF) ((void)0)
#endif

// The number of store violations reported (rechecks are not counted), and a
// dict mapping "file:line kind" of each reporting call site and violation
// kind to its count; recheck sites start with "recheck ". Both are empty in
// release builds.
PyAPI_FUNC(Py_ssize_t) _PyValue_HomeViolationCount(void);
PyAPI_FUNC(PyObject *) _PyValue_HomeViolationSites(void);

/* Does this ref own a count on its object? False for PyValue_NULL,
   references to immortal objects, borrowed and deferred references, indices
   and inline values. */
static inline int
PyValue_RefcountOnObject(PyValue ref)
{
    return !PyValue_IsNull(ref) && (ref.bits & Py_OWNED_MASK) == 0;
}

/* PyValue_RefcountOnObject() of a ref that is not PyValue_NULL, which
   needs one test fewer. */
static inline int
_PyValue_OwnsCountNotNull(PyValue ref)
{
    assert(!PyValue_IsNull(ref));
    return (ref.bits & Py_OWNED_MASK) == 0;
}

/* Report a value that holds no object (an inline int or float, or an
   index) passed to PyValue_AsKnownObjectBorrow() or
   PyValue_AsKnownObjectSteal(), and abort. Out of line and cold, so that a
   caller carries one compare and an untaken branch. */
PyAPI_FUNC(void) _Py_NO_RETURN _PyValue_FatalNotObject(PyValue ref);

/* The object that ref holds, borrowed. ref must hold an object or be
   PyValue_NULL. Every build checks this until packet V7 measures the check
   (pyjit/design/value-api.md, section 1.3). */
static inline PyObject *
PyValue_AsKnownObjectBorrow(PyValue ref)
{
    if (!PyValue_IsPointerSpace(ref)) {
        _PyValue_FatalNotObject(ref);
    }
    return BITS_TO_PTR_MASKED(ref);
}

/* Borrow the reference in *ref, typically a local: see PyValue_Borrow().
   Passing the slot's address lets the pointer-space test read it in place. */
static inline PyValue
PyValue_BorrowAt(const PyValue *ref)
{
    return (PyValue){ .bits = ref->bits | _PyValue_BorrowBitAt(ref) };
}

/* The object that ref holds, owned: ref's count, or a new one if ref owns
   none. ref must hold an object, as for PyValue_AsKnownObjectBorrow(). */
static inline PyObject *
PyValue_AsKnownObjectSteal(PyValue ref)
{
    if (!PyValue_IsPointerSpace(ref)) {
        _PyValue_FatalNotObject(ref);
    }
    if (_PyValue_OwnsCountNotNull(ref)) {
        return BITS_TO_PTR(ref);
    }
    else {
        return Py_NewRef(BITS_TO_PTR_MASKED(ref));
    }
}

static inline PyValue
PyValue_FromPyObjectSteal(PyObject *obj)
{
    assert(obj != NULL);
    assert((uintptr_t)obj < Py_PTR_LIMIT);
    unsigned int tag = _Py_IsImmortal(obj) ? Py_TAG_REFCNT : 0;
    PyValue ref = ((PyValue){.bits = ((uintptr_t)(obj)) | tag});
    PyValue_CheckValid(ref);
    return ref;
}

static inline PyValue
PyValue_FromPyObjectStealMortal(PyObject *obj)
{
    assert(obj != NULL);
    assert(!_Py_IsImmortal(obj));
    PyValue ref = (PyValue){ .bits = (uintptr_t)obj };
    PyValue_CheckValid(ref);
    return ref;
}

static inline PyValue
_PyValue_FromPyObjectNew(PyObject *obj)
{
    assert(obj != NULL);
    assert((uintptr_t)obj < Py_PTR_LIMIT);
    if (_Py_IsImmortal(obj)) {
        return (PyValue){ .bits = (uintptr_t)obj | Py_TAG_REFCNT };
    }
    _Py_INCREF_MORTAL(obj);
    PyValue ref = (PyValue){ .bits = (uintptr_t)obj };
    PyValue_CheckValid(ref);
    return ref;
}
#define PyValue_FromPyObjectNew(obj) _PyValue_FromPyObjectNew(_PyObject_CAST(obj))

static inline PyValue
_PyValue_FromPyObjectNewMortal(PyObject *obj)
{
    assert(obj != NULL);
    _Py_INCREF_MORTAL(obj);
    PyValue ref = (PyValue){ .bits = (uintptr_t)obj };
    PyValue_CheckValid(ref);
    return ref;
}
#define PyValue_FromPyObjectNewMortal(obj) _PyValue_FromPyObjectNewMortal(_PyObject_CAST(obj))

/* Create a new reference from an object with an embedded reference count */
static inline PyValue
PyValue_FromPyObjectBorrow(PyObject *obj)
{
    assert(obj != NULL);
    assert((uintptr_t)obj < Py_PTR_LIMIT);
    return (PyValue){ .bits = (uintptr_t)obj | Py_TAG_REFCNT};
}

static inline PyValue
PyValue_DUP(PyValue ref)
{
    if (_PyValue_OwnsCountNotNull(ref)) {
        _Py_INCREF_MORTAL(BITS_TO_PTR(ref));
    }
    return ref;
}

static inline PyValue
PyValue_DupImmortal(PyValue ref)
{
    assert(!PyValue_IsNull(ref));
    assert(!PyValue_RefcountOnObject(ref));
    assert(_Py_IsImmortal(BITS_TO_PTR_MASKED(ref)));
    return ref;
}

static inline bool
PyValue_IsHeapSafe(PyValue ref)
{
    return (ref.bits & Py_TAG_BITS) != Py_TAG_REFCNT ||
           !PyValue_IsPointerSpace(ref) || _Py_IsImmortal(BITS_TO_PTR_MASKED(ref));
}

static inline PyValue
PyValue_MakeHeapSafe(PyValue ref)
{
    if (PyValue_IsHeapSafe(ref)) {
        return ref;
    }
    PyObject *obj = BITS_TO_PTR_MASKED(ref);
    Py_INCREF(obj);
    ref.bits = (uintptr_t)obj;
    PyValue_CheckValid(ref);
    return ref;
}

static inline void
_PyValue_OwnArray(PyValue *values, Py_ssize_t count)
{
    for (Py_ssize_t i = 0; i < count; i++) {
        values[i] = PyValue_MakeHeapSafe(values[i]);
    }
}

// Convert a possibly deferred reference to a strong reference.
static inline PyValue
PyValue_AsStrongReference(PyValue value)
{
    return PyValue_FromPyObjectSteal(PyValue_AsKnownObjectSteal(value));
}

static inline void
PyValue_CLOSE(PyValue ref)
{
    if (_PyValue_OwnsCountNotNull(ref)) {
        Py_DECREF_MORTAL(BITS_TO_PTR(ref));
    }
}

static inline void
PyValue_CLOSE_SPECIALIZED(PyValue ref, destructor destruct)
{
    if (_PyValue_OwnsCountNotNull(ref)) {
        Py_DECREF_MORTAL_SPECIALIZED(BITS_TO_PTR(ref), destruct);
    }
}

static inline void
PyValue_XCLOSE(PyValue ref)
{
    if (PyValue_RefcountOnObject(ref)) {
        Py_DECREF_MORTAL(BITS_TO_PTR(ref));
    }
}

#define PyValue_CLEAR(REF) \
    do { \
        PyValue *_tmp_op_ptr = &(REF); \
        PyValue _tmp_old_op = (*_tmp_op_ptr); \
        *_tmp_op_ptr = PyValue_NULL; \
        PyValue_XCLOSE(_tmp_old_op); \
    } while (0)


// Python's `is`: the same reference, ignoring bit 0 of a pointer, or else
// the same value (see "Value identity" below), which only the out-of-line
// test can establish. Indices compare exactly.
// Note: this is a macro because MSVC (Windows) has trouble inlining it.
#define PyValue_Is(a, b) \
    (((((a).bits ^ (b).bits) & ~(uintptr_t)PyValue_IsPointerSpace(a)) == 0) \
     || _PyValue_IsSameValue((a), (b)))



/* Close, dup or steal a reference known to be in pointer space, which
   includes PyValue_NULL: the cases generator knows this of a value that
   passed a guard, was boxed or was built from an object (see "Inline values"
   in Tools/cases_generator/interpreter_definition.md). There a reference
   other than PyValue_NULL owns a count unless a tag bit is set, so these
   test Py_TAG_BITS alone instead of Py_OWNED_MASK. */
static inline int
_PyValue_PointerOwnsCount(PyValue ref)
{
    assert(PyValue_IsPointerSpace(ref));
    assert(!PyValue_IsNull(ref));
    return (ref.bits & Py_TAG_BITS) == 0;
}

// PyValue_NULL has no tag bits set, so XCLOSE tests for it first.
static inline void
_PyValue_XCLOSE_POINTER(PyValue ref)
{
    if (!PyValue_IsNull(ref) && _PyValue_PointerOwnsCount(ref)) {
        Py_DECREF_MORTAL(BITS_TO_PTR(ref));
    }
}

static inline void
_PyValue_CLOSE_POINTER(PyValue ref)
{
    if (_PyValue_PointerOwnsCount(ref)) {
        Py_DECREF_MORTAL(BITS_TO_PTR(ref));
    }
}

static inline void
_PyValue_CLOSE_SPECIALIZED_POINTER(PyValue ref, destructor destruct)
{
    if (_PyValue_PointerOwnsCount(ref)) {
        Py_DECREF_MORTAL_SPECIALIZED(BITS_TO_PTR(ref), destruct);
    }
}

static inline PyValue
_PyValue_DUP_POINTER(PyValue ref)
{
    if (_PyValue_PointerOwnsCount(ref)) {
        _Py_INCREF_MORTAL(BITS_TO_PTR(ref));
    }
    return ref;
}

static inline PyObject *
_PyValue_AsPyObjectStealPointer(PyValue ref)
{
    if (_PyValue_PointerOwnsCount(ref)) {
        return BITS_TO_PTR(ref);
    }
    return Py_NewRef(BITS_TO_PTR_MASKED(ref));
}

static inline bool
PyValue_IsMalformed(PyValue ref)
{
    return PyValue_IsPointerSpace(ref) && (_PyValue_BITS(ref) & Py_TAG_BITS) == Py_TAG_INVALID;
}

static inline PyTypeObject *
PyValue_TYPE(PyValue value) {
    if (PyValue_IsInline(value)) {
        return PyValue_IsInlineInt(value) ? &PyLong_Type : &PyFloat_Type;
    }
    return Py_TYPE(PyValue_AsKnownObjectBorrow(value));
}

// Converts a PyValue back to a PyObject *, converting the
// value to a new reference.
#define PyValue_AsPyObjectNew(value) Py_NewRef(PyValue_AsKnownObjectBorrow(value))

// Value type checks. Indices and inline values are not objects of these types.

#define VALUE_CHECK_FUNC(T) \
    static inline bool \
    PyValue_ ## T ## Check(PyValue value) { \
        return PyValue_IsPointerSpace(value) && \
               Py ## T ## _Check(PyValue_AsKnownObjectBorrow(value)); \
    }

VALUE_CHECK_FUNC(Gen)
VALUE_CHECK_FUNC(Bool)
VALUE_CHECK_FUNC(ExceptionInstance)
VALUE_CHECK_FUNC(Code)
VALUE_CHECK_FUNC(Function)

static inline bool
PyValue_LongCheck(PyValue value)
{
    return PyValue_IsInlineInt(value) ||
           (PyValue_IsPointerSpace(value) && PyLong_Check(PyValue_AsKnownObjectBorrow(value)));
}

static inline bool
PyValue_FloatCheck(PyValue value)
{
    return PyValue_IsInlineFloat(value) ||
           (PyValue_IsPointerSpace(value) && PyFloat_Check(PyValue_AsKnownObjectBorrow(value)));
}

static inline bool
PyValue_LongCheckExact(PyValue value)
{
    return PyValue_IsInlineInt(value) ||
           (PyValue_IsPointerSpace(value) && PyLong_CheckExact(PyValue_AsKnownObjectBorrow(value)));
}

/* Ints in specialized instructions. The int guards accept an inline int or
   a reference to an exact int with at most one digit; either way the value
   fits in int32. These accessors read both forms without converting the
   value to a PyObject *, so the cases generator does not box their
   argument (see "Inline values" in
   Tools/cases_generator/interpreter_definition.md). */
static inline bool
PyValue_IsCompactInt(PyValue ref)
{
    if (PyValue_IsInlineInt(ref)) {
        return true;
    }
    return PyValue_IsPointerSpace(ref)
           && _PyLong_CheckExactAndCompact(PyValue_AsKnownObjectBorrow(ref));
}

static inline int32_t
PyValue_CompactIntValue(PyValue ref)
{
    assert(PyValue_IsCompactInt(ref));
    if (PyValue_IsInlineInt(ref)) {
        return PyValue_InlineInt(ref);
    }
    return (int32_t)_PyLong_CompactValue((PyLongObject *)PyValue_AsKnownObjectBorrow(ref));
}

// A new int for a result outside int32 (the slow path of int arithmetic),
// or PyValue_NULL with an exception set.
PyAPI_FUNC(PyValue) _PyValue_BoxInt64(int64_t value);

// value << shift for a shift of 32 or more places, which makes a long int,
// or PyValue_NULL with an exception set.
PyAPI_FUNC(PyValue) _PyValue_LshiftInt32(int32_t value, int32_t shift);

/* Floats in specialized instructions. The float guards accept an inline
   float or a reference to an exact float, and PyValue_FloatValue() reads
   either without converting the value to a PyObject *. */
static inline bool
PyValue_FloatCheckExact(PyValue ref)
{
    return PyValue_IsInlineFloat(ref) ||
           (PyValue_IsPointerSpace(ref) && PyFloat_CheckExact(PyValue_AsKnownObjectBorrow(ref)));
}

static inline double
PyValue_FloatValue(PyValue ref)
{
    assert(PyValue_FloatCheckExact(ref));
    if (PyValue_IsInlineFloat(ref)) {
        return PyValue_InlineFloat(ref);
    }
    return PyFloat_AS_DOUBLE(PyValue_AsKnownObjectBorrow(ref));
}

/* Are a and b operands of specialized float arithmetic: two floats, or a
   float and a compact int, which converts to double exactly (see
   PyValue_IsCompactInt())? A NaN next to an int is refused, as the
   int-float forms of BINARY_OP_EXTEND that this replaces refused it; an
   inline float is never NaN, so that costs a test only for a box. */
static inline bool
PyValue_FloatOperandsCheck(PyValue a, PyValue b)
{
    if (PyValue_IsInlineFloat(a) && PyValue_IsInlineFloat(b)) {
        return true;
    }
    if (PyValue_FloatCheckExact(a)) {
        return PyValue_FloatCheckExact(b) ||
               (PyValue_IsCompactInt(b) && !isnan(PyValue_FloatValue(a)));
    }
    return PyValue_IsCompactInt(a) && PyValue_FloatCheckExact(b) &&
           !isnan(PyValue_FloatValue(b));
}

// An operand that passed PyValue_FloatOperandsCheck(), as a double.
static inline double
PyValue_NumberAsDouble(PyValue ref)
{
    if (PyValue_IsInlineFloat(ref)) {
        return PyValue_InlineFloat(ref);
    }
    if (PyValue_IsInlineInt(ref)) {
        return (double)PyValue_InlineInt(ref);
    }
    PyObject *op = PyValue_AsKnownObjectBorrow(ref);
    if (PyFloat_CheckExact(op)) {
        return PyFloat_AS_DOUBLE(op);
    }
    assert(_PyLong_CheckExactAndCompact(op));
    return (double)_PyLong_CompactValue((PyLongObject *)op);
}

/* If PyValue_FloatOperandsCheck(a, b), store a and b as doubles in *da
   and *db and return true. Two inline floats are tested first, which lets
   the compiler give them a path of their own. */
static inline bool
PyValue_FloatOperandValues(PyValue a, PyValue b, double *da, double *db)
{
    if (PyValue_IsInlineFloat(a) && PyValue_IsInlineFloat(b)) {
        *da = PyValue_InlineFloat(a);
        *db = PyValue_InlineFloat(b);
        return true;
    }
    if (!PyValue_FloatOperandsCheck(a, b)) {
        return false;
    }
    *da = PyValue_NumberAsDouble(a);
    *db = PyValue_NumberAsDouble(b);
    return true;
}

// A new float, or PyValue_NULL with an exception set.
PyAPI_FUNC(PyValue) _PyValue_BoxDouble(double value);

/* The result of float arithmetic: inline unless it is NaN, which is boxed
   (see "NaN and signed zero" in boil/design/02-tagged-values.md, section
   3). PyValue_NULL with an exception set if boxing fails. */
static inline PyValue
PyValue_FromDouble(double value)
{
    if (!isnan(value)) {
        PyValue ref = PyValue_FromDoubleNotNaN(value);
        // So that the caller's test for failure folds away on this path.
        if (PyValue_IsNull(ref)) {
            Py_UNREACHABLE();
        }
        return ref;
    }
    return _PyValue_BoxDouble(value);
}

/* A new reference to the object that ref holds, a new box if ref holds an
   inline value, or NULL with an exception set if boxing fails. ref must be a
   language value; debug builds check it (pyjit/design/value-api.md, section
   1.3). An exception may be pending, as for any upstream API that allocates:
   a failed box replaces it. */
static inline PyObject *
PyValue_AsObject(PyValue ref)
{
    assert(PyValue_IsLanguageValue(ref));
    if (PyValue_IsInline(ref)) {
        ref = PyValue_Box(ref);
        return PyValue_IsInline(ref) ? NULL : PyValue_AsKnownObjectSteal(ref);
    }
    return PyValue_AsPyObjectNew(ref);
}

/* For each inline value src[i], store a new box in dst[i]; leave the other
   elements of dst alone. Return 0, or -1 with an exception set and no boxes
   left. For constructors that steal src only on success. */
static inline int
_PyValue_BoxInto(const PyValue *src, PyObject **dst, Py_ssize_t n)
{
    for (Py_ssize_t i = 0; i < n; i++) {
        if (PyValue_IsInline(src[i])) {
            dst[i] = PyValue_AsObject(src[i]);
            if (dst[i] == NULL) {
                while (--i >= 0) {
                    if (PyValue_IsInline(src[i])) {
                        Py_DECREF(dst[i]);
                    }
                }
                return -1;
            }
        }
    }
    return 0;
}

static inline void
_PyThreadState_PushCValue(PyThreadState *tstate, _PyCValue *ref)
{
    ref->ref = PyValue_NULL;
}

static inline void
_PyThreadState_PushCValueNew(PyThreadState *tstate, _PyCValue *ref, PyObject *obj)
{
    _PyThreadState_PushCValue(tstate, ref);
    ref->ref = PyValue_FromPyObjectNew(obj);
}

#  define _PyCValue_CLEAR(REF) ((void)0)

static inline void
_PyThreadState_PopCValue(PyThreadState *tstate, _PyCValue *ref)
{
    PyValue_XCLOSE(ref->ref);
    _PyCValue_CLEAR(ref);
}

static inline PyValue
_PyThreadState_PopCValueSteal(PyThreadState *tstate, _PyCValue *ref)
{
    PyValue res = ref->ref;
    _PyCValue_CLEAR(ref);
    return res;
}

#define PyValue_XSETREF(dst, src) \
    do { \
        PyValue _tmp_dst_ref = (dst); \
        (dst) = (src); \
        PyValue_XCLOSE(_tmp_dst_ref); \
    } while(0)

// Like Py_VISIT but for PyValue fields
#define _Py_VISIT_VALUE(ref)                                         \
    do {                                                                \
        if (PyValue_IsObject(ref)) {                                 \
            int vret = _PyGC_VisitValue(&(ref), visit, arg);         \
            if (vret)                                                   \
                return vret;                                            \
        }                                                               \
    } while (0)

static inline void
_PyValue_CloseStack(PyValue *arguments, int total_args)
{
    // arguments is a pointer into the GC visible stack,
    // so we must NULL out values as we clear them.
    for (int i = total_args-1; i >= 0; i--) {
        PyValue tmp = arguments[i];
        arguments[i] = PyValue_NULL;
        PyValue_CLOSE(tmp);
    }
}

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_VALUE_H */
