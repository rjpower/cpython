#ifndef Py_INTERNAL_LONG_VALUE_H
#define Py_INTERNAL_LONG_VALUE_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_long.h"          // _PyLongSpan
#include "pycore_value.h"         // PyValue_FromInt64()

/* The value layer of int and float (pyjit/design/proposals/
 * value-api-v2a1a.md, sections 1 and 2): spans of values, results as
 * values, the word kernels, the value slots of int, float and bool, and the
 * extractor rows. It is apart from pycore_long.h, which pycore_value.h
 * includes, so that pycore_long.h stays free of values.
 *
 * int's value slots dispatch on representation (D1): the word path when
 * both operands have a word (an inline int, or a heap int of at most two
 * digits, |v| < 2**60: bool, subclass instances, 2**50 <= |v| < 2**60); the
 * span path otherwise; NotImplemented for an operand that is not an int.
 * Value slots take borrows, which their caller keeps alive, and return an
 * owned value, or PyValue_NULL with an exception set. */


/* Spans of values, and results as values. */

// The span of v, an int value: an inline int's word in buf, else its digits.
static inline _PyLongSpan
_PyLong_SpanOfValue(PyValueBorrow v, digit buf[_PyLong_WORD_DIGITS])
{
    if (PyValue_IsInlineInt(v)) {
        return _PyLong_SpanOfWord(PyValue_InlineInt64(v), buf);
    }
    PyObject *op = PyValue_AsKnownObjectBorrow(v);
    assert(PyLong_Check(op));
    return _PyLong_SpanOf((PyLongObject *)op);
}

/* Normalise and consume out: an inline int if the result fits (a reserved
   heap result is then freed: a working allocation, sized by the kernel's
   bound as upstream's); else the heap result, or a new heap int made from
   small (a too-large int, R27). PyValue_NULL with MemoryError. */
PyAPI_FUNC(PyValue) _PyLong_OutValue(_PyLongOut *out);

/* Normalise and consume out into an exact int object, for the object-level
   helpers (long_pow's accumulators, l_divmod): the heap result, a cached
   small int, or a new int made from small. NULL with MemoryError. */
PyAPI_FUNC(PyLongObject *) _PyLong_OutObject(_PyLongOut *out);


/* Word kernels (D9), shared by int's value slots and the typed handlers.
   Their operands are words: |a|, |b| < 2**60, so a sum or a difference
   fits int64. A result outside the inline range is a new heap int. */

#define _PyLong_WORD_LIMIT ((int64_t)1 << (PyLong_SHIFT * _PyLong_WORD_DIGITS))

/* _PyValue_MultiplyInt64() and _PyValue_LshiftInt64() (pycore_value.h,
   for the typed handlers in bytecodes.c) are the overflow paths, through the
   span kernels. */

// The word of v, an int value, if it has one.
static inline bool
_PyLong_WordOf(PyValueBorrow v, int64_t *a)
{
    if (PyValue_IsInlineInt(v)) {
        *a = PyValue_InlineInt64(v);
        return true;
    }
    const PyLongObject *op = (const PyLongObject *)PyValue_AsKnownObjectBorrow(v);
    assert(PyLong_Check(op));
    Py_ssize_t n = _PyLong_DigitCount(op);
    if (n > _PyLong_WORD_DIGITS) {
        return false;
    }
    int64_t m = n == 0 ? 0 : op->long_value.ob_digit[0];
    if (n == 2) {
        m |= (int64_t)op->long_value.ob_digit[1] << PyLong_SHIFT;
    }
    *a = _PyLong_IsNegative(op) ? -m : m;
    return true;
}

/* Both v and w are int values: their words, if both have one. Two inline
   ints take the first test. */
static inline bool
_PyLong_WordOperands(PyValueBorrow v, PyValueBorrow w, int64_t *a, int64_t *b)
{
    if (PyValue_BothInlineInt(v, w)) {
        *a = PyValue_InlineInt64(v);
        *b = PyValue_InlineInt64(w);
        return true;
    }
    return _PyLong_WordOf(v, a) && _PyLong_WordOf(w, b);
}

static inline PyValue
_PyLong_WordAdd(int64_t a, int64_t b)
{
    return PyValue_FromInt64(a + b);
}

static inline PyValue
_PyLong_WordSubtract(int64_t a, int64_t b)
{
    return PyValue_FromInt64(a - b);
}

