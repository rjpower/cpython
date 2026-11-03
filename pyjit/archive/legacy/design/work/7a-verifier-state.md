# Active collector verification state for test children

The queue landing review found that its subprocess helper selected `-I`,
so parent `PYTHON_GCVERIFY=kill` did not configure most child tests. The
first repair supplies a small test interface shared with the roots lane.
It has no queue dependency and can land before the queue repairs.

`_PyGC_TraceGetVerifyMode(interp, &extra)` reads the interpreter's collector
state. The return is 0/off, 1/retain, or 2/kill; extra bits are 1/pymem and
2/structs. `_testinternalcapi.gc_verify_mode()` returns `(mode, extra)`.
The non-mimalloc implementation returns `(0, 0)`. No environment variable
is echoed and the accessor neither collects nor changes configuration.

`test_verify_mode_state` starts fresh processes with explicit off, retain,
and extra-bit combinations, and kill combinations on debug builds. Its
negative control supplies retain with both extras to an isolated process
and asserts that the actual state is off. The queue helper will use this
interface in the next fixup; this commit alone does not repair forwarding.

Validation is on the patch above merge `36562302832`, which brings
integration `7af7ce89618` into `boil/7a`, with the existing default and
`--with-pydebug` builds and unchanged flags. Logs live under
`build/queue-review3/`:

- `getter-gate0-default.log`: rebuilt, gate 0 passed, 1,924 tests.
- `getter-gate1-default.log`: focused contract test passed.
- `getter-check-unbuilt.log`: required syntax checks passed for all seven
  module families; `_decimal` checked with both functional configurations.
  The script still reports its established no-header modules explicitly.
- `getter-gate0-debug.log`: rebuilt, gate 0 passed, 1,924 tests.
- `getter-gate1-debug.log`: focused contract test passed, including all
  four kill/extra-bit combinations.

The old-helper forwarding control fails with `ignore_environment=1` and
`isolated=1` under a kill-configured parent; retained output is
`verifier-forwarding-control.log`. Actual-state assertions are added only
after this accessor is available. No root-safety or performance claim is
made by this test interface.
