# Selected57: complete benchmark results

Candidate: `0dcba75bb3df40c32137db453b001f7e8f101de0`. Golden: `883524c180e4486b47df77ee2276c9276c0ec32c`.
Upstream: `3a440b65695615e9fd9912342b79cdd54fced0d2`.

Ratios are candidate/control; below 1 is faster. Each cell gives the
geomean of paired ratios and their observed minimum/maximum, not a
confidence interval. All-29 and confirmation campaigns remain separate;
their observations are not pooled. Matched PMU results follow in a separate section.
Upstream XML iterparse has opposing .685998–1.905215 paired ratios; the
two-pair aggregate is an observation, not a precise stable upstream gap. No
observations are discarded.

| Case | / golden56 geomean [range] | / upstream geomean [range] |
| --- | ---: | ---: |
| richards | 0.991148 [0.989757, 0.992542] | 0.864295 [0.835066, 0.894547] |
| go | 1.001569 [0.999390, 1.003754] | 1.142197 [1.139280, 1.145121] |
| nbody | 0.998218 [0.996356, 1.000082] | 0.859017 [0.842541, 0.875814] |
| float | 0.997315 [0.995332, 0.999301] | 0.767265 [0.757944, 0.776701] |
| spectral_norm | 0.929569 [0.928643, 0.930496] | 0.762685 [0.747181, 0.778511] |
| raytrace | 1.016835 [1.014581, 1.019094] | 1.002632 [1.002614, 1.002650] |
| deltablue | 1.009654 [1.006567, 1.012751] | 1.127146 [1.126090, 1.128203] |
| chaos | 1.008901 [1.004239, 1.013586] | 0.960809 [0.959031, 0.962591] |
| nqueens | 0.996621 [0.993025, 1.000230] | 1.218130 [1.213609, 1.222667] |
| fannkuch | 0.982754 [0.971516, 0.994122] | 1.226988 [1.209674, 1.244549] |
| generators | 1.014391 [1.013937, 1.014846] | 1.372523 [1.370372, 1.374678] |
| comprehensions | 1.001740 [0.998625, 1.004865] | 1.335003 [1.323598, 1.346506] |
| coroutines | 1.020923 [1.018671, 1.023179] | 1.277331 [1.273222, 1.281453] |
| unpack_sequence | 0.999655 [0.988095, 1.011350] | 1.163789 [1.152563, 1.175123] |
| deepcopy | 0.995406 [0.991789, 0.999035] | 1.349807 [1.337353, 1.362377] |
| deepcopy_reduce | 0.997920 [0.997643, 0.998197] | 1.354491 [1.346665, 1.362362] |
| deepcopy_memo | 0.960209 [0.957223, 0.963203] | 1.239728 [1.228379, 1.251181] |
| json_dumps | 1.003179 [0.991904, 1.014581] | 1.346196 [1.340284, 1.352134] |
| json_loads | 0.992996 [0.990268, 0.995732] | 1.174552 [1.131761, 1.218961] |
| pickle | 0.990852 [0.988411, 0.993299] | 1.100188 [1.084622, 1.115978] |
| unpickle | 1.005111 [0.999077, 1.011181] | 1.211311 [1.187681, 1.235410] |
| regex_compile | 1.007471 [0.999054, 1.015960] | 1.186710 [1.177973, 1.195513] |
| logging_format | 0.970118 [0.955356, 0.985108] | 1.242011 [1.239205, 1.244825] |
| logging_silent | 1.003784 [0.998585, 1.009011] | 0.690670 [0.682365, 0.699076] |
| logging_simple | 1.018912 [1.008290, 1.029647] | 1.259320 [1.232378, 1.286850] |
| xml_etree_generate | 0.989642 [0.984751, 0.994558] | 1.295304 [1.279274, 1.311536] |
| xml_etree_iterparse | 0.970587 [0.966608, 0.974582] | 1.143229 [0.685998, 1.905215] |
| xml_etree_parse | 0.999649 [0.996989, 1.002316] | 1.164433 [1.144556, 1.184655] |
| xml_etree_process | 1.002007 [0.994501, 1.009570] | 1.388621 [1.368483, 1.409056] |

dictionary-all29-golden56: 29 cases, 2 pairs, 116 timing rows; geomean 0.995586; 16 wins / 13 losses / 0 ties.

dictionary-all29-upstream: 29 cases, 2 pairs, 116 timing rows; geomean 1.127137; 6 wins / 23 losses / 0 ties.

dictionary-confirm9-golden56: 9 cases, 6 pairs, 108 timing rows; geomean 0.995750; 5 wins / 4 losses / 0 ties.

## Separate nine-case confirmation

Six pairs per case, golden56 control.

