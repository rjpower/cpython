Independent decision: REJECT_FIXED_ALL29_ELAPSED_GATES for admission source11aefbdd versus the qualified eba351 RC release image. The completed campaign is valid; its frozen performance gates reject further progression of this candidate.

All580 raw values from116 scored processes independently normalize to the retained loop-count-adjusted medians, min/max, spreads and MADs. Independent arithmetic reproduces the entire score, all10 primary and290 aggregate single-value deletions per round, and all29 workload-deletion diagnostics. No earlier pilot or rejected campaign values enter this score.

| Candidate / parent | Round0 | Round1 |
| --- | ---: | ---: |
| spectral_norm primary | 0.9401568964 | 0.9555421288 |
| GM29 | 1.0052088894 | 1.0045484533 |
| Primary10-deletion range | 0.939223347–0.941577936 | 0.953965066–0.956480065 |
| GM29 290-deletion range | 1.004347167–1.005880200 | 1.004277872–1.005532248 |
| GM28 excluding spectral, diagnostic | 1.0076136317 | 1.0063444159 |
| All GM28 diagnostic range | 1.001380926–1.007613632 | 1.001299039–1.006344416 |
| Improved / regressed workloads | 14 / 15 | 16 / 13 |

The primary improves, but GM29 regresses0.52%/0.45%, and every mandatory aggregate sensitivity result remains above1. This fails the declared strict GM29<1 requirement in both rounds. GM28 remains diagnostic only and introduces no additional veto. No individual regression disposition, replacement samples, smaller workload set or switch to another timing metric can override this fixed failure.

The full >2% union is10 workloads: Richards, go, nbody, raytrace, deltablue, chaos, nqueens, comprehensions, logging_silent and logging_simple. Five exceed2% in both rounds: Richards2.56%/2.75%, go3.50%/2.39%, nbody3.06%/5.30%, raytrace2.93%/3.26% and logging_silent11.85%/9.99%. Every union member's two actual ratios and ten-value ratio sensitivity ranges appear in [material-losses.md](material-losses.md); all workload/arm MADs, spreads, medians, whole-process ratios, RSS and residuals are in [all-workloads.csv](all-workloads.csv). These costs are retained, not dismissed as noise. The aggregate rejection needs no per-row waiver to advance.

Whole-process GM29 is0.9994104313/1.0022987696, a separate diagnostic that does not replace the registered-loop score. Candidate peak RSS ranges22,204–34,520KiB and22,236–34,564KiB; parent22,328–34,684KiB and22,028–34,820KiB. No arbitrary RSS veto is applied. Combined residual remains startup/import/setup/warmup/reporting/teardown/GNU-time-wrapper cost; it does not isolate compilation or establish a causal explanation for the regressions. Serial values remain correlated; leave-one checks are not confidence intervals.

All58 actual unscored origins independently replay their exact source/module/variant/import/ELF/PID routes, including deepcopy/logging variants. Each performs one run(1), not registered-loop coverage where loops differ. Candidate spectral origin retains the automatic active/success_seen and executable-map witness at that limited scope; scored native frequency and global compilation counts remain unmeasured. All348 quiet checks pass. Rehashed1,165 campaign artifacts and replayed current source/status/protected files, configurations/products/tool aliases/hashes/external ELF and the actual compiled-parent/prose-only MAINe7d4 bridge. Before/after/post-cleanup/terminal product inventories agree.

Root session89400 terminal0 and additive14:52:49 cleanup verify18 available locks, no native processes, unchanged source/product/ELF bindings and24 preserved parent fingerprints, before expiry. The actual `twenty_four_parent_fingerprints_unchanged` field was used; no old count/schema was assumed. Result SHA256 `5959f86ecd9d88fd316f1e7ce65a7c1f0fa55afe4e8de7859551c489d0a24540` is exact. Native safety/emission acceptance remains at its existing focused scope and does not alter this performance decision.

All prior failures and pilot decisions remain immutable. This result does not establish upstream parity, tracing-GC performance, production landing or a complete JIT. It does not justify expensive publication/full-Lib qualification of this candidate under the declared performance-first sequence. No reviewer target/compiler/profiler, source edit, adaptive rerun or replacement sample occurred.
