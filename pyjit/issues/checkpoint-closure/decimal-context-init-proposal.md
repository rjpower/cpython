# Decimal Context initialization and value getsets

Context numeric attributes currently have object-form getters/setters; even
where setters immediately call a value extractor, a core descriptor call has
already projected its value. `context_setattrs` also takes eight object
arguments. The Context constructor, `__replace__`, and `localcontext` all use
it. These must migrate together; an object-to-value helper inside the existing
setter alone does not close the core path.

Introduce value getter/setter bodies for prec, Emin, Emax, capitals, clamp
(and EXTRA_FUNCTIONALITY status/traps/allcr) using the same mpdecimal range,
error and status logic. Getters return PyValue_FromInt64/UInt64 (or a real
error); setters borrow the value and use the existing value integer extractors.
Use `_Py_VALUE_GETSET` for paired descriptor entries, whose shared object thunk
is only the actual external getset boundary. Preserve deletion semantics:
PyValue_NULL passed by a delete must produce the same exception as current
object setter; None is an actual value except where the caller explicitly
normalizes it. Rounding remains an object-owned Unicode property but accepts a
borrowed value through `getround_value`; preserve Unicode subclass behavior
and exact error text without projecting inline invalid values. flags/traps
remain pointer-space list/dict/signal-dict objects; check value type before
projection and retain the value owner across any reentrant truth callbacks.
For invalid inline values reproduce the existing type/error path rather than
blindly treating it as a pointer. EXTRA_FUNCTIONALITY's integer mask inputs
need value-aware long_as_flags and the existing flags/error mapping.

Refactor `context_setattrs` to borrow eight values. NULL sentinel means leave
unchanged; make `Context.__init__` explicitly normalize None to NULL exactly
as `NONE_TO_NULL` currently does. Preserve setter order prec, rounding, Emin,
Emax, capitals, clamp, traps, flags and stop at the first failure; this order
is observable if conversion invokes user code. `Context.__replace__` and
`localcontext` use NULL for omitted and must pass explicit None through as
today, not normalize it. Migrate all three callers in the same layer.

`__replace__` and localcontext can use selective method emission. Context's
`__init__` is an initializer slot, so do not apply the __new__-only selective
constructor marker. First extend the producer with a separately reviewed
selective initializer marker using existing `handle_value_new_or_init` and
`value_vectorcall`: it must preserve tp_new then tp_init ordering, init's int
error result and ownership of the newly allocated instance. Register its
value vectorcall only on the immutable Context base with correct object thunk
and prove mutable subclass new/init overrides take normal dispatch. No
hand-written second parser. Keep current context-copy/template behavior and
conditional contextvar/non-contextvar branches.

Tests cover getter and setter inline numbers, huge values and overflow,
bool/int subclasses and __index__ behavior, delete versus None, every pair of
invalid fields to verify ordering, flags/traps list/dict mutation and
reentrant callbacks, copy and localcontext ownership, defaults versus explicit
None, subclass constructor overrides and both build configurations. Generate
and compile Clinic fixtures; use focused Context classes before integration's
full Decimal suite and box gates.

## Concrete selective initializer producer

Add a separate `@value_initializer` paired with `@vectorcall`, valid only for
METHOD_INIT and mutually exclusive with the selective method/constructor
markers. Reject limited API, groups, aliases, deprecated parameters and
unsupported converters before mutating render copies. Do not inherit its flag
through clones. Feed the existing shared `is_value_function` predicate so both
parser and renderer choose value forms; retain the same one-core-file guards.

Existing `handle_value_new_or_init` emits the actual object `context_init`
tp_init boundary: it borrows tuple items, expands owned keyword scratch,
calls `context_init_value`, frees scratch and returns its integer status. The
existing `value_vectorcall()` calls `type->tp_new(type, empty_tuple, NULL)`,
checks for NULL, calls `context_init_value` with the original borrowed value
window, decrefs the fresh instance if initialization fails, otherwise returns
its owned value. This is valid for the exact immutable Context base because
`context_new` ignores args and kwargs and always allocates the requested
Context instance. Register only that base's `tp_vectorcall` thunk and
`tp_value_vectorcall` after type creation; prove mutable subclasses with
custom or reassigned __new__/__init__ take normal dispatch. If the runtime
can call this entry for a different `tp_new`, add and independently review a
checked fallback before admission. The existing vectorcall assertion checks
`tp_init` and immutability but does not itself assert `tp_new == context_new`.

Annotate only Context.__init__. Generated value body retains self as known
Context object and borrows eight arguments, defaulting each to PyValue_None.
Normalize None to PyValue_NULL only in this entry before calling the
value-native `context_setattrs`; `__replace__` and localcontext keep their
separate NULL default semantics. Preserve init's int status and flags/traps
lifetimes. Generate/compile fixture for both entry paths, keyword windows and
failure cleanup; include negative marker tests and mutable subclass tests.
