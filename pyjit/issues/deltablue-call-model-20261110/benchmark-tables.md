# DeltaBlue checkpoint benchmark tables

Lower elapsed ratio is better. All runs use unchanged kernels, inputs and loop
counts, GCC 15.2, CPU 15, hash 0, JIT disabled, and stable source/native identities.
Target confirmations use six balanced pairs; full29 screens use two per kernel.
Selected runtime 48 is `5559f60093e`; the starting checkpoint is 46 `b8e7e1bee66`.

## Six-pair confirmations

These are separate matched campaigns against each named control. Ranges are the
smallest and largest paired ratios, not confidence intervals. Every DeltaBlue
pair is faster than 46 and slower than upstream; every logging pair beats both.

| Case | / starting 46 | Pair range | / upstream | Pair range |
| --- | ---: | ---: | ---: | ---: |
| deltablue | 0.739181 | 0.728694–0.747892 | 1.225425 | 1.205604–1.234262 |
| logging_silent | 0.872129 | 0.846084–0.890561 | 0.758002 | 0.732618–0.773868 |

## Complete upstream screens

Selected 48 and rejected 49 are separate upstream campaigns. Their quotient is
not a direct component measurement. The initial48 screen remains in the
[classmethod integration record](integration48.md); the following selected
column uses the final restored build.

| Kernel | Selected 48/upstream | Rejected 49/upstream |
| --- | ---: | ---: |
| richards | 1.259954 | 1.264448 |
| go | 1.193123 | 1.193036 |
| nbody | 0.856192 | 0.832492 |
| float | 0.820246 | 0.777705 |
| spectral_norm | 0.793785 | 0.813811 |
| raytrace | 0.997861 | 1.106181 |
| deltablue | 1.219238 | 1.407207 |
| chaos | 0.958700 | 0.978827 |
| nqueens | 1.228902 | 1.209582 |
| fannkuch | 1.267697 | 1.242260 |
| generators | 1.440719 | 1.417844 |
| comprehensions | 1.384779 | 1.407938 |
| coroutines | 1.380326 | 1.365337 |
| unpack_sequence | 1.170939 | 1.170391 |
| deepcopy | 1.362120 | 1.328143 |
| deepcopy_reduce | 1.368778 | 1.364212 |
| deepcopy_memo | 1.324181 | 1.290434 |
| json_dumps | 1.347630 | 1.342856 |
| json_loads | 1.199333 | 1.197499 |
| pickle | 1.088824 | 1.106974 |
| unpickle | 1.207914 | 1.196364 |
| regex_compile | 1.207268 | 1.207394 |
| logging_format | 1.359994 | 1.383446 |
| logging_silent | 0.774431 | 0.944893 |
| logging_simple | 1.322987 | 1.324742 |
| xml_etree_generate | 1.283143 | 1.302422 |
| xml_etree_iterparse | 1.303432 | 1.304686 |
| xml_etree_parse | 1.152650 | 1.169694 |
| xml_etree_process | 1.433643 | 1.468572 |
| **Geometric mean** | **1.179430** | **1.195270** |

## Direct structural experiment

Each column is a separate two-pair comparison against frozen 48. The gate-off
variant retains shared ordinary probes and certificate construction. Do not
subtract separate campaigns to claim an exact causal cost.

| Case | Rejected 49/frozen 48 | Gate-off/frozen 48 |
| --- | ---: | ---: |
| deltablue | 1.152129 | 1.019543 |
| logging_silent | 1.212813 | 1.089607 |
| nbody | 0.997054 | 1.000250 |

## Evidence

`BASE/pyjit-link-wave1/deltablue-call-model-20261110` contains
`final48-all29-upstream`, `final48-targets-baseline46`, `final48-targets-upstream`,
`measure49-all29-upstream`, `measure49-call-pair48` and
`measure49-off-call-pair48`. Each retains command/input journals, paired values,
module evidence and stable identity snapshots. `final48-qualification.json`
checks the selected source, diagnostics, campaigns and matched profile capture.
[Report](checkpoint-report.md), [plan](plan.md) and [HANDOFF](../../HANDOFF.md)
explain the selection, exact paths and qualification limits.
