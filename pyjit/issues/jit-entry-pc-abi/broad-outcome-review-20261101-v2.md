Independent decision: REJECT_FIXED_ALL29_ELAPSED_GATES for source599897b6 versus the qualified eba351 RC release image. The complete corrected campaign is valid; its performance result rejects further progression of this candidate under the frozen design.

All580 raw values from116 scored processes independently normalize by their fixed loop counts to the retained medians, min/max, spread and MAD. Independent arithmetic reproduces the entire score, all10 primary and290 aggregate single-value deletions per round, and all29 workload-deletion diagnostics. No values from either four-row pilot or failed broad-v1 run enter this calculation.

| Candidate / parent | Round0 | Round1 |
| --- | ---: | ---: |
| spectral_norm primary | 0.8726779377 | 0.8613725635 |
| GM29 | 1.0226460339 | 1.0193604126 |
| Primary10-deletion range | 0.871279160–0.874303070 | 0.861129766–0.862161810 |
| GM29 290-deletion range | 1.021960072–1.023151056 | 1.018200231–1.019949789 |
| GM28 excluding spectral, diagnostic | 1.0284543730 | 1.0255097401 |
| All GM28 diagnostic range | 1.018374584–1.028454373 | 1.015163104–1.025509740 |
| Improved / regressed workloads | 8 / 21 | 7 / 22 |

The primary improves in both rounds, but GM29 regresses2.26%/1.94%; every required aggregate sensitivity result also exceeds1. The mandatory aggregate gate therefore fails. GM28 is reported solely as predeclared concentration information and supplies no extra veto. No individual-control waiver, extra round, reduced workload selection or publication qualification can turn this fixed result into a pass.

Every workload exceeding1.02 in either round is retained in [material-losses.md](material-losses.md), with both actual ratios and per-round ten-value ratio sensitivity ranges. The18-workload union is Richards, go, float, raytrace, deltablue, nqueens, fannkuch, generators, comprehensions, deepcopy, deepcopy_memo, json_dumps, pickle, regex_compile, logging_format, logging_silent, logging_simple and xml_etree_generate. Thirteen exceed2% in both rounds. Examples include logging_silent14.98%/14.42%, generators12.20%/11.87%, deepcopy_memo8.07%/7.09%, deltablue7.19%/6.89% and Richards4.65%/4.72%. These are recorded costs, not dismissed as noise. The candidate is rejected as a whole; accepting these costs to advance despite failed GM29 would contradict the frozen gate.

[all-workloads.csv](all-workloads.csv) records every row's medians, both MADs/spreads, ratio-deletion range, whole-process ratio, both RSS values and both combined residuals. Complete raw vectors,290-value sensitivities and29 workload deletions per round remain in recomputed.json. Whole-process GM29 is1.0174525850/1.0091124751. Candidate peak RSS ranges22,108–34,640KiB then21,908–34,652KiB; parent21,940–35,096KiB then22,212–34,484KiB. No arbitrary RSS veto is applied. Residual remains the combined startup/import/setup/warmup/reporting/teardown/GNU-time-wrapper cost, not isolated compilation time or proof of a specific regression cause.

All58 actual origins independently replay their precise module/variant/source/import/ELF/PID routes, including the deepcopy/logging variants omitted by the failed-v1 wrapper. Each origin is one unscored run(1), not registered-loop coverage for rows with other counts. Candidate spectral origin again proves automatic active/success_seen with its retained mapping witness; scored native frequency and global compilation counts remain unmeasured. All348 before/after quiet checks pass. Rehashed1,158 campaign artifacts and replayed current source/status/protected files, products/configs/tool aliases/hashes, external ELF and exact compiled-parent/prose-only source bridge. Before/after/post-cleanup/terminal inventories agree.

Root session46516 and campaign/gate/cleanup returned0. Additive13:50:50 root cleanup verifies16 available locks, no native processes, unchanged source/product/ELF bindings and18 preserved parent fingerprints before expiry. Campaign result SHA256 `90ba9b9f77884b8770e51cf2a5b43b444274ac91dccf02a456ae9850de69f446` is exact. Serial values remain correlated; sensitivity checks are not confidence intervals or cross-machine guarantees.

The failed broad-v1 run remains incomplete with zero scored values; the earlier67f5 rejection and limited599897 pilot acceptance remain unchanged at their scopes. This actual29-workload result supersedes neither record by rewriting it. It establishes no upstream parity, tracing-GC comparison, production landing or complete JIT correctness claim. No costly publication/full-Lib qualification is justified by this candidate's failed performance gate. No reviewer target/compiler/profiler, replacement sample or source edit occurred.
