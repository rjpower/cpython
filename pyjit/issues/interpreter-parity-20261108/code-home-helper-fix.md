# Frame code home helper discovery fix

The full suite at runtime `1c8774797f3` reported that
`_testinternalcapi.test_frame_code_home()` requires one argument when called
without one. `Lib/test/test_capi/test_misc.py` collects every `_testinternalcapi`
name beginning with `test_` into a `unittest.TestCase`; its test runner invokes
those functions without arguments. The frame code home helper is `METH_O` and
needs a mortal code object supplied by `test_pyvalue_api`.

In worktree `interpreter-parity-20261108` at `10627c339fa`, renamed the C
function and exported method to `check_frame_code_home` and updated its Python
caller. This preserves the ownership check while avoiding automatic no-argument
discovery. A repository `rg` before the edit found only the C definition,
method table entry and Python caller; after the edit those three locations use
the new name and no old-name source usage remains. `git diff --check` passed.
No build or tests were run; the coordinator owns integration and validation.
