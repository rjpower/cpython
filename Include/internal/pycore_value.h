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

/* Can releasing owner deallocate its object? Releasing a scalar, an
 * immortal or an owner that is not the last cannot run Python. */
static inline int
_PyValue_MayDealloc(PyValueBorrow owner)
{
    if (PyValue_IsObject(owner) && (owner.bits & 1) == 0) {
        return Py_REFCNT((PyObject *)owner.bits) == 1;
    }
    return 0;
}
#define _PyValue_MayDealloc(V0) (_PyValue_MayDealloc)(PyValue_Borrow(V0))

PyAPI_FUNC(void) _PyValueTemps_CloseNonempty(_PyValueTemps *temps);
PyAPI_FUNC(PyObject *) _PyValueTemps_Box(_PyValueTemps *temps, PyValueBorrow value);
#define _PyValueTemps_Box(V0, V1) (_PyValueTemps_Box)(V0, PyValue_Borrow(V1))

static inline PyObject *
_PyValueTemps_Object(_PyValueTemps *temps, PyValueBorrow value)
{
    if (PyValue_IsObject(value)) {
        return (PyObject *)(value.bits & ~(uintptr_t)1);
    }
    return _PyValueTemps_Box(temps, value);
}
#define _PyValueTemps_Object(V0, V1) (_PyValueTemps_Object)(V0, PyValue_Borrow(V1))

static inline void
_PyValueTemps_Close(_PyValueTemps *temps)
{
    if (temps != NULL && temps->count != 0) {
        _PyValueTemps_CloseNonempty(temps);
    }
}

/* The canonical form of *source, an owner, for a home.  If *source holds a
 * box whose value is inline, *source becomes that inline value, and the box
 * is released at once if *source owned a count of it. */
