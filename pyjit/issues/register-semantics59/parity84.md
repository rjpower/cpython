# Parity84: the clean changes profile83 ranked

Approved 2026-11-21. After [profile83](profile83.md) ranked the clean changes
that remain against upstream, the user approved its proposed wave. Gates are
correctness and the allocator limit (50M); measurements are recorded and
attributed, not gated.

Base: golden82, `5194e957c5d`. Integration branch `pyjit/parity84`; evidence
`E59/parity84/` (`coordinator.md` is the log; `common.md` and `brief-*.md`
are the unit briefs).

## Units

| Unit | Content | Profile83 items | Phase | State |
| --- | --- | --- | --- | --- |
| NB | Native-call publication as one counted loop; one native→Python entry routine replacing the eight-layer chain; value callees called without object-API thunks; exceptions built from the argument window; `isinstance` with an exact-type exit and one value entry | A1–A5 | 1 | merged (`3aaed495690`) |
| GL | Test the generators back-end stall (`IR_SKIP_RESUME`'s form) on CPU 15; specialize `GET_ITER` over a Python `__iter__`; create generators at the call; one "plain generator" bit for finishing; store the resume point at suspension | B1–B4 | 1 | merged (`a8b1c74c57e`); item 4 not done (below) |
| IT | Specialized membership tests and compare exits; inline `FOR_ITER` for `tp_iternext` iterators; one truth test for None or plain objects; the unpack element loop | D1–D3, E6, E9 | 1 | merged (`5a51846ce16`) |
| RM | Allocation entry, GC visits and unicode-key traversal, dict first insert and unicode lookup, `makeuniversal`, `json.dumps` integers, the borrow stack's base chunk, big-int bitwise ops, `MAKE_FUNCTION` | E1–E5, E7, E8, E10 | 1 | merged (`5fd86ffa6eb`) |
| BD | Per-site bind plans for defaults and keywords; `CALL_FUNCTION_EX` with a `**kwargs` dict binds in place; generator callees bind into the new generator | C1, C2 | 2, from IP1 | merged (`7de42f8e41f`); item 4 not done (below) |
| GN | One generator creation path (the creation routine performs the prologue; `_PyFrame_CopyCreatedGenerator` and `RETURN_GENERATOR`'s copy deleted); one-pass slot clearing at finish | B2, B3 (GL's remainder) | 2, from IP1, beside BD | merged (`96d30798906`), with a root fix (`4be2eb5d31f`) |

Hot-code size (profile83 group F) is not in this wave.

- **IP1:** merge NB, GL, IT and RM; full suites; instructions for all 29;
  timed all-29 against golden82 and upstream.
- **IP2:** merge BD and GN; full suites; timed all-29 against golden82 and
  upstream; results here.

## Phase 1 and IP1

IP1 is `5a51846ce16` (`pyjit/parity84`): NB, RM, GL and IT merged, with
magic 64436. The full suites in both roles fail only in golden82's known
set, by subtest. The allocator product is 45.02M (golden82: 45.45M).

Instructions per loop over all 29 kernels (`E59/parity84/int/ip1/instr/table.txt`):

| Build | Geomean against golden82 | Geomean against upstream |
| --- | ---: | ---: |
| golden82 | 1 | 1.034 |
| IP1 | 0.966 | 0.999 |

- Largest reductions against golden82:
  - generators 0.726, coroutines 0.912;
  - unpack_sequence 0.941, xml_etree_iterparse 0.944, xml_etree_process
    0.944, comprehensions 0.948, xml_etree_generate 0.952;
  - regex_compile 0.957, json_dumps 0.959, deepcopy_reduce 0.963, deepcopy
    0.971, richards 0.972, float 0.975, go 0.976.
- Only nbody rises (1.016). IT removed `UNPACK_SEQUENCE_TWO_TUPLE` (one
  unpack loop, no arity variants), so a two-element unpack costs 33 more
  instructions. On CPU 15, nbody's cycles fell 1.5%.
- Still furthest above upstream in instructions: deepcopy 1.152,
  xml_etree_iterparse 1.144, deepcopy_reduce 1.139, logging_format 1.122,
  logging_simple 1.113, comprehensions 1.108, xml_etree_process 1.096,
  regex_compile 1.083.

Timed all-29 on CPU 15 (`E59/parity84/bench/ip1-summary.txt`; two
rounds, 58 paired rows per control; time ratios, lower is faster):

| Build | Against golden82 | Against upstream |
| --- | ---: | ---: |
| golden82 (gap82 IP2 campaign) | 1 | 0.992 |
| IP1 | 0.948 | 0.949 |

- Against golden82:
  - generators 0.654, logging_silent 0.846, unpack_sequence 0.848;
  - xml_etree_iterparse 0.936, logging_format 0.938, json_dumps 0.940,
    comprehensions 0.943, deepcopy 0.944.
- Only float (1.014) and chaos (1.009) are above 1, although their
  instructions fell (0.975 and 0.991).
- Still above upstream's time:
  - deepcopy 1.175, deepcopy_reduce 1.166, xml_etree_process 1.100,
    logging_format 1.083;
  - xml_etree_iterparse 1.076, comprehensions 1.075, xml_etree_parse
    1.074, nqueens 1.067, json_dumps 1.064, logging_simple 1.051.
- Generators now run at 0.886 of upstream's time (golden82: 1.337), and
  unpack_sequence at 0.950 (1.133).

Cycle counts from the units on CPU 15:
- GL's branch form of `IR_SKIP_RESUME` takes generators from 171M to 152M
  cycles. With direct creation, generators run at 116M, against upstream's
  130–135M.
- IT's unpack loop takes unpack_sequence from 60.0k to 50.9k cycles
  (upstream: 53.4k), and taken branches from 39.6k to 22.0k.

What each unit found:

- **NB.** The publication loop saved only 3–15 instructions per call. The
  rest of a native call's entry cost is decode, spills and guards, and
  removing it needs per-site facts in the call word.
  - Native→Python calls with exact arguments bind in place.
  - Slot dunders and the exception constructors call the value forms
    directly.
  - `isinstance(o, C)` costs 332 instructions (upstream: 340).
- **RM.** Startup fell from 42.99M to 40.23M instructions (upstream:
  31.10M).
  - Allocation reached parity on xml_etree_parse.
  - The borrow stack's base chunk saves about 60 instructions per scope,
    not the 540 profile83 estimated.
  - `gc.get_referents(d)` no longer lists the str keys of a unicode-keys
    dict, as upstream.
- **IT.** regex_compile's slow int compares are `re._constants._NamedIntConstant`
  instances, an int subclass, scanned by tuple `in`. The same-type compare
  exit covers them. Heap ints that are not subclasses still take the
  generic compare: 415 instructions against upstream's 354.
- **GL.**
  - Generators whose code begins at `RETURN_GENERATOR` are created at the
    call (`CALL_PY_GENERATOR`, `GET_ITER_PY_GENERATOR` and the C entry).
    Code with a prologue (genexprs, cells, free variables) still runs a
    thread frame and copies it. GN finishes this.
  - Item 4 (storing the resume point past `RESUME` at suspension) was not
    done:
    - The word compare in `IR_SKIP_RESUME` is the only detector of an
      `INSTRUMENTED_RESUME` left at the resume point while the generator
      was suspended.
    - A flag set at yield would move the same cost.
    - Every reader of a suspended `instr_ptr` would change.
    - It would save about 3 of the roughly 165 instructions per step. A
      step already costs below upstream (167 against 177).

## Phase 2 and IP2

IP2 is `4be2eb5d31f` (`pyjit/parity84`): IP1 with GN, BD and one root fix,
magic 64439. The full suites in both roles fail only in golden82's known
set, by subtest. The allocator product is 44.84M.

- **GN.** One routine creates every generator, coroutine and async
  generator:
  - `_PyGen_NewCall` makes the object and its frame;
  - the binder fills the arguments;
  - `gen_prologue` makes free variables and cells, and runs a generator
    expression's `iter()` at creation.

  Deleted: `_PyFrame_CopyCreatedGenerator`, `generator_direct`,
  `RETURN_GENERATOR`'s body (the word stays as a marker),
  `_PyEvalFramePushAndInit*` and the created-plan flags.

  Moving a genexpr's `iter()` to the caller was tried and dropped: it broke
  `test_new_gen_from_gi_code` and `test_modify_f_locals`.

  Two observable changes, accepted as upstream-incidental:
  - a PEP 523 hook sees a generator's resumptions, not its creation;
  - generator code has no `COPY_FREE_VARS` or `MAKE_CELL` words.
- **BD.** A 16-byte `_PyBindPlan` in the call feedback (the entry grows from
  32 to 40 bytes) holds the keyword destinations, the default and
  keyword-only masks and the counts.
  - **What the plan depends on.** A function's version is its code's
    version, so the plan depends only on the code and the site.
    `len(__defaults__)` is tested and `__kwdefaults__` read at the call.
  - **One binder.** `call_bind_core` serves `CALL_PY_GENERAL`,
    `CALL_KW_PY`, the unspecialized `CALL` and `CALL_FUNCTION_EX`.
  - **`**kwargs` dicts** bind by name in place: exact dict, exact `str`
    keys, at most 16 items.
  - **Generator callees** bind into GN's creation routine, so there is one
    fill.
  - **Placement.** Inlining the binder made `ir_evaluate` spill (nbody +5%),
    so it is called out of line.
  - **Item 4 not done.** Routing NB's and GL's exact fills through a plan
    would cost a stack plan of about 111 instructions per call.
- **Root fix.** The generic creation path lost the locals mapping of a
  coroutine of unoptimized code (top-level `await`): test_builtin (160
  subtests) and test_repl failed in the full suites. Neither module was in
  a unit ladder. `_PyGen_NewCall` now takes the mapping, and
  `test_ir_generator_create` tests it.

Per call, instructions per iteration with the loop (IP1 → IP2; upstream):

| Call | IP1 | IP2 | Upstream |
| --- | ---: | ---: | ---: |
| `f(x)`, one default | 606 | 532 | 622 |
| `f(x, b=y)` | 717 | 631 | 726 |
| `f(1, **kws)` | 3,235 | 1,977 | 1,869 |
| logging's `warning` forward | 3,558 | 3,120 | 2,800 |
| a generator with a default | 1,235 | 1,185 | 1,107 |
| a generator with a closure | 1,323 | 1,234 | 1,228 |

Instructions per loop over all 29 kernels (`E59/parity84/int/ip2/instr/table.txt`):
0.962 of golden82, 0.996 of IP1, 0.995 of upstream. Against IP1:
coroutines 0.970, logging_silent 0.982, nqueens 0.986, go 0.988; the
largest rise is richards (1.007).

Timed all-29 on CPU 15 (`E59/parity84/bench/ip2-summary.txt`; two rounds,
58 paired rows per control):

| Build | Against golden82 | Against upstream |
| --- | ---: | ---: |
| IP1 | 0.948 | 0.949 |
| IP2 | 0.961 | 0.953 |

The difference is spectral_norm: 1.322 of golden82's time at IP2 (IP1:
0.958), with instructions unchanged. On CPU 15 (`int/ip2/cycles/probe1.txt`,
three repetitions):

