# Second re-measure, after register IR v2 phase A and int51, 2026-11-04

This report measures the PyValue runtime with the register-IR interpreter after
representation steps 7, 8, 9a and 9b and register IR v2 steps B0a, A1, A2 and
A5a (C, `5de7bd62ef9`). It compares C with the
[last re-measure's](remeasure-20261104.md) candidate (B, `c8a0b4dc54c`) and
with upstream CPython (U, `3a440b6`). It repeats that report's tables and
method. All three builds are release builds, and no JIT runs in any of them
(see Builds).

Each ratio follows [measurement.md](../../design/measurement.md): the
baseline's elapsed time (or RSS) divided by C's. A ratio above 1 favours C.
U/C 0.577 means that C takes 1.73 times as long as U, and B/C 1.302 means
that C takes 0.768 times as long as B.

**Summary.**

- **Kernels.** Across the 29 kernels, C takes 1.73 times as long as upstream
  (geometric mean U/C 0.577) and 0.768 times as long as B (B/C 1.302). No
  kernel is slower than under B. B takes 2.26 times as long as U; the last
  re-measure measured the same binary at 2.27, so B reproduces it.
- **pyperformance.** The 12 results of the eight benchmarks give U/C 0.545
  (C takes 1.83 times as long as U) and B/C 1.359 (C/B 0.736).
- **Dispatch.** C executes exactly the same words as B: 0.805 words per
  upstream instruction (geometric mean). All of the saving is in the cost per
  word.
- **Boxes.** Unchanged on 23 kernels. Six rise; the per-pass total rises 3.8%,
  from 14,393,143 to 14,941,610, and 78% of the rise is deepcopy_memo. The
  rise is the one the step 9 notes report: `id()` keys, now inline, boxed at
  `CONTAINS_OP_DICT`. The notes' second source, `dict.get`, does not occur
  in this `copy.py`.
- **Where the saving comes from.** On the triage's five kernels, C's time is
  67.9% of B's. Exception save/restore is gone: 9.6% of B's time, no samples
  in C. That bucket gives 30% of the saving, value bridging 28% (17.9% →
  9.2% of B's time), "other" 21% and the interpreter loop's own time 17%
  (37.9% → 32.4%). The loop is now 44% of the C−U gap, against 25% for
  bridging. On generators and unpack_sequence, two of the new five worst
  kernels, it is 75% and 97%.
- **RSS.** As measured by the method, B/C 0.930 and U/C 0.773. That
  comparison is distorted by the page cache: on this ZFS machine the freshly
  linked C binary was fully in the page cache, and B and U were not. With all
  three binaries fully cached, B/C is 1.010 and U/C 0.829. Anonymous memory
  is the same in B and C (see Kernels: peak RSS).

`perf` is unavailable (`kernel.perf_event_paranoid=4`), so this report has no
instruction or cycle counts. The attribution uses the triage's LD_PRELOAD
SIGPROF sampler and its symbol classifier.

## Builds

| Label | Commit | Build directory (under `build/`) | Configure | `sys.version` | Method JIT |
|---|---|---|---|---|---|
| C | `5de7bd62ef9` (HEAD of `boil/pyvalue-rc-universal`) | `remeasure-release-5de7bd62ef9` | `CC=gcc --with-mimalloc --with-computed-gotos --with-shapes` | `3.16.0a1+dev (heads/boil/pyvalue-rc-universal:5de7bd62ef9, Nov  4 2026, 10:02:21) [GCC 15.2.0]` | natural admission off; 0 units |
| C stats | `5de7bd62ef9` | `remeasure-stats-5de7bd62ef9` | the same, plus `--enable-pystats` | `… Nov  4 2026, 10:02:29 …` | compiled out (`_PyJIT_BASELINE_SUPPORTED` is 0 under `Py_STATS`) |
| B | `c8a0b4dc54c` | `remeasure-release-c8a0b4dc54c` (reused, not rebuilt) | `CC=gcc --with-mimalloc --with-computed-gotos --with-shapes` | `3.16.0a1+dev (heads/boil/pyvalue-rc-universal:c8a0b4dc54c, Nov  4 2026, 01:03:50) [GCC 15.2.0]` | natural admission off; 0 units |
| U | `3a440b65695` | `upstream-release-3a440b6` | `CC=gcc --with-mimalloc --with-computed-gotos` | `3.16.0a1+dev (heads/main:3a440b65695, Nov  3 2026, 15:54:35) [GCC 15.2.0]` | not present |

All builds use gcc 15.2.0 (Ubuntu 15.2.0-16ubuntu1) with `-O3 -g
-fno-omit-frame-pointer -mno-omit-leaf-frame-pointer`. None has PGO, LTO or
the tail-calling interpreter (`_Py_TAIL_CALL_INTERP` is undefined). Every
build's `pybuilddir.txt` reads `build/lib.linux-x86_64-3.16`. C was built
with `make -j16` from a detached worktree,
`build/worktrees/remeasure-5de7bd62ef9`, configured out of tree with the
configure line in `remeasure-release-c8a0b4dc54c/config.log`. B's build reads
`Lib/` from its source worktree, which the last re-measure had removed, so
`build/worktrees/remeasure-c8a0b4dc54c` was recreated at the same commit. Both
worktrees were removed after the run. The machine is an AMD Ryzen 7 3800X (8
cores, 16 threads, 64 GB) running Linux 7.0.0-43, with the amd-pstate-epp
driver and the powersave governor. The build tree is on ZFS.

**Method-JIT state.** The last re-measure's probe
(`build/pyjit-link-wave1/remeasure-jitstate.py`) ran on C, C stats and B.
Natural admission reads False in each (`static int natural_admission;`,
`Python/jit_runtime.c:115` in C). After a 100,000-iteration `for` loop, a
100,000-iteration scalar `while` loop, 10,000 calls of a small function and
one `run(1)` of richards, none of the 44 functions checked was admitted: each
has `active` False and `compile_attempts` 0, and `process_units` and
`process_bytes` are 0. `sys._jit.is_available()` is False in every build.

**Correctness smoke on C:** `-m test test_grammar test_int test_float
test_list test_dict test_generators test_json test_pickle -j8` passed all 8
files (1,814 tests run, 65 skipped).

## Conditions

All timed runs (kernels and pyperformance, for C, B and U) ran in one quiet
window, from 10:11:12 to 10:22:32. It opened when the 1-minute load average
was 1.41, with no other build or test running. This task ran nothing else in
the window. Order:

- **Kernels** (10:11–10:13). For each kernel in registry order: U, then B,
  then C, each a separate `run.py --only KERNEL` invocation with `--max-load
  4`. The load that `run.py` sampled ranged from 1.34 to 1.80.
- **pyperformance** (10:13–10:22). For each benchmark in turn: C, then B,
  then U. The highest load that pyperf recorded was 1.63.

Builds, the smoke test, RSS, pystats, dispatch counts, the static figures and
the sampler ran outside the window, from 10:00 to 10:30, at load 1 to 21 (the
builds raised it). Before the window, each pyperformance benchmark was started
once under each interpreter with `-p 1 -n 1 -w 0 -l 1`, to check that it runs
in its venv. Those outputs were discarded.

Each kernel time vector holds five serial calls in one process, after one
warm-up call. These samples are correlated; they are not independent process
pairs.

## Kernels: time

Command, per kernel and interpreter: `run.py --set kernels --only KERNEL
--metric time --max-load 4`, with the registry's fixed loop counts and no
retries or calibration. Driver: `build/pyjit-link-wave1/remeasure2-time-kernels.py`,
which merges the per-kernel outputs into one file per interpreter and keeps
each invocation's run record. Raw data: `remeasure-20261104b-{C,B,U}-kernels.json`.
Each value is the median of five timed calls of `run(loops)`, in milliseconds
per loop. Spread is `100 * (max / min - 1)` over the five calls; the JSON
files keep the raw vectors.

| Kernel | C ms | C spread % | B ms | B spread % | U ms | U spread % | U/C | B/C |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| richards | 105.4 | 1.2 | 159.2 | 4.0 | 60.89 | 1.1 | 0.578 | 1.511 |
| go | 243.9 | 1.2 | 325.7 | 6.4 | 119.0 | 2.7 | 0.488 | 1.335 |
| nbody | 134.7 | 1.1 | 184.0 | 9.2 | 111.2 | 12.9 | 0.826 | 1.366 |
| float | 125.4 | 4.3 | 163.1 | 2.5 | 81.34 | 5.8 | 0.649 | 1.300 |
| spectral_norm | 133.2 | 1.2 | 181.7 | 1.9 | 114.1 | 3.0 | 0.856 | 1.363 |
| raytrace | 556.2 | 2.0 | 695.1 | 4.5 | 329.5 | 2.3 | 0.592 | 1.250 |
| deltablue | 9.437 | 11.6 | 12.98 | 5.2 | 3.804 | 1.1 | 0.403 | 1.376 |
| chaos | 119.7 | 5.4 | 155.4 | 0.8 | 70.96 | 1.0 | 0.593 | 1.298 |
| nqueens | 160.2 | 2.8 | 200.0 | 1.1 | 91.42 | 5.2 | 0.571 | 1.248 |
| fannkuch | 797.5 | 6.6 | 1,000 | 6.9 | 406.3 | 7.4 | 0.509 | 1.254 |
| generators | 71.81 | 6.2 | 77.73 | 8.6 | 30.46 | 3.4 | 0.424 | 1.083 |
| comprehensions | 0.03775 | 3.7 | 0.04957 | 5.5 | 0.01823 | 5.8 | 0.483 | 1.313 |
| coroutines | 45.13 | 0.4 | 52.98 | 3.0 | 25.99 | 2.6 | 0.576 | 1.174 |
| unpack_sequence | 0.03075 | 0.3 | 0.03447 | 3.5 | 0.01379 | 3.1 | 0.448 | 1.121 |
| deepcopy | 0.674 | 1.3 | 0.9591 | 6.3 | 0.3147 | 0.5 | 0.467 | 1.423 |
| deepcopy_reduce | 0.00755 | 1.5 | 0.01099 | 0.6 | 0.003907 | 0.8 | 0.517 | 1.456 |
| deepcopy_memo | 0.0825 | 0.9 | 0.143 | 6.2 | 0.02951 | 5.3 | 0.358 | 1.733 |
| json_dumps | 19.70 | 1.2 | 25.09 | 1.5 | 11.80 | 9.7 | 0.599 | 1.274 |
| json_loads | 0.8412 | 2.2 | 0.9982 | 4.4 | 0.6226 | 1.1 | 0.740 | 1.187 |
| pickle | 0.3385 | 0.5 | 0.3989 | 0.3 | 0.279 | 0.2 | 0.824 | 1.178 |
| unpickle | 0.4517 | 6.4 | 0.4845 | 5.4 | 0.3664 | 8.6 | 0.811 | 1.073 |
| regex_compile | 308.1 | 0.4 | 411.1 | 0.4 | 172.1 | 0.1 | 0.559 | 1.334 |
| logging_format | 0.1431 | 2.8 | 0.198 | 2.7 | 0.07546 | 0.4 | 0.527 | 1.383 |
| logging_silent | 0.002532 | 3.9 | 0.003482 | 0.5 | 0.001123 | 1.2 | 0.443 | 1.375 |
| logging_simple | 0.1279 | 1.3 | 0.1718 | 2.9 | 0.08084 | 29.0 | 0.632 | 1.343 |
| xml_etree_generate | 235.9 | 1.4 | 315.8 | 1.0 | 134.1 | 3.2 | 0.568 | 1.339 |
| xml_etree_iterparse | 145.6 | 2.9 | 194.8 | 2.0 | 110.2 | 15.6 | 0.757 | 1.338 |
| xml_etree_parse | 197.5 | 6.1 | 230.4 | 1.8 | 158.9 | 6.6 | 0.805 | 1.167 |
| xml_etree_process | 152.3 | 0.3 | 207.6 | 2.1 | 89.01 | 0.7 | 0.584 | 1.363 |
| **geometric mean** | | | | | | | **0.577** | **1.302** |

Five worst kernels against U (U/C): deepcopy_memo 0.358, deltablue 0.403,
generators 0.424, logging_silent 0.443, unpack_sequence 0.448. Three of these
were among the last re-measure's five worst. deepcopy (now 0.467) and
deepcopy_reduce (0.517) have left that group; generators and unpack_sequence
have joined it. Five best: spectral_norm 0.856, nbody 0.826,
pickle 0.824, unpickle 0.811, xml_etree_parse 0.805.

Against B, C gains most on deepcopy_memo (B/C 1.733), richards (1.511),
deepcopy_reduce (1.456), deepcopy (1.423) and logging_format (1.383). It
gains least on unpickle (1.073), generators (1.083), unpack_sequence (1.121),
xml_etree_parse (1.167) and coroutines (1.174).

C's largest spread is 11.6% (deltablue), B's 9.2% (nbody) and U's 29.0%
(logging_simple). U's logging_simple, xml_etree_iterparse (15.6%) and nbody
(12.9%) spread more than 10%, so those three U/C ratios are less certain. No
conclusion here rests on a single kernel.

**Reproducibility.** B is the binary that the last re-measure timed as C.
Its times now are 1.010 of those (geometric mean over the 29 kernels;
kernels range from 0.928, logging_simple, to 1.088, xml_etree_parse). U's are
1.017 of the last re-measure's (0.928 to 1.201; the 1.201 is logging_simple,
the 29% spread above). A single kernel's ratio within about ±9% is therefore
noise across sessions.

## Kernels: peak RSS

Command: `run.py --set kernels --metric rss`, and `run.py --set memory --only
startup,imports --metric rss`. Each value is the median of three processes,
in MiB of peak RSS (`/usr/bin/time -f %M`). RSS ran outside the window.

The method gives one set; a page-cache check gives a second. Table 1 is the
method as before, run at 10:05–10:06 in the order C, U, B. Raw data:
`remeasure-20261104b-{C,B,U}-kernels-rss.json` and
`remeasure-20261104b-{C,B,U}-startup-rss.json`. Geometric means: **U/C 0.773,
B/C 0.930.** Startup: C 12,624 KiB, B 11,612, U 9,472 (U/C 0.750, B/C 0.920);
imports: C 15,400, B 14,144, U 11,216 (0.728, 0.918).

That B/C does not hold up. B's binary gave 12.72 MiB on richards in the last
re-measure and 11.48 MiB now. A probe that reads `/proc/self/status` at the
end of `nbody` shows that the difference is file-backed: RssAnon is 4,308 KiB
in both B and C, and RssFile is 8,576 KiB in C and 7,664 KiB in B.
`/proc/self/smaps` puts the difference in the `python` executable's own
mappings (`r-xp` 4,116 against 3,644 KiB; `r--p` 2,060 against 1,812 KiB),
although C's text is 199 KB smaller than B's. `fincore` showed C's `python`
fully in the page cache (43.1 of 43.5 MB; the linker wrote it at 10:02) and
B's and U's only partly (6.5 of 44.6 MB and 5.7 of 36.7 MB). On ZFS, `read()`
fills the ARC, not the page cache, so a file that has only been run has only
its touched pages there. `fincore` agrees: `objdump` had read all of B's
binary at 10:04, and it was still only 15% resident. *Hypothesis:* the kernel's fault-around
maps the neighbours of a faulting page only when they are already in the page
cache. That would make a freshly linked binary's RSS about 1 MiB higher.

Table 2 repeats the measurement with every page of each build's `python` and
`build/lib.linux-x86_64-3.16/*.so` first brought into the page cache through
`mmap` (`fincore`: 100% for all three), run at 10:24–10:26 in the order C, U,
B. Raw data: `remeasure-20261104b-{C,B,U}-kernels-rss-cached.json` and
`remeasure-20261104b-{C,B,U}-startup-rss-cached.json`. After this step the
probe reads RssFile 8,756 (C), 8,856 (B) and 7,884 KiB (U). This is the
like-for-like comparison.

Table 2: all binaries fully in the page cache.

| Kernel | C MiB | C spread % | B MiB | B spread % | U MiB | U spread % | U/C | B/C |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| richards | 12.62 | 16.2 | 12.69 | 16.8 | 10.16 | 16.2 | 0.805 | 1.006 |
| go | 14.48 | 15.4 | 14.70 | 20.6 | 12.00 | 10.3 | 0.829 | 1.015 |
| nbody | 12.47 | 5.4 | 12.76 | 7.1 | 10.21 | 6.9 | 0.818 | 1.023 |
| float | 20.20 | 4.5 | 20.36 | 3.2 | 27.29 | 1.7 | 1.351 | 1.008 |
| spectral_norm | 12.43 | 5.9 | 12.52 | 2.7 | 10.01 | 4.2 | 0.805 | 1.007 |
| raytrace | 14.00 | 30.2 | 13.93 | 29.7 | 11.37 | 20.5 | 0.812 | 0.996 |
| deltablue | 13.43 | 11.3 | 13.48 | 7.6 | 11.01 | 11.0 | 0.820 | 1.003 |
| chaos | 14.59 | 8.4 | 14.66 | 7.1 | 11.92 | 1.6 | 0.817 | 1.005 |
| nqueens | 12.42 | 6.8 | 12.57 | 4.6 | 10.07 | 4.8 | 0.811 | 1.012 |
| fannkuch | 12.24 | 5.8 | 12.57 | 5.0 | 10.32 | 4.2 | 0.843 | 1.027 |
| generators | 22.35 | 2.8 | 22.63 | 1.3 | 23.10 | 1.1 | 1.034 | 1.013 |
| comprehensions | 15.38 | 47.8 | 15.77 | 46.7 | 12.57 | 40.0 | 0.817 | 1.026 |
| coroutines | 12.45 | 2.7 | 12.63 | 4.3 | 10.09 | 5.8 | 0.810 | 1.014 |
| unpack_sequence | 12.71 | 38.1 | 12.65 | 37.3 | 10.17 | 29.3 | 0.800 | 0.995 |
| deepcopy | 14.41 | 9.7 | 14.39 | 11.9 | 11.60 | 5.4 | 0.805 | 0.999 |
| deepcopy_reduce | 14.30 | 1.3 | 14.37 | 3.0 | 11.58 | 2.6 | 0.810 | 1.005 |
| deepcopy_memo | 14.31 | 0.7 | 14.45 | 0.7 | 11.57 | 1.1 | 0.808 | 1.009 |
| json_dumps | 15.62 | 24.0 | 15.68 | 22.9 | 12.02 | 13.8 | 0.770 | 1.004 |
| json_loads | 15.92 | 1.7 | 15.86 | 4.0 | 12.59 | 4.2 | 0.791 | 0.996 |
| pickle | 15.43 | 27.9 | 15.45 | 27.8 | 12.46 | 23.3 | 0.808 | 1.002 |
| unpickle | 15.38 | 1.1 | 15.39 | 1.7 | 12.26 | 2.2 | 0.798 | 1.001 |
| regex_compile | 18.11 | 46.5 | 18.25 | 46.5 | 13.78 | 43.0 | 0.761 | 1.008 |
| logging_format | 18.98 | 33.0 | 19.34 | 32.6 | 15.22 | 16.8 | 0.802 | 1.019 |
| logging_silent | 17.11 | 1.3 | 17.41 | 1.1 | 13.52 | 1.1 | 0.790 | 1.018 |
| logging_simple | 18.79 | 1.6 | 18.71 | 0.4 | 14.60 | 1.2 | 0.777 | 0.996 |
| xml_etree_generate | 22.55 | 15.9 | 22.99 | 17.0 | 18.73 | 10.3 | 0.830 | 1.019 |
| xml_etree_iterparse | 22.52 | 2.5 | 23.02 | 0.3 | 18.20 | 0.6 | 0.808 | 1.022 |
| xml_etree_parse | 21.81 | 0.7 | 22.27 | 1.2 | 17.60 | 0.9 | 0.807 | 1.021 |
| xml_etree_process | 22.72 | 2.2 | 22.99 | 2.5 | 19.22 | 1.5 | 0.846 | 1.012 |
| **geometric mean** | | | | | | | **0.829** | **1.010** |

| Row (cached) | C KiB | B KiB | U KiB | U/C | B/C |
|---|---:|---:|---:|---:|---:|
| startup | 12,384 | 12,948 | 10,260 | 0.828 | 1.046 |
| imports | 15,124 | 15,120 | 11,912 | 0.788 | 1.000 |

With equal page-cache state, C's peak RSS is that of B to within 2.7% on
every kernel, about 1% lower on average, and 21% above upstream's (U/C
0.829). The spreads above 20% come, as before, from the first of the three
processes, which compiles the `.pyc` into a fresh cache; the median discards
it. float and generators are the kernels where C uses less memory than U.
*Hypothesis:* the last re-measure's U/C of 0.782 is also biased low, because
its C was freshly linked and its U was not.

