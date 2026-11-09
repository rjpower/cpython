Reject source `11aefbdd6405953a0bf3605b3a9f703c01285bf6` under the predeclared all-29 performance rule. Do not progress this candidate to costly publication/full-library qualification. This is the completed fixed campaign, not an incomplete administrative attempt or a request for replacement samples.

Independent recomputation from the complete score vectors gives:

| Criterion | Round 0 | Round 1 |
|---|---:|---:|
| GM29 candidate/parent | 1.005208889 | 1.004548453 |
| Spectral norm candidate/parent | 0.940156896 | 0.955542129 |
| GM29 single-value deletion range | 1.004347–1.005880 | 1.004278–1.005532 |
| Worst spectral single-value deletion | 0.941577936 | 0.956480065 |

Both GM29 point gates fail, and every one of the 290 GM value-deletion ratios per round remains above 1. Spectral point and all ten value-deletion criteria pass. The spectral improvement cannot override the aggregate rule. These are descriptive serial-sample decisions, not significance or confidence claims.

Every repeated workload loss above 2% has this disposition: unresolved measured tradeoff, with no basis to dismiss it as layout noise or assign causal call counts. The candidate is rejected rather than accepted with those costs.

| Workload | Round 0 slower | Round 1 slower |
|---|---:|---:|
| richards | 2.563% | 2.746% |
| go | 3.500% | 2.388% |
| nbody | 3.061% | 5.303% |
| raytrace | 2.927% | 3.255% |
| logging_silent | 11.854% | 9.994% |

One-round losses above 2% are also retained and unresolved: deltablue (+1.832%/+5.516%), chaos (+4.870%/+1.192%), nqueens (+2.179%/-0.180%), comprehensions (-1.688%/+2.654%), logging_simple (+4.360%/-0.285%). Positive values mean slower. Full vectors, spreads, MADs, process time, residual and RSS per workload are retained in `workloads.csv`; correlated samples and uneven spreads limit causal interpretation.

Concentration does not rescue the result. Removing spectral norm gives diagnostic GM28 ratios 1.007614/1.006344; removing silent logging still gives 1.001381/1.001299. All 29 workload-deletion diagnostics exceed 1 in both rounds. These are diagnostic observations only, not additional vetoes.

Resources do not create a separate rejection: GM peak-RSS ratios are 0.993345/0.996526, with no workload exceeding +2% RSS in both rounds. Individual RSS ratios span 0.9392–1.0551 and 0.9332–1.1019. Whole-process GM ratios are 0.999410/1.002299; combined startup/import/setup/warmup/reporting/teardown residual ratios are 0.991396/0.999620. Neither replaces the scored warmed elapsed criterion; isolated compilation time and dynamic native frequency remain unmeasured. No RSS threshold or new statistical rule is imposed.

The earlier 599897 four-workload pilot was accepted only as a pilot, before its separate full-29 rejection. Its samples are not pooled, replaced or reinterpreted here. This admission candidate has no new four-workload pilot claim. Its smaller aggregate loss than that earlier campaign is a cross-campaign observation, not a matched ablation proving that entry filtering caused recovery. Static evidence that all 14 negative guards skip the full call and preserve inspected PC registers remains valid, but does not establish elapsed causality.

Root confirms session 89400 terminal exit 0 and cleanup at 14:52:49.277582 UTC: all 18 locks available, no native PIDs, both runtime bindings and 24 historical fingerprints unchanged. This disposition checks complete score vectors and material/resource tradeoffs; the qualifier separately audits all 580 raw values, 58 origins and provenance. No target, compiler, probe, source change or extra sample was run for this review.
