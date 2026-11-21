# Profile83: where golden82 still trails upstream

On 2026-11-20 the user promoted gap82's integration to golden82 and asked for a
profiling pass: where is there still room for clean improvements? Profile83 is
analysis only; no source changed. Three agents compared golden82
(`5194e957c5d`, `BASE/golden82-release`) with upstream
(`upstream-release-devlibs-3a440b6`):

- P1: the evaluator, by handler, with gdb ledgers. It covered the evaluator
  kernels first, then the evaluator share of the library-heavy cases;
- P2: the 14 library-heavy cases and startup, by function;
- P3: cycles and the front end on CPU 15, all 29 cases.

Evidence is in `E59/profile83/`: `p1/report.md`, `p1/followup.md`,
`p2/report.md`, `p3/report.md` and their data; `coordinator.md` is the log.
Instruction counts are user-space, by 3L−L differencing. Savings are estimates
from per-visit ledgers times exact event counts; no change was built.

## Answer first

1. **The remaining gap is executed work.** Over the 19 cases where golden82
   uses more cycles than upstream, instructions account for 2.02 of the 2.03
   summed log-excess and cycles per instruction (CPI) for 0.01. Golden82 has the
   better CPI in 18 of 29 cases, mostly from fewer mispredicts. Only three
   cases lose time to CPI:
   - generators (CPI 1.19×; see [generators](#generators-a-back-end-stall));
   - unpack_sequence (1.08×, taken branches in the unpack loop);
   - fannkuch (1.08×, op-cache placement near capacity; it still wins on
     time with 10% fewer instructions).
2. **The ordinary paths are at parity or ahead.** Per call, `f(x, y)` is
   3 instructions cheaper than upstream, and a call/return pair costs 14
   more. Attribute loads and stores are 10–16 cheaper once upstream's
   receiver loads are counted; a C getset read is 205 cheaper. Loop control
   is 77 cheaper. A `yield from` level costs 73 + 40 against 83 + 37.
3. **What remains is fixed cost per event at boundaries:**
   - native calls: decoding the call and publishing the result
     (65–94 per call against about 20);
   - C code entering Python: eight layers against upstream's four
     (about 675 against 428 per entry);
   - creating and finishing generators;
   - Python calls that are not exact-args: defaults, keywords,
     `**kwargs`;
   - membership tests and iterators without an inline `FOR_ITER` form;
   - allocation entry, GC visits and the dict's first insert.

   Every item has a clean change (below), and none needs placement tuning.
   The evaluator rows of the library cases sum to about 630M instructions per
   loop gross, against about 380M of register-IR credits (locals, constants,
   generic attribute lookups), so handler tables overstate what is available.
4. **The hot set is 35% larger than upstream's** (lines covering 99% of
   samples, eight cases). Call and return handlers hold 322 of the 507 extra
   evaluator lines. The CPI model bounds what shrinking it can buy: at most
   0–3% of cycles in most cases. It is a secondary lever.

## Where each case stands

Time is the timed all-29 campaign (golden82 / upstream); instructions and
CPI are from P3's CPU 15 counts.

| Case | Time | Instr. | CPI | What dominates the excess |
| --- | ---: | ---: | ---: | --- |
| generators | 1.337 | 1.137 | 1.194 | creating each node's generator through a Python `__iter__` (+411 per value), finishing it (+115), a back-end stall in the resume path |
| deepcopy_reduce | 1.217 | 1.188 | 1.029 | evaluator and frame work in `copy.py` (37% of the gap), dict insert (12%), attribute getters |
| deepcopy | 1.196 | 1.189 | 1.015 | evaluator (86%): native-call publish, containers, 180 raises and catches per loop |
| logging_simple | 1.141 | 1.127 | 0.990 | Python-level record construction 12–24% dearer per function; `**kwargs` forward 10–15% |
| xml_etree_iterparse | 1.137 | 1.210 | 0.950 | a Python `__next__` resumed from C 204k times per loop (+209 each), allocation, C-call thunks, `_elementtree`, dict, GC visit |
| comprehensions | 1.135 | 1.169 | 0.972 | containers (dict get, list append, realloc), FOR_ITER, native-call publish, compare |
| unpack_sequence | 1.133 | 1.041 | 1.077 | taken branches in the unpack element loop |
| logging_format | 1.128 | 1.136 | 1.005 | as logging_simple |
| json_dumps | 1.107 | 1.121 | 1.019 | `snprintf` for ints (60% of its gap), the binder, borrow-stack growth |
| xml_etree_process | 1.101 | 1.164 | 0.938 | native calls, C entering Python (`TextIOWrapper.write` → `IncrementalDecoder.reset`), keyword binder, membership |
| xml_etree_parse | 1.098 | 1.078 | 1.031 | GC visit (+39M), allocation, dict, `_elementtree` |
| nqueens | 1.090 | 1.067 | 1.026 | generator steps and creation, MAKE_FUNCTION for genexprs |
| coroutines | 1.062 | 1.116 | 0.967 | generator finishing (+12.5%) and creation (+3.7% plus the call) |
| xml_etree_generate | 1.050 | 1.117 | 0.927 | as xml_etree_process |
| regex_compile | 1.043 | 1.131 | 0.930 | membership and element compare, native calls, generic FOR_ITER |
| the rest | 0.50–1.06 | | | at or ahead of upstream |

## Ranked clean improvements

Grouped by mechanism. "Saving" is in instructions; in the work-bound cases the
effect on time follows it. Sums are per loop over the profiled cases.

### A. Native calls and C entering Python (the largest)

| # | Change | Saving | Main cases |
| --- | --- | --- | --- |
| A1 | `call_publish` as one counted loop over the call's actual operands, not an unrolled four-operand take with five closes; decode length and self-state from one load; skip the displaced-value close when the destination is a temporary | 30–40 per native call; about −126M summed; comprehensions −3%, regex_compile −1.9%, xml −1.4 to −1.7%, go −1.5% | every case with builtin or method-descriptor calls |
| A2 | One native→Python entry: take the caller's argument window and push and initialize the frame in one routine, collapsing `PyObject_VectorcallMethod` → thunk → `_PyValue_WindowFromObjects` → `_PyEval_VectorValue` → `eval_frame_push_and_init_code` → `_PyEval_EvalFrameLoop` → `_PyEval_EvalFrameValue` | about 125 per entry (of +247); about −113M summed; xml_etree_generate −2.4%, process −2.2%, iterparse −2.0% (its `__next__` slot resume), raytrace −1.2 to −1.5% | xml, raytrace, regex_compile, slot dunders, `__init__` |
| A3 | Object-API thunk layer: call value-form callees directly where C code now goes through `value_vectorcall_thunk` (66 per call): `xmlparser_append_event`, `slot_tp_iternext_value` | about 25M (partly inside A2; count once) | xml_etree_iterparse, parse, process |
| A4 | Exceptions: build the exception from the argument window instead of an args tuple plus `PyObject_Call` (+400 to +900 per raise and catch) | about 15M; deepcopy up to −4% | deepcopy, xml (`element_getitem` IndexErrors) |
| A5 | `isinstance`: call `_PyValue_IsInstance` directly with the exact-type exit in the handler, instead of value → object → value through `PyObject_IsInstance` and four functions | 35–50 of +160 per call with A1; about −36M over 636k calls per loop in regex_compile and xml | xml cases, regex_compile, richards |

### B. Generator lifecycle

| # | Change | Saving | Main cases |
| --- | --- | --- | --- |
| B1 | Specialize `GET_ITER` (and the `yield from` operand) for a class with a Python `__iter__`: guard the type version and push the frame inline, as `CALL_ALLOC_AND_ENTER_INIT` does, instead of `_PyType_SlotIter` → `PyObject_CallOneArg` → `_PyEval_VectorValue` → a nested evaluator loop that only runs `RETURN_GENERATOR` (962 against upstream's 636) | ≥500 per iterable; generators −13% (instr. 1.137 → about 0.99) | generators |
| B2 | Finishing: one "plain generator" bit set at creation and cleared by whatever makes it non-plain (weakref, frame object, finalizer, exception state), replacing about 13 field tests in `_PyGen_TryCloseClearedNoReentry`; one loop for the three `GEN_CLOSE_INPUT` sequences; one slot-clearing pass | 60–80 per generator; coroutines −5.5 to −7% | coroutines, generators, nqueens |
| B3 | Creation: allocate the generator at the call and fill the arguments into `gen->gi_iframe`, deleting the stack frame, `_PyFrame_CopyCreatedGenerator` (58 against 27), the pop and the gap fill | 80–100 per generator; coroutines −8 to −10% | coroutines, generators, `any(genexpr)` |
| B4 | Resume: store the resume point past the `RESUME` word, and the return slot, at suspension, instead of `IR_SKIP_RESUME` and `_PyFrame_SetReturnValue` on every resume. This also removes the load chain behind the generators stall (below) | about 20 per step; nqueens −1.7% | nqueens, generators, iterparse |

B2–B4 touch the same creation and completion code and belong in one unit.
B4 must keep `PY_RESUME` events when monitoring is switched on while a
generator is suspended; the resumer can test the instrumentation version
with a predicted branch.

### C. Python binding

| # | Change | Saving | Main cases |
| --- | --- | --- | --- |
| C1 | Per-site bind plan for defaults and keywords (positional count, defaults start, keyword-to-slot map) and a straight-line binder; `call_bind_core` re-derives the plan every call (276 per defaulted call, 342–372 per keyword call; 51k keyword calls per xml_etree_generate loop) | 100–150 per call; about −34M; go −1.4%, richards −1%, xml −0.7% | go, richards, xml, json_dumps |
| C2 | `CALL_FUNCTION_EX` with a `**kwargs` dict binds in place, as it already does for a tuple (an empty `**kw` forward costs 1,258 against 602) | 10–15% of the logging gap | logging_format, logging_simple |

### D. Membership and iteration

| # | Change | Saving | Main cases |
| --- | --- | --- | --- |
| D1 | Membership: specialize `CONTAINS_OP_JUMP` for `str`, `tuple` and `list` containers and call the type function from the handler; the generic form runs out of line (92 instructions before the work), and the dict and set forms cost 81 and 90 against 37. In `_PyValue_RichCompareBool`, test identity and exact `str`/`int` first, as upstream's `PyObject_RichCompareBool` does (+93 per scanned `str` element) | 40 per test, 90 per scanned element; about −35M; xml_etree_process −1.3%, regex_compile −1.1% | regex_compile, xml |
| D2 | Generic `FOR_ITER`: extend the inline `FOR_ITER_VIRTUAL` form to `tp_iternext` iterators, with the exhaustion exit inline and shared with `FOR_ITER_GEN`, so `ir_run_FOR_ITER` and `_PyEval_CompleteForIter` are only the adaptive fallback | 50 per visit, 30 per exhaustion; about −42M; xml −1.0 to −1.2%, regex_compile −0.8% | xml, regex_compile, deepcopy |
| D3 | Unpack element loop: a branch-free ownership select and a loop with its own register allocation (now 24 instructions per element with three spilled invariants and two jumps; 49.5 taken branches per unpack against 33.5) | time −5 to −8% | unpack_sequence |

D1's compare exits also serve E6's int compares; do them once.

### E. Runtime memory, containers and single cases

| # | Change | Saving | Main cases |
| --- | --- | --- | --- |
| E1 | Allocation entry: choose the mimalloc heap once and drop the heap swap and the `PyObject_Malloc` hop for GC objects (+18 to +21 per wrapper layer; +45 to +87 per create-and-destroy) | about 35M; iterparse −3% | xml cases, regex_compile |
| E2 | GC visit: `_PyGC_VisitValue` inline in the header; visit dict keys in the tracing collector instead of `_Py_VISIT_ATOMIC` per unicode key (`dict_traverse` is 2.95× upstream's at startup) | 15–30M; parse −2% | xml_etree_parse, iterparse, startup |
| E3 | Dict: write the one-entry insert directly (`insert_to_emptydict` is a stub over `insertdict` and `dictresize`); compare unicode keys as objects instead of decoding on every probe | about 20M | xml, deepcopy, json, startup |
| E4 | `_elementtree`'s `makeuniversal`: look up with a known-bytes key and return a new reference with no boxing | about 10M | iterparse, parse |
| E5 | `json.dumps` integers through `_PyLong_WriteDecimal` instead of `PyOS_snprintf("%lld")` | about 7M; 60% of json_dumps' gap | json_dumps |
| E6 | Exact ints compared by word before `do_richcompare` (why they reach the slow path is not isolated) | about 18M, unconfirmed | regex_compile |
| E7 | `int ^= int` straight to `_PyLong_ValueXor`, with the span, output and word steps in one pass | go −1.7 to −2.2% | go |
| E8 | Borrow stack: give the thread a real base chunk, so the outermost scope does not grow and retire a chunk each time (540 per scope) | json_dumps −1% | json_dumps |
| E9 | One guarded truth test for "None or a type with no `__bool__` or `__len__`" | about 30 per test; generators −1% | generators |
| E10 | `MAKE_FUNCTION`: one globals and builtins probe in `PyFunction_NewWithQualName` | about 150 per genexpr | nqueens, comprehensions |

### F. Hot-code size (secondary)

Move guard misses, error, PEP 523 and non-plain paths of the call and return
handlers into one shared cold routine per family, and split the cold parts of
`insertdict` (3,180 bytes against upstream's 1,001), `generic_getattr_value`
and `call_bind_*`. Estimated 0–3% of cycles in the library cases, bounded by
the CPI model; the line reduction is assumed, not measured.

## Generators: a back-end stall

At gap82's IP2, generators lost 12–16% of cycles against unit G's build at
the same instruction count, and gap82 read it as placement. P3 finds instead:

- golden82 and G have the same instructions, taken branches, decoder and
  op-cache µops; golden82 takes 13% more cycles (171M against 151M per loop);
- the whole delta is `ic_fetch_stall.ic_stall_back_pressure` (+21.6M): the
  back end is full, not the front end starved;
- it sits in the generator transfer labels (`gen_resume`, `gen_yield`) and
  `LOAD_ATTR_SHAPE_LOCAL_TEMP`; every other handler's CPI is unchanged.

The best-supported cause is K2a's branch-free `IR_SKIP_RESUME`
(`26bf5bd0eab`): the next instruction pointer became the sum of three
loaded values, so the dispatch waits for them, where the old form was a
predicted branch. This is inferred from where the cycles moved and has not
been tested by a rebuild. B4 removes the computation from the resume path
altogether.

## Startup

`-S -B -c pass` is 63.7M instructions against 45.6M (+18.1M):

| Cause | Excess |
| --- | ---: |
| code-object construction and the IR build | +3.4M |
| dict operations | +3.2M |
| GC traverse and collect (`dict_traverse` alone +1.3M) | +3.1M |
| type creation | +2.4M |
| mimalloc core and heap setup | +1.9M |

E2 and E3 apply. The IR build is inherent until the deferred stack-bytecode
recovery or a cache.

## Not worth pursuing, or not clean

- Attribute handler bodies: their operand decode and release are paid for by
  the receiver loads the register IR removes.
- The exact-args call and return pair (+14 per pair) and `IS_OP_JUMP`
  (−12 per visit).
- Polymorphic call sites: +30 per iteration, and upstream misses there too.
- mimalloc against pymalloc: the allocator core is neutral; only the entry
  layers (E1) cost.
- Section grouping, alignment, PGO and LTO: placement, not structure. Both
  builds use gcc 15.2, `-O3`, no LTO and no PGO.
- `-fcf-protection=none` would favour upstream (its `endbr64` share is 2.5–3.3%
  of retired µops against golden82's 0.8–2.1%).
- fannkuch's op-cache loss and logging_silent: placement near capacity.

## Limits

- Savings are estimates, not built; where no ledger exists they assume 40–50%
  of the measured excess.
- IBS shares carry 10–25% error per item. P1 validated its handler map
  against an exact trace on generators (within 2.5 points). P2's two
  samplers disagree by 40–110M on rich compare, the malloc core and
  attributes; exact micros or ledgers decide those rows.
- The generators stall's cause is inferred, not tested.
- logging has no per-function gdb ledger; its split rests on per-function
  micros. json_dumps and deepcopy have no exact event counts.
- pyexpat's sign is undecided (−51M by one sampler, +16M by the other).
