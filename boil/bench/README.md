# boil/bench: the internal benchmark set

The set defined in boil/design/10-internal-benchmarks.md: three groups of
pure-Python workloads with no third-party dependency, run the same way at
every step, and one runner. Each step records it; ports are not a gate
until the ports phase.

```
python3 boil/bench/run.py --build build/baseline-default --label baseline \
    --set all --metric instr,rss -o boil/bench/results/baseline.json
python3 boil/bench/run.py --build build/baseline-default --label baseline \
    --set kernels,memory --metric time --max-load 4 --update \
    -o boil/bench/results/baseline.json
python3 boil/bench/compare.py boil/bench/results/{baseline,step1,step5c}.json
python3 -m unittest discover -s boil/bench/tests
```

Any Python 3.10 or later runs the runner; `--build` is the interpreter
under test (or its build directory). `--only NAME,...` selects benchmarks;
`--loops N` overrides the loop counts for smoke runs only.

## The set

**Kernels** (`internal/kernels/`, 29 benchmarks from 22 files). The
pyperformance 1.14.0 kernels of the Phase 1 subset, copied under their MIT
licence (`internal/kernels/LICENSE`) with pyperf removed. A module exposes
`run(loops)`, the work pyperformance times, and optionally `setup(variant)`,
the work it leaves untimed (building the generators tree, writing the
xml_etree file, capturing the regex corpus). deepcopy, pickle, logging and
xml_etree hold several benchmarks each, picked by the variant, so the names
are pyperformance's and the geomean is over the same 29 benchmarks as its
results files. Workload sizes are pyperformance's, with one change: go's
random generator, unseeded upstream, is seeded with 1, so every run plays
the same games. A loop is one pyperformance outer loop (for json_loads,
pickle, logging and unpack_sequence that is 10 to 400 inner operations).

**Object protocol and calls** (`internal/micro/`, 43 files). One statement
in a loop of 1000 iterations; the file's docstring says what an iteration
does. Note 10's list: attribute load and store, monomorphic and polymorphic
over two shapes; method call; instance creation; `__slots__` load; property;
class attribute; global and builtin load; `len`, `abs` and `math.sin`;
Python calls with one and three positional arguments and with keywords;
closure call; generator resume; list, tuple and dict subscript; `str` by
join and by `+=`; `try` with and without a raise; `isinstance`; int and
float chains; compare and branch. It also holds the 16 loops of the former
`boil/tools/micro_loops.py` with their bodies unchanged (the rows of notes
02 and 04). The empty range loop and the empty list loop are the skeletons:
every other row subtracts one, as note 02 does; the polymorphic rows loop
over a list of instances and subtract the list skeleton.

The former `micro_loops.py` rows, as notes 02, 04 and 05 name them, are:
empty loop skeleton `empty_loop`; empty loop, i >= 1025 `empty_loop_large_i`;
int add, result not cached `int_add`; int add, cached small result
`int_add_small`; float add `float_add`; int compare `int_compare`; float
compare `float_compare`; list-of-floats subscript `float_list_subscript`;
two STORE_FAST `store_fast_two`; attribute load `attr_load`; attribute
store, existing `attr_store`; attribute load, `__slots__` `slots_load`;
attribute load and store after `__dict__` `attr_load_after_dict` and
`attr_store_after_dict`; new instance, four attributes `new_instance`; new
instance, empty `__init__` `new_instance_empty`. On baseline they reproduce
note 02's values (skeleton 80.7 against 80, int add 164.0 against 164,
float add 130.1 against 130).

