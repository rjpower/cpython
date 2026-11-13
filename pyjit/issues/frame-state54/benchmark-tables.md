# Round54 benchmark tables

Completed campaign evidence. The coordinator rejects the performance candidates;
only cold correctness checkpoint54 is selected, with explicit diagnostic limits
and no performance lift. Ratios in the experimental sections are
candidate elapsed / golden51 elapsed: **lower is better**. Golden51 is
`0e5e61f4c6bc820f2dd1a5f9ee6f7f1768834254`. Do not multiply ratios across
campaigns or pool their samples. Tables round to six decimals; JSON retains
full precision.

E=`/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/frame-state54`.
All listed completed campaigns report stable before/after identity and no
campaign errors. Commands and raw per-process samples are beside each summary.
The unchanged workload driver uses seven timed observations per process,
compares their arithmetic means within each balanced pair, then reports the
geometric mean of pair ratios. GCC15.2, CPU15, hash0, JIT disabled.

## Candidate identities and sample scope

| Label | Candidate head | Completed scope used below |
| --- | --- | --- |
| F, feedback | `7ea40d5d67c` | all29 two-pair screen; hot six-pair confirmation |
| R, recipient | `612fb4e9d17` | all29 two-pair screen; hot six-pair confirmation |
| N, namespaces | `c2cc6777e4d` | hot six-pair confirmation |
| L, mapping | `b48fe2d3989` | initial hot two-pair screen; separate mapping six-pair diagnostic |
| S, initial overwrite | `4fbd16a1462` | initial hot two-pair screen; separate native census |
| S2, validated overwrite | `e95328ba1d6` | hot two-pair screen and six-pair confirmation; all29 two-pair screen; separate native census |
| R2, recipient NONE | `fcc82220ddd` | hot six-pair confirmation; all29 two-pair screen |
| Combined R2+S2 | `f3c464a54c8` | initial hot two-pair screen; six-pair hot confirmation; all29 two-pair screen |
| Compact F+N+L | `82afe7aef2d` | hot two-pair screen; rejected by coordinator |

S is the original four-geometry-check version. Its results do not describe S2.
R2-NONE and combined results below use their own matching identities; neither
inherits measurements from an earlier component candidate.

## All29: complete two-pair screens

Sources: `E/feedback-screen-all29-golden/summary.json` and
`E/recipient-screen-all29-golden/summary.json`.

| Workload | F pair 1 | F pair 2 | F geomean | R pair 1 | R pair 2 | R geomean |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| richards | 0.995208 | 1.026829 | 1.010895 | 0.986310 | 0.992073 | 0.989187 |
| go | 1.002285 | 1.000559 | 1.001421 | 1.000108 | 0.998841 | 0.999474 |
| nbody | 0.956623 | 0.949866 | 0.953238 | 1.047350 | 1.044185 | 1.045766 |
| float | 0.969278 | 0.971155 | 0.970216 | 0.988147 | 0.991634 | 0.989889 |
| spectral_norm | 0.910269 | 0.911117 | 0.910693 | 0.874040 | 0.868682 | 0.871357 |
| raytrace | 0.995581 | 1.006569 | 1.001060 | 0.999596 | 0.995108 | 0.997349 |
| deltablue | 0.982172 | 0.962524 | 0.972298 | 0.979737 | 0.969660 | 0.974686 |
| chaos | 1.013519 | 1.026245 | 1.019862 | 1.018139 | 0.989169 | 1.003550 |
| nqueens | 0.995731 | 1.014522 | 1.005083 | 1.001281 | 0.995721 | 0.998497 |
| fannkuch | 1.004289 | 1.016901 | 1.010576 | 0.979685 | 0.984381 | 0.982030 |
| generators | 1.016799 | 1.023571 | 1.020179 | 0.973336 | 0.963617 | 0.968464 |
| comprehensions | 0.980995 | 0.945616 | 0.963143 | 0.952431 | 0.957508 | 0.954966 |
| coroutines | 1.026752 | 1.025833 | 1.026292 | 1.070837 | 1.076199 | 1.073514 |
| unpack_sequence | 1.000554 | 1.007299 | 1.003921 | 1.000963 | 0.997986 | 0.999473 |
| deepcopy | 0.990129 | 0.999099 | 0.994604 | 0.984212 | 0.979824 | 0.982016 |
| deepcopy_reduce | 0.987441 | 1.010009 | 0.998661 | 0.980114 | 1.006465 | 0.993202 |
| deepcopy_memo | 0.993525 | 1.005970 | 0.999728 | 0.975252 | 0.969193 | 0.972217 |
| json_dumps | 0.994796 | 1.023597 | 1.009094 | 0.970950 | 0.966203 | 0.968574 |
| json_loads | 0.968218 | 0.996893 | 0.982451 | 0.993680 | 0.999740 | 0.996705 |
| pickle | 1.010643 | 1.000257 | 1.005436 | 1.025559 | 1.018681 | 1.022114 |
| unpickle | 1.007008 | 1.010886 | 1.008945 | 0.994039 | 1.004756 | 0.999383 |
| regex_compile | 0.995957 | 1.017793 | 1.006816 | 0.987770 | 1.003143 | 0.995427 |
| logging_format | 1.037859 | 1.062249 | 1.049983 | 0.992629 | 1.007789 | 1.000180 |
| logging_silent | 0.892291 | 0.894633 | 0.893462 | 1.015844 | 0.881133 | 0.946094 |
| logging_simple | 1.005968 | 0.992480 | 0.999201 | 1.009575 | 0.998885 | 1.004215 |
| xml_etree_generate | 1.001922 | 1.001070 | 1.001496 | 0.996440 | 0.983357 | 0.989877 |
| xml_etree_iterparse | 1.037729 | 1.008574 | 1.023048 | 1.000086 | 0.993809 | 0.996943 |
| xml_etree_parse | 0.989828 | 0.992651 | 0.991239 | 1.001228 | 1.007797 | 1.004507 |
| xml_etree_process | 0.990653 | 1.010985 | 1.000767 | 0.989153 | 0.993629 | 0.991388 |
| Equal-weight all29 geomean | — | — | 0.993748 | — | — | 0.989479 |

