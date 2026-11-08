# Decimal quantize, integral and round proposal

Existing C code is the behavior reference where `_pydecimal` differs. The
following changes require no change to Python-visible signatures or aliases.

For `quantize` and `to_integral_value`, `to_integral`,
`to_integral_exact`, opt in each Clinic method and retain concrete Decimal self
as PyObject*. `rounding` and `context` are borrowed values; quantize's exp is
also a borrowed value. The generated `to_integral` clone must forward the same
value arguments and return as `to_integral_value`.

Preserve the current order exactly: resolve and validate optional context
first using `optional_context_value`; copy its `mpd_context_t`; then check
rounding (None means unchanged) and apply it to the copy. A rounding object
continues through `getround`, including Unicode subclasses. Inline non-None
rounding produces the same TypeError and text without projection: factor the
existing type_error_int block into `invalid_rounding()` and use it from both
the object helper and the inline-invalid branch. Only after context and
rounding validation does quantize convert self and exp through
`convert_op_value`, holding/releasing owners as in the approved binary family.
No operand conversion or error is allowed to overtake context/rounding errors.
Allocate the result, run the same mpdecimal operation, propagate status into
the original Context, and return an owned value. Value/alias choice must retain
`mpd_qround_to_int` for the quiet methods and `mpd_qround_to_intx` for exact;
quantize keeps its existing qquantize and error labels. Handle qsetround's
internal error via a PyValue-returning equivalent, not a pointer return macro.

`__round__` needs a separate coordinated layer. Its omitted `ndigits` is a
null value sentinel, not Python None; the current C path checks `PyLong_Check`
(which accepts bool/int subclasses but not arbitrary `__index__`) and then
`PyLong_AsSsize_t` with exact overflow behavior. The value path should use
`PyValue_LongCheck` then `_PyValue_IntAsSsize_t` and preserve CURRENT_CONTEXT
lookup first, MPD_SSIZE_MIN handling, quantize and status. Explicit None must
still take the integer TypeError branch. With omitted ndigits it returns an
integer through `dec_as_long`, whose i64 fast path currently builds a
PyLong object. A value-native implementation must return PyValue_FromInt64
for that path, retain the PyLongWriter path for large integers and update all
six call sites with appropriate value slots/methods or true public object
boundaries; canonicalizing a freshly allocated small PyLong would leave the
core's end-to-end path unfinished. Its existing consumers include
as_integer_ratio, the nb_int slot and ceil/floor/trunc, so do not port the
Clinic wrapper alone or add a core object projection. Design paired slots and
object boundaries first.

Focused tests: omitted/None/real and invalid inline contexts, None and every
rounding constant, Unicode subclasses and invalid inline/string rounding,
combined-invalid argument ordering, finite/sNaN inputs, flags and Inexact traps,
subclass receivers, and quantum exponent. For round add omitted versus explicit
None, bool/int subclasses, __index__-only objects, integer bounds and
MPD_SSIZE_MIN, large integers, NaN/infinity and result ownership. Run the
relevant C Decimal test classes and compare C reference outputs where pure
Python Decimal differs; use full `test_decimal` only at integration.
