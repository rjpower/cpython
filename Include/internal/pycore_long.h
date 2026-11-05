#ifndef Py_INTERNAL_LONG_H
#define Py_INTERNAL_LONG_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_bytesobject.h"   // _PyBytesWriter
#include "pycore_runtime.h"       // _Py_SINGLETON()

/*
 * Default int base conversion size limitation: Denial of Service prevention.
 *
 * Chosen such that this isn't wildly slow on modern hardware and so that
 * everyone's existing deployed numpy test suite passes before
 * https://github.com/numpy/numpy/issues/22098 is widely available.
 *
 * $ python -m timeit -s 's = "1"*4300' 'int(s)'
 * 2000 loops, best of 5: 125 usec per loop
 * $ python -m timeit -s 's = "1"*4300; v = int(s)' 'str(v)'
 * 1000 loops, best of 5: 311 usec per loop
 * (zen2 cloud VM)
 *
 * 4300 decimal digits fits a ~14284 bit number.
 */
#define _PY_LONG_DEFAULT_MAX_STR_DIGITS 4300
/*
 * Threshold for max digits check.  For performance reasons int() and
 * int.__str__() don't checks values that are smaller than this
 * threshold.  Acts as a guaranteed minimum size limit for bignums that
 * applications can expect from CPython.
 *
 * % python -m timeit -s 's = "1"*640; v = int(s)' 'str(int(s))'
 * 20000 loops, best of 5: 12 usec per loop
 *
 * "640 digits should be enough for anyone." - gps
 * fits a ~2126 bit decimal number.
 */
#define _PY_LONG_MAX_STR_DIGITS_THRESHOLD 640

#if ((_PY_LONG_DEFAULT_MAX_STR_DIGITS != 0) && \
   (_PY_LONG_DEFAULT_MAX_STR_DIGITS < _PY_LONG_MAX_STR_DIGITS_THRESHOLD))
# error "_PY_LONG_DEFAULT_MAX_STR_DIGITS smaller than threshold."
#endif

/* runtime lifecycle */

extern PyStatus _PyLong_InitTypes(PyInterpreterState *);
extern void _PyLong_FiniTypes(PyInterpreterState *interp);


/* other API */

PyAPI_FUNC(void) _PyLong_ExactDealloc(PyObject *self);

#define _PyLong_SMALL_INTS _Py_SINGLETON(small_ints)

// _PyLong_GetZero() and _PyLong_GetOne() must always be available
// _PyLong_FromUnsignedChar must always be available
#if _PY_NSMALLPOSINTS < 257
#  error "_PY_NSMALLPOSINTS must be greater than or equal to 257"
#endif

#define _PY_IS_SMALL_INT(val) \
    (-_PY_NSMALLNEGINTS <= (val) && (val) < _PY_NSMALLPOSINTS)

// Return a reference to the immortal zero singleton.
// The function cannot return NULL.
static inline PyObject* _PyLong_GetZero(void)
{ return (PyObject *)&_PyLong_SMALL_INTS[_PY_NSMALLNEGINTS]; }

// Return a reference to the immortal one singleton.
// The function cannot return NULL.
static inline PyObject* _PyLong_GetOne(void)
{ return (PyObject *)&_PyLong_SMALL_INTS[_PY_NSMALLNEGINTS+1]; }

static inline PyObject* _PyLong_FromUnsignedChar(unsigned char i)
{
    return (PyObject *)&_PyLong_SMALL_INTS[_PY_NSMALLNEGINTS+i];
}

// _PyLong_Frexp returns a double x and an exponent e such that the
// true value is approximately equal to x * 2**e.  x is
// 0.0 if and only if the input is 0 (in which case, e and x are both
// zeroes); otherwise, 0.5 <= abs(x) < 1.0.
// Always successful.
//
// Export for 'math' shared extension
PyAPI_DATA(double) _PyLong_Frexp(PyLongObject *a, int64_t *e);

