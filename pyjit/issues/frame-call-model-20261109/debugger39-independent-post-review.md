# Debugger test-port post-review

The source39 diff is limited to `Lib/test/test_sys_settrace.py` and
`Lib/test/test_pdb.py`. Eleven original trace-jump methods now explicitly
expect a precise refusal at their attempted transfer while retaining their
unmodified function bodies. The shared checker verifies one attempt, the
expected `ValueError`, unchanged PC, line, code, local identities and output,
then lets the original function complete its normal cleanup. Three additional
controls check manager entry/exit and await order, handled-exception identity,
and a yielded payload's resume/finally path. The Pdb doctest expects its
documented immediate call-line stop; a real interaction test checks both
backends' public position and single continuation/binding.

The selected 14 trace methods passed in release and debug with the same
source-diff hash. Release `test_sys_settrace` ran 483 tests with one skip and
nine remaining ordinary errors; `test_pdb` ran 247 tests with 12 skips and
passed. Debug `test_pdb` independently passed 247 tests with 12 skips. The
remaining nine trace errors are outside this test-only packet. The combined
release command therefore exits nonzero; it is not a full debugger pass.

I accept the source and bounded executed evidence for the test-port commit.
No interpreter permission, Pdb runtime behavior or general trace-transfer
support was added by these two test files.