**Memory and the collector** (`internal/memory/`). Fixed workloads, one
loop each: `tuples_10m` (10M `(i, i)` alive), `instances_1m` (1M instances
with `a = i, b = 1.5, c = None, d = "x"`), `dicts_200k` (200k `{'a': i,
'b': str(i)}`), `startup` and `imports` (peak RSS only), and
`collector_live{16,128,512}`: keep L MB of `(k, [k])` units live and
allocate 1 GB of short-lived `[(j, [j]) for j in range(1000)]` batches per
loop, so the time is seconds per allocated GB. MB and GB are fixed object
counts, 2^20 and 2^30 bytes as the baseline build allocates them (168 bytes
per live unit, 136,202 per batch, measured with tracemalloc), so every
build runs the same objects; `memory/collector.py` documents the choice.

## Metrics

| Metric | How | Groups |
|---|---|---|
| `instr` | `perf stat -e instructions:u` at `loops` and `3 * loops`, differenced, median of 4 pairs after a warm-up; per iteration less the skeleton for micro | all |
| `time` | one process: a warm-up call, then the median of 5 `perf_counter_ns` timings of `run(loops)`; ns per loop, seconds per GB for the collector | kernels, memory |
| `rss` | peak RSS in KiB from GNU `/usr/bin/time -f %M`, median of 3 processes | kernels, memory |
| `boxes` | on an `--enable-pystats` build: inline int, float and array boxes per loop differenced at `loops` and `3 * loops`, with boxes by class and by instruction and the dispatches | kernels, micro |

Every measurement runs in a fresh `python -S -X pycache_prefix=DIR`
process, one fresh prefix per invocation. The child's environment drops
every `PYTHON*` variable, as `-E` would, and sets `PYTHONHASHSEED=0` (which
`-E` would ignore), so str hashing and dict probing are the same in every
run and the counts measure the build, not the seed; the runner refuses a
child with hash randomisation on. Loop counts are fixed per benchmark in
`run.py` and recorded with every value; nothing is calibrated. If `perf`
cannot count, GNU time is missing, or a boxes run finds no box counters,
the runner exits with an error and writes nothing.

`instr` is robust under load and is the per-step record. `time` is taken
only on an idle machine: `--max-load 4` samples the 1-minute load average
before every measurement and at the end, and fails the run, writing
nothing, if a sample reaches 4. `--update` merges a run's metrics into an
existing file for the same build. `boxes` reads the stats file Py_STATS
writes to the shared `/tmp/py_stats`: the runner takes the one file that
appears during its child's run and fails if another process wrote one at
the same time; it never clears the directory. For boxing call sites use
`boil/tools/measure/box_audit.py -v`.

The JSON (`schema: boil-bench/1`) holds `build` (label, path, `sys.version`,
`sys._git`, hash seed), `machine`, `results` (metric, benchmark, then the
value, unit, per-run values, loop and run counts, and for micro `raw` and
`skeleton`) and `runs` (per metric: timestamp, bench commit and whether
boil/bench was dirty, and the sampled 1-minute loads). `compare.py REF.json
OTHER.json...` prints one markdown table per metric and group with each
other build's ratio to the reference and the geomean; below 1 is fewer
instructions, less time or less memory.

## Results of record

Labels of record: `baseline` (`build/baseline-default`, upstream CPython
at 321925c92c9), `step1` (`build/step1-snapshot`) and `step5c`
(`build/step5c-snapshot`), one file each in `results/`, all recorded on
2026-10-27 from 41157098106 (bench code as committed). instr and rss ran
under a 1-minute load of 12 to 42; both are load-insensitive. time ran in
quiet windows with `--max-load 4` (baseline 14:07, step1 14:20, step5c
14:41; the highest load sampled 2.08, 2.15 and 2.4 on 16 cores); one
step5c attempt that met a load of 5.52 was refused and repeated.
The full per-benchmark tables are in `results/compare.md`, regenerated by
`python3 boil/bench/compare.py boil/bench/results/{baseline,step1,step5c}.json`.

Geomean ratio to `baseline` (below 1 is fewer instructions, less time or
less memory):

