# Decimal instance macro methods: source post-review

Reviewed frozen [`decimal-instance-macros.diff`](decimal-instance-macros.diff). **Approved; no must-fix finding.** Exactly 17 macro-family Decimal methods opt in to `METH_VALUE`, with 17 matching generated entries. The optional-context helper rejects inline and wrong-type values before converting operands, preserves the existing TypeError, and uses the established `CURRENT_CONTEXT` ownership path for `None`. A supplied valid Context remains borrowed from the generated argument window.

Each macro owns the converted Decimal receiver and polymorphic operands until mpdecimal has used them. Later conversion, allocation and status failures release earlier owners and any result. The two total comparisons remain context-free at the mpdecimal call, and the ternary method retains status/trap handling. Success transfers the Decimal result into an owned value.

Terra reports extension compile/link, 90 focused Context/usability tests, and all 17 methods checked for inline/large ints, bool and int subclasses, invalid operands, omitted/None/explicit Context, plus an Inexact trap. Inline invalid context was checked against the existing C TypeError contract because `_pydecimal` accepts it in at least one method. Merged integration remains the coordinator's gate.
