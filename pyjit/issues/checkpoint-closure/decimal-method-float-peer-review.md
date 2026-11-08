# Decimal method and float-chain peer review

The corrected `decimal-method-census.md` has 65 macro-family Clinic methods:
14+1+2 Decimal methods, 15+21+1+2 Context methods, and nine Context
predicates. The hand-written polymorphic methods and numeric slots are
separate. This is an appropriate bounded inventory for the next design
increment, but the proposed per-method Clinic opt-in does **not yet exist**
in the generator: `is_value_function` currently selects only the file-wide
`codegen.value_emission` flag. Define one explicit per-function marker (for
example `@value_method`) rather than silently changing all wrappers. It must
be rejected for slot/getset functions, legacy optional groups and converters
without a value form. Render and parse logic must use the same per-function
decision, including aliases of `Decimal.compare`; verify the generated
method flags, self/defining-class types, polymorphic argument types, return
type and error exits across all 65 methods. Their concrete Decimal/Context
receivers and optional Context parameters remain proven heap objects; only
polymorphic operands become borrowed values. Each impl must return owned
`PyValue` (or explicitly wrap an owned Decimal object), and the conversion
macros must preserve `NotImplemented` versus TypeError and close temporary
Decimal owners on every branch. Keep constructors and the listed hand-written
methods outside this bulk annotation until individually reviewed.

The proposed float conversion chain is sound with these ownership rules.
`PyFloat_AsDouble` already reads a float subclass's stored value rather than
calling its override. The cached `float.__abs__` yields an exact float, so
`PyValue_FromDouble(fabs(x))` preserves that bypass for finite values; compute
the sign with `copysign` first, preserving negative zero. Keep the existing
NaN/Infinity branches and status handling before `fabs`. Both
`float.as_integer_ratio` and `int.bit_length` are currently `METH_VALUE`
methods. `_PyValue_CallMethodNoArgs` can therefore carry the exact float and
denominator as values without an object thunk. Hold the owned ratio tuple
through two owned `PyTuple_GetValue` reads, then close it; hold the owned
denominator through the bit-length call and close it afterward. Extract the
owned bit-length result with `_PyValue_IntAsSsize_t`, close it, decrement `k`,
and pass the still-owned numerator as a borrow to the approved long
conversion before closing it. Every error exit must close exactly the owners
already acquired. The corrected `float.as_integer_ratio` producer now retains
its own numeric intermediates as values, so it does not force the old Decimal
side to box small numerator/denominator results.

Focused comparisons should include float subclass overrides of `__abs__`
and `as_integer_ratio` (neither should run), `+0.0` and `-0.0`, the two
infinities, NaN, a subnormal, a finite number whose ratio has a large
denominator, and a context with Inexact trapped. The object-call boundary
and zero-box assertion should be checked separately from result equality.
