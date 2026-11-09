# Decimal sign and unary families: coordinator post-review

Reviewed the frozen `decimal-instance-sign.diff` and
`decimal-instance-unary.diff` layers after their approved Context/instance
families. No must-fix finding. This review is independent of the implementer;
merged qualification remains outstanding.

The copy-sign/quantum methods resolve context before operand conversion, keep
each converted Decimal owner until the operation finishes, release the first
owner if the second conversion fails, and release the result on trapped status.
`same_quantum` returns a tagged boolean; `copy_sign` transfers its Decimal.
Selective Clinic emits actual value entries and borrows optional arguments.
The concrete Decimal receiver is the only unconditional object projection.

The 19 further unary/predicate methods share three bodies. Eight context-free
predicates return immortal booleans. Two context-sensitive predicates validate
the optional context before querying it. Nine unary operations preserve their
mpdecimal operation, Context status handling and result cleanup. The generated
wrappers retain aliases, defaults and keyword rules through the reviewed
selective Clinic producer.

Terra reports extension compile/link, focused sign/quantum probes, and 89
focused tests plus all-method comparisons for the unary/predicate family,
including special values and optional contexts. These are worker checks,
not the coordinator's full Decimal or R27 admission.

The rounding and string/classmethod proposals are also approved for
implementation. Their required ordering is context validation, rounding
validation, then operand conversion; quiet/exact integral methods keep their
different status policies. Classmethod subclass calls keep the owned base
Decimal alive and return the callback's arbitrary value directly. `__round__`
and the integer conversion consumers require a separate coherent paired-slot
layer; projecting a small integer to retain an old object local is not allowed.

Source post-review also approves the subsequent frozen `decimal-rounding.diff`
and `decimal-classmethods.diff`. The rounding error helper shares the existing
message rather than rebuilding an object API. All result/error paths preserve
the approved ordering and null result contract. The classmethod helper consumes
one nullable owned base Decimal, transfers it directly for the base type, or
closes it after the subclass callback; an arbitrary callback value is returned
without boxing. String buffers are freed after Unicode construction even when
that construction fails. Terra reports a combined extension build, 109 focused
tests and probes for subclass inline returns, conversion/trap behavior and
combined-invalid argument ordering. Merged validation is still required.