extern PyObject* _PyLong_FromBytes(const char *, Py_ssize_t, int);

// _PyLong_DivmodNear.  Given integers a and b, compute the nearest
// integer q to the exact quotient a / b, rounding to the nearest even integer
// in the case of a tie.  Return (q, r), where r = a - q*b.  The remainder r
// will satisfy abs(r) <= abs(b)/2, with equality possible only if q is
// even.
//
// Export for '_datetime' shared extension.
PyAPI_DATA(PyObject*) _PyLong_DivmodNear(PyObject *, PyObject *);

// _PyLong_Format: Convert the long to a string object with given base,
// appending a base prefix of 0[box] if base is 2, 8 or 16.
// Export for '_tkinter' shared extension.
PyAPI_DATA(PyObject*) _PyLong_Format(PyObject *obj, int base);

// Export for 'math' shared extension
PyAPI_DATA(PyObject*) _PyLong_Rshift(PyObject *, int64_t);

// Export for 'math' shared extension
PyAPI_DATA(PyObject*) _PyLong_Lshift(PyObject *, int64_t);

// Export for 'binascii' shared extension.
PyAPI_DATA(unsigned char) _PyLong_DigitValue[256];

/* Format the object based on the format_spec, as defined in PEP 3101
   (Advanced String Formatting). */
extern int _PyLong_FormatAdvancedWriter(
    _PyUnicodeWriter *writer,
    PyObject *obj,
    PyObject *format_spec,
    Py_ssize_t start,
    Py_ssize_t end);

extern int _PyLong_FormatWriter(
    _PyUnicodeWriter *writer,
    PyObject *obj,
    int base,
    int alternate);

extern char* _PyLong_FormatBytesWriter(
    PyBytesWriter *writer,
    char *str,
    PyObject *obj,
    int base,
    int alternate);

// Argument converters used by Argument Clinic

// Export for 'select' shared extension (Argument Clinic code)
PyAPI_FUNC(int) _PyLong_UnsignedShort_Converter(PyObject *, void *);

// Export for '_testclinic' shared extension (Argument Clinic code)
PyAPI_FUNC(int) _PyLong_UnsignedInt_Converter(PyObject *, void *);

// Export for '_blake2' shared extension (Argument Clinic code)
PyAPI_FUNC(int) _PyLong_UnsignedLong_Converter(PyObject *, void *);

// Export for '_blake2' shared extension (Argument Clinic code)
PyAPI_FUNC(int) _PyLong_UnsignedLongLong_Converter(PyObject *, void *);

// Export for '_testclinic' shared extension (Argument Clinic code)
PyAPI_FUNC(int) _PyLong_Size_t_Converter(PyObject *, void *);

PyAPI_FUNC(int) _PyLong_UInt8_Converter(PyObject *, void *);
PyAPI_FUNC(int) _PyLong_UInt16_Converter(PyObject *, void *);
PyAPI_FUNC(int) _PyLong_UInt32_Converter(PyObject *, void *);
PyAPI_FUNC(int) _PyLong_UInt64_Converter(PyObject *, void *);

/* Long value tag bits:
 * 0-1: Sign bits value = (1-sign), ie. negative=2, positive=0, zero=1.
 * 2: Set to 1 for the small ints
 * 3+ Unsigned digit count
 */
#define SIGN_MASK 3
#define SIGN_ZERO 1
#define SIGN_NEGATIVE 2
#define NON_SIZE_BITS 3
#define IMMORTALITY_BIT_MASK (1 << 2)

/* The functions _PyLong_IsCompact and _PyLong_CompactValue are defined
 * in Include/cpython/longobject.h, since they need to be inline.
 *
 * "Compact" values have at least one bit to spare,
 * so that addition and subtraction can be performed on the values
 * without risk of overflow.
 *
 * The inline functions need tag bits.
 * For readability, rather than do `#define SIGN_MASK _PyLong_SIGN_MASK`
 * we define them to the numbers in both places and then assert that
 * they're the same.
 */
