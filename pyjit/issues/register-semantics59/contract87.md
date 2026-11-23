# Contract87: the operation contract and the interpreter's last wave

**Selected:** the user promoted the integration (`684a98eb30e`) as golden87
on 2026-11-23 (tag `pyjit-golden-20261123-contract87`, branch
`pyjit/round87-golden`) and approved Phase 2, the baseline method JIT
([jit88](jit88.md)).

Approved 2026-11-22 as Phase 1 of [tiers87](../../design/proposals/tiers87.md):
the cases generator exports one operation contract, and the remaining
interpreter work goes to the shared runtime that compiled code also calls.
Stack bytecode recovery lands alongside.

Base: golden86, `3cac420ddbf`. Integration branch `pyjit/contract87`, head
`684a98eb30e` (magic 64446): ten units and two root fixes, 104 commits,
180 files, +34,594 −16,046 lines (generated files included). Evidence
`E59/contract87/`: `coordinator.md` is the log, `common.md` and
`brief-*.md` are the briefs, `<unit>/report.md` the reports, `int/` the
integration builds, smoke runs and full suites, and `bench/` the timing.
The gates were correctness and root's review of each diff. Selection is the
user's.

## Units

| Unit | Head | What changed | Measured |
| --- | --- | --- | --- |
| XP | `7909372888c` | The C `Element` iterates its children through its own `tp_iter` with no subclass check; the Python `Element` gains `__iter__`, so both iterate one way | — |
| EH | `400564ee390` | Raise and catch: exceptions built through the generic value call; StopIteration and AttributeError get value vectorcalls of their own; `_PyExc_BuildFromArray` deleted | instructions per episode, golden86 → EH (upstream): KeyError miss 2,088 → 1,876 (1,807); in the first revision, StopIteration 2,609 → 2,265 (2,180) and AttributeError 5,313 → 4,713 (4,860) |
| NV | `6dd6a36a1cf` | One native call adapter: the object vector on the C stack; VARARGS method descriptors as value forms; a native call scope makes borrow-release requests no-ops while it is innermost | golden86 → NV (upstream): `d.update(u)` 1,334 → 1,107 (915); weakref call 314 → 256 (202) |
| FR | `7ffb6823b0b` | One exec pointer per frame: the feedback block sits before the exec header, the frame keeps `ir_words` only (header 104 → 96 bytes) | `f(i)` push 387 → 385 |
| CR | `a864fc10dd3` | The operation record in the generator: uops with operands, ownership, guard facts, effect classes and a feedback schema; handler metadata bits and extents are projections of the record; `_PyIR_ALLOCATES` deleted | generated C byte-identical to golden86's |
| AT | `bb5ba043cec` | `LOAD_ATTR_METHOD_POLY` over the site's polymorphic rows; no MRO walk for `object`'s descriptors; `dict.update` as `METH_VALUE \| METH_KEYWORDS` | richards 0.832 of golden86's instructions; `o.__dict__.update(st)` 1,941 → 1,162 (1,424) |
| TR | `745b4a0d81e` | Labels become VM routines in `ceval_routines.h`, one source with two bindings; bodies use typed transfers only (the `EVALUATOR` kind deleted); a function form `ir_run_X` for each of the 309 handlers, run by `-X ir_forms` | instr29 1.0006 of its base |
| SB | `40438cb903b` | [Stack bytecode recovery](../../design/proposals/stack-bytecode-recovery.md): `co_code`, `.pyc` and `marshal` hold upstream's stack bytecode; the register IR is lowered at first call and kept hidden with a pcmap; fifteen image validators deleted | lowered IR matches golden86's on 9,708 Lib code objects, apart from listed and explained differences |
| LS | `ee319d70ea7` | The leaf subset ([tiers87 C8](../../design/proposals/tiers87.md#the-contract)): 60 value primitives in `ir_leaf.py`, each checked against its C; the `leaf` annotation enforced by `check_leaf()`; leaf IR on `UopRecord.leaf`; generated probes against a reference evaluator; `_PyObject_ValueSlot` and `_PyValue_NamesObject` | 113 of 364 uops inside the subset; raw-bits findings 36 → 14 tree-wide |
| ST | `f916dfdf941` | First-call lowering cost: regcfg tables sized by the code instead of the slot limit; `RETURN_VALUE_EMPTY` chosen in the assembler at stack depth 1, replacing a whole-function proof at every lowering | startup below |

Root's fixes on the integration branch:
- `49274a5b159`: SB's merge left `_Py_InitCleanup` sized for two IR words, so
  its stack bytecode lost `RETURN_VALUE`. Fixed to four bytes (`_PyCode4`).
- `684a98eb30e`: LS's `_LEAVES` cache, process-wide and keyed by `id(uop)`,
  kept every analysis's leaf IR, so test_ir_empty_return's generator test
  leaked about 356,000 references per `-R` repetition. Translating every
  uop takes 0.03 s, so the cache is gone.

## Results

Measured at m2 (`49274a5b159`, all units but ST). ST changes only first-call
lowering; its identity sweep found no difference in the in-tree kernels'
lowered code.

| Measure | contract87 | golden86 |
| --- | ---: | ---: |
| Timed all-29 geomean, vs upstream's time | **0.876** | 0.877 |
| Timed all-29 geomean, vs golden86's time | **1.001** | 1 |
| Instructions, all 29, vs upstream | 0.9213 | 0.928 |
| Instructions, all 29, vs golden86 | 0.9935 | 1 |
| Register-allocator product | 42.57M | 43.77M |
| `ir_evaluate` hot + cold | 0x15c58 + 0x9e5 | 0x158e3 + 0xb1a |

Faster than golden86: richards 0.789 (AT), deepcopy_reduce 0.910, deepcopy
0.928, nqueens 0.932, json_dumps 0.962. Slower at about equal instructions:
logging_silent 1.332, coroutines 1.104, float 1.064, unpack_sequence 1.044,
go 1.043, spectral_norm 1.027, generators 1.023, nbody 1.021. A counter run
on CPU 15 (`int/cycles/diag1-{A,B}.txt`, `diag2-F.txt`) places these:
- coroutines, logging_silent, unpack_sequence, go and generators move with
  op-cache placement: the same handler code runs from the decoder in one
  build and the op cache in another. Not tuned.
- float's indirect mispredictions rise from about 900 to 201,700 per loop
  from TR on: `sin`, `cos` and `sqrt` alternate through one shared
  native-call site. The baseline JIT's per-site calls remove this.

The user judged the flat geomean not worth further attribution.

Still slower than upstream's time: comprehensions 1.040, nqueens 1.039,
pickle 1.018, deepcopy 1.015, unpack_sequence 1.007 and logging_simple
1.005. Comprehensions moved from 1.107 to 1.040.

**Startup** (`instructions:u`, minimum of three):

| Command | golden86 | m2 (SB) | contract87 (ST) | upstream |
| --- | ---: | ---: | ---: | ---: |
| `python -I -S -c pass` | 37.5M | 122.0M | 79.0M | 30.6M |
| `python -I -c pass` | 56.4M | 186.9M | 121.5M | 45.5M |

SB lowers each code object at its first call, about 0.5M instructions each
before ST and 264K after. ST's diagnosis (`st/report.md`): 45% went to
`_PyIR_SelectEmptyReturns`, a whole-function ownership proof, and 14% to
tables sized by `IR_MAX_SLOTS`. What remains is linear in code size,
about 23K instructions per `x = g(a, i) + b.c` statement; jump fusion's
liveness and Compact's second run are the largest pieces and need a pass
redesign. The user accepts startup cost within reason; it is recorded, not
gated.

**Suites.** m2 release and debug: known failures only (test_sys_settrace 8,
test_ir_error_effects 5, test_generated_cases 1E 2F, test_tools
`test_raw_bits`), plus two debug load flakes that passed on rerun
(test_multiprocessing_forkserver, test_profiling). test_external_inspection
passes from SB on. Smoke6 at the ST merge: the same known set. The m3 full
suites at `684a98eb30e`, release and debug: the known set only, with no
flakes. The debug suite took 46 minutes, against m2's 1 h 25 min, because
ST cut the cost of starting a subprocess.

Every regrtest run now passes `--timeout` (release 1200 s, debug 3600 s):
m2's debug suite ran without a hang guard.

## Open findings

- First-call lowering costs about 264K instructions per code object. An IR
  cache, or lowering only warm code, would remove most of it; neither is
  planned.
- float's shared native-call site, above.
- `d.update(**kw)` unpacks the dict into keyword names and values: 2 keys
  2,811 instructions against golden86's 2,596 and upstream's 2,154 (AT).
- EH's exceptions pay 44 instructions per KeyError episode for the generic
  value call dispatch.
- `check_value_api.py` needs clang-query, which is not installed; NV's
  change was not checked by it.
- Commit `b2874c91e56` on SB's branch is titled as a settrace fix but
  removes about 25 crafted-image tests.