## pyperformance subset

The same eight benchmarks and settings as the last re-measure: pyperformance
1.14.0 (`/home/power/.cache/uv/archive-v0/xitntS2hDvEA0_ce/pyperformance/data-files/benchmarks`),
each `run_benchmark.py` run directly under pyperf 2.10.0 with `--processes 6`
and all other settings at pyperf's defaults (one warm-up and three values per
process, loops calibrated by pyperf). async_tree ran with `none`, and
unpickle_pure_python as `bm_pickle/run_benchmark.py --pure-python unpickle`.
Each interpreter had its own venv under `build/pyperf-venvs/remeasure2-{C,B,U}`,
holding a copy of the pure-Python pyperf package; the venvs were deleted
afterwards. Values are mean ± standard deviation in milliseconds over the 18
values. Spread is `100 * (max / min - 1)` over the same values. Raw data and
logs: `remeasure-20261104b-pyperf/{C,B,U}-<benchmark>.json`. Driver:
`build/pyjit-link-wave1/remeasure2-time-pyperf.sh`. pyperf flags all but two
results (C's hexiom and U's mdp) as possibly unstable: 18 values are too few
for its 1% criterion.

| Benchmark | C ms | B ms | U ms | U/C | B/C | spread % (C, B, U) | loops (C, B, U) |
|---|---:|---:|---:|---:|---:|---|---|
| hexiom | 12.48 ± 0.11 | 14.53 ± 0.34 | 6.633 ± 0.35 | 0.532 | 1.165 | 3.9, 11.0, 25.0 | 16, 8, 16 |
| pyflate | 655.8 ± 14 | 866.0 ± 19 | 416.3 ± 9.8 | 0.635 | 1.321 | 7.1, 6.6, 8.8 | 1, 1, 1 |
| scimark_fft | 625.9 ± 16 | 919.2 ± 23 | 390.0 ± 6.5 | 0.623 | 1.469 | 7.2, 8.2, 5.0 | 1, 1, 1 |
| scimark_lu | 236.9 ± 2 | 308.3 ± 9.1 | 153.1 ± 2.9 | 0.646 | 1.301 | 3.0, 11.4, 6.5 | 1, 1, 1 |
| scimark_monte_carlo | 175.1 ± 3.3 | 262.6 ± 13 | 75.68 ± 0.97 | 0.432 | 1.499 | 6.9, 22.2, 4.4 | 1, 1, 2 |
| scimark_sor | 275.2 ± 8.4 | 371.4 ± 7.3 | 136.7 ± 3 | 0.497 | 1.349 | 9.9, 7.0, 8.6 | 1, 1, 1 |
| scimark_sparse_mat_mult | 13.65 ± 0.15 | 20.37 ± 0.34 | 5.961 ± 0.23 | 0.437 | 1.493 | 4.1, 6.6, 10.8 | 8, 8, 32 |
| regex_v8 | 25.08 ± 0.58 | 28.78 ± 0.5 | 21.24 ± 1.4 | 0.847 | 1.147 | 7.3, 6.8, 27.9 | 4, 4, 8 |
| mdp | 2,740 ± 56 | 3,934 ± 70 | 1,357 ± 20 | 0.495 | 1.436 | 6.8, 6.6, 5.3 | 1, 1, 1 |
| async_tree_none | 587.6 ± 18 | 768.4 ± 23 | 357.2 ± 16 | 0.608 | 1.308 | 10.9, 11.1, 16.7 | 1, 1, 1 |
| unpickle_pure_python | 0.5819 ± 0.016 | 0.8263 ± 0.023 | 0.2625 ± 0.0049 | 0.451 | 1.420 | 7.5, 9.0, 6.9 | 16, 8, 32 |
| typing_runtime_protocols | 0.3558 ± 0.015 | 0.5187 ± 0.0067 | 0.1683 ± 0.0042 | 0.473 | 1.458 | 19.1, 4.9, 6.7 | 512, 256, 1024 |
| **geometric mean** | | | | **0.545** | **1.359** | | |

