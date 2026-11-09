# R4/R5 whole-workload results review

2026-11-07. **Accept the completed `r4-r5-whole-v2` comparison. Recommend
keeping the bounded R5 optimization on this evidence.** The declared
29-kernel paired elapsed factor is **0.992103465993**: approximately 0.79%
less elapsed time than R4 in this campaign. All four round aggregates favor
R5. The benefit is modest and mixed; local microbenchmark regressions and
the adverse earlier basic screen remain part of the decision. This is not
proof of broad parity, universal improvement or a cause for every changed row.

I reviewed the saved reports, raw vectors, journal, status, both identity
snapshots and retained collector log under
`python-boil-the-ocean/build/pyjit-link-wave1/interpreter-architecture-20261107/`.
I recomputed the estimators with short host `uv run` calculations. I ran no
native workloads or builds and changed no collector, benchmark or runtime
source. The [protocol review](whole-workload-r4-r5-protocol-review.md) records
the accepted collector and the rejected first preflight. That first attempt
stopped before timing; no prefix from it enters these results.

All eight host-driver invocations returned zero and retained complete
29-kernel reports. Their actual order is R4/R5, R5/R4, R4/R5, R5/R4, agreeing
with the recorded timestamps. Each of the 232 kernel processes uses the
same fixed loop count across roles and rounds, one warmup and five timings.
I checked all 1,160 raw timing values, each reported median, roster, loop
count, native revision, hash-seed record and calling-convention record.
Every reported median equals the median of its raw vector and its entry in
`round-medians.json`. Stderr contains the expected 29 progress rows per
invocation and stdout is empty; no rejected invocation or quiet wait occurs.

Before/after identity snapshots are exactly equal, with no failure or
postflight error. Runtime source is clean at `0953757f4e7`; inherited CPU
affinity is `[15]`. The executable paths, hashes and inodes distinguish the
retained R4 executable from R5. Their native revisions are
`b58036e9718`/`0953757f4e7`, and their ELF build IDs remain
`a410f4a9c634117fdb4921079b2bcdcdfd190b92`/
`9d98a70694382c4650742915f0560f12346c85bf`. Both probes report release
configuration, GCC, `-O3`, frame pointers, mimalloc, upstream JIT disabled
and the current runtime's method-JIT API absent. Probe imports resolve to
the same source/build paths without errors.

The stable shared manifest has 2,473 files: 2,154 runtime Lib Python files,
106 canonical benchmark Python files and all 213 extension DSOs. The latter
include 79 current ABI-3 files and 67 files for each of ABI-1 and ABI-2. The
native driver and kernel dependencies are covered. The collector hash is
the accepted `9ad59d3cf29053eac61cc405c904d481d7aa5420873e57745636688086dd28d7`.
All reports mark the canonical benchmark subtree dirty; the existing
`bench/results/checkpoint-6a0e4c12c17.md` documentation edit explains that
flag. Stable code-file hashes establish the benchmark source used here;
the dirty flag is not silently presented as a clean checkout. These
manifests do not inventory every transitive system library.

All 30 internal one-minute load samples per invocation are below four;
their maximum is 2.150391. Every outer launch is below three and every
outer completion below four. Componentwise maxima of the outer full load
tuples are `(2.150391, 5.159668, 4.754883)` before and
`(2.150391, 4.872070, 4.671387)` after. The higher historical five/fifteen
minute components do not violate the declared one-minute admission rule;
the record does not claim that every load component was below four.

For each kernel and round, I divided the R5 five-sample median by the R4
median from that same round. The primary per-kernel estimate is the
geometric mean of those four ratios. The primary set estimate weights the
29 declared kernels equally in log space. All ratios reproduce
`summary.json` exactly. Lower is faster.

