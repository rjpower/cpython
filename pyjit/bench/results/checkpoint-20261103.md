# Checkpoint measurement, 2026-11-03: PyValue runtime with the register-IR interpreter

This is the first performance measurement of the universal-value runtime with
the register-IR interpreter (C, `dbce7b1260d`). It compares C against upstream
CPython (U, `3a440b6`) and the interim RC runtime (I, `eba351fee7d`). All three
are JIT-off release builds. Each ratio is the baseline's elapsed time (or RSS)
divided by C's, so a ratio above 1 favours C. A ratio of 0.212 means that C
takes 4.7 times as long as the baseline.

**Summary.** Across the 29 kernels, C takes 4.7 times as long as upstream
(geometric mean U/C 0.212) and 3.8 times as long as the interim runtime
(I/C 0.260). The eight pyperformance benchmarks (12 results, counting scimark's five
sub-benchmarks separately) give a geometric mean U/C of 0.144 and I/C of
0.179. C executes fewer words than upstream executes instructions
(geometric mean C/U 0.81), so the slowdown comes from the cost of each word,
not from the number of words. On the eight object-heavy kernels a C word costs
75–125 ns, against 3.4–7.5 ns for an upstream instruction. Boxing is too rare to
explain this: no Python-heavy kernel makes more than 0.11 boxes per word. Peak
RSS is 34% higher than upstream's (U/C 0.747). Most of that is a fixed startup
cost of 12.4 MiB, against upstream's 9.5 MiB.

`perf` is unavailable on this host (`kernel.perf_event_paranoid=4`) and gdb is
not installed. This report therefore has no instruction counts, cycle counts or
profiles. The explanations under "Where the time goes" are hypotheses built
from dispatch counts, box counts and timings.

## Builds

| Label | Commit | Build directory (under `build/`) | Configure | Compiler and flags | `sys.version` |
|---|---|---|---|---|---|
| C | `dbce7b1260d` (branch `boil/pyvalue-rc-universal`) | `checkpoint-release-dbce7b1260d` | `CC=gcc --with-mimalloc --with-computed-gotos --with-shapes` | gcc 15.2.0, `-O3 -g -fno-omit-frame-pointer -mno-omit-leaf-frame-pointer` | `3.16.0a1+dev (heads/boil/pyvalue-rc-universal:dbce7b1260d, Nov  3 2026, 19:11:24) [GCC 15.2.0]` |
| C stats | `dbce7b1260d` | `checkpoint-stats-dbce7b1260d` | the same, plus `--enable-pystats` | the same | `… Nov  3 2026, 19:11:40 …` |
| U | `3a440b65695` | `upstream-release-3a440b6` | `CC=gcc --with-mimalloc --with-computed-gotos` | the same | `3.16.0a1+dev (heads/main:3a440b65695, Nov  3 2026, 15:54:35) [GCC 15.2.0]` |
| I | `eba351fee7d` | `rc-integrated-release-20261101` | `CC=gcc --with-mimalloc --with-computed-gotos --with-shapes` | the same | `3.16.0a1+dev (heads/weaver/python-boil-the-ocean-dirty:eba351fee7d, Nov  1 2026, 10:56:42) [GCC 15.2.0]` |

Every build's `pybuilddir.txt` reads `build/lib.linux-x86_64-3.16`. No build
has the JIT (`sys._jit.is_available()` is False), PGO, LTO or the tail-calling
interpreter, and all three run with the GIL. C was built from a detached
worktree at `build/worktrees/checkpoint-dbce7b1260d`, which was removed after
the run. I was built from a dirty tree whose `Lib/` has since been deleted.
For this run its `Lib/` was restored with `git archive eba351fee7d Lib`, so the
library may differ from the one I was built and tested against. I imported
`json` from the restored `Lib/`. All three builds lack the same optional
modules (`_bz2`, `_dbm`, `_decimal`, `_gdbm`, `_hashlib`, `_ssl`, `_tkinter`,
`_uuid`, `_zstd`), and no benchmark here uses them. The machine is an AMD Ryzen
7 3800X (8 cores, 16 threads, 64 GB) running Linux 7.0.0-43.

The correctness smoke on C ran before any measurement: `-m test test_grammar
test_int test_float test_list test_dict test_generators test_json test_pickle
-j8` passed all 8 files (1,814 tests run, 65 skipped).

## Conditions

The kernel times and pyperformance runs were taken in one quiet window, from
19:26 to 19:42. The window opened when the 1-minute load average fell
to 1.52. `run.py` ran with `--max-load 4`. The highest load it sampled was 1.75
during C's run, 1.65 during U's and 1.13 during I's. pyperf records the 1-minute load average with each run. Every run
stayed below 2 except three of C's scimark sub-benchmarks: lu (up to 2.98),
monte_carlo (up to 2.68) and sor (up to 2.02). C's scimark_lu also has the
largest relative standard deviation in the table, 6%. pyperf flagged every
result as possibly unstable, because six processes give 18 values, too few for
its 1% criterion. The order was:
kernels under U, C and I, then each pyperformance benchmark under C, U and I in
turn. This task's dispatch-count workers had finished before the window opened.
During the window the task ran nothing else apart from a few sub-second
scripts that read result files.
Builds, the smoke test, RSS, pystats and dispatch counts all ran outside the
window, at load averages between 4 and 18. RSS does not depend on CPU load.
Each time vector holds five serial calls in one process, after one warm-up
call. These samples are correlated; they are not independent process pairs.

## Kernels: time

Command: `run.py --set kernels --metric time --max-load 4`, using the
registry's fixed loop counts, with no retries or calibration. Raw data:
`checkpoint-20261103-{C,U,I}-kernels.json`. Each value is the median of five
timed calls of `run(loops)`, in milliseconds per loop. Spread is
`100 * (max / min - 1)` over the five calls. The JSON files keep the raw
vectors, minima and maxima.

