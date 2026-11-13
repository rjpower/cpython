# Round54 benchmark tables

Draft of completed campaigns; no selection or admission decision. Ratios are
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
| Compact F+N+L | `82afe7aef2d` | hot two-pair screen; rejected by coordinator |

S is the original four-geometry-check version. Its results do not describe S2.
R2-NONE at `fcc82220ddd` has source review but no native or timing result in
this snapshot.

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
