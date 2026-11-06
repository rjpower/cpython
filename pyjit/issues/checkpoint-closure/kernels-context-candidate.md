# Raw kernel census after Context, iterator and math integration

Source: dirty candidate over `6aec17eefeb`, packets in [integration manifest](second-integration-manifest.json). Capture: `build/pyjit-link-wave1/checkpoint-closure/kernels-context-iterators-math` in the runtime build root. All 87 processes completed. Nine of 29 kernels have zero boxes in every process; all 29 loop deltas are zero. The strict R27 gate fails.

| Kernel | Warmup | Low | High |
| --- | ---: | ---: | ---: |
| richards | 0 | 0 | 0 |
| go | 47 | 3 | 3 |
| nbody | 18 | 0 | 0 |
| float | 0 | 0 | 0 |
| spectral_norm | 0 | 0 | 0 |
| raytrace | 4 | 0 | 0 |
| deltablue | 0 | 0 | 0 |
| chaos | 4 | 3 | 3 |
| nqueens | 0 | 0 | 0 |
| fannkuch | 0 | 0 | 0 |
| generators | 0 | 0 | 0 |
| comprehensions | 46 | 37 | 37 |
| coroutines | 0 | 0 | 0 |
| unpack_sequence | 0 | 0 | 0 |
| deepcopy | 29 | 26 | 26 |
| deepcopy_reduce | 26 | 26 | 26 |
| deepcopy_memo | 26 | 26 | 26 |
| json_dumps | 433 | 380 | 380 |
| json_loads | 403 | 383 | 383 |
| pickle | 1024 | 1004 | 1004 |
| unpickle | 1004 | 1004 | 1004 |
| regex_compile | 339 | 332 | 332 |
| logging_format | 331 | 328 | 328 |
| logging_silent | 328 | 328 | 328 |
| logging_simple | 328 | 328 | 328 |
| xml_etree_generate | 330 | 335 | 335 |
| xml_etree_iterparse | 335 | 335 | 335 |
| xml_etree_parse | 335 | 335 | 335 |
| xml_etree_process | 335 | 335 | 335 |

Initial attribution resolves compiler/list PCs to `flowgraph.c:get_const_value`, attribute thunk owner to `ceval.c:_PyEval_ImportFrom`, deferred-annotation list projections to `codegen.c`, and additional import/setup boxes to type dictionary updates, `tp_new_wrapper` and `lru_cache_new`. These are remaining core work, not permitted external boundaries. The full suite has a separate, broader tail.
