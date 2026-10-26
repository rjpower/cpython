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
  _PyStackRef is the value type of the interpreter: a frame slot, an
  argument array element, a value passed between handlers.  Nothing is
  reference counted (the tracing collector owns memory, note 06 section 3.4),
  so a _PyStackRef owns nothing and copying one is all that "taking" or
  "giving up" a value means.  PyStackRef_FromPyObject() and
  PyStackRef_AsPyObjectBorrow() convert between a heap object and a
  reference to it; neither changes anything about the object.

  Encoding (boil/design/02-tagged-values.md, section 2.1)

    Top 16 bits     Low 48 bits         Meaning
    0x0000          0                   NULL
    0x0000          2                   ERROR
    0x0000          ptr                 heap object; low 3 bits zero
    0x0000          ptr | 2             PyStackRef_Wrap() pointer (debug builds)
    0x0001          int48               index (Py_INDEX_BASE)
    0x0002-0x7FF2   double + 2^49       inline float, +0.0 to +inf
    0x8002-0xFFF2   double + 2^49       inline float, -0.0 to -inf
    0xFFFE          0x0000 then int32   inline int (Py_INT_BASE)
    other                               unused: images of NaNs, and 0xFFFF

  Values below Py_PTR_LIMIT are pointer space. NaN results are always
  boxed, because some NaN images overlap the other regions.

  An index is the position of a virtual iterator in the FOR_ITER family
  (null_or_index) or the saved lasti during exception unwinding. It never
  reaches Python code and must not be converted to a PyObject *.

  An inline value must be boxed with PyStackRef_Box() before it is
  converted to a PyObject *. The cases generator does this for instruction
  inputs; see "Inline values" in
  Tools/cases_generator/interpreter_definition.md.
*/

/* An object address has these bits clear; ERROR and the debug build's
   wrapped pointers set Py_TAG_INVALID. */
#define Py_TAG_INVALID 2
#define Py_TAG_BITS 7

#  define _PyStackRef_BITS(REF) ((REF).bits)

#if SIZEOF_VOID_P != 8
#  error "_PyStackRef needs 64-bit words"
#endif

#define Py_STACKREF_INLINE_VALUES 1

#define Py_PTR_LIMIT     ((uintptr_t)1 << 48)
#define Py_INDEX_BASE    ((uintptr_t)1 << 48)
#define Py_DOUBLE_OFFSET ((uintptr_t)1 << 49)
#define Py_INT_BASE      ((uintptr_t)0xFFFE << 48)

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
    OBJECT_STAT_INC(inline_floats_created);
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
   object identity. Py_Is() stays pointer equality: C code sees only boxes. */

// The identity of op: its value encoding or its address.
PyAPI_FUNC(uintptr_t) _PyObject_ValueIdentity(PyObject *op);

#ifdef Py_STACKREF_INLINE_VALUES
// Do a and b, which are different references, hold the same value?
// Neither may be PyStackRef_NULL or PyStackRef_ERROR.
PyAPI_FUNC(int) _PyStackRef_IsSameValue(_PyStackRef a, _PyStackRef b);
#endif


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

#define PyStackRef_NULL_BITS 0
static const _PyStackRef PyStackRef_NULL = { .bits = PyStackRef_NULL_BITS };

#define PyStackRef_IsNull(ref) ((ref).bits == PyStackRef_NULL_BITS)
#define PyStackRef_True ((_PyStackRef){.bits = (uintptr_t)&_Py_TrueStruct })
#define PyStackRef_False ((_PyStackRef){.bits = (uintptr_t)&_Py_FalseStruct })
#define PyStackRef_None ((_PyStackRef){.bits = (uintptr_t)&_Py_NoneStruct })

#define PyStackRef_IsTrue(REF) ((REF).bits == (uintptr_t)&_Py_TrueStruct)
#define PyStackRef_IsFalse(REF) ((REF).bits == (uintptr_t)&_Py_FalseStruct)
#define PyStackRef_IsNone(REF) ((REF).bits == (uintptr_t)&_Py_NoneStruct)

/* The object that ref refers to.  ref must be in pointer space: box an
   inline value first. */
static inline PyObject *
PyStackRef_AsPyObjectBorrow(_PyStackRef ref)
{
    assert(PyStackRef_IsPointerSpace(ref));
    return (PyObject *)ref.bits;
}