Every interpreter produced 18 values for every result. B takes 2.49 times as
long as U. B's means are 0.994 of the last re-measure's C (the same binary)
and U's 1.010 of the last re-measure's U (geometric means). C gains most on
scimark_monte_carlo (1.499), scimark_sparse_mat_mult (1.493) and scimark_fft
(1.469), and least on regex_v8 (1.147) and hexiom (1.165). Because C is
faster, pyperf calibrated twice B's loops for C's hexiom,
unpickle_pure_python and typing_runtime_protocols.

## Dispatch counts

Command: `pyjit/bench/ir/run.py`'s workers, as in the last re-measure: one
`run(1)` after a warm-up `run(1)`, counted with `sys.monitoring` INSTRUCTION
events (words for C, instructions for U, NOT_TAKEN excluded). The 12 variants
of the four multi-variant modules ran through shim modules whose `setup()`
passes the variant name. Driver: `build/pyjit-link-wave1/remeasure2-dispatch.py`.
Raw data: `remeasure-20261104b-dispatch.json` and
`remeasure-20261104b-dispatch-variants.json`.

**Deviation.** `ir/run.py --kernels` fails on C. Its worker
`dynamic_counts.py` reads `opcode._ir_formats` for the slot-traffic estimate,
and B0a replaced that list with `_opcode.get_ir_contract()`. The IR worker
therefore ran through `build/pyjit-link-wave1/remeasure2-dyn/dyn_shim.py`.
The shim rebuilds `_ir_formats` from the contract rows and then calls
`dynamic_counts.main` unchanged. It sets only the two fields that `traffic()`
reads, `flags` (`_PyIR_WINDOW` is 0x2 in both trees) and `base`. The word
total is counted before `traffic()` runs and does not depend on it.
`ir/run.py` itself was not edited.

