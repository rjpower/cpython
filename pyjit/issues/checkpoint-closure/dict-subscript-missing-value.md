# Dict subclass `__missing__` value call

`dict-subscript-missing-value.patch` changes only the missing-key callback branch of `dict_subscript_value`. It calls the owned `__missing__` method with the borrowed PyValue key and returns its owned PyValue result. The exact-dict miss still uses `_PyErr_SetKeyErrorValue`, and the special-method lookup and callback order remain the same. This addresses the measured 1,060,000 CORE key boxes at `Objects/dictobject.c:3696`.

The patch applies cleanly to the current main runtime preimage. A syntax-only compile of the patched C unit with existing release build flags passed. No runtime interpreter was changed for this packet. At integration, run focused `test_dict` and check a subclass `__missing__` with inline int/float keys and results, callback errors, and exact-dict KeyError. The stats recount should show the CORE box site removed.
