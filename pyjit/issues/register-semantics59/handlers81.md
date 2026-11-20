# Handlers81 and calls80 (H0–H5)

Integration record, 2026-11-20. Branch `pyjit/handlers81` (`d6d1ed5ac93`,
pushed), from checkpoint78 (`99a04733128`). Worktree `BASE/worktrees/handlers81`;
builds `BASE/handlers81-{release,debug}`. Evidence and agent logs are in
`E59/handlers81/`, and the running log is `E59/handlers81/coordinator.md`.

The rollout follows the [handlers81 proposal](../../design/proposals/handlers81.md)
and then [operand-encoded calls](../../design/proposals/operand-calls80.md).
The user approved both with no performance gate. The
[plan](../interpreter-performance56/plan.md#current-rollout-handlers81-then-calls80)
has one row per step.

## Contents

| Step | Head | What it does |
| --- | --- | --- |
| H0 | `ea164548495` | `Tools/pyjit/handler_shape.py`, checkpoint78 baseline |
| H1a, H1b | `3f6cb5e89aa`, merged | Values canonical by construction; no `phase_state` in handlers |
| H2 | `81a2cea8cc2`..`aa0b5f87f7d` | Effect, detach, store, release in every handler; the discard slot; frame clear in upstream's order |
| H3 | `cfcec785504` | No bridge records or canaries in handlers |
| H4 | `744d4ad992a` | One computed-goto evaluator (`ir_evaluate`); tail-call backend deleted; 77 rare or generic handlers outlined |
| H5 step 1 | `d2a24599732` | Every Python child frame at the arena top; `saved_top` replaced by `_PyStackChunk.value_top` |
| H5 step 2a | `600b4237187` | `IR_OPERANDS` words; one instruction length (`_PyIR_WordLength`) for every walker |
| H5 steps 2b/2c | `8e70a4cdf82` | Every call's inputs are trailing operand words; the window form is deleted; handlers stay generated from `bytecodes.c` |
| H5 step 3 | `1337df6ded3` | Constant slots for every inline or immortal register input, call operands included, at most eight per code object; `return None` reads the None slot |
| Tests | `6b7ec735fa2`, `d6d1ed5ac93` | Expectations follow the IR: a constant-slot read counts as the load of its value in `bytecode_helper` |

The pyc magic number is 64432.

H5c (`pyjit/handlers81-h5c`, `4f431e2a5f3`) is parked. It cut each constant
slot's cost to 2 instructions per call, but it raised the cost of the first
slot, and the kernels came out a wash.

## Timed all-29

Each figure is a paired time ratio, H5 over the control; lower means H5 is
faster. Each campaign has 2 rounds, 58 pairs and 116 invocations, runs on CPU 15
through `bench-run.sh`, and passed the core gate (`E59/handlers81/bench/`). The
checkpoint78 columns repeat that checkpoint's own campaigns of 2026-11-18
(`E59/execution78-checkpoint-all29-*`). Each column is a separate
campaign.

| Case | H5 / golden73 | H5 / upstream | H5 / checkpoint78 | c78 / golden73 | c78 / upstream |
| --- | ---: | ---: | ---: | ---: | ---: |
| richards | 0.940 | 0.822 | 0.972 | 0.964 | 0.845 |
| go | 0.876 | 0.981 | 0.903 | 0.976 | 1.102 |
| nbody | 0.793 | 0.684 | 0.888 | 0.906 | 0.852 |
| float | 0.959 | 0.724 | 0.996 | 0.954 | 0.715 |
| spectral_norm | 0.759 | 0.576 | 0.665 | 1.075 | 0.811 |
| raytrace | 0.925 | 0.896 | 0.953 | 0.994 | 0.953 |
| deltablue | 0.951 | 1.046 | 1.011 | 0.941 | 1.050 |
| chaos | 0.924 | 0.837 | 0.926 | 0.996 | 0.923 |
| nqueens | 0.919 | 1.145 | 0.929 | 1.006 | 1.231 |
| fannkuch | 0.960 | 1.183 | 0.950 | 1.005 | 1.207 |
| generators | 1.004 | 1.300 | 0.998 | 1.019 | 1.422 |
| comprehensions | 0.914 | 1.225 | 0.941 | 1.002 | 1.262 |
| coroutines | 0.921 | 1.066 | 0.927 | 0.992 | 1.044 |
| unpack_sequence | 1.000 | 1.205 | 1.027 | 0.989 | 1.242 |
| deepcopy | 0.962 | 1.316 | 0.967 | 1.017 | 1.370 |
| deepcopy_reduce | 0.982 | 1.333 | 0.969 | 1.013 | 1.429 |
| deepcopy_memo | 0.893 | 1.090 | 0.883 | 1.009 | 1.179 |
| json_dumps | 0.964 | 1.285 | 0.955 | 0.992 | 1.170 |
| json_loads | 0.976 | 1.168 | 1.012 | 0.949 | 1.155 |
| pickle | 0.996 | 1.123 | 1.008 | 0.973 | 1.119 |
| unpickle | 0.987 | 1.150 | 0.997 | 0.982 | 1.263 |
| regex_compile | 0.922 | 1.088 | 0.914 | 1.011 | 1.189 |
| logging_format | 0.945 | 1.217 | 0.964 | 0.976 | 1.289 |
| logging_silent | 1.051 | 0.807 | 1.312 | 0.820 | 0.645 |
| logging_simple | 0.944 | 1.217 | 0.968 | 0.997 | 1.271 |
| xml_etree_generate | 0.986 | 1.281 | 0.974 | 1.003 | 1.299 |
| xml_etree_iterparse | 1.020 | 1.294 | 0.956 | 1.048 | 1.308 |
| xml_etree_parse | 1.003 | 1.139 | 0.993 | 0.992 | 1.115 |
| xml_etree_process | 0.970 | 1.350 | 0.969 | 1.009 | 1.417 |
| **Geomean (29)** | **0.944** | **1.065** | **0.958** | 0.986 | 1.112 |

Against checkpoint78 in the same campaign, H5 is 4.2% faster on the
geomean. 24 of the 29 cases are faster. Five are slower:

| Case | H5 / checkpoint78 |
| --- | ---: |
| logging_silent | 1.312 |
| unpack_sequence | 1.027 |
| json_loads | 1.012 |
| deltablue | 1.011 |
| pickle | 1.008 |

The gap to upstream shrinks from 11.2% to 6.5%. H5 is faster than upstream
on richards, go, nbody, float, spectral_norm, raytrace, chaos and
logging_silent.

### logging_silent

The loss against checkpoint78 is cycles, not instructions, and it was
already present at H4. The figures below are on CPU 15, with the campaign
driver in timing mode, for 7 samples of 60,000 loops
(`bench/logging-silent-pmu.txt` and `bench/logging-silent-stalls.txt`).

| Build | Cycles | Instructions | Frontend stall cycles |
| --- | ---: | ---: | ---: |
| checkpoint78 | 2.01–2.05G | 7.58G | 141M |
| H4 | 2.43G | 7.28G | 313M |
| H5 | 2.43–2.46G | 8.02G | 280M |

Branch misses are flat at about 4.3M, and L1 instruction-cache misses
barely move: 1.19M at checkpoint78 and 1.22M at H5.

- H4 runs 3.8% fewer instructions than checkpoint78 but 21% more cycles.
- Frontend stalls account for about 45% of that increase. The likely cause is
  layout: one 300-label evaluator function replaced checkpoint78's
  per-handler functions.
- H5 adds about 10% more instructions but no further cycles.

This loop is sensitive to layout, as golden73's logging regression was. On
CPU 2 with other sessions active, the same builds rank differently
(`bench/logging-silent-attrib.txt`). The next diagnostic is the deferred H6
layout step: hot-handler ordering and alignment.

## Kernel instructions

Instructions per loop (`E59/handlers81/instr.sh`, perf stat on CPU 2):

| Kernel | checkpoint78 | H3 | H4 | H5 step 1+2a | H5 | upstream |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| deltablue | 50.57M | 45.02M | 45.59M | 45.62M | 46.60M | 39.03M |
| raytrace | 3,395M | 3,048M | 3,010M | 2,993M | 3,070M | 2,938M |
| richards | 585.9M | 514.1M | 518.6M | 520.9M | 537.6M | 539.5M |
| coroutines | 383.8M | 355.0M | 362.6M | 343.7M | 345.5M | 294.6M |
| deepcopy_memo | 449.7K | 385.1K | 377.0K | 378.5K | 402.8K | 346.4K |

Compared with step 1+2a, the H5 steps change kernel instructions as follows:

| Kernel | Steps 2b/2c alone | Step 3 alone | Merged |
| --- | ---: | ---: | ---: |
| deltablue | +2.4% | −0.2% | +2.1% |
| raytrace | +3.1% | −0.1% | +2.6% |
| richards | +3.7% | −0.6% | +3.2% |
| coroutines | +2.4% | −1.8% | +0.5% |
| deepcopy_memo | +2.5% | −0.2% | +6.4% |

The design expected operand calls to save 0.35–5%. The estimate used
checkpoint78's handler costs. Under those costs an argument load cost 25–36
instructions; after H4 it costs about what its operand fill costs:

- 3 instructions per operand word;
- 9 per local;
- 7 per temporary.

Three further costs remain:

- `CALL_PY_EXACT_ARGS`' fast path grew from 122 to 156 instructions.
- Result publication tests each operand.
- In merged H5, each constant slot costs 13–15 instructions per call even
  when unused, because the frame exit tests and closes it. That cost explains
  deepcopy_memo's extra 3.8%.

## Calls

Instructions per call, from `execution79-call-ledger/scripts/perstat.sh`
(`E59/handlers81/h5s2bc-root/per-call-final.txt`):

| Case | checkpoint78 | H4 | H5 step 1+2a | H5 |
| --- | ---: | ---: | ---: | ---: |
| `f(x, y)` | 519 | 454 | 460 | 452 |
| `o.m(x)` | 485 | 448 | 454 | 472 |
| bound method | 530 | 454 | 494 | 443 |
| default argument | 590 | 582 | 591 | 599 |
| keyword argument | 822 | 760 | 768 | 724 |
| `o.store(x)`, returns None | 584 | 527 | 533 | 550 |
| `__init__` | 1,678 | 1,477 | 1,443 | 1,430 |
| callee local dies | 1,052 | 813 | 819 | 837 |
| polymorphic site | 1,064 | 959 | 972 | 957 |
| `len(x)` | 300 | 244 | 244 | 259 |
| list-subclass append | 1,620 | 1,565 | 1,565 | 1,420 |

## Evaluator shape

Release `ceval.o`, from `handler_shape.py`:

| | H4 | H5 step 1 | H5 |
| --- | ---: | ---: | ---: |
| Allocator product (limit 50M) | 31M | 31.3M | 41.9M |
| Evaluator frame | 168 B | 168 B | 152 B |
| Evaluator instructions | — | 18,231 | 19,982 |
| Hot-set fast paths | 1,434 | 1,421 | 1,443 |
| Canaries | 0 | 0 | 0 |

Operand calls used a third of the allocator headroom. A later step that
adds handler text must check it.

## Qualification

- **Full suites** at `6b7ec735fa2` fail only the six known modules
  (`E59/handlers81/full-h5/`): test_external_inspection,
  test_generated_cases, test_ir_error_effects,
  test_pyvalue_native_call_transaction, test_sys_settrace and test_tools.
  Debug also fails test_profiling, a load flake.
- **The three IR-shape modules** that failed there at first (test_ast,
  test_compiler_assemble, test_peepholer) pass after `d6d1ed5ac93`. That
  commit changes only tests.
- **The 14 modules that use `bytecode_helper`** pass on both builds, except
  the known test_external_inspection.
- **Ladders** at each step, both roles: known failures only.
- **Execution79's 14 call cases** run clean under `-X dev` on both builds.

## Behaviour changes

These are all upstream-incidental behaviours that the user lets us change:

- A discarded result lives until the next discard or frame exit. This is
  visible in test_gc, test_unpack, unittest AsyncMock, test_ctypes and the
  multiprocessing heap test, whose expectations follow it.
- Finalization order follows upstream's clear order, and `INSTRUMENTED_POP_ITER`
  fires at a different time.
- INSTRUCTION events come per IR word. `return None` and a call's argument
  loads are no longer events, and a call's operand words never are.
- Line jumps that need a temporary's owner are refused with ValueError:
  test_sys_settrace's six known jump tests, plus
  `test_jump_backward_over_async_listcomp` and its `_v2`. A coroutine
  redirect has no pathwise NULL proof, so SEND's input owner is unknown at
  the target.
- `sys.getrefcount` of a native call's argument reads one lower once the site
  has specialized, because operands are borrowed. The last result of a call
  site stays in a dead temporary until it is overwritten.
- Sizes:
  - the frame header shrinks from 112 to 104 bytes;
  - `_Py_DebugOffsets` shrinks by 8 bytes;
  - the magic number moves from 64427 to 64432.
- A `CALL_KW` whose names tuple has a `co_consts` index of 4096 or more raises
  SyntaxError ("code object too large"), as other 12-bit IR fields do.
- `MAX_UOP_PER_EXPANSION` drops from 10 to 6, so some call macros no
  longer expand for tier 2. The JIT is off in every measurement.

## Follow-ups

These are recorded and not gated:

1. **Layout (H6).** The logging_silent frontend stalls and the evaluator's
   hot-handler order.
2. **Constant slots.** A code-level constant range or a slot layout would
   remove H5c's fixed first-slot cost. Then the exit can skip constant slots.
3. **Calls.**
   - Inline release at a fixed arity for the fixed-arity builtins.
   - A cheaper `CALL_PY_EXACT_ARGS` fill.
   - Delete the transaction-based push paths that remain for bound-method,
     `CALL_FUNCTION_EX`, instrumented and monitored calls.
4. **Earlier items.**
   - The outlined generic heads run at unspecialized sites.
   - The generator steps' calls.
   - H2's dead markers.
