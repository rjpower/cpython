# Decimal rich comparison value slot

Port `dec_richcompare` as `_Py_value_richcompare` plus
`_Py_VALUE_THUNK_RICHCMP` at its actual object entry. Runtime slot dispatch
must continue to supply the Decimal receiver as the first argument and swap
the comparison opcode for reflected calls; use the already-reviewed
value-aware left/right state lookup. Check/projection of that concrete receiver
is allowed only after the Decimal type invariant. Keep the current-context
lookup before operand conversion and propagate its errors.

Replace `convert_op_cmp` with a value-aware, explicit tri-state result:
converted Decimal owners a/b, unsupported -> PyValue_NotImplemented, and
failure -> PyValue_NULL with exception. Preserve this exact branch order:

1. Decimal (including subtype): retain its object; long (including bool/int
   subtype): existing exact value-to-Decimal conversion; float (including
   subtype and inline): for ordering, call dec_addstatus(FloatOperation)
   before conversion and honor its trap; for equality/inequality set the
   FloatOperation status without raising its trap, then convert exactly.
2. Complex only for equality/inequality: actual complex values are pointer
   space; call PyComplex_AsCComplex while held. If imaginary part is zero,
   create a temporary `PyValue_FromDouble(real)` (NaN may allocate), set the
   FloatOperation status and convert to Decimal; close that owner on all
   paths. Nonzero imaginary returns NotImplemented. Ordering follows the
   existing Rational/isinstance branch, not a new shortcut.
3. Otherwise call `_PyValue_IsInstance(w, state->Rational)` in the existing
   order. On success, obtain owned numerator using `_PyValue_GetAttr` with a
   held name object; call PyDecType_FromLongExact directly on that borrowed
   value, then close it. For a finite Decimal receiver, obtain the denominator
   similarly, convert it, and retain the existing exact max-context
   multiplication, exponent restoration and error/status handling. If receiver
   is special, skip denominator lookup exactly as before. Retain the Rational
   owner through both lookups and all reentrant user callbacks; preserve
   numerator-before-denominator side effects and errors. False isinstance
   returns NotImplemented. Verify dynamic `__class__`/ABC behavior against
   the existing C code; do not project inline values to call isinstance.

For a/b, keep the existing mpd_qcmp status logic: sNaN and non-equality
unordered comparisons signal; qNaN ==/!= map to false/true without extra
signaling; respect InvalidOperation traps. Return PyValue_True/False, close
converted Decimal owners on all paths, and leave no object projection for a
boolean result. Do not route a private value call through the object thunk.

Focused probes: Decimal versus int51/large int/bool/subclasses, float finite,
±0/inf/qNaN/sNaN, FloatOperation flags and trap distinction for eq versus
ordering, complex zero and nonzero imaginary, Fraction and custom registered
Rational with descriptor side effects/failures and inline numerator or
 denominator, reflected/subclass overrides, unsupported operands and exact
C exception behavior. Include malformed numerator/denominator, allocation
failures and ownership/reentrancy checks. Compile the extension and targeted
comparison/context tests; coordinator owns full Decimal and R27 integration.
