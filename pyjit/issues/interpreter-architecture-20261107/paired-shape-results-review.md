# R5 paired shape results review

2026-11-07. **Accept the completed bounded comparison. Recommend keeping R5
provisionally for the planned matched whole-workload admission; final
performance acceptance remains open.** Temporary object, large-integer and
NaN reads improve repeatedly in this campaign. Folded-local reads regress,
and the basic screen has an adverse aggregate. The results support neither
an unconditional throughput claim nor a representative net benefit.

This independently reviews the retained `r5-paired-shape` and R4/R5 basic
records under `python-boil-the-ocean/build/pyjit-link-wave1/interpreter-architecture-20261107/`,
the collector source, and actual retained GCC disassembly. I recomputed the
statistics from child outputs using `uv run`; I ran no native workloads,
builds or tests and changed no runtime source or retained evidence. See the
[cold source review](cold-r5-review.md) for ownership and correctness scope.

The paired campaign contains 120 successful native children and 60 pairs:
six rounds, five value categories and two destination forms, with two fresh
processes per pair. Each child warms the reader for 20,000 iterations, then
retains five timings of 1,000,000 iterations. I checked every raw vector,
return code, empty stderr, native revision, CPU affinity, specialization and
destination record. The recorded commands match the collector and use CPU
15, fixed loop/sample counts and balanced R4/R5 then R5/R4 order for each
cell. Cell traversal rotates across rounds. No load waits or rejected child
prefixes occur in this completed campaign. Maximum before/after one-minute
load is 1.573242; the componentwise maxima of the full tuples are
`(1.573242, 2.061523, 2.229980)`.

Before/after snapshots are equal, source is clean at `0953757f4e7`, and the
collector hash matches the reviewed file. R4/R5 have distinct inodes and
stable executable hashes, with native revisions `b58036e9718`/`0953757f4e7`
and build IDs `a410f4a9c634117fdb4921079b2bcdcdfd190b92`/
`9d98a70694382c4650742915f0560f12346c85bf`. All 120 children have the same
63-entry imported-module origin/hash manifest, including the four imported
extension DSOs. This establishes the shared imported files used by the
comparison, not an inventory of every transitive system library. Successful
execution of the hash-matched collector enforces its before/after upstream
JIT-off assertions and the current runtime's method-JIT API-absence
assertions; the latter flags are also retained in every child record. Both
specialization/destination inspections agree within each child. Receiver and
value owners survive the timed loop, and GC is disabled during timing.

For each pair, I divided the R5 median of its five samples by the R4 median.
The table gives the geometric mean, median and full range of those six
ratios. Lower is faster. All values reproduce `summary.json`.

| Destination | Value | Geometric mean | Median | Pair range |
| --- | --- | ---: | ---: | ---: |
| Temporary | int | 0.926943 | 0.982012 | 0.693639–0.990654 |
| Temporary | float | 0.983583 | 0.985989 | 0.967407–0.993172 |
| Temporary | object | 0.889392 | 0.892755 | 0.878107–0.896919 |
| Temporary | bigint | 0.419321 | 0.422608 | 0.400309–0.425655 |
| Temporary | NaN | 0.715820 | 0.714099 | 0.711705–0.726473 |
| Folded local | int | 1.024783 | 1.025146 | 1.021463–1.027138 |
| Folded local | float | 1.018377 | 1.018937 | 1.002926–1.028417 |
| Folded local | object | 1.025985 | 1.026559 | 1.018755–1.034620 |
| Folded local | bigint | 1.008540 | 1.009662 | 0.996625–1.017741 |
| Folded local | NaN | 1.015070 | 1.014327 | 1.008646–1.023404 |

The temporary-int mean is distorted by the first R4 process: all five of
its samples are about 13.96–14.00 ns/iteration, while later R4 process
medians are about 9.60–9.75 ns. Its paired R5 median is 9.70 ns. This is a
process-level anomaly rather than one high timing sample that a within-child
median removes. Retain it in the reported result. As a labeled sensitivity
calculation, the other five pair ratios have geometric mean 0.982284,
consistent with the all-six median 0.982012. The campaign does not establish
a repeatable seven-percent integer improvement. The float effect is also
small. Object, bigint and NaN temporary gains are much larger and hold in
all six balanced pairs; the bigint and NaN source paths avoid repeated
canonicalization calls, while ordinary objects avoid pointer/type tests.

The local regressions cannot be dismissed as unchanged source behavior or
removed by subtracting the temporary/local forms. Twenty-nine of thirty
local pair ratios exceed one; the geometric-mean slowdowns range from 0.85%
to 2.60%. These forms have different store, overwrite and retirement work,
so they are specificity controls, not a subtraction estimator. There is no
declared representative frequency distribution that would justify pooling
the ten cells into a net workload score.

Actual GCC provides one concrete local-path difference. The measured reader
has a local receiver, folded-local destination and no companion NULL. R4
tests those conditions and reaches local publication without computing the
unused companion slot. R5 executes `lea 1(%r8),%edx` at `0x352d7e` and
`lea (%r12,%rdx,8),%rdx` at `0x352d82` before the destination/companion
branches. Those two address calculations are extra work on this route.
Ordinary local publication and its conditional numeric conversion remain
in both versions. Register allocation, code layout and branch placement also
change. The two extra instructions are a concrete observation, but neither
they nor placement/frequency hypotheses establish the cause or magnitude of
the measured regression without counters or a controlled variant.

I also recomputed every basic per-run median from its seven raw samples,
then each screen's median across three rounds. Both R4 and R5 identity
snapshots are stable; all recorded driver commands returned zero. Candidate
R5/R4 factors are deltablue 0.976367, logging_silent 1.022136,
comprehensions 1.060009, richards 1.054883, nbody 1.021940, float 1.040364
and generators 0.982199. Their geometric mean is **1.022097**, while the
upstream-control drift is 0.998896. Five of seven candidate kernels are
slower. These are sequential screens with substantial round variation:
candidate round-median ranges reach roughly 20–22% in some cells. The
almost unchanged aggregate upstream control does not cancel individual
workload drift or turn this into an interleaved R4/R5 experiment. Do not
normalize the paired results by that control or call the screen a proven
2.21% R5 regression. Its direction still belongs in the admission decision.

The narrow elapsed gains justify continuing the bounded experiment, so I
would retain the frozen candidate through the planned 29-kernel/subset
comparison. They do not yet justify permanent performance admission in the
presence of the local penalty and mixed basic screen. The final keep/revert
decision needs representative matched workload results with those costs
visible. Six rounds on one pinned CPU, short per-process samples and load
guards do not establish multi-host significance, fixed frequency, a causal
hardware explanation or broad Python speedup. The coherent-boundary stats
build and full release/debug suites now underway are correctness gates;
their completion will not resolve the performance tradeoff. No native
method/tracing backend or recovery implementation is implied by these
interpreter measurements.