/* A reference to obj, which must not be NULL. */
static inline _PyStackRef
_PyStackRef_FromPyObject(PyObject *obj)
{
    assert(obj != NULL);
    assert(((uintptr_t)obj & Py_TAG_BITS) == 0);
    return (_PyStackRef){ .bits = (uintptr_t)obj };
}
#define PyStackRef_FromPyObject(obj) _PyStackRef_FromPyObject(_PyObject_CAST(obj))

/* The names below come from reference counting, where they took, gave or
   copied a count.  Each is now a plain copy. */
#define PyStackRef_FromPyObjectSteal PyStackRef_FromPyObject
#define PyStackRef_FromPyObjectStealMortal PyStackRef_FromPyObject
#define PyStackRef_FromPyObjectNew PyStackRef_FromPyObject
#define PyStackRef_FromPyObjectNewMortal PyStackRef_FromPyObject
#define PyStackRef_FromPyObjectBorrow PyStackRef_FromPyObject
#define PyStackRef_AsPyObjectSteal PyStackRef_AsPyObjectBorrow
#define _PyStackRef_AsPyObjectStealPointer PyStackRef_AsPyObjectBorrow
#define PyStackRef_AsPyObjectNew PyStackRef_AsPyObjectBorrow
#define PyStackRef_Borrow(REF) (REF)
#define PyStackRef_BorrowAt(REF) (*(REF))
#define PyStackRef_DUP(REF) (REF)
#define _PyStackRef_DUP_POINTER(REF) (REF)
#define PyStackRef_DupImmortal(REF) (REF)
#define PyStackRef_MakeHeapSafe(REF) (REF)
#define PyStackRef_AsStrongReference(REF) (REF)
#define PyStackRef_CLOSE(REF) ((void)(REF))
#define PyStackRef_XCLOSE(REF) ((void)(REF))
#define PyStackRef_CLOSE_SPECIALIZED(REF, DESTRUCT) ((void)(REF))
#define _PyStackRef_CLOSE_POINTER(REF) ((void)(REF))
#define _PyStackRef_XCLOSE_POINTER(REF) ((void)(REF))
#define _PyStackRef_CLOSE_SPECIALIZED_POINTER(REF, DESTRUCT) ((void)(REF))
#define PyStackRef_CLEAR(REF) ((REF) = PyStackRef_NULL)
#define _Py_stackref_allow_borrows_outlive_close(ref) ((void)(ref))

// Python's `is`: the same encoding, or else the same value (see "Value
// identity" above), which only the out-of-line test can establish.
// Note: this is a macro because MSVC (Windows) has trouble inlining it.
#define PyStackRef_Is(a, b) \
    (((a).bits == (b).bits) || _PyStackRef_IsSameValue((a), (b)))

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

// value << shift for a shift of 32 or more places, which makes a long int,
// or PyStackRef_NULL with an exception set.
PyAPI_FUNC(_PyStackRef) _PyStackRef_LshiftInt32(int32_t value, int32_t shift);

/* Floats in specialized instructions. The float guards accept an inline
   float or a reference to an exact float, and PyStackRef_FloatValue() reads
   either without converting the stackref to a PyObject *. */
static inline bool
PyStackRef_FloatCheckExact(_PyStackRef ref)
{
    return PyStackRef_IsInlineFloat(ref) ||
           (PyStackRef_IsPointerSpace(ref) && PyFloat_CheckExact(PyStackRef_AsPyObjectBorrow(ref)));
}

static inline double
PyStackRef_FloatValue(_PyStackRef ref)
{
    assert(PyStackRef_FloatCheckExact(ref));
    if (PyStackRef_IsInlineFloat(ref)) {
        return PyStackRef_InlineFloat(ref);
    }
    return PyFloat_AS_DOUBLE(PyStackRef_AsPyObjectBorrow(ref));
}

/* Are a and b operands of specialized float arithmetic: two floats, or a
   float and a compact int, which converts to double exactly (see
   PyStackRef_IsCompactInt())? A NaN next to an int is refused, as the
   int-float forms of BINARY_OP_EXTEND that this replaces refused it; an
   inline float is never NaN, so that costs a test only for a box. */