| Case | Geomean [range] |
| --- | ---: |
| comprehensions | 0.997484 [0.974490, 1.011757] |
| deltablue | 1.000239 [0.994835, 1.010206] |
| generators | 1.024797 [0.992243, 1.119177] |
| coroutines | 1.019635 [1.015298, 1.028476] |
| richards | 0.991167 [0.985159, 0.999443] |
| logging_silent | 0.992049 [0.946656, 1.028672] |
| raytrace | 1.020117 [1.012356, 1.027468] |
| spectral_norm | 0.931732 [0.929744, 0.934007] |
| nqueens | 0.987729 [0.982278, 0.995268] |

Exact paired ratios, build identities and input hashes are retained in
`numbers.json`. Qualification and selection remain separate decisions.

## Selected57 upstream PMU

Candidate: `0dcba75bb3df40c32137db453b001f7e8f101de0`; upstream: `3a440b65695615e9fd9912342b79cdd54fced0d2`.

Three pairs per case, 18 valid invocations. Counts are normalized
per hot loop. Ratios are candidate/upstream geomeans; ranges are
observed paired extrema, not confidence intervals. Event counts
do not attribute elapsed time to individual components.

| Case | Event | Upstream count/loop geomean | Candidate count/loop geomean | Ratio [range] |
| --- | --- | ---: | ---: | ---: |
| deltablue | instructions:u | 38940216.086651 | 53014156.200378 | 1.361424 [1.361373, 1.361450] |
| deltablue | cycles:u | 15811512.586606 | 17683953.202017 | 1.118423 [1.076396, 1.150714] |
| comprehensions | instructions:u | 193897.056702 | 278516.873576 | 1.436416 [1.436064, 1.436617] |
| comprehensions | cycles:u | 74140.567249 | 98842.620628 | 1.333179 [1.319974, 1.341343] |
| coroutines | instructions:u | 294666139.278219 | 385373452.682796 | 1.307831 [1.304246, 1.309627] |
| coroutines | cycles:u | 108551087.833871 | 137619770.868433 | 1.267788 [1.265298, 1.270468] |

Exact pair ratios, per-role counts, build identities and input hashes
are retained in `perf-numbers.json`. Selection remains separate.

## Rejected performance bundle: complete measurements

Candidate: `9ec291998f7395fedf21f456e5eb58c2187d2e23` (rejected).
Golden56: `883524c180e4486b47df77ee2276c9276c0ec32c`.
Upstream: `3a440b65695615e9fd9912342b79cdd54fced0d2`.

Elapsed ratios are candidate/control; below 1 is faster. Ranges are observed
paired minima/maxima, not confidence intervals. All-29 and confirmation
campaigns are distinct; their observations are not pooled.

### All 29 cases — two pairs per control

| Case | / golden56 geomean [range] | / upstream geomean [range] |
| --- | ---: | ---: |
| richards | 0.990039 [0.976973, 1.003280] | 0.892258 [0.885598, 0.898969] |
| go | 0.986465 [0.977314, 0.995702] | 1.112828 [1.083243, 1.143220] |
| nbody | 1.001072 [0.987004, 1.015340] | 0.852659 [0.846091, 0.859278] |
| float | 1.017873 [1.014299, 1.021460] | 0.788617 [0.781880, 0.795412] |
| spectral_norm | 0.874336 [0.868516, 0.880195] | 0.730606 [0.726398, 0.734838] |
| raytrace | 1.013508 [1.010209, 1.016818] | 0.999984 [0.987738, 1.012381] |
| deltablue | 0.970865 [0.970840, 0.970891] | 1.204904 [1.198852, 1.210987] |
| chaos | 1.019537 [1.013474, 1.025637] | 0.950543 [0.914467, 0.988041] |
| nqueens | 0.990446 [0.982797, 0.998155] | 1.227823 [1.226867, 1.228781] |
| fannkuch | 0.997339 [0.991490, 1.003223] | 1.253759 [1.248930, 1.258606] |
| generators | 0.996487 [0.993029, 0.999957] | 1.368729 [1.347262, 1.390538] |
| comprehensions | 0.965999 [0.964121, 0.967879] | 1.294292 [1.293774, 1.294809] |
| coroutines | 1.050031 [1.045952, 1.054126] | 1.312683 [1.310873, 1.314496] |
| unpack_sequence | 1.017839 [0.998917, 1.037120] | 1.168787 [1.157330, 1.180357] |
| deepcopy | 0.989345 [0.987391, 0.991303] | 1.358507 [1.352732, 1.364307] |
| deepcopy_reduce | 0.987674 [0.983197, 0.992171] | 1.337880 [1.337054, 1.338706] |
| deepcopy_memo | 0.969734 [0.959528, 0.980049] | 1.255557 [1.252791, 1.258329] |
| json_dumps | 0.990207 [0.987954, 0.992465] | 1.349503 [1.338900, 1.360190] |
| json_loads | 1.012720 [0.998329, 1.027318] | 1.214153 [1.211338, 1.216974] |
| pickle | 0.998576 [0.994006, 1.003166] | 1.120321 [1.096821, 1.144324] |
| unpickle | 1.016016 [1.013753, 1.018285] | 1.231611 [1.216160, 1.247259] |
| regex_compile | 0.990950 [0.989318, 0.992584] | 1.198251 [1.197041, 1.199462] |
| logging_format | 1.042398 [1.041610, 1.043186] | 1.269976 [1.206469, 1.336827] |
| logging_silent | 1.142203 [1.136437, 1.147997] | 0.788098 [0.784772, 0.791438] |
| logging_simple | 1.011864 [0.975180, 1.049929] | 1.303390 [1.285278, 1.321758] |
| xml_etree_generate | 1.010592 [1.007687, 1.013506] | 1.296658 [1.292186, 1.301146] |
| xml_etree_iterparse | 0.992089 [0.976532, 1.007893] | 1.246095 [1.242101, 1.250102] |
| xml_etree_parse | 1.016208 [1.015595, 1.016820] | 1.169685 [1.166179, 1.173202] |
| xml_etree_process | 1.012312 [1.002347, 1.022376] | 1.411279 [1.397676, 1.425014] |

