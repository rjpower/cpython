# Hot85: the remaining slow cases, and golden84's hot code size

Approved 2026-11-21. After promoting [parity84](parity84.md)'s IP2 as
golden84, the user asked for two things:
- a breakdown, by opcode and code structure, of what still makes deepcopy and
  the other slow cases slower than upstream;
- a cleanup of the hot code size, with rare branches in hot handlers moved
  out to functions.

Base: golden84, `4be2eb5d31f`. Evidence `E59/hot85/` (`coordinator.md` is
the log; `common.md` and `brief-*.md` are the briefs). Gates are correctness
and the allocator limit (50M; golden84 is at 44.84M). Moving a rare path out
of line or declaring a rare helper cold is a structural change; padding,
alignment and layout searches are not.

## Phase A: analysis

| Unit | Content |
| --- | --- |
| LB | deepcopy, deepcopy_reduce, deepcopy_memo, logging_format, logging_simple, json_dumps, json_loads, pickle, unpickle: excess per loop by opcode family, by native symbol group and by Python function and line; exact ledgers of the five largest operations |
| XK | xml_etree_{iterparse,parse,process,generate}, nqueens, comprehensions, coroutines, regex_compile, deltablue: the same method, and the excess by source construct |
| HC | cycles, instructions and front-end counters on CPU 15 for all 29 (golden84, golden82, upstream); spectral_norm's op-cache mechanism; the hot footprint by function and handler; an inventory of rare paths in the 40 hottest handlers and 20 hottest helpers, with a proposed form per family; one prototype family in `worktrees/hot85-hc` |

## Phase A results

**LB** (`E59/hot85/lb/report.md`). Exact instruction traces of both builds,
attributed to handler, native function and Python line.
- deepcopy: +414K instructions per loop (1.159). Golden84 dispatches 19,230
  handlers against upstream's 34,533 and saves 226K in operand moves, but
  pays more per call, return and value check:
  - the call-operand release and publish pass, +205K (no upstream
    counterpart): `cls = type(x)` costs 102 against 30;
  - value primitives +133K, return teardown +93K, call push +67K;
  - `cls in _atomic_types` 242 against about 157 (a five-layer pointer
    hash);
  - 90 KeyError raises and catches per loop at +296 each (parity84 halved
    the cost).
- json_dumps +6.9M: one nine-operand call (`c_make_encoder`) at 3,761
  against 2,370; calls with more than four operands run per-operand loops.
- json_loads and unpickle: `dictresize` copies entries one at a time, 307
  instructions per call against 86.
- logging: `code.co_filename`/`co_name` are `_Py_T_OBJECT` members, which
  golden84 never specializes (364 against 38).

**HC** (`E59/hot85/hc/report.md`). Cycles and front-end counters on CPU 15.
- Over the 17 cases slower than upstream in cycles, instructions explain
  all of the gap (log-excess +1.221) and cycles per instruction are better
  (-0.265). Front-end losses: nqueens, xml_etree_parse, coroutines,
  fannkuch.
- spectral_norm's 1.33 times golden82's time: cold bytes inside the 12
  hottest handlers split op-cache entries, and the handlers stack up to 15
  ways deep in sets 38-43. Not footprint (same hot lines as golden82).
- In the top 40 handlers' hot lines, guard-miss, error, PEP 523, monitoring
  and allocation paths are 3% of the cold bytes; the refcount-reaches-zero
  block is 29%; uncommon variants 63%. 73% of hot lines are helpers.
- Prototype A (`__builtin_expect` on the refcount-zero branch) cut
  spectral_norm to 0.780 of golden84's cycles, deltablue 0.954, deepcopy
  0.963, robust to evaluator shifts. Prototype B (calling the dealloc out
  of line) lost the gain.
- Answer to the user's question: in handlers, hint rare branches and keep
  the common path contiguous rather than calling out; in helpers, split
  cold halves out of line.

**XK** (`E59/hot85/xk/report.md`). The same method on the xml cases,
nqueens, comprehensions, coroutines and regex_compile.
- xml_etree_iterparse: +706 instructions per event (+144M per loop).
  - The Python consumer is +437:
    - `FOR_ITER` on a Python class whose `__next__` is a generator's
      (C re-enters the evaluator): +183;
    - a negative `isinstance(event, Exception)`, which falls back to
      reading `__class__`: +163;
    - the deque truth test, the `yield from` relay, the pair unpack,
      `popleft` and the tuple build: +60 to +80 each;
    - golden84's specialized `elem.tag`: -177.
  - Parsing and event delivery in `_elementtree` are +269.
- nqueens: +753 per permutation (+30.4M per loop).
  - 95% is `tuple(pool[i] for i in indices[:r])`:
    - generator-expression creation and finish: +705;
    - the list-then-tuple build and builtin calls: +769;
    - per-item resume and yield: +507;
    - element loads and slices: +580.
  - These are against a register-IR move credit of -1,525.
- coroutines: 0.987 of upstream's instructions; the time loss is not
  footprint.
- Further items: the C-to-Python entry behind `TextIOWrapper.write`
  (+93M); the two-element unpack after parity84's single-loop form
  (+38M); `KeyError` in regex_compile (+32M); int-subclass operands
  (+23M); `Element` iteration through a sequence iterator and an
  `IndexError` per loop.

## Phase B1

Eight units from golden84 (briefs `E59/hot85/common-b.md`, `brief-*.md`):

| Unit | Content | Source |
| --- | --- | --- |
| HN | One branch-hint macro; the refcount-zero branch rare in every decref form; periodic check slow part out of line; cold attribute on truly cold functions | HC U1, U5 |
| CP | Skip the release pass when a call owes nothing (decided at specialization); wide calls by operand word; `CALL_PY_GENERAL` plan reuse; call helper shape and splits | LB1, LB3, LB9; HC U2, U4 |
| RT | Return clears only owner-capable slots; push from the same facts; retire path shape | LB4, LB5 |
| DT | `dictresize` with `memcpy`; pointer hash for default-hash objects; inserts; `_PyDict_NextValue`; `insertdict`/`_Py_dict_lookup` splits | LB6, LB7, LB2; HC U4 |
| AT | `_Py_T_OBJECT` member loads in one specialized form; `__class__` lookup; attribute helper splits | LB10; HC U4 |
| FL | Rare float and int operand cases out of line, counted first | HC U3 |
| XP | `Element` iterates natively; the `_elementtree` parse and event append do upstream's work | XK 2, 13 |
| EV | Small unpacks inline in the one handler; `LIST_APPEND` inline; intrinsic, slice and truth-test layers removed; list and range steps; dict-items step | XK 6, 9, 14, 15; LB8 |

Integration on `pyjit/hot85`: full suites, instructions for all 29, the
timed all-29 against golden84 and upstream, and cycles and decoder µops on
CPU 15 over golden84 and shifted diagnostic builds (HC U6), recorded, not
gated. Deferred to B2, after B1's integration because they overlap B1's
files: the C-to-Python entry, generator creation and resume, int-subclass
operands, `CALL_KW_PY`, and the `_json`/`_pickle` ports.
