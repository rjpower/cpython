# Selective Decimal constructor generation

The existing whole-file Clinic value-emission path already has the required
constructor body and two entries; it is not currently available selectively.
Add an explicit `@value_constructor` marker only for `@classmethod __new__`
with `@vectorcall`, initially reject `__init__`, limited API, optional groups,
aliases, deprecated parameters, unsupported converters and duplicate markers
before any converter mutation. Reject use with `@value_function`; never inherit
the marker through a Clinic clone. Apply the same one-file/core-only output
and fragment/header guard rules as selective methods.

In `Function`/DSL parser retain a separate `value_constructor` flag;
`is_value_function` returns true for this flag only after confirming METHOD_NEW
and vectorcall eligibility. Both renderer (`CRenderData.value_emission`) and
`ParseArgsCodeGen` must use this common predicate, and operate on render copies
so undecorated functions and clones cannot inherit converted converter state.
The existing `handle_value_new_or_init()` then emits `dec_new` as the true
object tp_new entry: it borrows the canonical tuple window, expands keyword
values into owned scratch when needed, calls `dec_new_value`, frees scratch,
and `_PyValue_THUNK_EXIT`s at the actual object caller. The existing
`value_vectorcall()` emits `dec_vectorcall` consuming the caller's borrowed
value window and returning an owned value; its type/immutable assertions and
error propagation remain intact. Both feed the same generated argument parser
and `dec_new_impl`, so defaults, NULL sentinel, None, keyword duplicates,
subclasses and error messages cannot diverge between paths. Preserve the existing
`value_emission` file-wide behavior and test both decorators and aliases.

Decimal's immutable heap type is created in `_decimal_exec` via
PyType_FromMetaclass. After successful creation and before publication, pair
its `tp_new == dec_new` with `tp_value_vectorcall = dec_vectorcall` and the
object `tp_vectorcall = _PyValue_VectorcallThunk`, following the existing
builtin exception-type setup. Confirm by tests that mutable Decimal subclasses
never inherit this immutable-type-only fast entry, and that overridden __new__
and __init__ dispatch through the normal path; if runtime inheritance does not
provide this guarantee, use a checked value-vectorcall dispatch/fallback design
and review it before landing. Do not change flags or memory layout without
proving runtime calling rules. Object result remains a concrete Decimal unless
subclass behavior returns another type; forward the generated owned result
without classifying a private call as an external thunk.

Selective constructor tests must generate, compile and execute one fixture
with both object tuple/kwargs and value vectorcall paths, checking zero/one/two
args, NULL-default versus explicit None, positional/keyword/duplicate errors,
unknown keywords, value ownership on allocation/converter failure, subclass
new/init overrides, and mutable subtype fallback. Compare against the prior C
Decimal constructor, not just `_pydecimal`. Add generator negative fixtures
for missing vectorcall, non-new use and incompatible markers. Regeneration
must be deterministic. The target method then borrows `value`/`context`,
validates context first using optional_context_value and calls the approved
PyDecType_FromObjectExact value helper; no separate hand-written parser.
