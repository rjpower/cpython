# Profiling failure investigation

Runtime source: `4b3d739d867b06ea4c9ed936982cfeb3503e93be`, unchanged throughout
these controls. No runtime repair was made.
The first final debug suite ran 54,421 tests and reported one failure in
`ProfileTest.test_calling_conventions`: profiling
`max(*([0],), **dict(key=int))` did not produce the expected `:0(max)` entry.
Its printed statistics also lacked the executed module entry. Missing raw
exec/max events are an inference from those statistics, not an observed event
trace. The cause remains unexplained.

The original receipt remains in
[`boundary-final-v1-debug-full.log`](/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/execution-boundaries-20261108/boundary-final-v1-debug-full.log).
The failing section is at lines 998–1063; the recorded seed is `1284068202`.
Source review:

This statement follows the existing instrumented `CALL_FUNCTION_EX` path into
`CALL_EX_NON_PY_GENERAL`, outside the new discarded Python-child selectors.
The native route retains its numeric destination and CALL/C_RETURN actions.
The new bool aliases are disabled in debug builds. No defect in the new
metadata, transaction flag, or return-completion code has been established.
`CProfileTest` inherits the same `test_calling_conventions` code object as
`ProfileTest`, but supplies a different profiler class. Ordinary unittest module
discovery finds CProfileTest first; randomized execution can change the order.
The additional probe exercises this shared code with both providers.
Completed controls:

Evidence is under the retained build directory
`pyjit-link-wave1/execution-boundaries-20261108/`:

- `profile-module-controls/`: golden debug and candidate debug each passed all
  481 whole-module tests.
- `profile-calling-controls/*-stress.json`: each of golden/candidate release and
  debug passed 600 statements using fresh Python Profile instances.
- `profile-calling-controls/*-actual-fixture.json`: each of those four roles
  passed 1,000 actual unittest method invocations, CProfile first then Python
  Profile, sharing the inherited code object. This is 6,000 statement executions
  per role, or 24,000 across the four roles.

The [probe](probe-profile-events.py) records raw dispatcher events, string versus
precompiled replays, physical/canonical IR, and public monitoring state only
when a failure occurs. All controls passed, so no diagnostic raw trace exists.
These passes neither explain the first failure nor establish baseline flakiness.
The repeated candidate debug suite completed **54,421 tests with zero failures**
in 10 minutes 23 seconds. `test_profiling` passed in 65 seconds at 01:59, using
explicit `--randseed 1284068202`; unlike the original default run, this retry
randomized execution order. The receipt is `boundary-final-debug-repeat-v2-debug-full.log`.
This is a full pass on retry with the earlier failure retained, not a
demonstrated repair. If the failure recurs, retain the
actual fixture and failure-only raw diagnostics before proposing a runtime fix.
