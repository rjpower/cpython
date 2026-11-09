# Decimal remaining queue

This is a source census of W5 after the selective Clinic generator, the
conversion helpers, the 65 original polymorphic macro methods, Context manual
methods and the in-review Decimal copy/sign pair. It is not a completed
value-native or no-box claim. Recount against the integration source after
applying the ordered patches; the W5 core mark has intentionally not been
asserted.

1. **Decimal instance unary and predicates.** Nine `Dec_UnaryFuncVA` methods
   remain: exp, ln, log10, next_minus, next_plus, normalize, sqrt,
   logical_invert and logb. Two `Dec_BoolFuncVA` methods (is_normal,
   is_subnormal) also take optional context. Eight context-free `Dec_BoolFunc`
   methods are is_canonical, is_finite, is_infinite, is_nan, is_qnan,
   is_snan, is_signed and is_zero. Reuse the reviewed optional-context value
   helper where appropriate, preserve status behavior and return value-native
   Decimal or tagged bool. Audit the precise Clinic aliases and test wrong
   contexts before marking each method.
2. **Decimal manual methods.** `quantize` needs borrowed exponent, rounding,
   context and exact validation order; a proposal is in review. The related
   `to_integral_value`, `to_integral` and `to_integral_exact` share optional
   rounding and context semantics but must preserve each status policy.
   `__round__` takes an optional polymorphic ndigits and needs its own integer
   conversion/overflow audit. `number_class` and `to_eng_string` take optional
   context and return strings. `__format__` accepts format and override values
   and requires a focused format/error/ownership design. `from_float` and
   `from_number` are classmethods accepting polymorphic values; verify exact
   subclass dispatch and FloatOperation behavior. Include methods emitted as
   direct METH_O (without `_impl`) when auditing; the signature census below
   only identifies `_impl` functions.
3. **Constructors and Context APIs.** Decimal `__new__`, Context `__init__`
   and `__replace__`, `create_decimal`, and `create_decimal_from_float` need
   full argument/default/type/conversion ownership designs. Constructor and
   initializer slot signatures differ from ordinary METH_VALUE wrappers;
   apply the established paired vectorcall/body scheme, not the selective
   method marker. Check the context lifecycle, exception messages, traps,
   float/int subclasses, contextvar builds and subclass constructors.
4. **Numeric and protocol slots.** Decimal's heap spec has add, subtract,
   multiply, remainder, divmod, power, negative, positive, absolute, bool,
   int, float, floor-divide and true-divide; richcompare, repr, str and hash
   also need separate contracts. Paired value slots and legacy object thunks
   must preserve reflected operation and subtype ordering, NotImplemented,
   inexact conversion/traps, owned result transfer and slot invocation
   semantics. Inventory Context and signal-dict slots/getsets independently;
   a `PyObject *` concrete Context or Decimal self is not evidence that a
   polymorphic argument or returned integer/float can be projected safely.
5. **Generated and module closure.** Regenerate Clinic from producer inputs;
   reconcile selective-value and value-core generator changes. Then perform a
   marked `_decimal` compiler census for all remaining legacy API calls and
   non-value slots, port module setup and error paths, and audit callback,
   GC, vectorcall and allocation failure ownership. Measure raw startup and
   suite attribution without relabeling. Only integration may run full
   `test_decimal`, all-extension build, debug suite and final R27 gates.

A parser over unmarked Clinic blocks with at least two `PyObject *` impl
parameters found Context `__init__`, `__replace__`, `create_decimal_from_float`,
`create_decimal`; Decimal `__new__`, `__format__`, the three to-integral
methods, `__round__`, the nine context-taking unary methods, two context-taking
predicates, `number_class`, `to_eng_string` and `quantize`. This is an aid,
not an exhaustive API inventory: direct METH_O functions, slots, generated
converters and nested helpers need independent review.
