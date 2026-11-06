# Measurement harness review, round 2

Source review on 2026-11-06 covers `measure.py` in this directory and
`../../bench/measure_pyperformance.py`. Runtime source and builds remain
frozen at candidate `6a0e4c12c17`, upstream `3a440b65695`, and interim
`eba351fee7d`. This review ran no builds, tests, or benchmarks and changed
only this receipt. The coordinator owns execution and result interpretation.

The corrected harness is accepted for diagnostic collection. There is no
performance conclusion in this review. The strict R27 census and kernel
gates remain failed; elapsed timing cannot clear either gate.

## Findings and corrections

1. **Pyperf result names:** the original reader required a `metadata.name`
   inside every benchmark record. Installed pyperf 2.10.0 moves metadata
   shared by all benchmarks into the suite's top-level `metadata`; a
   single-benchmark result therefore need not have benchmark-local metadata.
   The reader now merges suite metadata with benchmark metadata, with the
   latter taking precedence. This matches pyperf's loader and handles both
   single-benchmark files and SciMark's five results.
2. **RSS page warming:** the original enumeration searched for `lib.*`
   directly below the configured build directory. Actual extensions live in
   the directory named by `pybuilddir.txt`. The corrected code resolves that
   directory, selects the snapshot's SOABI suffix, and requires its count to
   match the retained module check's shared-module count. This excludes
   historical ABI artifacts without deleting them. It retains the interpreter
   and explicitly probed extensions, touches every page in the resulting file
   list, and saves that list and `fincore` output before RSS collection.

The coordinator reports that serialization checks using the installed
pyperf's `Run` and `BenchmarkSuite._as_json()` pass for one- and two-benchmark
files. The configured-directory/current-SOABI enumeration reports the expected
78 upstream, 78 interim, and 79 candidate shared extensions. These are
coordinator-reported checks, independently supported here by source inspection.

## Diagnostic attempt v3

Two earlier attempts stopped when the unchanged one-minute load guard reached
four, before completing their first round. Their evidence remains retained;
their partial samples must not enter a later aggregate.

Attempt v3 declares a quiet wait before every invocation: resume only when the
one-minute load is below three. Waits count against the existing 45-minute
timing window. Each wait sample records a timestamp, elapsed waiting time,
and the complete 1/5/15-minute load tuple. Completed waits accompany their
invocations; the current wait is also retained in `load-wait.json`. Deadline
expiry fails the attempt. Waiting depends only on host load and time.

The fixed three-round, 29-kernel rotation is unchanged. The child harness
still rejects a one-minute load at or above four; a child failure stops the
whole attempt and preserves postflight evidence. No prior samples are reused,
and no elapsed result controls retries or sample selection. The companion
runner also applies its load guard to the one-minute value while retaining
the full tuple.

## Integrity and limits

The inspected preflight records matching source/executable revisions, ELF
identities, module origins and module-check evidence. GCC, `-O3`, mimalloc,
and computed gotos match across the builds; shapes are configured in the
interim and candidate. Upstream JIT state is explicitly disabled. The current
candidate source contains no fork method-JIT probe or admission symbols,
consistent with the reported unavailable probe. Pyperf's worker environment
copies `PYTHONPATH`, and the runner explicitly inherits `PYTHONHASHSEED`.

Kernel summaries use baseline elapsed divided by candidate elapsed: values
above one favor the candidate. Interrupted attempts produce no completed
summary. Final review must still check all rounds, result rosters, load
records, before/after identities, pyperf source manifests, and RSS warming
receipts. Pyperformance and RSS remain separate diagnostic stages.

The kernel deadline is checked before an invocation and during quiet waits;
it does not kill an already running child. The final child could therefore
finish after the nominal window. Host load checks also cannot exclude every
short interference event. Retain sample spreads and avoid treating small
differences as established improvements. The zero kernel loop deltas do not
establish startup-inclusive zero boxing or whole-source R27 admission.
