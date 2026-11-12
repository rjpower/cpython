# Activation 53 final-home construction result

The full final-home candidate is **rejected and not promoted**. It improves
generators by 7.0% in six pairs against golden51, but coroutines regress 2.8%,
logging silent 24.4%, and comprehensions 1.5%; all six pairs lose in each of
those cases. The generator result does not meet the coroutine and breadth gates.
Golden51 remains selected and unchanged. The bounded native-only rescope was also rejected after measurement. No further
IR rescue queue is planned.

The candidate is `4b339980d020ae4f6026474e8942b7174ce57d16`; selected golden51
is `0e5e61f4c6bc820f2dd1a5f9ee6f7f1768834254`. The
[proposal and peer review](final-homes-proposal.md), [Astra review](astra-review.md),
[review resolution](review-resolution.md), [corrected design](../../design/activation-transfers.md),
and [source/native post-review](final-homes-post-review.md) describe the contract,
corrections and evidence.

## Measurement

Unchanged kernels, GCC 15.2, CPU 15, hash 0, JIT disabled. Ratios are elapsed
candidate/golden51; lower is faster. The screen and confirmation are separate
direct campaigns, not ratios to multiply. Losses count pairs above 1.0.

| Case | Screen: 2-pair geometric mean | Confirmation: 6-pair geometric mean | Confirmation pair range | Losses |
| --- | ---: | ---: | ---: | ---: |
| Generators | 0.936477 | 0.929631 | 0.922617–0.939132 | 0/6 |
| Coroutines | 1.033242 | 1.027712 | 1.022094–1.036887 | 6/6 |
| DeltaBlue | 0.993470 | 0.999114 | 0.990558–1.004414 | 4/6 |
| Logging silent | 1.186077 | 1.244130 | 1.094948–1.317357 | 6/6 |
| Comprehensions | 1.021965 | 1.014538 | 1.007823–1.023783 | 6/6 |
| Richards | 1.004303 | 0.999574 | 0.994945–1.003979 | 3/6 |

Logging's confirmation spread is wide, although every pair loses. Both campaigns
report stable identities, no failure and no postflight error. No all29 or upstream
campaign was run for this rejected candidate. Raw outputs, commands, summaries,
status and before/after identities are retained under E/`final-homes-{screen,confirm}-hot-golden`,
where E is `/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/activation53`.

Separate standard benchmark instruction differencing used two samples per image:
4/12 loops for coroutines and 60000/180000 for logging. Candidate/golden51
instructions per loop were 0.985784 and 1.008263 respectively, while the direct
elapsed confirmation ratios above were 1.027712 and 1.244130. Work count alone
does not explain the elapsed result; these measurements do not establish a
hardware cause. E/`fullA-instructions-{candidate,golden}.json`, their raw JSONL,
and `instruction-diagnostic-{before,after}` retain the counts and stable identity
census. These diagnostics are separate from quiet timing.

## Mechanism and qualification

The [post-review](final-homes-post-review.md) found no remaining blocking
source/native defect for isolated timing. Native inspection confirmed direct
construction bypasses transient thread-frame reservation, copying and pop;
native vector hits bypass creation-only evaluator entry. The selected native
symbols grow by 3206 bytes, not a whole-image size claim. A particular ordinary
exact-call warm path has 151 static instructions versus golden51's 149; this
is not a dynamic instruction ratio. The normal-ABI helper and its remaining
allocation, initialization and ownership work do not prove a performance gain.

Diagnostic hits identify 100000 native value-vector `Tree.__iter__` entries
and 242784 exact IR coroutine Fibonacci entries in unchanged workload loops.
The latter excludes one outer generic construction. A separate known-plan probe
recorded 64 entries; it is not part of the workload counts. These demonstrate
routes, not throughput.

The initial release run failed one tracing/redirect case among eight tests;
the same case passed on golden51. The correction retains CALL as the instruction
cursor separately from `return_pc` during result cleanup. A subsequent debug
compile needed a compatibility API port. Both corrections are reviewed and
retained with their original failure evidence.

At the current candidate head, release and debug builds each checked 117 modules;
569 filtered protocol tests passed per role. All eight construction cases passed
individually in fresh processes per role, as did two producer tests per role.
Four forced-allocation-failure probes covered generators and coroutines in both
roles. These are focused checks, not exhaustive allocation-failure coverage.
No full suite was run for the rejected candidate. Exact logs, hit recordings and
native identities are listed in the post-review and retained under E.