| Kernel | C ms | C spread % | U ms | U spread % | I ms | I spread % | U/C | I/C |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| richards | 485.1 | 1.9 | 70.97 | 2.8 | 69.38 | 1.8 | 0.146 | 0.143 |
| go | 1,556 | 0.8 | 112 | 11.6 | 153.1 | 4.4 | 0.072 | 0.098 |
| nbody | 1,455 | 6.8 | 96.85 | 0.9 | 121.2 | 1.3 | 0.067 | 0.083 |
| float | 937.9 | 7.5 | 77.1 | 5.5 | 96.5 | 7.1 | 0.082 | 0.103 |
| spectral_norm | 1,596 | 6.8 | 106.3 | 0.3 | 104.6 | 0.7 | 0.067 | 0.066 |
| raytrace | 3,331 | 5.3 | 313.1 | 1.8 | 358.1 | 0.9 | 0.094 | 0.107 |
| deltablue | 68.44 | 3.2 | 3.594 | 0.9 | 4.853 | 1.3 | 0.053 | 0.071 |
| chaos | 600.1 | 2.8 | 64.55 | 10.1 | 77.02 | 0.5 | 0.108 | 0.128 |
| nqueens | 210.6 | 2.7 | 90.02 | 10.8 | 118 | 1.1 | 0.427 | 0.560 |
| fannkuch | 1,055 | 5.5 | 385.3 | 1.4 | 510.1 | 0.7 | 0.365 | 0.484 |
| generators | 93.45 | 1.9 | 31.94 | 2.2 | 51.32 | 9.1 | 0.342 | 0.549 |
| comprehensions | 0.05775 | 3.2 | 0.01704 | 0.5 | 0.02439 | 9.2 | 0.295 | 0.422 |
| coroutines | 53.48 | 0.5 | 25.29 | 13.3 | 39 | 0.6 | 0.473 | 0.729 |
| unpack_sequence | 0.037 | 0.5 | 0.01327 | 6.2 | 0.02742 | 1.3 | 0.359 | 0.741 |
| deepcopy | 1.578 | 8.8 | 0.3004 | 2.3 | 0.3667 | 0.1 | 0.190 | 0.232 |
| deepcopy_reduce | 0.01676 | 1.3 | 0.003631 | 3.4 | 0.004411 | 0.8 | 0.217 | 0.263 |
| deepcopy_memo | 0.2766 | 8.1 | 0.02911 | 1.1 | 0.03816 | 0.6 | 0.105 | 0.138 |
| json_dumps | 43.04 | 3.2 | 10.93 | 0.4 | 12.29 | 2.4 | 0.254 | 0.286 |
| json_loads | 1.576 | 10.0 | 0.5715 | 0.4 | 0.5812 | 9.6 | 0.363 | 0.369 |
| pickle | 0.5169 | 1.7 | 0.2658 | 1.7 | 0.2621 | 0.7 | 0.514 | 0.507 |
| unpickle | 0.6267 | 7.7 | 0.3483 | 0.1 | 0.3478 | 1.3 | 0.556 | 0.555 |
| regex_compile | 672.3 | 8.2 | 170 | 7.1 | 224.2 | 9.5 | 0.253 | 0.333 |
| logging_format | 0.4227 | 3.6 | 0.07454 | 0.9 | 0.09279 | 3.7 | 0.176 | 0.219 |
| logging_silent | 0.007382 | 3.9 | 0.00112 | 2.0 | 0.001355 | 6.8 | 0.152 | 0.184 |
| logging_simple | 0.3958 | 2.0 | 0.06781 | 6.5 | 0.0775 | 2.6 | 0.171 | 0.196 |
| xml_etree_generate | 333.2 | 0.7 | 127.2 | 4.0 | 144.9 | 2.5 | 0.382 | 0.435 |
| xml_etree_iterparse | 222.1 | 1.0 | 111.2 | 5.7 | 113.3 | 2.3 | 0.501 | 0.510 |
| xml_etree_parse | 245.8 | 1.0 | 159.4 | 1.9 | 167.9 | 1.3 | 0.649 | 0.683 |
| xml_etree_process | 243.1 | 0.5 | 84.73 | 0.6 | 105 | 3.9 | 0.348 | 0.432 |
| **geometric mean** | | | | | | | **0.212** | **0.260** |

Five worst kernels against U (U/C): deltablue 0.053, nbody 0.067, spectral_norm 0.067, go 0.072, float 0.082.
Five best kernels against U: xml_etree_parse 0.649, unpickle 0.556, pickle 0.514, xml_etree_iterparse 0.501, coroutines 0.473.

C's spread is at most 10.0% (json_loads), and I's at most 9.6%. Four U values
spread more: go 11.6%, chaos 10.1%, nqueens 10.8% and coroutines 13.3%. Those
four ratios are less certain, but none would change the picture.

## Kernels: peak RSS

Command: `run.py --set kernels --metric rss`. Each value is the median of three
processes, in MiB of peak RSS. Raw data:
`checkpoint-20261103-{C,U,I}-kernels-rss.json`. The runs were made under load
(see Conditions).

