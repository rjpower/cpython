# Kernel boxing capture for `6a0e4c12c17`

The retained stats executable was built from clean runtime HEAD
`6a0e4c12c17`. Its ELF build ID is
`41a19527c221c6e36e096ef4a42a5aac5002442f`. The raw capture contains 87
records for 29 kernels. Eighteen kernels are startup-inclusive raw-zero, and
all 29 loop deltas are zero. The strict gate returned failure; the checkpoint
is not admitted.

The exact inputs are under
`build/pyjit-link-wave1/checkpoint-closure/kernels-6a0e4c12c17/` in the runtime
build tree: `raw.jsonl`, `kernels.json`, `counts-summary.json`, `gate.log`,
`source-status.txt`, and `elf/notes.txt`. The table below is transcribed from
`counts-summary.json`; warmup, low, and high are separate process records.
Zero rows have no boxes in any of those three records.

| Kernel | Warmup | Low | High | Raw-zero | Loop delta |
| --- | ---: | ---: | ---: | :---: | ---: |
| richards | 0 | 0 | 0 | yes | 0 |
| go | 0 | 0 | 0 | yes | 0 |
| nbody | 0 | 0 | 0 | yes | 0 |
| float | 0 | 0 | 0 | yes | 0 |
| spectral_norm | 0 | 0 | 0 | yes | 0 |
| raytrace | 0 | 0 | 0 | yes | 0 |
| deltablue | 0 | 0 | 0 | yes | 0 |
| chaos | 0 | 0 | 0 | yes | 0 |
| nqueens | 0 | 0 | 0 | yes | 0 |
| fannkuch | 0 | 0 | 0 | yes | 0 |
| generators | 0 | 0 | 0 | yes | 0 |
| comprehensions | 8 | 8 | 8 | no | 0 |
| coroutines | 0 | 0 | 0 | yes | 0 |
| unpack_sequence | 0 | 0 | 0 | yes | 0 |
| deepcopy | 0 | 0 | 0 | yes | 0 |
| deepcopy_reduce | 0 | 0 | 0 | yes | 0 |
| deepcopy_memo | 0 | 0 | 0 | yes | 0 |
| json_dumps | 1 | 1 | 1 | no | 0 |
| json_loads | 1 | 1 | 1 | no | 0 |
| pickle | 0 | 0 | 0 | yes | 0 |
| unpickle | 0 | 0 | 0 | yes | 0 |
| regex_compile | 14 | 14 | 14 | no | 0 |
| logging_format | 5 | 5 | 5 | no | 0 |
| logging_silent | 5 | 5 | 5 | no | 0 |
| logging_simple | 5 | 5 | 5 | no | 0 |
| xml_etree_generate | 7 | 15 | 15 | no | 0 |
| xml_etree_iterparse | 15 | 15 | 15 | no | 0 |
| xml_etree_parse | 15 | 15 | 15 | no | 0 |
| xml_etree_process | 15 | 15 | 15 | no | 0 |

Eleven kernels retain startup boxes. Remaining nonzero kernel sites
are comprehensions (8), JSON dumps/loads (1 each), regex compile (14), the
three logging kernels (5 each), and XML generation (7 warmup, 15 low/high)
plus its three sibling kernels (15 in each process). These are unresolved
R27 debt, not compatibility exceptions. The full startup-inclusive strict
gate remains failed even though loop deltas are zero.