| spectral_norm, per loop | Cycles | Instructions | Legacy-decoder µops |
| --- | ---: | ---: | ---: |
| golden82 | 219–237M | 822.0M | 14–19M |
| IP1 + GN | 242–248M | 820.5M | 330–333M |
| IP1 + GN + BD | 259–263M | 820.6M | 331–332M |
| IP2 | 301–304M | 820.6M | 607–630M |

The loop's code misses the op cache. The root fix changed a few lines off
spectral_norm's path and doubled the decoder µops on its own, so this is
code placement, not executed work. This project does not tune placement;
the structural lever is profile83's group F (hot-code size). Without
spectral_norm, IP2 runs 0.950 of golden82's time (IP1: 0.948) and 0.966 of
upstream's (IP1: 0.972).

The probe also shows that three campaign ratios above IP1's were noise:
- deltablue: cycles within 1–2% of golden82's;
- deepcopy: 1.50–1.57M cycles against golden82's 1.55M;
- logging_format: 333–348k against 345–378k.

Against golden82, IP2 is faster on:
- generators 0.613, logging_silent 0.810, unpack_sequence 0.843;
- xml_etree_iterparse 0.930, go 0.934, comprehensions 0.943, json_dumps
  0.944, deepcopy_reduce 0.947, logging_simple 0.952, xml_etree_process
  0.960.

