# Deque and bytes iterator value follow-up

Baseline: main runtime worktree `pyvalue-rc-universal` at `f81ebbd646dcf4f01c091b79fc109468bd98d19a`, with the separately frozen `w5-manual-deque-iterator.patch` applied first. The follow-up adds nine remaining `_collectionsmodule.c` sites: value-aware deque subtype construction, deque rich comparison with value-native iteration and a paired richcompare slot, and `_count_elements` lookup/iteration/call/update paths. It also ports the fast sequence and generic iterator paths in `Objects/bytesobject.c` to retain values through integer conversion.

Validation of this original follow-up source: `_collectionsmodule.c` compiled cleanly. `Objects/bytesobject.c` reports three earlier marked object-API errors outside these functions. A locked W5 shadow build imported all 117 modules with no failures; `test_collections` and `test_bytes` passed (469 tests, 10 skipped). W5 was restored and rebuilt under lock.

The original patch is preserved unchanged after review. Apply `w5-manual-deque-bytes-review-fixes.patch` immediately after it to preserve bool conversion semantics, preserve the default-zero identity path, check failed maxlen conversion, and add its regression.
