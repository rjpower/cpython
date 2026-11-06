# Decimal constructors and paired numeric slots

The module cannot claim value-native admission while constructor and numeric
entry points still require object operands or object returns. This design
covers the next coherent layers; marked-compiler and runtime gates must still
check the whole module. Concrete Decimal and Context instances remain heap
objects, but every polymorphic input and potentially inline output crosses the
core as a value.

## Decimal constructor and Context create_decimal

`Decimal.__new__` currently has object slot entry `dec_new`, with optional
`value` defaulting to the NULL sentinel and `context` defaulting to None.
`Context.create_decimal` calls `PyDec_FromObject`; these are the only callers
of `PyDecType_FromObjectExact` and `PyDec_FromObject`, respectively. Convert
those helpers to accept PyValueBorrow. Preserve this ordered dispatch: NULL
sentinel -> zero; pointer-space Decimal (including subclass) -> existing
Decimal copy/exact operation; pointer-space Unicode -> existing Unicode
parser; LongCheck including bool/subclasses -> value long conversion;
pointer-space tuple/list -> sequence conversion; FloatCheck including
subclasses -> record FloatOperation before float conversion; otherwise raise
the existing conversion TypeError using PyValue_TYPE. The context-limited
variant retains its existing NaN-payload and context/status semantics. Keep
owners of object-space inputs alive across reentrant operations and do not
project an inline operand merely to type-check.

`dec_new` must retain a genuine tp_new object entry for external object ABI and
have a separate value-native vectorcall entry, using the established Clinic
constructor body/vectorcall mechanism. The current selective marker rejects
new/init functions; extend the generator with an explicitly reviewed selective
constructor capability that pairs these two entries, or implement an explicit
paired wrapper if Clinic cannot express it. A true object caller may project an
inline result at its actual thunk; internal vectorcall takes borrowed values
and returns an owned value. Do not mark the unit or emit a fake object bridge
as a value path. Preserve subtype allocation and new/init dispatch, optional
NULL versus explicit None, keyword errors, context-before-value validation,
FloatOperation/traps and callback/reentrant lifetimes. `Context.create_decimal`
is an ordinary selective Clinic method and calls the value helper directly.
`Context.__init__`/`__replace__` are a separate multi-argument packet, not
silently covered by this constructor.

## Decimal numeric slots

For each existing numeric entry, supply both legacy object slot and the
corresponding `_Py_value_nb_*` spec slot, using `_Py_VALUE_THUNK_UNARY`,
`_Py_VALUE_THUNK_BINARY`, `_Py_VALUE_THUNK_TERNARY` or
`_Py_VALUE_THUNK_INQUIRY` at the real legacy boundary. Core value functions
accept borrowed values and return an owned PyValue, or scalar inquiry result.
Do not change type dispatch, reflected operation priority or legacy slot
metadata. Use a value-aware module-state finder based on PyValue_TYPE for all
operands in the same left/right/modulus order as the present
`find_state_left_or_right`/`find_state_ternary`; retain base type token refs
and existing subtype/error invariants. An object is projected only after a
Decimal type check. NULL modulus and None must retain the runtime's documented
ternary-slot conventions.

Port in bounded groups:

1. The six `Dec_BinaryNumberMethod` calls (add, subtract, multiply, true
   divide, remainder, floor divide): resolve context as before, use a
   value-aware conversion that distinguishes unsupported operand
   (PyValue_NotImplemented, no exception) from conversion failure
   (PyValue_NULL, exception), hold and release each converted Decimal exactly
   once, and preserve status/traps. Confirm reverse and Decimal-subclass
   dispatch against the C reference.
2. divmod and power: same operand/result ownership; `_PyTuple_FromPairTakeValue`
   consumes q and r even on tuple failure. Power handles optional modulus and
   unsupported third operand with identical NotImplemented/exception rules.
3. unary plus/minus/absolute and bool: checked known Decimal self; value
   results or scalar inquiry return. Preserve context lookup/status.
4. int/float and rounding-related methods: first implement
   `dec_as_long_value` directly from mpdecimal, with PyValue_FromInt64 on the
   successful i64 path and the existing PyLongWriter for genuinely large
   values. Port `__round__`, ceil/floor/trunc and nb_int together, and port
   as_integer_ratio's numerator/denominator value storage in the same coherent
   layer. Keep the specified round NULL/None and size/overflow rules in
   decimal-rounding-proposal.md. Float results use a value-native scalar path
   while preserving sNaN, infinity, signed zero and rounding. Do not hide a
   fresh primitive object construction behind object-to-value conversion.
5. richcompare: value-aware Decimal/int/float/Rational conversion and
   comparison status, paired `_Py_value_richcompare` plus object thunk;
   preserve unsupported result, FloatOperation/traps, operand order and
   subclass/reflected dispatch. Treat hash/repr/str contracts separately.

Before implementation, verify exact `_Py_value_*` slot availability/signatures
and constructor pairing against current integration headers and Clinic
producer; propose any missing shared interface for independent review. Each
layer gets marked unit compile, targeted C tests and peer post-review. Include
NULL/None, inline extrema, large int, bool/int subclass, Decimal subclasses,
reflected overrides, third-argument pow, NotImplemented, signal/trap and
injected allocation/constructor failures. Full Decimal suite and final
R27 box checks are coordinator integration gates.
