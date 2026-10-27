/* PyValue: the value type of the core (boil/design/06-architecture-v2.md,
   section 3.1). */

#ifndef Py_PYVALUE_H
#define Py_PYVALUE_H
#ifndef Py_LIMITED_API
#ifdef __cplusplus
extern "C" {
#endif

/*
  A PyValue is a 64-bit word that holds an int32, a double or a reference to
  a heap object.  It is the type of a frame slot, an argument array element
  and a value passed between handlers.  Nothing is reference counted (the
  tracing collector owns memory), so a PyValue owns nothing and copying one
  is all that "taking" or "giving up" a value means.

  Encoding

    Top 16 bits     Low 48 bits         Meaning
    0x0000          0                   NULL: absent, unbound, or an error
                                        with an exception set
    0x0000          ptr                 heap object; low 3 bits zero
    0x0001          int48               internal index, never visible to
                                        Python (the core's FOR_ITER state)
    0x0002-0x7FF2   double + 2^49       inline float, +0.0 to +inf
    0x8002-0xFFF2   double + 2^49       inline float, -0.0 to -inf
    0xFFFE          0x0000 then int32   inline int
    other                               unused: images of NaNs, and 0xFFFF

  Values below 2^48 are pointer space.  A NaN is always boxed, because some
  NaN images overlap the other regions.  None, True and False are pointers
  to static objects.

  Every PyValue is canonical.  PyValue_FromObject() turns an exact int that
  fits in int32 into an inline int and an exact float other than NaN into an
  inline float, so that a list, tuple, dict, set, cell, instance slot, global
  or frame slot never holds a box of such a value.  The only int and float
  objects that a PyValue refers to are NaNs, ints outside int32, and
  instances of subclasses.

  Converting a value to a PyObject * (PyValue_Object()) boxes an inline int
  or float into a fresh heap object.  Boxing runs no Python code; it fails
  only for want of memory.  A box is a PyObject *: converting it back gives
  the inline value, not a reference to the box.

  Identity.  An exact int that fits in int32, or an exact float other than
  NaN, may be held inline in one place and boxed in others, and a value may
  be boxed more than once.  The program cannot tell these apart: such a value
  has one identity, its inline encoding, whether it is held inline or boxed.
  Any other object's identity is its address.  An encoding is at least 2^49
  and an address is below 2^48, so an identity names exactly one value or
  object.  PyValue_Is(), and hence `is`, compares identities, and id()
  returns the identity.  Since every PyValue is canonical, two values have
  the same identity exactly when they have the same encoding.  0.0 and -0.0
  differ in bits, so `0.0 is -0.0` is False.  NaN is never inline and keeps
  object identity, as do bools and int subclasses.  Py_Is() stays pointer equality: C code sees only boxes, and
  two boxes of one value are different pointers, so C code that compares an
  object it read from a container with == against another object can see
  two boxes of one value as different.  Container lookups compare values,
  not pointers.
*/

typedef struct PyValue {
    uintptr_t bits;
} PyValue;

#define _PyValue_PTR_LIMIT     ((uintptr_t)1 << 48)
#define _PyValue_INDEX_BASE    ((uintptr_t)1 << 48)
#define _PyValue_DOUBLE_OFFSET ((uintptr_t)1 << 49)
#define _PyValue_INT_BASE      ((uintptr_t)0xFFFE << 48)
// An object address has these bits clear.
#define _PyValue_TAG_BITS      7
// The bits of an internal error value, which no object address has.
#define _PyValue_ERROR_BITS    2

#ifdef Py_STATS
#  define _PyValue_STAT_INC(name) _Py_STATS_EXPR(object_stats.name++)
#else
#  define _PyValue_STAT_INC(name) ((void)0)
#endif

static inline PyValue
_PyValue_FromBits(uintptr_t bits)
{
    PyValue v;
    v.bits = bits;
    return v;
}

static const PyValue PyValue_NULL = {0};

static inline int
PyValue_IsNull(PyValue v)
{
    return v.bits == 0;
}

static inline int
PyValue_IsInline(PyValue v)
{
    return v.bits >= _PyValue_DOUBLE_OFFSET;
}

static inline int
PyValue_IsInlineInt(PyValue v)
{
    return v.bits >= _PyValue_INT_BASE;
}

static inline int
PyValue_IsInlineFloat(PyValue v)
{
    return v.bits - _PyValue_DOUBLE_OFFSET
           < _PyValue_INT_BASE - _PyValue_DOUBLE_OFFSET;
}

// Does v refer to a heap object?  False for NULL, inline values and the
// core's internal values.
static inline int
PyValue_IsObject(PyValue v)
{
    return v.bits > _PyValue_ERROR_BITS && v.bits < _PyValue_PTR_LIMIT;
}

// The encoding of an inline int, which is also its identity.
static inline uintptr_t
_PyValue_Int32Bits(int32_t i)
{
    return _PyValue_INT_BASE | (uint32_t)i;
}

// The encoding of an inline float, which is also its identity.  d is not NaN.
static inline uintptr_t
_PyValue_DoubleBits(double d)
{
    uint64_t u;
    assert(!isnan(d));
    memcpy(&u, &d, sizeof(u));
    return u + _PyValue_DOUBLE_OFFSET;
}

static inline PyValue
PyValue_FromInt32(int32_t i)
{
    _PyValue_STAT_INC(inline_ints_created);
    return _PyValue_FromBits(_PyValue_Int32Bits(i));
}

// A new float object for value, or PyValue_NULL with an exception set.
PyAPI_FUNC(PyValue) _PyValue_BoxDouble(double value);

/* The value d: inline unless it is NaN, which is boxed.  PyValue_NULL with
   an exception set if boxing fails. */
static inline PyValue
PyValue_FromDouble(double d)
{
    if (!isnan(d)) {
        _PyValue_STAT_INC(inline_floats_created);
        PyValue v = _PyValue_FromBits(_PyValue_DoubleBits(d));
        // So that the caller's test for failure folds away on this path.
        if (PyValue_IsNull(v)) {
            Py_UNREACHABLE();
        }
        return v;
    }
    return _PyValue_BoxDouble(d);
}

// The canonical value of an exact int object that is not compact.
PyAPI_FUNC(PyValue) _PyValue_FromLongObject(PyObject *op);

/* Inlined with a static object such as Py_None, GCC follows it into the
   branches for the types it is not and warns of reads beyond it
   (-Warray-bounds).  Hiding where the pointer came from costs no
   instruction. */
#if defined(__GNUC__)
#  define _PyValue_HIDE_ORIGIN(p) __asm__("" : "+r"(p))
#else
#  define _PyValue_HIDE_ORIGIN(p) ((void)0)
#endif

/* The canonical value of op, which must not be NULL (see "Every PyValue is
   canonical" above): an inline int or float for an exact int that fits in
   int32 or an exact float other than NaN, and otherwise a reference to op. */
static inline PyValue
_PyValue_FromObject(PyObject *op)
{
    assert(op != NULL);
    assert(((uintptr_t)op & _PyValue_TAG_BITS) == 0);
    PyTypeObject *tp = Py_TYPE(op);
    if (tp == &PyFloat_Type) {
        _PyValue_HIDE_ORIGIN(op);
        double d = PyFloat_AS_DOUBLE(op);
        if (!isnan(d)) {
            return _PyValue_FromBits(_PyValue_DoubleBits(d));
        }
    }
    else if (tp == &PyLong_Type) {
        _PyValue_HIDE_ORIGIN(op);
        if (_PyLong_IsCompact((PyLongObject *)op)) {
            return _PyValue_FromBits(_PyValue_Int32Bits(
                (int32_t)_PyLong_CompactValue((PyLongObject *)op)));
        }
        return _PyValue_FromLongObject(op);
    }
    return _PyValue_FromBits((uintptr_t)op);
}
#define PyValue_FromObject(op) _PyValue_FromObject(_PyObject_CAST(op))

static inline int32_t
PyValue_Int32(PyValue v)
{
    assert(PyValue_IsInlineInt(v));
    return (int32_t)(uint32_t)v.bits;
}

/* The double that v holds inline.  A float object's double is
   PyFloat_AS_DOUBLE(PyValue_ObjectBorrow(v)). */
static inline double
PyValue_Double(PyValue v)
{
    assert(PyValue_IsInlineFloat(v));
    uint64_t u = v.bits - _PyValue_DOUBLE_OFFSET;
    double d;
    memcpy(&d, &u, sizeof(d));
    return d;
}

/* The object that v refers to.  v must not hold an inline value: use
   PyValue_Object() for a value that may be one. */
static inline PyObject *
PyValue_ObjectBorrow(PyValue v)
{
    assert(v.bits < _PyValue_PTR_LIMIT);
    return (PyObject *)v.bits;
}

/* A new box of the inline value v, or NULL with an exception set.  If
   memory runs out, the collector runs and the box is tried again first. */
PyAPI_FUNC(PyObject *) _PyValue_BoxInline(PyValue v);

/* _PyValue_BoxInline() for accessor, a function that cannot report an
   error: if the box cannot be made, the process aborts with a message
   naming accessor. */
PyAPI_FUNC(PyObject *) _PyValue_BoxOrAbort(PyValue v, const char *accessor);

/* The object that v holds: a new box if v holds an inline value, or NULL
   with an exception set if boxing fails.  PyValue_Object(PyValue_NULL) is
   NULL. */
static inline PyObject *
PyValue_Object(PyValue v)
{
    if (PyValue_IsInline(v)) {
        return _PyValue_BoxInline(v);
    }
    return PyValue_ObjectBorrow(v);
}

/* PyValue_Object() for accessor, which has no error channel: it never
   returns NULL for a value that is not PyValue_NULL, and aborts instead. */
static inline PyObject *
_PyValue_ObjectOrAbort(PyValue v, const char *accessor)
{
    if (PyValue_IsInline(v)) {
        return _PyValue_BoxOrAbort(v, accessor);
    }
    return PyValue_ObjectBorrow(v);
}

// Python's `is` (see "Identity" above).
static inline int
PyValue_Is(PyValue a, PyValue b)
{
    return a.bits == b.bits;
}

static inline PyTypeObject *
PyValue_Type(PyValue v)
{
    if (PyValue_IsInline(v)) {
        return PyValue_IsInlineInt(v) ? &PyLong_Type : &PyFloat_Type;
    }
    return Py_TYPE(PyValue_ObjectBorrow(v));
}

#ifdef __cplusplus
}
#endif
#endif  /* !Py_LIMITED_API */
#endif  /* !Py_PYVALUE_H */
