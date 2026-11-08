# Decimal Context unary family: peer review

The proposed 15 `DecCtx_UnaryFunc` methods can migrate together after selective Clinic value emission lands. Their generated wrapper must keep the checked Context receiver as `PyObject *`, lend `x` as `PyValueBorrow`, and return an owned `PyValue`. The shared operation still owns a converted Decimal object and must release it on allocation and status/trap failures before wrapping a successful result with `PyValue_FromObjectSteal`.

The new `convert_op_value` should project `v` only after `PyValue_IsObject(v)` proves it is a Decimal instance, then retain that object. `PyValue_LongCheck(v)` must include inline ints, heap ints, bool and int subclasses; use `PyDecType_FromLongExact(state->PyDec_Type, v, context)` so the result remains an exact base Decimal with the current context's error handling. Other types must raise the existing TypeError using `PyValue_TYPE(v)->tp_name`. Existing object callers of `convert_op` can stay on their current helper until their own migration.

Check all 15 generated declarations against the one shared macro, then run focused Context tests for inline boundaries, heap ints, bool/int subclasses, unsupported types and status traps. This review approves the interface and ownership recipe; it does not approve an implementation yet.
