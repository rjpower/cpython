# Raw kernel boxing at 66981c963be

All 29 loop deltas are zero, but strict startup-inclusive admission still
fails: 15 kernels have zero boxes in every raw process and 14 retain setup or
startup boxes. The capture has 87 valid warmup/low/high records. Embedded
executable revision and clean runtime source both identify `66981c963be`.
The exact stats ELF is retained for later source attribution.

| Kernel with remaining boxes | Warmup | Low | High |
| --- | ---: | ---: | ---: |
| comprehensions | 29 | 29 | 29 |
| deepcopy | 18 | 18 | 18 |
| deepcopy_reduce | 18 | 18 | 18 |
| deepcopy_memo | 18 | 18 | 18 |
| json_dumps | 83 | 83 | 83 |
| json_loads | 83 | 83 | 83 |
| regex_compile | 32 | 32 | 32 |
| logging_format | 23 | 23 | 23 |
| logging_silent | 23 | 23 | 23 |
| logging_simple | 23 | 23 | 23 |
| xml_etree_generate | 28 | 36 | 36 |
| xml_etree_iterparse | 36 | 36 | 36 |
| xml_etree_parse | 36 | 36 | 36 |
| xml_etree_process | 36 | 36 | 36 |

Raw-zero kernels: richards, go, nbody, float, spectral_norm, raytrace,
deltablue, chaos, nqueens, fannkuch, generators, coroutines, unpack_sequence,
pickle and unpickle. Counts sum all boxing kinds; zero differenced work does
not cancel the startup requirement. These are boxing counts, not elapsed-time
results or individual-patch attribution.

Evidence under the external build root:
`pyjit-link-wave1/checkpoint-closure/kernels-66981c963be/`, containing
`kernels.json`, original `raw.jsonl`, the strict failing gate log, and
`elf/python`. Final full-suite qualification, ownership inventory and process
accounting remain open. No new performance comparison is claimed.