The ns columns divide the quiet-window time per loop by the words in one
`run(1)`. This is a proxy: it assumes that each loop does the work of `run(1)`,
and it charges library time to the words. For C-heavy kernels (pickle,
unpickle, json_loads, xml_etree_parse) it is mostly library time. B executes
the same words as C, so its column uses C's words.

| Kernel | U instructions | C words | C/U | C ns per word | B ns per word | U ns per instruction | Per-dispatch cost C/U | C boxes per word |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| richards | 9,784,097 | 5,789,061 | 0.59 | 18.2 | 27.5 | 6.22 | 2.9 | 0.005 |
| go | 28,667,983 | 18,762,468 | 0.65 | 13.0 | 17.4 | 4.15 | 3.1 | 0.012 |
| nbody | 28,581,241 | 19,300,999 | 0.68 | 7.0 | 9.5 | 3.89 | 1.8 | 0.021 |
| float | 12,199,935 | 7,899,942 | 0.65 | 15.9 | 20.6 | 6.67 | 2.4 | 0.038 |
| spectral_norm | 19,045,221 | 17,011,597 | 0.89 | 7.8 | 10.7 | 5.99 | 1.3 | 0.040 |
| raytrace | 41,877,624 | 29,606,956 | 0.71 | 18.8 | 23.5 | 7.87 | 2.4 | 0.045 |
| deltablue | 726,098 | 545,927 | 0.75 | 17.3 | 23.8 | 5.24 | 3.3 | 0.004 |
| chaos | 10,532,435 | 7,321,871 | 0.70 | 16.4 | 21.2 | 6.74 | 2.4 | 0.066 |
| nqueens | 14,592,232 | 11,850,310 | 0.81 | 13.5 | 16.9 | 6.26 | 2.2 | 0.046 |
| fannkuch | 70,923,145 | 53,069,157 | 0.75 | 15.0 | 18.9 | 5.73 | 2.6 | 0.112 |
| generators | 6,806,866 | 6,106,863 | 0.90 | 11.8 | 12.7 | 4.47 | 2.6 | 0.016 |
| comprehensions | 3,223 | 2,417 | 0.75 | 15.6 | 20.5 | 5.66 | 2.8 | 0.074 |
| coroutines | 3,884,596 | 3,277,628 | 0.84 | 13.8 | 16.2 | 6.69 | 2.1 | 0.000 |
| unpack_sequence | 5,679 | 9,667 | 1.70 | 3.2 | 3.6 | 2.43 | 1.3 | 0.000 |
| deepcopy | 33,266 | 29,757 | 0.89 | 22.7 | 32.2 | 9.46 | 2.4 | 0.022 |
| deepcopy_reduce | 401 | 343 | 0.86 | 22.0 | 32.0 | 9.74 | 2.3 | 0.009 |
| deepcopy_memo | 5,672 | 4,920 | 0.87 | 16.8 | 29.1 | 5.20 | 3.2 | 0.042 |
| json_dumps | 460,179 | 316,144 | 0.69 | 62.3 | 79.4 | 25.65 | 2.4 | 0.054 |
| json_loads | 4,788 | 4,175 | 0.87 | 201.5 | 239.1 | 130.04 | 1.5 | 0.057 |
| pickle | 356 | 404 | 1.13 | 837.8 | 987.3 | 783.77 | 1.1 | 2.079 |
| unpickle | 373 | 355 | 0.95 | 1,272.4 | 1,364.7 | 982.32 | 1.3 | 0.000 |
| regex_compile | 22,225,651 | 18,001,919 | 0.81 | 17.1 | 22.8 | 7.75 | 2.2 | 0.064 |
| logging_format | 7,965 | 5,886 | 0.74 | 24.3 | 33.6 | 9.47 | 2.6 | 0.015 |
| logging_silent | 287 | 210 | 0.73 | 12.1 | 16.6 | 3.91 | 3.1 | 0.000 |
| logging_simple | 7,723 | 5,654 | 0.73 | 22.6 | 30.4 | 10.47 | 2.2 | 0.016 |
| xml_etree_generate | 8,673,036 | 6,773,770 | 0.78 | 34.8 | 46.6 | 15.46 | 2.3 | 0.022 |
| xml_etree_iterparse | 5,729,688 | 4,910,101 | 0.86 | 29.6 | 39.7 | 19.24 | 1.5 | 0.000 |
| xml_etree_parse | 1,543,374 | 1,173,352 | 0.76 | 168.3 | 196.4 | 102.97 | 1.6 | 0.000 |
| xml_etree_process | 7,577,080 | 6,074,727 | 0.80 | 25.1 | 34.2 | 11.75 | 2.1 | 0.000 |
| **geometric mean** | | | **0.805** | | | | | |

