# Checkpoint76: round 3 call path and the first value-rule pass

Integration record, 2026-11-18. Branch `pyjit/checkpoint76` (pushed), worktree
`BASE/worktrees/checkpoint76`, builds `BASE/checkpoint76-{release,debug}`.
Evidence: `E59/checkpoint76*-*.log`, `E59/execution76-checkpoint-*`.

## Contents

| Commit | Content |
| --- | --- |
| `ba2d7938541` | Round 3 call path (`pyjit/calls76`, [ledger](execution76-call-ledger.md#round-3-result)) |
| `0d18fff7a07` | Value rules 1–3, first pass (`pyjit/values76`) |
| `76fc2b68918` | Merge; regeneration leaves products unchanged |
| `2e9cb020e2e` | The value-API self-test passes the word and field to `IR_STORE_DST` (stale since round 2) |
| `013fb42c922` | Handlers keep the frame chain in frame-pointer builds |

Magic is unchanged (64428).

## Frame chain

The full suite exposed `test_c_stack_unwind`, which had failed since round 1
and was not in the focused sets. Round 1 compiled `ceval.c` with
`-fomit-frame-pointer`. The handlers then used `rbp` as an ordinary register,
and a frame-pointer unwinder found 3 of 10 Python levels. In a build
configured with frame pointers, `CFLAGS_HANDLERS` now adds `-ffixed-rbp`:
handlers still have no prologue but leave `rbp` alone, so the functions they
call link their frames to the interpreter entry's. Upstream instead keeps
frame pointers in every handler.

## Qualification

Full suite at `013fb42c922`, both roles (`E59/checkpoint76c-{release,debug}-*-full.log`):
667 of 678 files ran in each role. Six fail, all known:

- in golden73 too: `test_external_inspection`
  (`test_remote_delegated_discard_still_follows_receiver`) and `test_tools`
  (`test_raw_bits`);
- `test_ir_error_effects` (five subtests) and `test_generated_cases` (two
  methods);
- `test_sys_settrace`'s six debugger-jump errors (golden73 too);
- `test_pyvalue_native_call_transaction`
  (`test_exact_tuple_preparation_all_routes`, round 2 too).

## Performance

All-29 two-pair campaigns on CPU 15 with CPUs 7 and 15 reserved:
`E59/execution76-checkpoint-all29-{golden73,upstream}`. The golden73 campaign
rejected 2 invocations, the upstream campaign 8; all were rerun. Ratios are
candidate time over control time, so lower is better. The round 2 columns
come from `execution75-handlers-r2-all29-*`, measured before the reservation.

| Workload | Checkpoint76 / golden73 | Round 2 / golden73 | Checkpoint76 / upstream | Round 2 / upstream |
| --- | ---: | ---: | ---: | ---: |
| richards | 0.985 | 1.011 | 0.865 | 0.874 |
| go | 0.990 | 0.976 | 1.119 | 1.095 |
| nbody | 0.943 | 0.972 | 0.814 | 0.845 |
| float | 0.993 | 0.987 | 0.748 | 0.749 |
| spectral_norm | 0.973 | 1.043 | 0.735 | 0.781 |
| raytrace | 1.010 | 1.021 | 0.959 | 0.976 |
| deltablue | 1.118 | 1.048 | 1.191 | 1.141 |
| chaos | 0.980 | 0.992 | 0.908 | 0.947 |
| nqueens | 1.008 | 1.006 | 1.247 | 1.239 |
| fannkuch | 0.995 | 0.981 | 1.191 | 1.232 |
| generators | 1.001 | 1.030 | 1.167 | 1.299 |
| comprehensions | 0.988 | 0.979 | 1.288 | 1.288 |
| coroutines | 1.063 | 1.078 | 1.234 | 1.252 |
| unpack_sequence | 1.001 | 0.993 | 1.242 | 1.205 |
| deepcopy | 1.029 | 1.016 | 1.389 | 1.388 |
| deepcopy_reduce | 1.018 | 1.013 | 1.377 | 1.381 |
| deepcopy_memo | 1.044 | 1.015 | 1.290 | 1.244 |
| json_dumps | 1.005 | 1.001 | 1.371 | 1.360 |
| json_loads | 0.952 | 0.991 | 1.159 | 1.220 |
| pickle | 0.980 | 1.009 | 1.106 | 1.136 |
| unpickle | 0.959 | 0.986 | 1.173 | 1.198 |
| regex_compile | 1.019 | 0.996 | 1.215 | 1.196 |
| logging_format | 0.973 | 1.008 | 1.262 | 1.281 |
| logging_silent | 1.069 | 1.048 | 0.829 | 0.832 |
| logging_simple | 1.026 | 1.005 | 1.233 | 1.171 |
| xml_etree_generate | 1.015 | 1.012 | 1.317 | 1.311 |
| xml_etree_iterparse | 1.055 | 1.032 | 1.318 | 1.290 |
| xml_etree_parse | 0.992 | 1.003 | 1.108 | 1.144 |
| xml_etree_process | 1.030 | 1.012 | 1.440 | 1.432 |
| **Geomean** | **1.0067** | 1.0088 | **1.1292** | 1.1383 |

Checkpoint76 beats golden73 on 13 of 29 workloads and upstream on 7.
Numerical kernels improve most:

- spectral_norm 1.043 → 0.973;
- nbody 0.972 → 0.943;
- json_loads 0.991 → 0.952;
- unpickle 0.986 → 0.959.

DeltaBlue regresses from 1.048 to 1.118, although round 3 removed 4.4% of its
instructions. deepcopy_memo, logging_silent and xml_etree_iterparse regress
2–3%. The PMU campaign (`E59/execution76-checkpoint-pmu`) stopped at its first
invocation: its protocol rejects runs at a one-minute load of 4, and the
full suite was running on the other CPUs. It will be rerun into a new directory.

### DeltaBlue: placement, not work

Per DeltaBlue loop, CPU 15, (30 − 10 loops)/20, three repetitions
(`E59/execution76-checkpoint-frame-chain`). The variant builds recompile only
`ceval.o` with different handler flags.

| Build | Handler flags | Cycles | Instructions | Op-cache µops | Decoder µops |
| --- | --- | ---: | ---: | ---: | ---: |
| Round 2 | no frame pointer | 18.2M | 52.90M | | |
| Round 3 | no frame pointer | 15.6M | 50.46M | 26.3M | 20.6M |
| Round 3 | reserved `rbp` | 17.4M | 50.74M | 12.0M | 35.4M |
| Round 3 | frame pointer (upstream) | 16.6M | 52.32M | 20.9M | 28.0M |
| values76 | no frame pointer | 17.3M | 52.76M | | |
| Checkpoint76 | no frame pointer | 16.0M | 50.33M | 22.8M | 23.9M |
| Checkpoint76 | reserved `rbp` | 18.9M | 50.61M | 14.6M | 33.8M |

Round 3 alone runs DeltaBlue in 14% fewer cycles than round 2. Reserving `rbp`
adds 0.5% instructions and 3–4% loads and stores, but moves code so that
op-cache delivery halves. Frame pointers in every handler add 3.7%
instructions. The reserved register is the cheapest correct form; its cycle
cost here is placement, the same mechanism behind round 1's regressions
([handlers75](handlers75.md)). Until hot-code placement is controlled, a
single-workload time change cannot be attributed to the change that caused it.
