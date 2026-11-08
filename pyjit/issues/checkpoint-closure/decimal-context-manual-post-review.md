# Decimal hand-written Context methods: source post-review

Reviewed frozen [`decimal-context-manual.diff`](decimal-context-manual.diff). **Approved; no must-fix finding.** Seven method entries, including the conditional `EXTRA_FUNCTIONALITY` `apply` clone, have matching value signatures and generated `METH_VALUE` entries. The clone forwards its borrowed argument to `_apply`. `_apply` checks the fallible `dec_apply` result before transferring it, and `copy_decimal` transfers the owned converted Decimal directly, preserving identity for an existing Decimal input.

The copy and sign methods retain converted operands through mpdecimal calls, release them on later conversion/allocation/status failures, and transfer successful results. `same_quantum` returns an immortal value boolean after releasing both operands. No unrelated object-form Context methods change.

Terra reports release extension compile/link, 70 Context tests, focused parity for the public copy/sign/quantum methods across integers, subclasses and invalid operands, `copy_decimal` identity and `_apply` Inexact trap probes. A separate `-DEXTRA_FUNCTIONALITY` syntax-only compile covers the conditional clone. Merged integration remains the coordinator's gate.