Every kernel's word count and upstream instruction count equals the last
re-measure's, to the word (geometric mean 0.805 at both commits). None of the
steps changed the instruction stream: B0a adds a contract table and changes no
format, and A1, A2 and A5a change handler bodies. The step 9 notes found the
same with pystats execution counts. On the eight object-heavy kernels
(richards to chaos), a C word costs 7.0–18.8 ns, against 3.9–7.9 ns for an
upstream instruction: 1.3–3.3 times as much. The last re-measure measured
1.8–4.6 times.

## Boxes

Command: `run.py --set kernels --metric boxes --pystats-records …` on
`remeasure-stats-5de7bd62ef9`, through the same wrapper as before
(`build/pyjit-link-wave1/checkpoint-run-boxes.py`; C's dump still lacks the
handler-entry coverage lines that `run.py` requires, and prints one
`.specializable` flag twice). Raw data: `remeasure-20261104b-C-kernels-boxes.json`
and the original dumps in
`build/pyjit-link-wave1/remeasure-20261104b-C-kernels-pystats.jsonl` (not
committed; the boxes JSON records its path and hash). The site classes come
from V1's `Boxing by site (CLASS)` counters, differenced between the `loops`
and `3 * loops` processes. Script: `build/pyjit-link-wave1/remeasure-boxes.py`.
The site classes add up to each kernel's box total on every kernel.

Boxes per loop, by site class:

| Kernel | Total | unattributed | call adapter | list | iteration | dict | tuple | attribute | parser |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| richards | 26,229 | 18 | 1 | 0 | 0 | 0 | 0 | 26,210 | 0 |
| go | 229,242 | 119,908 | 87,634 | 21,601 | 0 | 0 | 0 | 98 | 0 |
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
| deepcopy | 664 | 240 | 244 | 0 | 180 | 0 | 0 | 0 | 0 |
| deepcopy_reduce | 3 | 2 | 1 | 0 | 0 | 0 | 0 | 0 | 0 |
| deepcopy_memo | 207 | 107 | 100 | 0 | 0 | 0 | 0 | 0 | 0 |
| json_dumps | 17,002 | 0 | 4,001 | 0 | 0 | 9,000 | 0 | 0 | 4,001 |
| json_loads | 240 | 0 | 180 | 0 | 0 | 0 | 0 | 0 | 60 |
| pickle | 840 | 0 | 60 | 400 | 0 | 360 | 20 | 0 | 0 |
| unpickle | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| regex_compile | 1,155,247 | 823,145 | 228,258 | 97,790 | 0 | 6,054 | 0 | 0 | 0 |
| logging_format | 90 | 20 | 70 | 0 | 0 | 0 | 0 | 0 | 0 |
| logging_silent | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| logging_simple | 90 | 20 | 70 | 0 | 0 | 0 | 0 | 0 | 0 |
| xml_etree_generate | 152,061 | 51,540 | 521 | 0 | 0 | 0 | 100,000 | 0 | 0 |
| xml_etree_iterparse | 361 | 20 | 241 | 0 | 0 | 30 | 70 | 0 | 0 |
| xml_etree_parse | 31 | 0 | 31 | 0 | 0 | 0 | 0 | 0 | 0 |
| xml_etree_process | 108 | 108 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |

Over one pass (each kernel's boxes per loop times its registry loop count,
summed over the 29 kernels), against the last re-measure's C:

| Site class | Last C, per pass | C, per pass | Share now |
|---|---:|---:|---:|
| unattributed | 7,443,289 | 7,981,950 | 53.4% |
| call adapter | 4,675,748 | 4,676,566 | 31.3% |
| list | 972,436 | 973,425 | 6.5% |
| iteration | 730,260 | 730,260 | 4.9% |
| dict | 232,084 | 240,084 | 1.6% |
| tuple | 216,074 | 216,074 | 1.4% |
| attribute | 71,242 | 71,240 | 0.5% |
| parser | 52,010 | 52,010 | 0.3% |
| **total** | **14,393,143** | **14,941,610** | |

Kernels whose box total changed (per loop; script
`build/pyjit-link-wave1/remeasure2-report-tables.py`):

| Kernel | Last C | C | Change | Loops per pass | Change per pass |
|---|---:|---:|---:|---:|---:|
| go | 229,237 | 229,242 | +5 | 1 | +5 |
| deepcopy | 424 | 664 | +240 | 300 | +72,000 |
| deepcopy_reduce | 1 | 3 | +2 | 20,000 | +40,000 |
| deepcopy_memo | 100 | 207 | +107 | 4,000 | +428,000 |
| pickle | 820 | 840 | +20 | 400 | +8,000 |
| regex_compile | 1,154,647 | 1,155,247 | +600 | 1 | +600 |

The other 23 kernels are unchanged to within 0.5 boxes per loop. These are the
same six kernels and the same totals as the step 9 notes'
(`repr-step9/boxes-{base,step9}-kernels.json`), so step 9 accounts for every
change, and steps 7, 8, A1, A2 and A5a changed no box count.

**What step 9 did to boxes.**

- **Float boxes** are unchanged on every kernel. Inline-only numeric guards
  changed which path a handler takes, not where a value is boxed.
- **Int boxes** are unchanged where every int was already inline. Widening
  to int51 makes more values inline, and an inline value that reaches an
  object API boxes there. Where base had one shared `PyLong`, C now boxes at
  each use.
- **deepcopy and deepcopy_memo.** These are the rise the step 9 notes report.
  `opcode[CONTAINS_OP_DICT].boxes` goes from 0 to 150 per loop in deepcopy and
  from 0 to 105 in deepcopy_memo: `d in memo` with an inline `id()` key.
  Allocations rise with them (deepcopy 1,380 → 1,620, deepcopy_memo 126 →
  233 per loop).
- **The notes' second source is not found.** The notes name a `dict.get(id(x))`
  call. `Lib/copy.py` here uses `if d in memo: return memo[d]` (lines
  121–126, the same in B, C and U), and the call-adapter class is unchanged
  in both kernels (244 and 100).
- **The other 90 new boxes in deepcopy** are unattributed and have no
  per-opcode counter. That equals the 90 `BINARY_OP_SUBSCR_DICT` →
  `PUSH_EXC_INFO` pairs per loop: the `KeyError` paths of `memo[id(memo)]` in
  `_keep_alive` and `memo[id(x)]` in `_deepcopy_tuple`. *Hypothesis:* raising
  `KeyError(key)` boxes the inline key for the exception.