Still above upstream's time:
- deepcopy 1.181, deepcopy_reduce 1.126, logging_format 1.080,
  xml_etree_iterparse 1.079, json_dumps 1.078;
- xml_etree_parse 1.077, nqueens 1.075, comprehensions 1.072,
  xml_etree_process 1.058.

## Open items

- Hot-code size (profile83 group F): spectral_norm shows op-cache misses
  20–40 times golden82's at constant instructions.
- Heap ints that are not subclasses still compare through the generic path
  (415 against upstream's 354).
- `next(g)` from C, `sum`/`any` over a genexpr and logging's `**kwargs`
  forward remain above upstream. Their cost is in the builtin-call
  adapter, the evaluator entry and `call_run_ex`, not in creation or
  binding.
- `CALL_ALLOC_AND_ENTER_INIT` binds `__init__` generically: `C(x)` costs
  1,741 against upstream's 1,395. A plan in its feedback is the follow-up.
- Unit ladders missed test_sys (phase 1) and test_builtin and test_repl
  (phase 2). The integration smoke now runs them.

## Selection

On 2026-11-21 the user promoted IP2 as golden84: `4be2eb5d31f`, branch
`pyjit/round84-golden`, tag `pyjit-golden-20261121-parity84`. The source
(`worktrees/parity84`, detached at the tag) and builds
(`BASE/parity84-{release,debug}`, aliased `BASE/golden84-{release,debug}`)
are frozen. The next wave is [hot85](hot85.md).
