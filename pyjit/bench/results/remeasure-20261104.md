# Re-measure after representation step 6c, 2026-11-04

This report measures the PyValue runtime with the register-IR interpreter after
representation steps 5 to 6c and value API V1 (C, `c8a0b4dc54c`). It compares
C with the checkpoint-triage fix, which turned natural method-JIT admission off
and comes just before step 5 (B, `1f19952778d`), and with upstream CPython (U,
`3a440b6`). It repeats the tables and method of the
[checkpoint report](checkpoint-20261103.md) and the attribution of its
[triage](checkpoint-20261103-triage.md). All three builds are release builds,
and no JIT runs in any of them (see Builds).

Each ratio in the tables follows [measurement.md](../../design/measurement.md):
the baseline's elapsed time (or RSS) divided by C's. A ratio above 1 favours C.
U/C 0.440 means that C takes 2.27 times as long as U, and B/C 1.160 means that
C takes 0.862 times as long as B.

**Summary.**

- **Kernels.** Across the 29 kernels, C takes 2.27 times as long as upstream
  (geometric mean U/C 0.440) and 0.862 times as long as B (B/C 1.160). B takes
  2.64 times as long as U. That is within 1.5% of the triage's 2.60 for the
  same code, so B reproduces the triage's "before".
- **pyperformance.** The 12 results of the eight benchmarks give U/C 0.395
  (C takes 2.53 times as long as U) and B/C 1.134 (C/B 0.882).
- **Dispatch and boxes.** Neither changed. C executes exactly the same words as
  the checkpoint, 0.805 words per upstream instruction (geometric mean), and
  it makes exactly the same boxes: every kernel's box total equals the
  checkpoint's.
