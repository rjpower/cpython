# W5 AST and IO post-review

The AST-only patch in `w5-ast.diff` has no must-fix finding. The generated converter now retains each list item returned by `PyList_GetValue` throughout recursive conversion, and retains the parent list through its owned `tmp` value. Every normal field exit clears `tmp` after closing it; failure labels therefore cannot close the preceding field's stale owner. Recursive-call entry failures close the item before reaching the parent cleanup label. Object fields transfer one projected owned reference to the AST arena, with a matching close if registration fails.

The existing `test_ast.AST_Tests.test_none_checks` runs the seven failing AST-field cases sequentially in one process. That sequence reproduced the stale-owner crash before the `tmp` reset and now passes. Luna regenerated the AST sources, built all extension modules, and ran `test_ast` and `test_compile`: 426 tests passed, 3 skipped. No new test that mirrors the implementation is needed.

The two substitutions in `w5-io.diff` also have no must-fix finding. Buffered seek uses the existing value-native offset converter used by nearby seek paths; FileIO passes its live borrowed name value directly to the value-native attribute setter. Terra's release build and focused `test_io` run passed: 1,066 tests, 28 skipped.

The follow-up FileIO audit proposal is sound if the enabled path constructs an owned Unicode mode and tuple, releases both on every exit, and checks the same `(name, mode, flags)` audit event shape. `_PySys_AuditEnabled` is an existing guard for avoiding audit-only materialization; `PySys_AuditTuple` forwards an already-built argument tuple to hooks.

The implemented `w5-io-audit.diff` satisfies those conditions, with no must-fix finding. The mode and tuple are decref'd on every success and failure path, and the audit hook sees the same flat three-item tuple. Terra's focused release run passed `test_audit` and `test_io.test_fileio` (134 tests, 4 skipped); a direct file-descriptor test observed `(0, 'rb', flags)`.
