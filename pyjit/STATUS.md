# Project status

Status as of 2026-11-05 (PST), after the landings through `d686623e3fb`. Update this
page at each landing.

## Outcome and next milestone

The universal-value runtime links and runs, in debug and release builds. The
first checkpoint measurement (time and RSS, quiet-ish host) is in.
[Checkpoint](bench/results/checkpoint-20261103.md) and
[triage](bench/results/checkpoint-20261103-triage.md):

- `dbce7b1260d` ran the 29 kernels at 4.6x upstream `3a440b6`. Cause: natural
  method-JIT admission ran hot code through generic helpers. With admission off
  (`1f19952778d`), kernels take 2.6x upstream (geomean, interleaved runs). The
  interim build (`eba351fee7d`) takes 1.21x.
- Where the remaining gap goes: interpreter-loop self cost 39% (state spilled to
  the stack, a per-word `_PyValueTemps` record), exception save/restore around
  releases 25%, value bridging 17%, other 14%.
- The register IR executes 0.81 words per upstream instruction (geomean). Peak
  RSS is 34% above upstream, mostly startup (12.4 MiB against 9.5 MiB).

[Re-measure after step 6c](bench/results/remeasure-20261104.md) (`c8a0b4dc54c`,
one quiet window, kernels at load at most 1.2):

- Kernels take 2.27x upstream (geomean; 2.64x at `1f19952778d`), 14% less
  time than before step 5. pyperformance subset: 2.53x upstream, 12% less.
- Exception save/restore fell from 22.3% to 8.2% of the "before" time on the
  triage's five worst kernels: 83% of the saving. The loop's own cost did not
  change and is again the largest bucket (32% of the gap), then value
  bridging (28%) and save/restore (14%, the step 7 sites).
- Words executed and boxes are unchanged (0.805 words per upstream
  instruction). Peak RSS stays 28% above upstream.

[Second re-measure](bench/results/remeasure-20261104b.md) (`5de7bd62ef9`,
after representation steps 7-9b and register IR v2 B0a, A1, A2, A5a; quiet
window, load at most 1.8):

- Kernels take 1.73x upstream (geomean; 2.27x at `c8a0b4dc54c`), 23% less
  time; no kernel slower. pyperformance subset: 1.83x upstream (2.53x), 26%
  less. Worst five against upstream: deepcopy_memo, deltablue, generators,
  logging_silent, unpack_sequence.
- Exception save/restore is gone (9.6% of the previous time, no samples now).
  The loop's own cost is the largest bucket on all five triage kernels and
  44% of the gap to upstream; value bridging 25% (native-call scope and
  transaction, reader and attribute completions), other 19%.
- Words executed unchanged (0.805 per upstream instruction): the whole gain
  is cost per word. Release loop 407,525 to 272,546 B; weighted shortest
  path 49.0 to 40.7 instructions per word (upstream 22.0). Boxes +3.8%, from
  `id()` keys boxed at `CONTAINS_OP_DICT` (deepcopy_memo).
- RSS needs every build fully in the page cache first (measurement.md); with
  that, C and B match (B/C 1.010) and upstream uses 17% less (U/C 0.829).

These numbers are not a baseline. The next milestone: the typed borrow (A5b)
and tail calls as the only backend (A3a-A3c) for the loop and bridging
costs, then the qualified baseline.

## Position

- Runtime branch `boil/pyvalue-rc-universal`, HEAD `d686623e3fb` (I5). The 27
  landings since `b9d3e3a3c9b` are in the table under "Active work packets".
- Landed 2026-11-03: native boundary P3 (`56ff66a596b`, `643c69c7244`,
  `6d39221996d`) and P4 (`2558376b321`, post-review fixes `fdd7fed3216`);
  representation steps 1 (`f129780a9a7`), 2 (`f7dfc5b961f`), 3 (`dbce7b1260d`,
  with 3b `379e92cded9`), 4 (`7171541fa49`, `PyValue_NULL` is all zero bits)
  and 5 (`0bf3ad80ecf`, release contract and retire path); natural method-JIT
  admission off by default (`1f19952778d`); value API V1 (`f3a20271f26`
  additive part; `1a71a5cc2cf` rename to `PyValue_AsKnownObjectBorrow`/`Steal`
  with a check in every build; `946f622e1d1` `pyvalue.h` moved to
  `Include/internal`; `48bb4d6d1d3` `PyValue_ERROR` removed).
