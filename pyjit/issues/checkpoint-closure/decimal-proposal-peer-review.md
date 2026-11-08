# Decimal value-core proposal peer review

The five-increment plan in `decimal-value-core-proposal.md` is a sound order
for a large extension, with one design decision required before the broad
method port. The 47 compiler diagnostics are a useful finite first sweep,
not an estimate of the object-slot or Clinic adapter work. Keep that
distinction explicit in the checkpoint gate.

The first signals/context increment is approved. In `dict_as_flags`, each
`PyDict_GetValue` result must remain owned through `_PyValue_IsTrue`, because
`__bool__` can run Python and mutate the dictionary; close it after the call
on both success and error. `list_as_flags` similarly owns each list item
through `exception_as_flag`; object-only signal classes may be projected only
after proving the item is an object, and inline values must take the same
invalid-signal path as before. `PyDict_SetValue` and `PyList_AppendValue` are
appropriate for stored values. Keep the five context setters' type, range,
and trap error mapping intact.

The conversion plan correctly identifies the central invariant: an int51
can enter `dec_from_long` through the existing `mpd_qset_i64` branch without
boxing. The current function uses `PyLong_Export` only to choose between that
branch and digit import. A direct inline branch must pass the same context
and status pointer; heap integers retain the current export/free path. Exact
float conversion should read inline floats as scalars and keep the existing
special-value, sign, context and exception behavior. Bool remains an object
subtype of int and must follow the existing acceptance/dispatch rule.

Before increment four, enumerate the actual polymorphic Clinic methods and
choose a concrete wrapper strategy for each. A file-wide `value_emission`
would change object-only methods and is unsuitable. `value_core` can remove
marked scalar calls inside existing object-signature wrappers, but cannot
make an adapter deliver an inline Decimal operand. Prefer the existing
`METH_VALUE` machinery with explicit value wrappers for the bounded
polymorphic set; add a per-function Clinic directive only if that census
shows a repeated form worth generating and its ownership contract is
specified. No generated output should be edited by hand.

For paired heap-type numeric slots, test direct object-slot callers as well
as value dispatch. Each object thunk must consume or project the value result
at its true compatibility boundary; the value slot must keep all operands
borrowed through libmpdec work and callbacks. Preserve `NotImplemented`
versus error, ternary absent-modulus handling, subtype dispatch, context
traps, and comparison signaling. A clean `Py_VALUE_CORE` compiler pass is
necessary but does not certify these semantics or zero CORE/THUNK boxes.
