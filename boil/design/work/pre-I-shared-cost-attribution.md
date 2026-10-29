# First shared-cost attribution

Completed bounded attribution, 2026-10-29. Four serial existing-product runs
passed; no build or source optimization was needed. The first elapsed closure 68da9ae2 gives upstream/fork
0.828243 overall. Diagnose three regressions and one winning object-protocol
control; do not rerun the 29-row score or tune individual rows.

| Existing row | Registered loops L | Upstream / fork | Reason |
| --- | ---: | ---: | --- |
| coroutines | 4 | 0.365 | Recursive coroutine creation, calls and resume |
| comprehensions | 6000 | 0.528 | Short-lived containers, generator expressions and calls |
| generators | 2 | 0.592 | Resume/creation over a tree retained by setup |
| richards | 2 | 1.897 | Winning object/attribute/call control |

Use the existing ce2ac release binary
`/home/power/code/cpython/.worktrees/boil-10a/build/default/python`
(SHA256 eafcda7a), not the newer bytes3a or isolated stats build. It retains
frame pointers and symbols. Reuse the first elapsed source/config/native
bindings; a later test-only source change needs its existing native bridge,
not another inventory campaign. Freeze driver7e61be5c, registryf8abc0b3,
the four unchanged workload files and the short commands/output below.
The unlanded candidate's known GC failures and fork-only -fno-gcse remain
limits. This is mechanism diagnosis, not correctness or throughput qualification.

## Scope and commands

Create one task directory Q under `build/checkpoint-i-baseline-binding/`.
Start with a host-only sampling/control probe. `/usr/bin/perf` exists and
perf_event_paranoid is -1; neither proves usable samples or callchains.
Use software cpu-clock:u to avoid depending on hardware PMU availability:

```sh
UV_CACHE_DIR=/home/power/.cache/uv /usr/bin/perf record -q \
  -e cpu-clock:u -F 199 --call-graph fp -o "$Q/host.data" -- \
  /home/power/.local/bin/uv run --no-project --python /usr/bin/python3 -- \
  python -c 'sum(i*i for i in range(4000000))'
/usr/bin/perf report --stdio --no-children -i "$Q/host.data"
```

Bound that harmless host probe to10s and check actual sample count, lost
records and symbol resolution. Also exercise enable/disable ACKs with a
host-only busy loop before relying on the documented perf control interface.
Installed perf-record(1) supports `-D -1 --control=fifo:CTL,ACK` and commands
`enable`/`disable`. If sampling or control is unavailable, run the same four
phase snapshots without perf and report **counter-only**; do not substitute
Python profiler overhead or an abort backtrace for native CPU attribution.

For a useful hot window, the only proposed code is a task-owned phase probe
using the existing driver's `load`, the same optional setup(None), and run(L).
The task-owned helper and exact launch are now frozen under
`build/checkpoint-i-baseline-binding/shared-cost-attribution-v1/`; neither
changes a production runner or workload. No new collector counters,
profiling framework, provenance service or general benchmark runner is needed.
Its exact operation is:

1. Import gc, time, json, the existing driver and _testinternalcapi; assert
   actual verifier off/extras0/stress0. Pre-open control/ACK endpoints.
2. Snapshot time, gc.get_stats()[2] and gc_root_stats()['total']; load the
   workload, call its setup(None) when present, and snapshot again.
3. Call run(L) once as the existing time driver's warmup; snapshot again.
4. Enable perf and await ACK, record t0, call run(L)50 times, record t1,
   disable perf and await ACK, then take the final snapshot. In counter-only
   mode omit the two control commands. Print raw phase snapshots, t1-t0,
   actual L/50 and mode; serialize outside the sampled region.

Fifty is an initial sampling budget, not a reliability threshold. Repeating
the original run(L) call preserves its call shape; it does not change loop
bodies or introduce a benchmark-specific operation. The first elapsed values
suggest about3–14 seconds of hot work per row. Counter boundaries include
their own getter/control overhead and the probe adds a Python frame; retain
these limits instead of claiming observer-free hot counts. No forced collect,
GC disabling, threshold/stress change or held queue probe is introduced.

After the probe/helper source is reviewed, the literal per-row command is:

