# Independent elapsed outcome — 2026-10-31 v1

Disposition: **REJECT the current thirteen-handler constant-operand increment on elapsed performance. No additional samples or qualification are authorized.** The primary workload failed the predeclared elapsed gate in both rounds. This decision uses actual elapsed measurements; the earlier evaluator-text increase remains diagnostic and is not the rejection rule.

## Exact campaign and artifact audit

The accepted elapsed-only plan is `1da3b0de4583d1a99b8650e9a3aa4cb32dfcf3dfde9521b439f00aa383bd3ec2`; held launch `05ec8db83a5997dc04c3ad03ca9045f95fe681d0259517bca2af0e01cefdba36`; actual activation `89ab33c0e3129386c397d59c0e490beb89b9e7cb68c82d0f0d02b03881db2aeb`. The window was 23:08:14.661586–23:53:14.661586 UTC. The parent image/source and candidate O3/source remain those bound by the [preparation review](quiet-preparation-20261031-v1.md), including exact candidate `a84965eaa90267ac1dbf1b4c97ab032aaf26cedd` and parent `e9e10d5b503eaf219797a5db44766eeab6fad4dc`.

Root confirmed outer session 26970 terminal zero at 23:09:15. Actual launch-result SHA is `cb434f57d86649492fc279af8b2b29a84c737e52f4647b2a4dbf927f8e25a5c7`: gate zero, post-cleanup verified, failure null, and finish before the original deadline. Terminal-cleanup SHA `9c8ef81e59caec5250223357547119ef18920d485109566302ac1a6c622b3dc8` reports both target PID lists empty, idle under both build locks, and no overrun. The independent audit additionally acquired both existing build locks and found neither image running.

I rehashed all 171 launch-result artifact bindings, replayed the actual activation and read-only source/product/tool/external-ELF verifier, and compared products-before, products-after, post-cleanup-products, terminal-products, and current inventories. All match. Native source HEADs remain exact/clean; the accepted PRIMARY benchmark ancestry/subtree/file bridge still passes. Actual inherited-lock receipts name both global slots, admission lock, and both build locks. Recorded campaign affinity is CPU0; expected sibling topology remains `0,8`.

All eight actual workload/build import-plus-loader traces reparse identically through the unchanged origin validator. They bind their own executable, stdlib/extensions, shared original benchmark module, and allowed external ELF files. Each produced `[]` after its one unmeasured warmup. All sixteen scored commands match their literal frozen cell, execute serially in the predeclared rotating order, return zero without timeout, and retain the exact seed-zero environment, stdout/stderr and command metadata. The coverage remains OFF/N0; no checker, stress, root-statistics, or specialized-handler-frequency credit is inferred.

All **80 raw elapsed durations** reconstruct the saved normalized vectors, medians, spreads, and score ratios exactly. They are five serial timed calls after one warmup in each of sixteen fresh processes, not eighty independent process samples. All 48 before/after quiet checks were accepted with unchanged CPU policy and one-minute load below 1.5; the maximum observed was 0.9931640625. All eight origin witnesses preceded scoring. No replacement instruction samples, new workload, calibration, retry, or extra round occurred.

## Actual elapsed decision

Positive percentages below mean the candidate took longer. Score SHA: `7e1d02f12e9e9c6b5676e53cfaf390426b5d75f137a74f7608031c50f0fa602a`.

| Registered workload | Round 0 candidate/parent | Round 1 candidate/parent |
| --- | ---: | ---: |
| coroutines — primary | 1.044653 (+4.47%) | 1.116384 (+11.64%) |
| spectral_norm — secondary | 1.086916 (+8.69%) | 1.190784 (+19.08%) |
| generators — control | 1.234133 (+23.41%) | 1.219633 (+21.96%) |
| unpack_sequence — control | 1.503807 (+50.38%) | 1.427862 (+42.79%) |

The primary required candidate/parent median ≤0.99 in each round, with supporting variability. It achieved neither improvement. Its per-loop medians were 40.5652→42.3766 ms in round 0 and 39.8791→44.5204 ms in round 1. Secondary results cannot rescue that failure. Both controls also exceed the predeclared investigation threshold in both rounds; no acceptance is attempted despite them.

The primary vectors have descriptive within-cell max/min spreads of approximately 6.6–10.8%; spectral_norm spreads reach approximately 58%. These are small serial selections, not confidence intervals or a general benchmark-suite estimate. They justify rejecting this increment under its fixed experiment, without claiming a precise universal slowdown. Controls and rotating order retain useful context; this review does not manufacture a noise adjustment or more samples to rescue the candidate.

The static work-removal proof and successful semantic selections remain valid evidence: fewer operand-frame operations did not establish an elapsed win. The measured 26,250-byte (+18.4248%) evaluator-text increase is preserved. Instruction counters were concretely unavailable because of permissions. Neither fact proves instruction-cache, branch-predictor, frequency, layout, GC, or dispatch causation. No hardware-counter or causal diagnosis was obtained in this campaign.

Preserve the rejected source, native products, original failure history, raw vectors, and both rounds. Do not land this increment or relabel its feedback as qualification. Any narrower implementation or diagnostic experiment requires a separate concrete hypothesis and finite review; this outcome grants no rerun.

Independent host audit: `build/constant-operands-review/quiet-outcome-20261031-v1.py` and its JSON receipt. No target/benchmark invocation occurred during the audit.