static inline PyValue
_PyLong_WordMultiply(int64_t a, int64_t b)
{
    int64_t product;
    if (!__builtin_mul_overflow(a, b, &product)) {
        return PyValue_FromInt64(product);
    }
    return _PyValue_MultiplyInt64(a, b);
}

static inline PyValue
_PyLong_WordZeroDivision(void)
{
    PyErr_SetString(PyExc_ZeroDivisionError, "division by zero");
    return PyValue_NULL;
}

// a // b, rounded toward minus infinity.
static inline PyValue
_PyLong_WordFloorDivide(int64_t a, int64_t b)
{
    if (b == 0) {
        return _PyLong_WordZeroDivision();
    }
    int64_t q = a / b;
    if (a % b != 0 && ((a < 0) != (b < 0))) {
        q -= 1;
    }
    return PyValue_FromInt64(q);
}

// a % b, with the sign of b.
static inline PyValue
_PyLong_WordRemainder(int64_t a, int64_t b)
{
    if (b == 0) {
        return _PyLong_WordZeroDivision();
    }
    int64_t r = a % b;
    if (r != 0 && ((r < 0) != (b < 0))) {
        r += b;
    }
    return PyValue_FromInt64(r);
}

// a << n, n >= 0 (the caller raises for a negative count).
static inline PyValue
_PyLong_WordLshift(int64_t a, int64_t n)
{
    assert(n >= 0);
    if (a == 0) {
        return PyValue_FromInt64(0);
    }
    if (n < 63) {
        int64_t shifted = (int64_t)((uint64_t)a << n);
        if (Py_ARITHMETIC_RIGHT_SHIFT(int64_t, shifted, n) == a) {
            return PyValue_FromInt64(shifted);
        }
    }
    return _PyValue_LshiftInt64(a, n);
}

// a >> n, n >= 0, rounded toward minus infinity.
static inline PyValue
_PyLong_WordRshift(int64_t a, int64_t n)
{
    assert(n >= 0);
    if (n >= 63) {
        return PyValue_FromInt64(a < 0 ? -1 : 0);
    }
    return PyValue_FromInt64(Py_ARITHMETIC_RIGHT_SHIFT(int64_t, a, n));
}

static inline PyValue
_PyLong_WordAnd(int64_t a, int64_t b)
{
    return PyValue_FromInt64(a & b);
}

static inline PyValue
_PyLong_WordOr(int64_t a, int64_t b)
{
    return PyValue_FromInt64(a | b);
}

static inline PyValue
_PyLong_WordXor(int64_t a, int64_t b)
{
    return PyValue_FromInt64(a ^ b);
}

static inline PyValue
_PyLong_WordNegative(int64_t a)
{
    return PyValue_FromInt64(-a);
}

static inline PyValue
_PyLong_WordInvert(int64_t a)
{
    return PyValue_FromInt64(~a);
}

static inline PyValue
_PyLong_WordAbsolute(int64_t a)
{
    return PyValue_FromInt64(a < 0 ? -a : a);
}

/* a ** b, b >= 0, by squaring: true with the result in *result, or false if
   a product leaves int64 (the caller takes the span path). */
static inline bool
_PyLong_WordPower(int64_t a, int64_t b, int64_t *result)
{
    assert(b >= 0);
    int64_t r = 1;
    while (b > 0) {
        if ((b & 1) && __builtin_mul_overflow(r, a, &r)) {
            return false;
        }
        b >>= 1;
        if (b > 0 && __builtin_mul_overflow(a, a, &a)) {
            return false;
        }
    }
    *result = r;
    return true;
}

static inline int
_PyLong_WordCompare(int64_t a, int64_t b)
{
    return (a > b) - (a < b);
}

/* The hash of a word: the value itself, since |a| < 2**60 < 2**61 - 1, with
   -1 mapped to -2 (long_hash()). */
static inline Py_hash_t
_PyLong_WordHash(int64_t a)
{
    return a == -1 ? -2 : (Py_hash_t)a;
}

// A compare operator on a three-way result.
static inline bool
_PyLong_CompareOp(Py_ssize_t cmp, int op)
{
    switch (op) {
        case Py_LT: return cmp < 0;
        case Py_LE: return cmp <= 0;
        case Py_EQ: return cmp == 0;
        case Py_NE: return cmp != 0;
        case Py_GT: return cmp > 0;
        case Py_GE: return cmp >= 0;
        default: Py_UNREACHABLE();
    }
}

static inline PyValue
_PyLong_CompareResult(Py_ssize_t cmp, int op)
{
    return _PyLong_CompareOp(cmp, op) ? PyValue_True : PyValue_False;
}