```sh
# SLOT is one available normal slot, 1 or 2; outer TMPDIR is unset.
# ROW is one table row; create ROWDIR, its cache/tmp and its two FIFOs first.
/usr/bin/flock -n "/tmp/boilgate-slot.$SLOT" \
  /usr/bin/timeout -k 30 150 "$R/boil/tools/gate_run.sh" \
  "$R/build/default/.gate.lock" "$R/build/default/python" \
  /usr/bin/taskset -c 0 /usr/bin/perf record -q \
  -e cpu-clock:u -F 199 --call-graph fp -D -1 \
  --control="fifo:$ROWDIR/ctl,$ROWDIR/ack" -o "$ROWDIR/perf.data" -- \
  /usr/bin/env TMPDIR="$ROWDIR/tmp" PYTHONHASHSEED=0 \
  "$R/build/default/python" -S -X "pycache_prefix=$ROWDIR/pycache" \
  "$Q/window_probe.py" "$P/boil/bench/internal/driver.py" \
  "$P/boil/bench/internal/kernels/$ROW.py" "$L" 50 \
  "$ROWDIR/ctl" "$ROWDIR/ack"
/usr/bin/perf report --stdio --no-children -i "$ROWDIR/perf.data"
/usr/bin/perf report --stdio --children -i "$ROWDIR/perf.data"
```

R is the ROOTS worktree above; P is primary. Use the established clean
HOME/LANG/LC_ALL/PATH/TZ environment, with other PYTHON*/LD_* overrides absent.
The explicit outer flock is necessary: gate_run auto-admits regrtest, not
ordinary profiling commands. It and the build lock persist through cleanup.
Do not queue behind another run of this binary. Coordinator reserves CPU0
and sibling8 from competing native work for this short opportunity; record
affinity/load and stop on unavailable admission, timeout or diagnostic error.
Keep raw stdout/stderr and perf data, including failures. No automatic retry.

## What the existing counters can answer

- gc.get_stats()[2] supplies cumulative collections, collected, candidates,
  duration and bytes_freed. Compare phase deltas. live_bytes is a last-mark
  gauge; max_pause is a lifetime maximum, not an additive hot-window maximum.
  The duration covers collection through sweep, including inline collections;
  it does not account for every queued Python finalizer/callback outside it.
- Root totals supply roots_ns and its precise/stack/data/struct timing,
  scanned words, frames_walked, walk-cache hits/misses and fallbacks. Root time
  is part of collection time: do not add them. frames_walked counts C root
  walk work, not Python frames created/resumed. Keep raw values and deltas.
- gc_pauses.py measures only callback-reported pauses and omits inline ones.
  root_stats.py exit journals include setup/finalization. Neither alone gives
  the requested hot window. sys.getallocatedblocks and bytes_freed are not
  cumulative allocated-byte counters; tracemalloc would add a separate tax.
- Software samples expose CPU stacks, not exact call/allocation counts.
  Inspect GC/root/mark/sweep and allocator paths, gen_new_with_qualname,
  gen_send_ex2, _PyEvalFramePushAndInit, value box/array bridges and evaluator
  self time. Report exclusive leaf buckets separately from overlapping
  inclusive paths; frame allocation and allocator samples must not be summed
  twice. Resolve inline evaluator sites with existing symbols/addr2line or
  perf annotate from the same data, reporting lost/truncated/unresolved stacks.

ce has no Py_STATS product. Exact Object allocations/size bins, box kinds/sites,
Frames pushed, frame objects and EVAL_CALL counters can follow on the separately
qualified H3 stats cohort, as mechanism evidence only. Use its corrected reader
and original semantic box sites; old mix.py's dispatch label/cutoff cannot
prove full handler coverage. Existing box_audit.py is a later focused bridge
oracle, not a reason to run every micro case or compare instrumented elapsed
time with release. Native sampling/cumulative GC time need not wait for H3.

## Report and stop

One four-row table is sufficient: original ratio; setup/warm/hot observed
time; hot collection time/count/candidates/freed bytes; last live-byte gauge;
root-time split; leading exclusive native buckets and unresolved fraction.
Retain raw phase data and a few representative callchains. Initial budget:
one host probe plus one target process per row,150s ceiling each,10min total.
Stop once these distinguish the next shared intervention. Extend only one
named ambiguous measurement if its expected information changes that choice
within the remaining budget; no automatic repeat count or new full score.

