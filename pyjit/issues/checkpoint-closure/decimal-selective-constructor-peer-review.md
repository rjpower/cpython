# Selective constructor: coordinator peer review

Approved `decimal-selective-constructor-proposal.md` for implementation,
subject to the registration and subclass checks below. It reuses the existing
whole-file value constructor parser and both established entry conventions;
it does not duplicate argument parsing or invent a core object bridge.

Keep selective constructors distinct from selective methods. Reject unsupported
shapes before mutating converter state, work on render copies, and preserve
whole-file value emission. Only `__new__` with explicit vectorcall support is
in this packet. The object tuple/kwargs entry is a genuine compatibility
boundary; the type's value-vectorcall entry returns an owned value directly.

The registration plan follows the exception-type setup in
`Objects/exceptions.c`: publish the paired object thunk/value vectorcall only
after successful type construction. `PyType_Type` supplies the callable's
vectorcall offset. Do not add unrelated instance flags or layout fields.
The type-inheritance code copies vectorcall offsets but does not generally
copy a type object's own constructor vectorcall function. Verify that mutable
Decimal subclasses take the normal type-call path, including overridden
new/init and later attribute changes, before asserting this invariant in the
generator. A failed invariant requires a reviewed checked fallback, not
disabling subclass tests.

Post-review requires deterministic regeneration, mixed marked/unmarked
fixtures, negative decorator cases, both object and value calls, defaults and
keyword failures, converter ownership failures and subtype callbacks. The
constructor conversion helpers and Context initializer remain separately
tracked; this producer capability alone does not admit Decimal.
