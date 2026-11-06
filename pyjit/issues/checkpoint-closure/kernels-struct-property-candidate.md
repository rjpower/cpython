# Raw kernel census after struct, property and constant ports

Source matches committed `7baf96ed017` (capture binary stamped its predecessor dirty source). Exact source diff and 87 raw process records are retained under runtime evidence `checkpoint-closure/kernels-struct-property-candidate/`. Eleven of 29 kernels are zero in every raw process. All 29 loop deltas are zero; strict R27 still fails.

| Kernel | Warmup | Low | High |
| --- | ---: | ---: | ---: |
| richards | 0 | 0 | 0 |
| go | 3 | 3 | 3 |
| nbody | 0 | 0 | 0 |
| float | 0 | 0 | 0 |
| spectral_norm | 0 | 0 | 0 |
| raytrace | 0 | 0 | 0 |
| deltablue | 0 | 0 | 0 |
| chaos | 3 | 3 | 3 |
| nqueens | 0 | 0 | 0 |
| fannkuch | 0 | 0 | 0 |
| generators | 0 | 0 | 0 |
| comprehensions | 43 | 37 | 37 |
| coroutines | 0 | 0 | 0 |
| unpack_sequence | 0 | 0 | 0 |
| deepcopy | 29 | 26 | 26 |
| deepcopy_reduce | 26 | 26 | 26 |
| deepcopy_memo | 26 | 26 | 26 |
| json_dumps | 376 | 376 | 376 |
| json_loads | 379 | 379 | 379 |
| pickle | 1004 | 1004 | 1004 |
| unpickle | 1004 | 1004 | 1004 |
| regex_compile | 326 | 326 | 326 |
| logging_format | 321 | 321 | 321 |
| logging_silent | 321 | 321 | 321 |
| logging_simple | 321 | 321 | 321 |
| xml_etree_generate | 323 | 331 | 331 |
| xml_etree_iterparse | 331 | 331 | 331 |
| xml_etree_parse | 331 | 331 | 331 |
| xml_etree_process | 331 | 331 | 331 |

The final struct source equals the independently reviewed W5 packet after rebasing over mechanical API changes. Three builds include all 117 modules, lint passes, and 4,700 integration tests pass in each debug/release profile. Stats-specific formatter zero-box and failing-allocator owner-metadata probes pass. Earlier readonly-property wording failures and malformed patch attempts remain retained; only the generated v2 readonly delta was integrated.
