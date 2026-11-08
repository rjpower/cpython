# Decimal Context binary family: source post-review

Reviewed frozen [`decimal-context-binary.diff`](decimal-context-binary.diff). **Approved; no must-fix finding.** The 21 decorated Context methods match all 21 `DecCtx_BinaryFunc` call sites and generated `METH_VALUE` entries. Their wrappers retain the checked Context object and lend both operands as values; the shared macro raises the same TypeError for an unsupported operand.

The first converted Decimal remains owned while the second is converted. If that conversion fails, the first is released. Allocation and status failures release both operands and any allocated result; success transfers the Decimal result into an owned value. The patch leaves the two context-free binary methods, ternary method, and object-form numeric slots untouched for separate review.

Terra reports extension compile/link, 70 Context tests, direct comparison of all 21 methods with `_pydecimal` for inline/large ints, bool, int subclass, Decimal operands and invalid operands on both sides, plus an Inexact trap probe. Merged integration remains the coordinator's gate.