#if SIGN_MASK != _PyLong_SIGN_MASK
#  error "SIGN_MASK does not match _PyLong_SIGN_MASK"
#endif
#if NON_SIZE_BITS != _PyLong_NON_SIZE_BITS
#  error "NON_SIZE_BITS does not match _PyLong_NON_SIZE_BITS"
#endif

/* All *compact" values are guaranteed to fit into
 * a Py_ssize_t with at least one bit to spare.
 * In other words, for 64 bit machines, compact
 * will be signed 63 (or fewer) bit values
 */

/* Return 1 if the argument is compact int */
static inline int
_PyLong_IsNonNegativeCompact(const PyLongObject* op) {
    assert(PyLong_Check(op));
    return ((op->long_value.lv_tag & ~IMMORTALITY_BIT_MASK) <= (1 << NON_SIZE_BITS));
}


static inline int
_PyLong_BothAreCompact(const PyLongObject* a, const PyLongObject* b) {
    assert(PyLong_Check(a));
    assert(PyLong_Check(b));
    return (a->long_value.lv_tag | b->long_value.lv_tag) < (2 << NON_SIZE_BITS);
}
static inline bool
_PyLong_IsZero(const PyLongObject *op)
{
    return (op->long_value.lv_tag & SIGN_MASK) == SIGN_ZERO;
}

static inline bool
_PyLong_IsNegative(const PyLongObject *op)
{
    return (op->long_value.lv_tag & SIGN_MASK) == SIGN_NEGATIVE;
}

static inline bool
_PyLong_IsPositive(const PyLongObject *op)
{
    return (op->long_value.lv_tag & SIGN_MASK) == 0;
}

/* Return true if the argument is a small int */
static inline bool
_PyLong_IsSmallInt(const PyLongObject *op)
{
    assert(PyLong_Check(op));
    bool is_small_int = (op->long_value.lv_tag & IMMORTALITY_BIT_MASK) != 0;
    if (is_small_int) {
        assert(PyLong_CheckExact(op));
        assert(_Py_IsImmortal(op));
        assert((_PyLong_IsCompact(op)
                && _PY_IS_SMALL_INT(_PyLong_CompactValue(op))));
    }
    return is_small_int;
}

static inline Py_ssize_t
_PyLong_DigitCount(const PyLongObject *op)
{
    assert(PyLong_Check(op));
    return (Py_ssize_t)(op->long_value.lv_tag >> NON_SIZE_BITS);
}

/* Equivalent to _PyLong_DigitCount(op) * _PyLong_NonCompactSign(op) */
static inline Py_ssize_t
_PyLong_SignedDigitCount(const PyLongObject *op)
{
    assert(PyLong_Check(op));
    Py_ssize_t sign = 1 - (op->long_value.lv_tag & SIGN_MASK);
    return sign * (Py_ssize_t)(op->long_value.lv_tag >> NON_SIZE_BITS);
}

static inline int
_PyLong_CompactSign(const PyLongObject *op)
{
    assert(PyLong_Check(op));
    assert(_PyLong_IsCompact((PyLongObject *)op));
    return 1 - (op->long_value.lv_tag & SIGN_MASK);
}

static inline int
_PyLong_NonCompactSign(const PyLongObject *op)
{
    assert(PyLong_Check(op));
    assert(!_PyLong_IsCompact((PyLongObject *)op));
    return 1 - (op->long_value.lv_tag & SIGN_MASK);
}

/* Do a and b have the same sign? */
static inline int
_PyLong_SameSign(const PyLongObject *a, const PyLongObject *b)
{
    return (a->long_value.lv_tag & SIGN_MASK) == (b->long_value.lv_tag & SIGN_MASK);
}

