# Call record cleanup: source post-review

2026-11-07, runtime baseline `7780e32e0e2`. **Accepted for integration.** The
reviewed uncommitted diff is confined to `Python/ceval.c` (1 insertion,
11 deletions). I did not edit runtime source, build or run tests.

The diff removes the write-only `_PyCallTransaction.effects` field, its
initializer and five writes. It also corrects the comment that claimed the
record helpers tracked effects. `call_record_expand_method` still converts the
helper's negative status to error; `prepared`, monitoring and the actual
helper call remain. No guard, refresh, owner transfer, publication, error path
or provider invocation changed. The generated handler's live `CALL_EFFECTS`
protocol is untouched. There is no source finding to fix before validation.

The coordinator's retained `build/pyjit-link-wave1/interpreter-architecture-20261107/r1-checks.json`
command journal records passing release/debug builds, each with 117 modules and zero failed
imports. The retained release and debug test logs each report 916 tests run,
one skip, and all five focused files successful (`test_call`, `test_ir`,
`test_pyvalue_native_call_transaction`, `test_sys_settrace`,
`test_monitoring`). The coordinator reports no new warning lines relative to
the prior C build. This is a code clarity cleanup; it does not establish a
speed improvement or authorize removing `CALL_EFFECTS` from generated
handlers. The coordinator owns integration admission.
