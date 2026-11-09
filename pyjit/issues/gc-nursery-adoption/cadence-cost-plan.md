# Held cadence cost comparison

This makes the accepted elapsed-first cadence design concrete for helper
preparation. Independent review, successful focused native feedback, matched
fresh release products and exact helper activation still precede measurement.
The overall 2026-11-01 04:18:11 UTC deadline remains unchanged. This document
authorizes no build, workload, timing or additional diagnostic exposure.

## Fixed comparison

Keep three frozen source arms: historical pre-adoption `e9e10d5b503`, mandatory
adoption with automatic majors `ef0af95904b8`, and halfway cadence `c8cc7718ef3`.
If feedback changes source, review and bind the exact successor before any
scores. Historical source is a temporary comparison, never a runtime option.
Do not include the rejected constant-operand implementation.

Use matched GCC O3 configuration/tools and each image's own stdlib/extensions.
Reuse the accepted quiet supervisor, both machine slots and all three build locks,
CPU 0/sibling 8, fixed load cap 1.5 and recorded unchanged CPU policy. One
60-minute activation includes admission, witnesses, samples and cleanup. Build
preparation is a separate finite scope, outside the quiet window.

| Registered workload | Loops per call | Role |
| --- | ---: | --- |
| coroutines | 4 | GC-heavy primary aggregate |
| generators | 2 | GC-heavy primary aggregate |
| comprehensions | 6000 | GC-heavy primary aggregate |
| richards | 2 | Control |

Two rotating-order rounds, five fresh scored processes per arm/workload/round:
**120 sample processes, one timed call each**. Freeze literal arm/sample order
before activation. Each uses the existing module loader/setup and one untimed
warmup at its registered loops. Do not substitute five serial samples in one
process. Keep workload sources and timed `module.run(loops)` unchanged.

Add twelve separately declared origin processes, one per arm/workload, before
scoring: 132 processes in total. Origins use the same wrapper/setup/warmup with
zero timed samples, no retention batches, and their own verbose import/ELF
traces. They earn origin evidence only. No extra
counter probe, calibration, workload or repetition follows from this plan.

Before loader/setup/warmup, set threshold0 to exactly 32 MiB, enable automatic
GC and use OFF/N0 with no Python GC callbacks installed. Preserve natural
allocation debt from warmup into the hot call: no threshold reset or forced
major between warmup and timing. Preserve `max(threshold0, live_bytes)` majors.
Record the configured byte policy and actual available deadline observations;
do not call the threshold a constant measured heap size. Hardware instruction
counters remain unavailable under the already recorded permission failure.
Omit those diagnostics before scores, without replacement exposure or sysctl
changes.

## Outside-clock observations

A narrow wrapper may reuse the existing phase-snapshot pattern in
`build/gc-post-cleanup-performance-v2/profiles32/window_probe.py`. Time only
the original workload call, retaining native collection, checking, queue and
reclamation work that it causes. No Python pause callback or profiler runs in
the timing interval. Import diagnostic modules before setup/warmup.

Save pre/post setup, warmup and hot-call facts, with clear phase boundaries.
Native GC statistics and state reads allocate; their snapshots are bracketing
observations, not exact instruction-boundary debt values. Record each arm's
explicit schema:

- Historical: `gc.get_count()[0]` debt plus ordinary GC statistics. M/H/P are
  unavailable through its API; do not fabricate them.
- Adopted automatic majors: existing read-only D/P state and ordinary stats.
  Its allocation limit is a scheduling probe, not a new stored M field.
- Cadence: existing read-only D/P/M/H/pending state and ordinary stats.

Use ordinary per-kind cumulative collections, collected/candidate counts and
duration. Their deltas bracket observation work as well as the hot call.
Existing local `_remote_debugging.get_gc_stats` snapshots may retain all raw
ring records outside the clock. Native timestamps on the verified matching
monotonic basis identify records within the hot interval. The rings hold only
eleven young and three major records; detect counter gaps/wrap and report
incomplete event coverage. Do not add live polling or infer missing events.
Native failure checks remain separate correctness prerequisites. Ring duration
is cumulative; an individual retained interval is `ts_stop - ts_start`.
Those intervals and aggregate collector durations exclude later queue/finalizer
delivery, although the timed workload includes delivery that it causes.

Capture process peak RSS immediately after the timed call, before retention
probes. It includes startup/setup/warmup and is not hot-only RSS. Likewise,
`max_pause` is a process-lifetime high-water mark, not a subtractable phase
maximum. Retained event records supply individual native pauses for those
events only. `GCStatsInfo.heap_size` is pre-collection object count, not a
post-major retained count.

After hot observations, each of the 120 scored processes runs exactly three
unscored registered-loop batches, each followed by an explicit full major and
scalar retained-byte/tracked-count observation: 360 unscored workload calls
and 360 explicit majors. This is separate functional traffic, with no timing
reported. Observe gen2 `live_bytes` after that explicit major and the immediate
`len(gc.get_objects())` separately.
Drop each temporary `gc.get_objects()` list before the next batch/major; do not
retain the heap through the observer. Keep only small scalar result rows;
the helper review must verify their fixed footprint and observation order.
Predeclare a final-minus-first retained-growth bound of 32 tracked objects and
16 KiB, allowing the fixed result rows. Report every point and absolute values;
do not silently increase the bound after scores. This finite settling check
does not prove arbitrary workload/resource cleanup: focused native lifetime
and queue tests supply those separate witnesses.

## Decision

The primary ratio compares halfway cadence with adopted automatic majors.
Require a lower geometric mean of the three GC-heavy median elapsed times in
both rounds, with consistent signs and no material order/load/control drift.
No workload or Richards may regress by more than 2% in both rounds for this
default policy increment. Save every raw duration, per-row median/range,
absolute time and ratio. Five fresh samples and descriptive ranges are not a
confidence interval. Noisy or inconclusive data accepts nothing and authorizes
no automatic extra samples.

Stable retained bounds and finite old/resource reclamation remain mandatory.
Peak RSS within 5% is the simple envelope; any larger bounded increase requires
an explicit absolute-MiB versus elapsed-benefit decision before acceptance.
Unbounded retention or missed cleanup rejects the policy regardless of speed.
Instructions and text size diagnose results and have no arbitrary veto.

Report adoption versus historical elapsed separately. Explain or correct a
repeatable regression above 2% before acceptance, rather than hiding it in the
cadence aggregate. If halfway loses, reject that source increment and retain
mandatory admission/manual minors with automatic majors as the single policy.
Broad qualification then covers the one chosen coherent source.