| Kernel | C MiB | C spread % | U MiB | U spread % | I MiB | I spread % | U/C | I/C |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| richards | 13.43 | 17.5 | 9.312 | 13.4 | 11.3 | 19.3 | 0.693 | 0.841 |
| go | 15.38 | 16.5 | 11.18 | 9.9 | 12.79 | 23.4 | 0.727 | 0.832 |
| nbody | 13.64 | 5.3 | 9.57 | 6.0 | 11.36 | 5.7 | 0.702 | 0.833 |
| float | 21.16 | 3.6 | 26.59 | 2.2 | 28.02 | 1.9 | 1.256 | 1.324 |
| spectral_norm | 13.44 | 2.1 | 9.5 | 4.9 | 11.17 | 3.1 | 0.707 | 0.831 |
| raytrace | 15.07 | 23.7 | 10.56 | 24.8 | 12.77 | 34.5 | 0.701 | 0.847 |
| deltablue | 14.66 | 5.5 | 10.4 | 10.4 | 12.11 | 12.6 | 0.710 | 0.826 |
| chaos | 14.83 | 8.3 | 11.21 | 0.0 | 13.15 | 7.0 | 0.756 | 0.887 |
| nqueens | 12.56 | 3.4 | 9.41 | 5.2 | 11.11 | 4.9 | 0.749 | 0.884 |
| fannkuch | 12.54 | 4.5 | 9.406 | 7.6 | 11.11 | 4.2 | 0.750 | 0.886 |
| generators | 23.54 | 2.5 | 22.23 | 2.2 | 23.96 | 2.6 | 0.944 | 1.018 |
| comprehensions | 16.48 | 40.7 | 11.84 | 41.3 | 14.15 | 59.8 | 0.719 | 0.859 |
| coroutines | 12.66 | 3.3 | 9.406 | 4.2 | 11.15 | 3.4 | 0.743 | 0.881 |
| unpack_sequence | 12.81 | 35.2 | 9.391 | 31.1 | 11.39 | 42.7 | 0.733 | 0.889 |
| deepcopy | 15.31 | 9.7 | 10.74 | 7.4 | 13.24 | 11.4 | 0.702 | 0.865 |
| deepcopy_reduce | 15.17 | 2.1 | 10.88 | 0.6 | 13.02 | 1.8 | 0.717 | 0.858 |
| deepcopy_memo | 15.15 | 0.2 | 10.84 | 0.8 | 13.07 | 1.6 | 0.716 | 0.862 |
| json_dumps | 16.1 | 23.6 | 11.44 | 12.2 | 14.77 | 27.8 | 0.711 | 0.918 |
| json_loads | 16.73 | 1.7 | 11.93 | 2.0 | 14.7 | 2.5 | 0.713 | 0.879 |
| pickle | 16.36 | 27.5 | 11.75 | 24.5 | 13.91 | 35.5 | 0.718 | 0.850 |
| unpickle | 16.18 | 0.2 | 11.6 | 2.0 | 13.88 | 0.4 | 0.717 | 0.858 |
| regex_compile | 19.49 | 35.6 | 13.23 | 43.1 | 16.62 | 49.3 | 0.679 | 0.853 |
| logging_format | 19.71 | 24.7 | 14.39 | 21.2 | 17.73 | 26.4 | 0.730 | 0.900 |
| logging_silent | 18.15 | 1.6 | 12.89 | 0.6 | 16.17 | 0.3 | 0.711 | 0.891 |
| logging_simple | 19.31 | 1.2 | 14.14 | 0.8 | 17.41 | 1.6 | 0.732 | 0.902 |
| xml_etree_generate | 22.51 | 16.9 | 17.9 | 9.5 | 21.07 | 19.2 | 0.795 | 0.936 |
| xml_etree_iterparse | 23.22 | 1.3 | 17.54 | 0.7 | 21.62 | 0.9 | 0.755 | 0.931 |
| xml_etree_parse | 22.44 | 0.6 | 16.88 | 0.1 | 21.01 | 1.8 | 0.752 | 0.936 |
| xml_etree_process | 23.28 | 1.2 | 18.35 | 1.9 | 21.99 | 0.1 | 0.788 | 0.944 |
| **geometric mean** | | | | | | | **0.747** | **0.893** |

