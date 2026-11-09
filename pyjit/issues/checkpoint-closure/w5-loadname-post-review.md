# LOAD_NAME value-port post-review

## Initial review, superseded

I initially approved the patch after checking owner transfer, lazy-import reification and the generated instruction. That review missed the difference between direct dict lookup and generic mapping lookup for a globals dict subclass. The correction below supersedes that approval; the patch was not landed.

## Corrected review

The four-file `w5-loadname.diff` has one must-fix finding. `_PyEval_LoadName` now returns an owned value directly to the generated `LOAD_NAME` instruction. Its `value_load_from_dict` helper holds found owners across lazy-import reification, but using it for globals changes dict-subclass behavior. The prior globals path calls `PyDict_GetValue`, which bypasses an overridden `__getitem__`; the helper's `PyAnyDict_CheckExact` fast path excludes subclasses, so it invokes the override. For example, after locals miss, `exec('x', G(x=1), {})` with `G(dict)` whose `__getitem__` raises should still resolve `x`. Keep globals on `PyDict_GetValue`, then reify its owned result with globals as the namespace. Locals and builtins may use the helper. Missing locals and missing name retain their prior errors. The bytecode source and regenerated cases both consume the owner; no old object-result call remains.

Terra's release build and focused scope/import/compile run passed 373 tests, 4 skipped. The generated cases should be regenerated again if the bytecode source changes during integration.

Separately, the initial SystemExit value-setter implementation in `Python/errors.c` had one semantic edge: after a custom exception constructor returns a different `BaseException` subtype, delegating to `_PyErr_SetObject(tstate, exception, exc)` would call the original constructor a second time. The existing normalization accepts any `BaseException` result. Delegating with the actual result type, `(PyObject *)Py_TYPE(exc)`, keeps the result normalized and retains the old chaining and traceback path. Terra was notified before integration.
