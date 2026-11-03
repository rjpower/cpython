# M2 unsupported-fixture correction review

ACCEPT source readiness for a separately authorized fresh28-method debug campaign. Exact clean3174bc0ab726cb7b7b39f25d93f4ec9194c88d3a, parent3bcb891956a9d2796bbe5e92c4f0c9e2f2516899; only Lib/test/test_jit_method.py changes.

The one permitted method now builds a tuple and calls callback(*args). Actual compiler starred-call lowering uses CALL_FUNCTION_EX; frozen helper classification4 remains NULL/unsupported while BUILD_TUPLE49 has a real helper. The callback appends to a local ledger, checked exactly[7] immediately after the unsupported method returns9. Existing affected-only inactive/disabled/recompile-refusal and unrelated native-entry/result assertions remain. This restores the intended fallback witness as supported coverage grows; it changes no runtime policy or adapter.

Independent AST projection confirms all28 IDs unchanged,27 other method bodies exact and the entire module exact after replacing only the allowed method. The one-path commit, exact patch/source, four artifact hashes and six authority references pass. Old3bcb worktree remains clean with its original test bytes; its failed13PASS/1FAIL/14not-run outcome is preserved. No compiler or candidate ran. Actual corrected behavior, including all14 natural methods, remains pending; no old failed outcome is reclassified.
