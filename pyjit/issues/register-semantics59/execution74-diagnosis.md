# Execution74: logging cause, layout sensitivity and the upstream gap

Bounded diagnosis of golden73 `44f066f5fd5`, 2026-11-17/18. It changes no
runtime source and selects nothing. Controls: selected57 `0dcba75bb3d` and
upstream `3a440b65695`, all on CPU 15 with hash seed 0 and the JIT disabled.

## Logging's golden73 regression is code placement

Per hot loop of `logging_silent`, mean of two stat runs per role:

| Event | Upstream | Selected57 | Golden73 |
| --- | ---: | ---: | ---: |
| cycles | 4,982 | 3,232 | 3,765 |
| instructions | 13,944 | 13,492 | 13,502 |
| µops from legacy decoder | 4,746 | 18 | 5,751 |
| µops from op cache | 8,159 | 12,358 | 6,621 |
| branch mispredictions | 2.0 | 2.0 | 2.0 |
| I-cache 32-byte fetch misses | 40.0 | 0.0 | 0.0 |

Golden73 retires the same instructions as selected57 but delivers almost half
its µops from the legacy decoder instead of the Zen 2 op cache. High-rate cycle
samples put the whole 532-cycle increase in three handlers:
`CALL_PY_GENERAL` +279, `POP_JUMP_IF_FALSE_BOOL` +148 and `RETURN_VALUE_EMPTY`
+60. The first two are instruction-for-instruction identical in both builds;
only their addresses moved.

To test causation, `exec74-layout-probe-release` rebuilds golden73's exact
source in a copied build directory, adding only `-falign-functions=64` to
`OPT`. Three rotated rounds per role:

| Per loop | Selected57 | Golden73 | Probe73 |
| --- | ---: | ---: | ---: |
| cycles | 3,328 | 3,787 | 3,213 |
| instructions | 13,492 | 13,502 | 13,532 |
| µops from legacy decoder | 195 | 5,766 | 11 |

The probe is 15.2% faster than golden73 on logging and 3.5% faster than
selected57. The logging regression accepted at golden73 selection is not a
cost of boundary73's design.

## Placement moves most workloads

The two-pair all-29 probe73/golden73 campaign (`execution74-probe73-all29-golden73`)
changes no executed work, yet individual workloads move from **0.872**
(logging_silent) to **1.086** (DeltaBlue). Spectral norm moves 1.063,
comprehensions 1.040, unpack_sequence 0.928, json_dumps 0.959 and nqueens 0.973.
The geometric mean is **0.9994** with 15 wins. `go` (0.988–1.312) is unstable.

Every screen since round67 applies per-workload protection limits of a few
percent to one build per role. A placement effect of this size can decide
those predicates on its own. Logging appears in about 25 report tables at
1.00–1.60, with tight paired ranges. That pattern fits per-build placement
rather than run-to-run noise. This note does not re-adjudicate earlier
rejections, which had other failing predicates too.

## Golden73 versus upstream

Two-pair all-29 campaign, `execution74-golden73-all29-upstream`; geometric
mean **1.1355**, 7 wins of 29. Part of the first round overlapped another
session's build; every invocation passed the load gate then in force.

| Faster than upstream | Ratio | Slower than upstream | Ratio |
| --- | ---: | --- | ---: |
| logging_silent | 0.731 | xml_etree_parse | 1.594 (1.396–1.820) |
| float | 0.745 | xml_etree_process | 1.396 |
| spectral_norm | 0.753 | deepcopy | 1.388 |
| nbody | 0.869 | logging_simple | 1.366 |
| richards | 0.876 | deepcopy_reduce | 1.361 |
| chaos | 0.953 | json_dumps | 1.335 |
| raytrace | 0.973 | comprehensions | 1.331 |
| | | fannkuch | 1.267 |
| | | logging_format | 1.266 |
| | | generators | 1.250 |

The remaining cases fall between 1.087 and 1.233: deepcopy_memo, unpickle,
json_loads, nqueens, regex_compile, coroutines, unpack_sequence, go, deltablue,
pickle and xml_etree_iterparse.

## Where upstream's instructions go

`execution74-upstream-attribution` records retired-instruction samples at
9,999 Hz for six workloads. Golden73 attributes by handler symbol. Upstream
maps `_PyEval_EvalFrameDefault` addresses to opcodes through DWARF lines in
`generated_cases.c.h`. Each cell is golden73 minus upstream, as a percentage of
upstream's total instructions per loop:

| Workload | Call/return/frame | Generator/await | Attribute | Data movement + control | Other | Total |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| deltablue | +16.6 | 0.0 | +18.3 | +1.2 | +0.2 | +36.3 |
| comprehensions | +3.4 | +3.4 | +2.4 | +12.0 | +22.7 | +43.9 |
| generators | +2.0 | +31.7 | +1.8 | −12.2 | +10.3 | +33.7 |
| coroutines | +7.6 | +17.1 | 0.0 | +4.6 | −0.6 | +28.7 |
| nqueens | 0.0 | +10.0 | −0.4 | +10.8 | +14.1 | +34.6 |
| raytrace | +13.1 | −0.7 | +3.9 | +4.4 | −0.5 | +20.3 |

"Other" includes allocation, native calls and helpers.

The register IR removes most data movement: DeltaBlue's load/store/copy handlers
fall from 6.8% to 1.4% of upstream's total. The same operations then cost more
per execution: FOR_ITER_LIST 2.15×, LOAD_ATTR_CLASS 2.0×,
LOAD_ATTR_METHOD_WITH_VALUES 1.9× and CALL_METHOD_DESCRIPTOR_FAST 2.0×
upstream's instructions. Static reading of the hot paths shows fixed costs on
each handler:

- A frame-pointer prologue and `leave`.
- A `frame->instr_ptr` store.
- Three instructions to decode each 12-bit operand.
- A tag test before each count update.
- A code-object load and compare that decides whether the destination is a local.
- An out-of-line `PyList_GET_VALUE` call in FOR_ITER_LIST, with three spills.

Both builds use `-fno-omit-frame-pointer -mno-omit-leaf-frame-pointer`. Upstream
pays its frame setup once per evaluation rather than once per handler.

## Evidence

All paths are under `E59`.

- `execution74-logging-frontend{,-driver.py,-perf-wrapper.sh}`: Zen 2 counters and flat samples.
- `execution74-logging-decoder{,-driver.py}`: decoder and op-cache µop samples.
- `execution74-logging-layout-probe{,-driver.py}`: probe counters.
- `execution74-probe73-all29-golden73` and `execution74-golden73-all29-upstream`: timing campaigns.
- `execution74-upstream-attribution{,-driver.py}`: counts, samples and `attribution-*.json`.
- `execution74-core-gate-check`: test of the core-local harness gate under load.
- `execution74-probe73-all29-upstream`: failed at its first invocation on the old load gate; retained.

The probe build directory is `BASE/exec74-layout-probe-release`.