- Landed 2026-11-04: step 5 review fixes (`c4b01b23a54`); steps 6a
  (`cedd72de803`, 648 save/restore sites removed by
  `Tools/build/sweep_value_saves.py`, deleted in step 7d and kept in the
  history at `fbe866fae0a`), 6b (`f9bd0364fd9`, 834 helper calls
  inlined, 12 helpers deleted) and 6c (`fbe866fae0a`, 368 redundant
  `|| PyErr_Occurred()` terms); `PyDict_Pop` takes its status from the
  operation (`c8a0b4dc54c`; it failed whenever an exception was pending, which
  crashed release builds when an error path freed an interned string).
  Save/restore sites: 766 before step 6, 117 after.
- Landed 2026-11-04 (continued): representation step 7 (`ab07912b7cb` status
  APIs take their status from the operation, 88 of 89; `1a5600cc9f5` residue
  saves; `13ae4cfddda` reentry defences back to upstream form; `78908a19694`
  `_PyValue_RestoreException` deleted; `1a9d0a6fbbe` perf trampoline watcher
  fix). `PyErr_Occurred()` over files in both trees: 4,163 before step 7,
  3,068 after (upstream 3,083); `|| PyErr_Occurred()` 386 to 28 (28).
  Register IR v2 step B0a (`54825c08a1a`): the generated instruction contract
  table; the passes, assembler, `dis` and verifier read it; compiled output
  unchanged.
- Landed 2026-11-04 (continued): representation step 8. `5640c46361c` step 7
  review fixes (an iterator step whose box fails consumes the item; a pop
  boxes before it removes). `1381d5b49c0` the home checker counts stores, at
  the frame store macros and the dict, list and set writers. `308a86c3914`
  canonical construction (an exact int that fits inline or an exact non-NaN
  float is always stored inline; `Mortal` variants and the generator's
  `POINTER_CONSTRUCTORS` deleted) and `PyValue_Is` as a bit compare (the
  deleted fallback had 0 true results in 3.59 billion calls over the suite);
  the home checker is a debug assert. `f6508007b94` `object.__hash__`,
  `__repr__`, `__eq__` and method equality use value identity
  (`_PyObject_IsSame`). Int and float guards still accept boxes until step 9a.
