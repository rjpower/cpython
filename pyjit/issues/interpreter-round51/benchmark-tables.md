# Round 51 benchmark tables

Ratios are candidate/control elapsed time; lower is
faster. Unchanged kernels run with GCC 15.2, CPU 15, hash 0, JIT disabled and
balanced ordering. Source and binary identities must be recorded for each
completed campaign before interpreting its results.

The selected runtime is `0e5e61f4c6b`. Component experiments use a rebuilt
`bf810ec1a33` control; final comparisons use the actual prior selected image.
Combinations are measured directly. The hot
set comprises DeltaBlue, Richards, comprehensions, generators, coroutines and
spectral norm, with silent logging as a mandatory guard. Two pairs screen; six
pairs confirm claimed improvements and suspicious regressions.

## Two-pair screen against bf810

| Candidate | DeltaBlue | Richards | Comprehensions | Generators | Coroutines | Spectral norm | Logging silent |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 1 | 1.009829 | 1.021337 | 0.978406 | 1.000136 | 0.999195 | 0.999788 | 1.009425 |
| 2 | 0.977526 | 0.981836 | 0.954573 | 1.030544 | 0.985250 | 0.990451 | 1.008897 |
| 3 | 1.008292 | 0.989114 | 0.966134 | 1.012850 | 0.995017 | 0.903312 | 0.846526 |
| 4 | 1.013111 | 0.989621 | 0.969971 | 1.015538 | 0.960153 | 0.870885 | 0.980958 |
| 5 | 1.025503 | 0.994579 | 0.991308 | 1.008276 | 0.961508 | 0.904456 | 0.994221 |

These are screen ratios. Option 1 failed the breadth gate; option 5 was
discarded after confirmation. The direct composition trials below select only
3+4 and reject the iteration addition.

## Six-pair confirmation against bf810

| Candidate | DeltaBlue | Comprehensions | Generators | Coroutines | Spectral norm | Logging silent |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 2 | 0.989596 | 0.960253 | 1.022307 | — | — | 1.010509 |
| 3 | 1.022158 | 0.974249 | 1.021656 | — | 0.892686 | 0.852953 |
| 4 | 1.028112 | — | 1.007676 | 0.950130 | 0.864861 | 0.975341 |
| 5 | 1.051695 | — | 1.008468 | 0.958486 | 0.900842 | 0.990015 |

A dash denotes a case not included in that confirmation. Composition was
measured directly.

## Combined 3+4 against prior selected bf810, six pairs

| Case | Geometric mean | Pair range |
| --- | ---: | ---: |
| deltablue | 1.036881 | 1.006033–1.056974 |
| richards | 0.966903 | 0.960411–0.974205 |
| comprehensions | 1.000125 | 0.983945–1.012699 |
| generators | 1.012945 | 1.003293–1.021219 |
| coroutines | 0.893073 | 0.879814–0.908275 |
| spectral_norm | 0.897685 | 0.895534–0.899040 |
| logging_silent | 0.891657 | 0.827254–1.038904 |

Logging was slower in one of six pairs; this spread remains an explicit tradeoff.

## Trial 2+3+4 against 3+4, six pairs

| Case | Geometric mean | Pair range |
| --- | ---: | ---: |
| deltablue | 1.049535 | 1.023319–1.066904 |
| richards | 1.016375 | 1.008738–1.021249 |
| comprehensions | 0.972975 | 0.969018–0.980819 |
| generators | 1.009343 | 1.005354–1.014457 |
| coroutines | 1.041391 | 1.032486–1.047486 |
| spectral_norm | 1.007036 | 1.004855–1.009039 |
| logging_silent | 1.018455 | 0.993152–1.131837 |

This is the incremental comparison, not a comparison with the prior selected runtime.

## Final candidate: all 29 kernels, two pairs

| Case | / prior50 | / upstream | / golden39 |
| --- | ---: | ---: | ---: |
| richards | 0.957596 | 1.212473 | 0.973428 |
| go | 0.954835 | 1.165580 | 0.948328 |
| nbody | 1.006869 | 0.860843 | 1.001892 |
| float | 0.984417 | 0.777540 | 0.973367 |
| spectral_norm | 0.899051 | 0.779487 | 1.245339 |
| raytrace | 0.979939 | 0.946163 | 0.976123 |
| deltablue | 1.044924 | 1.158076 | 0.705840 |
| chaos | 0.986427 | 0.952459 | 0.968306 |
| nqueens | 0.989169 | 1.191039 | 0.992036 |
| fannkuch | 1.006078 | 1.273581 | 1.008692 |
| generators | 1.013071 | 1.451075 | 1.000564 |
| comprehensions | 0.990767 | 1.364519 | 1.023417 |
| coroutines | 0.896925 | 1.264618 | 1.001160 |
| unpack_sequence | 1.007413 | 1.166191 | 0.995963 |
| deepcopy | 0.996355 | 1.388390 | 1.011436 |
| deepcopy_reduce | 0.995554 | 1.373031 | 1.011762 |
| deepcopy_memo | 1.026955 | 1.377822 | 1.028947 |
| json_dumps | 0.999706 | 1.334742 | 0.982847 |
| json_loads | 1.001197 | 1.207826 | 0.990977 |
| pickle | 0.987861 | 1.118151 | 0.987365 |
| unpickle | 1.005062 | 1.187558 | 0.995637 |
| regex_compile | 0.993004 | 1.172782 | 0.998487 |
| logging_format | 0.998155 | 1.280554 | 1.008193 |
| logging_silent | 0.889552 | 0.719533 | 0.801428 |
| logging_simple | 0.999677 | 1.267318 | 0.981052 |
| xml_etree_generate | 1.003437 | 1.257046 | 0.990074 |
| xml_etree_iterparse | 1.009739 | 1.299373 | 1.007398 |
| xml_etree_parse | 0.990048 | 1.140769 | 0.997827 |
| xml_etree_process | 0.990551 | 1.387593 | 0.997137 |
| Geometric mean | 0.985700 | 1.156535 | 0.982829 |

## Final candidate against upstream, six pairs

| Case | Geometric mean | Pair range |
| --- | ---: | ---: |
| deltablue | 1.159832 | 1.088204–1.199605 |
| generators | 1.445513 | 1.433223–1.465323 |
| coroutines | 1.267666 | 1.243260–1.333087 |
| logging_silent | 0.775668 | 0.754934–0.791214 |

The two-pair all-29 screen is a guardrail, not grounds to dismiss a material
regression. Six-pair confirmations retain DeltaBlue, generator and deepcopy
memo losses. See the [report](report.md) for interpretation and qualification.

## PMU diagnostics against upstream

Three paired post-warmup intervals per case; ratios are counts per loop. All
24 event intervals ran at 100%.

| Case | Instructions | User cycles |
| --- | ---: | ---: |
| deltablue | 1.352487 | 1.173937 |
| generators | 1.414550 | 1.443516 |
| coroutines | 1.388163 | 1.249676 |
| logging_silent | 0.953890 | 0.730284 |

Logging cycle ratios ranged from 0.657228 to 0.775115.

## Deepcopy memo against prior50, six pairs

| Case | Geometric mean | Pair range |
| --- | ---: | ---: |
| deepcopy_memo | 1.019933 | 0.990105–1.033859 |

Five of six pairs were slower.