| metric, group | rows | step1 | step5c |
|---|---:|---:|---:|
| instr, kernels | 29 | 1.012 | 1.151 |
| instr, micro | 43 | 0.894 | 0.781 |
| instr, memory | 6 | 1.159 | 1.998 |
| time, kernels | 29 | 1.039 | 1.309 |
| time, memory | 6 | 1.181 | 2.113 |
| rss, kernels | 29 | 1.080 | 4.123 |
| rss, memory | 8 | 1.143 | 1.774 |

step5c against baseline, the rows that carry these numbers:

- Kernels, instr: comprehensions 1.361, coroutines 1.354, float 1.325,
  deepcopy 1.305 and nbody 1.284; spectral_norm 0.893 and richards 0.964.
- Micro, instr: `math.sin` call 2.724, generator resume 2.349 and `str +=`
  2.192 per iteration, and attribute store 1.352. The boxes metric on
  `build/step5c-pystats-snapshot` accounts for three of them: per iteration,
  the `math.sin` call boxes a float and an argument array, the generator
  boxes the int it yields, and the store boxes the int it stores in the
  instance; `str +=` boxes nothing. Reading `__dict__` no longer slows
  attribute access (0.128 and 0.287), and int add, `try` without a raise
  and local stores take less than half.
- Kernels, time: coroutines 2.458, comprehensions 2.135, generators 1.749
  and nbody 1.616 carry the geomean; richards 0.884 and spectral_norm 0.904.
  Time exceeds instr most where allocation is heaviest (coroutines 2.458
  against 1.354), consistent with the collector's share (note 08 section
  7.1).
- Memory: the collector costs 0.533, 0.580 and 0.547 s per allocated GB on
  baseline at 16, 128 and 512 MB live, and 2.859, 3.237 and 4.248 times
  that on step5c: the cost grows with the live heap, the number step 7 must
  move. 10M two-tuples take 2.505 the time, 3.368 the instructions and
  1.854 the peak RSS; the collector rows 2.16 to 2.55 the instructions and
  about 2.1 to 2.5 the peak RSS.
- Kernel peak RSS grows most on the allocation-heavy kernels: nbody 8.19
  (9,528 KiB to about 78,000 KiB), fannkuch 7.63, coroutines 7.52 and
  comprehensions 6.99, consistent with garbage accumulating between the
  collector's runs (note 08 section 7.1).

## Earlier records in this directory

Recorded with pyperformance and the former `boil/tools` scripts before this
set existed; kept as history.

- Phase 1 baselines, 2026-10-23 at 321925c92c9 (16 cores, 62 GiB, gcc
  15.2.0, no PGO or LTO): `default.json`, `jit.json` and `ft.json` from
  `pyperformance run --rigorous=no` on the 22-kernel subset; geomean
  against default: jit 0.834, ft 1.144. `compare-default-vs-*.txt` are
  their tables; `rss-*.json` the peak RSS of the former `rss.py`;
  `sanity-test.txt` the sanity tests. The existing JIT's 16.6 percent is
  the bar the fork's own tier 0 must clear.
- Step records: `step1-delete-tier2*.json`, `tagged-*.json`, `ir-only.json`,
  `final.json` and their `compare-*.txt`.
- Step 6a, 2026-10-27: `step6a.json` and `compare-default-vs-step6a.txt`,
  the subset on `build/dev-default` at a7ff5d45e28: geomean 1.46x slower,
  nbody 1.47x faster, coroutines 2.75x and generators 2.55x slower
  (note 08 section 7.1). pyperf's psutil was built from source; the
  prebuilt abi3 wheel segfaulted at import.

The former tools are folded into `run.py`: `micro_loops.py` is the micro
set (same loop bodies, same 200 and 600 repeat counts, same skeleton
subtraction); `rss.py` is the rss metric and the `startup`, `imports` and
`dicts_200k` rows (its `loops` row, every micro loop once, is not kept);
`measure/perloop.py` is the instr metric on the kernels, now without pyperf
or a venv; `measure/boxes.py` is the boxes metric.
