# Integer and float magnitude distribution in upstream CPython

Question: which inline integer width should the NaN-boxed value use (int32 today, int51, or CRuby-style 62-bit fixnums with flonums)?
Source: upstream CPython `3a440b65695`, `gcc -O2`, `--with-mimalloc --with-computed-gotos`, no pydebug, plus `intwidth.patch`. Run on 2026-11-03.

## Recommendation

**Choose int51.** Decision rule from the design review, G5 row: int51 if the 32-51 bucket is material and 52-62 negligible.

| aggregate (all workloads) | int results | small -5..256 | <=31 bit non-small | 32-51 | 52-62 | >62 |
|---|---|---|---|---|---|---|
| kernels (29, 23.5 M results) | 100% | 75.10% | 20.08% | **4.04%** | 0.43% | 0.35% |
| pyperformance subset (30 benchmarks, 49.0 M) | 100% | 70.78% | 25.09% | **3.30%** | 0.40% | 0.43% |
| test-suite slice (11 modules, 34.1 M) | 100% | 56.55% | 25.14% | 2.00% | 4.87% | 11.43% |
| all (106.6 M) | 100% | 67.17% | 24.01% | 3.05% | 1.84% | 3.94% |

Floats: 98.8 M results; 96.03% inside |exp| <= 255, 3.45% zero, 0.36% nan/inf, **0.157% outside the window** (kernels and pyperformance: under 0.001%; only the test slice has any: 0.79%, from deliberate edge-case tests in test_math, test_float and test_statistics).

Reasoning:

1. The 32-51 bucket is material and is almost entirely one thing: the 47-bit values (16.1% of non-small ints in the kernels, 9.8% in pyperformance) are `id()` results, which are user-space addresses on x86-64 Linux (bit length 47 exactly). The rest of the bucket is unsigned 32-bit words (crypto_pyaes T-table words, bit length 32, 8.5% of that benchmark's ints), plus checksum-style values. Per workload it ranges from 0 to 99% (deepcopy_memo 99.4%, deepcopy 91.5%, deepcopy_reduce 88.2%, json_dumps 61.8%, pprint 34.8%, crypto_pyaes 8.5%). All of it fits in 51 signed bits (|x| <= 2^50) with room to spare: the bit-length histogram shows only 19 of 23.5 M kernel results, and 111 of 49 M pyperformance results, at exactly 51 bits, so the sign-bit question (51-bit signed holds bit length <= 50) does not change the answer.
2. The 52-62 bucket is negligible for ordinary programs: 0.43% (kernels), 0.40% (pyperformance). Known sources, all narrow: `time.time_ns()` and `os.stat` `*_ns` fields (61 bits; logging `LogRecord.__init__` is 4.5-5.5% of logging_format/simple ints, and about 1800 per process at import time through `_path_stat`), Zobrist random 62-bit hashes in `go` (8.5% of that kernel), `_pydecimal` limits such as `MAX_PREC`/`MAX_EMAX` = 10^18-1 (12.9% in decimal_factorial), float `as_integer_ratio` mantissas (53 bits; statistics, test_math), `random.getrandbits(53+)`. Only the test slice, which deliberately probes edge cases, reaches 4.9%.
3. CRuby's scheme (evaluate only if 52-62 is material and out-of-window floats are rare) is not triggered: 52-62 is not material in realistic workloads. Even then the float side would be fine (out-of-window floats are rare), so the only thing int62 would buy is the sources listed in point 2, which stay correct as boxed PyLongs under int51.
4. Cost of int32 vs int51: int32 would box roughly 3-4% of all int results (and nearly all of `id()`-keyed memo/marker traffic in copy, json, pprint). int51 removes those at no change to the float path (all non-NaN floats stay inline).

## Method

Patch `intwidth.patch` (2 files, `Objects/longobject.c`, `Objects/floatobject.c`) adds static counters, dumped by an `atexit` handler as one JSON line per process (append, `O_APPEND`) to the file named by `INTWIDTH_OUT`. Each line has: `int` buckets, `float` buckets, `hist` (exact bit length of |x| for every non-small int, 0..65, 65 = 65 or more), and `sites` (when `INTWIDTH_SITES` is set: counts by (bucket, top Python frame file:line:function) for the 32-51 and 52-62 buckets).

Counted int results (creation events, one count each):
- `get_small_int()`: every return of a cached small int (-5..256), whichever operation produced it (counted separately as the small subset).
- `_PyLong_FromMedium` and `medium_from_stwodigits`: single-digit results (bit length <= 30), which covers the BINARY_OP int specialisations (`_PyLong_Add/Subtract/Multiply` compact paths), `PyLong_From*` and `range` iteration.
- `_PyLong_FromLarge`, and the large branches of `PyLong_FromLong/LongLong/Ssize_t` and `PyLong_FromUnsignedLong/LongLong/Size_t`, with exact bit length; `PyLong_FromDouble` large branch (always > 62 bits).
- `long_normalize()`: every multi-digit or non-small digit-array result (add/sub/mul/divmod/shift/bitwise/pow/`int()` from string/`int.from_bytes`/`getrandbits`), counted by bit length from the digits. `maybe_small_long` hits are counted by `get_small_int`.
Floats: every `PyFloat_FromDouble` (all float arithmetic specialisations and constructors go through it in this tree), bucketed by unbiased binary exponent (frexp exponent minus 1): in window |e| <= 255, outside (above 255 or below -255, subnormals included), zero, NaN, inf.

Workloads:
- Kernels: all 29 in `pyjit/bench/internal/kernels`, run through the repo's `driver.py` in `run` mode with the fixed loop counts and variants from `run.py` (one `run(loops)` call, `-S`, `PYTHONHASHSEED=0`). `run.py` itself was not used, since it launches many processes per kernel; the time metric is not needed here. Baseline: the same driver with an empty module; subtracted per process.
- pyperformance 1.14.0 installed with `uv` into a venv on this build, but its own benchmark-venv creation needs `pip`/network and failed (`wheel`). Instead each `bm_*/run_benchmark.py` was run directly with the uv venv (`--worker -w 0 -n 1 -l 1`; pickle and sqlglot_v2 per sub-benchmark). 30 benchmarks: nbody spectral_norm float chaos richards deltablue raytrace json_dumps json_loads pickle/unpickle go hexiom unpack_sequence crypto_pyaes mdp pidigits sqlglot_v2 (parse, transpile, optimize, normalize) scimark telco decimal_factorial meteor_contest pyflate nqueens fannkuch barnes_hut base64 pprint regex_effbot regex_v8. `tomli_loads` was dropped (module not installed). The baseline is `venv/bin/python -c pass`, subtracted per process.
- Test slice: `-m test -j4 test_int test_long test_float test_math test_statistics test_random test_hashlib test_json test_struct test_time test_datetime`, 89 processes, all passed. The `-c pass` baseline is subtracted per process.

Known blind spots and biases:
- Counts are creation events, not live values or reads. A long-lived 62-bit int read a million times counts once; hot loops over an existing big value are under-weighted, and `id()`-style values used as dict keys count once per `id()` call, not per lookup.
- Operations that return an existing int object (`+ 0`, `int(x)` of an int, `abs` of a non-negative, `max`) are not counted. Ints created as code constants from `.pyc` marshal data are not counted by arithmetic, but the marshal reader's `PyLong_From*` calls are (startup noise, mostly baseline-subtracted).
- Startup/import noise: every process creates about 770 47-bit and 85 61-bit ints at startup (module/stat/id work). Baselines are subtracted, but pyperf worker processes import more (`os.stat` `*_ns` values), leaving a residual of about 1800 52-62-bit and 2100 32-51-bit ints per process. On short benchmarks this inflates 52-62 (it is 3-5% of tiny benchmarks such as unpack_sequence, base64, deltablue, which really have none). The kernel runs and the large benchmarks are not affected.
- Intermediate results of big-int algorithms are counted at `long_normalize`; this only touches the > 62 bucket.
- `pyperformance` was run with one value per worker (`-l 1`), so workload sizes are small and differ from the in-tree kernel loop counts; the in-tree kernels and pyperformance rows of the same benchmark are therefore not comparable in absolute counts, only in shares.
- Float "in window" assumes CRuby's about 2^+-255 rule; counts are of float results created, so loads of constants are not seen. Test-slice float results are dominated by edge-case tests.
- The site table records the top Python frame at creation; C callers (`time.time_ns`, `id`, `hash`) appear under their Python caller.
- Sites are only recorded per bucket (32-51 and 52-62), so 47-bit `id()` values (a 32-51 source) were identified by site (copy.py:121 `id(x)`, json/encoder.py:262, pprint.py `id(object)`), not by an `id()` hook.

Files: `intwidth.patch`, `intwidth-raw/*.jsonl` (raw JSON lines; per workload; `kernel-baseline_empty` and `pp-baseline_empty` are the baselines), `intwidth-runk.py`, `intwidth-runpp.sh`, `intwidth-agg.py` (driver scripts, kept for reproducibility).

## Results per workload

### kernels

Ints (bucket = bit length of |x|; small ints counted separately; small + <=31 + 32-51 + 52-62 + >62 = total)

| workload | int results | small -5..256 (%) | <=31 bit non-small (%) | 32-51 (%) | 52-62 (%) | >62 (%) |
|---|---|---|---|---|---|---|
| chaos | 3028093 | 3019574 (99.72) | 8236 (0.27) | 186 (0.01) | 91 (0.00) | 6 (0.00) |
| comprehensions | 122715 | 116402 (94.86) | 5467 (4.46) | 709 (0.58) | 132 (0.11) | 5 (0.00) |
| coroutines | 1456712 | 1456358 (99.98) | 354 (0.02) | 0 (0.00) | 0 (0.00) | 0 (0.00) |
| deepcopy | 215106 | 16056 (7.46) | 406 (0.19) | 198519 (92.29) | 120 (0.06) | 5 (0.00) |
| deepcopy_memo | 466106 | 2079 (0.45) | 3383 (0.73) | 460519 (98.80) | 120 (0.03) | 5 (0.00) |
| deepcopy_reduce | 162108 | 2079 (1.28) | 19383 (11.96) | 140521 (86.68) | 120 (0.07) | 5 (0.00) |
| deltablue | 228250 | 155165 (67.98) | 73059 (32.01) | 26 (0.01) | 0 (0.00) | 0 (0.00) |
| fannkuch | 4868005 | 4867999 (100.00) | 6 (0.00) | 0 (0.00) | 0 (0.00) | 0 (0.00) |
| float | 100188 | 1103 (1.10) | 99049 (98.86) | 6 (0.01) | 30 (0.03) | 0 (0.00) |
| generators | 1300909 | 805206 (61.90) | 495455 (38.09) | 183 (0.01) | 60 (0.00) | 5 (0.00) |
| go | 891980 | 637738 (71.50) | 100694 (11.29) | 214 (0.02) | 75970 (8.52) | 77364 (8.67) |
| json_dumps | 135157 | 34084 (25.22) | 10470 (7.75) | 90377 (66.87) | 210 (0.16) | 16 (0.01) |
| json_loads | 210138 | 96454 (45.90) | 108961 (51.85) | 4423 (2.10) | 283 (0.13) | 17 (0.01) |
| logging_format | 203562 | 139470 (68.51) | 32640 (16.03) | 20927 (10.28) | 10475 (5.15) | 50 (0.02) |
| logging_silent | 72230 | 9448 (13.08) | 61591 (85.27) | 672 (0.93) | 469 (0.65) | 50 (0.07) |
| logging_simple | 193231 | 129424 (66.98) | 32616 (16.88) | 20672 (10.70) | 10469 (5.42) | 50 (0.03) |
| nbody | 20016 | 1033 (5.16) | 18983 (94.84) | 0 (0.00) | 0 (0.00) | 0 (0.00) |
| nqueens | 1269397 | 1268971 (99.97) | 426 (0.03) | 0 (0.00) | 0 (0.00) | 0 (0.00) |
| pickle | 20396 | 19038 (93.34) | 753 (3.69) | 352 (1.73) | 247 (1.21) | 6 (0.03) |
| raytrace | 380788 | 235090 (61.74) | 145398 (38.18) | 205 (0.05) | 90 (0.02) | 5 (0.00) |
| regex_compile | 1573037 | 901219 (57.29) | 666520 (42.37) | 652 (0.04) | 372 (0.02) | 4274 (0.27) |
| richards | 228183 | 119903 (52.55) | 108252 (47.44) | 28 (0.01) | 0 (0.00) | 0 (0.00) |
| spectral_norm | 5413255 | 2853729 (52.72) | 2559526 (47.28) | 0 (0.00) | 0 (0.00) | 0 (0.00) |
| unpack_sequence | 8025 | 2066 (25.74) | 5959 (74.26) | 0 (0.00) | 0 (0.00) | 0 (0.00) |
| unpickle | 244298 | 92940 (38.04) | 144753 (59.25) | 6352 (2.60) | 247 (0.10) | 6 (0.00) |
| xml_etree_generate | 309250 | 306445 (99.09) | 1685 (0.54) | 633 (0.20) | 469 (0.15) | 18 (0.01) |
| xml_etree_iterparse | 52984 | 50306 (94.95) | 1518 (2.87) | 673 (1.27) | 469 (0.89) | 18 (0.03) |
| xml_etree_parse | 94162 | 91289 (96.95) | 1753 (1.86) | 633 (0.67) | 469 (0.50) | 18 (0.02) |
| xml_etree_process | 185417 | 182686 (98.53) | 1611 (0.87) | 633 (0.34) | 469 (0.25) | 18 (0.01) |
| **sum** | 23453698 | 17613354 (75.10) | 4708907 (20.08) | 948115 (4.04) | 101381 (0.43) | 81941 (0.35) |

Floats

| workload | float results | in window |e|<=255 (%) | outside (%) | zero (%) | nan/inf (%) |
|---|---|---|---|---|
| chaos | 2346762 | 1875032 (79.90) | 0 (0.00) | 471728 (20.10) | 2 (0.00) |
| comprehensions | 132 | 132 (100.00) | 0 (0.00) | 0 (0.00) | 0 (0.00) |
| coroutines | 1 | 0 (0.00) | 0 (0.00) | 0 (0.00) | 0 (0.00) |
| deepcopy | 120 | 120 (100.00) | 0 (0.00) | 0 (0.00) | 0 (0.00) |
| deepcopy_memo | 120 | 120 (100.00) | 0 (0.00) | 0 (0.00) | 0 (0.00) |
| deepcopy_reduce | 120 | 120 (100.00) | 0 (0.00) | 0 (0.00) | 0 (0.00) |
| deltablue | 990 | 990 (100.00) | 0 (0.00) | 0 (0.00) | 0 (0.00) |
| fannkuch | 1 | 0 (0.00) | 0 (0.00) | 0 (0.00) | 0 (0.00) |
| float | 1600035 | 1600024 (100.00) | 0 (0.00) | 9 (0.00) | 2 (0.00) |
| generators | 60 | 60 (100.00) | 0 (0.00) | 0 (0.00) | 0 (0.00) |
| go | 179844 | 172066 (95.68) | 0 (0.00) | 7776 (4.32) | 2 (0.00) |
| json_dumps | 214 | 210 (98.13) | 0 (0.00) | 0 (0.00) | 4 (1.87) |
| json_loads | 522 | 515 (98.66) | 0 (0.00) | 1 (0.19) | 6 (1.15) |
| logging_format | 30479 | 30478 (100.00) | 0 (0.00) | 1 (0.00) | 0 (0.00) |
| logging_silent | 473 | 472 (99.79) | 0 (0.00) | 1 (0.21) | 0 (0.00) |
| logging_simple | 30473 | 30472 (100.00) | 0 (0.00) | 1 (0.00) | 0 (0.00) |
| nbody | 5400407 | 5400400 (100.00) | 0 (0.00) | 7 (0.00) | 0 (0.00) |
| nqueens | 1 | 0 (0.00) | 0 (0.00) | 0 (0.00) | 0 (0.00) |
| pickle | 488 | 485 (99.39) | 0 (0.00) | 1 (0.20) | 2 (0.41) |
| raytrace | 4948321 | 4770986 (96.42) | 0 (0.00) | 177333 (3.58) | 2 (0.00) |
| regex_compile | 372 | 372 (100.00) | 0 (0.00) | 0 (0.00) | 0 (0.00) |
| richards | 1 | 0 (0.00) | 0 (0.00) | 0 (0.00) | 0 (0.00) |
| spectral_norm | 2028521 | 2028521 (100.00) | 0 (0.00) | 0 (0.00) | 0 (0.00) |
| unpack_sequence | 1 | 0 (0.00) | 0 (0.00) | 0 (0.00) | 0 (0.00) |
| unpickle | 488 | 485 (99.39) | 0 (0.00) | 1 (0.20) | 2 (0.41) |
| xml_etree_generate | 546 | 543 (99.45) | 0 (0.00) | 1 (0.18) | 2 (0.37) |
| xml_etree_iterparse | 546 | 543 (99.45) | 0 (0.00) | 1 (0.18) | 2 (0.37) |
| xml_etree_parse | 546 | 543 (99.45) | 0 (0.00) | 1 (0.18) | 2 (0.37) |
| xml_etree_process | 546 | 543 (99.45) | 0 (0.00) | 1 (0.18) | 2 (0.37) |
| **sum** | 16571125 | 15914232 (96.04) | 0 (0.00) | 656863 (3.96) | 30 (0.00) |

Mean of per-workload int shares (equal weight per workload, workloads with >1000 int results): small=58.69%, le31=27.21%, b32_51=12.97%, b52_62=0.81%, gt62=0.32%; max 32-51 = 98.80%, max 52-62 = 8.52%
Float outside-window share: mean 0.000%, max 0.000%

Top Python call sites for the 52-62-bit bucket (kernels), top frame at creation (C callers show their Python caller):

- 34275: kernel-go /home/power/code/cpython/.worktrees/python-jit/pyjit/bench/internal/kernels/go.py:153 in update
- 33899: kernel-go /home/power/code/cpython/.worktrees/python-jit/pyjit/bench/internal/kernels/go.py:154 in update
- 10000: kernel-logging_format .worktrees/python-boil-the-ocean/build/worktrees/upstream-intwidth/Lib/logging/__init__.py:300 in __init__
- 10000: kernel-logging_simple .worktrees/python-boil-the-ocean/build/worktrees/upstream-intwidth/Lib/logging/__init__.py:300 in __init__
- 7473: kernel-go /home/power/code/cpython/.worktrees/python-jit/pyjit/bench/internal/kernels/go.py:148 in __init__
- 474: kernel-logging_format <frozen importlib._bootstrap_external>:155 in _path_stat
- 468: kernel-logging_silent <frozen importlib._bootstrap_external>:155 in _path_stat
- 468: kernel-logging_simple <frozen importlib._bootstrap_external>:155 in _path_stat
- 468: kernel-xml_etree_generate <frozen importlib._bootstrap_external>:155 in _path_stat
- 468: kernel-xml_etree_iterparse <frozen importlib._bootstrap_external>:155 in _path_stat
- 468: kernel-xml_etree_parse <frozen importlib._bootstrap_external>:155 in _path_stat
- 468: kernel-xml_etree_process <frozen importlib._bootstrap_external>:155 in _path_stat

Top Python call sites for the 32-51-bit bucket (kernels), top frame at creation (C callers show their Python caller):

- 424000: kernel-deepcopy_memo ode/cpython/.worktrees/python-boil-the-ocean/build/worktrees/upstream-intwidth/Lib/copy.py:121 in deepcopy
- 90010: kernel-json_dumps hon/.worktrees/python-boil-the-ocean/build/worktrees/upstream-intwidth/Lib/json/encoder.py:262 in iterencode
- 63000: kernel-deepcopy ode/cpython/.worktrees/python-boil-the-ocean/build/worktrees/upstream-intwidth/Lib/copy.py:121 in deepcopy
- 54000: kernel-deepcopy ode/cpython/.worktrees/python-boil-the-ocean/build/worktrees/upstream-intwidth/Lib/copy.py:237 in _keep_alive
- 40000: kernel-deepcopy_reduce ode/cpython/.worktrees/python-boil-the-ocean/build/worktrees/upstream-intwidth/Lib/copy.py:121 in deepcopy
- 40000: kernel-deepcopy_reduce ode/cpython/.worktrees/python-boil-the-ocean/build/worktrees/upstream-intwidth/Lib/copy.py:237 in _keep_alive
- 27000: kernel-deepcopy ode/cpython/.worktrees/python-boil-the-ocean/build/worktrees/upstream-intwidth/Lib/copy.py:199 in _deepcopy_dict
- 20000: kernel-deepcopy_reduce ode/cpython/.worktrees/python-boil-the-ocean/build/worktrees/upstream-intwidth/Lib/copy.py:250 in _reconstruct
- 20000: kernel-deepcopy_reduce ode/cpython/.worktrees/python-boil-the-ocean/build/worktrees/upstream-intwidth/Lib/copy.py:199 in _deepcopy_dict
- 20000: kernel-deepcopy_reduce ode/cpython/.worktrees/python-boil-the-ocean/build/worktrees/upstream-intwidth/Lib/copy.py:240 in _keep_alive
- 18000: kernel-deepcopy ode/cpython/.worktrees/python-boil-the-ocean/build/worktrees/upstream-intwidth/Lib/copy.py:173 in _deepcopy_list
- 18000: kernel-deepcopy ode/cpython/.worktrees/python-boil-the-ocean/build/worktrees/upstream-intwidth/Lib/copy.py:240 in _keep_alive

### pyperformance

Ints (bucket = bit length of |x|; small ints counted separately; small + <=31 + 32-51 + 52-62 + >62 = total)

| workload | int results | small -5..256 (%) | <=31 bit non-small (%) | 32-51 (%) | 52-62 (%) | >62 (%) |
|---|---|---|---|---|---|---|
| barnes_hut | 2317634 | 2307331 (99.56) | 6105 (0.26) | 2154 (0.09) | 1816 (0.08) | 228 (0.01) |
| base64 | 48936 | 36467 (74.52) | 8211 (16.78) | 2148 (4.39) | 1876 (3.83) | 234 (0.48) |
| chaos | 1736729 | 1722364 (99.17) | 10141 (0.58) | 2180 (0.13) | 1816 (0.10) | 228 (0.01) |
| crypto_pyaes | 2490917 | 1661683 (66.71) | 615201 (24.70) | 211926 (8.51) | 1879 (0.08) | 228 (0.01) |
| decimal_factorial | 425567 | 319663 (75.11) | 46148 (10.84) | 3536 (0.83) | 54808 (12.88) | 1412 (0.33) |
| deltablue | 49749 | 36389 (73.15) | 8936 (17.96) | 2380 (4.78) | 1816 (3.65) | 228 (0.46) |
| fannkuch | 4906651 | 4896654 (99.80) | 5879 (0.12) | 2074 (0.04) | 1816 (0.04) | 228 (0.00) |
| float | 138857 | 29817 (21.47) | 104898 (75.54) | 2098 (1.51) | 1816 (1.31) | 228 (0.16) |
| go | 931972 | 667709 (71.64) | 106712 (11.45) | 2270 (0.24) | 77695 (8.34) | 77586 (8.32) |
| hexiom | 125521 | 114837 (91.49) | 6379 (5.08) | 2261 (1.80) | 1816 (1.45) | 228 (0.18) |
| json_dumps | 51758 | 31752 (61.35) | 6885 (13.30) | 11077 (21.40) | 1816 (3.51) | 228 (0.44) |
| json_loads | 39899 | 29276 (73.38) | 6466 (16.21) | 2101 (5.27) | 1823 (4.57) | 233 (0.58) |
| mdp | 5429269 | 3987808 (73.45) | 1437216 (26.47) | 2201 (0.04) | 1816 (0.03) | 228 (0.00) |
| meteor_contest | 247270 | 237014 (95.85) | 6083 (2.46) | 2129 (0.86) | 1816 (0.73) | 228 (0.09) |
| nbody | 59235 | 30142 (50.89) | 24955 (42.13) | 2094 (3.54) | 1816 (3.07) | 228 (0.38) |
| nqueens | 1308228 | 1297745 (99.20) | 6339 (0.48) | 2100 (0.16) | 1816 (0.14) | 228 (0.02) |
| pickle (2 procs) | 83883 | 62619 (74.65) | 12770 (15.22) | 4306 (5.13) | 3718 (4.43) | 470 (0.56) |
| pidigits | 272282 | 110414 (40.55) | 38466 (14.13) | 2165 (0.80) | 1848 (0.68) | 119389 (43.85) |
| pprint | 3739346 | 2429034 (64.96) | 6131 (0.16) | 1302107 (34.82) | 1846 (0.05) | 228 (0.01) |
| pyflate | 7612657 | 4427808 (58.16) | 3180333 (41.78) | 2364 (0.03) | 1924 (0.03) | 228 (0.00) |
| raytrace | 421657 | 265684 (63.01) | 151567 (35.95) | 2350 (0.56) | 1828 (0.43) | 228 (0.05) |
| regex_effbot | 42320 | 32214 (76.12) | 5937 (14.03) | 2094 (4.95) | 1816 (4.29) | 259 (0.61) |
| regex_v8 | 436549 | 91805 (21.03) | 340162 (77.92) | 2209 (0.51) | 1816 (0.42) | 557 (0.13) |
| richards | 154974 | 90257 (58.24) | 60397 (38.97) | 2276 (1.47) | 1816 (1.17) | 228 (0.15) |
| scimark | 9134947 | 6017053 (65.87) | 3113546 (34.08) | 2266 (0.02) | 1852 (0.02) | 230 (0.00) |
| spectral_norm | 5452109 | 2882517 (52.87) | 2565462 (47.05) | 2086 (0.04) | 1816 (0.03) | 228 (0.00) |
| sqlglot_v2 (4 procs) | 300983 | 203576 (67.64) | 43598 (14.49) | 32444 (10.78) | 15195 (5.05) | 6170 (2.05) |
| telco | 994004 | 625114 (62.89) | 364768 (36.70) | 2078 (0.21) | 1816 (0.18) | 228 (0.02) |
| unpack_sequence | 43368 | 33304 (76.79) | 5924 (13.66) | 2082 (4.80) | 1823 (4.20) | 235 (0.54) |
| **sum** | 48997271 | 34678050 (70.78) | 12295615 (25.09) | 1615556 (3.30) | 197171 (0.40) | 210879 (0.43) |

Floats

| workload | float results | in window |e|<=255 (%) | outside (%) | zero (%) | nan/inf (%) |
|---|---|---|---|---|
| barnes_hut | 29136396 | 29038144 (99.66) | 2 (0.00) | 98244 (0.34) | 6 (0.00) |
| base64 | 2856 | 2705 (94.71) | 2 (0.07) | 143 (5.01) | 6 (0.21) |
| chaos | 1308927 | 1042934 (79.68) | 2 (0.00) | 265985 (20.32) | 6 (0.00) |
| crypto_pyaes | 2345 | 2194 (93.56) | 2 (0.09) | 143 (6.10) | 6 (0.26) |
| decimal_factorial | 7251 | 7104 (97.97) | 2 (0.03) | 143 (1.97) | 2 (0.03) |
| deltablue | 2316 | 2165 (93.48) | 2 (0.09) | 143 (6.17) | 6 (0.26) |
| fannkuch | 2282 | 2131 (93.38) | 2 (0.09) | 143 (6.27) | 6 (0.26) |
| float | 1602282 | 1602122 (99.99) | 2 (0.00) | 152 (0.01) | 6 (0.00) |
| go | 181992 | 174066 (95.64) | 2 (0.00) | 7918 (4.35) | 6 (0.00) |
| hexiom | 2284 | 2133 (93.39) | 2 (0.09) | 143 (6.26) | 6 (0.26) |
| json_dumps | 2283 | 2132 (93.39) | 2 (0.09) | 143 (6.26) | 6 (0.26) |
| json_loads | 2478 | 2327 (93.91) | 2 (0.08) | 143 (5.77) | 6 (0.24) |
| mdp | 4604114 | 4452599 (96.71) | 2 (0.00) | 151507 (3.29) | 6 (0.00) |
| meteor_contest | 35144 | 34777 (98.96) | 2 (0.01) | 359 (1.02) | 6 (0.02) |
| nbody | 5402708 | 5402541 (100.00) | 2 (0.00) | 159 (0.00) | 6 (0.00) |
| nqueens | 2282 | 2131 (93.38) | 2 (0.09) | 143 (6.27) | 6 (0.26) |
| pickle (2 procs) | 5040 | 4738 (94.01) | 4 (0.08) | 286 (5.67) | 12 (0.24) |
| pidigits | 2282 | 2131 (93.38) | 2 (0.09) | 143 (6.27) | 6 (0.26) |
| pprint | 2359 | 2208 (93.60) | 2 (0.08) | 143 (6.06) | 6 (0.25) |
| pyflate | 2390 | 2239 (93.68) | 2 (0.08) | 143 (5.98) | 6 (0.25) |
| raytrace | 4950527 | 4773043 (96.41) | 2 (0.00) | 177476 (3.58) | 6 (0.00) |
| regex_effbot | 2284 | 2133 (93.39) | 2 (0.09) | 143 (6.26) | 6 (0.26) |
| regex_v8 | 2283 | 2132 (93.39) | 2 (0.09) | 143 (6.26) | 6 (0.26) |
| richards | 2282 | 2131 (93.38) | 2 (0.09) | 143 (6.27) | 6 (0.26) |
| scimark | 13169890 | 11911288 (90.44) | 2 (0.00) | 1258594 (9.56) | 6 (0.00) |
| spectral_norm | 2030803 | 2030652 (99.99) | 2 (0.00) | 143 (0.01) | 6 (0.00) |
| sqlglot_v2 (4 procs) | 12658 | 12034 (95.07) | 8 (0.06) | 584 (4.61) | 32 (0.25) |
| telco | 2282 | 2131 (93.38) | 2 (0.09) | 143 (6.27) | 6 (0.26) |
| unpack_sequence | 2292 | 2141 (93.41) | 2 (0.09) | 143 (6.24) | 6 (0.26) |
| **sum** | 62485312 | 60521206 (96.86) | 66 (0.00) | 1963838 (3.14) | 202 (0.00) |

Mean of per-workload int shares (equal weight per workload, workloads with >1000 int results): small=69.29%, le31=22.36%, b32_51=4.06%, b52_62=2.23%, gt62=2.05%; max 32-51 = 34.82%, max 52-62 = 12.88%
Float outside-window share: mean 0.053%, max 0.088%

Top Python call sites for the 52-62-bit bucket (pyperformance), top frame at creation (C callers show their Python caller):

- 34275: pp-go nv/lib/python3.16/site-packages/pyperformance/data-files/benchmarks/bm_go/run_benchmark.py:149 in update
- 33899: pp-go nv/lib/python3.16/site-packages/pyperformance/data-files/benchmarks/bm_go/run_benchmark.py:150 in update
- 22425: pp-decimal_factorial ython/.worktrees/python-boil-the-ocean/build/worktrees/upstream-intwidth/Lib/_pydecimal.py:5757 in _normalize
- 20002: pp-decimal_factorial ython/.worktrees/python-boil-the-ocean/build/worktrees/upstream-intwidth/Lib/_pydecimal.py:4164 in Etiny
- 10001: pp-decimal_factorial ython/.worktrees/python-boil-the-ocean/build/worktrees/upstream-intwidth/Lib/_pydecimal.py:1746 in _fix
- 9942: pp-sqlglot_v2 <frozen importlib._bootstrap_external>:155 in _path_stat
- 7473: pp-go nv/lib/python3.16/site-packages/pyperformance/data-files/benchmarks/bm_go/run_benchmark.py:144 in __init__
- 3318: pp-pickle <frozen importlib._bootstrap_external>:155 in _path_stat
- 2488: pp-sqlglot_v2 ode/cpython/.worktrees/python-boil-the-ocean/build/worktrees/upstream-intwidth/Lib/enum.py:1345 in __hash__
- 1731: pp-pyflate <frozen importlib._bootstrap_external>:155 in _path_stat
- 1686: pp-crypto_pyaes <frozen importlib._bootstrap_external>:155 in _path_stat
- 1647: pp-pprint <frozen importlib._bootstrap_external>:155 in _path_stat

Top Python call sites for the 32-51-bit bucket (pyperformance), top frame at creation (C callers show their Python caller):

- 900002: pp-pprint e/cpython/.worktrees/python-boil-the-ocean/build/worktrees/upstream-intwidth/Lib/pprint.py:886 in _safe_repr
- 300000: pp-pprint e/cpython/.worktrees/python-boil-the-ocean/build/worktrees/upstream-intwidth/Lib/pprint.py:841 in _safe_repr
- 202838: pp-crypto_pyaes /tmp/iw/venv/lib/python3.16/site-packages/pyaes/aes.py:219 in encrypt
- 100001: pp-pprint e/cpython/.worktrees/python-boil-the-ocean/build/worktrees/upstream-intwidth/Lib/pprint.py:195 in _format
- 9001: pp-json_dumps hon/.worktrees/python-boil-the-ocean/build/worktrees/upstream-intwidth/Lib/json/encoder.py:262 in iterencode
- 5752: pp-crypto_pyaes /tmp/iw/venv/lib/python3.16/site-packages/pyaes/aes.py:233 in encrypt
- 2008: pp-sqlglot_v2 <frozen importlib._bootstrap>:304 in acquire
- 2008: pp-sqlglot_v2 <frozen importlib._bootstrap>:367 in release
- 1047: pp-crypto_pyaes <frozen importlib._bootstrap_external>:503 in _compile_bytecode
- 1016: pp-sqlglot_v2 <frozen importlib._bootstrap>:543 in _call_with_frames_removed
- 960: pp-sqlglot_v2 ktrees/python-boil-the-ocean/build/worktrees/upstream-intwidth/Lib/collections/__init__.py:447 in namedtuple
- 948: pp-sqlglot_v2 ode/cpython/.worktrees/python-boil-the-ocean/build/worktrees/upstream-intwidth/Lib/copy.py:121 in deepcopy

### tests

Ints (bucket = bit length of |x|; small ints counted separately; small + <=31 + 32-51 + 52-62 + >62 = total)

| workload | int results | small -5..256 (%) | <=31 bit non-small (%) | 32-51 (%) | 52-62 (%) | >62 (%) |
|---|---|---|---|---|---|---|
| tests (89 procs) | 34132164 | 19301412 (56.55) | 8581830 (25.14) | 682393 (2.00) | 1663822 (4.87) | 3902707 (11.43) |
| **sum** | 34132164 | 19301412 (56.55) | 8581830 (25.14) | 682393 (2.00) | 1663822 (4.87) | 3902707 (11.43) |

Floats

| workload | float results | in window |e|<=255 (%) | outside (%) | zero (%) | nan/inf (%) |
|---|---|---|---|---|
| tests (89 procs) | 19781010 | 18480621 (93.43) | 155725 (0.79) | 785857 (3.97) | 358807 (1.81) |
| **sum** | 19781010 | 18480621 (93.43) | 155725 (0.79) | 785857 (3.97) | 358807 (1.81) |

Mean of per-workload int shares (equal weight per workload, workloads with >1000 int results): small=56.55%, le31=25.14%, b32_51=2.00%, b52_62=4.87%, gt62=11.43%; max 32-51 = 2.00%, max 52-62 = 4.87%
Float outside-window share: mean 0.787%, max 0.787%

Top Python call sites for the 52-62-bit bucket (tests), top frame at creation (C callers show their Python caller):

- 520000: tests n/.worktrees/python-boil-the-ocean/build/worktrees/upstream-intwidth/Lib/test/test_math.py:708 in msum
- 139144: tests ython/.worktrees/python-boil-the-ocean/build/worktrees/upstream-intwidth/Lib/statistics.py:1603 in _exact_ratio
- 129314: tests e/cpython/.worktrees/python-boil-the-ocean/build/worktrees/upstream-intwidth/Lib/random.py:915 in getrandbits
- 110802: tests n/.worktrees/python-boil-the-ocean/build/worktrees/upstream-intwidth/Lib/test/test_long.py:851 in check_truediv
- 99739: tests e/cpython/.worktrees/python-boil-the-ocean/build/worktrees/upstream-intwidth/Lib/random.py:916 in getrandbits
- 75162: tests e/cpython/.worktrees/python-boil-the-ocean/build/worktrees/upstream-intwidth/Lib/random.py:249 in _randbelow_with_getrandbits
- 69973: tests <frozen importlib._bootstrap_external>:155 in _path_stat
- 52285: tests n/.worktrees/python-boil-the-ocean/build/worktrees/upstream-intwidth/Lib/test/test_long.py:863 in check_truediv
- 50728: tests n/.worktrees/python-boil-the-ocean/build/worktrees/upstream-intwidth/Lib/test/test_long.py:104 in truediv
- 41542: tests n/.worktrees/python-boil-the-ocean/build/worktrees/upstream-intwidth/Lib/test/test_math.py:713 in msum
- 38697: tests ython/.worktrees/python-boil-the-ocean/build/worktrees/upstream-intwidth/Lib/_pydecimal.py:750 in from_float
- 25978: tests ython/.worktrees/python-boil-the-ocean/build/worktrees/upstream-intwidth/Lib/_pydecimal.py:752 in from_float

Top Python call sites for the 32-51-bit bucket (tests), top frame at creation (C callers show their Python caller):

- 48120: tests ython/.worktrees/python-boil-the-ocean/build/worktrees/upstream-intwidth/Lib/statistics.py:1603 in _exact_ratio
- 41593: tests n/.worktrees/python-boil-the-ocean/build/worktrees/upstream-intwidth/Lib/test/test_long.py:98 in truediv
- 37050: tests thon/.worktrees/python-boil-the-ocean/build/worktrees/upstream-intwidth/Lib/_pydatetime.py:881 in _to_microseconds
- 36162: tests thon/.worktrees/python-boil-the-ocean/build/worktrees/upstream-intwidth/Lib/dataclasses.py:499 in add_fns_to_class
- 33805: tests e/cpython/.worktrees/python-boil-the-ocean/build/worktrees/upstream-intwidth/Lib/random.py:916 in getrandbits
- 26883: tests e/cpython/.worktrees/python-boil-the-ocean/build/worktrees/upstream-intwidth/Lib/random.py:249 in _randbelow_with_getrandbits
- 21481: tests <frozen importlib._bootstrap>:543 in _call_with_frames_removed
- 18042: tests worktrees/python-boil-the-ocean/build/worktrees/upstream-intwidth/Lib/test/test_hashlib.py:923 in selftest_seq
- 17102: tests n/.worktrees/python-boil-the-ocean/build/worktrees/upstream-intwidth/Lib/test/test_long.py:257 in check_bitop_identities_1
- 15637: tests ython/.worktrees/python-boil-the-ocean/build/worktrees/upstream-intwidth/Lib/_pydecimal.py:1422 in __truediv__
- 14553: tests ython/.worktrees/python-boil-the-ocean/build/worktrees/upstream-intwidth/Lib/_pydecimal.py:750 in from_float
- 14285: tests <frozen importlib._bootstrap>:367 in release
### Exact bit-length histogram (non-small results, baseline-subtracted)

| bit length | kernels | pyperformance | tests |
|---|---|---|---|
| <=31 | 4708907 (80.63% of non-small) | 12295615 (85.87% of non-small) | 8581926 (57.87% of non-small) |
| 32-40 | 10325 (0.18% of non-small) | 212230 (1.48% of non-small) | 206384 (1.39% of non-small) |
| 41-46 | 1 (0.00% of non-small) | 389 (0.00% of non-small) | 71661 (0.48% of non-small) |
| 47 | 937766 (16.06% of non-small) | 1402467 (9.79% of non-small) | 208130 (1.40% of non-small) |
| 48 | 0 (0.00% of non-small) | 91 (0.00% of non-small) | 18642 (0.13% of non-small) |
| 49 | 2 (0.00% of non-small) | 149 (0.00% of non-small) | 51905 (0.35% of non-small) |
| 50 | 2 (0.00% of non-small) | 119 (0.00% of non-small) | 47909 (0.32% of non-small) |
| 51 | 19 (0.00% of non-small) | 111 (0.00% of non-small) | 77753 (0.52% of non-small) |
| 52 | 27 (0.00% of non-small) | 146 (0.00% of non-small) | 146368 (0.99% of non-small) |
| 53 | 63 (0.00% of non-small) | 181 (0.00% of non-small) | 936090 (6.31% of non-small) |
| 54-60 | 19349 (0.33% of non-small) | 56333 (0.39% of non-small) | 468454 (3.16% of non-small) |
| 61 | 45529 (0.78% of non-small) | 101719 (0.71% of non-small) | 96167 (0.65% of non-small) |
| 62 | 36413 (0.62% of non-small) | 38792 (0.27% of non-small) | 16749 (0.11% of non-small) |
| >62 | 81941 (1.40% of non-small) | 210879 (1.47% of non-small) | 3902706 (26.31% of non-small) |
