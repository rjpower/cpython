# Independent PMU collector peer at source32

Disposition: approved for coordinator preflight and capture. Reviewed the actual
`capture-workload-perf.py`; no candidate or perf process was executed by A.

The collector keeps the sealed FIFO implementation and its outside-interval
native conditions, raw outputs, load/timeout guards and 99% event-running gate.
It binds current clean source32/golden, configured builds, native probes,
117-module logs, ELF build IDs and retained SHA hashes to the repeat identities.
Extension and input fingerprints are compared before/after capture. Record mode
additionally checks the expected executable build ID and lost-event records.

Source review found and resolved these concrete issues before approval:

- Fixed-length twelve-character native revisions rejected the actual
  eleven-character identities. Exact recorded native git plus validated
  full-HEAD prefix now anchors the check.
- Stable current files alone did not bind calibration to its historical inputs.
  All 109 recorded driver-source hashes are now checked and included in capture
  snapshots.
- A command reconstructed from its own row loop count did not validate registry
  workload shape. Actual registry loops, path and variant now participate.
- Collection failure now retains postflight evidence; extra post-interval
  validation failures mark their raw invocation invalid.
- Recursive build sysconfig bytecode caches and benchmark caches participate in
  the input fingerprints.

Independent host checks passed: AST parsing; actual repeat calibration with
subprocess execution forbidden (24 rows, 28 samples per role/case); mocked
successful and failed campaign status, including rejection of incomplete/failed
campaign completion. `git diff --check` passed. An initial host invocation raced
the function-signature edit and raised TypeError; rerunning with the final
explicit roster argument passed. No production change was made for that harness
error.

This approves collection mechanics, not a PMU result or causal hypothesis.
Root must inspect native preflight, raw counter scheduling, status and identity
stability before using counts. Per-loop values remain in the sealed invocation
records; paired ratios and CPI can be derived without changing the measured
interval.

## Coordinator preflight correction

The first real collector preflight failed before any native invocation:
`waits, load, expired = ...` made `load` local throughout `collect()`, so its
initial module-loader call raised UnboundLocalError. The earlier source review
and mocked collect/main status check missed this. The failed output remains at
`ninth-workload-perf-stat`.

H renamed that local to `load_average` and its forwarded argument. A reviewed
the actual narrow correction, then independently executed the **real collect
body** with external operations stubbed: six correctly balanced invocations,
two snapshots and the real child-metadata check passed. Subprocess entry points
were forbidden and ELF copies disabled. The correction is approved for a fresh
coordinator preflight; no FIFO, timing or validation policy changed.
