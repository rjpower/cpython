# Round 50 diagnostic tables

## Selected checkpoint: all 29 kernels

Selected `bf810ec1a33` compared with golden39, upstream and frozen48, two
balanced pairs per unchanged kernel. Entries are elapsed geometric means;
lower is better. Source: `final50-all29-{golden39,upstream,frozen48}/summary.json`
in the evidence directory. Six-pair target confirmations appear below.

| Kernel | / golden39 | / upstream | / frozen48 |
| --- | ---: | ---: | ---: |
| richards | 0.998573 | 1.285004 | 1.004109 |
| go | 0.993990 | 1.210108 | 1.018467 |
| nbody | 1.020623 | 0.864580 | 0.986658 |
| float | 0.986886 | 0.791350 | 0.975023 |
| spectral_norm | 1.369933 | 0.875053 | 1.106127 |
| raytrace | 0.993105 | 0.962557 | 0.973961 |
| deltablue | 0.681829 | 1.137799 | 0.920804 |
| chaos | 0.999344 | 0.959003 | 0.993376 |
| nqueens | 0.995633 | 1.204931 | 0.966673 |
| fannkuch | 1.019665 | 1.254286 | 1.010283 |
| generators | 0.984703 | 1.423010 | 0.971380 |
| comprehensions | 1.014104 | 1.363579 | 0.975387 |
| coroutines | 1.116468 | 1.396957 | 1.017817 |
| unpack_sequence | 1.008546 | 1.149433 | 0.993993 |
| deepcopy | 1.027002 | 1.380219 | 1.005653 |
| deepcopy_reduce | 1.002881 | 1.377472 | 1.006309 |
| deepcopy_memo | 1.012277 | 1.319099 | 1.034347 |
| json_dumps | 0.966531 | 1.335399 | 1.009285 |
| json_loads | 0.959954 | 1.228219 | 1.002495 |
| pickle | 0.985810 | 1.085085 | 0.992565 |
| unpickle | 0.992155 | 1.201392 | 1.005149 |
| regex_compile | 1.008322 | 1.190321 | 1.013903 |
| logging_format | 1.010940 | 1.308421 | 0.992477 |
| logging_silent | 0.878900 | 0.812169 | 1.042419 |
| logging_simple | 1.016661 | 1.310996 | 0.952996 |
| xml_etree_generate | 1.000005 | 1.284258 | 0.998252 |
| xml_etree_iterparse | 0.999475 | 1.309950 | 0.983963 |
| xml_etree_parse | 0.987418 | 1.146833 | 0.994776 |
| xml_etree_process | 1.006504 | 1.430296 | 1.006513 |
| **29-kernel geometric mean** | **0.996588** | **1.177045** | **0.997971** |

## Selected checkpoint: six-pair targets

| Workload | / golden39 | / upstream | / frozen48 | / frozen A50 |
| --- | ---: | ---: | ---: | ---: |
| DeltaBlue | 0.685934 | 1.123299 | 0.927374 | 0.995403 |
| Logging silent | 0.895185 | 0.804557 | 1.043134 | 1.074052 |
| nbody | 0.999513 | — | 0.970471 | 1.015991 |
| spectral_norm | 1.370828 | — | — | — |
| coroutines | 1.110488 | — | — | — |
| logging_format | 1.034296 | — | — | — |
| logging_simple | 0.995530 | — | — | — |

Sources: `final50-targets-{golden39,upstream,frozen48}/summary.json` and
`final50-canonical50a/summary.json`. A dash means that campaign did not run
that target. Different campaigns must not be multiplied to infer another ratio.

## Final PMU diagnostics

`final50-perf-stat/normalized-summary.json` gives per-iteration means from
matched intervals. These are diagnostics rather than balanced timing admission.

| Workload | Metric | Upstream | Selected | Selected / upstream |
| --- | --- | ---: | ---: | ---: |
| DeltaBlue | Instructions | 38,928,146 | 52,673,868 | 1.353105 |
| DeltaBlue | User cycles | 15,366,174 | 17,358,250 | 1.129640 |
| Logging silent | Instructions | 13,943.98 | 13,351.03 | 0.957476 |
| Logging silent | User cycles | 4,752.67 | 3,800.85 | 0.799730 |

The direct `final50-canonicalA-stat/normalized-summary.json` comparison has
12 valid intervals and stable identities:

| Workload | Instructions / A | Cycles / A | Elapsed / A |
| --- | ---: | ---: | ---: |
| DeltaBlue | 0.999984765 | 0.999888 | 0.999650 |
| Logging silent | 1.000000052 | 1.085878 | 1.074645 |

The logging cycle increase has no established cause. The matched cycle-record
campaign has four valid captures with zero lost samples.

The `baseline50-perf-stat/normalized-summary.json` artifact under
`BASE/pyjit-link-wave1/deltablue-call-model-20261110` records the means of three
intervals per role. BASE is `/home/power/code/cpython/.worktrees/python-boil-the-ocean/build`.
Candidate here means selected48 `5559f60093e`, not experimental A50. Upstream
is `3a440b65695`. Values are per unchanged iteration; elapsed times are
profiling diagnostics, not balanced admission timings.