Workloads with geomean below 1: F 12/29;
R 22/29. These two-pair screens remain
separate from the longer hot confirmation.

## Hot workloads: confirmation versus initial screen

Cells below are geometric means of paired ratios. F/R/N each have **six pairs**;
L/S each have **two pairs**, with no six-pair confirmation implied.

| Workload | F, 6 pairs | R, 6 pairs | N, 6 pairs | L, initial 2 pairs | S, initial 2 pairs |
| --- | ---: | ---: | ---: | ---: | ---: |
| generators | 1.006147 | 0.965596 | 0.982507 | 0.976634 | 0.994402 |
| coroutines | 1.040777 | 1.068741 | 1.050058 | 1.091024 | 1.075836 |
| deltablue | 0.984315 | 0.975386 | 1.008873 | 0.990439 | 0.974189 |
| richards | 1.010465 | 1.000928 | 1.003377 | 0.987545 | 1.008620 |
| comprehensions | 0.953164 | 0.959056 | 0.991078 | 0.961114 | 0.967587 |
| logging_silent | 0.950116 | 0.927950 | 1.029641 | 0.918127 | 0.872475 |

Pair ranges, retained to show spread:

| Workload | F, 6 pairs | R, 6 pairs | N, 6 pairs | L, initial 2 pairs | S, initial 2 pairs |
| --- | --- | --- | --- | --- | --- |
| generators | 0.969585–1.032938 | 0.945883–0.982647 | 0.947773–0.996838 | 0.967629–0.985723 | 0.988388–1.000453 |
| coroutines | 1.027019–1.061867 | 1.062990–1.075592 | 1.015377–1.060373 | 1.086935–1.095128 | 1.074004–1.077672 |
| deltablue | 0.965762–0.992747 | 0.972054–0.981843 | 1.006685–1.012985 | 0.989342–0.991536 | 0.971162–0.977226 |
| richards | 0.988026–1.021065 | 0.995617–1.011325 | 0.999792–1.007152 | 0.984886–0.990210 | 1.001452–1.015840 |
| comprehensions | 0.947570–0.962386 | 0.955104–0.962707 | 0.986135–0.995088 | 0.958336–0.963900 | 0.961171–0.974045 |
| logging_silent | 0.875937–1.032160 | 0.877901–1.036510 | 0.984790–1.124443 | 0.859616–0.980621 | 0.870647–0.874307 |

Sources: `E/{feedback,recipient,globals}-confirm-hot-golden/summary.json`;
`E/{locals,clears}-screen-hot-golden/summary.json`. Earlier F/R/N hot screens
are retained in their own directories and are not mixed into these confirmations.

## Compact and validated S2: separate completed campaigns

The coordinator rejects compact F+N+L after its two-pair hot screen; its actual
80-byte header does not establish a useful tradeoff. This is a component
campaign decision, not an overall selection. S2 is the constructor-validated
candidate at `e95328ba1d6`; its screens and confirmation remain separate from
initial S and from each other. Cells show geomean (pair minimum–maximum).