- Landed 2026-11-04 (continued): register IR v2 A1 (`44eb4e716dd`, the value
  record only in the 115 handlers whose contract row has BOXES) and A2
  (`335b86b71e9`, `next_instr` passed by value to the 12 helper families; the
  executing copy is fixed for the code object's life), with `c8637fe7c6b`
  (a failed re-instrumentation walk no longer trips A2's debug check; the
  cases generator refuses a value use of an input it boxed). Release loop
  406 KB to 303 KB. Kernel time against `54825c08a1a` (information, not a
  gate): A1 0.983, A2 alone 1.024, both 0.986; A2's gain is for the
  tail-call build (A3a). Compiled output unchanged.
- Landed 2026-11-04 (continued): register IR v2 A5a. `3c7ac067f7f` the home
  checker rejects a borrowed mortal in a frame slot (before the change: 510.5M
  such stores at 13 sites over 623 modules, 364M from `LOAD_FAST_BORROW`).
  `d60d3be46d6` no frame slot holds a borrow: `LOAD_FAST_BORROW` and the
  other producers take a count for a mortal, in-place reads of locals get
  conditional releases from the cases generator, the window `OwnArray` calls
  become a debug recheck, magic 64403 (census after: 0). `3e28e1755bd`
  post-review: the generator follows copies and object pointers of in-place
  inputs. Release loop 308,756 to 279,995 B; kernel time 0.939-0.949 of
  `c8637fe7c6b` (information, not a gate).
- Landed 2026-11-04 (continued): representation step 9a (`bf0639df819`,
  inline ints are int51; numeric guards inline-only; magic 64404) and 9b
  (`80bb9c84701`, the trace JIT carries scaled int64 scalars). The review's
  differential fuzzer (160,000 operator cases, 3,680 random traced loops)
  matched upstream everywhere. Kernel time 0.964 of `c8637fe7c6b`
  (information, not a gate). `5de7bd62ef9`: release order around calls
  matches upstream (temporaries cleared from the highest slot down; a call's
  local result stored after the window retires and after the periodic
  check; `CALL_NON_PY_GENERAL` has upstream's periodic check), and five fork
  tests check the current rules: 9 of the 11 long-standing failures fixed.
- Landed 2026-11-04 (continued): value-form container calls
  (`2f572ea0ea8`). Eleven sites that boxed an inline input for a
  `PyObject *` call now call existing value-form functions: dict and set
  contains, compare with two inline operands, `SET_ADD`, `BUILD_SET`,
  `MAP_ADD`, `STORE_GLOBAL` and `STORE_NAME` on an exact dict, `DELETE_SUBSCR`
  on an exact dict, list store and list or tuple subscript with an inline
  index. Boxes per loop: nqueens 541,065 to 40,325, raytrace 1.34M to 0.93M,
  go 229k to 125k, chaos 484k to 315k, deepcopy_memo 207 to 102. Compiled
  output unchanged. The sites left need value-form functions from value API
  V2a-V5 (FOLLOWUPS.md, "Box sites").
- Landed 2026-11-04 (continued): register IR v2 A5b-1 (`a6002e5854b`), the
  typed borrow's type and readers: `PyValueBorrow`, `PyValue_Borrow` as the
  view, readers behind same-name `_Generic` macros, `-Werror=incompatible-pointer-types`
  for in-tree units, negative compile tests in `test_check_value_api`. No run-time
  change; compiled output unchanged. Integration caught the box-site commit's
  one `MakeHeapSafe(PyValue_Borrow(...))`, which no longer compiled (renamed to
  the interim tagging form).
- Landed 2026-11-04 (continued): the fork's JIT is out of the build
  (`faddfd1454d`; `_PyJIT_BASELINE_SUPPORTED` is 0 everywhere, the source
  stays, `test_jit_method` skips) until the JIT stage rebuilds it on the final
  IR. User decision, so IR steps carry no JIT upkeep.
- Landed 2026-11-04 (continued): upstream CPython's JIT remnants removed
  (`36aa4a0345a`): `jit_publish.c`, `jit_unwind.c` and their headers, build
  entries and configure check; the perf eh_frame builder moved into
  `perf_jit_trampoline.c`. `sys._jit` now reports the fork's JIT
  (`is_available` is `_PyJIT_BASELINE_SUPPORTED`, so False for now), with three
  new tests. `regen-configure` reproduces the edited `configure`. Targeted
  tests run=2,085, failures=2 (the two `test_capi` failures, since fixed); its own suites run=51,617,
  failures=2.
- Landed 2026-11-04 (continued): register IR v2 A5b-2 (`fc981364c56`): the
  generator gives each handler input one C type, a view or an owner, with
  `TAKE` and `OWNER` where a view becomes an owner; S3, S7 and the spend
  rules fail closed. Post-review found three fail-open rules (none fired on
  `bytecodes.c`); all 12 findings fixed before landing. Generated release
  order unchanged. Suite (A5b's T_2 before the fixes, gcc debug): run=51,621, failures=2; the shared tree's full debug suite at `df1503353d7`: run=51,646, failures=2 (the two `test_capi` failures, since fixed in `c6fb9beb79f`).
- Landed 2026-11-04 (continued): value API M4a (`df1503353d7`): the value
  field contract tested on a type with fields of each kind;
  `structseq.__reduce__` stores by value. Suite (M4's T_1): run=51,631,
  failures=2.
- Landed 2026-11-04 (continued): SOABI `boilvalrc2` (`6ce98fe4790`) and value
  API V2a-0 part 1 (`e8cb83c42be`): 53 value-slot fields in `PyTypeObject`
  as opaque storage, the slot table, inheritance pass and debug invariant,
  `PyValue_NotImplemented`, thunk and NotImplemented macros; no type
  converted. Post-review: one must-fix (the invariant check never saw a
  Python class's final slots) fixed before landing. Suite (V2a-0's T_1):
  run=51,636, failures=2; targeted on both shared builds 4,742, failures=2.
- Landed 2026-11-04 (continued): value API M4b (`ca5b20c64d9`): slice bounds
  are value homes, `PySliceObject` is internal, `_PySlice_FromValues`; the
  readers use inline ints directly. Post-review: no must-fix; three missing
  regression tests added. A `Py_STATS` build fix after A5b-2
  (`d568bea0c45`).
- Landed 2026-11-04 (continued): register IR v2 A3 step 1 (`7b33e876eb3`,
  `c51200c5a1c`, `e7a1322e93a`, `15859d9a592`): the fork's JIT hooks out and
  its source deleted (user decision; history keeps it); tail calls are the
  only backend (`preserve_none` on Clang, `no_callee_saved_registers` on
  GCC, recorded as `PY_TAIL_CALL_CC`); `--with-computed-gotos` and
  `--disable-gil` are gone; an activation record per eval loop; two helper
  fixes take the canary functions from 200 to 133. Suites at `15859d9a592`,
  gcc debug and release: run=51,627, failures=2 (the two `test_capi`
  failures, since fixed in `c6fb9beb79f`; 60 JIT tests deleted). The agent's clang-21 release suite:
  run=51,586, failures=2.
- Landed 2026-11-04 (continued): register IR v2 A5b-3 (`c71a3ca6aa2`), the
  typed borrow's heap side: producers that lend a value return
  `PyValueBorrow`; stores take owners with `PyValue_DUP`; `MakeHeapSafe`,
  `IsHeapSafe`, `OwnArray`, `BorrowAt`, `AsStrongReference` and
  `_PyValue_BorrowTagged` deleted, so no mortal carries the no-count tag;
  the heap-safety asserts become the home check; `PyDict_*Value`,
  `PySet_ContainsValue` and the cell value forms are internal; R26 lints
  `raw-bits` (widened), `public-value`, `borrow-field`, report-only
  `home-copy` (93). An 8-worker Sonnet fan-out fixed 186 of 194 compile
  sites in 3.6 minutes. Post-review: no must-fix, 4 should-fix (two lint
  holes, public getters returning a borrow, the home-copy retype), all in
  A5b-4. With it, `eab52a937a6` lets A3's activation record sit at the stack
  pointer (clang-21 debug aborted). Suites at `c71a3ca6aa2`, gcc debug and
  release: run=51,630, failures=2 (the two `test_capi` failures, since fixed).
- Landed 2026-11-04 (continued): value API V2a-0 part 2 (`f7d374b1f92`):
  slotdef value data and pairs, the value dispatchers and slot adapter, core
  object slots as thunks, box kinds in stats builds, the compat-body lint
  rule; no type converted (V2a-1a converts int, float, bool, None).
  Post-review: no must-fix; three should-fix fixed before landing (box
  helpers in the compat-body rule, the mixed-operand box record:
  `x == 'a'` boxes 1 before, 3 now, removed by V2a-1a; an
  `object.__hash__` test). Suites at `f7d374b1f92`, gcc debug and release:
  run=51,681, failures=2 (the two `test_capi` failures, since fixed). `make check-value-api`: 169
  units, 0 failing findings.
- Landed 2026-11-04 (continued), since `f7d374b1f92`:
  - `0bf1b0486a5`, A3 step 2: register-IR handlers have no addressable locals
    (G9). Helper results return as two-word structs or `_PyStatusValue` in
    registers; call transactions keep their record in an out-of-line entry's
    frame. The generator's lint refuses an address taken of a handler local.
    The box-area handlers are pinned at 97 and may only shrink.
    `enter_handlers()` is a normal-ABI trampoline, the only non-tail call into
    a handler. It works around a GCC 15.2 miscompile (values live across a
    call into a `no_callee_saved_registers` function are reloaded from a
    callee-saved register that was never set); reproducer
    `build/pyjit-link-wave1/a3/gcc-ncsr-bug.c`, risk R13 in
    [the A3 proposal](design/proposals/register-ir-v2-a3.md).
  - `c6fb9beb79f`: the two long-standing test_capi failures
    (test_is_unique_temporary, test_unicode.test_new) are fixed; they compare
    `sys.getrefcount` with a fresh local, because the call window owns a copy.
    Since then the suites have run with 0 failures.
  - `763c8ececd5`, A5b-4: the ownership primitives take their final names,
    `PyValue_DUP(PyValue_FromObjectBorrow(o))`, `PyValue_XDUP`,
    `PyValue_XCLOSE` and `PyValue_FromObjectSteal`. Home copies are typed
    `PyValueBorrow`, with 56 commented exceptions. `PyList_GET_VALUE` and
    `PyStructSequence_GET_VALUE` are internal. The A5b-3 review's lint fixes
    are in.
  - `c66a6339ccf`, W1b: the list, tuple, dict, set, cell and struct-sequence
    object forms are compatibility-only, so a core call to one is a compile
    error. 1,064 sites in 86 files: 814 by script or generator, 234 by Haiku
    and Sonnet workers. The 44 sites in abstract.c, descrobject.c, object.c
    and typeobject.c are deferred. New tools: `value_sweep.py` and
    `wave_files.py`.
  - `b9d3e3a3c9b`, M3: the value vectorcall and `METH_VALUE` (0x0400), with
    `im_self` and `m_self` as homes, and the `_Py_wrappervaluefunc` wrapper
    shape. SOABI `boilvalrc3`. Argument Clinic emits value functions through a
    `value_emission` directive, with getargs value helpers.
    `_testclinic_value` is not yet built: it needs V2a-1a.
  - Suites at `b9d3e3a3c9b`, gcc debug and release: run=51,810, failures=0.
    `make check-value-api`: 169 units, 0 failing findings, 5 deferred.
  - Follow-ups (`build/pyjit-link-wave1/FOLLOWUPS.md`): FOR_ITER's monitoring
    events differ from upstream (I3); `await ag.athrow(StopIteration(5))`
    aborts the debug build, pre-existing (I3); M3b message names (W2).
- Suites at `faddfd1454d`, debug and release alike: run=51,615, failures=2
  (38 more skipped than at `a6002e5854b`: the JIT tests),
  both in test_capi (test_is_unique_temporary, test_unicode.test_new): a local
  passed to a call reads `sys.getrefcount` 2 against upstream's 1, because
  the call window owns a copy. Fixed in `c6fb9beb79f` by comparing against a
  fresh local. `make check-value-api`: 177 units, 0 findings.
- All 67 shared modules whose third-party headers exist build. The decimal,
  tkinter, ssl, sqlite and zstd headers are not installed on this host.

## Active work packets

Landed since `b9d3e3a3c9b`, oldest first (2026-11-04 to 2026-11-05):

| Commit | Step | Change |
|---|---|---|
| `9f40967b7e3` | value API G-1 | Box primitives take `_PyBoxAt` (owner, site, kind) in stats builds. |
| `c0d363672a5` | register IR B-pin | Handler numbers live in a checked-in table; magic stays 64404. |
| `edfbcc58928` | value API V2a-1a | int, float, bool and None value slots, digit-span kernels; 1.4 million differential cases match. |
| `2759372ebc7` | register IR B1 | "Too many temporaries" is a SyntaxError that names the limit. |
| `53c8c8ad715` | register IR B2 | `NOT_TAKEN` and four dead handlers gone; jumps fire `BRANCH_LEFT`; Lib has 82,051 fewer static words (1.74%). Magic 64405. |
| `13053640663` | value API M1b | Subscript, item, contains, iteration and send dispatch on values. |
| `01e8f3bc4ea` | value API M2 | Attribute, descriptor and getset dispatch on value receivers. |
| `e891a10794e` | value API W1 batch-10 | The 44 deferred container sites use value forms. |
| `d68db1c040e` | register IR B7 | Scalar constants live in hidden frame slots; Lib has 0.34% fewer static words. Magic 64406. |
| `342ff1b1b31` | value API I2 | Slices build and subscript on values. |
| `1dadf75de5a` | value API I1 | Operator, comparison, truth and conversion handlers run on values. |
| `c59b17d7836` | value API G | R27 box checker `check_r27.py`; stats summary attributes box sites. |
| `9fdea3b9310` | fix | Regenerated `test_frozenmain.h` for magic 64406. |
| `e1572ec086c` | value API V2a-1a-iii | Python classes get value sides for operator, compare, hash, repr and item slots. |
| `9563654f360` | register IR B4 | Feedback entries sized by kind, binary-search lookup, NOFB handlers gone; feedback memory on Lib falls 24%. Magic 64407. |
| `4eeb6ea2ff2` | register IR B6 | A compare and its conditional jump on one line become one `COMPARE_OP_JUMP`, `CONTAINS_OP_JUMP` or `IS_OP_JUMP` word; on Lib 12,989 pairs fuse and 118 stay apart. Forward jumps decode a signed distance. Magic 64408. |
| `1fc13423e46` | register IR B5 | A call whose result a `POP_TOP` discards, with a word of the line after it, has no destination; every `PUSH_NULL` is elided (17,461 words to 0 on Lib; `POP_TOP` 38,503 to 30,021). `f_lineno` models stack slots from the words. Magic 64409. |
| `56906c53125` | fix (B7) | A pushed frame writes its self slot only when the call has a self. A specialised call from a coroutine had erased a hidden constant (asyncio `test_shield_effect` segfault). |
| `64eab46d9da` | fix | `test_code` drops `PUSH_NULL` from its list of artificial instructions. |
| `1996cbbb080` | value API I4 | Attribute, `super` and import handlers call the value dispatchers; `import x` in a loop makes no box. |
| `b904ab8f014` | value API I3 | Subscript store and delete, `FOR_ITER`, `SEND`, `GET_LEN`, `MATCH_KEYS` and `MATCH_CLASS` call the value dispatchers. |
| `5c94f78a582` | value API I3, `INTERPRETER_EXIT` | The evaluation loop returns `PyValue`; only `_PyEval_EvalFrameDefault`, the PEP 523 path and the perf trampoline box the result. |
| `e94ed139c64` | tests | `test_pyvalue_box_sites` covers the I1 to I4 handlers; two stale descriptions corrected. |
| `1a9b96b1000` | value API W4-0 | Fields of eight modules (deque, itertools, `_pickle`, `_functools`, `_operator`, `_queue`, `_asyncio`, `_elementtree`) are value homes; `_PyValue_AsObjectStealAt` boxes at object exits. |
| `8eee8081ca0` | register IR B5b | `UNPACK_SEQUENCE` and `UNPACK_EX` write consecutive locals (2,408 of 3,343 unpack sites on non-test Lib); `LIST_APPEND`, `SET_ADD` and `MAP_ADD` read their items from fields. Lib loses 5,415 static words. Magic 64410. |
| `a1ecc0eb4f3` | tests | Two stale tests fixed after the integration point's full suites (hidden constant slots; the code constructor's new optional arguments). |
| `d686623e3fb` | value API I5 | Call handlers, monitoring and tracing run on values; seven boxing call helpers and the vector halves of the native scope are deleted. The monitored kernel makes 0 boxes (was 4,000). |

Validation:

- Integration point, 2026-11-05: the full suites at `8eee8081ca0`, release and
  debug, ran 52,178 tests each. Two failed, both stale tests; `a1ecc0eb4f3`
  fixed them. (Earlier full pass: `01e8f3bc4ea`, M2, 51,977 tests each.)
- I5 ran its step tests and a smoke set on both builds. Full suites run only at
  integration points (user rule, 2026-11-05; see [PROCESS](PROCESS.md)).
- Magic numbers: 64405 after B2, 64406 after B7, 64407 after B4, 64408 after
  B6, 64409 after B5, 64410 after B5b. The coordinator bumps the number once per
  format landing with `tools/bump_magic.py`.
- B7 left `Programs/test_frozenmain.h` stale (marshal layout). `9fdea3b9310`
  regenerated it. The landing script now regenerates it and runs `test_embed`.

User decisions, 2026-11-05:

- Do not harden against hand-crafted code objects or marshal streams.
- After the interpreter checkpoint, `co_code` and `.pyc` return to stack
  bytecode with the IR as a hidden cache
  ([proposal](design/proposals/stack-bytecode-recovery.md)).

In flight (briefs in `build/pyjit-link-wave1/`):

1. W2, the core wave, is running. It lands in two parts. W2a is Clinic
   regeneration and slots. W2b is callers, fields and the deferred `ceval.c`
   sites. W4 starts from W2a.
2. G2 has the A4 and B3 prototypes off-tree. Open question 9 is counted: a
   fixed-dst rule removes 9 to 14% of class tests at no word cost, so the
   decision stays between A4 and B3
   ([register-ir.md](design/register-ir.md) G2).

Next: W2a, W2b, W4, W5 and the R27 gate. G2's timed decision runs on a quiet
host. Then the checkpoint measurement. After the checkpoint comes the
stack-bytecode recovery
([proposal](design/proposals/stack-bytecode-recovery.md)).

## Design decisions

- [PyValue](design/pyvalue.md), [review 1](design/reviews/pyvalue-review-1.md):
  D1 G1' fixes (done); D2 canonical construction and release contract; D3 int51
  inline ints (chosen from the [int-width histogram](bench/results/pyvalue-int-width-20261103.md));
  D4 static borrowing (joint with IR-D3); D5 NULL = 0 (done, step 4); D6 ABI policy (done);
  D7 type-slot gap after baseline (rows updated); D8 G7 non-goal, G8 deleted.
- [Native boundary](design/native-boundary.md),
  [review 1](design/reviews/native-boundary-review-1.md): accepted with changes;
  the page describes P4 as built.
- [A3 tail calls](design/proposals/register-ir-v2-a3.md), revision 3 after
  [review 1](design/reviews/register-ir-v2-a3-review-1.md) and the user's
  review: no addressable locals in handlers by construction; 128 B record;
  two landings, 16.5 agent-h, about 6 h.
- [Value-API route to R27](design/proposals/value-api-v2a0.md), revision 3
  after [review 1](design/reviews/value-api-v2a0-review-1.md): mechanism
  steps, then compiler-driven waves; inline value slots; gate attribution by
  boundary address.
- [Value API](design/value-api.md): values are the internal API and
  `PyObject *` is the compatibility layer. Review 1, revision 2, review 2 and
  revision 3 (`74236263ef1`) are done; accepted for packet V1.
- [Register IR](design/register-ir.md), [review 1](design/reviews/register-ir-review-1.md),
  [review 2](design/reviews/register-ir-review-2.md): accepted after review 2.
  IR-D1 one encoding revision (IR v2) with a bit budget; IR-D2 generated
  instruction contract; IR-D3 ownership = static slot class plus value tag;
  IR-D4 JIT boundary state is open. One limits rule; names past 4,095 lower to
  an intrinsic; 8-byte feedback offsets.
- [Representation packet](design/proposals/pyvalue-representation-packet.md),
  [review](design/reviews/pyvalue-representation-packet-review-1.md): accepted
  plan for D2, D3, D5, G1', G3, G10 and G11.

## Measurement setup

- Debug builds: `build/pyvalue-rc-debug-20261102-v1`,
  `build/pyvalue-rc-bringup-debug`, `build/pyvalue-rc-triage-rt-debug`.
  Release build: `build/pyvalue-rc-release-20261103` (mimalloc, computed gotos,
  shapes, -O3).
- Upstream control: `build/upstream-release-3a440b6` (`3a440b65695`, -O3).
  `kernel.perf_event_paranoid=4` blocks `perf`, so the instruction metric is
  unavailable. `tools/sampler/` has an LD_PRELOAD SIGPROF sampler for profiles.
- Lessons from the checkpoint:
  - `sys._jit` covers only upstream's JIT. The harness must also assert the
    fork's method-JIT state (natural admission).
  - The pystats build compiles that JIT out, so pystats dispatch counts do not
    see it.
  - A `pybuilddir.txt` of `none` makes a release build 5 to 12x slower. Check it.

## Stages

Stage 1 ([runtime integration](stages/01-runtime/README.md)) is the critical
path. Stage 2 ([baseline](stages/02-baseline/README.md)) is blocked by it.
Stages [3](stages/03-method-jit/README.md), [4](stages/04-tracing-jit/README.md)
and [5](stages/05-extensions/README.md) are deferred or waiting on the
native-boundary work. See [HANDOFF](HANDOFF.md), [PROCESS](PROCESS.md) and
[WORKLOG](WORKLOG.md).
