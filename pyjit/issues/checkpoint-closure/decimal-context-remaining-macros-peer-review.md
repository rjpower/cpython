# Remaining Decimal Context macro methods: peer review

Approved the bounded plan for 12 methods: nine `DecCtx_BoolFunc` or `_NO_CTX` predicates, two `DecCtx_BinaryFunc_NO_CTX` total comparisons, and one `DecCtx_TernaryFunc` (`fma`). Each should be individually decorated for selective value Clinic output. The existing value converter supplies owned Decimal operands; a failed later conversion releases earlier owners.

Predicates release their converted operand and return immortal `PyValue_True` or `PyValue_False`. The two context-free comparisons use the context only for conversion and must not acquire new status/trap handling. `fma` retains its existing context and status behavior, releasing all converted operands and any allocated result on failures. Decimal object-form slots and hand-written methods remain outside this increment. Focused comparisons should check invalid types at every operand position and trap behavior for `fma`.
