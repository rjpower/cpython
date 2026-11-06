# Decimal Context canonical methods: source post-review

Reviewed frozen [`decimal-context-canonical.diff`](decimal-context-canonical.diff). **Approved; no must-fix finding.** Both generated entries use `METH_VALUE`. Each implementation proves the argument is an object before `PyDec_Check`, retaining the exact existing TypeError for primitives and non-Decimal objects. `is_canonical` returns an immortal tagged boolean. `canonical` duplicates the borrowed Decimal value, preserving identity and ownership for Decimal subclasses.

Terra reports extension compile/link and direct checks for canonical identity, Decimal subclasses, inline int/float, `None`, bool rejection and the exact TypeError. Merged integration remains the coordinator's gate.
