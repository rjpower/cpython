# IP: where the rir90 interpreter's time goes

The evidence files it names (`table1.md`, `counts.md`, `account*.md`,
`geo29.md`, `micro/`, `sym/`, `hand/`, the scripts) are in the rir90 build
tree at `register-semantics59/rir90/ip/`.

Unit IP, 2026-11-26. Build `rir90ip-release` (`d8cb2109192`) against
`golden87-release` (`684a98eb30e`). JIT off on both, user-space `perf`, CPUs 0-3.
This unit diagnoses only; it merges nothing.

## Conclusions

My all-29 run reproduces root's figure: **1.563** of golden87's cycles (root
measured 1.559), at **1.407** of its instructions. So about 11% of the gap is
lost IPC and the rest is extra instructions.

Neither the word count nor the dispatch causes the gap:
- rir90 executes as many words as golden87 does, or fewer: 1.00× on
  spectral_norm, richards, logging_silent and generators, 0.74× on nbody,
  0.50× on unpack_sequence (Py_STATS).
- The dispatch sequence is the same six instructions in both builds: load
  the word, mask 9 bits, load the table entry, jump, plus `endbr64` and the
  `instr_ptr` store.

Golden87 runs the same words with smaller handler bodies. The causes, ranked
by expected gain on the all-29 cycle geomean:

