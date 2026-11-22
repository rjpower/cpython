# Tight86: hot85's deferred items, as tight code

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
  copies;
- the dict-items fused iteration step.

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
