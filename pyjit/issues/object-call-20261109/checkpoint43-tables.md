# Checkpoint 43 tables

Historical measurements of source `47dc23f48cf1470d4a98137beb5d313b5093033d`. **API gate rejected; no full 43 suite; successor 44 in progress.** These tables do not qualify a later source revision. See [report](checkpoint43-report.md) and [independent evidence audit](final-cold-review43.md).

Evidence root `E`: `/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/object-call-20261109`. Ratios are candidate/control; below 1 means less elapsed time or counted work. Values are rounded to six decimals.

## All 29 elapsed cases

Two paired rounds per case, separately against frozen 39 (`4d666e1b482…`) and upstream (`3a440b65695…`). Sources: `E/measure43-all29-{golden,upstream}/summary.json`. This is a screening aggregate, not a waiver for regressions.

| Case | 43 / frozen 39 | 43 / upstream |
| --- | ---: | ---: |
| richards | 0.988279 | 1.281499 |
| go | 0.967199 | 1.184386 |
| nbody | 1.004906 | 0.859812 |
| float | 0.985026 | 0.795360 |
| spectral_norm | 1.066410 | 0.694370 |
| raytrace | 0.993342 | 0.984527 |
| deltablue | 0.983551 | 1.659254 |
| chaos | 1.003801 | 0.971804 |
| nqueens | 0.995255 | 1.195709 |
| fannkuch | 0.999925 | 1.235980 |
| generators | 1.006279 | 1.459339 |
| comprehensions | 1.057634 | 1.388715 |
| coroutines | 1.063140 | 1.339229 |
| unpack_sequence | 1.010850 | 1.156443 |
| deepcopy | 1.004105 | 1.371915 |
| deepcopy_reduce | 0.963915 | 1.376611 |
| deepcopy_memo | 1.005180 | 1.336223 |
| json_dumps | 1.003934 | 1.340933 |
| json_loads | 0.984086 | 1.223602 |
| pickle | 0.991060 | 1.091993 |
| unpickle | 1.037276 | 1.205596 |
| regex_compile | 0.986776 | 1.188939 |
| logging_format | 1.026262 | 1.453085 |
| logging_silent | 0.958613 | 0.864754 |
| logging_simple | 1.055744 | 1.400693 |
| xml_etree_generate | 1.006017 | 1.286375 |
| xml_etree_iterparse | 1.009149 | 1.283906 |
| xml_etree_parse | 0.988488 | 1.157398 |
| xml_etree_process | 1.000197 | 1.392433 |
| **Geometric mean (29)** | **1.004676** | **1.191545** |

## Six-pair elapsed follow-ups versus frozen 39

Sources: `E/measure43-basic-golden39` and `E/measure43-secondary-regressions-golden39`. Pair ranges show the spread, not confidence intervals.

| Case | Geometric mean | Pair minimum | Pair maximum | Slower pairs |
| --- | ---: | ---: | ---: | ---: |
| logging_silent | 0.959975 | 0.949314 | 0.969576 | 0/6 |
| coroutines | 1.067700 | 1.032593 | 1.089639 | 6/6 |
| deltablue | 0.993731 | 0.982368 | 1.006645 | 1/6 |
| generators | 1.000976 | 0.993111 | 1.008557 | 4/6 |
| nbody | 1.005488 | 0.992923 | 1.018734 | 4/6 |
| spectral_norm | 1.081747 | 1.056895 | 1.103310 | 6/6 |
| comprehensions | 1.055996 | 1.045373 | 1.063960 | 6/6 |
| logging_simple | 1.027095 | 0.953031 | 1.102142 | 5/6 |

## Long elapsed micros

Six pairs per comparison; unchanged workload bodies. Class load uses 20,000 outer loops and keyword call 2,000. Sources: `E/measure43-{class_attr_load,call_py_kw}-{golden,upstream}-long`.

| Case | 43 / frozen 39 | Pair range | 43 / upstream | Pair range |
| --- | ---: | --- | ---: | --- |
| class_attr_load | 0.156627 | 0.155704–0.157329 | 1.047186 | 0.973676–1.090316 |
| call_py_kw | 0.604334 | 0.596956–0.612614 | 1.070554 | 1.052664–1.091015 |

## Micro instructions per inner iteration

Source: `E/instructions43-micro/{upstream,golden,candidate}.json`. Four 200/600-loop differences per entry; 28 successful perf finishes per role, 100% event coverage. Raw is the median differenced estimate before skeleton subtraction; net subtracts that role’s empty-loop median. Neither is a direct opcode count.

| Case | Upstream raw / net | Frozen 39 raw / net | Candidate 43 raw / net |
| --- | ---: | ---: | ---: |
| empty_loop | 80.848 / — | 91.996 / — | 91.997 / — |
| class_attr_load | 144.752 / 63.904 | 657.188 / 565.191 | 168.999 / 77.002 |
| call_py_kw | 719.800 / 638.952 | 1429.001 / 1337.004 | 1081.000 / 989.003 |

| Case | 43 / 39 raw / net | 43 / upstream raw / net |
| --- | ---: | ---: |
| class_attr_load | 0.257155 / 0.136240 | 1.167510 / 1.204971 |
| call_py_kw | 0.756473 / 0.739715 | 1.501806 / 1.547851 |

## Coroutine whole-process instruction differences

Source: `E/instructions43-coroutines`. Four 4/12-loop differences, no skeleton subtraction; ten successful perf finishes per role, 100% event coverage.

| Role | Median instructions per loop |
| --- | ---: |
| Upstream | 294,894,391.625 |
| Frozen 39 | 408,259,942.250 |
| Candidate 43 | 408,017,082.688 |

43/39: **0.999405**; 43/upstream: **1.383604**. Near-equal work versus 39 does not establish a CPI, cache, branch-prediction or layout cause for the elapsed regression. No matched 39 cycles were captured in this packet.

## Hot-interval PMU ratios versus upstream

Source: `E/gap43-perf-stat`: 18/18 valid invocations, three rounds per role/case, 100% event coverage, approximately 1.966–2.022-second hot intervals. Ratios use per-role median counts per hot loop. The 2/1 role-order split is diagnostic, not a balanced elapsed campaign.

| Case | Instructions | Cycles | CPI ratio |
| --- | ---: | ---: | ---: |
| logging_silent | 1.036363 | 0.872322 | 0.841715 |
| coroutines | 1.378196 | 1.321449 | 0.958825 |
| deltablue | 1.870776 | 1.661810 | 0.888300 |

These counters and whole-process differences have different warmup and measurement boundaries; do not pool them. `E/gap43-perf-record` retained six complete, stable profiles. This table makes no candidate-43 sample-share attribution.
