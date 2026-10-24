# Baselines

Recorded 2026-10-23 at commit 321925c92c9 (CPython 3.16.0a1+dev main) before
any fork changes. Machine: 16 cores, 62 GiB, gcc 15.2.0, clang 21.1.8 for the
JIT stencils only. No PGO, no LTO, so absolute numbers are below a release
build; the three variants are comparable with each other.

| variant | configure | geomean vs default |
|---|---|---|
| default | `CC=gcc ../../configure --with-lto=no` | 1.000 |
| jit | `CC=gcc ../../configure --enable-experimental-jit` | 0.834 (faster) |
| ft | `CC=gcc ../../configure --disable-gil` | 1.144 (slower) |

Benchmark set: the 30-benchmark pyperformance subset listed in the plan
(nbody, spectral_norm, float, richards, raytrace, deltablue, chaos, go,
nqueens, fannkuch, unpack_sequence, generators, json_dumps, json_loads,
regex_compile, pickle, unpickle, xml_etree, comprehensions, coroutines,
deepcopy, logging), run with `pyperformance run --rigorous=no`. Raw results:
`default.json`, `jit.json`, `ft.json`. Tables: `compare-default-vs-*.txt`
(produced by `boil/tools/pyperf_compare.py`). Peak RSS of fixed workloads:
`rss-*.json` (`boil/tools/rss.py`).

Notable JIT results: richards 0.32, nbody 0.60, spectral_norm 0.63, float 0.69,
coroutines 1.05. The existing JIT's 16.6 percent geomean is the bar the fork's
own tier 0 must clear. The unpack_sequence ratio is noise on a sub-millisecond
value.

Sanity: `test_int test_float test_dict test_list test_class test_functools`
pass on the default build (`sanity-test.txt`).