| Workload | Metric | Upstream | Selected48 | Selected48 / upstream |
| --- | --- | ---: | ---: | ---: |
| DeltaBlue | Native instructions | 38,936,924 | 55,343,778 | 1.42137 |
| DeltaBlue | User cycles | 15,868,451 | 18,680,197 | 1.17719 |
| DeltaBlue | Elapsed ms | 3.74456 | 4.42345 | 1.18130 |
| Logging silent | Native instructions | 13,943.98 | 14,451.03 | 1.03636 |
| Logging silent | User cycles | 4,739.16 | 3,513.77 | 0.74143 |
| Logging silent | Elapsed µs | 1.12092 | 0.84111 | 0.75037 |

The matched `baseline50-perf-record` campaign contains cycle captures and
native annotations for both workloads and roles. Identity checks passed and
all four captures report zero lost samples. `native50a` contains before/after
method and CALL disassembly for the candidate. `delta50a-*-checks.json` and
associated logs record regeneration, builds and focused checks. These files are
under the same evidence directory.

## A50 against frozen48

Two balanced pairs in `measure50a-call-pair48/summary.json` compare candidate
`e014965013b` with frozen48 `5559f60093e`; identities were stable. Ratios are
elapsed candidate/control, so lower is better. These are component measurements,
not an upstream comparison or a final batch admission.

| Workload | Pair 1 | Pair 2 | Geometric mean |
| --- | ---: | ---: | ---: |
| DeltaBlue | 0.908322 | 0.937216 | 0.922656 |
| Logging silent | 0.980452 | 0.973443 | 0.976941 |
| nbody | 0.969363 | 0.973507 | 0.971432 |

## A50 against golden39: all 29 kernels

Frozen A `e014965013b` versus golden39 `4d666e1b482`, two balanced pairs per
unchanged kernel. Ratios are elapsed candidate/control geometric means; lower
is better. This provisional screen predates the C follow-up and cold recovery
change. Its overall geometric mean is **0.998194**; it masks individual losses.
The source is `measure50a-all29-golden39/summary.json` in the evidence directory.

| Kernel | A / golden39 |
| --- | ---: |
| richards | 0.989339 |
| go | 0.994916 |
| nbody | 0.985488 |
| float | 0.993214 |
| spectral_norm | 1.283569 |
| raytrace | 0.992288 |
| deltablue | 0.680512 |
| chaos | 0.997839 |
| nqueens | 1.008228 |
| fannkuch | 1.028526 |
| generators | 0.992750 |
| comprehensions | 1.025328 |
| coroutines | 1.088057 |
| unpack_sequence | 1.010607 |
| deepcopy | 1.016894 |
| deepcopy_reduce | 1.006861 |
| deepcopy_memo | 0.988366 |
| json_dumps | 1.011976 |
| json_loads | 0.991782 |
| pickle | 1.010353 |
| unpickle | 0.992701 |
| regex_compile | 1.001209 |
| logging_format | 1.050384 |
| logging_silent | 0.827462 |
| logging_simple | 1.054447 |
| xml_etree_generate | 1.013700 |
| xml_etree_iterparse | 1.008011 |
| xml_etree_parse | 1.008231 |
| xml_etree_process | 1.018868 |

## C50 comparisons

Clean six-pair campaigns compare `3e96bd9311e` directly with frozen48 and
with frozen A50. Values are elapsed candidate/control geometric means; lower is
better. The original `measure50c-call-pair48` overlapped debug testing and is
excluded from admission.

| Workload | C / frozen48 | C / A50 | C / A50 pair range |
| --- | ---: | ---: | ---: |
| DeltaBlue | 0.964298 | 1.043648 | 1.031816–1.054530 |
| Logging silent | 0.949135 | 0.968599 | 0.926884–0.987764 |
| nbody | 1.011550 | 1.038921 | 1.033430–1.051407 |

Sources are `measure50c-clean-call-pair48/summary.json` and
`measure50c-canonical50a/summary.json` in the evidence directory. Separately,
`coverage50-adjacent.json` records 42,710 refined entries and 40,629 committed
activations after 30 warmup iterations and one counted iteration. Coverage is
not measured during the timing campaign.

The final direct-overlap candidate `f267e9209cf` was also compared directly
with frozen A in `measure50c5-canonical50a/summary.json`, six balanced pairs:

| Workload | C follow-up / A50 | Pair range |
| --- | ---: | ---: |
| DeltaBlue | 1.028054 | 0.995028–1.039992 |
| Logging silent | 1.140367 | 1.130504–1.150701 |
| nbody | 1.008522 | 0.974979–1.023302 |

Five of six DeltaBlue and nbody pairs and all six logging pairs were slower.
The follow-up was rejected. These results do not measure selected `bf810ec1a33`.
