# Activation 53 native-only construction result

The native-only construction candidate is **rejected and not promoted**. Six
pairs against golden51 improve generators 5.9%, but DeltaBlue regresses 2.4%
and Richards 2.7%, with six losses in each. All three activation 53 candidates
are rejected; golden51 remains selected and unchanged. The fresh selected51 versus
upstream comparison is in the [tables](benchmark-tables.md). Candidate source/build
directories are retired after verified archival; the branch remains pushed.

Candidate `779a1568a41652736dbd4c93b968d3381a9693b5` was built from golden51
`0e5e61f4c6bc820f2dd1a5f9ee6f7f1768834254`. The [proposal](native-construction-proposal.md),
[peer review](native-construction-peer.md) and [post-review](native-construction-post-review.md)
describe the bounded transaction and its source/native inspection. The
post-review accepted the mechanism for isolated screening, not throughput:
success bypasses temporary frame construction and evaluator entry, while native
provider branches, helper cost and code layout still require measurement. The
runtime diff leaves ceval, IR and generated products unchanged.

## Measurement

Unchanged kernels, GCC 15.2, CPU 15, hash 0, JIT disabled. Ratios are elapsed
candidate/golden51; lower is faster. The screen and confirmation are separate
direct campaigns, not ratios to multiply. Losses count pairs above 1.0.

| Case | Screen: 2-pair geometric mean | Confirmation: 6-pair geometric mean | Confirmation pair range | Losses |
| --- | ---: | ---: | ---: | ---: |
| Generators | 0.948050 | 0.940807 | 0.933855–0.950759 | 0/6 |
| Coroutines | 1.007538 | 1.004339 | 0.999832–1.007909 | 5/6 |
| DeltaBlue | 1.025437 | 1.024337 | 1.008533–1.038163 | 6/6 |
| Logging silent | 0.916938 | 0.973093 | 0.866070–1.013261 | 1/6 |
| Comprehensions | 0.997135 | 0.995756 | 0.990313–0.999909 | 0/6 |
| Richards | 1.030639 | 1.027155 | 1.021537–1.031756 | 6/6 |

Logging confirmation has a wide pair spread. Both campaigns report stable
identities, no failure and no postflight error. Raw outputs, commands, summaries,
status and identities are under E/`native-construction-{screen,confirm}-hot-golden`,
where E is `/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/activation53`.
No full suite or all29 campaign was run for the rejected candidate.

## Qualification and route evidence

Release passed 569 existing focused tests plus seven new construction tests;
the new module was initially uncommitted, with the same body in the candidate
commit. Debug passed 576 focused tests including those seven. A native hit probe
recorded 128 generator and 128 coroutine constructions across the combined map
and object-vector API routes. One full unchanged Tree workload loop recorded
100000 admitted allocations and zero errors. The counts establish actual route
execution, not a throughput benefit. Focused checks and post-review do not
establish a full-suite pass.
