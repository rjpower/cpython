# Checkpoint78: value fields paired with object slots

Integration record, 2026-11-18. Branch `pyjit/checkpoint78` (`99a04733128`,
pushed), worktree `BASE/worktrees/checkpoint78`, builds
`BASE/checkpoint78-{release,debug}`.

## Contents

Checkpoint78 is [checkpoint77](checkpoint77.md) plus `pyjit/slots78`. It
replaces the reverted complete value-slot table ([record](values77.md#the-value-slot-table-reverted)):

- **Rule.** The object slot is the definition. A value field is an optional
  native implementation of the same slot, valid only while it pairs with the
  object slot.
- **Dispatch** (`4e010467cf1`). A dispatcher calls the value field when it is
  non-zero. Otherwise it calls a cold, out-of-line per-kind adapter, passing
  the object slot it already loaded, and the adapter never re-reads a type.
  `binary_op1` and `ternary_op` therefore call exactly the slots they read
  before the first call, as upstream does.
- **Rich comparison** (`32c4d160c6e`). `do_richcompare` re-reads the operand
  types at each of its three steps, as upstream does. The closes of the
  immortal NotImplemented became assertions.
- **`PyType_Modified`** (`458d1372b2a`, `99a04733128`). The public entry
  clears each value field that no longer pairs with its object slot. That is
  the path an extension takes after storing a slot by hand. The interpreter's
  own invalidation runs on every class-attribute store, keeps pairs through
  `update_one_slot`, and skips the check. With the check, a class-attribute
  store took 3.5 times as long.
- **Tests** (`2ed0d070ce5`). `test_value_slots.SlotPairingTests` covers:
  - the review's stale-slot and freed-type cases (they match upstream);
  - a slot stored after ready, with and without `PyType_Modified`;
  - a replaced paired slot;
  - `next()` on the `co_branches()` iterator, a type that is never readied.

A slot stored after ready **without** `PyType_Modified` is called only when
its row has no native pair. A paired row keeps its old native until
`PyType_Modified`, which the C API requires after such a store.

Sizes, release, in bytes:

| Function | checkpoint77 | checkpoint78 | Reverted table |
| --- | ---: | ---: | ---: |
| `_PyValue_GetAttr` | 883 | 281 | 258 |
| `_PyValue_SetAttr` | 1,520 | 439 | 419 |
| `_PyValue_DescrGet` | 447 | 53 | 30 |
| `_PyValue_RichCompare` | 1,368 | 1,438 | 1,422 |

`_PyValue_RichCompare` is still far above upstream's 907 bytes. The design
review attributes the difference to register spills across its live set and
to the operand-type decoding.

Instructions per loop against checkpoint77 (`E59/execution78-checkpoint-instructions.txt`):
raytrace +0.2%, pickle +0.8%, xml_etree_process −0.2%; DeltaBlue,
json_dumps and deepcopy are unchanged.

## Qualification

Full suite at `99a04733128`, both roles (`E59/checkpoint78b-{release,debug}-*-full.log`).
Each role ran 667 of 678 files and failed only the six known ones. In the
release run `test_gc_verify` also failed once. It passed six of six runs
alone on checkpoint77 and checkpoint78, and earlier full suites list it as
intermittent.

## Performance

All-29 campaigns as for checkpoint77 (`E59/execution78-checkpoint-all29-*`).
The golden73 campaign rejected 2 invocations and the upstream campaign 28,
while debug suites ran on the other L3 domain; all were rerun.

| Workload | 78 / golden73 | 77 / golden73 | 76 / golden73 | 78 / upstream | 77 / upstream | 76 / upstream |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| richards | 0.964 | 0.936 | 0.985 | 0.845 | 0.819 | 0.865 |
| go | 0.976 | 0.970 | 0.990 | 1.102 | 1.099 | 1.119 |
| nbody | 0.906 | 0.927 | 0.943 | 0.852 | 0.783 | 0.814 |
| float | 0.954 | 0.965 | 0.993 | 0.715 | 0.733 | 0.748 |
| spectral_norm | 1.075 | 1.007 | 0.973 | 0.811 | 0.761 | 0.735 |
| raytrace | 0.994 | 0.979 | 1.010 | 0.953 | 0.951 | 0.959 |
| deltablue | 0.941 | 0.955 | 1.118 | 1.050 | 1.076 | 1.191 |
| chaos | 0.996 | 0.973 | 0.980 | 0.923 | 0.910 | 0.908 |
| nqueens | 1.006 | 0.998 | 1.008 | 1.231 | 1.238 | 1.247 |
| fannkuch | 1.005 | 0.990 | 0.995 | 1.207 | 1.193 | 1.191 |
| generators | 1.019 | 1.007 | 1.001 | 1.422 | 1.267 | 1.167 |
| comprehensions | 1.002 | 1.001 | 0.988 | 1.262 | 1.351 | 1.288 |
| coroutines | 0.992 | 1.077 | 1.063 | 1.044 | 1.254 | 1.234 |
| unpack_sequence | 0.989 | 1.017 | 1.001 | 1.242 | 1.237 | 1.242 |
| deepcopy | 1.017 | 1.009 | 1.029 | 1.370 | 1.369 | 1.389 |
| deepcopy_reduce | 1.013 | 1.007 | 1.018 | 1.429 | 1.356 | 1.377 |
| deepcopy_memo | 1.009 | 1.008 | 1.044 | 1.179 | 1.225 | 1.290 |
| json_dumps | 0.992 | 0.999 | 1.005 | 1.170 | 1.355 | 1.371 |
| json_loads | 0.949 | 0.980 | 0.952 | 1.155 | 1.207 | 1.159 |
| pickle | 0.973 | 0.972 | 0.980 | 1.119 | 1.084 | 1.106 |
| unpickle | 0.982 | 0.948 | 0.959 | 1.263 | 1.175 | 1.173 |
| regex_compile | 1.011 | 1.000 | 1.019 | 1.189 | 1.189 | 1.215 |
| logging_format | 0.976 | 0.996 | 0.973 | 1.289 | 1.156 | 1.262 |
| logging_silent | 0.820 | 0.944 | 1.069 | 0.645 | 0.743 | 0.829 |
| logging_simple | 0.997 | 1.010 | 1.026 | 1.271 | 1.234 | 1.233 |
| xml_etree_generate | 1.003 | 1.022 | 1.015 | 1.299 | 1.333 | 1.317 |
| xml_etree_iterparse | 1.048 | 1.063 | 1.055 | 1.308 | 1.312 | 1.318 |
| xml_etree_parse | 0.992 | 0.990 | 0.992 | 1.115 | 1.138 | 1.108 |
| xml_etree_process | 1.009 | 1.020 | 1.030 | 1.417 | 1.441 | 1.440 |
| **Geomean** | **0.9856** | **0.9915** | **1.0067** | **1.1123** | **1.1167** | **1.1292** |
| Faster than control | 17 | 16 | 13 | 7 | 7 | 7 |

The upstream campaigns are noisier per workload than the golden73
campaigns. json_dumps moves from 1.355 to 1.170 against upstream between
checkpoints 77 and 78, while its ratio against golden73 moves from 0.999 to
0.992. Single upstream ratios carry roughly ±10% campaign-to-campaign
variation; the geomean moves by less than 0.5%.
