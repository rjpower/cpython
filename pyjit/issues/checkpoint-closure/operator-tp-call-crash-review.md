# Operator direct `tp_call` regression

The `af82c03c4e5` full release suite crashes in `test_pyvalue_operator_readers` at `legacy_call`: `type(functor).__call__(functor, target)`. This exercises each callable type's object `tp_call`, rather than its normal vectorcall route. My earlier operator post-review missed this dispatch distinction.

All three new `_operator` type specifications set `Py_tp_call` to `_PyValue_TpCallThunk`. That thunk reads the type-level `_PyType_VALUE_SLOT(type, call)` and invokes it without a null fallback (`Objects/call.c:1616`). `itemgetter`, `attrgetter` and `methodcaller` instead store paired **instance** vectorcall functions in their objects; none has a type-level `tp_value.call` row. In a release build the thunk invokes a null function pointer. The ordinary `f(value)` path uses the instance value-vectorcall field and therefore passed focused tests.

Change the three `Py_tp_call` entries to `PyVectorcall_Call`. Its existing object entry (`Objects/call.c:249`) duplicates tuple/dict value words into an owned call window, dispatches `_PyValue_InstanceVectorcall`, and exits through the genuine object boundary. It reads the paired instance value field and preserves direct `type(f).__call__` semantics. Do not change the generic `_PyValue_TpCallThunk` or invent a type-level call row.

`Lib/test/test_pyvalue_operator_readers.py` already exercises `legacy_call` for all three types, including methodcaller with keyword and scalar arguments. Re-run this focused module in release and debug after the source freeze; no duplicate test is needed. This is a must-fix before admitting the operator packet.