/* int's value slots (section 2.1). The object slots of PyLong_Type are
   their thunks. */

PyAPI_FUNC(PyValue) _PyLong_ValueAdd(PyValueBorrow v, PyValueBorrow w);
PyAPI_FUNC(PyValue) _PyLong_ValueSubtract(PyValueBorrow v, PyValueBorrow w);
PyAPI_FUNC(PyValue) _PyLong_ValueMultiply(PyValueBorrow v, PyValueBorrow w);
PyAPI_FUNC(PyValue) _PyLong_ValueGCD(PyValueBorrow v, PyValueBorrow w);
PyAPI_FUNC(PyValue) _PyLong_ValueRemainder(PyValueBorrow v, PyValueBorrow w);
PyAPI_FUNC(PyValue) _PyLong_ValueDivmod(PyValueBorrow v, PyValueBorrow w);
PyAPI_FUNC(PyValue) _PyLong_ValueFloorDivide(PyValueBorrow v,
                                             PyValueBorrow w);
PyAPI_FUNC(PyValue) _PyLong_ValueTrueDivide(PyValueBorrow v,
                                            PyValueBorrow w);
PyAPI_FUNC(PyValue) _PyLong_ValueLshift(PyValueBorrow v, PyValueBorrow w);
PyAPI_FUNC(PyValue) _PyLong_ValueRshift(PyValueBorrow v, PyValueBorrow w);
PyAPI_FUNC(PyValue) _PyLong_ValueAnd(PyValueBorrow v, PyValueBorrow w);
PyAPI_FUNC(PyValue) _PyLong_ValueXor(PyValueBorrow v, PyValueBorrow w);
PyAPI_FUNC(PyValue) _PyLong_ValueOr(PyValueBorrow v, PyValueBorrow w);
PyAPI_FUNC(PyValue) _PyLong_ValuePower(PyValueBorrow v, PyValueBorrow w,
                                       PyValueBorrow z);
PyAPI_FUNC(PyValue) _PyLong_ValueNegative(PyValueBorrow v);
PyAPI_FUNC(PyValue) _PyLong_ValueAbsolute(PyValueBorrow v);
PyAPI_FUNC(PyValue) _PyLong_ValueInvert(PyValueBorrow v);
PyAPI_FUNC(PyValue) _PyLong_ValueFloat(PyValueBorrow v);
// nb_positive, nb_int and nb_index: an exact int of the same value.
PyAPI_FUNC(PyValue) _PyLong_ValueLong(PyValueBorrow v);
PyAPI_FUNC(int) _PyLong_ValueBool(PyValueBorrow v);
// A str.
PyAPI_FUNC(PyObject *) _PyLong_ValueRepr(PyValueBorrow v);
/* Write the decimal repr of v, an int value, to writer: an inline int's word
   span through the decimal kernel (R-inline in _PyValue_WriteRepr()). 0, or
   -1 with an exception set. */
PyAPI_FUNC(int) _PyLong_WriteDecimal(_PyUnicodeWriter *writer,
                                     PyValueBorrow v);

/* The span path of the binary operator op (NB_ADD ... NB_XOR, as
   BINARY_OP's oparg, and NB_POWER for two operands), on any two int
   values, words included: the test oracle of the word paths
   (_testinternalcapi.int_span_path). */
PyAPI_FUNC(PyValue) _PyLong_SpanPathBinary(int op, PyValueBorrow v,
                                           PyValueBorrow w);

PyAPI_FUNC(PyValue) _PyLong_ValueRichCompareSlow(PyValueBorrow v,
                                                 PyValueBorrow w, int op);

/* Rule R-inline (D13): the dispatchers call these for an inline operand,
   whose type the encoding fixes; int's tp_value stores the same
   functions. */
static inline PyValue
_PyLong_ValueRichCompare(PyValueBorrow v, PyValueBorrow w, int op)
{
    if (PyValue_BothInlineInt(v, w)) {
        return _PyLong_CompareResult(
            _PyLong_WordCompare(PyValue_InlineInt64(v),
                                PyValue_InlineInt64(w)), op);
    }
    return _PyLong_ValueRichCompareSlow(v, w, op);
}

static inline Py_hash_t
_PyLong_ValueHash(PyValueBorrow v)
{
    if (PyValue_IsInlineInt(v)) {
        return _PyLong_WordHash(PyValue_InlineInt64(v));
    }
    PyObject *op = PyValue_AsKnownObjectBorrow(v);
    return _PyLong_SpanHash(_PyLong_SpanOf((PyLongObject *)op));
}