Repeated full-heap work over a retained live set favors generations/remembered
sets or root/liveness work. Frame creation/resume/copy/clearing dominance favors
the common frame/window protocol and H2; an H1 convention experiment still
needs matched-parent release evidence. Demonstrated box/call bridges favor6e
and shared argument-window/canonical-ingress changes. Evaluator self time
requires source/inlined-site resolution before proposing dispatch work.
Reject complexity without material shared cost and broad expected benefit;
no row-specific opcode, fused-pair quota or speculative speedup promise follows.

Host feasibility: `host.data` has 41 software samples and zero lost samples,
with partially unresolved host symbols. The first FIFO host control rejected
the actual `ack\n\0` response and remains preserved as a failed host-only
receipt. The successor explicitly accepts `ack\n` or `ack\n\0`; both enable
and disable ACKs passed, with zero lost samples. This is feasibility evidence,
not target sampling or complete callchain quality. Helper SHA256 2c9988e9,
exact four-row launch 40289b23. The coordinator reviewed both and granted the
four once-only runs. No source/native optimization changes were made.

## Actual result

All four exact commands returned zero in 37.14 seconds, and listed input
hashes matched before/after. Mode was off/extras0/stress0 throughout. The
build/global-slot locks and CPU reservation were released at terminal;
there was no retry. `result.json`, raw stdout/stderr, `analysis.json`,
perf data, leaf/inclusive reports and stacks remain under the task directory.

| Row | Setup / warm seconds | Hot seconds | GC seconds / fraction | Collections | Root seconds, within GC |
| --- | ---: | ---: | ---: | ---: | ---: |
| coroutines | 0.0005 / 0.284 | 13.296 | 7.180 / 54.0% | 301 | 0.892 |
| comprehensions | 0.0469 / 0.185 | 12.291 | 5.771 / 47.0% | 115 | 0.416 |
| generators | 0.1155 / 0.121 | 5.917 | 1.823 / 30.8% | 59 | 0.603 |
| richards | 0.0024 / 0.059 | 3.036 | 0 / 0% | 0 | 0 |

Hot candidates were 56.43M/42.85M/17.35M/0; reported bytes freed were
10.88GB/2.67GB/2.22GB/0. Last-mark live-byte gauges were 3.05MB/22.62MB/
7.87MB/2.80MB; the Richards gauge was unchanged and does not measure its
post-warmup allocation footprint. All root fallback/unpinned deltas were zero.
Raw root splits and counts remain in the snapshots; root totals above must
not be added to GC duration. Finalizer/callback work outside collection still
limits that duration as explained above.

| Row | Samples / lost | Unresolved leaf | Evaluator exclusive | Other leading exclusive samples |
| --- | ---: | ---: | ---: | --- |
| coroutines | 2337 / 0 | 6.3% | 27.3% | collect 17.5%, live_walk_block 7.2%, gen_traverse 5.6% |
| comprehensions | 2438 / 0 | 3.2% | 21.1% | collect 12.4%, MiMalloc 8.0%, live_walk_block 5.1%, list_traverse 4.3% |
| generators | 1154 / 0 | 3.6% | 51.1% | collect 9.1%, live_visit 8.6%, live_walk_block 3.8% |
| richards | 604 / 0 | 11.4% | 79.5% | FrameClearAndPop 4.1%, Frame_ClearExceptCode 2.2% |

Unresolved leaf addresses are mainly libc. Every recorded chain contains
some unresolved ancestry, so callchains are useful but incomplete; maximum
recorded depths were 43/32/41/27. Zero lost events does not establish complete
unwinding or exact semantic counts. Percentages are exclusive samples;
inclusive reports are retained separately and have not been added to them.

This is sufficient first evidence to prioritize shared GC/churn reduction:
all three slow rows spend a substantial measured fraction collecting, while
the winning object/call control has no hot collections. A generational or
other retained-set intervention deserves its existing design/ownership
review before dispatch-only tuning. These four rows do not prove a particular
generational implementation or a numerical speedup. Coroutines alone reports
48.56M collected objects against 56.43M candidates: per-object death bookkeeping
may remain expensive even if old-live scans disappear. Those totals do not
measure allocation ages. Candidate/survival,
death-pipeline and RSS evidence must separate those costs. Generator evaluator
work remains material after GC; resolve its source sites and use qualified
H3 frame/box counters if that distinction would change the next intervention.
No broad counter instrumentation or additional profile run is required to
act on the present shared-cost finding. Known ce correctness failures,
observer cost and the fork/upstream eval-flag difference remain unchanged.