- **regex_compile** shifts boxes between classes: `opcode[COMPARE_OP].boxes`
  falls by 1,274, and call adapter (+885) and list (+989) rise. *Hypothesis:*
  `_sre.MAXREPEAT` (2^32 − 1) is now inline, as the step 9 notes suggest: it
  no longer boxes in a compare against a boxed constant, and it boxes when
  passed to C. pickle's 20 extra boxes are in the dict class; they were not
  traced.

The time does not follow the boxes: deepcopy_memo, with twice the boxes, is
the kernel that gains most against B (1.733). The step 9 notes measured
deepcopy and deepcopy_memo as neutral for step 9 alone (0.996 and 0.980).

## Static figures

Scripts: `build/pyjit-link-wave1/remeasure2-static/static.sh` (a copy of
`irv2-a5a/static.sh`: `irv2/labels.py` and `irv2/hmin.py`) and
`irv2/weighmin.py`. The lower bound is the shortest call-free path from each
handler's entry to its dispatch, weighted by handler entries over the 29
kernels. The weights are `remeasure-20261104-C-kernels-boxes.json`, the same
file the A1/A2 and A5a notes used; the words are identical, so the weights
are too. The 59% set is the handlers with a call-free path in B, C and
upstream (59% of entries); the 78% set leaves out upstream. Files:
`remeasure2-static/{size,labels,hmin}-{B,C}.*` and `weighmin-{59,78}.md`.

| Build | Loop (B) | `ceval.o` text (B) | `python` text (B) | Lower bound, 59% set: instructions / stack references per word | Lower bound, 78% set | Handlers without a call-free path |
|---|---:|---:|---:|---:|---:|---:|
| B `c8a0b4dc54c` | 407,525 | 594,877 | 6,957,549 | 49.0 / 12.1 | 53.1 / 12.3 | 152 |
| C `5de7bd62ef9` | 272,546 | 473,589 | 6,758,081 | 40.7 / 7.6 | 43.3 / 7.8 | 150 |
| U `3a440b6` | 58,419 (`_PyEval_EvalFrameDefault`) | 107,446 | 5,836,932 | 22.0 / 1.3 | – | – |

The loop is `_PyEval_EvalFrameLoop` for B and C. Against B, C's loop is 33%
smaller. The lower bound has 17% fewer instructions and 37% fewer stack
references per word. Upstream's path is still 1.85 times shorter, with 5.8
times fewer stack references.

For context, the step notes measured each step on its own base (not
remeasured here): A1+A2 took the loop from 406,199 to 303,064 B on
`54825c08a1a`; A5a from 308,756 to 279,995 B on `c8637fe7c6b`; step 9 from
308,756 to 300,880 B on `c8637fe7c6b`.

Top handlers by share of entries, instructions / stack references:

| Handler | Share | B | C | upstream |
|---|---:|---:|---:|---:|
| `LOAD_FAST_BORROW` | 13.8% | 44 / 12 | 35 / 7 | 14 / 1 |
| `STORE_FAST` | 4.7% | 45 / 10 | 24 / 5 | 18 / 1 |
| `LOAD_ATTR_SHAPE` | 4.5% | 95 / 18 | 85 / 15 | – |
| `POP_JUMP_IF_FALSE_BOOL` | 4.3% | 59 / 11 | 54 / 7 | – |
| `POP_TOP` | 3.8% | 29 / 10 | 21 / 5 | 14 / 1 |
| `LOAD_GLOBAL_MODULE` | 3.6% | 61 / 11 | 57 / 8 | 40 / 1 |
| `PUSH_NULL` | 3.0% | 25 / 9 | 22 / 5 | 11 / 1 |
| `LOAD_SMALL_INT_INLINE` | 2.9% | 31 / 10 | 28 / 6 | 17 / 1 |
| `JUMP_BACKWARD` | 2.8% | 27 / 12 | 19 / 5 | 13 / 2 |

`RETURN_VALUE` (4.2%) and `CALL_PY_EXACT_ARGS` (2.7%) have no call-free path.

## Attribution

Method: the last re-measure's, which is the triage's. The LD_PRELOAD sampler
`pyjit/tools/sampler/perf-triage-sampler.c` (built with `gcc -O2 -shared
-fPIC`) ran at `SAMPLER_US=500` and `SAMPLER_DEPTH=1`, with the triage's
symbol classifier copied verbatim. Its buckets:

- eval loop: `_PyEval_EvalFrameLoop` against `_PyEval_EvalFrameDefault`;
- exception: `PyErr_GetRaisedException`, `PyErr_SetRaisedException`,
  `_PyValue_RestoreException`, `PyErr_Occurred` and three restore helpers;
- bridge: `value_*`, `_PyValueTemps_*`, `_PyValue_*`, `PyValue_*`,
  `call_transaction_*`, `_PyNativeScope_*` and `_PyBorrow_*`.

Each kernel ran `round(1.2e9 / t)` loops, where t is the triage's quiet F time
for B and C and its U time for U. The share of the gap to U is
(X·f_X − U·f_U)/(X − U), where X is the build, f the category's fraction of
the build's samples, and the times are this report's quiet-window medians.
The share of the B−C saving is (B·f_B − C·f_C)/(B − C). Script:
`build/pyjit-link-wave1/remeasure2-sample.py` (the last re-measure's, with the
scratch path changed). Attribution: `remeasure2-attrib.py`. Raw data:
`remeasure-20261104b-attribution.json`. The kernels are the triage's five
worst, as the brief asks: deepcopy_memo, richards, deltablue, deepcopy and
logging_silent. The samplers ran at 10:04 (the two extra kernels below at
about 10:27), outside the window.

Means over the five kernels:

| Category | Last re-measure: share of C−U gap (C = `c8a0b4dc54c`) | Now: share of B−U gap (the same binary) | Now: share of C−U gap | Share of the B−C saving | B time, % of B | C time, % of B | U time, % of B |
|---|---:|---:|---:|---:|---:|---:|---:|
| Interpreter loop's own time | 32% | 32% | 44% | 17% | 37.9 | 32.4 | 15.8 |
| Exception save/restore | 14% | 14% | 0% | 30% | 9.6 | 0.0 | 0.0 |
| Value bridging and boxing | 28% | 26% | 25% | 28% | 17.9 | 9.2 | 0.0 |
| Attribute lookup | 6% | 6% | 5% | 6% | 5.9 | 4.0 | 2.0 |
| Frames | 5% | 3% | 7% | −2% | 4.9 | 5.5 | 3.0 |
| Other (object work, allocation, dicts, libc) | 16% | 20% | 19% | 21% | 23.8 | 16.8 | 9.8 |
| **Total** | | | | | **100** | **67.9** | **30.6** |

The specialiser bucket is below 0.2% everywhere and is left out. B's sample
fractions reproduce the last re-measure's for the same binary: loop 37.9%
against 37.9%, exception 9.6% against 9.9%, bridge 17.9% against 19.2%, frames
4.9% against 6.2% and other 23.8% against 21.1%.

