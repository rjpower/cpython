# Decimal sign/quantum post-review

Reviewed `decimal-instance-sign.diff` against the current Decimal context, conversion, and Clinic contracts. No must-fix finding.

Both methods resolve the optional context before converting operands, as the prior `CONTEXT_CHECK_VA` path did. `optional_context_value` retains the borrowed-context behavior of both `CURRENT_CONTEXT` implementations. The first converted Decimal owner is released if the second conversion fails, and both owners are released after the mpdecimal call. `copy_sign` releases the result on status failure and otherwise transfers it as an owned value. `same_quantum` returns tagged bool values without changing its status behavior. The generated Clinic methods accept borrowed value arguments and preserve the concrete Decimal self object.

Terra reported successful extension compile/link and direct probes for inline and large integers, bool and int subclasses, invalid operands, and omitted, `None`, explicit, and invalid contexts. I inspected the source and did not rerun those probes.
