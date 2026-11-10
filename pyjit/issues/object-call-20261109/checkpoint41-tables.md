# Candidate41 benchmark tables

Candidate `0a35a382909`, frozen39 `4d666e1b482`, upstream `3a440b65695`.
GCC 15.2, CPU 15, hash 0, JIT off. Two balanced rounds, seven samples per process.
Each ratio is candidate/control elapsed time. Both campaigns pass identity gates.

| Kernel | /frozen39 | /upstream |
| --- | ---: | ---: |
| richards | 1.025545 | 1.263427 |
| go | 0.986625 | 1.160125 |
| nbody | 1.015605 | 0.838358 |
| float | 1.010494 | 0.792403 |
| spectral_norm | 1.029190 | 0.667057 |
| raytrace | 1.025476 | 0.964655 |
| deltablue | 1.038892 | 1.671145 |
| chaos | 1.001217 | 0.963154 |
| nqueens | 0.999853 | 1.196395 |
| fannkuch | 1.028410 | 1.254816 |
| generators | 1.008660 | 1.473958 |
| comprehensions | 1.010981 | 1.337831 |
| coroutines | 1.064424 | 1.351663 |
| unpack_sequence | 0.998856 | 1.157485 |
| deepcopy | 0.993900 | 1.367301 |
| deepcopy_reduce | 0.995898 | 1.345946 |
| deepcopy_memo | 0.974711 | 1.309809 |
| json_dumps | 1.003869 | 1.341689 |
| json_loads | 0.997999 | 1.205152 |
| pickle | 0.997038 | 1.101701 |
| unpickle | 1.002302 | 1.189356 |
| regex_compile | 1.010833 | 1.182209 |
| logging_format | 0.993716 | 1.307105 |
| logging_silent | 1.054426 | 0.944927 |
| logging_simple | 1.027788 | 1.337933 |
| xml_etree_generate | 1.017571 | 1.314431 |
| xml_etree_iterparse | 0.981128 | 1.261405 |
| xml_etree_parse | 0.986146 | 1.127425 |
| xml_etree_process | 1.021451 | 1.431158 |
| **Geometric mean** | **1.010243** | **1.181049** |

## Long-interval micro results

Six balanced rounds per role/case; unchanged bodies. Class reads use 20,000
loops, keywords 2,000; report these separately from the earlier short campaign.

| Micro | /frozen39 | /upstream |
| --- | ---: | ---: |
| class_attr_load | 0.169142 | 1.024064 |
| call_py_kw | 0.636259 | 1.124120 |

Class/upstream pairs span 0.978–1.103; its mean does not establish parity.
All six keyword/upstream pairs exceed 1.108. Application regressions require
repeat/diagnosis; large micro gains do not qualify this image.
