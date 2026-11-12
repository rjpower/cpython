# Activation 53 completion result

The completion candidate is **rejected and not promoted**. Six direct pairs
against selected golden51 show a 3.1% coroutine regression and a 1.1% Richards
regression, each in all six pairs. Golden51 remains unchanged; the full final-home and native-only candidates are also rejected.

Candidate source is `4d40da15b6e2bb9299fa56ef5d08d884a927e7d1` on
`pyjit/activation53-completion`; the control is golden51
`0e5e61f4c6bc820f2dd1a5f9ee6f7f1768834254`. The [proposal](completion-proposal.md),
[Astra review](astra-review.md), [review resolution](review-resolution.md), and
[source/native post-review](completion-post-review.md) describe the contract and
implementation. The post-review found no blocking source defect and confirmed
that the unobserved generator completion path removes the hot C helper boundary.
Ordinary return has no new completion guard. The selected native symbols grow by
761 bytes; this is not a whole-image size result. Removing the boundary did not
produce an acceptable throughput result.

## Measurement

Unchanged kernels, GCC 15.2, CPU 15, hash 0, JIT disabled. Ratios are elapsed
candidate/golden51; lower is faster. The screen and confirmation are separate
direct campaigns, not ratios to multiply. Losses count pairs above 1.0.

| Case | Screen: 2-pair geometric mean | Confirmation: 6-pair geometric mean | Confirmation pair range | Losses |
| --- | ---: | ---: | ---: | ---: |
| Generators | 0.990493 | 0.994377 | 0.987477–1.001501 | 1/6 |
| Coroutines | 1.021513 | 1.031130 | 1.025242–1.036236 | 6/6 |
| DeltaBlue | 0.985352 | 0.989166 | 0.981583–0.995987 | 0/6 |
| Logging silent | 1.010022 | 0.959817 | 0.861368–0.999575 | 0/6 |
| Comprehensions | 0.989078 | 0.988206 | 0.981280–0.997215 | 0/6 |
| Richards | 1.009366 | 1.011310 | 1.008910–1.013916 | 6/6 |

Logging's confirmation has a wide pair spread. Both campaigns report no failure
or postflight error and stable identities. Raw outputs, commands, summaries,
status and before/after identities are under
`/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/activation53/completion-{screen,confirm}-hot-golden`.
No all29 or upstream campaign was run for this rejected candidate.

## Qualification and limits

Release and debug builds each checked 117 modules. The initial dirty release
candidate passed 10 focused tests. On committed release, four protocol modules
passed 309 filtered tests; the overall run failed because the requested fifth
module, `test.test_ir_source_observation`, does not exist. Corrected IR context
runs passed 39 filtered tests in each of release and debug. Debug also passed
317 filtered protocol and semantic tests. Logs are retained in the activation53
evidence directory.

The first protocol attempt is excluded: preflight rejected the old ELF identity
against the new source HEAD before any tests ran. No full suites were run for
this candidate. The source/native review establishes the stated mechanism, not
an integrated performance or full-qualification pass; it does not establish a
result for lane A.
