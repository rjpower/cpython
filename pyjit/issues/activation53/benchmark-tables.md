# Activation 53 benchmark tables

Elapsed ratios are candidate/control, lower is faster. Unchanged kernels use
GCC 15.2, CPU 15, hash 0 and JIT disabled. Each campaign is a direct comparison;
do not multiply or subtract its ratios. E is
`/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/activation53`.
Summaries, individual outputs, commands, before/after identities and status are
retained in the named E directories. All completed campaigns below report stable
identities and no campaign or postflight error.

## Rejected candidates versus golden51

Six-pair confirmation geometric means, ranges and loss counts; values above 1
are losses. Screen results are retained in the linked component reports.

| Case | Completion mean (range; losses) | Full final-home mean (range; losses) | Native-only mean (range; losses) |
| --- | --- | --- | --- |
| generators | 0.994377 (0.987477–1.001501; 1/6) | 0.929631 (0.922617–0.939132; 0/6) | 0.940807 (0.933855–0.950759; 0/6) |
| coroutines | 1.031130 (1.025242–1.036236; 6/6) | 1.027712 (1.022094–1.036887; 6/6) | 1.004339 (0.999832–1.007909; 5/6) |
| deltablue | 0.989166 (0.981583–0.995987; 0/6) | 0.999114 (0.990558–1.004414; 4/6) | 1.024337 (1.008533–1.038163; 6/6) |
| logging_silent | 0.959817 (0.861368–0.999575; 0/6) | 1.244130 (1.094948–1.317357; 6/6) | 0.973093 (0.866070–1.013261; 1/6) |
| comprehensions | 0.988206 (0.981280–0.997215; 0/6) | 1.014538 (1.007823–1.023783; 6/6) | 0.995756 (0.990313–0.999909; 0/6) |
| richards | 1.011310 (1.008910–1.013916; 6/6) | 0.999574 (0.994945–1.003979; 3/6) | 1.027155 (1.021537–1.031756; 6/6) |

See [completion](completion-result.md), [full final-home](final-homes-result.md)
and [native-only](native-construction-result.md) for the two-pair screens and
qualification limits. No rejected candidate ran all29 or an upstream campaign.

## Selected golden51 versus upstream: two-pair all29 screen

E/`selected51-refresh-all29-upstream`; selected51 and upstream are immutable.
Geometric mean across all 29 case means is 1.156831; six case means win. The
individual paired ratios are retained below. Generators vary widely, so their
two-pair mean is a screen, not a stable estimate.

| Case | Pair 1 | Pair 2 | Geometric mean |
| --- | ---: | ---: | ---: |
| richards | 1.219834 | 1.225483 | 1.222655 |
| go | 1.104163 | 1.158228 | 1.130873 |
| nbody | 0.845040 | 0.853905 | 0.849461 |
| float | 0.781514 | 0.761786 | 0.771587 |
| spectral_norm | 0.798553 | 0.794217 | 0.796382 |
| raytrace | 0.939326 | 0.933793 | 0.936555 |
| deltablue | 1.177481 | 1.159495 | 1.168453 |
| chaos | 0.957060 | 0.947246 | 0.952140 |
| nqueens | 1.194579 | 1.202051 | 1.198309 |
| fannkuch | 1.273666 | 1.257511 | 1.265562 |
| generators | 1.437137 | 1.784438 | 1.601400 |
| comprehensions | 1.372295 | 1.366372 | 1.369331 |
| coroutines | 1.265773 | 1.261537 | 1.263654 |
| unpack_sequence | 1.145406 | 1.169219 | 1.157252 |
| deepcopy | 1.359446 | 1.341954 | 1.350672 |
| deepcopy_reduce | 1.335494 | 1.348508 | 1.341985 |
| deepcopy_memo | 1.316038 | 1.303808 | 1.309909 |
| json_dumps | 1.356048 | 1.377119 | 1.366543 |
| json_loads | 1.242195 | 1.232984 | 1.237581 |
| pickle | 1.095678 | 1.085857 | 1.090756 |
| unpickle | 1.191398 | 1.205533 | 1.198445 |
| regex_compile | 1.143926 | 1.167469 | 1.155638 |
| logging_format | 1.269245 | 1.212873 | 1.240739 |
| logging_silent | 0.773477 | 0.767520 | 0.770493 |
| logging_simple | 1.248580 | 1.230005 | 1.239258 |
| xml_etree_generate | 1.264298 | 1.285200 | 1.274706 |
| xml_etree_iterparse | 1.264577 | 1.267180 | 1.265878 |
| xml_etree_parse | 1.162345 | 1.128617 | 1.145357 |
| xml_etree_process | 1.416106 | 1.398903 | 1.407478 |

## Selected golden51 versus upstream: six-pair hot confirmation

E/`selected51-refresh-hot-upstream`; this independent six-pair confirmation
retains the all29 screen above. Losses count pairs above 1.0.

| Case | Geometric mean | Pair range | Losses |
| --- | ---: | ---: | ---: |
| generators | 1.450583 | 1.441005–1.462817 | 6/6 |
| coroutines | 1.257081 | 1.240689–1.271521 | 6/6 |
| deltablue | 1.174507 | 1.170100–1.186879 | 6/6 |
| logging_silent | 0.730916 | 0.662089–0.771119 | 0/6 |
| comprehensions | 1.376346 | 1.355705–1.402835 | 6/6 |
| richards | 1.245939 | 1.208283–1.271766 | 6/6 |
