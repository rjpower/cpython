# CJK codec value-call increment

Baseline: S worktree `build/checkpoint-bulk-20261105` at `e983958bedeca23a192f3a566d2f27b4a0c9a7db`.

The patch ports `_cjkcodecs.getcodec` to a `METH_VALUE|METH_O` entry and retains the codec capsule, imported factory, and factory result as `PyValue` across the call. It also makes the private multibyte error-handler call return an owned `PyValue`; only tuple results are projected to a known object after type validation, while the callback's position remains a value through integer conversion.

Validation: all seven affected translation units compile in S's build. In a W5 shadow build, 117 modules imported with no failures; `test_multibytecodec`, `test_codeccallbacks`, and six CJK encoding suites passed (486 tests run, 4 skipped). The two W5 source files were restored and the original W5 tree rebuilt afterward. The patch is awaiting post-review.