static inline Py_ALWAYS_INLINE PyValue
_PyValue_Publish(PyValue *source)
{
    if (PyValue_IsObject(*source)) {
        PyObject *obj = (PyObject *)(source->bits & ~(uintptr_t)1);
        // A static immortal always carries the no-count tag.
        assert((source->bits & 1) || !_Py_IsStaticImmortal(obj));
        if (Py_IS_TYPE(obj, &PyLong_Type) || Py_IS_TYPE(obj, &PyFloat_Type)) {
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

  A PyValue also holds exact ints that fit in int51 and exact non-NaN floats
  without a heap object, and interpreter-internal indices. The bits are laid
  out as follows (pyjit/design/pyvalue.md, section 1.3):

    Top 16 bits     Low 48 bits         Meaning
    0x0000          0x000000000000      NULL
    0x0000          ptr, bit 0 = 0      owned reference to a mortal object
    0x0000          ptr, bit 0 = 1      no count: an immortal object
    0x0000          ptr | 2             PyValue_Wrap() pointer (debug builds;
                                        release builds wrap the bare pointer)
    0x0001          int48               index (Py_INDEX_BASE)
    0x0002-0x7FF2   double + 2^49       inline float, +0.0 to +inf
    0x8002-0xFFF2   double + 2^49       inline float, -0.0 to -inf
    0xFFF8-0xFFFF   low 51 bits of int  inline int51 (Py_INT_BASE)
    other                               unused: images of NaNs

  Values below Py_PTR_LIMIT are pointer space; Py_TAG_REFCNT and
  Py_TAG_INVALID have meaning only there. Indices and inline values own
  nothing, so CLOSE, DUP and Borrow leave them unchanged. NaN results are
  always boxed, because some NaN images overlap the other regions.

  A pointer to an immortal object always has bit 0 set, so each value has
  one encoding, and bit 0 means immortal in every word (H1; during
  finalization, which makes some immortals mortal again, a tagged word may
  outlive its object's immortality). A borrow (PyValueBorrow) holds the
  exact bits of an owner, without a tag of its own; it lives only in C
  locals and parameters, and PyValue_DUP() makes an owner of it for a
  home (pyjit/design/pyvalue.md D4).

  NULL is zero, so zero-filled memory holds NULL values: memory from a
  zeroing allocator needs no further initialization. The bits 1, 2 and 3
  are never produced.

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

static inline bool PyValue_IsNull(PyValueBorrow r) { return r.bits == PyValue_NULL_BITS; }
#define PyValue_IsNull(V0) (PyValue_IsNull)(PyValue_Borrow(V0))

static inline bool PyValue_IsPointerSpace(PyValueBorrow r) { return _PyValue_BITS(r) < Py_PTR_LIMIT; }
#define PyValue_IsPointerSpace(V0) (PyValue_IsPointerSpace)(PyValue_Borrow(V0))
static inline bool PyValue_IsTaggedIndex(PyValueBorrow r) { return _PyValue_BITS(r) - Py_INDEX_BASE < Py_DOUBLE_OFFSET - Py_INDEX_BASE; }
#define PyValue_IsTaggedIndex(V0) (PyValue_IsTaggedIndex)(PyValue_Borrow(V0))
static inline bool PyValue_IsInline(PyValueBorrow r) { return _PyValue_BITS(r) >= Py_DOUBLE_OFFSET; }
#define PyValue_IsInline(V0) (PyValue_IsInline)(PyValue_Borrow(V0))
static inline bool PyValue_IsInlineInt(PyValueBorrow r) { return _PyValue_BITS(r) >= Py_INT_BASE; }
#define PyValue_IsInlineInt(V0) (PyValue_IsInlineInt)(PyValue_Borrow(V0))
static inline bool PyValue_IsInlineFloat(PyValueBorrow r) { return _PyValue_BITS(r) - Py_DOUBLE_OFFSET < Py_INT_BASE - Py_DOUBLE_OFFSET; }
#define PyValue_IsInlineFloat(V0) (PyValue_IsInlineFloat)(PyValue_Borrow(V0))
// Exact: the AND has the top Py_INT_SHIFT bits set only if both words do.
static inline bool PyValue_BothInlineInt(PyValueBorrow a, PyValueBorrow b) { return (_PyValue_BITS(a) & _PyValue_BITS(b)) >= Py_INT_BASE; }
#define PyValue_BothInlineInt(V0, V1) (PyValue_BothInlineInt)(PyValue_Borrow(V0), PyValue_Borrow(V1))

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

// Can v be held inline as an int? Every test of the inline int range uses
// this, so that the range is defined in one place.
static inline bool
_PyValue_IntFitsInline(int64_t v)
{
    return Py_INT_MIN <= v && v <= Py_INT_MAX;
}

// The encoding of an inline int, which is also its identity (see "Value
// identity" below).
static inline uintptr_t
_PyValue_IntBits(int64_t i)
{
    assert(_PyValue_IntFitsInline(i));
    return Py_INT_BASE | ((uintptr_t)i & ~Py_INT_BASE);
}

static inline PyValue
PyValue_FromInlineInt(int64_t i)
{
    OBJECT_STAT_INC(inline_ints_created);
    return (PyValue){ _PyValue_IntBits(i) };
}

/* The value of an inline int times 2**Py_INT_SHIFT: its word shifted left,
   which drops the region bits. Scaled values add, subtract and compare as
   the values do, and multiply by an unscaled value. Such an operation
   overflows int64 exactly when its result leaves the inline range, so one
   overflow test (__builtin_*_overflow, or jo in the JIT) checks the range. */
static inline int64_t
_PyValue_IntScaled(PyValueBorrow r)
{
    assert(PyValue_IsInlineInt(r));
    return (int64_t)(_PyValue_BITS(r) << Py_INT_SHIFT);
}
#define _PyValue_IntScaled(V0) (_PyValue_IntScaled)(PyValue_Borrow(V0))

// The inline int whose scaled value is s, a multiple of 2**Py_INT_SHIFT.
static inline PyValue
_PyValue_FromIntScaled(int64_t s)
{
    assert((s & (((int64_t)1 << Py_INT_SHIFT) - 1)) == 0);
    OBJECT_STAT_INC(inline_ints_created);
    return (PyValue){ ((uint64_t)s >> Py_INT_SHIFT) | Py_INT_BASE };
}

static inline int64_t
PyValue_InlineInt64(PyValueBorrow r)
{
    return Py_ARITHMETIC_RIGHT_SHIFT(int64_t, _PyValue_IntScaled(r), Py_INT_SHIFT);
}
#define PyValue_InlineInt64(V0) (PyValue_InlineInt64)(PyValue_Borrow(V0))

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
PyValue_InlineFloat(PyValueBorrow r)
{
    assert(PyValue_IsInlineFloat(r));
    uint64_t u = _PyValue_BITS(r) - Py_DOUBLE_OFFSET;
    double d;
    memcpy(&d, &u, sizeof(d));
    return d;
}
#define PyValue_InlineFloat(V0) (PyValue_InlineFloat)(PyValue_Borrow(V0))

// i is in -1 .. 2**47 - 1.
static inline PyValue
PyValue_TagIndex(intptr_t i)
{
    assert(i >= -1 && i < ((intptr_t)1 << 47));
    return (PyValue){ Py_INDEX_BASE | ((uintptr_t)i & (Py_PTR_LIMIT - 1)) };
}

static inline intptr_t
PyValue_UntagIndex(PyValueBorrow r)
{
    assert(PyValue_IsTaggedIndex(r));
    return Py_ARITHMETIC_RIGHT_SHIFT(intptr_t, (intptr_t)(_PyValue_BITS(r) << 16), 16);
}
#define PyValue_UntagIndex(V0) (PyValue_UntagIndex)(PyValue_Borrow(V0))

/* Increments a tagged index, but does not check for overflow */
static inline PyValue
PyValue_IncrementTaggedIndexNoOverflow(PyValue r)
{
    assert(PyValue_UntagIndex(r) >= 0);
    assert(PyValue_UntagIndex(r) < ((intptr_t)1 << 47) - 1);
    _PyValue_BITS(r)++;
    return r;
}

PyAPI_FUNC(PyValue) _PyValue_BoxInlineValue(PyValueBorrow ref);
#define _PyValue_BoxInlineValue(V0) (_PyValue_BoxInlineValue)(PyValue_Borrow(V0))

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

   An exact int that fits in int51, or an exact float other than NaN, is
   held inline in every home (frame slot, container element, field, cell,
   dict key or value, set key): the PyValue_FromPyObject*() constructors
   turn such an object into its inline value, and _PyValue_CheckHome()
   asserts it at each store. Only C code holds such a value boxed, in a
   PyObject * or a C local PyValue, and a value may be boxed more than once.
   The program must not be able to tell these apart, so such a value has a
   single identity: its inline encoding, _PyValue_IntBits() or
   _PyValue_DoubleBits(), whether it is held inline or boxed. Any other
   object's identity is its address. An encoding is at least Py_DOUBLE_OFFSET
   (2**49) and an address is below Py_PTR_LIMIT (2**48), so an identity names
   exactly one value or object.

   `is` on two values (PyValue_Is(), hence IS_OP) compares their bits: a
   value in a home has one encoding, which is its identity or its address.
   operator.is_ and operator.is_not, which see objects and so may see boxes,
   compare identities, and id() returns the identity, so id(x) == id(y)
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
PyValue_IsValid(PyValueBorrow ref)
{
    /* NULL is invalid, and so are the bits Py_TAG_INVALID, which no value has */
    return ref.bits > Py_TAG_INVALID;
}
#define PyValue_IsValid(V0) (PyValue_IsValid)(PyValue_Borrow(V0))

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
#define PyValue_NotImplemented ((PyValue){.bits = ((uintptr_t)&_Py_NotImplementedStruct) | Py_TAG_REFCNT })

/* Initializer for the canonical value of a statically allocated immortal
   object, e.g. a keyword in a static tuple.  C accepts an address plus a
   constant in a static initializer but not an address OR-ed with one; the
   object is aligned, so the sum equals the tagged bits. */
#define _PyValue_STATIC_IMMORTAL(op) { .bits = (uintptr_t)(op) + Py_TAG_REFCNT }

static inline bool
PyValue_IsTrue(PyValueBorrow ref)
{
    return ref.bits == (((uintptr_t)&_Py_TrueStruct) | Py_TAG_REFCNT);
}
#define PyValue_IsTrue(V0) (PyValue_IsTrue)(PyValue_Borrow(V0))

static inline bool
PyValue_IsFalse(PyValueBorrow ref)
{
    return ref.bits == (((uintptr_t)&_Py_FalseStruct) | Py_TAG_REFCNT);
}
#define PyValue_IsFalse(V0) (PyValue_IsFalse)(PyValue_Borrow(V0))

static inline bool
PyValue_IsNone(PyValueBorrow ref)
{
    return ref.bits == (((uintptr_t)&_Py_NoneStruct) | Py_TAG_REFCNT);
}
#define PyValue_IsNone(V0) (PyValue_IsNone)(PyValue_Borrow(V0))

/* Whether this thread's interpreter has begun clearing at finalization
   (finalize_interp_clear(), PyInterpreterState_Clear()), which makes some
   immortal objects mortal again: interned strings, the bases and MROs of
   static types, the common constants. A word tagged before then may now
   point to a mortal object; H1 below holds until then. */
PyAPI_FUNC(int) _PyValue_ClearingImmortals(void);

#ifdef Py_DEBUG

static inline void
PyValue_CheckValid(PyValueBorrow ref)
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
            // H1: tagged implies immortal (see _PyValue_IsHome()).
            assert(_Py_IsImmortal(obj) || _PyValue_ClearingImmortals());
            break;
        default:
            assert(0);
    }
}
#define PyValue_CheckValid(V0) (PyValue_CheckValid)(PyValue_Borrow(V0))

#else

/* No check, but the argument must still be a value. */
#define PyValue_CheckValid(V0) ((void)sizeof(PyValue_Borrow(V0)))

#endif

/* The home check: the form of a value held in a home (a frame slot,
   container element, field, cell, dict key or value, or set key).

   - It is canonical: an exact int that fits inline and an exact non-NaN
     float are never held as pointers.
   - A pointer without the no-count tag is never a static immortal (H2).
   - A pointer with the no-count tag is an immortal (H1), unless the
     interpreter has begun clearing its immortals. H1 holds in every word,
     of either type: a borrow carries its owner's exact bits (B1), so no
     word is tagged on a mortal object (pyjit/design/pyvalue.md D4).

   The predicate exists in every build; the checks below report only in
   debug builds. */
static inline bool
_PyValue_IsHome(PyValueBorrow ref)
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
    if ((ref.bits & Py_TAG_BITS) == 0) {
        if (_Py_IsStaticImmortal(obj)) {
            return false;
        }
    }
    else if (!_Py_IsImmortal(obj) && !_PyValue_ClearingImmortals()) {
        return false;
    }
    return _PyObject_ValueIdentity(obj) == (uintptr_t)obj;
}
#define _PyValue_IsHome(V0) (_PyValue_IsHome)(PyValue_Borrow(V0))

#ifdef Py_DEBUG
/* A violation of the home check is a fatal error naming the call site and
   kind. With _PYTHON_VALUE_HOME_REPORT set in the environment (read once,
   at the first violation; -E and -I ignore it) it is only counted, by call
   site and kind, readable through
   _testinternalcapi.pyvalue_home_violations() and
   pyvalue_home_violation_sites(): a census of the writers to fix.

   _PyValue_CheckHome() checks a store, and the count is of stores.
   _PyValue_RecheckHome() checks a value already stored (the frame check, a
   GC visit), which it may see many times: it counts apart, as a backstop
   for a writer that has no store check. site is "file:line". */
PyAPI_FUNC(void) _PyValue_ReportHome(PyValueBorrow ref, const char *site,
                                     int recheck);

#define _PyValue_CHECK_HOME(REF, RECHECK) \
    do { \
        PyValueBorrow _home_ref = PyValue_Borrow(REF); \
        if (!_PyValue_IsHome(_home_ref)) { \
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
        PyValueBorrow _temp_ref = PyValue_Borrow(REF); \
        if (!PyValue_IsTaggedIndex(_temp_ref)) { \
            _PyValue_CheckHome(_temp_ref); \
        } \
    } while (0)
/* The N temporaries at VALUES, a window that a handler consumes: they own
 * their values, so the handler need not take counts of them. */
#define _PyValue_RecheckWindow(VALUES, N) \
    do { \
        PyValue *_window_refs = (VALUES); \
        for (Py_ssize_t _i = 0; _i < (Py_ssize_t)(N); _i++) { \
            _PyValue_RecheckHome(_window_refs[_i]); \
        } \
    } while (0)
#else
#define _PyValue_CheckHome(REF) ((void)0)
#define _PyValue_RecheckHome(REF) ((void)0)
#define _PyValue_CheckTemp(REF) ((void)0)
#define _PyValue_RecheckWindow(VALUES, N) ((void)0)
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
PyValue_RefcountOnObject(PyValueBorrow ref)
{
    return !PyValue_IsNull(ref) && (ref.bits & Py_OWNED_MASK) == 0;
}
#define PyValue_RefcountOnObject(V0) (PyValue_RefcountOnObject)(PyValue_Borrow(V0))

/* PyValue_RefcountOnObject() of a ref that is not PyValue_NULL, which
   needs one test fewer. */
static inline int
_PyValue_OwnsCountNotNull(PyValueBorrow ref)
{
    assert(!PyValue_IsNull(ref));
    return (ref.bits & Py_OWNED_MASK) == 0;
}
#define _PyValue_OwnsCountNotNull(V0) (_PyValue_OwnsCountNotNull)(PyValue_Borrow(V0))

/* Report a value that holds no object (an inline int or float, or an
   index) passed to PyValue_AsKnownObjectBorrow() or
   PyValue_AsKnownObjectSteal(), and abort. Out of line and cold, so that a
   caller carries one compare and an untaken branch. */
PyAPI_FUNC(void) _Py_NO_RETURN _PyValue_FatalNotObject(PyValueBorrow ref);
#define _PyValue_FatalNotObject(V0) (_PyValue_FatalNotObject)(PyValue_Borrow(V0))

/* The object that ref holds, borrowed. ref must hold an object or be
   PyValue_NULL. Every build checks this until packet V7 measures the check
   (pyjit/design/value-api.md, section 1.3). */
static inline PyObject *
PyValue_AsKnownObjectBorrow(PyValueBorrow ref)
{
    if (!PyValue_IsPointerSpace(ref)) {
        _PyValue_FatalNotObject(ref);
    }
    return BITS_TO_PTR_MASKED(ref);
}
#define PyValue_AsKnownObjectBorrow(V0) (PyValue_AsKnownObjectBorrow)(PyValue_Borrow(V0))

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

/* The canonical constructors (FromPyObjectSteal, FromPyObjectNew,
   FromObjectBorrow) make an exact int that fits inline, or an exact float
   other than NaN, an inline value: Steal frees the box at once (the release
   contract under "Value identity"), New and Borrow leave it to its owner.
   Any other object becomes a pointer, with the no-count tag if it is
   immortal. Code that goes on to use the object keeps its PyObject * in a C
   local: the value may hold no object. */

/* The pointer value of obj, owning what obj owned, never inline. Only for
   boxes, which are pointers by definition (PyValue_Box()), and for the
   canonical constructors themselves: a home never receives a box. */
static inline PyValue
_PyValue_PointerSteal(PyObject *obj)
{
    assert(obj != NULL);
    assert((uintptr_t)obj < Py_PTR_LIMIT);
    unsigned int tag = _Py_IsImmortal(obj) ? Py_TAG_REFCNT : 0;
    PyValue ref = ((PyValue){.bits = ((uintptr_t)(obj)) | tag});
    PyValue_CheckValid(ref);
    return ref;
}

// Out of line: the canonical value of an exact int or float, consumed.
PyAPI_FUNC(PyValue) _PyValue_FromNumberSteal(PyObject *obj);

static inline bool
_PyValue_IsNumberType(PyTypeObject *type)
{
    return type == &PyLong_Type || type == &PyFloat_Type;
}

/* Python's `is` on two objects, either of which may be NULL: the same
   address, or boxes of one inline value (see "Value identity"). C code that
   tests identity on Python's behalf (operator.is_, object.__eq__, method
   equality) uses this; Py_Is() stays pointer equality. */
static inline int
_PyObject_IsSame(PyObject *a, PyObject *b)
{
    if (a == b) {
        return 1;
    }
    return a != NULL && b != NULL && _PyValue_IsNumberType(Py_TYPE(a))
           && _PyObject_ValueIdentity(a) == _PyObject_ValueIdentity(b);
}

#ifdef Py_STATS
// The current thread's box-site class, or fallback if none is set
// (Include/internal/pycore_value_api.h).
PyAPI_FUNC(int) _PyValue_BoxSiteOr(int fallback);
#endif

/* The borrowed canonical value of obj, which its owner keeps alive: the
   exact bits that PyValue_FromPyObjectNew() would give it, with no count
   taken (B1). NULL gives PyValue_NULL. PyValue_DUP() makes an owner of it.
   Only an exact int or float calls out of line. */
static inline PyValueBorrow
PyValue_FromObjectBorrow(PyObject *obj)
{
    PyValueBorrow value = {(uintptr_t)obj};
    if (obj == NULL) {
        return value;
    }
    assert((uintptr_t)obj < Py_PTR_LIMIT);
    if (_PyValue_IsNumberType(Py_TYPE(obj))) {
        uintptr_t identity = _PyObject_ValueIdentity(obj);
        if (identity >= Py_DOUBLE_OFFSET) {
#ifdef Py_STATS
            _Py_STATS_EXPR(object_stats.entry_unbox[
                _PyValue_BoxSiteOr(BOX_SITE_UNATTRIBUTED)]++);
#endif
            value.bits = identity;
            return value;
        }
    }
    if (_Py_IsImmortal(obj)) {
        value.bits |= Py_TAG_REFCNT;
    }
    return value;
}

static inline PyValue
PyValue_FromPyObjectSteal(PyObject *obj)
{
    assert(obj != NULL);
    if (_PyValue_IsNumberType(Py_TYPE(obj))) {
        return _PyValue_FromNumberSteal(obj);
    }
    return _PyValue_PointerSteal(obj);
}

static inline PyValue
_PyValue_FromPyObjectNew(PyObject *obj)
{
    assert(obj != NULL);
    assert((uintptr_t)obj < Py_PTR_LIMIT);
    if (_PyValue_IsNumberType(Py_TYPE(obj))) {
        PyValueBorrow value = PyValue_FromObjectBorrow(obj);
        if (PyValue_IsInline(value)) {
            // An inline value owns nothing: the borrow's word is an owner.
            PyValue owned = {value.bits};
            return owned;
        }
    }
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
PyValue_DUP(PyValueBorrow ref)
{
    if (_PyValue_OwnsCountNotNull(ref)) {
        _Py_INCREF_MORTAL(BITS_TO_PTR(ref));
    }
    return (PyValue){ref.bits};
}
#define PyValue_DUP(V0) (PyValue_DUP)(PyValue_Borrow(V0))

/* PyValue_DUP() of a value that may be PyValue_NULL. */
static inline PyValue
PyValue_XDUP(PyValueBorrow ref)
{
    if (PyValue_RefcountOnObject(ref)) {
        _Py_INCREF_MORTAL(BITS_TO_PTR(ref));
    }
    return (PyValue){ref.bits};
}
#define PyValue_XDUP(V0) (PyValue_XDUP)(PyValue_Borrow(V0))

static inline PyValue
PyValue_DupImmortal(PyValue ref)
{
    assert(!PyValue_IsNull(ref));
    assert(!PyValue_RefcountOnObject(ref));
    assert(_Py_IsImmortal(BITS_TO_PTR_MASKED(ref)));
    return ref;
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

static inline void
PyValue_XCLOSE_SPECIALIZED(PyValue ref, destructor destruct)
{
    if (PyValue_RefcountOnObject(ref)) {
        Py_DECREF_MORTAL_SPECIALIZED(BITS_TO_PTR(ref), destruct);
    }
}

#define PyValue_CLEAR(REF) \
    do { \
        PyValue *_tmp_op_ptr = &(REF); \
        PyValue _tmp_old_op = (*_tmp_op_ptr); \
        *_tmp_op_ptr = PyValue_NULL; \
        PyValue_XCLOSE(_tmp_old_op); \
    } while (0)


// Python's `is` on values held in homes, or built by the canonical
// constructors: the same bits, ignoring bit 0 (the no-count tag) of a
// pointer. Such a value has one encoding (see "Value identity" above); a
// box held in a C local is not one. Indices compare exactly.
static inline bool
PyValue_Is(PyValueBorrow a, PyValueBorrow b)
{
    return ((a.bits ^ b.bits) & ~(uintptr_t)PyValue_IsPointerSpace(a)) == 0;
}
#define PyValue_Is(V0, V1) (PyValue_Is)(PyValue_Borrow(V0), PyValue_Borrow(V1))

/* The word itself, for the few places outside the value headers that need
   it (R26, pyjit/design/value-api.md section 1.6). Everything else reads a
   value through the predicates and accessors above. */

// The same word, tag included: a probe's fast test that a stored key is
// the one it looks up, before it compares objects.
static inline bool
_PyValue_IsSameWord(PyValueBorrow a, PyValueBorrow b)
{
    return a.bits == b.bits;
}
#define _PyValue_IsSameWord(V0, V1) \
    (_PyValue_IsSameWord)(PyValue_Borrow(V0), PyValue_Borrow(V1))

// The word as an integer: for a debug dump, or a key made of a constant's
// identity.
static inline uintptr_t
_PyValue_Word(PyValueBorrow value)
{
    return value.bits;
}
#define _PyValue_Word(V0) (_PyValue_Word)(PyValue_Borrow(V0))

/* The value of OP, a static immortal object that C code uses as a sentinel
   in a home, such as the set's dummy key: what PyValue_FromPyObjectNew()
   gives it, as a constant expression. It owns no count and never reaches
   Python. */
#define _PyValue_SENTINEL(OP) \
    ((PyValue){ .bits = (uintptr_t)(OP) | Py_TAG_REFCNT })



/* Close, dup or steal a reference known to be in pointer space, which
   includes PyValue_NULL: the cases generator knows this of a value that
   passed a guard, was boxed or was built from an object (see "Inline values"
   in Tools/cases_generator/interpreter_definition.md). There a reference
   other than PyValue_NULL owns a count unless a tag bit is set, so these
   test Py_TAG_BITS alone instead of Py_OWNED_MASK. */
static inline int
_PyValue_PointerOwnsCount(PyValueBorrow ref)
{
    assert(PyValue_IsPointerSpace(ref));
    assert(!PyValue_IsNull(ref));
    return (ref.bits & Py_TAG_BITS) == 0;
}
#define _PyValue_PointerOwnsCount(V0) (_PyValue_PointerOwnsCount)(PyValue_Borrow(V0))

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

static inline void
_PyValue_XCLOSE_SPECIALIZED_POINTER(PyValue ref, destructor destruct)
{
    if (!PyValue_IsNull(ref) && _PyValue_PointerOwnsCount(ref)) {
        Py_DECREF_MORTAL_SPECIALIZED(BITS_TO_PTR(ref), destruct);
    }
}

static inline PyValue
_PyValue_DUP_POINTER(PyValueBorrow ref)
{
    if (_PyValue_PointerOwnsCount(ref)) {
        _Py_INCREF_MORTAL(BITS_TO_PTR(ref));
    }
    return (PyValue){ref.bits};
}
#define _PyValue_DUP_POINTER(V0) (_PyValue_DUP_POINTER)(PyValue_Borrow(V0))

static inline PyObject *
_PyValue_AsPyObjectStealPointer(PyValue ref)
{
    if (_PyValue_PointerOwnsCount(ref)) {
        return BITS_TO_PTR(ref);
    }
    return Py_NewRef(BITS_TO_PTR_MASKED(ref));
}

static inline bool
PyValue_IsMalformed(PyValueBorrow ref)
{
    return PyValue_IsPointerSpace(ref) && (_PyValue_BITS(ref) & Py_TAG_BITS) == Py_TAG_INVALID;
}
#define PyValue_IsMalformed(V0) (PyValue_IsMalformed)(PyValue_Borrow(V0))

static inline PyTypeObject *
PyValue_TYPE(PyValueBorrow value) {
    if (PyValue_IsInline(value)) {
        return PyValue_IsInlineInt(value) ? &PyLong_Type : &PyFloat_Type;
    }
    return Py_TYPE(PyValue_AsKnownObjectBorrow(value));
}
#define PyValue_TYPE(V0) (PyValue_TYPE)(PyValue_Borrow(V0))

// Converts a PyValue back to a PyObject *, converting the
// value to a new reference.
#define PyValue_AsPyObjectNew(value) Py_NewRef(PyValue_AsKnownObjectBorrow(value))

// Value type checks. Indices and inline values are not objects of these types.

#define VALUE_CHECK_FUNC(T) \
    static inline bool \
    PyValue_ ## T ## Check(PyValueBorrow value) { \
        return PyValue_IsPointerSpace(value) && \
               Py ## T ## _Check(PyValue_AsKnownObjectBorrow(value)); \
    }

VALUE_CHECK_FUNC(Gen)
#define PyValue_GenCheck(V0) (PyValue_GenCheck)(PyValue_Borrow(V0))
VALUE_CHECK_FUNC(Bool)
#define PyValue_BoolCheck(V0) (PyValue_BoolCheck)(PyValue_Borrow(V0))
VALUE_CHECK_FUNC(ExceptionInstance)
#define PyValue_ExceptionInstanceCheck(V0) \
    (PyValue_ExceptionInstanceCheck)(PyValue_Borrow(V0))
VALUE_CHECK_FUNC(Code)
#define PyValue_CodeCheck(V0) (PyValue_CodeCheck)(PyValue_Borrow(V0))
VALUE_CHECK_FUNC(Function)
#define PyValue_FunctionCheck(V0) (PyValue_FunctionCheck)(PyValue_Borrow(V0))

static inline bool
PyValue_LongCheck(PyValueBorrow value)
{
    return PyValue_IsInlineInt(value) ||
           (PyValue_IsPointerSpace(value) && PyLong_Check(PyValue_AsKnownObjectBorrow(value)));
}
#define PyValue_LongCheck(V0) (PyValue_LongCheck)(PyValue_Borrow(V0))

static inline bool
PyValue_FloatCheck(PyValueBorrow value)
{
    return PyValue_IsInlineFloat(value) ||
           (PyValue_IsPointerSpace(value) && PyFloat_Check(PyValue_AsKnownObjectBorrow(value)));
}
#define PyValue_FloatCheck(V0) (PyValue_FloatCheck)(PyValue_Borrow(V0))

static inline bool
PyValue_LongCheckExact(PyValueBorrow value)
{
    return PyValue_IsInlineInt(value) ||
           (PyValue_IsPointerSpace(value) && PyLong_CheckExact(PyValue_AsKnownObjectBorrow(value)));
}
#define PyValue_LongCheckExact(V0) (PyValue_LongCheckExact)(PyValue_Borrow(V0))

/* Numbers in specialized instructions. The int and float guards accept
   inline values only (PyValue_IsInlineInt(), PyValue_IsInlineFloat()):
   canonical construction makes every exact int in the inline range and
   every exact float but NaN inline, so a boxed operand is a larger int or a
   NaN, and the specialized instruction exits for it. The accessors
   (PyValue_InlineInt64(), PyValue_InlineFloat()) are not conversions to a
   PyObject *, so the cases generator does not box their argument (see
   "Inline values" in Tools/cases_generator/interpreter_definition.md). */

// A new int for a value outside the inline range, or PyValue_NULL with an
// exception set. The slow path of PyValue_FromInt64() and of int arithmetic.
PyAPI_FUNC(PyValue) _PyValue_BoxInt64(int64_t value);

// The canonical value of an int64: inline if it fits, else a new int, or
// PyValue_NULL with an exception set (FOR_ITER_RANGE, len()).
static inline PyValue
PyValue_FromInt64(int64_t value)
{
    if (_PyValue_IntFitsInline(value)) {
        return PyValue_FromInlineInt(value);
    }
    return _PyValue_BoxInt64(value);
}

// a * b for inline ints a and b whose product leaves the inline range, as
// a new int, or PyValue_NULL with an exception set.
PyAPI_FUNC(PyValue) _PyValue_MultiplyInt64(int64_t a, int64_t b);

// value << shift, for a nonzero value and a shift that leaves int64, or
// PyValue_NULL with an exception set.
PyAPI_FUNC(PyValue) _PyValue_LshiftInt64(int64_t value, int64_t shift);

/* Are a and b operands of specialized float arithmetic: two inline floats,
   or an inline float and an inline int? Every int51 converts to a double
   exactly, and an inline float is never NaN. */
static inline bool
PyValue_FloatOperandsCheck(PyValueBorrow a, PyValueBorrow b)
{
    if (PyValue_IsInlineFloat(a)) {
        return PyValue_IsInlineFloat(b) || PyValue_IsInlineInt(b);
    }
    return PyValue_IsInlineInt(a) && PyValue_IsInlineFloat(b);
}
#define PyValue_FloatOperandsCheck(V0, V1) (PyValue_FloatOperandsCheck)(PyValue_Borrow(V0), PyValue_Borrow(V1))

// An operand that passed PyValue_FloatOperandsCheck(), as a double.
static inline double
PyValue_NumberAsDouble(PyValueBorrow ref)
{
    if (PyValue_IsInlineFloat(ref)) {
        return PyValue_InlineFloat(ref);
    }
    return (double)PyValue_InlineInt64(ref);
}
#define PyValue_NumberAsDouble(V0) (PyValue_NumberAsDouble)(PyValue_Borrow(V0))

/* If PyValue_FloatOperandsCheck(a, b), store a and b as doubles in *da
   and *db and return true. Two inline floats are tested first, which lets
   the compiler give them a path of their own. */
static inline bool
PyValue_FloatOperandValues(PyValueBorrow a, PyValueBorrow b, double *da, double *db)
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
#define PyValue_FloatOperandValues(V0, V1, V2, V3) (PyValue_FloatOperandValues)(PyValue_Borrow(V0), PyValue_Borrow(V1), V2, V3)

// A new float for a NaN, the only float with no inline form, or
// PyValue_NULL with an exception set.
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
PyValue_AsObject(PyValueBorrow ref)
{
    assert(PyValue_IsLanguageValue(ref));
    if (PyValue_IsInline(ref)) {
        PyValue box = _PyValue_BoxInlineValue(ref);
        return PyValue_IsInline(box) ? NULL : PyValue_AsKnownObjectSteal(box);
    }
    return PyValue_AsPyObjectNew(ref);
}
#define PyValue_AsObject(V0) (PyValue_AsObject)(PyValue_Borrow(V0))

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