/* Initialize the tag of a freshly-allocated int. */
static inline void
_PyLong_InitTag(PyLongObject *op)
{
    assert(PyLong_Check(op));
    op->long_value.lv_tag = SIGN_ZERO; /* non-immortal zero */
}

#define TAG_FROM_SIGN_AND_SIZE(sign, size) \
    ((uintptr_t)(1 - (sign)) | ((uintptr_t)(size) << NON_SIZE_BITS))

static inline void
_PyLong_SetSignAndDigitCount(PyLongObject *op, int sign, Py_ssize_t size)
{
    assert(size >= 0);
    assert(-1 <= sign && sign <= 1);
    assert(sign != 0 || size == 0);
    assert(!_PyLong_IsSmallInt(op));
    op->long_value.lv_tag = TAG_FROM_SIGN_AND_SIZE(sign, size);
}

static inline void
_PyLong_SetDigitCount(PyLongObject *op, Py_ssize_t size)
{
    assert(size >= 0);
    assert(!_PyLong_IsSmallInt(op));
    op->long_value.lv_tag = (((size_t)size) << NON_SIZE_BITS) | (op->long_value.lv_tag & SIGN_MASK);
}

#define NON_SIZE_MASK ~(uintptr_t)((1 << NON_SIZE_BITS) - 1)

static inline void
_PyLong_FlipSign(PyLongObject *op)
{
    assert(!_PyLong_IsSmallInt(op));
    unsigned int flipped_sign = 2 - (op->long_value.lv_tag & SIGN_MASK);
    op->long_value.lv_tag &= NON_SIZE_MASK;
    op->long_value.lv_tag |= flipped_sign;
}

#define _PyLong_DIGIT_INIT(val) \
    { \
        .ob_base = _PyObject_HEAD_INIT(&PyLong_Type), \
        .long_value  = { \
            .lv_tag = TAG_FROM_SIGN_AND_SIZE( \
                (val) == 0 ? 0 : ((val) < 0 ? -1 : 1), \
                (val) == 0 ? 0 : 1) | IMMORTALITY_BIT_MASK, \
            { ((val) >= 0 ? (val) : -(val)) }, \
        } \
    }

#define _PyLong_FALSE_TAG TAG_FROM_SIGN_AND_SIZE(0, 0)
#define _PyLong_TRUE_TAG TAG_FROM_SIGN_AND_SIZE(1, 1)

static inline int
_PyLong_CheckExactAndCompact(PyObject *op)
{
    return PyLong_CheckExact(op) && _PyLong_IsCompact((const PyLongObject *)op);
}


/* Digit spans and results (pyjit/design/proposals/value-api-v2a1a.md,
 * section 1.2).
 *
 * The digit kernels of Objects/longobject.c read their operands as spans:
 * the magnitude digits of an int, least significant first, and its sign. A
 * heap int presents its own digits; an int held inline in a value presents
 * its word, split into _PyLong_WORD_DIGITS digits in a local buffer
 * (_PyLong_SpanOfWord(); _PyLong_SpanOfValue() in pycore_long_value.h). A
 * span is not an object, so no inline int ever becomes one.
 *
 * Kernels build their results in a _PyLongOut. An out holds computed
 * results only, never a copy of an input: there is no function that copies
 * a span into an out. */

/* The digits of a word: |v| < 2**60. */
#define _PyLong_WORD_DIGITS 2
static_assert(PyLong_SHIFT * _PyLong_WORD_DIGITS == 60,
              "a word is two 30-bit digits");

typedef struct {
    const digit *d;     // magnitude, least significant first; d[n-1] != 0
    Py_ssize_t n;       // digit count, 0 for zero
    int neg;            // 1 if negative; never for zero
} _PyLongSpan;

// The span of op, any instance of int or a subclass.
static inline _PyLongSpan
_PyLong_SpanOf(const PyLongObject *op)
{
    _PyLongSpan span = {op->long_value.ob_digit, _PyLong_DigitCount(op),
                        _PyLong_IsNegative(op)};
    return span;
}

