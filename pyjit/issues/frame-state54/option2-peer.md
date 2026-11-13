# Option 2 independent peer review

Source reviewed against selected51, not executed. Approve the proposed
implementation subject to root's normal source/native/semantic gates.

The allocation in `Python/ir.c` puts words immediately after `_PyCodeExec`;
`_PyIRStaticExec` and `ceval.c:entry_trampoline` have the same prefix. The
executing-code owner pins ordinary image lifetime independently of mutable
function code. Prefix derivation therefore preserves authority and adds no GC
edge. The base sentinel has no words and cannot use this accessor. Keep that
restriction at existing cold diagnostic boundaries rather than adding a guard
to every feedback operation. Retain the direct prefix path: deriving through
the function's code would be wrong as well as more expensive.

One documentation correction is required: Astra's F section says interpreter
exit/base sentinels lack the prefix. The exit trampoline **has** the prefix
(`ceval.c:1077`); only the base sentinel lacks it. The proposal already states
this correctly. This does not block its implementation.

No unresolved source-contract blocker found. Native review must still count
the dependent entries loads per feedback access and verify changed home/header
offsets. Removing a cached header pointer is not an unconditional work saving.