- **Where the saving comes from.** On the triage's five worst kernels, the
  time spent in exception save/restore fell by 63%, from 22.3% of B's time to
  8.2%. That accounts for 83% of the time C saves against B. Its share of the
  gap to upstream fell from 30% (B) to 14% (C). The interpreter loop's own
  time did not change (31.8% of B's time under B, 31.3% under C). It is now
  the largest bucket, at 32% of the C−U gap, followed by value bridging at 28%.
- **RSS.** Peak RSS is unchanged against B (B/C 0.996) and stays 28% above
  upstream's (U/C 0.782).

`perf` is unavailable (`kernel.perf_event_paranoid=4`), so this report has no
instruction or cycle counts. The attribution uses the triage's LD_PRELOAD
SIGPROF sampler and its symbol classifier.

## Builds

| Label | Commit | Build directory (under `build/`) | Configure | `sys.version` | Method JIT |
|---|---|---|---|---|---|
| C | `c8a0b4dc54c` (HEAD of `boil/pyvalue-rc-universal`) | `remeasure-release-c8a0b4dc54c` | `CC=gcc --with-mimalloc --with-computed-gotos --with-shapes` | `3.16.0a1+dev (heads/boil/pyvalue-rc-universal:c8a0b4dc54c, Nov  4 2026, 01:03:50) [GCC 15.2.0]` | natural admission off; 0 units |
| C stats | `c8a0b4dc54c` | `remeasure-stats-c8a0b4dc54c` | the same, plus `--enable-pystats` | `… Nov  4 2026, 01:04:56 …` | compiled out (`_PyJIT_BASELINE_SUPPORTED` is 0 under `Py_STATS`) |
| B | `1f19952778d` | `remeasure-release-1f19952778d` (removed after the run) | `CC=gcc --with-mimalloc --with-computed-gotos --with-shapes` | `3.16.0a1+dev (heads/boil/jit-terminal-settlement-91-g1f19952778d:1f19952778d, Nov  4 2026, 00:) [GCC 15.2.0]` | natural admission off; 0 units |
| U | `3a440b65695` | `upstream-release-3a440b6` | `CC=gcc --with-mimalloc --with-computed-gotos` | `3.16.0a1+dev (heads/main:3a440b65695, Nov  3 2026, 15:54:35) [GCC 15.2.0]` | not present |

All four builds use gcc 15.2.0 (Ubuntu 15.2.0-16ubuntu1) with `-O3 -g
-fno-omit-frame-pointer -mno-omit-leaf-frame-pointer`, and none has PGO, LTO or
the tail-calling interpreter. Every build's `pybuilddir.txt` reads
`build/lib.linux-x86_64-3.16`. All lack the same ten optional modules (the
ssl, sqlite and similar headers are not installed), and no benchmark here uses
them. B's `sys.version` is cut short in the build time (`00:)`).
*Hypothesis:* the long `git describe` name fills the version buffer. C and B
were built from detached worktrees,
`build/worktrees/remeasure-{c8a0b4dc54c,1f19952778d}`, which were removed
after the run. The machine is an AMD Ryzen 7 3800X (8 cores, 16
threads, 64 GB) running Linux 7.0.0-43, with the amd-pstate-epp driver and
the powersave governor.

**Method-JIT state.** `sys._jit.is_available()` is False in every build, but
it covers only upstream's copy-and-patch JIT. The fork's method JIT is read
through `_testinternalcapi`:

- `jit_method_natural(flag)` sets natural admission and returns the previous
  setting. Calling it with False and then restoring the returned value reads
  the state. The default is set by `static int natural_admission;`
  (`Python/jit_runtime.c:116` in B), which is zero.
- `jit_method_info(function)` reports a function's `active`,
  `compile_attempts` and the process-wide `process_units` and
  `process_bytes`.

Script: `build/pyjit-link-wave1/remeasure-jitstate.py`. Under C and B, natural
admission reads False. After a 100,000-iteration `for` loop, a
100,000-iteration scalar `while` loop (the shape the observed-loop optimiser
lowers), 10,000 calls of a small function and one `run(1)` of richards, none
of the 44 functions checked (the 4 probe functions and richards' 40) was
admitted. Each has `active` False and `compile_attempts` 0, and
`process_units` and `process_bytes` are 0. The
process therefore holds no method-JIT or trace code, and there was no
trace-JIT activity. The stats build also reads False, and it compiles the
method JIT out.

The correctness smoke ran on B and on C before any measurement: `-m test
test_grammar test_int test_float test_list test_dict test_generators
test_json test_pickle -j8` passed all 8 files on each (1,814 tests run, 65
skipped).

## Conditions

All timed runs (kernels and pyperformance, for C, B and U) ran in one quiet
window, from 01:10 to 01:24. It opened when the 1-minute load average fell to
1.23. The two other agents had stopped building and testing, and this task ran
nothing else in the window apart from sub-second scripts that read result
files. Order:

- **Kernels.** For each kernel in registry order: U, then B, then C, each a
  separate `run.py --only KERNEL` invocation with `--max-load 4`. This is the
  triage's interleaving. The highest load that `run.py` sampled was 1.17.
- **pyperformance.** For each benchmark in turn: C, then B, then U.

From about 01:17, from U's regex_v8 through the last four pyperformance
benchmarks (mdp, async_tree, unpickle_pure_python and
typing_runtime_protocols), the load that pyperf recorded rose to 2.0–3.1.
The highest values are U's mdp (3.09) and C's async_tree (2.64). No other heavy process was visible when this was
checked at 01:23. These results are less certain than the rest, though the
interleaving spread the load over all three interpreters alike.

Builds, the smoke tests, RSS, pystats, dispatch counts and the sampler ran
outside the window, at load 0.2 to 5. B's and U's RSS and sampler runs were
taken before C existed (00:12–00:14). C's ran just before the window
(01:05–01:10). Before the window, each pyperformance benchmark was started
once under each interpreter with `-p 1 -n 1 -w 0 -l 1`, to check that it runs
in its venv. Those outputs were discarded and are not measurements.

Each kernel time vector holds five serial calls in one process, after one
warm-up call. These samples are correlated; they are not independent process
pairs.

## Kernels: time

Command, per kernel and interpreter: `run.py --set kernels --only KERNEL
--metric time --max-load 4`, using the registry's fixed loop counts, with no
retries or calibration. Driver: `build/pyjit-link-wave1/remeasure-time-kernels.py`,
which also merges the per-kernel outputs into one file per interpreter and keeps
each invocation's run record. Raw data: `remeasure-20261104-{C,B,U}-kernels.json`.
Each value is the median of five timed calls of `run(loops)`, in milliseconds
per loop. Spread is `100 * (max / min - 1)` over the five calls. The JSON files
keep the raw vectors.

| Kernel | C ms | C spread % | B ms | B spread % | U ms | U spread % | U/C | B/C |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| richards | 160.3 | 1.3 | 183.8 | 7.5 | 60.36 | 1.6 | 0.376 | 1.146 |
| go | 318.7 | 8.2 | 358.8 | 4.0 | 120.3 | 9.2 | 0.378 | 1.126 |
| nbody | 185.9 | 0.6 | 186.8 | 1.8 | 95.39 | 7.1 | 0.513 | 1.005 |
| float | 169.9 | 4.0 | 182.4 | 2.3 | 82.13 | 4.0 | 0.483 | 1.073 |
| spectral_norm | 181.3 | 1.6 | 214.9 | 1.7 | 114.4 | 2.1 | 0.631 | 1.186 |
| raytrace | 716.4 | 8.4 | 794.0 | 0.9 | 330.8 | 8.3 | 0.462 | 1.108 |
| deltablue | 12.97 | 3.6 | 16.50 | 7.6 | 3.736 | 3.0 | 0.288 | 1.273 |
| chaos | 151.2 | 0.3 | 180.1 | 7.2 | 72.18 | 5.5 | 0.477 | 1.191 |
| nqueens | 186.7 | 0.5 | 205.6 | 4.9 | 96.52 | 2.9 | 0.517 | 1.101 |
| fannkuch | 978.4 | 5.8 | 995.7 | 9.3 | 384.4 | 5.7 | 0.393 | 1.018 |
| generators | 73.19 | 4.9 | 91.01 | 1.4 | 32.61 | 5.0 | 0.446 | 1.244 |
| comprehensions | 0.04638 | 3.9 | 0.05552 | 0.6 | 0.01697 | 1.1 | 0.366 | 1.197 |
| coroutines | 53.51 | 1.1 | 53.16 | 1.1 | 25.04 | 0.6 | 0.468 | 0.994 |
| unpack_sequence | 0.03438 | 0.7 | 0.03467 | 0.9 | 0.01377 | 3.2 | 0.401 | 1.008 |
| deepcopy | 0.9771 | 1.7 | 1.181 | 1.4 | 0.3174 | 2.4 | 0.325 | 1.209 |
| deepcopy_reduce | 0.01109 | 1.9 | 0.01341 | 1.6 | 0.003797 | 1.0 | 0.342 | 1.208 |
| deepcopy_memo | 0.1423 | 2.2 | 0.1698 | 2.1 | 0.03059 | 7.5 | 0.215 | 1.193 |
| json_dumps | 24.89 | 1.9 | 31.26 | 7.0 | 11.83 | 1.7 | 0.475 | 1.256 |
| json_loads | 0.9839 | 6.0 | 1.320 | 6.4 | 0.5498 | 4.3 | 0.559 | 1.342 |
| pickle | 0.3912 | 8.4 | 0.4926 | 1.5 | 0.2778 | 5.0 | 0.710 | 1.259 |
| unpickle | 0.4631 | 12.3 | 0.6036 | 1.1 | 0.3523 | 1.7 | 0.761 | 1.303 |
| regex_compile | 424.6 | 8.0 | 472.2 | 8.0 | 167.7 | 0.8 | 0.395 | 1.112 |
| logging_format | 0.1927 | 5.6 | 0.2253 | 8.5 | 0.08129 | 1.4 | 0.422 | 1.169 |
| logging_silent | 0.003388 | 0.6 | 0.004183 | 1.5 | 0.001118 | 1.3 | 0.330 | 1.235 |
| logging_simple | 0.1852 | 1.4 | 0.1916 | 3.9 | 0.06729 | 2.5 | 0.363 | 1.034 |
| xml_etree_generate | 297.9 | 11.2 | 364.8 | 1.4 | 134.2 | 8.9 | 0.450 | 1.224 |
| xml_etree_iterparse | 194.2 | 7.2 | 234.9 | 1.7 | 111.7 | 3.2 | 0.575 | 1.209 |
| xml_etree_parse | 211.7 | 1.3 | 248.0 | 5.1 | 165.4 | 2.2 | 0.781 | 1.171 |
| xml_etree_process | 218.1 | 8.6 | 250.6 | 2.7 | 82.10 | 6.7 | 0.376 | 1.149 |
| **geometric mean** | | | | | | | **0.440** | **1.160** |

Five worst kernels against U (U/C): deepcopy_memo 0.215, deltablue 0.288,
deepcopy 0.325, logging_silent 0.330, deepcopy_reduce 0.342. These are the
same kernels that came out worst in the triage, after the fix.
Five best: xml_etree_parse 0.781, unpickle 0.761, pickle 0.710, spectral_norm
0.631, xml_etree_iterparse 0.575.

Against B, C gains most on json_loads (B/C 1.342), unpickle (1.303),
deltablue (1.273), pickle (1.259) and json_dumps (1.256). It gains least on
coroutines (0.994), nbody (1.005), unpack_sequence (1.008), fannkuch (1.018)
and logging_simple (1.034). Coroutines is the only kernel where C is slower,
by 0.6%, which is inside the noise.

C's largest spread is 12.3% (unpickle), B's 9.3% (fannkuch) and U's 9.2% (go).
Seven C values spread 8% or more: go, raytrace, pickle, unpickle,
regex_compile, xml_etree_generate and xml_etree_process. Their B/C ratios are
less certain. No conclusion here rests on a single kernel.

Against the triage's quiet interleaved run (same commits, 2026-11-03): U's
times now are 0.995 of the triage's (geometric mean over the 29 kernels), and
B's are 1.010 of the triage's F. Individual kernels move by up to 11%.

## Kernels: peak RSS

Command: `run.py --set kernels --metric rss`, and `run.py --set memory --only
startup,imports --metric rss`. Each value is the median of three processes,
in MiB of peak RSS. Raw data: `remeasure-20261104-{C,B,U}-kernels-rss.json`
and `remeasure-20261104-{C,B,U}-startup-rss.json`. These runs were outside
the window; RSS does not depend on CPU load.

| Kernel | C MiB | C spread % | B MiB | B spread % | U MiB | U spread % | U/C | B/C |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| richards | 12.72 | 18.7 | 12.59 | 18.9 | 9.680 | 14.1 | 0.761 | 0.990 |
| go | 14.45 | 19.6 | 14.39 | 21.4 | 11.30 | 9.0 | 0.782 | 0.996 |
| nbody | 12.76 | 5.4 | 12.80 | 4.9 | 9.766 | 4.8 | 0.765 | 1.003 |
| float | 20.50 | 3.0 | 20.30 | 3.4 | 26.63 | 1.6 | 1.299 | 0.990 |
| spectral_norm | 12.53 | 3.3 | 12.54 | 1.9 | 9.562 | 4.4 | 0.763 | 1.001 |
| raytrace | 14.22 | 30.6 | 14.29 | 29.8 | 10.86 | 21.0 | 0.764 | 1.005 |
| deltablue | 13.59 | 10.3 | 13.45 | 9.1 | 10.56 | 12.8 | 0.777 | 0.990 |
| chaos | 14.70 | 7.3 | 14.77 | 4.4 | 11.39 | 1.3 | 0.775 | 1.005 |
| nqueens | 12.47 | 4.6 | 12.44 | 8.3 | 9.551 | 5.1 | 0.766 | 0.998 |
| fannkuch | 12.50 | 4.8 | 12.59 | 1.1 | 9.727 | 4.7 | 0.778 | 1.008 |
| generators | 22.54 | 3.1 | 22.66 | 3.0 | 22.63 | 0.8 | 1.004 | 1.006 |
| comprehensions | 15.84 | 44.5 | 15.70 | 46.8 | 11.93 | 40.8 | 0.753 | 0.991 |
| coroutines | 12.63 | 3.0 | 12.53 | 2.7 | 9.551 | 4.9 | 0.756 | 0.992 |
| unpack_sequence | 12.94 | 34.4 | 12.77 | 35.9 | 9.652 | 26.0 | 0.746 | 0.987 |
| deepcopy | 14.61 | 8.9 | 14.59 | 11.7 | 11.09 | 5.7 | 0.760 | 0.999 |
| deepcopy_reduce | 14.50 | 1.1 | 14.56 | 1.7 | 11.08 | 1.6 | 0.764 | 1.004 |
| deepcopy_memo | 14.59 | 0.6 | 14.34 | 3.2 | 10.93 | 1.1 | 0.750 | 0.983 |
| json_dumps | 16.04 | 24.4 | 15.96 | 25.5 | 11.57 | 15.1 | 0.721 | 0.995 |
| json_loads | 16.10 | 3.1 | 16.15 | 0.9 | 12.00 | 3.7 | 0.745 | 1.003 |
| pickle | 15.52 | 28.6 | 15.41 | 29.0 | 11.58 | 28.1 | 0.746 | 0.993 |
| unpickle | 15.52 | 0.7 | 15.46 | 1.9 | 11.88 | 1.6 | 0.766 | 0.996 |
| regex_compile | 18.28 | 47.8 | 18.19 | 43.1 | 13.34 | 42.1 | 0.730 | 0.995 |
| logging_format | 19.48 | 30.9 | 19.36 | 30.9 | 14.73 | 20.1 | 0.756 | 0.994 |
| logging_silent | 17.66 | 1.0 | 17.40 | 0.6 | 13.00 | 1.8 | 0.736 | 0.986 |
| logging_simple | 19.00 | 1.0 | 18.87 | 0.8 | 14.14 | 1.0 | 0.744 | 0.993 |
| xml_etree_generate | 22.93 | 16.2 | 22.59 | 16.6 | 18.14 | 7.6 | 0.791 | 0.986 |
| xml_etree_iterparse | 22.92 | 1.3 | 22.99 | 1.2 | 17.73 | 0.4 | 0.773 | 1.003 |
| xml_etree_parse | 22.21 | 2.1 | 22.27 | 0.7 | 16.89 | 1.1 | 0.761 | 1.003 |
| xml_etree_process | 23.18 | 1.7 | 23.13 | 0.9 | 18.36 | 0.6 | 0.792 | 0.998 |
| **geometric mean** | | | | | | | **0.782** | **0.996** |

| Row | C KiB | B KiB | U KiB | U/C | B/C |
|---|---:|---:|---:|---:|---:|
| startup | 12,896 | 12,576 | 9,664 | 0.749 | 0.975 |
| imports | 15,284 | 15,100 | 11,324 | 0.741 | 0.988 |

C's RSS is that of B to within 1.7% on every kernel. Startup is 320 KiB (2.5%)
higher. The spreads above 20% come, as in the checkpoint, from the first of
the three processes, which compiles the `.pyc` into a fresh cache and peaks
higher; the median discards it. float is the only kernel where C uses clearly
less memory than U (inline coordinates). The checkpoint measured U/C 0.747 on
`dbce7b1260d`, which ran with admission on. *Hypothesis:* its admitted JIT
code accounts for the difference. B/C 0.996 is the like-for-like comparison.

## pyperformance subset

These are the checkpoint's eight benchmarks from pyperformance 1.14.0
(`/home/power/.cache/uv/archive-v0/xitntS2hDvEA0_ce/pyperformance/data-files/benchmarks`),
with the same settings. Each `run_benchmark.py` ran directly under pyperf
2.10.0, with `--processes 6` and all other settings at pyperf's defaults: one
warm-up and three values per process, with loops calibrated by pyperf.
async_tree ran with `none`, and unpickle_pure_python as
`bm_pickle/run_benchmark.py --pure-python unpickle`. Each interpreter had its
own venv under `build/pyperf-venvs/remeasure-{C,B,U}`, holding a copy of the
pure-Python pyperf package. The venvs were deleted afterwards. Values are mean
± standard deviation in milliseconds over the 18 values. Spread is
`100 * (max / min - 1)` over the same values. Raw data and logs:
`remeasure-20261104-pyperf/{C,B,U}-<benchmark>.json`. pyperf flags every
result as possibly unstable, as in the checkpoint: 18 values are too few for
its 1% criterion.

| Benchmark | C ms | B ms | U ms | U/C | B/C | spread % (C, B, U) | loops (C, B, U) |
|---|---:|---:|---:|---:|---:|---|---|
| hexiom | 15.22 ± 0.11 | 15.23 ± 0.53 | 6.615 ± 0.072 | 0.435 | 1.001 | 2.5, 9.7, 3.7 | 8, 8, 16 |
| pyflate | 854.7 ± 30 | 919.6 ± 30 | 418.1 ± 8.9 | 0.489 | 1.076 | 9.8, 8.8, 8.0 | 1, 1, 1 |
| scimark_fft | 946.6 ± 31 | 1,068 ± 27 | 395.5 ± 8.6 | 0.418 | 1.128 | 11.6, 7.6, 7.4 | 1, 1, 1 |
| scimark_lu | 300.8 ± 8.7 | 325.7 ± 12 | 145.0 ± 4.5 | 0.482 | 1.083 | 9.5, 12.5, 10.3 | 1, 1, 1 |
| scimark_monte_carlo | 247.3 ± 8.9 | 295.1 ± 8.3 | 75.58 ± 3.0 | 0.306 | 1.193 | 13.0, 9.4, 21.0 | 1, 1, 2 |
| scimark_sor | 379.1 ± 8.3 | 393.4 ± 14 | 130.7 ± 3.8 | 0.345 | 1.038 | 8.5, 10.6, 9.0 | 1, 1, 1 |
| scimark_sparse_mat_mult | 20.38 ± 0.17 | 24.73 ± 0.29 | 6.104 ± 0.19 | 0.300 | 1.213 | 2.6, 4.7, 11.3 | 8, 8, 32 |
| regex_v8 | 30.30 ± 0.61 | 32.75 ± 1.2 | 21.16 ± 0.91 | 0.698 | 1.081 | 10.6, 9.2, 15.7 | 4, 4, 8 |
| mdp | 3,903 ± 96 | 5,151 ± 79 | 1,353 ± 35 | 0.347 | 1.320 | 7.1, 5.8, 8.6 | 1, 1, 1 |
| async_tree_none | 774.7 ± 29 | 875.1 ± 30 | 354.5 ± 13 | 0.458 | 1.130 | 12.0, 12.5, 16.0 | 1, 1, 1 |
| unpickle_pure_python | 0.8488 ± 0.020 | 0.9352 ± 0.034 | 0.2540 ± 0.011 | 0.299 | 1.102 | 12.1, 11.9, 16.0 | 8, 8, 32 |
| typing_runtime_protocols | 0.5146 ± 0.019 | 0.6644 ± 0.012 | 0.1673 ± 0.0048 | 0.325 | 1.291 | 12.6, 8.0, 9.0 | 256, 256, 1024 |
| **geometric mean** | | | | **0.395** | **1.134** | | |

Every interpreter produced 18 values for every result. B takes 2.87 times as
long as U (U/B 0.348). The checkpoint's U/C of 0.144 was `dbce7b1260d` with
admission on, so it is not comparable. U's means are 1.020 of the
checkpoint's (geometric mean). C gains most on mdp (1.320) and
typing_runtime_protocols (1.291), and nothing on hexiom (1.001). U's regex_v8
and all three interpreters' mdp, async_tree, unpickle_pure_python and
typing_runtime_protocols ran at the higher load described under Conditions.

## Dispatch counts

Command: `pyjit/bench/ir/run.py --ir-python C/python --upstream-python U/python
--commit c8a0b4dc54c7e095c65ca4b9bdd5fc4e0af27c46 --kernels NAME --out …`,
one invocation per single-module kernel, six at a time. As in the
checkpoint, the 12 variants of the four multi-variant modules ran through
shim modules whose `setup()` passes the variant name, with `ir/run.py`'s
worker, arguments and environment. Driver:
`build/pyjit-link-wave1/remeasure-dispatch.py`. Raw data:
`remeasure-20261104-dispatch.json` and `remeasure-20261104-dispatch-variants.json`.
The counts are `sys.monitoring` INSTRUCTION events during one `run(1)` after
a warm-up `run(1)`: words for C, instructions for U, with NOT_TAKEN excluded.
The checkpoint report describes the limits.

The ns columns divide the quiet-window time per loop by the words in one
`run(1)`. This is a proxy: it assumes that each loop does the work of `run(1)`,
and it charges library time to the words. For C-heavy kernels (pickle,
unpickle, json_loads, xml_etree_parse) it is mostly library time.

| Kernel | U instructions | C words | C/U | C ns per word | U ns per instruction | Per-dispatch cost C/U | C boxes per word |
|---|---:|---:|---:|---:|---:|---:|---:|
| richards | 9,784,097 | 5,789,061 | 0.59 | 27.7 | 6.17 | 4.5 | 0.005 |
| go | 28,667,983 | 18,762,468 | 0.65 | 17.0 | 4.20 | 4.0 | 0.012 |
| nbody | 28,581,241 | 19,300,999 | 0.68 | 9.6 | 3.34 | 2.9 | 0.021 |
| float | 12,199,935 | 7,899,942 | 0.65 | 21.5 | 6.73 | 3.2 | 0.038 |
| spectral_norm | 19,045,221 | 17,011,597 | 0.89 | 10.7 | 6.01 | 1.8 | 0.040 |
| raytrace | 41,877,624 | 29,606,956 | 0.71 | 24.2 | 7.90 | 3.1 | 0.045 |
| deltablue | 726,098 | 545,927 | 0.75 | 23.8 | 5.15 | 4.6 | 0.004 |
| chaos | 10,532,435 | 7,321,871 | 0.70 | 20.6 | 6.85 | 3.0 | 0.066 |
| nqueens | 14,592,232 | 11,850,310 | 0.81 | 15.8 | 6.61 | 2.4 | 0.046 |
| fannkuch | 70,923,145 | 53,069,157 | 0.75 | 18.4 | 5.42 | 3.4 | 0.112 |
| generators | 6,806,866 | 6,106,863 | 0.90 | 12.0 | 4.79 | 2.5 | 0.016 |
| comprehensions | 3,223 | 2,417 | 0.75 | 19.2 | 5.27 | 3.6 | 0.074 |
| coroutines | 3,884,596 | 3,277,628 | 0.84 | 16.3 | 6.45 | 2.5 | 0.000 |
| unpack_sequence | 5,679 | 9,667 | 1.70 | 3.6 | 2.43 | 1.5 | 0.000 |
| deepcopy | 33,266 | 29,757 | 0.89 | 32.8 | 9.54 | 3.4 | 0.014 |
| deepcopy_reduce | 401 | 343 | 0.86 | 32.3 | 9.47 | 3.4 | 0.003 |
| deepcopy_memo | 5,672 | 4,920 | 0.87 | 28.9 | 5.39 | 5.4 | 0.020 |
| json_dumps | 460,179 | 316,144 | 0.69 | 78.7 | 25.70 | 3.1 | 0.054 |
| json_loads | 4,788 | 4,175 | 0.87 | 235.7 | 114.83 | 2.1 | 0.057 |
| pickle | 356 | 404 | 1.13 | 968.2 | 780.45 | 1.2 | 2.030 |
| unpickle | 373 | 355 | 0.95 | 1,304.6 | 944.54 | 1.4 | 0.000 |
| regex_compile | 22,225,651 | 18,001,919 | 0.81 | 23.6 | 7.55 | 3.1 | 0.064 |
| logging_format | 7,965 | 5,886 | 0.74 | 32.7 | 10.21 | 3.2 | 0.015 |
| logging_silent | 287 | 210 | 0.73 | 16.1 | 3.90 | 4.1 | 0.000 |
| logging_simple | 7,723 | 5,654 | 0.73 | 32.8 | 8.71 | 3.8 | 0.016 |
| xml_etree_generate | 8,673,036 | 6,773,770 | 0.78 | 44.0 | 15.47 | 2.8 | 0.022 |
| xml_etree_iterparse | 5,729,688 | 4,910,101 | 0.86 | 39.6 | 19.50 | 2.0 | 0.000 |
| xml_etree_parse | 1,543,374 | 1,173,352 | 0.76 | 180.4 | 107.16 | 1.7 | 0.000 |
| xml_etree_process | 7,577,080 | 6,074,727 | 0.80 | 35.9 | 10.83 | 3.3 | 0.000 |
| **geometric mean** | | | **0.805** | | | | |

The 17 single-module kernels give exactly the checkpoint's counts for both
builds. Each variant is 5 lower in both builds, because this shim is 5
instructions shorter than the checkpoint's. The geometric mean is 0.805 at
both commits; the triage quotes it as 0.81. Steps 5 to 6c and V1 did not
change the IR compiler, so the words are the same, and all of C's saving is in
the cost per word. On the checkpoint's eight object-heavy kernels (richards to
chaos in the table) a C word costs 9.6–27.7 ns, against 3.3–7.9 ns for an
upstream instruction: 1.8–4.6 times as much. The
checkpoint, with admission on, measured 75–125 ns.

## Boxes

Command: `run.py --set kernels --metric boxes --pystats-records …` on
`remeasure-stats-c8a0b4dc54c`, through the checkpoint's wrapper
(`build/pyjit-link-wave1/checkpoint-run-boxes.py`; C's dump still lacks the
handler-entry coverage lines that `run.py` requires). Raw data:
`remeasure-20261104-C-kernels-boxes.json` and the original dumps in
`build/pyjit-link-wave1/remeasure-20261104-C-kernels-pystats.jsonl` (not
committed; the boxes JSON records its original path and its hash). The site classes come from V1's
`Boxing by site (CLASS)` counters, differenced between the `loops` and
`3 * loops` processes in the JSON's `raw_counters`. Script:
`build/pyjit-link-wave1/remeasure-boxes.py`.
`Tools/scripts/summarize_stats.py` prints the same table for a single dump,
but it does not difference. The site classes add up to each kernel's int plus
float box total on every kernel. Every "Entry unboxing by site" counter
differences to 0 on every kernel.

Boxes per loop, by site class:

| Kernel | Total | unattributed | call adapter | list | iteration | dict | tuple | attribute | parser |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| richards | 26,229 | 18 | 1 | 0 | 0 | 0 | 0 | 26,210 | 0 |
| go | 229,237 | 119,904 | 87,634 | 21,601 | 0 | 0 | 0 | 98 | 0 |
| nbody | 400,041 | 400,040 | 1 | 0 | 0 | 0 | 0 | 0 | 0 |
| float | 300,002 | 1 | 300,001 | 0 | 0 | 0 | 0 | 0 | 0 |
| spectral_norm | 676,302 | 1 | 41 | 0 | 676,260 | 0 | 0 | 0 | 0 |
| raytrace | 1,336,856 | 946,723 | 350,771 | 30,000 | 0 | 0 | 0 | 9,362 | 0 |
| deltablue | 2,427 | 885 | 1,230 | 0 | 0 | 0 | 0 | 312 | 0 |
| chaos | 483,627 | 293,501 | 190,033 | 93 | 0 | 0 | 0 | 0 | 0 |
| nqueens | 541,065 | 460,422 | 40,324 | 40,319 | 0 | 0 | 0 | 0 | 0 |
| fannkuch | 5,956,610 | 3,462,490 | 1,870,591 | 623,529 | 0 | 0 | 0 | 0 | 0 |
| generators | 100,000 | 100,000 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| comprehensions | 180 | 54 | 108 | 0 | 0 | 0 | 18 | 0 | 0 |
| coroutines | 2 | 1 | 0 | 0 | 0 | 0 | 1 | 0 | 0 |
| unpack_sequence | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| deepcopy | 424 | 0 | 244 | 0 | 180 | 0 | 0 | 0 | 0 |
| deepcopy_reduce | 1 | 0 | 1 | 0 | 0 | 0 | 0 | 0 | 0 |
| deepcopy_memo | 100 | 0 | 100 | 0 | 0 | 0 | 0 | 0 | 0 |
| json_dumps | 17,002 | 0 | 4,001 | 0 | 0 | 9,000 | 0 | 0 | 4,001 |
| json_loads | 240 | 0 | 180 | 0 | 0 | 0 | 0 | 0 | 60 |
| pickle | 820 | 0 | 60 | 400 | 0 | 340 | 20 | 0 | 0 |
| unpickle | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| regex_compile | 1,154,647 | 824,419 | 227,373 | 96,801 | 0 | 6,054 | 0 | 0 | 0 |
| logging_format | 90 | 20 | 70 | 0 | 0 | 0 | 0 | 0 | 0 |
| logging_silent | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| logging_simple | 90 | 20 | 70 | 0 | 0 | 0 | 0 | 0 | 0 |
| xml_etree_generate | 152,061 | 51,540 | 521 | 0 | 0 | 0 | 100,000 | 0 | 0 |
| xml_etree_iterparse | 361 | 20 | 241 | 0 | 0 | 30 | 70 | 0 | 0 |
| xml_etree_parse | 31 | 0 | 31 | 0 | 0 | 0 | 0 | 0 | 0 |
| xml_etree_process | 108 | 108 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |

In total, over one pass (each kernel's boxes per loop times its registry loop
count, summed over the 29 kernels):

| Site class | Boxes per pass | Share |
|---|---:|---:|
| unattributed | 7,443,289 | 51.7% |
| call adapter | 4,675,748 | 32.5% |
| list | 972,436 | 6.8% |
| iteration | 730,260 | 5.1% |
| dict | 232,084 | 1.6% |
| tuple | 216,074 | 1.5% |
| attribute | 71,242 | 0.5% |
| parser | 52,010 | 0.4% |
| **total** | **14,393,143** | |

No boxes per loop fall in the number, compare, repr, sequence, call, set,
cell/structseq, interpreter, field, frame or compiler classes. (Whole-process
dumps, as `summarize_stats.py` prints them, also show a few field boxes from
startup; they difference to zero.)

**Against the checkpoint.** Every kernel's box total is the same as in
`checkpoint-20261103-C-kernels-boxes.json`, to the box. Steps 5 to 6c change
how boxes are freed, not where they are made, and V1 adds counters. To compare
the classes, each site in `checkpoint-20261103-C-box-callers.json` was
assigned the class whose counter C now increments at that site:

- `_PyBorrow_ValueVector`, `_PyNativeScope_Objects` and `_PyBorrow_TupleItems`
  under calls → call adapter;
- `_PyBorrow_TupleItems` under `vgetargskeywords` → parser;
- every interpreter site (`_PyEval_EvalFrameLoop`, `_PyValueTemps_Box`,
  `borrow_or_box`, `value_binary_operation`) → unattributed;
- `listiter_next` and `tupleiter_next` → iteration;
- `list_pop`, `PyList_GetItemRef` and `array_new` → list;
- `_PyDict_NextEntryRefKnownHash` and `dict_getitemref_owned` → dict;
- `_PyObject_TryGetInstanceAttribute` → attribute;
- `store_tuple_elements`, `unicode_format_getnextarg`, `filter_search`,
  `compute_abstract_methods` and `StopIteration_init` → tuple.

Script: `build/pyjit-link-wave1/remeasure-box-compare.py`. With this mapping,
26 of the 29 kernels match class by class, to the box. The other three
differ by boxes that the checkpoint charged to an interpreter site and that C
counts under list or tuple:

- nqueens: 40,319 (`value_binary_operation`, `ceval.c:1028` → list);
- chaos: 93 (→ list);
- comprehensions: 18 (→ tuple).

*Hypothesis:* these are tail calls from list or tuple code. The checkpoint's
`__builtin_return_address` histogram charges a tail call to the caller's
caller, as that report warns.

The two sources therefore agree. Half of all boxes (51.7%) come from the
interpreter, which V1 has not annotated yet ("unattributed"). A third (32.5%)
come from the call adapters that box scalar arguments for native calls. The
rest come from C code reading inline elements out of lists, iterators, dicts
and tuples.

## Exception save/restore and the triage's attribution

Method: the triage's own, from its `decompose.py`. The LD_PRELOAD sampler
`pyjit/tools/sampler/perf-triage-sampler.c` ran at `SAMPLER_US=500` and
`SAMPLER_DEPTH=1`. The triage's symbol classifier was copied verbatim. Its
eval-loop bucket is `_PyEval_EvalFrameLoop` against `_PyEval_EvalFrameDefault`;
its exception bucket is `PyErr_GetRaisedException`, `PyErr_SetRaisedException`,
`_PyValue_RestoreException`, `PyErr_Occurred` and three restore helpers; its
bridge bucket is `value_*`, `_PyValueTemps_*`, `_PyValue_*`, `PyValue_*`,
`call_transaction_*`, `_PyNativeScope_*` and `_PyBorrow_*`. Each kernel ran
for about 1.2 s per build: `round(1.2e9 / t)` loops, where t is the triage's
quiet F time for B and C and its U time for U. That gives 979–1,372 samples
per profile. For each category the share of the gap to U is
(X·f_X − U·f_U)/(X − U), where X is the build, f the category's fraction of
the build's samples, and the times are this report's quiet-window medians. The
share of the B−C saving is (B·f_B − C·f_C)/(B − C). Script:
`build/pyjit-link-wave1/remeasure-sample.py`; raw data:
`remeasure-20261104-attribution.json`. The kernels are the triage's five worst
by its own attribution's F/U: deepcopy_memo, richards, deltablue, deepcopy and
logging_silent. Ranked by the triage's quiet table instead, deepcopy_reduce
would replace richards.

Means over the five kernels:

| Category | Triage: share of F−U gap | Now: share of B−U gap | Now: share of C−U gap | Share of the B−C saving | B time, % of B | C time, % of B | U time, % of B |
|---|---:|---:|---:|---:|---:|---:|---:|
| Interpreter loop's own time | 30% | 25% | 32% | 2% | 31.8 | 31.3 | 13.3 |
| Exception save/restore | 25% | 30% | 14% | 83% | 22.3 | 8.2 | 0.0 |
| Value bridging and boxing | 22% | 24% | 28% | 10% | 17.6 | 15.8 | 0.0 |
| Attribute lookup | 6% | 5% | 6% | 2% | 5.1 | 4.8 | 1.5 |
| Frames | 3% | 3% | 5% | −1% | 4.9 | 5.1 | 2.4 |
| Other (object work, allocation, dicts) | 14% | 13% | 16% | 5% | 18.3 | 17.4 | 8.1 |
| **Total** | | | | | **100** | **82.7** | **25.4** |

The specialiser bucket is below 0.5% everywhere and is left out of the table.
The triage's 29-kernel means were 39%, 25%, 17%, 3%, 2% and 14%. Its 25% for
exception save/restore holds on these five kernels too.

| Kernel | Triage F/U | B/U | C/U | C/B | Samples (C, B, U) | Exception share of the gap: triage, B, C | Exception time, % of B's time: B → C |
|---|---:|---:|---:|---:|---|---|---|
| deepcopy_memo | 5.42 | 5.55 | 4.65 | 0.838 | 1,135, 1,372, 1,233 | 22%, 26%, 13% | 21.4 → 8.4 |
| richards | 4.83 | 3.04 | 2.66 | 0.873 | 979, 1,138, 1,246 | 27%, 35%, 19% | 23.3 → 10.3 |
| deltablue | 4.51 | 4.42 | 3.47 | 0.786 | 981, 1,222, 1,163 | 28%, 32%, 16% | 25.0 → 9.1 |
| deepcopy | 3.90 | 3.72 | 3.08 | 0.827 | 1,025, 1,185, 1,141 | 24%, 27%, 11% | 19.7 → 6.4 |
| logging_silent | 3.66 | 3.74 | 3.03 | 0.810 | 1,086, 1,257, 1,187 | 27%, 30%, 12% | 22.0 → 6.6 |

Steps 5 to 6c therefore removed 63% of the time spent in exception
save/restore on these kernels (22.3% of B's time → 8.2%). What remains is
about a third of the bucket. The triage's richards ratio (4.83) came from its
loaded run; its quiet run gave 3.04, which B matches.

With about 1,000 samples per profile, each fraction has a standard error of
about 1.3 percentage points. The share of a B−C saving divides by a
difference of 13–21% of B's time, so for one kernel it can be off by 10
points or more. The five-kernel means are steadier. The first table's
categories are disjoint and follow the triage's; "other" includes library and
allocator code.

Across all 29 kernels, the B→C saving tracks the triage's per-kernel
exception fraction (`fracF` in `checkpoint-20261103-triage-gap.json`), with
Pearson r 0.85. json_loads (exception fraction 0.26) saves 25.5%. coroutines,
unpack_sequence, fannkuch and nbody (fractions 0.001–0.05) save between
−0.6% and 1.7%.

A deeper profile of C (`SAMPLER_DEPTH=4`; deltablue, deepcopy_memo and
richards; raw data not kept) shows where the remaining save/restore samples
come from:

- `value_reader_complete` and the loop in `value_operation_close3`
  (`Python/ceval.c:887`). That function still saves and restores around each
  of its three releases; it is a loop release, class C of the packet's §3.1,
  which is left for step 7.
- `native_call_finish`: 78% of deepcopy_memo's `PyErr_GetRaisedException`
  samples.
- `call_transaction_invoke`.
- `PyErr_Occurred` tests in `_PyObject_GetAttrValue`,
  `_PyObject_GenericGetAttrWithDict` and the eval loop.

## What changed

**Exception save/restore.** This bucket accounts for most of the change. On
the five kernels its time fell by 63%, which is 83% of what C saves against
B. Its share of the gap to upstream fell from 30% to 14%. The source agrees.
Counting `Objects/`, `Python/` and `Modules/` in each worktree:

| Call | B | C | U |
|---|---:|---:|---:|
| `_PyValue_RestoreException(` | 831 | 136 | 0 |
| `PyErr_GetRaisedException(` | 1,279 | 588 | 194 |
| `PyErr_Occurred()` | 4,017 | 3,608 | 2,426 |

Step 5 stated the release contract (freeing an exact int or float box never
runs Python or touches the exception). It deleted the saves inside
`value_operation_refresh` and the other revalidation functions, and
`_PyValueTemps_PublishObject`. Step 6a removed the saves around plain
releases, 6b inlined the exception-saving release helpers (such as
`type_mutation_close` and `dict_close_value`), and 6c dropped redundant
`|| PyErr_Occurred()` terms. This measurement compares only B and C, so it
cannot split the saving between these steps. *Hypothesis:* step 5's removal
of the per-word save in the interpreter's revalidation and 6a/6b's sweep of
dictionary and type code contribute most. Across the 29 kernels, the saving
rises with the triage's exception fraction (r 0.85; see the previous
section). The five kernels that gain most (json_loads, unpickle, deltablue,
pickle and json_dumps) have fractions of 0.15 to 0.26. For json_loads, the
largest, the triage named `dict_setdefault_object_owned`, `_PyValue_Hash`
and `insertdict`. The remaining third sits in the loop and reader releases
of `ceval.c` (`value_operation_close3`, `value_reader_complete`), in
`native_call_finish` and in standalone `PyErr_Occurred` tests: the sites the
packet leaves for step 7.

**The interpreter loop's own time.** This did not change: 31.8% of B's time
under B, 31.3% under C, against 13.3% for U. Because the gap shrank, its share
of the gap rose from 25% to 32%, and it is again the largest bucket. The words
are identical (dispatch C/U 0.805 at both commits). Step 5 changed the
generated handlers: a handler-local `ir_displaced` replaced the
`pending_local` slot, and the uses of `value_temps` in
`generated_ir_cases.c.h` fell from 803 to 616. Neither moved the loop's self
time measurably. *Hypothesis:* the cost is
still the state that the triage found spilled to the stack (`next_instr`,
`frame` and the slot base, whose addresses escape into out-of-line helpers),
plus the per-word temps record that the remaining handlers set up. Steps 5 to
6c do not touch these; register IR v2 is meant to. The kernels whose gap the
triage put almost entirely in the loop (coroutines 89%, unpack_sequence 97%,
nbody 83%) gain under 1% from B to C.

**Value bridging and boxing.** This bucket fell by about a tenth (17.6% →
15.8% of B's time) and supplies 10% of the saving. Its share of the gap rose
to 28%, second only to the loop. *Hypothesis:* the saving is step 5's
simpler retire path: `_PyValueTemps_CloseNonempty` became a plain release
loop, and the box is canonicalised and freed in place, without the
`PublishObject` round trip. V1 renamed and merged conversion functions
(`PyValue_AsPyObjectNewBoxed` into `PyValue_AsObject`) and added the box
counters, which compile only into the stats build. No cost change is expected
from V1, and none can be separated here. The box counts are identical to the
checkpoint's, so boxing itself did not change. What remains in this bucket is
the bridge machinery around native and attribute calls: `call_transaction_*`,
`value_operation_refresh`, `value_reader_complete` and
`_PyNativeScope_Begin`/`End`, which lead the bridge samples on all five
kernels.

**Other, attribute lookup and frames.** These barely moved (18.3% → 17.4%,
5.1% → 4.8% and 4.9% → 5.1% of B's time). That is within sampling error, and
none of the steps targets them. *Hypothesis:* the 5% of the saving in "other"
comes from the 6a/6b sweep in library code that the classifier puts in
"other" (dict and type internals once their saves are gone), not from new
work being removed.

**Memory.** Peak RSS did not change (B/C 0.996). C's startup is 320 KiB
higher than B's. C's text is only 27 KB larger than B's (`size`: 6,957,549
against 6,930,666 bytes), so code size does not explain it. The cause was not
investigated.

## Deviations, omissions and caveats

- **Load during pyperformance.** U's regex_v8 and the last four benchmarks
  ran at load 2.0–3.1 (see Conditions). The kernel times ran at load at most 1.17.
- **No instruction counts or cycle profiles.** `perf_event_paranoid=4`. The
  attribution is from SIGPROF samples, about 1,000 per profile.
- **Attribution on five kernels only**, as the brief asks. The triage's 25%
  was a 29-kernel mean; the matching five-kernel figure is 25% in the triage
  and 30% in this run's B.
- **B's and U's RSS and sampler runs** were taken 50 minutes before C's,
  before the candidate existed. RSS and sample fractions do not depend on
  load the way time does, and the attribution uses only quiet-window times.
- **Boxes** needed the checkpoint's `run.py` wrapper (missing handler-entry
  coverage lines in the dump). The class comparison depends on the mapping
  listed under Boxes.
- **Dispatch.** The variant shims differ from the checkpoint's by 5
  instructions per `run(1)` (see Dispatch counts).
- **Truncated `sys.version`** in B (see Builds).

## Files

In `pyjit/bench/results/`:
- `remeasure-20261104-{C,B,U}-kernels.json`: kernel times, one run record per
  kernel invocation.
- `remeasure-20261104-{C,B,U}-kernels-rss.json` and
  `remeasure-20261104-{C,B,U}-startup-rss.json`: peak RSS.
- `remeasure-20261104-pyperf/`: pyperf JSON files and logs.
- `remeasure-20261104-dispatch.json`, `remeasure-20261104-dispatch-variants.json`:
  dispatch counts.
- `remeasure-20261104-C-kernels-boxes.json` (15.9 MB): boxes on the stats
  build. The original dumps (10.9 MB) are kept uncommitted as
  `build/pyjit-link-wave1/remeasure-20261104-C-kernels-pystats.jsonl`.
- `remeasure-20261104-attribution.json`: sample fractions, top symbols and
  the attribution per kernel and build.

In `build/pyjit-link-wave1/`: `remeasure-jitstate.py` (the method-JIT probe),
`remeasure-time-kernels.py` (interleaved kernel timing),
`remeasure-pyperf-one.sh` and `remeasure-time-pyperf.sh` (pyperformance),
`remeasure-dispatch.py`, `remeasure-boxes.py`, `remeasure-box-compare.py`,
`remeasure-sample.py` (the sampler driver with the triage's classifier) and
its kernel runner `remeasure-kern2.py`, `remeasure-attrib.py` and
`remeasure-tables.py`. They ran from a scratch directory,
`/tmp/remeasure-20261104`, since removed, and name it in their paths. The C
builds `build/remeasure-release-c8a0b4dc54c` and
`build/remeasure-stats-c8a0b4dc54c` are kept. They read `Lib/` from the source worktree, which was removed, so
recreate it before use: `git -C /home/power/code/cpython worktree add
--detach build/worktrees/remeasure-c8a0b4dc54c c8a0b4dc54c`. B's build
directory was removed.