/* float's value slots. */

PyAPI_FUNC(PyValue) _PyFloat_ValueAdd(PyValueBorrow v, PyValueBorrow w);
PyAPI_FUNC(PyValue) _PyFloat_ValueSubtract(PyValueBorrow v, PyValueBorrow w);
PyAPI_FUNC(PyValue) _PyFloat_ValueMultiply(PyValueBorrow v, PyValueBorrow w);
PyAPI_FUNC(PyValue) _PyFloat_ValueTrueDivide(PyValueBorrow v,
                                             PyValueBorrow w);
PyAPI_FUNC(PyValue) _PyFloat_ValueRemainder(PyValueBorrow v,
                                            PyValueBorrow w);
PyAPI_FUNC(PyValue) _PyFloat_ValueDivmod(PyValueBorrow v, PyValueBorrow w);
PyAPI_FUNC(PyValue) _PyFloat_ValueFloorDivide(PyValueBorrow v,
                                              PyValueBorrow w);
PyAPI_FUNC(PyValue) _PyFloat_ValuePower(PyValueBorrow v, PyValueBorrow w,
                                        PyValueBorrow z);
PyAPI_FUNC(PyValue) _PyFloat_ValueNegative(PyValueBorrow v);
PyAPI_FUNC(PyValue) _PyFloat_ValueAbsolute(PyValueBorrow v);
// nb_positive and nb_float: an exact float of the same value.
PyAPI_FUNC(PyValue) _PyFloat_ValueFloat(PyValueBorrow v);
// nb_int: the integral part.
PyAPI_FUNC(PyValue) _PyFloat_ValueInt(PyValueBorrow v);
PyAPI_FUNC(int) _PyFloat_ValueBool(PyValueBorrow v);
// A str.
PyAPI_FUNC(PyObject *) _PyFloat_ValueRepr(PyValueBorrow v);
PyAPI_FUNC(PyValue) _PyFloat_ValueRichCompareSlow(PyValueBorrow v,
                                                  PyValueBorrow w, int op);
PyAPI_FUNC(Py_hash_t) _PyFloat_HashObject(PyObject *op);

// A compare operator on two doubles.
static inline bool
_PyFloat_CompareOp(double a, double b, int op)
{
    switch (op) {
        case Py_LT: return a < b;
        case Py_LE: return a <= b;
        case Py_EQ: return a == b;
        case Py_NE: return a != b;
        case Py_GT: return a > b;
        case Py_GE: return a >= b;
        default: Py_UNREACHABLE();
    }
}

static inline PyValue
_PyFloat_CompareResult(double a, double b, int op)
{
    return _PyFloat_CompareOp(a, b, op) ? PyValue_True : PyValue_False;
}

/* R-inline: two inline floats, or an inline float and an inline int (exact
   as doubles), compare as doubles. */
static inline PyValue
_PyFloat_ValueRichCompare(PyValueBorrow v, PyValueBorrow w, int op)
{
    /* Both are set whenever the call returns true; the initialisers quiet
       -Wmaybe-uninitialized, which cannot see through the inlined test. */
    double a = 0.0, b = 0.0;
    if (PyValue_FloatOperandValues(v, w, &a, &b)) {
        return _PyFloat_CompareResult(a, b, op);
    }
    return _PyFloat_ValueRichCompareSlow(v, w, op);
}

static inline Py_hash_t
_PyFloat_ValueHash(PyValueBorrow v)
{
    if (PyValue_IsInlineFloat(v)) {
        // An inline float is never NaN, the one hash that reads the object.
        return _Py_HashDouble(NULL, PyValue_InlineFloat(v));
    }
    return _PyFloat_HashObject(PyValue_AsKnownObjectBorrow(v));
}


/* Extractors (section 2.4): one row per object extractor, which is now its
   thunk, with that function's contract and messages. A row reads an inline
   int directly, a heap int through a span reader, and a non-int through
   _PyValue_Index() if the object function accepts __index__. */

PyAPI_FUNC(int) _PyValue_AsInt(PyValueBorrow v);
PyAPI_FUNC(long) _PyValue_AsLong(PyValueBorrow v);
PyAPI_FUNC(long long) _PyValue_AsLongLong(PyValueBorrow v);
PyAPI_FUNC(long) _PyValue_AsLongAndOverflow(PyValueBorrow v, int *overflow);
PyAPI_FUNC(long long) _PyValue_AsLongLongAndOverflow(PyValueBorrow v,
                                                     int *overflow);