| Workload | Compact, screen 2 pairs | S2, screen 2 pairs | S2, confirmation 6 pairs |
| --- | --- | --- | --- |
| generators | 1.007677 (1.003916–1.011451) | 0.981193 (0.951024–1.012318) | 0.997904 (0.988731–1.009129) |
| coroutines | 1.026788 (1.012145–1.041643) | 1.013047 (0.997879–1.028445) | 1.024050 (1.003519–1.052579) |
| deltablue | 1.008884 (1.005745–1.012032) | 0.992883 (0.987881–0.997910) | 0.998218 (0.982935–1.022186) |
| richards | 1.002294 (0.996525–1.008096) | 0.997347 (0.991799–1.002926) | 1.001253 (0.996912–1.005208) |
| comprehensions | 0.966026 (0.960903–0.971176) | 0.963503 (0.954311–0.972783) | 0.964513 (0.956049–0.972832) |
| logging_silent | 1.100487 (1.025231–1.181268) | 0.913577 (0.864252–0.965718) | 0.919179 (0.868307–1.025860) |

Sources: `E/compact-screen-hot-golden/summary.json`,
`E/clears-validated-screen-hot-golden/summary.json` and
`E/clears-validated-confirm-hot-golden/summary.json`.

### S2 all29: complete two-pair screen

Source: `E/clears-validated-screen-all29-golden/summary.json`.

| Workload | Pair 1 | Pair 2 | Geomean |
| --- | ---: | ---: | ---: |
| richards | 0.977001 | 1.004227 | 0.990521 |
| go | 1.015383 | 1.006103 | 1.010732 |
| nbody | 1.017306 | 1.010640 | 1.013968 |
| float | 1.012308 | 0.990215 | 1.001201 |
| spectral_norm | 0.885117 | 0.900758 | 0.892904 |
| raytrace | 1.002090 | 0.998566 | 1.000326 |
| deltablue | 1.015940 | 0.986967 | 1.001348 |
| chaos | 1.001629 | 0.999653 | 1.000641 |
| nqueens | 0.977399 | 0.989702 | 0.983531 |
| fannkuch | 0.995242 | 0.989418 | 0.992326 |
| generators | 1.002460 | 1.002951 | 1.002705 |
| comprehensions | 0.967461 | 0.963612 | 0.965534 |
| coroutines | 1.001005 | 1.008460 | 1.004726 |
| unpack_sequence | 0.996987 | 1.000239 | 0.998612 |
| deepcopy | 1.006543 | 1.003033 | 1.004787 |
| deepcopy_reduce | 0.999317 | 1.006656 | 1.002980 |
| deepcopy_memo | 0.940419 | 0.945681 | 0.943046 |
| json_dumps | 0.998159 | 0.968262 | 0.983097 |
| json_loads | 0.947300 | 1.003176 | 0.974838 |
| pickle | 1.018418 | 1.010189 | 1.014295 |
| unpickle | 1.023835 | 1.009991 | 1.016889 |
| regex_compile | 1.007044 | 1.007794 | 1.007419 |
| logging_format | 1.020919 | 1.024003 | 1.022460 |
| logging_silent | 0.873121 | 0.913403 | 0.893035 |
| logging_simple | 1.029073 | 1.009528 | 1.019254 |
| xml_etree_generate | 0.999618 | 1.006468 | 1.003037 |
| xml_etree_iterparse | 1.005843 | 1.035352 | 1.020491 |
| xml_etree_parse | 1.018210 | 1.016761 | 1.017485 |
| xml_etree_process | 1.003091 | 1.007310 | 1.005199 |
| Equal-weight all29 geomean | — | — | 0.992128 |

Workloads with geomean below 1: 10/29. No S2 admission is inferred from this
screen or the hot confirmation.

## L: separate mapping diagnostics, six balanced pairs

Each process uses 20,000 iterations, one warmup and seven timed samples.
Exec reuses its globals/locals within a sample and executes precompiled
`answer=17`. Class repeats precompiled `class C: pass`; setup explicitly
collects type cycles outside each measured interval. Timeit disables automatic
cyclic GC during each run and restores its prior state. These diagnose steady
mapping entry; they are not startup measurements or additions to all29.

| Diagnostic | Pair 1 | Pair 2 | Pair 3 | Pair 4 | Pair 5 | Pair 6 | Geomean |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| exec | 1.232117 | 1.170124 | 1.265910 | 1.209217 | 1.215732 | 1.242584 | 1.222247 |
| class | 0.980905 | 1.072714 | 1.060125 | 1.042953 | 1.033521 | 1.065341 | 1.042134 |