The high spreads (up to 60%, in comprehensions, regex_compile and
unpack_sequence) come from the first of the three processes. That process
compiles the kernel's `.pyc` into a fresh cache prefix and peaks higher; the
median discards it. Most of C's extra memory is a fixed startup cost. The
memory group's `startup` and `imports` rows (`run.py --set memory --only
startup,imports --metric rss`, raw data in
`checkpoint-20261103-{C,U,I}-startup-rss.json`) give:

| Row | C KiB | U KiB | I KiB | U/C | I/C |
|---|---:|---:|---:|---:|---:|
| startup | 12,664 | 9,688 | 11,368 | 0.765 | 0.898 |
| imports | 15,460 | 11,308 | 13,544 | 0.731 | 0.876 |

Only two kernels use less memory under C. float holds 100,000 points whose
coordinates are inline floats, and C needs 21.2 MiB for it against U's 26.6 MiB
(U/C 1.256). generators is close to even (0.944).

## pyperformance subset

These are eight pure-Python benchmarks from pyperformance 1.14.0
(`/home/power/.cache/uv/archive-v0/xitntS2hDvEA0_ce/pyperformance/data-files/benchmarks`).
They have no third-party dependencies, and the kernels do not cover them:
hexiom, pyflate, scimark (five sub-benchmarks), regex_v8, mdp, async_tree
(argument `none`, pyperformance's default), unpickle_pure_python
(`bm_pickle/run_benchmark.py --pure-python unpickle`) and
typing_runtime_protocols. Each `run_benchmark.py` ran directly, under pyperf
2.10.0, with `--processes 6` and all other settings at pyperf's defaults: one
warm-up and three values per process, with loops calibrated by pyperf.
Settings were the same for all three interpreters. Each interpreter had its
own venv under `build/pyperf-venvs/checkpoint-{C,U,I}`, holding a copy of the
pure-Python pyperf package, and the venvs were deleted afterwards. psutil was
not installed, so pyperf recorded less system metadata. Values are mean ±
standard deviation in milliseconds over the values from the six processes.
Raw data: `checkpoint-20261103-pyperf/{C,U,I}-<benchmark>.json`.

| Benchmark | C ms | U ms | I ms | U/C | I/C | values per interpreter (C, U, I) | loops (C, U, I) |
|---|---:|---:|---:|---:|---:|---|---|
| hexiom | 59.68 ± 2.3 | 6.392 ± 0.35 | 8.905 ± 0.27 | 0.107 | 0.149 | 18, 18, 18 | 2, 32, 16 |
| pyflate | 3,568 ± 45 | 405.4 ± 14 | 499.5 ± 16 | 0.114 | 0.140 | 18, 18, 18 | 1, 1, 1 |
| scimark_fft | 3,131 ± 71 | 378.8 ± 7.4 | 432.9 ± 15 | 0.121 | 0.138 | 18, 18, 18 | 1, 1, 1 |
| scimark_lu | 851.7 ± 51 | 149.9 ± 3.7 | 150.1 ± 6.2 | 0.176 | 0.176 | 18, 18, 18 | 1, 1, 1 |
| scimark_monte_carlo | 991.5 ± 28 | 73.22 ± 3.5 | 105.1 ± 2.7 | 0.074 | 0.106 | 18, 18, 18 | 1, 2, 1 |
| scimark_sor | 1,883 ± 56 | 128.7 ± 1.2 | 191.7 ± 7.3 | 0.068 | 0.102 | 18, 18, 18 | 1, 1, 1 |
| scimark_sparse_mat_mult | 46.97 ± 1.6 | 5.748 ± 0.18 | 7.19 ± 0.097 | 0.122 | 0.153 | 18, 18, 18 | 4, 32, 16 |
| regex_v8 | 49.08 ± 2.5 | 21.01 ± 0.72 | 21.78 ± 0.62 | 0.428 | 0.444 | 18, 18, 18 | 4, 8, 8 |
| mdp | 6,948 ± 156 | 1,335 ± 27 | 1,637 ± 47 | 0.192 | 0.236 | 18, 18, 18 | 1, 1, 1 |
| async_tree_none | 1,243 ± 44 | 345.6 ± 16 | 391.6 ± 18 | 0.278 | 0.315 | 18, 18, 18 | 1, 1, 1 |
| unpickle_pure_python | 2.701 ± 0.066 | 0.2634 ± 0.0035 | 0.3601 ± 0.002 | 0.097 | 0.133 | 18, 18, 18 | 2, 32, 16 |
| typing_runtime_protocols | 0.7143 ± 0.0066 | 0.1607 ± 0.0037 | 0.2102 ± 0.0081 | 0.225 | 0.294 | 18, 18, 18 | 256, 1024, 512 |
| **geometric mean** | | | | **0.144** | **0.179** | | |

## Dispatch counts

Command: `pyjit/bench/ir/run.py --ir-python C/python --upstream-python U/python
--commit dbce7b1260d --kernels NAME --out …`. `--commit` makes the worker use
C's own `dis`, `opcode` and metadata files; the tool's default is
`eba351fee7d`. The 17 single-module kernels ran as separate invocations, six
at a time; a single serial run would have taken more than an hour. `ir/run.py`
calls `setup(None)`, which the four multi-variant modules (deepcopy, pickle,
logging, xml_etree) reject. Their 12 variants therefore ran through shim
modules whose `setup()` passes the variant name; the worker, its arguments and
its environment were `ir/run.py`'s own. Raw data:
`checkpoint-20261103-dispatch.json` and
`checkpoint-20261103-dispatch-variants.json`. They keep the per-kernel top
words, constant loads, slot traffic and other counts.

The counts are `sys.monitoring` INSTRUCTION events during one `run(1)` call
after a warm-up `run(1)`: words for C, instructions for U, with NOT_TAKEN
excluded. They count instrumented handlers, and monitoring reports a
specialised instruction under its generic name. One `run(1)` is not the
registry's loop count. For kernels whose loop is tiny (unpack_sequence,
comprehensions, pickle, unpickle, json_loads, deepcopy_reduce,
logging_silent), the counts are small, and the roughly ten instructions of the
counting script around the call matter.

| Kernel | U instructions | C words | C/U |
|---|---:|---:|---:|
| fannkuch | 70,923,145 | 53,069,157 | 0.75 |
| raytrace | 41,877,624 | 29,606,956 | 0.71 |
| go | 28,667,983 | 18,762,468 | 0.65 |
| nbody | 28,581,241 | 19,300,999 | 0.68 |
| spectral_norm | 19,045,221 | 17,011,597 | 0.89 |
| nqueens | 14,592,232 | 11,850,310 | 0.81 |
| float | 12,199,935 | 7,899,942 | 0.65 |
| chaos | 10,532,435 | 7,321,871 | 0.70 |
| richards | 9,784,097 | 5,789,061 | 0.59 |
| generators | 6,806,866 | 6,106,863 | 0.90 |
| coroutines | 3,884,596 | 3,277,628 | 0.84 |
| deltablue | 726,098 | 545,927 | 0.75 |
| unpack_sequence | 5,679 | 9,667 | 1.70 |
| comprehensions | 3,223 | 2,417 | 0.75 |
| json_dumps | 460,179 | 316,144 | 0.69 |
| json_loads | 4,788 | 4,175 | 0.87 |
| regex_compile | 22,225,651 | 18,001,919 | 0.81 |
| deepcopy | 33,271 | 29,762 | 0.89 |
| deepcopy_reduce | 406 | 348 | 0.86 |
| deepcopy_memo | 5,677 | 4,925 | 0.87 |
| pickle | 361 | 409 | 1.13 |
| unpickle | 378 | 360 | 0.95 |
| logging_format | 7,970 | 5,891 | 0.74 |
| logging_silent | 292 | 215 | 0.74 |
| logging_simple | 7,728 | 5,659 | 0.73 |
| xml_etree_generate | 8,673,041 | 6,773,775 | 0.78 |
| xml_etree_iterparse | 5,729,693 | 4,910,106 | 0.86 |
| xml_etree_parse | 1,543,379 | 1,173,357 | 0.76 |
| xml_etree_process | 7,577,085 | 6,074,732 | 0.80 |
| **geometric mean** | | | **0.81** |

Combining time and dispatch gives the time per executed word. Divide the time
per loop by the words in one `run(1)`. This is a proxy: it assumes each loop
does the same work as `run(1)`, and it charges time spent in C library code to
the words. For C-heavy kernels such as pickle, json_loads and xml_etree_parse,
the result is mostly library time.

| Kernel | C ns per word | U ns per instruction | Per-dispatch cost C/U | C boxes per word |
|---|---:|---:|---:|---:|
| richards | 83.8 | 7.25 | 11.6 | 0.005 |
| go | 83.0 | 3.91 | 21.2 | 0.012 |
| nbody | 75.4 | 3.39 | 22.2 | 0.021 |
| float | 118.7 | 6.32 | 18.8 | 0.038 |
| spectral_norm | 93.8 | 5.58 | 16.8 | 0.040 |
| raytrace | 112.5 | 7.48 | 15.1 | 0.045 |
| deltablue | 125.4 | 4.95 | 25.3 | 0.004 |
| chaos | 82.0 | 6.13 | 13.4 | 0.066 |
| nqueens | 17.8 | 6.17 | 2.9 | 0.046 |
| fannkuch | 19.9 | 5.43 | 3.7 | 0.112 |
| generators | 15.3 | 4.69 | 3.3 | 0.016 |
| comprehensions | 23.9 | 5.29 | 4.5 | 0.074 |
| coroutines | 16.3 | 6.51 | 2.5 | 0.000 |
| unpack_sequence | 3.8 | 2.34 | 1.6 | 0.000 |
| deepcopy | 53.0 | 9.03 | 5.9 | 0.014 |
| deepcopy_reduce | 48.2 | 8.94 | 5.4 | 0.003 |
| deepcopy_memo | 56.2 | 5.13 | 11.0 | 0.020 |
| json_dumps | 136.1 | 23.75 | 5.7 | 0.054 |
| json_loads | 377.5 | 119.36 | 3.2 | 0.057 |
| pickle | 1,263.7 | 736.30 | 1.7 | 2.005 |
| unpickle | 1,740.8 | 921.46 | 1.9 | 0.000 |
| regex_compile | 37.3 | 7.65 | 4.9 | 0.064 |
| logging_format | 71.8 | 9.35 | 7.7 | 0.015 |
| logging_silent | 34.3 | 3.84 | 8.9 | 0.000 |
| logging_simple | 69.9 | 8.77 | 8.0 | 0.016 |
| xml_etree_generate | 49.2 | 14.66 | 3.4 | 0.022 |
| xml_etree_iterparse | 45.2 | 19.41 | 2.3 | 0.000 |
| xml_etree_parse | 209.5 | 103.28 | 2.0 | 0.000 |
| xml_etree_process | 40.0 | 11.18 | 3.6 | 0.000 |

## Boxes

There are two sources, both from the C pystats build. On the 29 kernels they
agree to the box: every kernel's total is the same in both.

1. `run.py --set kernels --metric boxes` on `checkpoint-stats-dbce7b1260d`
   (raw data: `checkpoint-20261103-C-kernels-boxes.json`). This gives each
   kernel's int and float box totals, the per-instruction `BOX_STAT` counters
   and every parsed counter. It ran through a wrapper with two relaxations,
   and is otherwise `run.py` unchanged. First, C's dump has no "Handler entry
   coverage" lines, which `run.py` requires, so the handler metadata is
   recorded as empty. Second, the dump prints the flag
   `opcode[GET_ITER].specializable : 1` twice, and `run.py` rejects duplicate
   keys, so a repeated `.specializable` flag is accepted. That flag is not a
   counter.
2. The existing counters do not attribute boxes made by C callers. C's dump
   has no `Object box site[...]` counters, so `box_sites` is empty. The
   per-instruction `BOX_STAT` counts only boxes made inside IR handlers: none
   of fannkuch's 5.96 million boxes per loop, and 411,384 of raytrace's 1.34
   million. A measurement-only patch,
   `build/pyjit-link-wave1/checkpoint-box-callers.diff`, adds a histogram to
   `_PyValue_BoxInlineValue`. The histogram is keyed on
   `__builtin_return_address(0)` and on the caller's own return address
   (`__builtin_return_address(1)`; the build keeps frame pointers), and is
   written out at exit. Without the second level, most sites resolve only to
   the wrappers `PyValue_AsObject` or `_PyValueTemps_Box`. The patch was
   compiled into a copy of the stats build, `checkpoint-stats-boxcallers-dbce7b1260d`;
   no source tree was changed. Each kernel ran at `loops` and `3 * loops`
   after a warm-up, without `-X pystats`, and each site's count was
   differenced and symbolised with `addr2line -i`. Driver:
   `build/pyjit-link-wave1/checkpoint-box-callers.py`. Raw data:
   `checkpoint-20261103-C-box-callers.json`.

As `run.py` documents, the box counts are differences between processes, so
they include startup and setup variation. They count calls that box, not fresh
allocations or CPU cost. In the site names below, `A < B` means that A is
inlined into, or called by, B, and `← X` is the function that called that
chain. A site inside the eval loop is named after its IR instruction; the
caller of the eval loop says nothing about the site, so it is omitted.

### Top 20 box sites overall

"Boxes per pass" weights each kernel by its registry loop count. It is each
kernel's boxes per loop times that kernel's loops, summed over the 29 kernels:
the boxes in one timed call of every kernel.

| # | Box site (callee chain < caller) | Boxes per pass | Share | Main kernels (boxes per loop) |
|---:|---|---:|---:|---|
| 1 | `_PyBorrow_ValueVector (borrow.c:368) ← _Py_BuiltinCallFast_Value (ceval.c:1145)` | 1,904,552 | 13.2% | fannkuch 1,870,587, go 13,965, logging_format 10 |
| 2 | `PyValue_AsObject (pyvalue.c:131) < _PyValueTemps_Box (pyvalue.c:181) ← IR BUILD_SLICE (generated_ir_cases.c.h:4267)` | 1,747,169 | 12.1% | fannkuch 1,731,245, regex_compile 15,923, nqueens 1 |
| 3 | `PyValue_AsObject (pyvalue.c:131) < _PyValueTemps_Box (pyvalue.c:181) ← IR BUILD_SLICE (generated_ir_cases.c.h:4259)` | 1,731,245 | 12.0% | fannkuch 1,731,245 |
| 4 | `_PyNativeScope_Objects (native_scope.c:249) ← _PyCFunction_CallScoped (methodobject.c:543)` | 1,091,522 | 7.6% | float 300,000, raytrace 128,392, regex_compile 106,964 |
| 5 | `PyValue_AsObject (pyvalue.c:131) < _PyValueTemps_Box (pyvalue.c:181) ← IR STORE_SUBSCR (generated_ir_cases.c.h:18326)` | 862,590 | 6.0% | regex_compile 701,952, nqueens 80,638, raytrace 80,000 |
| 6 | `PyValue_AsObject (pyvalue.c:131) < _PyValueTemps_Box (pyvalue.c:181) ← value_binary_operation (ceval.c:1023)` | 779,917 | 5.4% | raytrace 227,668, nbody 200,020, chaos 55,349 |
| 7 | `PyValue_AsObject (pyvalue.c:131) ← listiter_next (listobject.c:3820)` | 703,260 | 4.9% | spectral_norm 676,260, deepcopy 90 |
| 8 | `_PyBorrow_ValueVector (borrow.c:368) ← _PyCallMethodDescriptorFast_Value (ceval.c:1194)` | 675,650 | 4.7% | regex_compile 7,414, xml_etree_iterparse 220, comprehensions 108 |
| 9 | `PyValue_AsObject (pyvalue.c:131) ← list_pop (listobject.c.h:252)` | 645,130 | 4.5% | fannkuch 623,529, go 21,601 |
| 10 | `PyValue_AsObject (pyvalue.c:131) < _PyValueTemps_Box (pyvalue.c:181) ← value_binary_operation (ceval.c:1019)` | 542,308 | 3.8% | raytrace 227,668, nbody 200,020, chaos 50,000 |
| 11 | `_PyBorrow_ValueVector (borrow.c:368) ← _Py_CallBuiltinClass_Value (ceval.c:1238)` | 427,364 | 3.0% | chaos 130,002, go 58,630, raytrace 41,672 |
| 12 | `IR COMPARE_OP (generated_ir_cases.c.h:7906)` | 379,766 | 2.6% | raytrace 205,692, chaos 84,076, regex_compile 4,931 |
| 13 | `IR COMPARE_OP (generated_ir_cases.c.h:7917)` | 375,322 | 2.6% | raytrace 205,692, chaos 84,076, go 1,447 |
| 14 | `IR SET_ADD (generated_ir_cases.c.h:17176)` | 339,464 | 2.4% | nqueens 339,464 |
| 15 | `PyValue_AsObject (pyvalue.c:131) ← IR INTERPRETER_EXIT (generated_ir_cases.c.h:11653)` | 288,654 | 2.0% | generators 100,000, xml_etree_generate 51,030, regex_compile 37,620 |
| 16 | `PyValue_AsObject (pyvalue.c:131) ← PyList_GetItemRef (listobject.c:308)` | 256,801 | 1.8% | regex_compile 96,801, pickle 400 |
| 17 | `_PyBorrow_ValueVector (borrow.c:368) ← _Py_BuiltinCallFastWithKeywords_Value (ceval.c:1170)` | 237,396 | 1.6% | raytrace 130,666, regex_compile 82,730, pickle 60 |
| 18 | `PyValue_AsObject (pyvalue.c:131) ← _PyDict_NextEntryRefKnownHash (dictobject.c:3127)` | 226,020 | 1.6% | json_dumps 9,000, pickle 340, xml_etree_iterparse 20 |
| 19 | `IR MAP_ADD (generated_ir_cases.c.h:14962)` | 216,000 | 1.5% | comprehensions 36 |
| 20 | `_PyBorrow_ValueVector (borrow.c:368) ← _PyNativeScope_BeginObjectVector (native_scope.c:174)` | 182,366 | 1.3% | chaos 25,031, json_dumps 4,001, regex_compile 2,797 |

### The five kernels with the most boxes per loop

**fannkuch**: 5,956,610 boxes per loop (int 5,956,610, float 0); histogram total 5,956,610; per-opcode BOX_STAT 0

| Site | Boxes per loop |
|---|---:|
| `_PyBorrow_ValueVector (borrow.c:368) ← _Py_BuiltinCallFast_Value (ceval.c:1145)` | 1,870,587 |
| `PyValue_AsObject (pyvalue.c:131) < _PyValueTemps_Box (pyvalue.c:181) ← IR BUILD_SLICE (generated_ir_cases.c.h:4259)` | 1,731,245 |
| `PyValue_AsObject (pyvalue.c:131) < _PyValueTemps_Box (pyvalue.c:181) ← IR BUILD_SLICE (generated_ir_cases.c.h:4267)` | 1,731,245 |
| `PyValue_AsObject (pyvalue.c:131) ← list_pop (listobject.c.h:252)` | 623,529 |
| `_PyBorrow_ValueVector (borrow.c:368) ← _PyNativeScope_BeginObjectVector (native_scope.c:174)` | 2 |

**raytrace**: 1,336,856 boxes per loop (int 779,378, float 557,478); histogram total 1,336,856; per-opcode BOX_STAT 411,384

| Site | Boxes per loop |
|---|---:|
| `PyValue_AsObject (pyvalue.c:131) < _PyValueTemps_Box (pyvalue.c:181) ← value_binary_operation (ceval.c:1023)` | 227,668 |
| `PyValue_AsObject (pyvalue.c:131) < _PyValueTemps_Box (pyvalue.c:181) ← value_binary_operation (ceval.c:1019)` | 227,668 |
| `IR COMPARE_OP (generated_ir_cases.c.h:7906)` | 205,692 |
| `IR COMPARE_OP (generated_ir_cases.c.h:7917)` | 205,692 |
| `_PyBorrow_ValueVector (borrow.c:368) ← _Py_BuiltinCallFastWithKeywords_Value (ceval.c:1170)` | 130,666 |

**regex_compile**: 1,154,647 boxes per loop (int 1,154,647, float 0); histogram total 1,154,647; per-opcode BOX_STAT 9,683

| Site | Boxes per loop |
|---|---:|
| `PyValue_AsObject (pyvalue.c:131) < _PyValueTemps_Box (pyvalue.c:181) ← IR STORE_SUBSCR (generated_ir_cases.c.h:18326)` | 701,952 |
| `_PyNativeScope_Objects (native_scope.c:249) ← _PyCFunction_CallScoped (methodobject.c:543)` | 106,964 |
| `PyValue_AsObject (pyvalue.c:131) ← PyList_GetItemRef (listobject.c:308)` | 96,801 |
| `_PyBorrow_ValueVector (borrow.c:368) ← _Py_BuiltinCallFastWithKeywords_Value (ceval.c:1170)` | 82,730 |
| `PyValue_AsObject (pyvalue.c:131) ← IR INTERPRETER_EXIT (generated_ir_cases.c.h:11653)` | 37,620 |

**spectral_norm**: 676,302 boxes per loop (int 16,942, float 659,360); histogram total 676,302; per-opcode BOX_STAT 0

| Site | Boxes per loop |
|---|---:|
| `PyValue_AsObject (pyvalue.c:131) ← listiter_next (listobject.c:3820)` | 676,260 |
| `_PyBorrow_ValueVector (borrow.c:368) ← _Py_CallBuiltinClass_Value (ceval.c:1238)` | 40 |
| `PyValue_AsObject (pyvalue.c:131) < _PyValueTemps_Box (pyvalue.c:181) ← value_binary_operation (ceval.c:1023)` | 1 |
| `_PyBorrow_ValueVector (borrow.c:368) ← _PyNativeScope_BeginObjectVector (native_scope.c:174)` | 0 |

**nqueens**: 541,065 boxes per loop (int 541,065, float 0); histogram total 541,065; per-opcode BOX_STAT 339,464

| Site | Boxes per loop |
|---|---:|
| `IR SET_ADD (generated_ir_cases.c.h:17176)` | 339,464 |
| `PyValue_AsObject (pyvalue.c:131) < _PyValueTemps_Box (pyvalue.c:181) ← IR STORE_SUBSCR (generated_ir_cases.c.h:18326)` | 80,638 |
| `_PyBorrow_ValueVector (borrow.c:368) ← _Py_CallBuiltinClass_Value (ceval.c:1238)` | 40,322 |
| `PyValue_AsObject (pyvalue.c:131) ← value_binary_operation (ceval.c:1028)` | 40,319 |
| `PyValue_AsObject (pyvalue.c:131) < _PyValueTemps_Box (pyvalue.c:181) ← value_binary_operation (ceval.c:1023)` | 40,319 |

In summary, most boxes come from four mechanisms:
- **Native call arguments.** Inline ints and floats passed to C functions are
  boxed: `_PyBorrow_ValueVector` under the builtin and method-descriptor fast
  calls, and `_PyNativeScope_Objects` under `_PyCFunction_CallScoped`. Examples
  are `perm1.pop(0)` and `perm1.insert(r, …)` in fannkuch, and `math.sin`,
  `cos` and `sqrt` in float.
- **Generic operations.** `value_binary_operation` boxes both operands of an
  unspecialised binary operation, such as nbody's `** (-1.5)`. COMPARE_OP
  boxes both operands in its generic path (raytrace, chaos). BUILD_SLICE boxes
  its int bounds (fannkuch's `perm[:k + 1]` and `perm[k::-1]`).
  STORE_SUBSCR's generic store boxes the stored value (regex_compile,
  nqueens).
- **C code reading containers.** Inline elements are boxed when C code reads
  them out of a list: `listiter_next` (spectral_norm's `enumerate(u)` over a
  list of floats), `list_pop`, `PyList_GetItemRef` and
  `_PyDict_NextEntryRefKnownHash`.
- **Instructions that store into containers or globals.** SET_ADD (nqueens),
  MAP_ADD (comprehensions) and STORE_GLOBAL (go) box the stored value.

## Where the time goes

**Diagnostic probes (not part of the record).** The probes below ran right
after the window, at load 1.2 to 4.7. Each value is the best of seven calls of
a function looping 200,000 times over the body shown, in nanoseconds per
iteration, the loop included. Script: `build/pyjit-link-wave1/checkpoint-probe.py`.
`dis.dis(f, adaptive=True)` on the same loops in an earlier probe run showed
specialised words (`FOR_ITER_RANGE`, `BINARY_OP_ADD_INT`, `COMPARE_OP_INT`,
`CALL_LEN`, `BINARY_OP_SUBSCR_LIST_INT`).

| Loop body (in `for i in range(N)`) | C ns per iteration | U ns per iteration | C/U |
|---|---:|---:|---:|
| for_pass | 73.0 | 17.2 | 4.2 |
| int_add | 99.7 | 36.0 | 2.8 |
| int_compare | 213.4 | 29.7 | 7.2 |
| float_add | 240.6 | 35.6 | 6.8 |
| float_mul | 233.4 | 37.8 | 6.2 |
| attr_load | 275.8 | 24.2 | 11.4 |
| attr_store | 364.4 | 25.3 | 14.4 |
| py_call | 724.1 | 51.0 | 14.2 |
| method_call | 871.2 | 48.3 | 18.0 |
| builtin_len | 778.1 | 34.3 | 22.7 |
| math_sqrt | 755.0 | 55.1 | 13.7 |
| list_load | 42.6 | 27.4 | 1.6 |
| list_store | 31.7 | 28.2 | 1.1 |
| tuple_unpack | 244.5 | 27.0 | 9.0 |
| new_instance | 1395.2 | 184.3 | 7.6 |

Bodies: `pass`; `x = x + 1`; `if i < 5: x = 1`; `x = x + 1.5`;
`x = x * 1.0`; `o.a`; `o.a = 2`; `g(1)` (a module-level function);
`o.m()`; `len(l)`; `s(2.0)` (with `s = math.sqrt`); `l[1]`; `l[1] = 5`;
`a, b = t`; `O()`, where `O.__init__` sets one attribute.

Across the kernels, two facts hold. First, on every Python-heavy kernel C
executes fewer words than upstream executes instructions (0.59–0.90). The only
kernels above 1 are unpack_sequence (1.70) and pickle (1.13), and both have
tiny counts. The slowdown is therefore a cost per word. Second, boxes are rare: at most 0.11 per word on
any Python-heavy kernel. A box is one call to a small allocator, so boxing can
explain only a small part of a 12–19× slowdown. That is a hypothesis: no
per-box cost was measured. The per-word cost falls into two groups. The eight
kernels dominated by attribute access, calls and float arithmetic (richards,
go, nbody, float, spectral_norm, raytrace, deltablue, chaos) cost 75–125 ns
per word, 11–25 times upstream's cost per instruction. The kernels dominated
by int, list, generator and coroutine work (nqueens, fannkuch, generators,
coroutines, comprehensions) cost 15–24 ns per word, 2.5–4.5 times upstream's.
The probes show the same split. A list load or store costs about what it costs
upstream. Int arithmetic and the bare loop cost 3–4 times as much. Float
arithmetic and int compare-and-branch cost 6–7 times as much. Attribute access
costs 11–14 times as much, and every kind of call (Python function, method,
builtin, `math.sqrt`) costs 700–870 ns, 14–23 times upstream's.

The five kernels that fare worst against U:

- **deltablue** (U/C 0.053; 68.4 ms against 3.59 ms per loop). C executes 0.75
  times upstream's dispatches (545,927 words against 726,098 instructions per
  `run(1)`), at 125 ns per word against 4.95 ns per instruction. There are only
  2,427 boxes per loop (0.004 per word), mostly argument vectors of native
  calls, so boxing plays no part. Of C's words, 35% are LOAD_ATTR (189,904),
  followed by CALL (58,188), RETURN_VALUE (51,422), LOAD_GLOBAL, COMPARE_OP
  and STORE_ATTR (21,506). *Hypothesis:* the time goes to instance attribute
  access and Python-to-Python calls. These are the slowest words in the probes
  (attribute load 11×, method call 18×), and they make up most of deltablue's
  words.
- **nbody** (0.067; 1,455 ms against 96.9 ms). The dispatch ratio is 0.68
  (19.3 M against 28.6 M), at 75 ns per word against 3.4 ns. nbody makes
  400,040 float boxes per loop, every one in `value_binary_operation`. That
  function boxes both operands of the unspecialised `** (-1.5)`, which runs
  200,020 times per loop (once for each pair of bodies at each step). This comes to 0.021 boxes per word.
  Even at an assumed 50 ns per box it would be 20 ms, 1.4% of C's time. There
  are no calls or attribute accesses in the hot loop. Of C's words, 36% are
  BINARY_OP (6.9 M) on floats, followed by STORE_SUBSCR into lists (1.5 M),
  UNPACK_SEQUENCE (1.2 M) and 2.5 M STORE_FAST. *Hypothesis:* the specialised
  float arithmetic words are themselves slow: the float add and multiply
  probes cost 6–7 times upstream's. The register form keeps 3.2 M
  LOAD_FAST_BORROW and 2.5 M STORE_FAST words, so it removes fewer moves than
  the 0.68 dispatch ratio might suggest.
- **spectral_norm** (0.067; 1,596 ms against 106 ms). The dispatch ratio is
  0.89, the smallest saving among the slow kernels, at 94 ns per word against
  5.6 ns. There are 676,260 float boxes per loop, all made by `listiter_next`:
  `enumerate(u)` runs over a list of inline floats, and C code boxes each
  element it reads. That is 0.040 boxes per word. Of C's words, 40% are
  BINARY_OP (6.76 M), mostly int arithmetic in `eval_A`, and there are 687,000
  CALLs, one per `eval_A` call. *Hypothesis:* the 687,000 Python calls, at
  roughly 700 ns each in the probe against 51 ns upstream, account for about
  0.45 s of C's 1.6 s. The int arithmetic and the float multiply-add account
  for most of the rest.
- **go** (0.072; 1,556 ms against 112 ms). The dispatch ratio is 0.65, at 83
  ns per word against 3.9 ns. There are 229,237 boxes per loop (0.012 per
  word): STORE_GLOBAL boxes the int it stores in the module dict (102,460),
  builtin-class calls box their arguments (58,631), and `list.pop` boxes the
  element it returns (21,601). LOAD_ATTR makes up 28% of C's words (5.28 M),
  and there are 1.14 M CALLs. *Hypothesis:* as for deltablue, attribute access
  and method calls carry the cost.
- **float** (0.082; 938 ms against 77.1 ms). The dispatch ratio is 0.65, at
  119 ns per word against 6.3 ns. There are 300,002 boxes per loop, all for
  arguments of native calls, boxed in `_PyNativeScope_Objects` under
  `_PyCFunction_CallScoped`. For each of the 100,000 points, `sin(i)` and
  `cos(i)` box the int `i` (200,000 int boxes) and `sqrt` boxes a float
  (100,000). That is 0.038 boxes per word. The word mix is LOAD_ATTR 1.70 M,
  BINARY_OP 1.10 M, STORE_ATTR 0.90 M and CALL 0.60 M. `Point` has
  `__slots__`, and each point takes nine slot stores, three each in
  `__init__`, `normalize` and `maximize`. The calls are the constructor, the
  methods and the `math` functions. *Hypothesis:* calls (700–870 ns each in
  the probes) and instance creation (7.6×) dominate, followed by slot
  attribute access. The probes measured attributes on a plain instance, not
  slot descriptors. Boxing the `math` arguments is a small part.

The kernels that fare best against U are xml_etree_parse (0.649), unpickle
(0.556), pickle (0.514), xml_etree_iterparse (0.501) and coroutines (0.473).
The first four spend most of their time in C library code (expat, `_pickle`),
where the interpreter's per-word cost matters less. coroutines executes 0.84
times upstream's dispatches, at 16 ns per word, in the cheap group. Among the
pyperformance benchmarks, regex_v8 (0.428), which spends its time in `_sre`,
and async_tree (0.278) fare best. hexiom, pyflate, scimark and
unpickle_pure_python, which are dominated by Python arithmetic, attribute
access and calls, sit at 0.07–0.18.

**Next measurement.** All of these hypotheses come down to the cost of single
handlers: CALL and frame setup, LOAD_ATTR and STORE_ATTR on shaped instances,
and float BINARY_OP and COMPARE_OP on inline values. Without `perf`, the next
step would be a per-handler cycle count, for example `rdtsc` around the
handler bodies in a measurement-only copy, like the box-caller patch. Lowering
`perf_event_paranoid` would allow a profile instead.

## Deviations, omissions and caveats

- **No micro set (item 6).** The registry gives micro benchmarks only the
  `instr` and `boxes` metrics, so `run.py --set micro --metric time` selects
  nothing; a probe produced `{'time': {}}`. It was also first on the scope-cut
  list.
- **No instruction counts or profiles.** `perf_event_paranoid=4`, and neither
  gdb nor any other sampling tool is installed.
- **I's library.** I ran with a `Lib/` restored from the commit, not the dirty
  tree it was built from (see Builds).
- **Boxes.** These needed the `run.py` wrapper and the caller patch described
  under Boxes. With `__builtin_return_address(0)`, a call that the compiler
  turns into a sibling (tail) call is charged to the caller's caller. Both
  levels of the histogram are kept in the raw JSON.
- **Dispatch counts.** These cover one `run(1)` under monitoring, not the timed
  loop counts. For seven kernels the counts are tiny (see Dispatch counts).
- **Load.** RSS, boxes and dispatch counts ran at load averages of 4 to 18.
  Only the time results come from the quiet window.

## Files

In `pyjit/bench/results/`:
- `checkpoint-20261103-{C,U,I}-kernels.json`: kernel time.
- `checkpoint-20261103-{C,U,I}-kernels-rss.json`: kernel peak RSS.
- `checkpoint-20261103-{C,U,I}-startup-rss.json`: startup and import RSS.
- `checkpoint-20261103-C-kernels-boxes.json`: `run.py` boxes on the C stats
  build.
- `checkpoint-20261103-C-box-callers.json`: per-caller box histogram per
  kernel, with addr2line symbols.
- `checkpoint-20261103-dispatch.json` and
  `checkpoint-20261103-dispatch-variants.json`: dispatch counts.
- `checkpoint-20261103-pyperf/`: pyperf JSON files and logs.

In `build/pyjit-link-wave1/`: `checkpoint-box-callers.diff` (the
measurement-only patch), `checkpoint-box-callers.py` (the histogram driver),
`checkpoint-run-boxes.py` (the `run.py` boxes wrapper) and
`checkpoint-probe.py` (the diagnostic probes). The candidate
builds `build/checkpoint-release-dbce7b1260d` and
`build/checkpoint-stats-dbce7b1260d` are kept, along with the patched copy
`build/checkpoint-stats-boxcallers-dbce7b1260d` (177 MB; it can be deleted).
These out-of-tree builds read `Lib/` from the source worktree, which was
removed as the brief asks. Before using them again, recreate it with `git -C
/home/power/code/cpython worktree add --detach
build/worktrees/checkpoint-dbce7b1260d dbce7b1260d`. Likewise, I needs its
`Lib/` restored, as described under Builds.