| Kernel | Paired geometric factor | Four-pair range |
| --- | ---: | ---: |
| richards | 0.983424 | 0.964092–0.995414 |
| go | 0.964522 | 0.960228–0.969265 |
| nbody | 1.006931 | 1.003263–1.011043 |
| float | 1.030708 | 1.019377–1.043217 |
| spectral_norm | 0.937324 | 0.929168–0.942869 |
| raytrace | 0.991276 | 0.978276–1.002630 |
| deltablue | 0.959658 | 0.956881–0.964141 |
| chaos | 0.976854 | 0.973065–0.980386 |
| nqueens | 1.002048 | 0.997369–1.006678 |
| fannkuch | 0.994815 | 0.987432–1.003889 |
| generators | 0.982529 | 0.974367–0.992923 |
| comprehensions | 1.010590 | 0.999630–1.038430 |
| coroutines | 1.005550 | 1.000607–1.009382 |
| unpack_sequence | 1.000467 | 0.998531–1.002420 |
| deepcopy | 0.994947 | 0.982518–1.006175 |
| deepcopy_reduce | 1.024255 | 0.990584–1.055223 |
| deepcopy_memo | 0.992077 | 0.988106–0.999018 |
| json_dumps | 0.992844 | 0.975032–1.009346 |
| json_loads | 0.992304 | 0.987433–0.997552 |
| pickle | 0.997188 | 0.985667–1.008107 |
| unpickle | 1.004533 | 0.991272–1.016344 |
| regex_compile | 0.991785 | 0.985114–1.003142 |
| logging_format | 1.000430 | 0.969187–1.011936 |
| logging_silent | 0.974295 | 0.958294–0.980289 |
| logging_simple | 0.983148 | 0.964522–0.999981 |
| xml_etree_generate | 0.999892 | 0.991599–1.011702 |
| xml_etree_iterparse | 0.999588 | 0.982469–1.011665 |
| xml_etree_parse | 0.980975 | 0.971501–0.998491 |
| xml_etree_process | 1.000940 | 0.981148–1.017303 |

The four set factors are 0.990112459034, 0.994051643728, 0.992345910692 and
0.991907810241. Combining the two R4-first rounds gives 0.991228555807;
the two R5-first rounds give 0.992979148420. The aggregate direction therefore
survives both order positions. Nineteen per-kernel primary estimates are
below one; eleven kernels improve in all four pairs. Nbody, float and
coroutines regress in all four, including approximately 3.07% more elapsed
time for float. Mixed rows near one should not be called established wins
or regressions merely because their aggregate falls on one side of one.

The secondary estimator divides each role's median across its four round
medians. Its 29-kernel geometric factor is 0.991789604759. It agrees in
direction but remains distinct from the predeclared paired estimator.
Worst across-round spreads, defined as maximum/minimum minus one for one
role/kernel, are 3.5268% for R4 logging_simple and 6.5920% for R5
deepcopy_reduce. Five samples in one process do not make five independent
process replications. Four paired rounds on one host, fixed kernel order
and alternating whole-suite blocks leave temporal, frequency and placement
effects unresolved. No confidence or multi-host significance claim follows.

The [paired shape review](paired-shape-results-review.md) remains valid:
temporary object/bigint/NaN factors are 0.889392/0.419321/0.715820, while
folded-local factors range from 1.008540 to 1.025985 and 29/30 local pairs
are adverse. The integer temporary outlier remains in its original record.
The earlier sequential seven-kernel basic factor is 1.022097, with upstream
drift 0.998896. This direct, balanced R4/R5 check provides stronger evidence
for this increment than that noisier sequential comparison; it does not erase
the earlier result or authorize pooling or control-drift adjustment.

The combination of reviewed ownership/code-generation behavior, repeated
shape-specific gains and modest benefit on the complete predeclared set
justifies keeping this small optimization. Its local cost remains real
within the micro campaign. The whole-set factors do not prove that the
removed conversion causes every distant kernel change, nor do the two
extra local-path address calculations prove the local slowdown's cause.
The separate upstream/interim/R5, pyperformance and counter campaigns must
still report the overall runtime gap and remaining work. This review does
not approve uncollected results, declare upstream parity or imply a native
method/tracing backend or recovery implementation.
