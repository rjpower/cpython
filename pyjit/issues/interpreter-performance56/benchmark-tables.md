# Selected56 benchmark tables

Selected runtime: `883524c180e4486b47df77ee2276c9276c0ec32c`.
Elapsed candidate/control ratios; lower is better. Each all-29 comparison has
two balanced pairs and 116 native commands. Ranges are observed pair ranges,
not confidence intervals. Golden54 and upstream are measured directly in separate
campaigns; their ratios must not be multiplied.

| Benchmark | /golden54 | Pair range | /upstream | Pair range |
| --- | ---: | --- | ---: | --- |
| richards | 0.705291 | 0.691736–0.719112 | 0.875120 | 0.855926–0.894744 |
| go | 0.995968 | 0.989724–1.002251 | 1.153009 | 1.144890–1.161185 |
| nbody | 1.009694 | 1.004464–1.014951 | 0.851462 | 0.843693–0.859301 |
| float | 1.003437 | 0.998580–1.008317 | 0.790806 | 0.788365–0.793255 |
| spectral_norm | 1.035320 | 1.032991–1.037654 | 0.821792 | 0.820841–0.822744 |
| raytrace | 1.037725 | 1.034892–1.040566 | 0.977158 | 0.972666–0.981671 |
| deltablue | 0.982608 | 0.979457–0.985769 | 1.162111 | 1.154009–1.170268 |
| chaos | 1.025119 | 1.018615–1.031663 | 0.929217 | 0.917871–0.940704 |
| nqueens | 1.035200 | 1.034181–1.036219 | 1.217342 | 1.214993–1.219696 |
| fannkuch | 0.999914 | 0.998645–1.001185 | 1.239178 | 1.233085–1.245301 |
| generators | 0.948994 | 0.929137–0.969275 | 1.371818 | 1.348054–1.396002 |
| comprehensions | 0.986013 | 0.985837–0.986190 | 1.337402 | 1.334799–1.340011 |
| coroutines | 0.988903 | 0.958080–1.020717 | 1.255482 | 1.248140–1.262867 |
| unpack_sequence | 0.980759 | 0.969909–0.991731 | 1.175946 | 1.170976–1.180937 |
| deepcopy | 0.991202 | 0.978280–1.004295 | 1.369845 | 1.366733–1.372963 |
| deepcopy_reduce | 1.003490 | 0.998359–1.008648 | 1.363200 | 1.358490–1.367926 |
| deepcopy_memo | 0.975768 | 0.975396–0.976141 | 1.283413 | 1.276111–1.290756 |
| json_dumps | 0.995131 | 0.995014–0.995248 | 1.359789 | 1.356692–1.362894 |
| json_loads | 1.005325 | 1.003211–1.007445 | 1.258430 | 1.230154–1.287356 |
| pickle | 0.992045 | 0.984214–0.999939 | 1.107311 | 1.104197–1.110433 |
| unpickle | 1.001702 | 0.993029–1.010451 | 1.212236 | 1.210774–1.213699 |
| regex_compile | 1.000798 | 0.999673–1.001925 | 1.202856 | 1.200518–1.205198 |
| logging_format | 0.963972 | 0.956518–0.971483 | 1.224647 | 1.208812–1.240689 |
| logging_silent | 0.947066 | 0.904314–0.991840 | 0.690918 | 0.683512–0.698405 |
| logging_simple | 0.987289 | 0.982118–0.992486 | 1.265587 | 1.222289–1.310419 |
| xml_etree_generate | 1.014057 | 1.011135–1.016987 | 1.305414 | 1.298686–1.312176 |
| xml_etree_iterparse | 1.027050 | 1.022621–1.031498 | 1.274537 | 1.269532–1.279562 |
| xml_etree_parse | 0.981598 | 0.977724–0.985487 | 1.142545 | 1.140113–1.144982 |
| xml_etree_process | 0.991927 | 0.984018–0.999900 | 1.415576 | 1.404721–1.426514 |
| **Geometric mean** | **0.984655** | 17/29 wins | **1.141077** | 7/29 wins |

## Six-pair confirmation against golden54

84 native commands; all six focus cases improve in every pair. Raytrace
regresses in every pair. This is a separate campaign from the all-29 table.

| Benchmark | Ratio | Pair range |
| --- | ---: | --- |
| deltablue | 0.981856 | 0.947643–0.997174 |
| generators | 0.951182 | 0.939691–0.965754 |
| coroutines | 0.990377 | 0.962840–0.999833 |
| richards | 0.712389 | 0.705732–0.717182 |
| comprehensions | 0.983860 | 0.978990–0.989874 |
| logging_silent | 0.904529 | 0.893978–0.915880 |
| raytrace | 1.045040 | 1.038798–1.049473 |

## Matched upstream hardware counters

Three balanced pairs, 24 valid commands, 100% counter running time. Counts
are normalized per benchmark loop. These are native instructions and user
cycles, not Python bytecode counts.

| Benchmark | Instructions /upstream | Cycles /upstream |
| --- | ---: | ---: |
| deltablue | 1.361430 | 1.152042 |
| generators | 1.411083 | 1.340458 |
| coroutines | 1.307831 | 1.238294 |
| logging_silent | 0.967588 | 0.674832 |

Evidence under E56 (`HANDOFF.md` defines its absolute path):
`closure56-all29-{golden,upstream}`, `closure56-confirm7-golden`,
`closure56-perf-stat`, `closure56-perf-record`, and
`final-measurement/numbers.json`. Every campaign validates source/native
identity before and after execution. [Analysis and limitations](report.md).
