# Diagnostic perf collection proposal

This collector gathers bounded hardware-counter and callchain evidence for the
three kernels already selected for source-line analysis: `deltablue`,
`logging_silent`, and `comprehensions`. It uses existing upstream and candidate
builds. It does not build, change, or qualify either interpreter. Every output
is marked diagnostic and ineligible for the R27 gate.

## Inputs and identity

The caller supplies the medians file and both configured build executables.
Before launch, the collector compares each live executable's ELF build ID with
the retained symbolization ELF and its saved build-ID note. It runs the live
configured executable so its standard library and extensions resolve from the
correct build tree; retained ELFs are symbolization evidence only. A fresh
output directory is mandatory. Commands, environment policy, build IDs, raw
counter output, child timings, stderr, exit codes, and failures stay in that
directory. A postflight identity failure is recorded and makes the collection
invalid.

The child environment removes inherited `PYTHON*` settings and `LD_PRELOAD`,
sets `PYTHONHASHSEED=0`, and uses `-S -B` with a unique pycache prefix. The
collector does not use `LD_PRELOAD`. The coordinator must confirm JIT is off
and module origins before launch. The collector preserves each child command
and its warmup/setup measurements for that review.

## Measured region

Each child imports and sets up one benchmark, runs its established warmup, then
waits for a `perf` control FIFO. The parent keeps both FIFOs open with
nonblocking read/write anchors to avoid startup deadlocks; close-on-exec and
`close_fds` keep those anchors out of `perf` and the child. The child writes
`enable\n`, waits up to ten seconds for exactly `ack\n` or `ack\n\0`, runs the
hot region, then writes `disable\n` and waits for the matching acknowledgement in a
`finally` path. Ack timestamps and latency, hot loop count, wall time, process
CPU time, child return status, and exceptions are retained even when the
disable exchange fails. Control and Python bracket work occur while counters
are enabled; their fixed overhead is part of these raw counts. The separate
empty-region calibration estimates that overhead but does not subtract it.
The first host control check returned `ack\n\0` for both control commands,
which the initial strict `ack\n` parser rejected. All 18 empty-run invocations
failed safely and produced no benchmark measurements. The parser now accepts
only those two exact byte sequences; result metadata retains the response
string and hex bytes, and rejects any other suffix or payload.

Stat mode uses three rotating groups, each explicitly user-only:

* `{cycles:u,instructions:u}`
* `{branches:u,branch-misses:u}`
* `{cache-references:u,cache-misses:u}`

It performs three rounds, rotating case, role, and group order. Per-role loop
counts use the median of the three supplied fresh timing samples and target
about two seconds of hot work. The optional `--empty` stat run keeps setup,
warmup, and control exchanges, but calls `run(0)`, sets the hot target and loop
count to zero, and records calibration counters without per-loop division.
Raw counts remain untouched; empty counts are never subtracted from workload
counts.

Record mode is a separate pass, targets about five seconds per role/case,
records `cycles:u` at 499 Hz with an 8 KiB DWARF stack dump and explicitly
user-only callchains (`--user-callchains`), and keeps build IDs in the
perf data without updating the user build-ID cache. A `perf buildid-list
--with-hits` receipt records the sampled module paths and IDs for later
symbolization. The collector does not symbolize callchains or infer callers.
The first recordings failed before their children because perf otherwise
requested kernel callchains at paranoid 2. The user-only probe and corrected
six-capture run succeeded; failed attempts remain separate evidence.

## Admission and failure handling

Each invocation is limited to 60 seconds. A timeout or interruption kills and
reaps the process group. Before every run, the collector records the 1/5/15
minute load tuple and waits until one-minute load is below 3, with a 45-minute
window. It rejects a sample if the pre- or post-run one-minute load is at least
4. Expired windows stop the schedule and remain explicit failures.

Every requested event must appear exactly once with its exact `:u` scope, a
nonnegative numeric count, positive event runtime, and at least 99 percent
running time. Unexpected, missing, duplicate, unsupported, or not-counted
events invalidate that invocation; values are never filled with zero. Status
also records incomplete schedules, command and child errors, load rejections,
and pre/post identity differences. No failed invocation is silently filtered.

## Scope and limits

The output is diagnostic evidence for later analysis, not a performance claim
or R27 qualification. The caller runs the empty calibration and positive-loop
collection separately, reviews all raw rows, and performs symbolization only
against the retained matching ELFs. This tool does not change benchmark
workloads, calculate speedups, attribute callchains, or decide whether a
runtime change is acceptable.

## Execution and review

Peer review and post-review accepted the bounded collector and its strict
response/event validation. The coordinator executed 18 successful empty
calibrations, 54 counter invocations and six user-only recordings, preserving
the failed protocol and permission attempts separately. The independent
[hardware evidence review](hardware-perf-review-6a0e4c12c17.md) reconciles raw
events, arithmetic, sample conservation, module identities and stack limits.
