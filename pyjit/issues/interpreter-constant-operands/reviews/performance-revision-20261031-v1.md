# Revised held performance decision review, 2026-10-31 v1

Disposition: **ACCEPT the held decision revision**, SHA256 `881f9e1052fac89956d8f481e78de760a36c9c2724c7a7bc3093c13e2f67bf2f`, in `performance-revision-20261031.md`. No timing execution is authorized by this review.

The revision explicitly follows the user's steering before timing scores exist. Evaluator text growth and instruction counts become explanatory diagnostics, with no automatic rejection or rescue rule. The valid matched 18.4248% text measurement and original threshold-based receipt remain historical evidence; the current performance decision is open. No new size threshold is substituted.

Actual elapsed performance now decides acceptance: coroutines must improve its median by at least 1% in each round, with variability small enough to support that claim. Spectral norm cannot rescue a primary failure. A control regression above 2% requires investigation before acceptance; noisy or inconclusive evidence does not accept and does not automatically authorize more samples. Full vectors and ranges remain necessary; five serial elapsed samples do not establish an independent-sample confidence interval by themselves.

The revised rule retains the original four registered workloads, fixed work, two rotating-order rounds, four instruction pairs and five elapsed samples per cell. That remains 64 low/high instruction pairs and 80 elapsed samples across sixteen workload/build/round cells, plus the already specified probes/warmups. It adds no workload or repetition after scores. The 45-minute quiet window, matching source/product/configuration and OFF/N0 child policy, correct origin/build locks, load/CPU/sibling admission and cleanup remain required.

The separately reviewed runner-level preservation of original low/high perf records is still a prerequisite: current normalized differences alone cannot earn raw-pair credit. Exact sources/images/commands, a finite window and noise handling must be frozen before activation. Semantic and actual word/native-work-removal review remain prerequisites; the failed PIE extraction has not supplied those words. Optional cache/branch counters are appropriately deferred to a separately bounded diagnosis rather than made new measurement requirements.

This revision neither accepts the runtime increment nor replaces coherent qualification after a successful performance decision. No timing score, target command or new measurement ran during this review. The frozen earlier plan/reviews/receipts are preserved rather than rewritten.