| Kernel | Last C/U | B/U | C/U | C/B | Samples (C, B, U) | Loop, % of B's time: B → C | Exception: B → C | Bridge: B → C | Other: B → C | Loop's share of the gap: B → C |
|---|---:|---:|---:|---:|---|---|---|---|---|---|
| deepcopy_memo | 4.65 | 4.84 | 2.80 | 0.577 | 724, 1,204, 1,285 | 30.7 → 23.7 | 10.8 → 0 | 22.3 → 11.1 | 33.7 → 20.2 | 26% → 37% |
| richards | 2.66 | 2.61 | 1.73 | 0.662 | 647, 976, 1,198 | 44.4 → 34.2 | 13.0 → 0 | 12.8 → 8.8 | 11.8 → 7.8 | 36% → 42% |
| deltablue | 3.47 | 3.41 | 2.48 | 0.727 | 698, 969, 1,201 | 44.2 → 38.7 | 10.8 → 0 | 14.7 → 8.3 | 11.9 → 12.4 | 35% → 45% |
| deepcopy | 3.08 | 3.05 | 2.14 | 0.703 | 709, 1,031, 1,211 | 31.7 → 27.5 | 6.2 → 0 | 16.4 → 8.5 | 39.7 → 29.9 | 30% → 43% |
| logging_silent | 3.03 | 3.10 | 2.26 | 0.727 | 814, 1,130, 1,221 | 38.5 → 38.0 | 7.2 → 0 | 23.3 → 9.2 | 22.1 → 13.8 | 34% → 55% |

Top five self-cost functions in C (fraction of C's samples, bucket):

| Kernel | 1 | 2 | 3 | 4 | 5 |
|---|---|---|---|---|---|
| deepcopy_memo | `_PyEval_EvalFrameLoop` 41.0% (loop) | `_PyCFunction_CallScoped` 5.0% (other) | `_PyNativeScope_End` 3.5% (bridge) | `_PyNativeScope_Begin` 3.3% (bridge) | `_Py_dict_lookup` 3.0% (other) |
| richards | `_PyEval_EvalFrameLoop` 51.6% (loop) | `value_reader_complete` 3.2% (bridge) | `value_attribute_complete` 3.1% (bridge) | `_PyObject_GetMethodValue` 2.9% (attr) | `_PyObject_GenericGetAttrWithDict` 2.9% (attr) |
| deltablue | `_PyEval_EvalFrameLoop` 53.3% (loop) | `_PyTypeCache_Lookup` 3.4% (attr) | `_PyFrame_PushOverlapping` 3.3% (frames) | `_Py_type_getattro_value` 2.7% (attr) | `value_reader_complete` 2.6% (bridge) |
| deepcopy | `_PyEval_EvalFrameLoop` 39.1% (loop) | `_dl_mcount_wrapper` 3.4% (other) | `_Py_dict_lookup` 3.2% (other) | `mi_free_ex` 2.3% (other) | `_PyNativeScope_End` 2.0% (bridge) |
| logging_silent | `_PyEval_EvalFrameLoop` 52.3% (loop) | `call_transaction_invoke` 6.9% (bridge) | `_dl_mcount_wrapper` 5.8% (other) | `_PyFrame_ClearExceptCode` 5.5% (frames) | `eval_frame_push_and_init_code` 5.4% (frames) |

`_dl_mcount_wrapper` is the last exported symbol in libc's `.text`, at
0x196fd0. The symboliser charges the following 166 KB of internal code to it.
*Hypothesis:* these are glibc's multiarch string and memory routines, such as
`memmove` and `memset`.

**Two of the new worst kernels.** generators and unpack_sequence were sampled
the same way after the five. They are kept in the JSON under
`_extra_not_in_means` and are not in the means above. Their gap to U is
almost all loop:

- **generators:** the loop's own time is 75% of the C−U gap (71.5% of C's
  samples), and "other" is 19%.
- **unpack_sequence:** the loop is 97% of the gap (98.0% of C's samples).
  The kernel also executes 1.70 words per upstream instruction.

**Sampling error and limits.**

- Because the loop counts are fixed from the triage's times and C is faster,
  C's profiles have 647–814 samples. A fraction near 0.5 then has a standard
  error of about 1.9 percentage points, and one near 0.1 about 1.1. The
  five-kernel means are steadier.
- The share of a B−C saving divides by a difference of 27–42% of B's time.
- **Inlining moves cost between buckets.** Under A2's new signatures, GCC
  inlines `call_transaction_begin` into `_PyEval_EvalFrameLoop` in C; it has
  no symbol there. In B it took 1.8% of B's time, in the bridge bucket. If it
  costs the same in C, that cost is now in the loop bucket. Then the loop's
  own fall is about 7 points rather than 5.5, and the bridge's about 7
  rather than 8.7.
- `value_operation_close3`, `value_operation_refresh`,
  `call_transaction_invoke` and `value_reader_complete` exist in C only as
  `.constprop` or `.isra` clones. The classifier strips the suffix.

## What changed

**Exception save/restore.** Gone. It took 9.6% of B's time on the five
kernels; C has no sample in it (0 of 3,592). It accounts for 30% of the B−C
saving, and it was 14% of the gap to upstream. Step 7 targeted these sites:

- 7a takes API status from the operation, not from `PyErr_Occurred()`;
- 7b removes the remaining saves around releases. Among them is the loop
  release `value_operation_close3`, which the last re-measure found holding
  part of the rest of the bucket: it has four save or restore calls in B and
  none in C;
- 7c returns the reentry defences to upstream's form;
- 7d deletes `_PyValue_RestoreException`.

The source agrees. These are lines containing each call in `Objects/`,
`Python/` and `Modules/`:

| Call | B | C | U |
|---|---:|---:|---:|
| `_PyValue_RestoreException(` | 136 | 0 | 0 |
| `PyErr_GetRaisedException(` | 588 | 207 | 194 |
| `PyErr_Occurred()` | 3,608 | 2,462 | 2,426 |

C is now within 7% of upstream on both. No other step in the range touches
this bucket. This measurement compares only B and C, so it cannot split the
saving between 7a and 7d.

**The interpreter loop's own time.** It fell from 37.9% to 32.4% of B's time
(−15%). This is 17% of the saving. It is probably larger: the inlined
`call_transaction_begin` now counts here (see the limits above). Because the
other buckets fell further, the loop's share of the gap rose from 32% to 44%.
It is now the largest bucket on every one of the five kernels. The words are
identical, so the whole change is in cost per word. Three of the steps change
the loop's handlers, and the static figures move with the time (the loop is
33% smaller, the per-word lower bound has 17% fewer instructions and 37% fewer
stack references):

- **A1** emits the value record only in handlers that box. Its notes measured
  −5.6 instructions and −4.9 stack references per word, and a time ratio of
  0.983.
- **A2** passes the next instruction by value. Under computed goto it adds 1.4
  instructions per word on its own (time 1.024 alone, 0.986 with A1); its
  gain is meant for A3a's tail-call build.
- **A5a** keeps borrows out of frame slots, so `STORE_FAST`, `POP_TOP` and
  `BINARY_OP_ADD_INT` no longer test or drop a borrow tag (`STORE_FAST`
  45 → 24 instructions from B to C). Its notes measured the loop −9.3% and a
  time ratio of 0.94–0.95.
- **9a's inline-only numeric guards** drop the boxed arms' type loads. The
  step 9 notes measured a time ratio of 0.964 for 9a and 9b together, led by
  fannkuch, spectral_norm and nbody.

The step notes each measured on their own base, and a union cannot be split
by step (measurement.md). *Hypothesis:* A5a and A1 contribute most of the
loop's fall, as their static changes are the largest. What remains is the
gap that the static figures show: 40.7 instructions and 7.6 stack references
per word against upstream's 22.0 and 1.3. On kernels that are almost all
loop, C gains least against B: generators 1.083, unpack_sequence 1.121 and
coroutines 1.174. *Hypothesis:* their cost is the state kept on the stack
across handlers, which no step here removes.

**Value bridging and boxing.** This bucket fell by half, from 17.9% to 9.2%
of B's time. That is 28% of the saving. Its share of the gap is almost
unchanged (26% → 25%), so it fell about as fast as the gap did. By symbol,
over the five kernels, in % of B's time:

- `call_transaction_begin`: 1.8 → inlined (see above).
- `value_operation_refresh`: 2.2 → 0.7.
- `_PyNativeScope_End`: 1.9 → 0.7.
- `value_operation_close3`: 1.0 → 0.
- `_PyValue_IsSameValue`: 0.5 → 0. Step 8 deleted it: identity is a bit
  compare once construction is canonical.
- `call_transaction_invoke`: 2.2 → 1.5.

The steps behind this:

- step 8: canonical construction, `is` as a bit compare, and value identity in
  object-level hash, repr and equality;
- 7b: the saves leave the bridge helpers, which makes them smaller;
- A2: the helpers' new signatures change GCC's inlining;
- A5a: helpers no longer handle a tagged local;
- `5de7bd62ef9`: matches upstream's release order around calls.

*Hypothesis:* step 8 and the smaller helpers after 7b give most of the fall,
but the samples cannot separate them, and inlining moves an unknown part
into the loop bucket. What remains is the native-call and attribute boundary:
`call_transaction_invoke`, `value_reader_complete`, `value_operation_refresh`,
`_PyNativeScope_Begin`/`End`, `PyValue_FromObjectBorrow` and
`value_attribute_complete`. These seven hold 6.0 of the bucket's 9.2
points. Boxing itself did not change except for step 9's rise (see Boxes),
which these kernels do not show in time.

**Other, attribute lookup and frames.**

- **"Other"** fell from 23.8% to 16.8% of B's time (21% of the saving).
  - `PyLong_AsLongAndOverflow` (1.0 → 0, mostly deepcopy_memo and deepcopy).
    *Hypothesis:* before 9a, an `id()` beyond int32 was a heap `PyLong`, and
    each canonical construction (step 8) tested it with this call. Now it is
    inline.
  - `_Py_dict_lookup`, `PyDict_GetValue` and `_PyDict_GetValue_KnownHash`
    each fell by 0.3–0.5 points. *Hypothesis:* step 8's value identity in
    hash and equality.
  - `mi_free_ex` and `_PyObject_MiMalloc` rose by 0.6 together.
    *Hypothesis:* the extra boxes in deepcopy.
- **Attribute lookup** fell from 5.9% to 4.0%, mostly in richards and
  deltablue (`_PyTypeCache_Lookup` 1.5 → 0.8; `type_cache_admitted` 0.5 →
  below C's top 30 symbols).
  This was not traced to a step.
- **Frames** went from 4.9% to 5.5%, within sampling error.
  `eval_frame_push_and_init_code` rose from 0.5 to 1.1. *Hypothesis:* an
  inlining change.

**B0a and 9b.** B0a adds the instruction contract table and its consumers,
and changes no format. The words are identical, and no time effect is
expected or separable. Its one visible effect here is the dispatch tool's
shim. 9b carries int51 in the trace JIT, which is off in these builds. It
has no measurable effect here.

**Memory.** With equal page-cache state, peak RSS is unchanged against B
(B/C 1.010) and 21% above upstream's (U/C 0.829). C's text is 199 KB smaller
than B's. The method's own numbers (B/C 0.930) are a page-cache artifact; see
Kernels: peak RSS.

**What the next steps address.** This is a hypothesis from the buckets, not
a measurement.

- **A3a** (tail calls, one activation record, no address-taken locals)
  addresses the loop's own time. That is now the largest bucket: 44% of the
  C−U gap on the five kernels, and 75–97% on generators and unpack_sequence.
  It targets the stack references that the static figures show, 7.6 per word
  against upstream's 1.3. The packet's prototype measured 0.870 and 1.5
  stack references per word with A1 and A2.
- **A5b** (the typed borrow, the producer list, `MakeHeapSafe` out of the
  setters) addresses value bridging and part of "other". `MakeHeapSafe` is
  inline, so its tests are charged to its callers: dict, tuple and list
  setters ("other") and handlers ("loop"). Bridging is 25% of the gap. Its
  largest items are the native-call scope and transaction
  (`_PyNativeScope_*`, `call_transaction_invoke`) and the reader and
  attribute completions. A5b's packet entry does not name those. Expect
  A5b's direct gain to be a fraction of the bridging bucket, not all of it.

## Deviations, omissions and caveats

- **Dispatch tool shim.** `ir/run.py` fails after B0a; the IR worker ran
  through `remeasure2-dyn/dyn_shim.py` (see Dispatch counts). Word totals do
  not depend on it.
- **A second RSS set.** The method's RSS numbers depend on the page-cache
  state of each binary. A second set with every binary fully cached is
  reported as the like-for-like one. Both sets are kept.
- **Two extra sampled kernels** (generators, unpack_sequence), reported apart
  from the five-kernel means.
- **Fewer samples for C** (647–814 per profile), because the loop counts are
  the triage's and C is faster.
- **B's source worktree** was recreated at `c8a0b4dc54c` so that B's build
  could read `Lib/`; the build itself was reused.
- **No instruction counts or cycle profiles.** `perf_event_paranoid=4`.
- **Boxes** needed the same `run.py` wrapper as last time (missing
  handler-entry coverage lines in the dump, and one duplicate flag).

## Files

In `pyjit/bench/results/`:
- `remeasure-20261104b-{C,B,U}-kernels.json`: kernel times, one run record
  per kernel invocation.
- `remeasure-20261104b-{C,B,U}-kernels-rss.json`,
  `remeasure-20261104b-{C,B,U}-startup-rss.json`: peak RSS by the method.
- `remeasure-20261104b-{C,B,U}-kernels-rss-cached.json`,
  `remeasure-20261104b-{C,B,U}-startup-rss-cached.json`: peak RSS with every
  binary fully in the page cache.
- `remeasure-20261104b-pyperf/`: pyperf JSON files and logs.
- `remeasure-20261104b-dispatch.json`, `remeasure-20261104b-dispatch-variants.json`:
  dispatch counts.
- `remeasure-20261104b-C-kernels-boxes.json` (15.8 MB): boxes on the stats
  build. The original dumps (10.9 MB) are kept uncommitted as
  `build/pyjit-link-wave1/remeasure-20261104b-C-kernels-pystats.jsonl`.
- `remeasure-20261104b-attribution.json`: sample fractions, top symbols and
  the attribution per kernel and build, the last re-measure's C fractions for
  comparison, and the two extra kernels.

In `build/pyjit-link-wave1/`:
- `remeasure2-time-kernels.py`: interleaved kernel timing.
- `remeasure2-pyperf-one.sh`, `remeasure2-time-pyperf.sh`: pyperformance.
- `remeasure2-dispatch.py` and `remeasure2-dyn/dyn_shim.py`: dispatch counts.
- `remeasure2-sample.py`: the sampler driver.
- `remeasure2-attrib.py`: the attribution.
- `remeasure2-tables.py`, `remeasure2-report-tables.py`: the tables.
- `remeasure2-static/`: the static figures.
- From the last re-measure: `remeasure-jitstate.py`, `remeasure-boxes.py` and
  `checkpoint-run-boxes.py`.

The scripts name the scratch directory `/tmp/remeasure2`. It held the built
sampler, the kernel runner (a copy of `remeasure-kern2.py`), the raw samples
and the RSS probes, and was removed. The C builds
`build/remeasure-release-5de7bd62ef9` and `build/remeasure-stats-5de7bd62ef9`
are kept. They read `Lib/` from the source worktree, which was removed, so
recreate it before use: `git -C /home/power/code/cpython worktree add --detach
build/worktrees/remeasure-5de7bd62ef9 5de7bd62ef9`. B's build needs
`build/worktrees/remeasure-c8a0b4dc54c` in the same way.