Source: `E/locals-mapping-confirm-golden/summary.json`; per-run JSON records
raw seconds, GC policy, executable/git identity, JIT and hash state.

## Initial S: actual native census, not timing

One unchanged run(1) after one warmup run(1), counted under GDB between the two
signal boundaries. Candidate `4fbd16a1462`, release ELF SHA256
`d7d0f5ae2bb7bf1aeec248de14700a165a66005b4e619ab5d0cd3c30a6723c00`.
The successful-overwrite breakpoint is at handler offset `+0xd7`.

| Kernel | Generic ADD_INT entries | Refined entries | Successful overwrites |
| --- | ---: | ---: | ---: |
| coroutines | 0 | 121392 | 121392 |
| deltablue | 1 | 2798 | 2798 |
| generators | 0 | 0 | 0 |
| comprehensions | 0 | 0 | 0 |

Sources: `E/clears-hits-snapshot-{coroutines,deltablue,generators,comprehensions}.log`;
all four retries exited normally. `clears-hits-snapshot-commands.json` retains
the command/script identity. The earlier failed `clears-hits-coroutines.log`
is a failed diagnostic attempt, not a runtime finding; its cause is not
established here. Counts establish reached routes in these bounded runs,
not a speedup or an instruction/cycle budget.

## Validated S2: matching native census, not timing

The repeated counts use S2 `e95328ba1d6`, release ELF SHA256
`ec5afe84413004d1da4471163eb13e19fffa67f8dcae7e10dc9c966ec698a83a`
(build ID `54e7bcd1acb1cd299b33f28685e51a6b5d77acfb`). Its success
breakpoint is at the new handler offset `+0x95`, not initial S's `+0xd7`.
The bounded one-warmup/one-counted-run protocol is unchanged.

| Kernel | Generic ADD_INT entries | Refined entries | Successful overwrites |
| --- | ---: | ---: | ---: |
| coroutines | 0 | 121392 | 121392 |
| deltablue | 1 | 2798 | 2798 |

