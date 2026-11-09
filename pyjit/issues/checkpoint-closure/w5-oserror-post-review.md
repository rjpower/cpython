# OSError value-home post-review

The frozen `w5-oserror.diff` has no must-fix finding. The parser owns each tuple element in `OSErrorArgs` and clears every owner on both constructor paths and their failures. On Windows, it accepts the same long-compatible `winerror` values, constructs a replacement errno and tuple before swapping the args, and closes both old and new owners on the right paths. Each OSError payload is an owned value home; `_Py_T_VALUE` members, traversal, clear, `_ssl`, `_io`, string and reduce paths use the new layout consistently.

The string path snapshots all four format operands before calling user code, so a field mutation during `__str__` or `__repr__` cannot invalidate later operands. Its missing-field and Windows precedence conditions match the prior branches. The SystemExit follow-up converts only an inline non-int code to its string before the existing raw printing path; it closes the code owner and preserves the subprocess `1.25` output.

Terra's focused release run passed `test_exceptions`, `test_sys`, `test_ssl`, `test_io` and `test_weakref`: 1,664 tests, 59 skipped. This review did not run a Windows build. The separate `_json` shared-module link issue still needs resolution during integration.
