# Decimal Context remaining macro methods: source post-review

Reviewed frozen [`decimal-context-rest.diff`](decimal-context-rest.diff). **Approved; no must-fix finding.** It annotates exactly nine predicates, two context-free total comparisons and `fma`; the generated header has 12 matching `METH_VALUE` method entries.

The predicates convert and release their operand, then return the immortal `PyValue_True` or `PyValue_False`. The total-comparison methods preserve their context-free mpdecimal operation and have no new status handling. `fma` releases earlier converted operands when a later conversion fails, releases all operands on allocation/status failure, and releases the result on status failure before transferring a successful result as an owned value.

Terra reports extension compile/link, 70 focused Context tests, and comparisons of all 12 methods with `_pydecimal` across integer boundaries, bool/int subclasses, invalid operands in every position and an Inexact trap. Merged integration remains the coordinator's gate.
