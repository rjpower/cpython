# Decimal float-conversion post-review

Reviewed the refreshed `decimal-float-conversion.diff` layered after the
approved integer conversion. **Approved; no remaining must-fix finding.**

The helper now accepts a borrowed value, reads a float subclass's stored
double without invoking overrides, and records the sign before `fabs`, so
negative zero remains negative. NaN and infinity keep their existing paths.
The exact float's `as_integer_ratio` and the denominator's `bit_length` are
both `METH_VALUE` methods in the generated headers. The ratio tuple, its two
owned elements and the bit-length result are closed in the right order; the
numerator remains owned through the Decimal integer conversion. All
object-form callers wrap their still-live arguments as borrows. The cached
float `__abs__` slot is no longer needed.

The first patch used `PyValue_CLOSE` for nullable tuple elements in its error
branch, which would assert in a debug build. The refreshed patch uses
`PyValue_XCLOSE` there. The author reports extension compile/link, 84 focused
Decimal tests, direct finite/special/subnormal and float-subclass probes,
and 20 constructor tests after that correction. This review does not certify
the full Decimal module as value-native; its Clinic and slot entry work is a
separate increment.
