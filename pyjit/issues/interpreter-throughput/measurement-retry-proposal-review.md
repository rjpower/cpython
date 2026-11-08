# Environmental measurement retries: proposal review

2026-11-06. **ACCEPT the bounded proposal with the classification and completion
rules below.** This reviews `pyjit/bench/run.py` and
`pyjit/issues/checkpoint-closure/measure.py`; it does not accept an unreviewed
implementation or any resulting performance measurements. Runtime/builds stay
frozen. The reviewer ran no test, benchmark, build or profiler.

The runner samples load before measurement and again after all measurement
work. At load >= the configured limit it raises `MeasurementError`, prints
the following line to stderr, returns one, and writes no report:

```text
run.py: 1-minute load average N.NN reached --max-load 4.0; no measurement report written
```

The command's `--max-load 4` parses as float, hence `4.0` in this message.
A rejection at the last load sample can follow one ordinary timing-progress
line for the current role/kernel. Thus a single-line-only matcher would miss
legitimate environmental rejections, while substring matching arbitrary stderr
could disguise a real runtime failure.

## Accepted retry rule

- Retain the declared three-round, 29-kernel, three-role rotation. Each cell
  accepts its **first** zero-exit, valid report and is never run again merely
  because another cell rejected load. The same in-progress campaign can retain
  that prefix; old failed campaign directories are not resumed or pooled.
- Before every attempt, wait for the same one-minute load <3; retain full load
  tuples/timestamps and keep runner rejection >=4. No threshold relaxation.
- Retry only return code one, no report file, empty stdout, and stderr exactly
  matching the anchored runner message above, optionally preceded by the one
  expected normal timing-progress line for this exact role/kernel. Permit the
  normal terminal newline, not arbitrary additional messages or tracebacks.
  Check the message's configured limit, not only its load number or a substring.
- Store a numbered attempt record and distinct command/stdout/stderr/return-code
  artifacts before deciding to retry. Retain the classification, report path,
  pre/post load and quiet-wait observations. Never overwrite a successful report
  or replace a previously accepted cell. A report present on a failed invocation
  is a contradiction and fatal; a missing/malformed zero-exit report is fatal.
- Every other error is fatal: crash, signal, traceback, unsupported operation,
  timeout or non-guard measurement failure. Do not catch such failures and
  quietly advance to another cell.
- The same 45-minute global budget covers quiet waits and all attempts. Check
  the deadline before starting another subprocess and bound an invocation by
  the remaining budget. Expiry fails collection; retain its accepted prefix and
  attempts, but write no complete roster aggregate. Final postflight/status
  evidence must still be preserved. Only all 261 accepted cells plus unchanged
  identities/errors policy permit the completed diagnostic summary.

This is environmental censoring under a declared rule. The post-measurement
guard can reject work that has already run; load may correlate with elapsed
time. Retaining the journal makes that visible, but does not turn the final
sample into an unbiased estimate of all host conditions. Classification must
never compare timings or retain the fastest attempt. Timings printed by a
rejected invocation are not aggregate inputs.

## Minimum checks and source review

Use small subprocess/clock fixtures, not benchmark workloads: exact rejected
load then first accepted report; a real crash remaining fatal; deadline expiry
starting no next subprocess; preservation of an already accepted earlier cell;
and rejection of a guard-like traceback or a failed invocation with a report.
Cover both pre-measurement one-line and post-measurement progress-plus-guard
forms. Do not add a retry framework or change runner/hot-workload behavior.

Root must peer-review the actual small harness diff and executed fixture
evidence before starting a fresh v3 directory. Keep both earlier failures and
their identities intact. This proposal changes collection resilience, not the
workload roster, accepted-value rule, runtime source, or interpretation of the
final candidate/baseline ratio.
