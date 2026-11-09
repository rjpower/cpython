# Environmental measurement retries: source review

2026-11-06. **ACCEPT** the final `measure.py` implementation and eight-fixture
snapshot. No remaining source-review blocker to a fresh diagnostic campaign.
This accepts the collection protocol, not performance results or qualification.
Runtime source and builds remain frozen.

The review covered `pyjit/issues/checkpoint-closure/measure.py`,
`pyjit/bench/run.py`, and `pyjit/bench/tests/test_checkpoint_measure.py`.
The runner's native child has no independent timeout; killing only the Python
driver would therefore have been insufficient. The final implementation starts
each driver in a new session. Timeout or interruption sends group TERM, allows
two seconds for output collection, and sends group KILL in `finally`, even if
the driver already exited or grace-period collection was interrupted. It then
reaps the driver, preserves output and classification, and raises the original
failure. Timeout and interruption never trigger environmental retries.

## Accepted behavior

- Only exit one, absent report, empty stdout, and the exact anchored load-guard
  message permit retry. The sole optional preceding line must be the runner's
  formatted timing progress for this exact role and kernel. Extra output,
  tracebacks, wrong roles, and failed invocations with reports are fatal.
- Each scheduled cell retains its first valid positive finite timing report.
  Accepted earlier cells are not rerun. The three-round, 29-kernel, three-role
  rotation, quiet-wait threshold below three, runner guard at four, and global
  45-minute timing budget are unchanged. Deadline checks reject late successes
  and prevent another launch after expiry.
- Numbered attempts retain command, report path, classification, full stdout
  and stderr, full pre/post load tuples, and quiet-wait observations. The full
  invocation journal is replaced atomically before launch and after outcomes.
  This explicitly amends the proposal's separate command/stdout/stderr-file
  wording: complete strings in each numbered JSON record satisfy that evidence
  requirement without redundant files.
- Launch errors, malformed reports, runtime failures, timeout and interruption
  remain fatal. The outer postflight path remains in `finally`; a failed
  campaign cannot reach the complete-roster aggregate writer. A new output
  directory excludes both earlier failed campaigns and their partial prefixes.

The eight fixtures cover rejection then acceptance with prefix retention,
non-guard failure, failed invocation with a report, strict progress/output
classification, timeout group cleanup, interruption during cleanup, late
success, and deadline expiry after rejection. The coordinator independently
reported **8 passed** on this frozen snapshot. This reviewer inspected the
fixtures and source and ran `git diff --check`; no test, benchmark, build or
profiler was run by this reviewer.

## Interpretation limits

The runner can reject load after measurement has already occurred. This is
declared environmental censoring, not proof of an unbiased host sample.
Rejected progress timings remain evidence only and never enter aggregates.
No attempt is selected by elapsed time. Cleanup may extend beyond the timing
deadline solely to stop an aborted process group; it accepts no late result.
Final measurement review must still verify all 261 accepted cells, unchanged
identities, raw attempt history and the absence of errors before interpreting
the diagnostic ratios.