final-all29-golden56: 29 cases, 2 pairs, 116 timing rows; geomean 1.001784; 15 wins / 14 losses / 0 ties.

final-all29-upstream: 29 cases, 2 pairs, 116 timing rows; geomean 1.144639; 7 wins / 22 losses / 0 ties.

final-confirm9-golden56: 9 cases, 6 pairs, 108 timing rows; geomean 0.996933; 5 wins / 4 losses / 0 ties.

### Separate confirmation — six pairs, golden56 control

| Case | Geomean [range] |
| --- | ---: |
| comprehensions | 0.948526 [0.925331, 0.969335] |
| deltablue | 0.966836 [0.955469, 0.975668] |
| generators | 0.993400 [0.974954, 1.012781] |
| coroutines | 1.046046 [1.032976, 1.056326] |
| richards | 0.997461 [0.988257, 1.017172] |
| logging_silent | 1.131569 [1.101124, 1.145630] |
| raytrace | 1.012544 [1.005835, 1.018399] |
| spectral_norm | 0.887055 [0.883850, 0.894165] |
| nqueens | 1.006876 [0.996451, 1.030738] |

### PMU — three pairs per control

Counts are normalized per hot loop. Lower count ratios mean fewer events,
not an elapsed-time attribution. These counts do not assign exclusive causal
percentages to implementation components.

#### final-perf-stat — 18 valid invocations

| Case | Event | upstream count/loop geomean | Candidate count/loop geomean | Ratio [range] |
| --- | --- | ---: | ---: | ---: |
| comprehensions | instructions:u | 193977.891541 | 270445.551957 | 1.394208 [1.393729, 1.394465] |
| comprehensions | cycles:u | 73250.053647 | 96443.778841 | 1.316638 [1.307907, 1.333799] |
| logging_silent | instructions:u | 13943.983330 | 13492.030176 | 0.967588 [0.967588, 0.967588] |
| logging_silent | cycles:u | 4777.497727 | 3756.918992 | 0.786378 [0.781805, 0.790780] |
| coroutines | instructions:u | 295475984.504274 | 385373447.427778 | 1.304246 [1.304246, 1.304246] |
| coroutines | cycles:u | 108309945.955125 | 140676788.279368 | 1.298835 [1.294346, 1.303616] |

#### final-golden-stat — 18 valid invocations

| Case | Event | golden count/loop geomean | Candidate count/loop geomean | Ratio [range] |
| --- | --- | ---: | ---: | ---: |
| comprehensions | instructions:u | 284007.501104 | 270445.029546 | 0.952246 [0.952211, 0.952297] |
| comprehensions | cycles:u | 99725.170915 | 95869.841233 | 0.961340 [0.958080, 0.965461] |
| logging_silent | instructions:u | 13492.026459 | 13492.030116 | 1.000000 [1.000000, 1.000000] |
| logging_silent | cycles:u | 3322.451413 | 3751.358093 | 1.129093 [1.116059, 1.151920] |
| coroutines | instructions:u | 385373445.462366 | 385373446.466667 | 1.000000 [1.000000, 1.000000] |
| coroutines | cycles:u | 134961022.976043 | 140797745.442642 | 1.043247 [1.041083, 1.045859] |

Exact pair ratios, per-role PMU counts and input hashes are retained in
`numbers.json`. Selection and regression interpretation remain separate.
