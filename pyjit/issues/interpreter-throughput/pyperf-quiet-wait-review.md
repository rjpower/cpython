# Pyperformance quiet-wait source review

2026-11-07. **ACCEPT** the actual bounded prelaunch-wait change in
`pyjit/bench/measure_pyperformance.py` and the four controller fixtures in
`pyjit/bench/tests/test_measure_pyperformance.py`. No source-review blocker
remains. This is not acceptance of new subset results. Runtime source/builds
remain frozen at `7780e32e0e2`.

The failed first v3 subset is excluded in its entirety. Its final prelaunch
load sample exceeded four before command nine; its postflight retained clean,
unchanged build/source identities. The completed 261-cell kernel stage remains
accepted and is not rerun. Root must declare the fresh subset directory and use
that exact directory in analysis, without pooling the failed subset's prefix.

The actual implementation matches the reviewed proposal:

- A timed command starts a monotonic deadline from the caller's remaining
  budget. Quiet-wait samples retain full load tuples, timestamps, elapsed time
  and the exact command in the existing fsynced JSONL journal.
- Waiting resumes below three, with sleeps bounded by ten seconds and remaining
  budget. The unchanged hard guard at four is sampled again before launch.
  A spike at this last check returns to the same bounded wait, without starting
  or retrying any native command.
- Expiry records a wait-timeout and prevents launch. The final deadline check
  covers journal work, and `communicate` receives the remaining budget after
  process creation. Waiting does not provide another full command timeout.
- Each native command still runs at most once. Nonzero exit, timeout and result
  validation failures remain fatal; no values or failed-child results are
  selected or replaced. The 24-invocation order and benchmark settings remain
  unchanged. RSS callers with no timeout retain their immediate guard.

The four fixtures cover busy-to-quiet launch with reduced timeout, expiry with
no launch, a final-guard spike returning to wait, and native failure without
retry. The author reports four passed. The reviewer read their actual source
and the implementation diff and ran `git diff --check`; it ran no Python test,
native workload, build or profiler. The coordinator's independent focused
controller check is the execution gate before the fresh subset.

The journal's `start` phase precedes `Popen`; as before, it records a launch
attempt rather than independently proving a child executed. Completed native
counts must reconcile finish/validated-result records and actual result files.
No threshold relaxation, within-child load filtering or performance conclusion
is introduced by this harness change.
