# Checkpoint 46 elapsed tables

**Measurement snapshot only; promotion is determined in the coordinator’s report.** Ratios are candidate/control elapsed time, so below 1 is faster. Each paired ratio divides the candidate’s seven-sample arithmetic mean by its paired control mean; case results are geometric means of those ratios. Ranges are observed paired minima/maxima, not confidence intervals.

| Role | Clean source identity |
| --- | --- |
| candidate | `b8e7e1bee6611eeda320ce163cd3ce89323da877` |
| golden | `4d666e1b482cb4983c7683f91c5922af14d0ab40` |
| upstream | `3a440b65695615e9fd9912342b79cdd54fced0d2` |

Evidence root `E`: `/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/object-call-20261109`. These are independent paired campaigns; do not multiply their ratios to infer another comparison. Micros are separate from the 29 applications and their aggregate.

## Evidence checks

All seven campaigns have equal before/after identity snapshots, clean expected source revisions, empty identity errors, `stable_identity: true`, and null failure/postflight error. Host-side recomputation matched all raw stdout metadata/samples, journal means, paired ratios, geometric means and ranges. All timed invocations returned zero, used CPU 15, JIT off and hash randomization off; each pair used equal fixed loops and roles alternated evenly.

| Campaign under E | Cases | Pairs per case | Successful timed invocations |
| --- | ---: | ---: | ---: |
| `measure46-all29-golden` | 29 | 2 | 116 |
| `measure46-all29-upstream` | 29 | 2 | 116 |
| `measure46-regressions-golden39` | 8 | 6 | 96 |
| `measure46-class_attr_load-golden-long` | 1 | 6 | 12 |
| `measure46-class_attr_load-upstream-long` | 1 | 6 | 12 |
| `measure46-call_py_kw-golden-long` | 1 | 6 | 12 |
| `measure46-call_py_kw-upstream-long` | 1 | 6 | 12 |

No invalid timed data or identity drift was found. The keyword/upstream journal also includes one `quiet-wait` load record; it is not a timed invocation or sample and is excluded from the counts. These checks validate the retained elapsed packet, not source/API or full-suite qualification.

## All 29 applications

Two pairs per case against each control. Fixed loops below apply to both independent comparisons. Sources: `E/measure46-all29-{golden,upstream}/{commands,summary}.json`.

| Case | Fixed loops | 46 / frozen 39 | 46 / upstream |
| --- | ---: | ---: | ---: |
| richards | 2 | 0.986243 | 1.279206 |
| go | 1 | 0.975997 | 1.201597 |
| nbody | 1 | 0.998795 | 0.844284 |
| float | 1 | 0.998415 | 0.776443 |
| spectral_norm | 1 | 1.074240 | 0.699677 |
| raytrace | 1 | 1.007806 | 0.986151 |
| deltablue | 30 | 0.992910 | 1.632329 |
| chaos | 2 | 0.996387 | 0.963282 |
| nqueens | 1 | 0.995127 | 1.209858 |
| fannkuch | 1 | 1.027719 | 1.260110 |
| generators | 2 | 0.999182 | 1.429352 |
| comprehensions | 6,000 | 1.097698 | 1.443802 |
| coroutines | 4 | 1.054585 | 1.345239 |
| unpack_sequence | 4,000 | 1.000211 | 1.071506 |
| deepcopy | 300 | 0.971288 | 1.377903 |
| deepcopy_reduce | 20,000 | 0.986321 | 1.377533 |
| deepcopy_memo | 4,000 | 1.008337 | 1.344590 |
| json_dumps | 10 | 0.992082 | 1.321851 |
| json_loads | 200 | 0.993941 | 1.194448 |
| pickle | 400 | 1.002225 | 1.097465 |
| unpickle | 300 | 1.000386 | 1.198311 |
| regex_compile | 1 | 1.016194 | 1.206131 |
| logging_format | 1,000 | 0.963919 | 1.277231 |
| logging_silent | 60,000 | 0.964722 | 0.888067 |
| logging_simple | 1,000 | 0.995714 | 1.317541 |
| xml_etree_generate | 1 | 0.999232 | 1.271011 |
| xml_etree_iterparse | 1 | 1.017912 | 1.278397 |
| xml_etree_parse | 1 | 0.998008 | 1.157827 |
| xml_etree_process | 1 | 1.006553 | 1.404637 |
| **Geometric mean, 29 cases** | — | **1.003819** | **1.181202** |

## Eight-case follow-up versus frozen 39

Six pairs per case, separately measured from the all-29 screen. Source: `E/measure46-regressions-golden39`.

| Case | Fixed loops | Geometric mean | Pair minimum | Pair maximum | Slower pairs |
| --- | ---: | ---: | ---: | ---: | ---: |
| logging_silent | 60,000 | 0.973238 | 0.964980 | 0.978156 | 0/6 |
| coroutines | 4 | 1.057772 | 1.048273 | 1.068856 | 6/6 |
| deltablue | 30 | 0.985848 | 0.977600 | 0.995200 | 0/6 |
| generators | 2 | 1.011833 | 0.996753 | 1.025312 | 4/6 |
| nbody | 1 | 1.010290 | 0.993485 | 1.040752 | 4/6 |
| spectral_norm | 1 | 1.081081 | 1.046814 | 1.100653 | 6/6 |
| comprehensions | 6,000 | 1.094809 | 1.084529 | 1.106845 | 6/6 |
| logging_simple | 1,000 | 0.957587 | 0.900102 | 1.007994 | 1/6 |
| **Geometric mean, these 8 cases** | — | **1.020455** | — | — | — |

The eight-case aggregate covers this selected roster only; it is not a replacement all-29 estimate.

## Long micros

Six pairs per case/control. Workload bodies are unchanged, with fixed outer loops as shown. Sources: `E/measure46-{class_attr_load,call_py_kw}-{golden,upstream}-long`. Each row is a direct measured comparison; no upstream ratio is inferred through frozen 39.

| Case | Control | Fixed loops | Geometric mean | Pair minimum | Pair maximum |
| --- | --- | ---: | ---: | ---: | ---: |
| class_attr_load | frozen 39 | 20,000 | 0.160125 | 0.155339 | 0.179835 |
| class_attr_load | upstream | 20,000 | 1.004341 | 0.969803 | 1.080403 |
| call_py_kw | frozen 39 | 2,000 | 0.609568 | 0.599918 | 0.619055 |
| call_py_kw | upstream | 2,000 | 1.052195 | 1.020751 | 1.070182 |

The class micro exercises one repeated class integer attribute; it does not establish broad lookup coverage. Keep these micro results separate from application admission and from earlier candidate 43 instruction/PMU campaigns.
