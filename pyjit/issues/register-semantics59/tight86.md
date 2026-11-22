# Tight86: hot85's deferred items, as tight code

**Selected:** the user promoted IP1 (`3cac420ddbf`) as golden86 on
2026-11-22 (tag `pyjit-golden-20261122-tight86`, branch
`pyjit/round86-golden`), with no layout rule, and approved dropping XP's
subclass check (`pyjit/contract87-xp`, `7909372888c`).

Approved 2026-11-22. The user promoted [hot85](hot85.md)'s IP3 as golden85
(`2a9ea8c4f69`) and asked for three things:
- land the remaining fixes;
- keep the code clean and tightly defined, with no spaghetti;
- prefer tighter hot code to layout rules. Layout comes last, as one
  simple rule, and only if it is still necessary.

Base: golden85, `2a9ea8c4f69`. Evidence `E59/tight86/` (`coordinator.md` is
the log; `common.md` and `brief-*.md` are the briefs). The gates are
correctness, the allocator limit (50M; golden85 is at 44.39M) and root's
review of each diff. Review sends back:
- a second path for an existing mechanism;
- a flag that chooses between an old form and a new one;
- a special case keyed to a benchmark;
- a layer with a single user;
- a cold copy of a hot branch.

## Units

| Unit | Mechanism | Cases | Source |
| --- | --- | --- | --- |
| HT | The handler skeleton (decode, view, destination store, old-value close, detach and release, branch recording) emitted by the generator, so each handler pays only what its facts require | every case; json_dumps jumps and attribute loads, nqueens subscripts | LB "per-handler body differences"; XK §2 |
| GE | Generator creation in one pass; a plain finish without the finalizer call; resume and yield that pay only for their consumer | nqueens, comprehensions, coroutines, generators, iterparse | XK 4, 7 |
| PC | One frame slot contract with a cheap return; call and return bodies; `CALL_KW_PY` through a bind plan; `CALL_FUNCTION_EX` to Python | deepcopy, deepcopy_reduce, regex_compile, json_dumps, the serializer | LB 4, 5; XK 5, 12; RT |
| NC | A per-site owed-operand fact that replaces `IR_F_CALL_LOCALS`; regcfg folding only across instructions that cannot run Python; native call bodies | deepcopy, comprehensions, nqueens, json_dumps | LB 3; XK 9, 10; CP |
| NE | One entry from C into Python; slot calls made unbound; `PyObject_VectorcallMethod` | xml cases, iterparse, regex_compile, deepcopy_reduce | XK 1 |
| CM | Equality fast exits in one routine; membership on the value key; int-subclass hashing; default-equality `COMPARE_OP`; `str in str` | deepcopy, regex_compile, comprehensions, generate | XK 11, 12; LB 7; DT |
| EX | `_json` encoder; `_pickle` opcode boundary and `Pdata`; mimalloc retire hysteresis | json_dumps, json_loads, pickle, unpickle | LB 2; CP follow-up |
| IA | First attribute store into a fresh instance; `__dict__` materialization; `dict.update` into an instance dict | deepcopy_reduce | LB deepcopy_reduce |

Integration on `pyjit/tight86`, in the same way as hot85:
- full suites, release and debug;
- instructions for all 29;
- the timed all-29 against golden85 and upstream;
- cycles and decoder µops on CPU 15.

The small hot sets (spectral_norm, nbody, coroutines) are measured again
after the integration. A layout rule is considered only if the tighter
handlers have not made them robust.