| # | Cause | Kernels | Evidence (numbers below) | Design change | Expected gain |
|---|---|---|---|---|---|
| 1 | **The compiler outlines `static inline` helpers.** GCC exhausts `inline-unit-growth` in `ceval.c`, which holds the interpreter, the function forms and the stubs (35k generated lines). So `PyValue_XCLOSE` (484 call sites in `ir_evaluate`; golden87 has 7), `_PyRop_FloatBinary`, `_PyRop_RangeStep`, the list rows, `_PyFrame_ClearSlots`, `_PyThreadState_PushFrameUnchecked` and others become calls, and the owners are spilled around each call. | 25 of 29 (all but the C-heavy ones) | `--param=inline-unit-growth=1000` on `ceval.o` alone takes the geomean from **1.563 to 1.396** (instructions 1.407 to 1.349). nbody 2.99→2.28, spectral 2.24→1.85, richards 1.61→1.36, chaos 2.01→1.65 | The generator declares the inlining of hot leaf rows and value-API calls (`Py_ALWAYS_INLINE` from the row's declared `inline` contract) instead of relying on GCC's unit budget. It drops the closes of owners that are NULL by construction (see 2), which removes most of the call sites. The forms and stubs move to their own translation units. Blanket inlining makes things worse (see below), so the inlining must be targeted. | **10.7% measured** (lower bound: some sites, such as GetItem's list rows, stay outlined) |
| 2 | **Arm bodies are heavier even when inlined.** After fix 1, each arm still costs 7-22 cycles more than golden87's arm for the same word. The generated C keeps `this_instr`, `own_*` and `old_dst` live across exits. It passes spans as address-taken compound literals. Its frame is 10 KB (golden87's is 152 bytes), with 3.3× the stack-slot references (4131 against 1257), so it spills and reloads on the dispatch path. It closes owners that a guard has already proved to be immortal or inline (a bool, a float, an int). It reads mode operands at run time: the branch sense in `BranchTruth.*`, the operator in `Binary.Float` and `Binary.Int` (two switches and a jump table). | all pure-Python | Microbenchmarks, cycles per arm above the loop, with inlining fixed against golden87: BranchTruth.Bool 20.8 vs 3.3; GetItem.ListIndex 29.7 vs 7.3; Binary.Float(mul) 18.7 vs 5.4; Add.Float 17.3 vs 5.6; GetAttr.Shape 18.0 vs 5.6; Add.Int 13.4 vs 6.4. In the kernels, the ir_evaluate excess remains the largest category after fix 1 (nbody +208M cycles/run, spectral +130M, richards +70M, logging_silent +70M) | Tight arm bodies from the generator: facts from guards elide closes and type tests (a guarded bool, float or inline int has no owner to close). The generator emits scalar locals with no address-taken aggregates. Error and deopt exits recompute the word address instead of keeping it live. Mode selectors become handler numbers, one arm per operator or branch sense, as the plan already states for immutable mode operands. The guard and the compute share one operand decode (no `Applies` followed by a second decode). | **largest remaining item: an estimated 10-15% (upper bound about 19% if the bodies matched golden87's)** |
| 3 | **Unpack goes through a heap owner buffer, and span geometry is decoded at run time.** `_PyRop_UnpackTuple`/`UnpackList` `PyMem_Malloc` a buffer, fill it in reverse, and `_PyRop_StoreOwners` decodes each destination with `_PyIR_SpanElement` (a format lookup and a role loop per element). `_PyRop_CloseOwners` then closes the displaced values and frees the buffer, and `_PyIR_SpanWordLength` computes the word length on every execution. Golden87 writes the items straight into the locals. | unpack_sequence, nbody, spectral_norm (any unpacking loop) | Per unpack, instructions (cycles): 2-tuple 374 (103) vs 108 (18); 10-tuple 822 (205) vs 302 (66). That is all of unpack_sequence's excess (+1.67G instructions/run, malloc/free +0.41G of it), +0.23G on nbody and +0.16G on spectral | Unpack arms publish directly into the destination homes. The destination slots and their order are fixed at lowering, with no buffer, no second pass and no reversal at run time. The generator emits constant word lengths and element positions for each arm's declared layout, so no format scan runs at run time. | **about 5%** (unpack_sequence 2.94→~1.05 alone gives 3.5%) |
| 4 | **The call protocol.** Python functions that golden87 runs as `CALL_PY_GENERAL` go to `Call.Generic` in rir90 (logging_silent 600k per run, richards 219k). Each such call runs `_PyRop_PythonTarget`, `_PyRop_PushPython` and `_PyBindPlan_ForCall`. The frame push, clear and return-value helpers are outlined (cause 1). | logging_*, richards, deltablue, go, raytrace | Call.Exact+Return.Empty: 322 vs 266 instructions per call. logging_silent call/frame excess: +238M instructions/run, of which +120M remain with fix 1 | A `Call.PyGeneral` arm: the old specialization has no arm, which the port rule counts as a defect. The frame push and return are inlined by declared contract. | about 1-1.5% |
| 5 | **The generator protocol.** `Move.Retire` (one per generator return) resolves the parallel move at run time: it clears a 512-byte bitset, scans for pending sources in O(n²) and decodes the spans, about 490 instructions per move. Iterating a generator with a `for` loop goes through `ForIter.Slot`, which reaches `tp_iternext` and a nested `ir_evaluate` activation; golden87 has `FOR_ITER_GEN`. | generators, coroutines, comprehensions | generators: Move ≈ +100M of +353M instructions/run; generator and iteration excess +175M instructions and +67M cycles per run with fix 1 | The translator sequences parallel moves at lowering (F3's exit condition was 0 Moves) or coalesces the homes. A `ForIter.Gen` arm that pushes the generator frame. | about 0.5-1% |
| 6 | First-call lowering | all | Steady state: 0, because the lowering symbols do not appear in steady-state runs. Once per process: +24-26M instructions more than golden87 (+85M logging, +136M pickle, +172M generators, which includes warm-up) | none needed | 0 |
| 7 | Dispatch and operand decode | all | Same instruction sequence in both builds. Word counts are equal or lower | none | 0 |

Lost IPC follows mostly from cause 1. The cycle-to-instruction gap shrinks
from 11% to 3.5% with inlining fixed. The rest of the IPC loss comes from the
spills on the dispatch path in cause 2 and from the per-operator jump table:
nbody has 2.39M branch misses per run against 1.08M.

**Causes shared by most kernels:** 1 and 2, in every pure-Python kernel.
Causes 3-5 are specific to their kernels.

**One caution about fixing cause 1:** aggressive inlining is worse than the
single parameter. Adding large-function-growth, max-inline-insns-single and
max-inline-insns-auto, and early-inlining-insns gives a geomean of 1.467
against 1.396, with generators at 2.67 and nqueens at 2.19. Declared,
targeted inlining is the fix, not a global flag.

## Evidence

All files are in `R/ip/`. Kernels run through `drive.py`, which runs one
warm-up call and then RUNS calls. Per-run figures are the difference between
RUNS=5 and RUNS=0, divided by 5, averaged over two repeats. The runs used
`taskset` on CPUs 2/3 (step 1) and on 0/1 with 8 and 9 idle (later).

### Step 1: seven kernels (`table1.md`, `stat-*.csv`)

| kernel | golden87 instr | rir90 instr | ratio | golden87 cycles | rir90 cycles | ratio | IPC g/r |
|---|---|---|---|---|---|---|---|
| nbody | 827M | 1724M | 2.09 | 261M | 782M | 3.00 | 3.17/2.20 |
| unpack_sequence | 997M | 2666M | 2.67 | 229M | 673M | 2.94 | 4.36/3.96 |
| spectral_norm | 768M | 1354M | 1.76 | 237M | 530M | 2.24 | 3.24/2.56 |
| generators | 576M | 929M | 1.61 | 220M | 422M | 1.91 | 2.61/2.20 |
| richards | 768M | 1136M | 1.48 | 327M | 471M | 1.44 | 2.35/2.41 |
| logging_silent | 761M | 1018M | 1.34 | 263M | 379M | 1.44 | 2.89/2.68 |
| pickle | 1183M | 1194M | 1.01 | 446M | 467M | 1.05 | 2.65/2.55 |

The per-symbol split is in `sym/<build>-<kernel>-{instructions,cycles}.txt`
and the per-handler split of `ir_evaluate` in `hand/`. `handlers.py` maps
samples to the nearest preceding `opcode_targets` label, which is reliable for
the release build only: inlining changes the block layout.

### Step 2: words and instructions per word (`counts.md`, from Py_STATS builds)

| kernel | golden87 words | rir90 words | golden87 instr/word | rir90 instr/word | golden87 ir_evaluate/word | rir90 ir_evaluate/word |
|---|---|---|---|---|---|---|
| nbody | 17.56M | 12.92M | 47.1 | 133.5 | 38.1 | 77.7 |
| unpack_sequence | 6.46M | 3.26M | 154 | 818 | 59.0 | 130.0 |
| spectral_norm | 11.57M | 11.57M | 66.4 | 117.0 | 58.9 | 63.0 |
| generators | 11.61M | 11.81M | 49.6 | 78.7 | 38.6 | 42.8 |
| richards | 10.69M | 10.68M | 71.9 | 106.3 | 59.2 | 68.4 |
| logging_silent | 7.32M | 7.32M | 103.9 | 139.0 | 72.8 | 70.8 |

Outside nbody and unpack, the inline part of each word costs about the same
as in golden87. The excess sits in the routines that the handlers call, which
is cause 1, and in rows that golden87 does not have, which are causes 3-5.

**Dispatch** (objdump, both builds):
`mov 0x8(%r),%rW; lea opcode_targets; mov %rW,%rdx; and $0x1ff,%edx; jmp *(%rax,%rdx,8)`.
Operand fields decode as shift and mask in both builds.

**Hot arms.** `Binary_Float` (release) is 63 instructions plus calls to
`_PyRop_FloatBinaryApplies` (22 instructions on the hot path),
`_PyRop_FloatBinary` (a switch through a jump table, `notrack jmp *%rax`) and
3× `PyValue_XCLOSE`. It keeps `own_x`, `own_y` and the word address in stack
slots at -0x27a0 to -0x27d0(%rbp). Golden87's `BINARY_OP_MULTIPLY_FLOAT` is
55 instructions with no call and no stack slot, and it closes no operands,
because a guarded float has no owner.

`ForIter_Range` calls `_PyRop_RangeStep` (declared `static inline`), spills
four registers around the call, and then reloads `next_instr` from the stack
for the dispatch. Golden87's `FOR_ITER_RANGE` steps the range inline.

`Jump_Poll` always calls `check_periodics`, where golden87 tests the flag
inline.

`BranchTruth_Bool` reads the branch sense from operand C, adds a guard on it,
and calls `XCLOSE` on a bool. Golden87's `POP_JUMP_IF_TRUE_BOOL` only clears
the home.

**Why GCC outlines:** `-fopt-info-inline-missed` on `ceval.c` reports, for
`ir_evaluate` → `PyValue_XCLOSE`, 378 sites with "inline-unit-growth limit
reached", 438 with "early-inlining-insns", and 115 with "call is unlikely".
The same reasons appear for `_PyRop_AnchorForSrcs`, `_PyIR_SpanWordLength`
and `_PyRop_FloatBinary*`.

### Microbenchmarks (`micro/`): per iteration of a 1M loop, instructions/cycles

The word counts per iteration are the same in both builds (Py_STATS). Each
loop runs `ForIter.Range`, `Jump.Poll` and the arm under test.

| case | golden87 | rir90 | rir90 + inline-unit-growth |
|---|---|---|---|
| empty loop | 65.9/20.3 | 107.9/32.0 | 92.9/26.6 |
| `z = x * y` (Binary.Float) | 116.0/25.7 | 226.0/64.3 | 179.0/45.3 |
| `z = x + y` (Add.Float) | 116.0/25.9 | 198.0/58.1 | 156.9/43.9 |
| `z = i + k` (Add.Int) | 114.0/26.7 | 163.0/43.2 | 152.9/40.0 |
| `z = L[j]` | 125.0/27.6 | 217.0/59.1 | 205.0/56.3 |
| `L[j] = x` | 128.0/28.9 | 226.0/58.7 | 214.9/59.7 |
| `f()` (Call.Exact + Return.Empty) | 332.0/74.3 | 430.0/119.9 | 415.0/115.1 |
| `z = o.a` (GetAttr.Shape) | 117.0/25.9 | 183.0/46.4 | 172.0/44.6 |
| `a, b = t` | 174.0/38.6 | 486.4/138.8 | 467.4/129.2 |
| 10-way unpack | 368.0/85.8 | 934.2/236.2 | 915.2/231.8 |
| `if flag:` (BranchTruth.Bool) | 97.9/23.6 | 155.0/44.4 | 144.0/47.4 |
| `x*y - w*x + y` | 260.0/62.4 | 564.0/208.0 | 426.0/120.4 |

### Steps 3-4: excess by cause, per run (`account.md`; with fix 1: `account-inl.md`)

The figures are rir90 minus golden87, in M per run.

| cause | nbody | unpack | spectral | generators | richards | logging_silent | pickle |
|---|---|---|---|---|---|---|---|
| ir_evaluate bodies, instructions | +333 | +44 | +47 | +57 | +98 | -15 | 0 |
| outlined static-inline helpers, instructions | +366 | +180 | +278 | +62 | +180 | +11 | 0 |
| unpack owner buffer + malloc/free, instructions | +228 | +1433 | +161 | 0 | 0 | 0 | +2 |
| call, frame push, return, instructions | 0 | 0 | +78 | +50 | +87 | +238 | +3 |
| generator and iteration protocol, instructions | +8 | 0 | +22 | +175 | +5 | 0 | 0 |
| total instructions | +897 | +1669 | +586 | +353 | +367 | +257 | +10 |
| total cycles | +521 | +445 | +293 | +201 | +144 | +116 | +21 |
| with fix 1: ir_evaluate bodies, cycles | +208 | +62 | +130 | +63 | +70 | +70 | +2 |
| with fix 1: total cycles | +323 | +430 | +207 | +165 | +82 | +118 | +15 |

### The all-29 confirmation (`geo29.md`, `stat29-*.csv`)

These are cycles relative to golden87, with the instruction ratio in
parentheses:

| | geomean |
|---|---|
| rir90 | 1.563 (1.407) |
| rir90, `ceval.o` with `--param=inline-unit-growth=1000` | 1.396 (1.349) |
| rir90, aggressive inlining | 1.467 (1.351) |

The per-kernel figures are in `geo29.md`.

**Reproduction:** I removed the scratch builds (`rir90ip-inl` and the two
Py_STATS builds). To recreate the fix-1 measurement:
1. Configure a release build of `rir90-ip`.
2. Rebuild only `Python/ceval.o`, adding `--param=inline-unit-growth=1000`
   to its compile line, and relink.
3. Run `stat29.sh` and `geo29.py`.

## Side findings

- **Py_STATS build crash:** a Py_STATS build of rir90 segfaults on
  generators. `INSTRUCTION_STATS` indexes `pair_count` with
  `IR_ACTIVATION()->last_handler`, which a nested activation
  (`gen_send_value_ex` → `_PyEval_EvalFrameValue`) leaves uninitialized
  (`Python/ceval_macros.h:18`). I counted with a temporary local bounds guard
  in my stats build and reverted it; nothing is committed.
- **Missing arms that affect timing:** CALL_PY_GENERAL (cause 4),
  FOR_ITER_GEN (cause 5), and nbody's `**` takes `Binary.Generic` (200k per
  run) where golden87 has BINARY_OP_POWER.
- **A slip in my build directory:** at 03:29 I ran `make -n -B` in
  `rir90ip-release`. Make regenerated its Makefile and re-ran configure,
  which I killed. I repaired the directory as `mkbuild.sh` would have:
  `config.status --recheck`, `config.status`, the `PYTHON_FOR_REGEN` sed,
  and `pybuilddir.txt`. The `python` binary (03:04) was not rebuilt, and
  its counts match the earlier runs. The Makefile is now newer than the
  objects, so the next `make` will relink.
