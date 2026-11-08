# Final measurement preparation

2026-11-08. Source and retained-artifact audit only; no measurements, builds,
or native workloads were run. The coordinator serializes execution after the
combined change is committed and qualified, and after all disposable stats
patches are retired and the release build is restored. The commands below use
new output names: never overwrite earlier evidence.

The target remains the unchanged `logging_silent` variant of
`pyjit/bench/internal/kernels/logging.py`, at its registry count of 60,000
loops. Upstream and candidate are the separate GCC release builds; CPU 15,
hash seed zero and both JITs off are required. Two six-round elapsed campaigns
are necessary for the plan's parity decision. The first below also measures
two controls and supplies fresh calibration for the hardware-counter collector.
Use the same logging loop count in both campaigns.

## Setup and target elapsed measurements

From the canonical checkout, choose a **fresh**, fully validated candidate
build log containing its actual module-check summary. The path shown is the
P3 build log; replace it with the final landed release log if that is what
actually identifies the qualified build. The upstream and interim paths are
the existing controls, not copies in another checkout.

```sh
cd /home/power/code/cpython/.worktrees/python-jit
J=$PWD
B=/home/power/code/cpython/.worktrees/python-boil-the-ocean/build
E8=$B/pyjit-link-wave1/interpreter-parity-20261108
U=$B/upstream-release-devlibs-3a440b6
I=$B/interim-release-devlibs-eba351
C=$B/pyvalue-rc-repr-release
ULOG=$B/pyjit-link-wave1/checkpoint-closure/checkpoint-6a0e4c12c17-final/upstream-build.log
ILOG=$B/pyjit-link-wave1/checkpoint-closure/checkpoint-6a0e4c12c17-final/interim-build.log
CLOG=$E8/p3-release-build.log
export UV_CACHE_DIR=/home/power/.cache/uv

uv run --no-project --python /usr/bin/python3 -- python -B \
  "$J/pyjit/issues/interpreter-parity-20261108/measure-target.py" \
  --upstream "$U" --candidate "$C" --module-log "upstream=$ULOG" \
  --module-log "candidate=$CLOG" --cpu 15 --rounds 6 \
  --cases logging_silent,deltablue,comprehensions \
  --output "$E8/final-target-1"
uv run --no-project --python /usr/bin/python3 -- python -B \
  "$J/pyjit/issues/interpreter-parity-20261108/measure-target.py" \
  --upstream "$U" --candidate "$C" --module-log "upstream=$ULOG" \
  --module-log "candidate=$CLOG" --cpu 15 --rounds 6 \
  --cases logging_silent --output "$E8/final-target-2"
```

These outputs must be new directories. The target collector records source,
module and ELF identity snapshots before/after; it rejects invalid preflight,
changed identity, child or load failures. Its generated native driver checks
CPU 15, upstream JIT disabled and natural-method admission API absent before
each timed call; it checks hash seed and affinity in returned metadata. Each
process has a warmup and seven timed values; six rounds alternate role order.
Read `status.json`, `commands.json`, raw stdout/stderr and `summary.json`.
For each campaign, use the geometric mean of the six candidate/upstream paired
ratios and require GM <= 1.00 and every ratio <= 1.02 for the plan's parity
criterion. A successful collector exit alone is not the parity decision.

## Hardware counters and sampled stacks

The reusable E7 collector is
`pyjit/issues/checkpoint-closure/diagnostic-perf.py`; the reviewed disposable
copy is `E8/final-diagnostic-perf.py`. Its complete exact change from that
historical collector is saved as `E8/final-diagnostic-perf.diff`: absolute
canonical source root, plus child CPU/JIT/hash conditions immediately before
the perf enable and after the perf disable. The before/after observations
include `sys._git`; a failed or changed condition fails the child. Perf FIFO
acknowledgements, event groups, warmup, duration, validation, scheduling and
measurement boundaries are otherwise unchanged. Review this diff before use.
`taskset` below pins the collector and its descendants; the child verifies the
actual inherited affinity, not just the launch command.

The collector's `--case` still validates calibration for *all three* reviewed
cases and both roles. Convert the first target campaign's per-process mean
nanoseconds and recorded fixed loop counts to the required `round-medians`
shape; six fresh values per role/case are retained (the collector takes their
median). These data calibrate a roughly two-second stat interval and do not
substitute for the counter results themselves.

