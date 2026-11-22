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
| BD | Per-site bind plans for defaults and keywords; `CALL_FUNCTION_EX` with a `**kwargs` dict binds in place; generator callees bind into the new generator | C1, C2 | 2, from IP1 | running |
| GN | One generator creation path (the creation routine performs the prologue; `_PyFrame_CopyCreatedGenerator` and `RETURN_GENERATOR`'s copy deleted); one-pass slot clearing at finish | B2, B3 (GL's remainder) | 2, from IP1, beside BD | running |

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
