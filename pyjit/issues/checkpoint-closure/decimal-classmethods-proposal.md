# Decimal string and classmethod packet

This packet covers Decimal.number_class, Decimal.to_eng_string,
Decimal.from_float, Decimal.from_number and Context.create_decimal_from_float.
It depends on the approved selective Clinic generator, float/int conversion
cores, and optional-context helper.

For the two Decimal string methods, annotate Clinic entries, borrow the
optional context value, and resolve it first via optional_context_value. Keep
the concrete Decimal receiver object, the exact mpd_class/mpd_to_eng_size,
Unicode construction and mpd_free ordering, error messages and capitals
behavior. Check the resulting Unicode pointer before transferring ownership to
a PyValue; return PyValue_NULL on failure.

For from_float/from_number, the METH_VALUE classmethod self and defining-class
metadata must still produce the same concrete `type` and `cls` objects. The
argument is a borrowed value. Obtain the current borrowed Context with the
existing helper applied to None; add a forward declaration because the
classmethod bodies precede that helper's definition. Keep current-context
lookup before conversion. `PyDecType_FromFloatExact` already accepts the
borrowed value. Change the single caller and implementation of
`PyDecType_FromNumberExact` to accept PyValueBorrow: guard object projection
before PyDec_Check, preserve Decimal-exact conversion, LongCheck including
bool and int subclasses, FloatCheck and the FloatOperation status/trap, and
use PyValue_TYPE for the existing unsupported-type error. The helper still
returns a nullable, owned base Decimal object.

When the requested type is the base Decimal type, transfer its owned result to
a value. For a subclass, call the requested type with `_PyValue_CallOneArg`,
passing borrowed type and base result; keep the result object owned through the
call, release it exactly once afterward, and return the call's owned PyValue
or its null error sentinel. This preserves a subclass constructor returning a
different Python type without an object projection or a changed constructor
order. A failure before the call returns PyValue_NULL. Compare against the
existing C implementation for constructor callbacks and unusual returns.

Context.create_decimal_from_float borrows the numeric value directly and calls
`PyDecType_FromFloat(state->PyDec_Type, f, context)`; transfer its owned
Decimal result after checking null. Existing context rounding, signals and
traps remain in that helper.

Focused verification covers all three conversion entry points with int/float,
bool and subclasses, infinities/NaNs and invalid inputs; FloatOperation and
Inexact traps, Decimal subclasses with custom constructors including an inline
return, and callback exception/ownership; string modes with capitals, invalid
inline context, NaNs and Unicode allocation failures where practical. Check
source C behavior where Python Decimal differs. Compile the shared extension
and run the smallest relevant existing Decimal test classes; merged full
coverage and R27 gates belong to integration.