```sh
uv run --no-project --python /usr/bin/python3 -- python -B - "$E8" <<'PY'
import json, sys
from pathlib import Path
base = Path(sys.argv[1])
rows = json.loads((base / 'final-target-1/commands.json').read_text())
cases = ('deltablue', 'logging_silent', 'comprehensions')
roles = ('upstream', 'candidate')
data = {role: {case: [] for case in cases} for role in roles}
for row in rows:
    if row.get('role') in roles and row.get('case') in cases and 'mean_ns' in row:
        data[row['role']][row['case']].append(row['mean_ns'] / row['loops'])
assert all(len(data[role][case]) == 6 for role in roles for case in cases)
path = base / 'final-perf-calibration.json'
with path.open('x') as out:
    json.dump(data, out, indent=2)
    out.write('\n')
PY

mkdir "$E8/final-perf-identities"
cp "$U/python" "$E8/final-perf-identities/upstream-python"
cp "$C/python" "$E8/final-perf-identities/candidate-python"
readelf -n "$U/python" > "$E8/final-perf-identities/upstream-elf-notes.txt"
readelf -n "$C/python" > "$E8/final-perf-identities/candidate-elf-notes.txt"

taskset -c 15 env LD_BIND_NOW=1 UV_CACHE_DIR="$UV_CACHE_DIR" \
  uv run --no-project --python /usr/bin/python3 -- python -B \
  "$E8/final-diagnostic-perf.py" --mode stat --case logging_silent \
  --medians "$E8/final-perf-calibration.json" --upstream "$U" --candidate "$C" \
  --identity-profiles "$E8/final-perf-identities" --output "$E8/final-target-perf-stat"

taskset -c 15 env LD_BIND_NOW=1 UV_CACHE_DIR="$UV_CACHE_DIR" \
  uv run --no-project --python /usr/bin/python3 -- python -B \
  "$E8/final-diagnostic-perf.py" --mode record --case logging_silent \
  --medians "$E8/final-perf-calibration.json" --upstream "$U" --candidate "$C" \
  --identity-profiles "$E8/final-perf-identities" --output "$E8/final-target-perf-record"
```

Stat mode schedules 18 invocations: three rounds, two roles and three event
groups. The `{cycles:u,instructions:u}` group is the desired comparable pair;
branches and cache are also collected by this existing collector. Each child
warms with 60,000 loops; FIFO control gates the actual hot call. The collector
uses at least 99% event running, rejects load >= 4, records all load tuples,
acks, raw perf JSON, child conditions, command and identity information, and
requires matching live/retained ELF build IDs before and after. Compare the
median per-workload-loop instructions and cycles by role; use separate
numerators and denominators, then report candidate/upstream ratios. Counts
include the small hot-region control/timer overhead. `--empty` with `--mode
stat` and a separate new output directory provides the existing empty-region
calibration if needed. Record mode captures one `cycles:u` 499 Hz DWARF stack
profile per role with a roughly five-second hot region and retained build IDs;
inspect lost samples and symbolization before interpreting samples. It is not
a second elapsed parity campaign. If perf permissions or hardware events fail,
retain the failure artifacts and report the missing evidence; do not substitute
whole-process `perf stat` counts as if they represented the gated interval.

## Broad 29-kernel control

`pyjit/issues/checkpoint-closure/measure.py` is the reusable E7 three-round,
three-role collector. It checks the exact 29-item registry, rotates upstream,
interim and candidate order, journals per-child results and load, snapshots
source/build/module identities before and after, and writes
`round-medians.json`, `qualification-status.json` and a table. Run it only
in a separate quiet window; `taskset` pins the whole process tree to CPU 15.
Its native timing driver does not itself assert CPU affinity at each timed
call, so CPU pinning here is inherited from `taskset` and has weaker recorded
proof than the reviewed target driver. Its build probes record JIT state.

```sh
taskset -c 15 env LD_BIND_NOW=1 UV_CACHE_DIR="$UV_CACHE_DIR" \
  uv run --no-project --python /usr/bin/python3 -- python -B \
  "$J/pyjit/issues/checkpoint-closure/measure.py" --candidate "$C" \
  --output "$E8/final-kernels-29" --qualification diagnostic \
  --module-log "upstream=$ULOG" --module-log "interim=$ILOG" \
  --module-log "candidate=$CLOG"
```

`diagnostic` is the collector's label, not a waiver of source, identity or
correctness checks: inspect its preflight/postflight errors and require stable
identities before using the results. The collector's printed ratios use
baseline/candidate, whereas the parity target above uses candidate/upstream;
state the direction explicitly. The broad run is a control and does not replace
the target's two paired six-round campaigns. Prior E7 counter ratios and the
E8 baseline describe different captures, not an effect-size comparison with
this new candidate. CPU pinning and load checks do not eliminate host noise.
