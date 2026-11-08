# Decimal Context unary family: source post-review

Reviewed frozen [`decimal-context-unary.diff`](decimal-context-unary.diff), which changes `_decimal.c` and its generated Clinic header. **Approved; no must-fix finding.** Exactly 15 `DecCtx_UnaryFunc` call sites now have `@value_function`, 15 generated method entries use `METH_VALUE`, and every implementation receives a checked Context object plus borrowed `PyValue` argument.

The shared converter projects only proven Decimal objects, retains the Decimal before use, and accepts inline/heap integers (including bool and int subclasses) through `PyDecType_FromLongExact` with the base Decimal type. Unsupported types keep the existing TypeError text via the value's type. The unary macro releases the converted operand on allocation and status failures, releases the result on status failure, and transfers a successful result into an owned value. No other object-form Decimal methods are changed by this patch.

Terra reports extension compile/link, 70 focused Context tests, direct comparison of all 15 methods with `_pydecimal` for inline/large ints, bool, int subclass and unsupported float/string, plus an Inexact trap probe. This is a source and focused-evidence approval; merged integration remains the coordinator's gate.
