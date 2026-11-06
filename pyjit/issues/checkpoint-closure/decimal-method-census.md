# Decimal value-wrapper inventory and proposal

This inventory describes the W5 `_decimal.c` source after the signal/context
increment. It is not an admission claim: the module remains object-form and
must retain its non-native status until all value slots, wrappers, producers,
module initialization and box accounting pass review.

The following 65 Clinic-generated methods share polymorphic conversion macros:

- Decimal binary (14): compare, compare_signal, max, max_mag, min, min_mag, next_toward,
  remainder_near, logical_and, logical_or, logical_xor, rotate, scaleb,
  shift. Ternary (1): fma. Binary without context (2): compare_total,
  compare_total_mag.
- Context unary (15): abs, exp, ln, log10, minus, next_minus, next_plus,
  normalize, plus, to_integral_value, to_integral_exact, to_integral, sqrt,
  logb, logical_invert.
- Context binary (21): add, compare, compare_signal, divide, divide_int, max,
  max_mag, min, min_mag, multiply, next_toward, quantize, remainder,
  remainder_near, subtract, logical_and, logical_or, logical_xor, rotate, scaleb, shift. Ternary (1):
  fma. Binary without context (2): compare_total, compare_total_mag.
- Context predicates (9): is_normal, is_subnormal, is_finite, is_infinite,
  is_nan, is_qnan, is_snan, is_signed, is_zero.

Additional hand-written polymorphic Clinic implementations using conversion
macros are Decimal.copy_sign, same_quantum and quantize; Context.divmod,
 power, _apply, copy_abs, copy_decimal, copy_negate, number_class,
 to_sci_string, to_eng_string, copy_sign and same_quantum. Decimal power,
 arithmetic and comparison use number/richcompare slots, not these method
 wrappers. Constructors, create_decimal, float conversion and methods with
 scalar/optional arguments require their own argument-by-argument audit; this
 list does not certify them.

Proposed interface for peer review: explicitly opt each polymorphic Clinic
method into the existing value-emission machinery, keeping its concrete
Decimal/Context receiver as a checked object but carrying each polymorphic
argument and return as owned/borrowed PyValue according to existing Clinic
contracts. Update conversion macros to consume borrowed values and preserve
NotImplemented versus TypeError rules. Produce legacy object entry thunks only
at actual external C boundaries; retain existing signatures, defaults and
subclass dispatch. Generate changes from Clinic input/producer, not generated
headers alone. Do not introduce a global directive that silently converts all
Decimal methods: audit constructors, optional arguments, return types and each
manual function first. Paired value number and richcompare slots require a
separate inventory and review.

`PyDecType_FromFloatExact` currently calls the cached float absolute operation
and `as_integer_ratio` through object APIs. Replacing just `PyFloat_AsDouble`
would retain object-call boundaries and could change float-subclass behavior. Its complete
value-native call and ratio extraction chain requires a separate reviewed
change, retaining exact signed zero, infinities/NaNs, status/traps and
subclass semantics.

The Decimal heap-type slot table (`dec_slots`) has object-only richcompare,
repr, str and hash plus numeric add, subtract, multiply, remainder, divmod,
power, negative, positive, absolute, bool, int, float, floor-divide and
true-divide. The binary/ternary operations accept polymorphic numeric operands;
int/float slots can return inline values. Their paired value slots and legacy
object entry thunks must be designed together, preserving reflected dispatch,
subclass precedence and NotImplemented; repr/str/hash and bool have distinct
result contracts. `signaldict_richcompare` and the constructor/new slots are
separate object-boundary audits. This is an inventory, not evidence that any
of those slots have been ported.
