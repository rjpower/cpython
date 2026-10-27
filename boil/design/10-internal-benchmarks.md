# 10: The internal benchmark set, and the ports tree deferred

Status: decided 2026-10-27. The user's ruling at 6b rung 4: do not worry
about the ports tree until the API is settled; the CPython suite keeps us
honest; define and use our own benchmarks; ports come after we have an
API we are happy with and that works well. This note defines the set,
the metrics and the runner, and records the plan change.

## 1. Why

Two days of steps 5 and 6 showed what each measurement is for. The
CPython suite on both builds catches correctness. The per-loop
instruction counts on four benchmarks catch the interpreter's own cost
but missed the collector's (note 08 section 7.1): allocation-heavy
workloads regressed 2x and more while the four stayed flat. pyperformance
needs a venv with compiled third-party packages, and the first run died
on one. The ports tree measures the C API's surface, which is still
moving (6b changed every calling convention; 6c changes the header; 10b
adds handle discipline), so each port patch is rewritten at each step.

So: one in-tree benchmark set with no third-party dependency, run the
same way at every step, with the collector and the object protocol in it;
and the ports revived once against the settled API.

## 2. The set

Three groups, all pure Python and stdlib, under `boil/bench/internal/`.

**Kernels.** The pyperformance kernels we already track, copied in-tree
under their MIT licence with `pyperf` removed: each module exposes
`run(loops)` and returns nothing. richards, go, nbody, float,
spectral_norm, raytrace, deltablue, chaos, nqueens, fannkuch,
generators, comprehensions, coroutines, unpack_sequence, deepcopy,
json_dumps, json_loads, pickle, unpickle, regex_compile, logging,
xml_etree. These are the Phase 1 subset less nothing; the geomean on
them is the number the phase targets (note 06 section 6, steps 8 and 9)
are stated against.

**Object protocol and calls** (`micro/`). Per-iteration kernels in the
manner of `boil/tools/micro_loops.py` (differenced over two repeat
counts so start-up cancels), one file each: instance attribute load and
store, monomorphic and polymorphic over two shapes; method call; instance
creation with four attributes; `__slots__` load; property; class
attribute; global and builtin load; builtin call with inline arguments
(`len`, `abs`, `math.sin`); Python call with one and three positional
arguments, and with keywords; closure call; generator resume; list, tuple
and dict subscript; `str` building by join and by `+=`; `try`/`except`
with and without a raise; `isinstance`; int and float arithmetic chains;
comparison and branch. Each has an upstream-identical source so the
baseline build runs it unchanged.

**Memory and the collector** (`memory/`). Fixed workloads with peak RSS
and time: 10M two-tuples kept alive; 1M four-attribute instances kept
alive; 200k small dicts; and a collector benchmark parameterised by live
heap size and garbage rate (keep L MB live, allocate G MB of short-lived
objects, report seconds per allocated GB at L in {16, 128, 512} MB),
which is the number step 7 must move.

## 3. Metrics

| Metric | How | Use |
|---|---|---|
| instructions per iteration or per loop | `perf stat -e instructions:u`, differenced over two repeat counts, median of 4 | the per-step record: robust under build load, so every step records it |
| wall time | median of N runs on an idle machine, `time.perf_counter_ns` inside the process | waypoints and phase targets only |
| peak RSS | `/usr/bin/time -f %M` | memory group, and any step touching layout |
| boxes and dispatches per loop | the `Py_STATS` build, as `boil/tools/measure/boxes.py` does | steps that change the value model or the IR |

## 4. The runner

`boil/bench/run.py --build BUILD --set {kernels,micro,memory,all}
--metric {instr,time,rss,boxes} -o FILE.json` runs the set in fresh
`python -S -E` processes with a fresh `PYTHONPYCACHEPREFIX`, writes one
JSON with the machine, commit, build and per-benchmark values, and
refuses to guess if `perf` cannot count. `boil/bench/compare.py A.json
B.json` prints a table with ratios and the geomean per group. Results
live in `boil/bench/results/<label>.json`; the labels of record are
`baseline` (`build/baseline-default`, upstream at 321925c92c9),
`step1`, `step5c`, and one per step from 6b on. `boil/bench/README.md`
documents the set and lists the results of record. The existing
`perloop.py`, `boxes.py`, `micro_loops.py` and `rss.py` are folded in or
deleted so there is one way to measure.

## 5. The plan change

- Every step's gate is: the full suite on both builds, `-u cpu
  test_cext`, regen clean, and the internal set recorded with
  attribution. Ports are not a gate for any step until the ports phase.
- The ports tree is frozen at `cpython-3.16-value-v1` with the port
  states recorded in note 06 section 19. six, attrs, pycparser and
  markupsafe pass there; cffi, pyyaml and numpy fail at the API surface.
- The ports phase follows 10b, when the C API has its final shape
  (values, shapes in the header, handle discipline for references held
  across allocations). Cython is ported first, as the generator for two
  of the others; then cffi, pyyaml, numpy, and new ports from there.
  Patch lines per port stay the compatibility metric.
- A benchmark step (B) builds this set now, in parallel with 6b's close
  and 6c, in a separate worktree, and records the labels of record.

## 6. What the set does not do

It does not replace pyperformance as the external comparison: a
pyperformance run against the Phase 1 baseline is still made at each
waypoint (one per phase), on an idle machine, with psutil built from
source. It does not include C-extension workloads; those return with
the ports phase.