PyAPI_FUNC(int) _PyValue_AsInt32(PyValueBorrow v, int32_t *result);
PyAPI_FUNC(int) _PyValue_AsInt64(PyValueBorrow v, int64_t *result);
PyAPI_FUNC(int) _PyValue_AsUInt32(PyValueBorrow v, uint32_t *result);
PyAPI_FUNC(int) _PyValue_AsUInt64(PyValueBorrow v, uint64_t *result);
PyAPI_FUNC(unsigned long) _PyValue_IntAsUnsignedLong(PyValueBorrow v);
PyAPI_FUNC(unsigned long long) _PyValue_IntAsUnsignedLongLong(
    PyValueBorrow v);
PyAPI_FUNC(unsigned long) _PyValue_AsUnsignedLongMask(PyValueBorrow v);
PyAPI_FUNC(unsigned long long) _PyValue_AsUnsignedLongLongMask(
    PyValueBorrow v);
PyAPI_FUNC(Py_ssize_t) _PyValue_IntAsSsize_t(PyValueBorrow v);
PyAPI_FUNC(size_t) _PyValue_IntAsSize_t(PyValueBorrow v);
PyAPI_FUNC(double) _PyValue_IntAsDouble(PyValueBorrow v);
PyAPI_FUNC(Py_ssize_t) _PyValue_AsNativeBytes(PyValueBorrow v, void *buffer,
                                              Py_ssize_t n, int flags);
PyAPI_FUNC(void *) _PyValue_IntAsVoidPtr(PyValueBorrow v);
PyAPI_FUNC(int) _PyLong_ValueAsByteArray(PyValueBorrow value,
    unsigned char *bytes, size_t n, int little_endian, int is_signed,
    int with_exceptions);
// PyFloat_AsDouble(): __float__, then __index__.
PyAPI_FUNC(double) _PyValue_AsDouble(PyValueBorrow v);

#define _PyValue_AsInt(V) (_PyValue_AsInt)(PyValue_Borrow(V))
#define _PyValue_AsLong(V) (_PyValue_AsLong)(PyValue_Borrow(V))
#define _PyValue_AsLongLong(V) (_PyValue_AsLongLong)(PyValue_Borrow(V))
#define _PyValue_AsLongAndOverflow(V, O) \
    (_PyValue_AsLongAndOverflow)(PyValue_Borrow(V), O)
#define _PyValue_AsLongLongAndOverflow(V, O) \
    (_PyValue_AsLongLongAndOverflow)(PyValue_Borrow(V), O)
#define _PyValue_AsInt32(V, R) (_PyValue_AsInt32)(PyValue_Borrow(V), R)
#define _PyValue_AsInt64(V, R) (_PyValue_AsInt64)(PyValue_Borrow(V), R)
#define _PyValue_AsUInt32(V, R) (_PyValue_AsUInt32)(PyValue_Borrow(V), R)
#define _PyValue_AsUInt64(V, R) (_PyValue_AsUInt64)(PyValue_Borrow(V), R)
#define _PyValue_IntAsUnsignedLong(V) \
    (_PyValue_IntAsUnsignedLong)(PyValue_Borrow(V))
#define _PyValue_IntAsUnsignedLongLong(V) \
    (_PyValue_IntAsUnsignedLongLong)(PyValue_Borrow(V))
#define _PyValue_AsUnsignedLongMask(V) \
    (_PyValue_AsUnsignedLongMask)(PyValue_Borrow(V))
#define _PyValue_AsUnsignedLongLongMask(V) \
    (_PyValue_AsUnsignedLongLongMask)(PyValue_Borrow(V))
#define _PyValue_IntAsSsize_t(V) (_PyValue_IntAsSsize_t)(PyValue_Borrow(V))
#define _PyValue_IntAsSize_t(V) (_PyValue_IntAsSize_t)(PyValue_Borrow(V))
#define _PyValue_IntAsDouble(V) (_PyValue_IntAsDouble)(PyValue_Borrow(V))
#define _PyValue_AsNativeBytes(V, B, N, F) \
    (_PyValue_AsNativeBytes)(PyValue_Borrow(V), B, N, F)
#define _PyValue_IntAsVoidPtr(V) (_PyValue_IntAsVoidPtr)(PyValue_Borrow(V))
#define _PyValue_AsDouble(V) (_PyValue_AsDouble)(PyValue_Borrow(V))

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_LONG_VALUE_H */