// The span of v, |v| < 2**60, with its digits in buf.
static inline _PyLongSpan
_PyLong_SpanOfWord(int64_t v, digit buf[_PyLong_WORD_DIGITS])
{
    uint64_t m = v < 0 ? 0 - (uint64_t)v : (uint64_t)v;
    assert(m >> (PyLong_SHIFT * _PyLong_WORD_DIGITS) == 0);
    buf[0] = (digit)(m & PyLong_MASK);
    buf[1] = (digit)(m >> PyLong_SHIFT);
    _PyLongSpan span = {buf, buf[1] ? 2 : (buf[0] ? 1 : 0), v < 0};
    return span;
}

/* A result under construction. A kernel reserves its size bound in digits
 * (_PyLong_OutReserve()), writes the magnitude to d, and sets n (the digits
 * written, perhaps with leading zeros) and neg. The value layer consumes it
 * with _PyLong_OutValue() or _PyLong_OutObject() (pycore_long_value.h),
 * which normalise; error paths release it with _PyLong_OutAbandon(). */
typedef struct {
    digit small[_PyLong_WORD_DIGITS];
    PyLongObject *heap;     // a reserved heap result, or NULL
    digit *d;               // small, or the digits of heap
    Py_ssize_t n;
    int neg;
} _PyLongOut;

// The heap case of _PyLong_OutReserve(): a new int of bound digits.
PyAPI_FUNC(digit *) _PyLong_OutReserveHeap(_PyLongOut *out, Py_ssize_t bound);

/* Reserve bound digits for a result: small if bound <= _PyLong_WORD_DIGITS,
   else a new heap int of bound digits (a result allocation, sized as
   upstream's). NULL with MemoryError. */
static inline digit *
_PyLong_OutReserve(_PyLongOut *out, Py_ssize_t bound)
{
    assert(bound >= 0);
    out->n = bound;
    out->neg = 0;
    if (bound <= _PyLong_WORD_DIGITS) {
        out->heap = NULL;
        out->d = out->small;
        return out->d;
    }
    return _PyLong_OutReserveHeap(out, bound);
}

static inline void
_PyLong_OutAbandon(_PyLongOut *out)
{
    if (out->heap != NULL) {
        Py_DECREF(out->heap);
        out->heap = NULL;
    }
}

// The normalised result as a span, valid until the out is consumed.
static inline _PyLongSpan
_PyLong_OutSpan(_PyLongOut *out)
{
    Py_ssize_t n = out->n;
    while (n > 0 && out->d[n - 1] == 0) {
        n--;
    }
    out->n = n;
    _PyLongSpan span = {out->d, n, n != 0 && out->neg};
    return span;
}

/* Span readers: the digit loops of the object readers, which become their
   wrappers. */
// The value of a, or -1 with *overflow set to the sign of a. Never raises.
PyAPI_FUNC(int64_t) _PyLong_SpanAsInt64AndOverflow(_PyLongSpan a,
                                                   int *overflow);
// PyLong_AsDouble()'s digit path: -1.0 with OverflowError.
PyAPI_FUNC(double) _PyLong_SpanAsDouble(_PyLongSpan a);
// _PyLong_Frexp() of a span.
PyAPI_FUNC(double) _PyLong_SpanFrexp(_PyLongSpan a, int64_t *e);
// _PyLong_NumBits() of a span: -1 with OverflowError.
PyAPI_FUNC(int64_t) _PyLong_SpanNumBits(_PyLongSpan a);
// The hash of a, as long_hash().
PyAPI_FUNC(Py_hash_t) _PyLong_SpanHash(_PyLongSpan a);
// PyLong_AsNativeBytes() of a span.
PyAPI_FUNC(Py_ssize_t) _PyLong_SpanAsNativeBytes(_PyLongSpan a, void *buffer,
                                                 Py_ssize_t n, int flags);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_LONG_H */