Not in tight86, kept on the design list:
- the raise-and-catch path (`_PyEval_ProcessError`, `error_clear_slots`;
  regex_compile's `KeyError`);
- reading `ir_words` and `ir_feedback` through the exec rather than per-frame
  copies.

The dict-items step was on this list; IA took it as a fourth goal
(`FOR_ITER_DICT_ITEMS`).

## IP1 results

IP1 is `3cac420ddbf` on `pyjit/tight86` (magic 64442): all eight units
merged onto golden85, 61 files, +3,875 −1,634. Each unit's report is in
`E59/tight86/<unit>/report.md`; root's log is `coordinator.md`.

| Measure | IP1 | golden85 |
| --- | ---: | ---: |
| Timed all-29 geomean, vs upstream's time | **0.877** | 0.900 |
| Timed all-29 geomean, vs golden85's time | **0.970** | 1 |
| Instructions, all 29, vs upstream | 0.9277 | 0.9572 |
| `ir_evaluate` size | 88,291 B | 88,999 B |
| Register-allocator product | 43.77M | 44.39M |

What the units removed, in instructions per operation (golden85 → IP1,
upstream in brackets):
- HT: the truth-slot jumps 44 → 32 (24); `LOAD_ATTR_MODULE` 76 → 63 (46);
  branch-taken history, written by every conditional jump and read by
  nothing, is deleted. Allocator −1.09M.
- PC: a Python return 215 → 160; constant slots own nothing.
- GE: a generator expression's first step 1,020 → 892 (991); nqueens at
  upstream's instruction count.
- NC: the call-site release visits only the operands the site owes.
- NE: one C-to-Python entry; the xml cases execute 2% fewer instructions.
- CM: enum `==` 282 → 161 (209); a plain object `!=` 383 → 159 (252).
- EX: json_loads 0.925 of upstream's instructions; mimalloc keeps an
  emptied head page through the retire delay.
- IA: `for k, v in d.items()` over two items 1,949 → 1,411 (1,544);
  deepcopy and deepcopy_reduce 0.977 of golden85's instructions each.

Timed, still slower than upstream: comprehensions 1.107 (its two rounds
1.033 and 1.187), deepcopy_reduce 1.051, deepcopy 1.049, nqueens 1.041,
json_dumps 1.017, pickle 1.017 and logging_simple 1.015. json_loads moved
below upstream (0.902). Against golden85 the largest gains are float 0.846,
json_loads 0.877, coroutines 0.923, spectral_norm 0.926 and chaos 0.946.
Slower than golden85: xml_etree_process 1.024, xml_etree_parse 1.017,
nbody 1.016, xml_etree_iterparse 1.013, json_dumps 1.007 and
logging_format 1.005.

**Layout.** A counter run on CPU 15 (`int/cycles/diag1-table.txt`) finds
one consistent loss. nbody executes the same instructions and taken
branches as golden85, but its cycles are 1.5–6% higher in all five passes:
branches that hit only in the second-level BTB rise 20%, mispredictions
23% and fetch stalls 6%. No unit changed nbody's code; its unchanged
handlers predict worse at their new addresses. The xml cases execute
fewer instructions and show no consistent cycle loss; their timed
difference is within their spread. Tighter handlers made spectral_norm
(0.919 of golden85's cycles), float (0.865) and coroutines (0.917) faster
in every shifted layout. No layout rule is proposed.

**Suites.** Release: the known failures only, by subtest (test_sys_settrace
8, test_ir_error_effects 5, test_generated_cases' three,
test_external_inspection 1, and test_tools `test_raw_bits` with the same 22
findings as golden85's source). test_profiling failed once under load and
passed four reruns. Debug: the same known set by subtest.

Open findings, for the next wave:
- `CALL_NON_PY_GENERAL` reaches a C function through three layers, about
  100 instructions of prologue per call (NC).
- `dict.update` into an instance dict still pays the general call path
  (IA).
- Loading `x.__dict__` walks `PyType_IsSubtype(C, object)` in the
  descriptor check, 26 of its 48 excess instructions (IA).
- HT's remaining per-handler costs: a fused compare and jump builds a bool
  and tests it (about 10 of 66), the old-value close on temporaries, the
  detach and taken tests of one owner bit, and field decode.
- pickle matches upstream's instruction count, but its cache and branch
  misses are 45–60% above upstream's, spread across functions (EX).
- `_PyBorrow_TupleItems` always sets the borrow-release bit (NC).
- The free-threading GC still visits constant slots; this project builds
  only the GIL runtime (PC).

## XP's subclass check

XP gave `Element` a native `tp_iter`. Its `element_iter` returns a sequence
iterator instead when a subclass overrides `__getitem__` without `__iter__`,
so that such a subclass still iterates through its `__getitem__`. This is
valid Python, and it costs one pointer compare per `iter()` call, not per
step. The pure-Python `Element` defines `__getitem__` and no `__iter__`, so
today both implementations iterate an overriding subclass through
`__getitem__`. Dropping the check would leave one iteration path, and make
the C `Element` behave like `list`, whose subclasses do not change
iteration by overriding `__getitem__`. To keep the two implementations in
agreement, the pure-Python `Element` would gain an `__iter__` at the same
time. The user has not decided.
