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

## Phase B1 results

**IP3** (`2a9ea8c4f69`, `pyjit/hot85`): golden84 plus AT, FL, CP (with its
follow-up), DT, XP, HN and EV, and two root fixes. Magic 64441. RT is not
merged. Evidence `E59/hot85/int/` (builds, suites, instruction counts,
cycles) and `E59/hot85/bench/` (campaigns).

- **Timed all-29: 0.949 of golden84's time and 0.900 of upstream's**
  (golden84 runs 0.953 of upstream's). Instructions: 0.961 of golden84's
  and 0.956 of upstream's.
- **Code size:** `ir_evaluate` 88,999 bytes (golden84 92,111). The register
  allocator's product is 44.39M (golden84 44.84M; the limit is 50M).
- **Correctness:** full suites, release and debug, with known failures only
  (see below for the one new failure and its fix).

What each unit did:

| Unit | Change | Instructions vs golden84, unit alone |
| --- | --- | --- |
| AT | `_Py_T_OBJECT` member loads specialized (`LOAD_ATTR_SLOT_POINTER`; `co_filename` 331 to 36 instructions); a negative `isinstance` skips the `__class__` read when the type uses object's getter; cold attribute-miss paths | 0.991; logging_simple 0.946, iterparse 0.954 |
| FL | Inline-number operand closes are `DEAD()`, so float handlers shrink from 623 to 383 bytes; known non-number results skip the number test. Mixed int/float operands are common (spectral_norm's `TRUEDIV` 100%), so they stay inline | 0.9955; nbody 0.962 |
| CP | `IR_F_CALL_LOCALS`: when every operand is a local, the release pass handles only the callable; wide calls by word; a tuple's items borrowed as one vector; per-code bind plans; cold halves of the call helpers | 0.998; json_dumps 0.989, deepcopy 0.990 |
| DT | `dictresize` copies in one block; pointer hash direct for default-hash objects; `insertdict` and `_Py_dict_lookup` keep one exact-str probe and move the rest out of line | 0.992; json_loads 0.959, unpickle 0.954 |
| XP | `Element` iterates its child array natively; `makeuniversal` keyed by the C string; event append in line | xml parse 0.867, iterparse 0.942 |
| HN | `Py_UNLIKELY` moves to `pyport.h` as the one hint; the refcount-zero branch is rare in every decref form; the periodic slow part is out of line; zero-call monitoring, redirect and stack-chunk callees are cold (2.6 KB out of `ir_evaluate`) | neutral |
| EV | 2- and 3-element unpacks straight-line; `LIST_APPEND` in line; slice and intrinsic layers removed; `POP_JUMP_IF_{TRUE,FALSE}_SLOT` for heap types with a truth slot | 0.995; unpack_sequence 1.017 |

RT is not merged. Constant image slots never own, but popped frames must
still be NULL-filled, so its constant-block return removes only a test and a
branch: 0.9998 of golden84's instructions. It added three hot lines to the
hottest return handler, and its cycle changes moved in both directions with
layout. The structural step it found goes to the design list: read constants
from the exec image as an operand class, or drop the NULL invariant of
popped frames.

Per case (instructions from `int/ip2/instr`, time from
`bench/ip3-summary.txt`, two paired rounds per case):

| Case | Instr vs golden84 | Instr vs upstream | Time vs golden84 | Time vs upstream | Golden84 time vs upstream |
| --- | ---: | ---: | ---: | ---: | ---: |
| richards | 0.990 | 0.880 | 0.965 | 0.750 | 0.754 |
| go | 0.988 | 0.998 | 0.976 | 0.891 | 0.939 |
| nbody | 0.940 | 0.758 | 0.870 | 0.609 | 0.699 |
| float | 0.976 | 0.800 | 0.982 | 0.674 | 0.688 |
| spectral_norm | 0.949 | 0.657 | 0.857 | 0.566 | 0.658 |
| raytrace | 0.975 | 0.948 | 0.966 | 0.831 | 0.837 |
| deltablue | 0.994 | 1.053 | 0.956 | 0.965 | 0.997 |
| chaos | 0.979 | 0.885 | 0.972 | 0.791 | 0.826 |
| nqueens | 0.991 | 1.028 | 1.011 | 1.073 | 1.075 |
| fannkuch | 0.993 | 0.889 | 1.023 | 0.979 | 0.965 |
| generators | 0.999 | 0.819 | 1.012 | 0.820 | 0.826 |
| comprehensions | 0.970 | 1.072 | 1.019 | 1.046 | 1.072 |
| coroutines | 0.997 | 0.991 | 0.948 | 0.986 | 1.047 |
| unpack_sequence | 1.017 | 0.996 | 0.999 | 0.976 | 0.954 |
| deepcopy | 0.949 | 1.097 | 0.940 | 1.087 | 1.181 |
| deepcopy_reduce | 0.954 | 1.082 | 0.969 | 1.095 | 1.126 |
| deepcopy_memo | 0.946 | 0.998 | 0.936 | 0.961 | 1.032 |
| json_dumps | 0.964 | 1.027 | 0.964 | 1.016 | 1.078 |
| json_loads | 0.961 | 0.999 | 1.010 | 1.013 | 1.020 |
| pickle | 0.993 | 1.016 | 1.000 | 1.067 | 1.047 |
| unpickle | 0.966 | 0.974 | 0.966 | 0.970 | 1.009 |
| regex_compile | 0.978 | 1.058 | 0.939 | 0.980 | 1.012 |
| logging_format | 0.935 | 1.043 | 0.936 | 0.955 | 1.080 |
| logging_silent | 1.000 | 0.951 | 0.985 | 0.767 | 0.777 |
| logging_simple | 0.931 | 1.029 | 0.967 | 0.955 | 1.039 |
| xml_etree_generate | 0.900 | 0.954 | 0.844 | 0.839 | 1.017 |
| xml_etree_iterparse | 0.874 | 0.998 | 0.872 | 0.937 | 1.079 |
| xml_etree_parse | 0.864 | 0.913 | 0.827 | 0.919 | 1.077 |
| xml_etree_process | 0.900 | 0.984 | 0.842 | 0.918 | 1.058 |

Seven cases remain slower than upstream: deepcopy_reduce, deepcopy,
nqueens, pickle, comprehensions, json_dumps and json_loads. All but pickle
and json_loads also execute more instructions than upstream. fannkuch,
comprehensions, generators, nqueens and json_loads run 1–2% above golden84
at equal or fewer instructions; their two rounds differ by 2–4%.

**Cycles on CPU 15** (`int/cycles/ip2-A-table.md`; pass A, three
repetitions). The ensemble is the integration build plus three diagnostic
builds that shift `ir_evaluate`'s body by 32, 64 and 96 bytes. Ranges are
cycles relative to golden84:

| Case | Range | Case | Range |
| --- | --- | --- | --- |
| spectral_norm | 0.853–0.892 | deltablue | 0.953–0.984 |
| nbody | 0.857–0.886 | float | 0.948–0.984 |
| deepcopy | 0.939–0.951 | fannkuch | 0.959–0.982 |
| coroutines | 0.913–0.959 | generators | 0.999–1.059 |
| richards | 0.963–0.974 | | |

- spectral_norm still misses the op cache. Its loop runs 456–514M
  legacy-decoder µops in every layout, against golden84's 608M; the AT+FL
  build alone had brought it down to about 18M.
- The diagnostic shift moves every handler together. It therefore tests
  conflicts between handlers and helpers, but not conflicts between
  handlers, which are what HC found in spectral_norm.
- Each unit's own build (golden84 plus that unit;
  `int/cycles/sn-units-A-table.md`) gives these decoder µops relative to
  golden84:

  | Case | Lowest | Highest | IP3 |
  | --- | --- | --- | ---: |
  | spectral_norm | EV 0.40 | HN 1.03 | 0.75 |
  | nbody | DT 0.03 | CP 1.75 | 0.29 |
  | coroutines | DT 0.24 | CP 1.01 | 0.87 |

  HN's build has the same `ir_evaluate` text as HC's prototype A, yet it
  stays at golden84's level. These small hot sets still depend on where the
  hot handlers land relative to one another. B1's structural changes did not
  make that robust: smaller float handlers, the refcount-zero hint and cold
  callees. AT+FL's 0.690 on spectral_norm was one layout's outcome.
  Instructions remain the reliable signal of a change; cycles on these cases
  move by 10–25% with unrelated edits.

**Correctness.** Release and debug full suites (`int/ip2/full/`) show only
known failures by subtest: test_sys_settrace 8 errors, test_ir_error_effects
5, test_generated_cases the three known, test_external_inspection 1, and
test_raw_bits with golden84's findings. There was one new failure:
test_embed's `test_borrow_scope_outside_eval`.
- Cause: CP borrows a parsed tuple's items from the tuple, as upstream
  does, so the base borrow region no longer takes a count for an "O" item.
- Fix: the test keeps the tuple itself in the base region (`2a9ea8c4f69`).
- The other root fix, `feb8e91f613`, compares mro entries through the value
  API rather than raw bits; AT had masked the tag bits directly.

**Allocator layout.** CP's follow-up traced its json_loads +1.8% to mimalloc
retiring and refilling one 224-byte-class page about 40 times per loop.
Under `PYTHONMALLOC=malloc` the instruction counts are equal. Which
long-lived blocks share that page decides whether it empties, so any change
to a long-lived allocation size can move the effect. In IP3, json_loads
runs 0.961 of golden84's instructions but 1.010 of its time. The real fix
belongs in the allocator: retire hysteresis for one-page size classes.
