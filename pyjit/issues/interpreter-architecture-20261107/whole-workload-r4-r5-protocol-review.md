# R4/R5 whole-workload protocol review

2026-11-07. **Accept the proposed reuse of `bench/run.py` for a bounded
supplemental R4/R5 comparison, with explicit identity/environment gates.
Recommend four rounds, AB/BA/AB/BA, for balanced order.** Three rounds
AB/BA/AB remain a useful screen, but give R4 two first positions and R5 one;
that imbalance limits a small-effect admission conclusion. No workload was
run for this review.

I inspected the actual `resolve_build`, `build_info`, measurement and load
paths in `pyjit/bench/run.py`, its native `internal/driver.py`, and the
existing measurement plan. Passing the retained R4 executable in the release
build directory works directly: resolution preserves its distinct filename.
The same R5 stdlib and extension files can therefore serve both executables,
as in the accepted paired-shape comparison. There is no need for a new build,
worktree, runtime switch or benchmark implementation.

`--set kernels --metric time --time-runs 5 --max-load 4` selects exactly the
29 declared kernels. Their loop counts are fixed in the benchmark table,
never calibrated separately for each executable. Each kernel runs in a fresh
native process, performs untimed setup and one warmup at that loop count,
then retains five timed calls. Startup and setup are outside the timed calls.
Use no loop override or update/merge mode. Keep each complete invocation's
JSON, stdout, stderr, command, return code, order and timestamps in a new
campaign directory.

The proposed quiet launch below three and post-check below four belong in
the outer wrapper, with full load tuples retained. Internally `run.py`
samples only the one-minute load before every kernel and once at the end,
failing at four. Its output records those samples, not per-kernel full
1/5/15-minute tuples. Record that distinction. A failed invocation returns
without writing its final timing JSON; its earlier raw time vectors are not
journaled. Preserve the failure logs and absent-report status, stop the
campaign and do not reuse successful prefixes. This satisfies the proposed
no-retry/no-pooling rule, but must not be described as retention of raw
vectors from a failed invocation. Successful complete reports retain all
five values for every kernel.

`run.py` removes `PYTHON*` variables and sets hash seed zero. It does not
remove `LD_PRELOAD`, set `LD_BIND_NOW`, enforce JIT-off or hash imported
files. The outer wrapper should sanitize those native-loader variables as
in the paired campaign and launch both roles under the same recorded
environment. Probe both roles before and after using that environment and
`-S`: require their exact native Git revisions, upstream JIT disabled and
the current runtime's method-JIT API absent. Freeze source at clean R5
`0953757f4e7`, both executable hashes/build IDs/inodes, configured compiler
and flags, and the benchmark/driver identities. Do not infer R4's native
revision from the shared source HEAD or shared sysconfig files.

The paired-shape manifest covers 63 imported modules and four extension
DSOs. That is too narrow by itself for all 29 kernels, which exercise other
native providers. The simplest sufficient extension is one before/after hash
manifest of all release extension DSOs, together with the common stdlib
source identity and clean source check. Require equality across both roles
and across the campaign; retain probes that imports resolve into those
common source/build locations. This avoids instrumenting every timed child
or altering the frozen runtime. A hash manifest establishes the common
extension files; it is not a claim to inventory all transitive system
libraries.

Before collection, fix the estimator: take the median of the five samples
for each role/kernel/round, form the same-round R5/R4 ratio, report every
round ratio and its spread, and use the geometric mean across rounds for
each kernel. An explicitly equal-weight 29-kernel geometric mean summarizes
this declared set. Do not weight the ten shape microbenchmarks into it or
subtract local controls. Retain the previous basic screen separately and
do not normalize this direct R4/R5 comparison by upstream drift.

This checks an actual workload tradeoff that the sequential basic screens
cannot isolate well. Alternating whole-suite blocks still leave temporal
drift and fixed kernel ordering; a small mixed effect may remain unresolved.
Report that limit rather than claim statistical certainty from five samples
sharing one process. The agreed U/I/R5 kernel, pyperformance and selected
counter campaigns retain their broader purpose. Keep/revert must account
for the observed local penalty and adverse basic screen as well as any
whole-workload gain. Correctness qualification and this performance
admission are separate decisions.

Source acceptance, same day: I reviewed the final `measure-r4-r5.py`, SHA-256
`1bd54885c73711a904ce63dbb448e3d2d4f8fd7bc0af4781099bed40984aec61`.
**Accept this collector for the bounded campaign; no remaining source
blocker.** It implements four balanced whole-roster rounds, preserves
same-round ratios separately from the ratio of across-round medians, checks
native revisions and per-report medians, and snapshots the same inherited
affinity. Before/after manifests cover runtime `Lib/**/*.py`, canonical
`bench/**/*.py` including the actual native driver and kernel dependencies,
all 79 release extension DSOs, collectors and executable identities. Both
native probes run in the sanitized environment and enforce JIT-off/current
method-API absence. `LD_BIND_NOW=1` reaches the measured children; no claim
depends on a `PYTHON*` JIT variable that `run.py` would remove.

The source review found and the coordinator repaired three evidence/lifecycle
gaps before execution: missing benchmark-source hashes, loss of timeout
output/journal or terminal status when final snapshot validation failed,
and possible survival of a native benchmark descendant after killing only
the host driver. The final collector starts a new process session, kills
the process group on timeout, drains/reaps the driver, retains stdout/stderr
and the attempted command, then propagates failure. Its final journal/status
write survives a postflight snapshot exception and preserves the earlier
failure when present. Rejected campaigns remain separate from successful
measurements; raw-vector retention on failed `run.py` invocations has the
limitation already stated above. These failure paths were source-reviewed,
not exercised by launching native workloads. This acceptance covers the
collector, not uncollected results or final R5 performance admission.

Preflight correction, same day: **accept collector SHA-256
`9ad59d3cf29053eac61cc405c904d481d7aa5420873e57745636688086dd28d7`.**
The original count incorrectly treated every on-disk DSO as a current-ABI
module; the source review missed that distinction. I independently counted
213 files: 67 ABI-1, 67 ABI-2 and 79 current ABI-3 extensions. The corrected
predicate requires 79 filenames ending in the SOABI obtained from the native
probe. The hash manifest still includes all 213 files, preserving the older
artifacts and detecting any change to them. Reconstructing the prior
predicate reproduces the previously accepted collector hash exactly, so no
other collector code changed. Syntax parsing passes. The retained first
attempt's traceback stops at the old preflight assertion before any timing;
it supplies no accepted timing results. The retry uses the separate fresh
`r4-r5-whole-v2` directory. I ran no native workloads for this correction.
