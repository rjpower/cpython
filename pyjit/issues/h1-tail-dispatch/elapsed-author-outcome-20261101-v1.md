# VM0 elapsed author outcome

2026-11-01. **Reject C and T under the frozen gates; numeric selector G.**
This is the helper author’s outcome report, not independent acceptance. The
coordinator owns the independent decision. No new sample, target, compiler,
probe or workload was run for this report.

G is GCC computed, C Clang computed and T Clang tail; all are forks. There is
no upstream CPython arm. The runtime sources match outside `boil`; the observed
package differences are the declared compiler/backend policies, including their
effective flags. This pilot establishes no GC parity or isolated cause.

| Round | Workload | C/G elapsed | T/C elapsed | T/G elapsed |
| --- | --- | ---: | ---: | ---: |
| 0 | coroutines | 1.061794 | 1.019815 | 1.082834 |
| 0 | generators | 0.963954 | 0.962478 | 0.927785 |
| 0 | comprehensions | 1.080142 | 1.052277 | 1.136608 |
| 0 | richards | 0.925508 | 0.987071 | 0.913543 |
| 1 | coroutines | 1.056042 | 1.016109 | 1.073053 |
| 1 | generators | 0.936387 | 0.971544 | 0.909741 |
| 1 | comprehensions | 0.958758 | 1.143139 | 1.095994 |
| 1 | richards | 0.938785 | 1.004639 | 0.943139 |

| Round | Four-row GM C/G | Four-row GM T/C | Four-row GM T/G |
| --- | ---: | ---: | ---: |
| 0 | 1.005749 | 1.004842 | 1.010618 |
| 1 | 0.971299 | 1.031877 | 1.002261 |

C loses the coroutine primary by 6.18% and 5.60%; T loses against G by
8.28% and 7.31%, and against C by 1.98% and 1.61%. Every comparison fails the
required primary ratio <=0.99 in both rounds. T/C and T/G also have repeated
>2% comprehensions regressions. C/G’s second-round GM gain cannot erase its
primary loss. These are actual rejected gates, without a noise exception.

| Contrast / round | Primary benefit ns | Sum MADs ns | One-sided LOO ratio range |
| --- | ---: | ---: | ---: |
| C/G / 0 | -9629339 | 1955172 | 1.056140–1.065123 |
| C/G / 1 | -8996985 | 5410587 | 1.050150–1.084867 |
| T/C / 0 | -3278576 | 3707801 | 1.013534–1.025002 |
| T/C / 1 | -2731085 | 9577249 | 0.989110–1.034814 |
| T/G / 0 | -12907915 | 2404253 | 1.076165–1.088342 |
| T/G / 1 | -11728070 | 6492614 | 1.056498–1.092807 |

All primary benefits are negative; all MAD/LOO adequacy gates fail. MAD and
leave-one-out are descriptive checks on five correlated serial values, not
confidence intervals. No extra samples or more favorable subset are warranted.

| Round | Workload | Peak RSS KiB G / C / T | Spread % G / C / T |
| --- | --- | ---: | ---: |
| 0 | coroutines | 67040 / 69500 / 68744 | 2.786 / 3.441 / 4.043 |
| 0 | generators | 73628 / 71544 / 71536 | 21.506 / 26.121 / 28.560 |
| 0 | comprehensions | 86352 / 85520 / 85752 | 12.561 / 14.898 / 12.038 |
| 0 | richards | 30652 / 29952 / 30340 | 0.521 / 1.491 / 0.894 |
| 1 | coroutines | 66896 / 68884 / 68996 | 8.710 / 10.273 / 13.214 |
| 1 | generators | 70984 / 71388 / 71640 | 29.898 / 34.989 / 28.992 |
| 1 | comprehensions | 85928 / 85072 / 85188 | 12.160 / 12.311 / 12.499 |
| 1 | richards | 30572 / 30188 / 29976 | 1.501 / 0.290 / 0.233 |

The retained JSON includes every raw vector, normalization, median, spread,
RSS ratio, MAD and all ten one-sided leave-one-out values per primary pair.
RSS is whole-process GNU-time maximum RSS, including setup and warmup; lower
memory on a cell does not waive the elapsed gate. No arbitrary RSS veto was
added.

The read-only audit rehashed 265 terminal artifacts and current source,
products, compiler/tool and external ELF bindings; it reparsed all twelve raw
origin traces, all 120 scored values and the exact 36 command records. There
were 24 scored processes plus twelve one-loop zero-sample origin processes;
the latter provide workload/own-stdlib/own-extension identity, not additional
registered-loop timing samples. All 72 quiet checks passed, maximum load
0.682617<4, CPU0/sibling8 policy retained. Gate/inner return codes
are zero, products match before/after/terminal inventories, all three targets
were idle under their locks, and cleanup/evidence finished before the declared
deadline. Prior native feedback supplies semantic/root evidence; this campaign
does not add per-PID root-mode telemetry or dynamic dispatch-frequency counts.

The first author auditor failed by comparing the origin parser’s generic scope
text with the campaign’s deliberate witness-specific replacement. Its script
is preserved. The corrected reader asserts both exact labels separately and
compares every underlying Python/ELF/hash/status field; no campaign artifact,
runtime check or measured value changed.

Recommend retaining G under the pre-score selector, with no Clang upstream
build, full-29 backend rescue or automatic retiming. The original GC deadline
and matched GCC parity protocol remain. Root independent acceptance is still
required; performance, source correctness and later integration qualification
remain distinct decisions.

Evidence: `build/vm0-elapsed-author-outcome-20261101-v1/outcome.json` SHA256
`0afb84a360aa2d7ff1a72513d7a5f5f5425641e67a59d3010a990f8ba1bea45f`; author receipt
`ae1c60ba186b9e5b265b41e5ab3d71c01e36f99bd08da3618b362fa8ef6b3cd2`. Original completed campaign:
`build/vm0-nonstats-elapsed-v1`; source/plan/native bindings remain immutable.
