# Project status

Status as of 2026-11-04 10:50 PST, from the coordinator fact sheet. Update this
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

- Runtime branch `boil/pyvalue-rc-universal`, HEAD `5de7bd62ef9`.
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
- Suites at `5de7bd62ef9`, debug and release alike: run=51,571, failures=2,
  both in test_capi (test_is_unique_temporary, test_unicode.test_new): a local
  passed to a call reads `sys.getrefcount` 2 against upstream's 1, because
  the call window owns a copy; open for IR v2 B5 (register-ir.md G4). `make check-value-api`: 177 units, 0 findings.
- All 67 shared modules whose third-party headers exist build. The decimal,
  tkinter, ssl, sqlite and zstd headers are not installed on this host.

## Active work packets

Running now:

1. Register IR v2 A5b phase 1: the typed-borrow design proposal
   (`A5B-BRIEF.md`), reviewed before implementation.

Next, in order:

2. A5b implementation; then A3a-A3c (tail calls become the only backend;
   `clang-21` and gcc 15.2 are installed)
   ([packet](design/proposals/register-ir-v2-packet.md)).
3. Value-form dict lookups for `CONTAINS_OP_DICT` (the `id()` key boxes).
4. Value API V1 follow-ups (box-site annotations in the interpreter files,
   `entry_unbox` counts, seven modules that define a bare `Py_BUILD_CORE`),
   then V2a-0 onwards.

## Design decisions

- [PyValue](design/pyvalue.md), [review 1](design/reviews/pyvalue-review-1.md):
  D1 G1' fixes (done); D2 canonical construction and release contract; D3 int51
  inline ints (chosen from the [int-width histogram](bench/results/pyvalue-int-width-20261103.md));
  D4 static borrowing (joint with IR-D3); D5 NULL = 0 (done, step 4); D6 ABI policy (done);
  D7 type-slot gap after baseline (rows updated); D8 G7 non-goal, G8 deleted.
- [Native boundary](design/native-boundary.md),
  [review 1](design/reviews/native-boundary-review-1.md): accepted with changes;
  the page describes P4 as built.
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
