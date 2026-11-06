# Cold-path integration kernel recount

Source: dirty candidate after d6eae4b1c1c, frozen for full qualification.
All 29 kernels complete with real handler coverage and zero loop deltas.
The strict raw gate still fails; counts below sum every boxing kind.

| Kernel | Warmup | Low | High |
| --- | ---: | ---: | ---: |
| richards | 0 | 0 | 0 |
| go | 59 | 7 | 7 |
| nbody | 18 | 0 | 0 |
| float | 0 | 0 | 0 |
| spectral_norm | 0 | 0 | 0 |
| raytrace | 8 | 4 | 4 |
| deltablue | 0 | 0 | 0 |
| chaos | 20 | 7 | 7 |
| nqueens | 0 | 0 | 0 |
| fannkuch | 0 | 0 | 0 |
| generators | 4 | 4 | 4 |
| comprehensions | 103 | 92 | 92 |
| coroutines | 0 | 0 | 0 |
| unpack_sequence | 0 | 0 | 0 |
| deepcopy | 77 | 74 | 74 |
| deepcopy_reduce | 74 | 74 | 74 |
| deepcopy_memo | 74 | 74 | 74 |
| json_dumps | 473 | 419 | 419 |
| json_loads | 442 | 422 | 422 |
| pickle | 1035 | 1008 | 1008 |
| unpickle | 1008 | 1008 | 1008 |
| regex_compile | 390 | 371 | 371 |
| logging_format | 370 | 367 | 367 |
| logging_silent | 367 | 367 | 367 |
| logging_simple | 367 | 367 | 367 |
| xml_etree_generate | 388 | 374 | 374 |
| xml_etree_iterparse | 374 | 374 | 374 |
| xml_etree_parse | 374 | 374 | 374 |
| xml_etree_process | 374 | 374 | 374 |

8 kernels have zero boxes in all three raw processes: richards, float, spectral_norm, deltablue, nqueens, fannkuch, coroutines, unpack_sequence.

Remaining source attributions include binary constant folding, range iteration setup,
ABC initialization, mappingproxy, StoreName and object-form call/attribute continuations.
See the retained raw journal and remaining-pcs.txt in this attempt; inlined PCs
alone may not identify the caller when the compiler has merged their code.
