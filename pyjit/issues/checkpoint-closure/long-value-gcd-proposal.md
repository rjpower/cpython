# Exact integer value GCD

Add `_PyLong_ValueGCD(PyValueBorrow a, PyValueBorrow b) -> PyValue` to
`pycore_long_value.h`. Preconditions: both non-null operands are ints accepted
by the exact-base long implementation; the result is a new owned non-negative
int value, or PyValue_NULL with the existing allocation exception. No Python
numeric dispatch, user subclass overrides or inline-to-object projection.

1. Obtain digit spans from `_PyLong_SpanOfValue` into independent word buffers.
   Attempt `_PyLong_SpanAsInt64AndOverflow` on each. If both fit, compute
   unsigned magnitudes without signed negation overflow (in particular
   INT64_MIN), then Euclidean word gcd; return `PyValue_FromUInt64` so the
   2**63 result is handled correctly. Zero/zero returns inline zero.
2. If only one fits, return exact-base `_PyLong_ValueAbsolute` of the other
   for a zero word. Otherwise compute `_PyLong_ValueRemainder(huge, word)`
   directly, with owned temporary and explicit null/error cleanup. Its
   absolute remainder is smaller than the word magnitude; read it by span
   and finish the unsigned word gcd, closing the temporary. Preserve the
   sign-independent mathematical result and avoid reflected/dynamic dispatch.
3. If neither fits, both are pointer-space long objects. Extract only after
   that proof and reuse the existing Lehmer algorithm in one internal
   value-returning body. Preserve all existing ref ownership, allocation
   failures and digit mutation rules. Heap-sized result transfers via
   PyValue_FromObjectSteal; the final small result uses PyValue_FromUInt64.
   Do not duplicate the Lehmer loop.
4. Retain `_PyLong_GCD(PyObject *, PyObject *)` solely as a true compatibility
   entry, implemented with value argument conversion and `_PyValue_THUNK_EXIT`
   at that actual object boundary. Mark/document that provider per R27. Core
   callers must migrate to `_PyLong_ValueGCD`; this wrapper cannot be used to
   launder core boxes. Confirm exact symbol export and caller inventory before
   the mark is integrated.

Focused tests compare math.gcd/reference Euclid across zero, signs, inline
limits, INT64_MIN/MAX, 2**63, mixed word/huge and both-huge random values;
include bool/int subclass where supported, allocation-failure cleanup and
large-value performance parity with the existing Lehmer path. Compile the long
unit and run narrow math/Decimal tests. Independent review precedes editing
`Objects/longobject.c` or the header. Decimal.as_integer_ratio may then call
this exact-base provider alongside `_PyLong_ValueMultiply`,
`_PyLong_ValueFloorDivide` and `_PyLong_ValuePower` without thunking.
