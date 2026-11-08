# Actual allocation-stress state and fixture restoration

`gc_verify_mode()` reports verifier mode and extras, not allocation stress.
The stress interval is a process-wide `stress_every` in gc_live.c. Moreover,
`gc_alloc_in_reftracer()` used to set it to 1, then unconditionally to 0.
Its test runs in the test_gc worker, so later tests silently lost an
inherited PYTHON_GCSTRESS=1000 setting. Environment text cannot prove that
setting survived. This standalone shared prerequisite has no queue-logic
or queue-helper change.

`_PyGC_LiveGetStress(void)` reads the configured interval directly;
`_testinternalcapi.gc_get_stress()` exposes that integer, with 0 meaning
off. It does not read the environment or set the mode. The reftracer
fixture saves the actual prior interval and restores it on its existing
common cleanup path, including the allocation-error break. A tracer-install
failure occurs before the interval changes.

`test_stress_mode_state` observes explicit 0 and 1000 in fresh children,
and 0 with -E despite an environment value of 1000. Its restoration child
starts with inherited 1000, calls the fixture without a preparatory mode
setter, then asserts the same interval remains. The existing direct
reftracer test also asserts unchanged state around the fixture.

Negative-control provenance: above exact base `d43c8b4dd32`, first install
only getter and tests, keeping the old fixture's zero-reset line and no
saved interval local. Rebuild the existing debug configuration, then run
`gate.sh 1 build/debug test_gc -m test_stress_mode_state`. This failed with
regrtest status 2 and the child assertion `(1000, 0)`. Only afterward add
save/restore. Exact negative source is retained as
build/queue-review3/stress-negative-source.patch; its log is
stress-old-restoration-negative.log. No mode setter masks the forwarding
or restoration oracle.

Positive validation uses unchanged existing flags, gcc 15.2.0, release
-O3 and debug -Og; logs are under build/queue-review3:

| Gate | Result | Log |
|---|---|---|
| default rebuild/smoke | 1,924 tests, 50 skips | stress-positive-gate0-default.log |
| debug rebuild/smoke | 1,924 tests, 20 skips | stress-positive-gate0-debug.log |
| default focused | 2 tests | stress-positive-focused-default.log |
| debug focused | 2 tests | stress-positive-focused-debug.log |
| debug focused, kill | 2 tests | stress-positive-focused-kill.log |
| debug focused, stress 1000 | 2 tests | stress-positive-focused-stress.log |
| supported unbuilt modules | seven families, decimal twice | stress-positive-unbuilt.log |

The coordinator approved this shared prerequisite separately from queue
landing, with the same independent-integration procedure as the verifier
accessor. Normal full-suite evidence at queue head d43c8b4dd32 retains its
original provenance: both runs had ambient stress 0, and restoring their
actual prior 0 has the same behavior as resetting to 0. Collector routing,
marking, sweeping and ordering are unchanged; the getter is read-only.
The added contract is covered by focused default/debug runs. This is the
coordinator's affected-test-helper rerun ruling, not a claim that the full
suite was rerun on this prerequisite. Full plain-kill, ASan and stress
verification follows shared integration; no uniform stress11 claim is made
from the earlier environment-only runs.


A build-only worker wrapper was also qualified. It reads gc_get_stress()
at entry and in finally around runpy(worker), records the actual test list,
filter, profile, PID, verifier extras, interpreter flags and source/config
hashes, and never calls a mode setter. The two-test real worker passed
with stress=1000 at both ends (stress-worker-positive-v2.log/.jsonl).
-E fails at actual stress=0 before running the worker; a deliberately
missing unittest module still exits 1 while both witnesses remain 1000,
so the finally check does not turn failures green. Those controls are
stress-worker-E-negative.log, stress-worker-failure-control.log and
stress-worker-control.jsonl.

The first positive wrapper attempt failed in diagnostic code after its
two tests passed: the end record read sys.argv after runpy had changed it.
It is retained as stress-worker-positive.log and supplies no successful
end-state evidence. Capturing the config hash before runpy repairs the
wrapper; the second run has exactly one start/end pair with matching
current source hashes. Wrapper sources remain in task-owned build
storage for the coordinator's shared-manifest work:

- assert_stress_worker.py: SHA256 8c6383ca06b4d8de7ae30317eb526915da802600a551bc735981222a95859c31

- assert_stress_worker.sh: SHA256 93b6fd3b19a2c83e1f0c84baf18e40e1d6ee3e1132f7cee002ea2919fe41e8ad
