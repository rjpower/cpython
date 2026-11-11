# Round 50 diagnostic tables

The `baseline50-perf-stat/normalized-summary.json` artifact under
`BASE/pyjit-link-wave1/deltablue-call-model-20261110` records the means of three
intervals per role. BASE is `/home/power/code/cpython/.worktrees/python-boil-the-ocean/build`.
Candidate here means selected48 `5559f60093e`, not experimental A50. Upstream
is `3a440b65695`. Values are per unchanged iteration; elapsed times are
profiling diagnostics, not balanced admission timings.

| Workload | Metric | Upstream | Selected48 | Selected48 / upstream |
| --- | --- | ---: | ---: | ---: |
| DeltaBlue | Native instructions | 38,936,924 | 55,343,778 | 1.42137 |
| DeltaBlue | User cycles | 15,868,451 | 18,680,197 | 1.17719 |
| DeltaBlue | Elapsed ms | 3.74456 | 4.42345 | 1.18130 |
| Logging silent | Native instructions | 13,943.98 | 14,451.03 | 1.03636 |
| Logging silent | User cycles | 4,739.16 | 3,513.77 | 0.74143 |
| Logging silent | Elapsed µs | 1.12092 | 0.84111 | 0.75037 |

The matched `baseline50-perf-record` campaign contains cycle captures and
native annotations for both workloads and roles. Identity checks passed and
all four captures report zero lost samples. `native50a` contains before/after
method and CALL disassembly for the candidate. `delta50a-*-checks.json` and
associated logs record regeneration, builds and focused checks. These files are
under the same evidence directory.

## A50 against frozen48

Two balanced pairs in `measure50a-call-pair48/summary.json` compare candidate
`e014965013b` with frozen48 `5559f60093e`; identities were stable. Ratios are
elapsed candidate/control, so lower is better. These are component measurements,
not an upstream comparison or a final batch admission.

| Workload | Pair 1 | Pair 2 | Geometric mean |
| --- | ---: | ---: | ---: |
| DeltaBlue | 0.908322 | 0.937216 | 0.922656 |
| Logging silent | 0.980452 | 0.973443 | 0.976941 |
| nbody | 0.969363 | 0.973507 | 0.971432 |