Both runs exited normally. Sources:
`E/clears-validated-hits-{coroutines,deltablue}.log` and
`E/clears-validated-hits-commands.json`. These are actual S2 runs, not
relabeling of initial S's census. The counts agree at these two reached
workloads; the distinct executable removes the recurring geometry checks as
documented in [S2's native review](option5-validated-post-review.md).

## R2-NONE: completed six-pair hot confirmation

Source: `E/recipient-default-hot6-golden/summary.json`; candidate
`fcc82220ddd42fc49ffd1885dcd79e6964889703`. Every row has six balanced pairs.

| Workload | Pair 1 | Pair 2 | Pair 3 | Pair 4 | Pair 5 | Pair 6 | Geomean |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| generators | 0.960946 | 0.959553 | 0.938469 | 0.963368 | 0.960291 | 0.953867 | 0.956045 |
| coroutines | 1.064701 | 1.068543 | 1.053558 | 1.085006 | 1.054671 | 1.056746 | 1.063816 |
| deltablue | 1.008816 | 1.009609 | 1.024618 | 1.007841 | 1.013422 | 1.010292 | 1.012417 |
| richards | 1.001118 | 1.014092 | 1.013312 | 1.013497 | 1.005845 | 1.021929 | 1.011610 |
| comprehensions | 1.015778 | 1.032690 | 0.997369 | 1.023417 | 1.001169 | 1.006976 | 1.012824 |
| logging_silent | 1.004083 | 1.132908 | 1.213036 | 0.995123 | 1.065574 | 1.000152 | 1.065518 |

This confirmation remains separate from R2's initial screen and all29 screen.
Only generators has a geomean below 1 among these six workloads; this is
not an admission decision.

## R2-NONE all29: completed two-pair screen

Source: `E/recipient-default-screen-all29-golden/summary.json`; same R2 head.

| Workload | Pair 1 | Pair 2 | Geomean |
| --- | ---: | ---: | ---: |
| richards | 0.988275 | 1.007414 | 0.997799 |
| go | 0.990848 | 1.008014 | 0.999394 |
| nbody | 1.018548 | 1.006206 | 1.012358 |
| float | 0.982192 | 0.981966 | 0.982079 |
| spectral_norm | 0.945181 | 0.935427 | 0.940291 |
| raytrace | 1.007617 | 1.011025 | 1.009319 |
| deltablue | 1.020456 | 1.008594 | 1.014507 |
| chaos | 0.983501 | 0.976118 | 0.979803 |
| nqueens | 0.995115 | 0.999699 | 0.997404 |
| fannkuch | 0.951263 | 0.958423 | 0.954836 |
| generators | 0.959004 | 0.976301 | 0.967614 |
| comprehensions | 1.005541 | 1.033393 | 1.019372 |
| coroutines | 1.086325 | 1.096270 | 1.091286 |
| unpack_sequence | 0.986971 | 1.004525 | 0.995709 |
| deepcopy | 1.007105 | 0.995764 | 1.001419 |
| deepcopy_reduce | 1.014251 | 1.004132 | 1.009179 |
| deepcopy_memo | 0.954278 | 0.957299 | 0.955788 |
| json_dumps | 0.972568 | 0.986946 | 0.979731 |
| json_loads | 0.989900 | 0.973278 | 0.981554 |
| pickle | 1.016328 | 0.996780 | 1.006506 |
| unpickle | 0.997738 | 1.008089 | 1.002900 |
| regex_compile | 0.993267 | 1.004390 | 0.998813 |
| logging_format | 1.033677 | 1.001755 | 1.017591 |
| logging_silent | 1.049928 | 1.039837 | 1.044870 |
| logging_simple | 1.016118 | 1.049897 | 1.032870 |
| xml_etree_generate | 1.009015 | 1.013954 | 1.011481 |
| xml_etree_iterparse | 1.009360 | 1.001435 | 1.005390 |
| xml_etree_parse | 0.981458 | 0.994366 | 0.987891 |
| xml_etree_process | 1.004720 | 1.008402 | 1.006559 |
| Equal-weight all29 geomean | — | — | 0.999754 |

Workloads with geomean below 1: 14/29. Do not pool these two pairs with the
six-pair hot confirmation.

## Combined R2+S2: completed initial two-pair hot screen

Source: `E/combined-screen-hot-golden/summary.json`; candidate
`f3c464a54c89783e50af1ce9f3cffec6f5e7279d`.

| Workload | Pair 1 | Pair 2 | Geomean |
| --- | ---: | ---: | ---: |
| generators | 0.914672 | 0.942939 | 0.928698 |
| coroutines | 1.090260 | 1.099600 | 1.094920 |
| deltablue | 1.018645 | 0.998823 | 1.008685 |
| richards | 1.001059 | 1.010597 | 1.005817 |
| comprehensions | 0.995747 | 0.990500 | 0.993120 |
| logging_silent | 0.997627 | 0.992174 | 0.994897 |

All three completed campaigns above have matching before/after identity,
clean source and status with stable_identity=true, failure=null and
postflight_error=null. Their raw samples and commands remain beside the
summary files. The completed combined confirmation and all29 screen follow;
none of these campaigns is pooled with another.

## Combined R2+S2: completed six-pair hot confirmation

Source: `E/combined-confirm-hot6-golden/summary.json`; same combined head
`f3c464a54c89783e50af1ce9f3cffec6f5e7279d`.

| Workload | Pair 1 | Pair 2 | Pair 3 | Pair 4 | Pair 5 | Pair 6 | Geomean |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| generators | 0.958237 | 0.953360 | 0.959557 | 0.952030 | 0.951194 | 0.967407 | 0.956948 |
| coroutines | 1.099099 | 1.095934 | 1.100366 | 1.095058 | 1.099069 | 1.101271 | 1.098464 |
| deltablue | 1.005578 | 0.994587 | 1.007046 | 1.007653 | 1.017790 | 1.006824 | 1.006557 |
| richards | 1.004044 | 1.010270 | 0.998244 | 0.992784 | 1.004877 | 1.003866 | 1.002332 |
| comprehensions | 0.995508 | 0.989136 | 0.997825 | 0.993431 | 0.991667 | 1.000776 | 0.994716 |
| logging_silent | 0.990656 | 1.135058 | 0.989046 | 0.991354 | 0.993350 | 0.992683 | 1.014028 |


## Combined R2+S2 all29: completed two-pair screen

Source: `E/combined-screen-all29-golden/summary.json`.

| Workload | Pair 1 | Pair 2 | Geomean |
| --- | ---: | ---: | ---: |
| richards | 0.960269 | 0.997733 | 0.978822 |
| go | 1.013942 | 1.009195 | 1.011566 |
| nbody | 1.003173 | 1.008512 | 1.005839 |
| float | 0.983195 | 0.979318 | 0.981255 |
| spectral_norm | 0.894912 | 0.895859 | 0.895385 |
| raytrace | 1.000227 | 1.004931 | 1.002577 |
| deltablue | 0.985864 | 0.999141 | 0.992480 |
| chaos | 1.002944 | 1.005852 | 1.004397 |
| nqueens | 1.003917 | 0.994756 | 0.999326 |
| fannkuch | 0.973842 | 0.999891 | 0.986781 |
| generators | 0.953754 | 0.960540 | 0.957141 |
| comprehensions | 0.992048 | 0.993622 | 0.992835 |
| coroutines | 1.094975 | 1.038459 | 1.066342 |
| unpack_sequence | 0.994380 | 1.001913 | 0.998139 |
| deepcopy | 0.991294 | 0.997912 | 0.994597 |
| deepcopy_reduce | 1.005333 | 1.006298 | 1.005815 |
| deepcopy_memo | 0.944402 | 0.946007 | 0.945204 |
| json_dumps | 0.992851 | 1.003843 | 0.998332 |
| json_loads | 0.968013 | 0.943063 | 0.955456 |
| pickle | 1.037965 | 1.005311 | 1.021508 |
| unpickle | 1.002422 | 1.008979 | 1.005695 |
| regex_compile | 1.015157 | 1.010011 | 1.012581 |
| logging_format | 1.013893 | 0.996914 | 1.005368 |
| logging_silent | 0.985544 | 0.990014 | 0.987776 |
| logging_simple | 1.061274 | 0.998432 | 1.029374 |
| xml_etree_generate | 0.980729 | 1.000652 | 0.990641 |
| xml_etree_iterparse | 1.003850 | 1.009986 | 1.006913 |
| xml_etree_parse | 1.007346 | 0.990948 | 0.999113 |
| xml_etree_process | 0.982336 | 0.982927 | 0.982631 |
| Equal-weight all29 geomean | — | — | 0.993149 |

Workloads with geomean below 1: 17/29.

Both completed campaigns retain clean stable before/after identities and no
campaign/postflight errors. Raw process samples and commands remain beside
the summaries. The coordinator rejects all performance candidates, including
original R despite its breadth, because the material call-heavy coroutine
regression persists. Combined hot confirmation has generators 0.956948 and
coroutines 1.098464; its all29 geomean 0.993149 does not override that loss.
These are recorded workload outcomes, not an explanation of elapsed cause.
The separate cold correctness-only candidate is still under qualification;
selected51 has not changed.

## Final cold correctness candidate versus golden51

Candidate is `61015a9452d9069646fb5fede9ff963896833b89`; baseline is
selected51 `0e5e61f4c6bc820f2dd1a5f9ee6f7f1768834254`. These are direct
measurements of the three-file cold correction, not relabeled R2 evidence.
Both completed campaigns have identical before/after identity records, clean
source, stable_identity=true, failure=null and postflight_error=null.
They use the unchanged driver and seven observations per process described
above; sample sets remain separate. This candidate is a correctness checkpoint,
not a claimed performance winner.

### Final all29: two balanced pairs

Source: `E/final54-all29-golden/summary.json`.

| Workload | Pair 1 | Pair 2 | Geomean |
| --- | ---: | ---: | ---: |
| richards | 0.969965 | 1.006287 | 0.987959 |
| go | 1.003051 | 1.006938 | 1.004993 |
| nbody | 1.008787 | 0.991008 | 0.999858 |
| float | 0.979056 | 1.003086 | 0.990998 |
| spectral_norm | 1.020103 | 1.012324 | 1.016206 |
| raytrace | 0.999064 | 1.002855 | 1.000957 |
| deltablue | 1.005428 | 0.994732 | 1.000066 |
| chaos | 1.006708 | 1.000914 | 1.003807 |
| nqueens | 0.991261 | 0.994285 | 0.992772 |
| fannkuch | 0.997555 | 0.994681 | 0.996117 |
| generators | 1.010890 | 1.004554 | 1.007717 |
| comprehensions | 1.001140 | 0.992226 | 0.996673 |
| coroutines | 0.989233 | 0.998549 | 0.993880 |
| unpack_sequence | 1.006381 | 0.999638 | 1.003004 |
| deepcopy | 0.993353 | 1.003521 | 0.998424 |
| deepcopy_reduce | 0.998140 | 1.002955 | 1.000544 |
| deepcopy_memo | 1.002165 | 0.998349 | 1.000255 |
| json_dumps | 1.009829 | 0.990561 | 1.000148 |
| json_loads | 1.023282 | 0.996046 | 1.009572 |
| pickle | 1.024294 | 0.997288 | 1.010701 |
| unpickle | 1.001376 | 1.003459 | 1.002417 |
| regex_compile | 1.005372 | 0.998365 | 1.001863 |
| logging_format | 1.031238 | 1.020070 | 1.025639 |
| logging_silent | 1.139660 | 1.137035 | 1.138347 |
| logging_simple | 1.005476 | 1.011206 | 1.008337 |
| xml_etree_generate | 1.006681 | 0.982405 | 0.994469 |
| xml_etree_iterparse | 0.999289 | 0.976010 | 0.987581 |
| xml_etree_parse | 0.997439 | 1.049157 | 1.022971 |
| xml_etree_process | 0.996101 | 1.000182 | 0.998139 |
| Equal-weight all29 geomean | — | — | 1.006381 |

Workloads with geomean below 1: 11/29. The unrounded aggregate is
1.0063812430291168. This two-pair scope does not establish a general gain.

### Final hot confirmation: six balanced pairs

Source: `E/final54-hot6-golden/summary.json`.

| Workload | Paired samples | Geomean | Pair minimum | Pair maximum |
| --- | ---: | ---: | ---: | ---: |
| generators | 6 | 0.996988 | 0.959719 | 1.013777 |
| coroutines | 6 | 0.997860 | 0.986398 | 1.009610 |
| deltablue | 6 | 1.005646 | 0.994954 | 1.020668 |
| richards | 6 | 1.007019 | 1.001649 | 1.016567 |
| comprehensions | 6 | 0.993883 | 0.984551 | 0.999321 |
| logging_silent | 6 | 0.978879 | 0.876076 | 1.005017 |

Four of six geomeans are below 1; equal-weight hot6 geomean is 0.996669.
Do not pool these pairs with all29 or treat that six-workload aggregate as
the all29 result. In particular logging_silent is 1.138347 in all29 (two
pairs 1.139660/1.137035), but 0.978879 in hot6 with a 0.876076–1.005017 pair
range. The wide, differing observations remain visible; no hardware cause,
stable logging improvement or elapsed equivalence is inferred.

Raw commands, per-process samples and identity/status files are retained in
`E/final54-all29-golden` and `E/final54-hot6-golden`. Direct upstream results
are reported below only for a completed campaign. The coordinator owns any
final checkpoint selection, recorded below.

## Final cold candidate versus upstream: completed all29 screen

Ratios in this section are **cold61015 elapsed / upstream elapsed**, not
cold/golden51. Baseline is upstream `3a440b65695`; candidate remains
`61015a9452d9069646fb5fede9ff963896833b89`. Source:
`E/final54-all29-upstream/summary.json`. This has two balanced pairs per
workload, clean identical before/after identities, stable_identity=true and
no campaign/postflight error. Keep it separate from golden comparisons.

| Workload | Pair 1 | Pair 2 | Geomean |
| --- | ---: | ---: | ---: |
| richards | 1.229727 | 1.244033 | 1.236860 |
| go | 1.164815 | 1.170263 | 1.167536 |
| nbody | 0.861732 | 0.860083 | 0.860907 |
| float | 0.789106 | 0.782442 | 0.785767 |
| spectral_norm | 0.806653 | 0.796973 | 0.801799 |
| raytrace | 0.950372 | 0.927872 | 0.939055 |
| deltablue | 1.186064 | 1.085506 | 1.134672 |
| chaos | 0.960655 | 0.919440 | 0.939821 |
| nqueens | 1.194882 | 1.186347 | 1.190607 |
| fannkuch | 1.258960 | 1.197840 | 1.228020 |
| generators | 1.449262 | 1.450931 | 1.450097 |
| comprehensions | 1.359034 | 1.358308 | 1.358671 |
| coroutines | 1.273974 | 1.265055 | 1.269507 |
| unpack_sequence | 1.169938 | 1.164474 | 1.167203 |
| deepcopy | 1.352384 | 1.340086 | 1.346221 |
| deepcopy_reduce | 1.353835 | 1.339872 | 1.346835 |
| deepcopy_memo | 1.333761 | 1.262223 | 1.297499 |
| json_dumps | 1.327753 | 1.350888 | 1.339271 |
| json_loads | 1.215000 | 1.216235 | 1.215617 |
| pickle | 1.106569 | 1.122564 | 1.114538 |
| unpickle | 1.217441 | 1.187163 | 1.202207 |
| regex_compile | 1.170163 | 1.178463 | 1.174306 |
| logging_format | 1.345038 | 1.313189 | 1.329018 |
| logging_silent | 0.739646 | 0.756964 | 0.748255 |
| logging_simple | 1.276266 | 1.317639 | 1.296788 |
| xml_etree_generate | 1.282355 | 1.269258 | 1.275790 |
| xml_etree_iterparse | 1.278535 | 1.270199 | 1.274360 |
| xml_etree_parse | 1.151358 | 1.154616 | 1.152986 |
| xml_etree_process | 1.391138 | 1.392048 | 1.391593 |
| Equal-weight all29 geomean | — | — | 1.156543 |

Workloads with geomean below 1: 6/29. The unrounded aggregate is
1.1565432755153398. This does not establish an upstream performance win.

The first `E/final54-hot6-upstream` campaign failed its load gate during
round 6 comprehensions candidate despite that subprocess returning 0. Its
status has failure set and stable identity; it is not a completed six-pair
comparison. No partial ratios are included here. The completed replacement below has its own successful summary and status;
keep the failed initial campaign excluded.


## Final selected54 versus upstream: valid hot6 confirmation

Source: `E/final54-hot6-confirm2-upstream/summary.json`. Six balanced pairs
per workload, failure=null, postflight_error=null and stable_identity=true.
This is the completed replacement for the failed-load campaign above. Ratios
are selected61015/upstream elapsed; do not pool them with the all29 screen.

| Workload | Pairs | Geomean | Pair minimum | Pair maximum |
| --- | ---: | ---: | ---: | ---: |
| generators | 6 | 1.450207 | 1.424808 | 1.472757 |
| coroutines | 6 | 1.254281 | 1.240417 | 1.265182 |
| deltablue | 6 | 1.154663 | 1.134484 | 1.184544 |
| richards | 6 | 1.280930 | 1.229266 | 1.407654 |
| comprehensions | 6 | 1.390876 | 1.347206 | 1.511459 |
| logging_silent | 6 | 0.712216 | 0.671358 | 0.779797 |

Richards, comprehensions and logging have wide pair ranges; preserve those
ranges rather than describing a precise stable gap. Before/after identities
are clean and stable in all accepted final timing campaigns. The earlier
failed-load evidence remains retained and excluded.

## Final matched PMU and record evidence

Sources: `E/final54-stat-cycles` and `E/final54-stat-branches`. Each completes
18/18 valid invocations: three cases × two runtimes × three rounds. Status
reports complete=true, stable_identity=true, no failure/postflight error.
The following ratios compare actual selected61015 with upstream; they are
PMU counts, not elapsed benchmark ratios or an exclusive cycle partition.

| Workload | Instructions | Cycles | Branches | Branch misses |
| --- | ---: | ---: | ---: | ---: |
| deltablue | 1.353135 | 1.210496 | 1.403099 | 0.892484 |
| generators | 1.413895 | 1.434569 | 1.642096 | 0.965225 |
| coroutines | 1.386260 | 1.249114 | 1.479239 | 1.073262 |

`E/final54-record` completes 6/6 valid invocations (three cases × two runtimes),
with stable identities and no lost records. Candidate flat reports are
`flat-{deltablue,generators,coroutines}-candidate.txt` in that directory.

| Candidate case | Flat symbol/mechanism | Sample share |
| --- | --- | ---: |
| generators | SEND | 26.96% |
| generators | YIELD | 11.78% |
| coroutines | call | 10.43% |
| coroutines | RETURN_GENERATOR | 7.70% |
| coroutines | ClearLocals | 5.84% |
| coroutines | completion | 6.00% |
| deltablue | call | 12.78% |
| deltablue | clear | 5.42% |

These flat symbols identify reached execution work. They do not form an
exclusive ownership/dispatch/GC budget, establish how much can be removed,
or predict elapsed improvement from adding a bitmap or compiler tier.

## Coordinator selection

Select only the three-file cold constructor correctness correction at
`61015a9452d9069646fb5fede9ff963896833b89`, published as
`pyjit/round54-golden` and annotated tag
`pyjit-golden-20261112-frame-state`. All nine performance variants are rejected.
Final prior-golden all29 is 1.006381 (11/29 lower), separate hot6 is 0.996669;
upstream all29 is 1.156543 (6/29 faster). These samples establish no general
performance lift. In particular keep the all29 logging loss and wide hot
results visible. Selection retains the failed full diagnostics and unresolved
profiling finding in [qualification](qualification.md); it does not convert
those runs into passes. Round54 is closed; active work is the plan-only
[round55 architecture review](../call-model55/plan.md).