static inline bool
PyStackRef_FloatOperandsCheck(_PyStackRef a, _PyStackRef b)
{
    if (PyStackRef_IsInlineFloat(a) && PyStackRef_IsInlineFloat(b)) {
        return true;
    }
    if (PyStackRef_FloatCheckExact(a)) {
        return PyStackRef_FloatCheckExact(b) ||
               (PyStackRef_IsCompactInt(b) && !isnan(PyStackRef_FloatValue(a)));
    }
    return PyStackRef_IsCompactInt(a) && PyStackRef_FloatCheckExact(b) &&
           !isnan(PyStackRef_FloatValue(b));
}

// An operand that passed PyStackRef_FloatOperandsCheck(), as a double.
static inline double
PyStackRef_NumberAsDouble(_PyStackRef ref)
{
    if (PyStackRef_IsInlineFloat(ref)) {
        return PyStackRef_InlineFloat(ref);
    }
    if (PyStackRef_IsInlineInt(ref)) {
        return (double)PyStackRef_InlineInt(ref);
    }
    PyObject *op = PyStackRef_AsPyObjectBorrow(ref);
    if (PyFloat_CheckExact(op)) {
        return PyFloat_AS_DOUBLE(op);
    }
    assert(_PyLong_CheckExactAndCompact(op));
    return (double)_PyLong_CompactValue((PyLongObject *)op);
}

/* If PyStackRef_FloatOperandsCheck(a, b), store a and b as doubles in *da
   and *db and return true. Two inline floats are tested first, which lets
   the compiler give them a path of their own. */
static inline bool
PyStackRef_FloatOperandValues(_PyStackRef a, _PyStackRef b, double *da, double *db)
{
    if (PyStackRef_IsInlineFloat(a) && PyStackRef_IsInlineFloat(b)) {
        *da = PyStackRef_InlineFloat(a);
        *db = PyStackRef_InlineFloat(b);
        return true;
    }
    if (!PyStackRef_FloatOperandsCheck(a, b)) {
        return false;
    }
    *da = PyStackRef_NumberAsDouble(a);
    *db = PyStackRef_NumberAsDouble(b);
    return true;
}

// A new float, or PyStackRef_NULL with an exception set.
PyAPI_FUNC(_PyStackRef) _PyStackRef_BoxDouble(double value);

/* The result of float arithmetic: inline unless it is NaN, which is boxed
   (see "NaN and signed zero" in boil/design/02-tagged-values.md, section
   3), and always boxed in builds without inline values. PyStackRef_NULL
   with an exception set if boxing fails. */
static inline _PyStackRef
PyStackRef_FromDouble(double value)
{
#ifdef Py_STACKREF_INLINE_VALUES
    if (!isnan(value)) {
        _PyStackRef ref = PyStackRef_FromDoubleNotNaN(value);
        // So that the caller's test for failure folds away on this path.
        if (PyStackRef_IsNull(ref)) {
            Py_UNREACHABLE();
        }
        return ref;
    }
#endif
    return _PyStackRef_BoxDouble(value);
}

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
    ref->ref = PyStackRef_NULL;
}

static inline void
_PyThreadState_PushCStackRefNew(PyThreadState *tstate, _PyCStackRef *ref, PyObject *obj)
{
    _PyThreadState_PushCStackRef(tstate, ref);
    ref->ref = PyStackRef_FromPyObjectNew(obj);
}

/* The collector scans the C stack conservatively, so a popped reference
   left in its slot keeps the object alive until the stack memory is reused.
   The barrier keeps the store, which is dead to the compiler. */
#  define _PyCStackRef_CLEAR(REF) \
    do { \
        (REF)->ref = PyStackRef_NULL; \
        __asm__ volatile("" : : "r"(REF) : "memory"); \
    } while (0)

static inline void
_PyThreadState_PopCStackRef(PyThreadState *tstate, _PyCStackRef *ref)
{
    PyStackRef_XCLOSE(ref->ref);
    _PyCStackRef_CLEAR(ref);
}

static inline _PyStackRef
_PyThreadState_PopCStackRefSteal(PyThreadState *tstate, _PyCStackRef *ref)
{
    _PyStackRef res = ref->ref;
    _PyCStackRef_CLEAR(ref);
    return res;
}


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
