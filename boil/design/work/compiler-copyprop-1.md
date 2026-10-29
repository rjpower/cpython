# compiler-copyprop rung 1: a pending load does not survive an op that runs code

Status: implemented; landing-review fixups recorded below, 2026-10-29. Branch
`boil/compiler-copyprop`, from integration 41c716c5947. Owner: the
compiler-fix agent. Inputs: review 2 §5.1 (`boil/ARCHITECTURE_REVIEW_2.md`)
and the PROCESS.md 0.1 and 0.3 rows.

## The defect

`_PyRegPass_CopyProp()` (`Python/regcfg_passes.c`) folds a
`LOAD_FAST_BORROW` of local x into the op that reads it when nothing
writes x in between and the line does not change. The reader then reads x
when it runs, not when the load ran. Any op in between that runs Python
code can rebind x through `sys._getframe().f_locals`: a call, a finalizer,
a signal handler, a monitoring callback, or another thread at a GIL
switch. The stack machine has already pushed the old value at that point.

Reproducers on the 41c716c5947 build, giving the fork's result and then
upstream 3.14's:

- a call: `return x + change()`, where `change()` rebinds x, gives 2
  against 1;
- globals that are a dict subclass: in `x + G`, the globals'
  `__getitem__` rebinds x (`_PyEval_LoadGlobalValue` calls
  `PyMapping_GetOptionalItem`), giving 12 against 11;
- globals that are an exact dict: in `x + GB`, `GB` is a builtin and the
  module dict holds a `str` subclass key with the same hash. Its `__eq__`
  runs during the lookup and rebinds x, giving 12 against 11.

The brief assumed that folds under a global stay. The second and third
reproducers overturn that: a generic `LOAD_GLOBAL` runs Python code. The
coordinator ruled at 16:05 PDT.

Review round 1 found a second path, which needs no op between the load
and the reader. A specializer that runs Python code makes the word be
dispatched again (`DISPATCH_SAME_OPARG()`), and that dispatch reads every
field anew. `specialize_dict_hint()` looks the attribute up in an instance
dict whose keys are not all exact str, which compares a colliding key and
so calls its `__eq__`. With `x = o; return x.a`, where o's `__dict__` holds
such a key, the fork returned the attribute of the object that `__eq__`
bound to x in 4 of 200 calls; upstream did so in 0.

## The rule

A pending load dies at the first op after it that can run Python code,
unless that op is its reader. The existing kills stay: a write to x, and
a line change.

"Can run Python code" is a property of the op's handler family, which the
cases generator derives. There is no list in the pass. A family runs code
if any member does one of the following. The members are the generic
handler, its specialisations, its instrumented twin and the IR-only
handlers named after a member.

1. It makes an escaping call, which `call_effects.py` defines as the
   reentrant kind (`Properties.escapes`).
2. It checks the eval breaker (`eval_breaker`).
3. It switches frames: it pushes a frame (`switches_frame`), or returns
   or yields to one (`returns`). `CALL_PY_EXACT_ARGS`,
   `LOAD_ATTR_PROPERTY`, `BINARY_OP_SUBSCR_GETITEM`, `FOR_ITER_GEN` and
   `SEND_GEN` push a frame, and none of them is flagged as escaping.
4. It is an instrumented twin (`INSTRUMENTED_X`). A twin exists to fire a
   monitoring event, and the event calls a Python callback. The
   `INSTRUMENTED_JUMP` macro hides that call from the escape analysis, so
   `INSTRUMENTED_NOT_TAKEN`, `_POP_ITER`, `_JUMP_FORWARD` and
   `_POP_JUMP_IF_[NOT_]NONE` have no escaping call.

A uop annotated `specializing` is not counted (see the contract below).
`Properties.escapes` ignores a call on a path that ends in an error exit,
as in `RAISE_VARARGS`. That path unwinds to a handler block, which starts
a region, and the pass clears the pending set there. So the reader is
never reached past such a call.

The generator sets `IRFormat.runs_code` in `ir_generator.compute_format()`,
beside the borrow mask, which is also a family property. It emits
`_PyIR_RUNS_CODE` in `pycore_ir_metadata.h`, as bit 128 of the `uint8_t`
`_PyIRFormat.flags`. Bits 1, 2, 4, 8, 32 and 64 are in use and 16 stays
free. The pass tests `_PyRegCfg_FormatOf(op->opcode)->flags &
_PyIR_RUNS_CODE`. `_opcode.get_ir_formats()` returns the flags unchanged.
No format changed except for gaining the flag.

**The reader side.** A handler reads an input field at the first uop that
takes the input, which need not be its first uop. A borrowable input is
sound only if every member of the family takes it before any of its uops
runs code. The generator clears the borrow bit of an input that a member
takes after a uop that runs code. Today no mask changes:
`STORE_SUBSCR`'s value is taken after `_SPECIALIZE_STORE_SUBSCR`, which
is a specializer.

**The specializer contract.** A specializer runs no Python code. It
inspects types and versions and rewrites the handler. When it
specialises, the same word is dispatched again and reads every field
anew, so a specializer that ran code would expose an immediate reader as
well. The generator relies on the `specializing` annotation for this. The
contract is enforced as follows:

- `specialize_dict_hint()` declines a dict whose keys are not all exact
  str, before the lookup, as the module and global specializers already
  do (`dk_kind != DICT_KEYS_UNICODE`). This is its own commit, approved by
  the coordinator.
- MRO lookups also require the pre-lookup `type_lookup_runs_no_code()`
  guard. A cache miss compares class-namespace keys, including allowed
  str subclasses whose `__eq__` runs Python. The original pressure-based
  probes missed this path; invalidating the class version before each
  attempt reaches it. The guard precedes every specialization lookup,
  including statistics-only lookups. The forced-phase test below proves
  the listed load/store/subscript paths, not every possible native path.
- The checker that enforces the contract, by classifying the callees of
  the `_Py_Specialize_*` functions, is a PROCESS.md 0.3 row owned by 10a
  tooling, which the coordinator added.

## Drain points, and the bound on the claim

The live collector runs Python code from the queue (weak reference
callbacks, `__del__`, `tp_del`, the destroy hooks), from `gc.callbacks`,
and from `DEBUG_STATS` output, in three places:

- `run_queue()` (`Python/gc_live.c`), reached from `_Py_RunGC()` at the
  eval breaker (clause 2);
- an explicit collection (`gc.collect()`, `PyGC_Collect()`), reached only
  through a call (clause 1);
- `_PyGC_LiveRunQueue()`, which `sys.settrace`, `sys.setprofile` and
  `sys.monitoring` call when they install a hook, also through a call.

A collection that an allocation starts (`_PyGC_LiveOnThreshold()` calls
`_PyGC_LiveCollectHere()`) sets `no_python`. It queues the calls, writes
debug output with `fputs`, skips `gc.callbacks` and schedules the
breaker. With no reference counting, `STORE_FAST`, `POP_TOP` and
`DELETE_FAST` release nothing. An op that only allocates (`BUILD_TUPLE`,
`MAKE_CELL`, `LIST_APPEND`) therefore does not kill a pending load.

The claim has one exception, already on record. A dealloc in the sweep
can call `PyBuffer_Release()` (`picklebuf_dealloc()`,
`unpackiter_dealloc()`), which can reach a Python `__release_buffer__`
(review 2 §5.3, the 0.3 row owned by 10a rung 3). Until that row closes,
an allocating op can run Python code in that case. The coordinator ruled
that the claim is bounded on that row, and added a clause to the row's
acceptance test: no Python code under an op that only allocates.

Treating allocating ops as running code until then was rejected. It would
lose the folds under `BUILD_TUPLE` and the like to work around a
collector defect that has an owner and a fix. Removing the workaround
later would be a second change to the same pass. That is the stopgap form
PROCESS.md section 3 rules out.

## Not covered: per-instruction events (a compatibility break)

`INSTRUMENTED_INSTRUCTION` (sys.monitoring INSTRUCTION events) and
`f_trace_opcodes` fire before every IR word. Take a callback at the
reader's word, or at any word between the load and the reader, that
rebinds x. The reader sees the new value, where upstream kept the old one
on the stack. Covering this would mean no fold under any word that can be
instrumented, which is every fold. That alternative is rejected (ruling of
16:05 PDT). The coordinator adds the PROCESS.md 0.2 row at landing. PROCESS
§0.2 asks the rung that makes a break to document it. The documentation is
a note under the INSTRUCTION event in `Doc/library/sys.monitoring.rst`
and under `frame.f_trace_opcodes` in `Doc/reference/datamodel.rst`; this
needs the coordinator's approval, since `Doc/` is outside the brief's
files. Line events are covered by the same-line rule, which is unchanged.

## Folds lost

A load folds only when its reader is the next op after it that runs code.
Folds stay under:

- constants (`LOAD_CONST`, `LOAD_SMALL_INT`, `LOAD_COMMON_CONSTANT`);
- other locals, and `LOAD_DEREF`;
- `PUSH_NULL`, `COPY`, `SWAP`, `NOP`, `BUILD_TUPLE` and `IS_OP`;
- wherever the reader comes right after the load (`x.a`, `x[i]`,
  `x + y`).

Folds are lost under `LOAD_GLOBAL`, `LOAD_ATTR`, `BINARY_OP`,
`COMPARE_OP`, `CALL` and the like: `x + G`, `x + o.a`, `x * (a + b)`,
`x[i + 1]`, `x < len(y)`.

The measurement is a static count over Lib with
`boil/tools/measure/fold_count.py`. The script compiles every file and
walks `dis`. For each remaining `LOAD_FAST_BORROW`, it records what stops
the scan first: the reader, a line change, or an op flagged
`_PyIR_RUNS_CODE`. At 41c716c5947, 2,075 files leave 223,617 such words.
The record will give the count after the fix with the same sources, the
split by stopping op, and the total IR words. Gate 4 records the
instruction deltas against `boil/bench/results/6d.json`, attributed and
not tuned.

## Changes and file ownership

- `Tools/cases_generator/ir_generator.py`: `uop_runs_code()`,
  `runs_code()`, `inputs_read_after_code()`, `IRFormat.runs_code`.
  `ir_metadata_generator.py` emits the flag. `Include/internal/pycore_ir.h`
  defines `_PyIR_RUNS_CODE`. `pycore_ir_metadata.h` is regenerated.
- `Python/regcfg_passes.c`: the kill and the pass comment, together with
  the pyc magic bump to 3729 in `Include/internal/pycore_magic_number.h`.
- `Python/specialize.c`: the guard in `specialize_dict_hint()`, in its own
  commit.
- Tests: `Lib/test/test_peepholer.py` (`CopyPropagationAcrossCodeTests`)
  and `Lib/test/test_generated_cases.py`.
- Notes: note 06 §13.1 (the paragraph "A fold across a call reads the
  local after the call" is replaced by the rule), note 07's pass table,
  and this document.
- `boil/tools/measure/fold_count.py`.

No file of another running rung is touched. `call_effects.py`, which 10a
rung 2b is editing, stays unchanged.

## Tests

Each semantic test records evidence at the event. Each function returns
two things: the reader's result, computed from the operand the reader
used, and x after the line, which shows that the rebinding happened.
Where a callback rebinds x, the test also records that the callback ran
in the target frame. The ordinary rebinding cases run 100 times, past
specialisation. Trace and monitoring cases run once with their hooks
installed to isolate event delivery; the heap-operand case runs 20 times.
The forced-phase fixture runs three attempts per path, independently of
adaptive counters; its separate receiver regressions run 300 calls.

In `test_peepholer.py`, class `CopyPropagationAcrossCodeTests`:

1. `test_call`: `x + change()`.
2. `test_property` and `test_getattr`: `x + o.attr`, with a property and
   with `__getattr__`. A property runs as `LOAD_ATTR_PROPERTY` once
   specialised.
3. `test_subscript`: `x + o[0]`, where `__getitem__` rebinds x.
4. `test_reflected_binary_op`: `x + (y + o)`, where `o.__radd__` rebinds
   x.
5. `test_comparison`: `x + (o < 0)`, where `__lt__` rebinds x.
6. `test_global_in_dict_subclass` and `test_global_colliding_key`: the
   two `LOAD_GLOBAL` reproducers.
7. `test_under_settrace`: tests 1 and 2 with a tracer installed. The
   tracer records the call event of the rebinding function.
8. `test_under_monitoring`: a CALL callback for `len` in `x + len(s)`,
   which records that it ran and rebinds x, and test 2 with PY_START and
   LINE events enabled. No expression today keeps a pending load across
   a conditional jump to its reader: the reader of a short-circuit or
   conditional expression sits in a merge block, where the pending set is
   reset. Clause 4 is therefore proven by test 13, not by a semantic test.
9. `test_specializer_runs_no_code`: the colliding-key `x.a` probe over
   300 cold-to-specialised calls. It asserts every result and records
   that `__eq__` ran while f ran.
10. `test_folded_operand_outlives_rebinding`: `x + o`, where X.`__add__`
    rebinds x, collects and returns `NotImplemented`, and `O.__radd__`
    records the original operand's tag. It passes today through
    conservative retention, and runs under `PYTHON_GCVERIFY=kill` on the
    debug build. It is a semantic regression, not the missing-root oracle
    for the 10a rung 3 row: "a reader's
    folded operand needs an anchor across the reader's own collecting
    calls".
11. Disassembly through `Instruction.srcs`. The folds that stay:
    `x + 1000`, `x + y`, `x + c` with c a cell, `x == (a, b)` (across
    `BUILD_TUPLE`) and `x + (a is b)`. The folds lost: `x + G` and
    `x + o.a`.

In `test_generated_cases.py`:

12. `test_runs_code`: synthetic families, one per clause, and a quiet
    family. The test checks both the format and the emitted C line.
13. `test_runs_code_covers_every_handler`: over the real `bytecodes.c`,
    every instruction that runs code by its own uops (computed in the
    test, not by `runs_code()`) has the flag in its format. Named
    positive and negative instructions are checked as well.
14. `test_borrowed_input_read_before_code`: an input taken after an
    escaping uop loses its borrow bit, and one taken after a
    `specializing` uop keeps it.

Negative controls. Tests 1 to 8 and the lost-fold half of 11 must fail on
the 41c716c5947 build, and test 9 on the build without the guard. Test 12
must fail with clause 4 dropped, test 13 with clause 4 dropped, and test
14 both with the late-input condition dropped and with specializers
counted. The preserved-fold half of 11 must fail with a kill at every op;
this is a local edit, never committed. The record shows each failing
output. Test 10's control belongs to the 10a row.

Gate 1 runs before every commit on test_compile, test_compiler_codegen,
test_dis, test_sys_settrace, test_sys_setprofile, test_frame,
test_scope, test_peepholer, test_grammar, test_generators,
test_coroutines and test_generated_cases. Then gates 0 to 3 at the head,
and gate 4 with `GATE_LABEL=copyprop`.

## Risks

- Expected disassemblies change where a fold is lost; the record lists
  each one.
- Instruction counts rise on the kernels, because loads come back. They
  are recorded, not recovered (PROCESS.md section 3).
- The guard in `specialize_dict_hint()` stops specialising attributes on
  instances whose dict has non-str keys. They run the generic handler
  instead.

## Hand-offs

- The specializer contract, enforced by a checker: 10a tooling (a 0.3
  row; the coordinator adds it).
- `type_lookup_runs_no_code()` in `Python/specialize.c` repeats the
  static `mro_dicts_unicode()` of `Objects/typeobject.c`, which 10a rung
  2b owns while it is running. Once 10a lands, the typeobject.c function
  is exported and the specialize.c copy goes. Owner: the coordinator, to
  relay to 10a, or to this agent after 10a lands.
- The anchor for a folded operand across the reader's own collecting
  calls: 10a rung 3 (a 0.3 row, with test 10 as its oracle).
- The allocation bound: review 2 §5.3's row (10a rung 3), with the added
  acceptance clause.
- Per-instruction events: the 0.2 row (the coordinator). The library
  documentation is approved and committed (`Doc/library/sys.monitoring.rst`,
  INSTRUCTION; `Doc/reference/datamodel.rst`, `f_trace_opcodes`).
- `INSTRUMENTED_JUMP` hides an escaping call from the analyzer, so
  `HAS_ESCAPES_FLAG` is wrong for the instrumented jump handlers.
  Nothing reads it for those handlers today. Owner: the cases generator,
  at step 8, when tier 0 starts reading the flags.

## Review

Round 1 (codex, `reviews/compiler-copyprop-1-review-1.md`, verdict revise):

| Finding | Disposition |
|---|---|
| B1: the specializer invariant is false, and the reader check misses the second dispatch | Reproduced (4 of 200). The guard goes in `specialize_dict_hint()` with test 9. The contract and its second dispatch are stated above. The type-path audit is recorded. The checker is a 0.3 row. |
| B2: an allocation can run Python code through the sweep's `PyBuffer_Release` | Accepted. The claim is bounded on the §5.3 row, with an added acceptance clause (ruling). Killing at allocating ops was rejected as a stopgap. |
| B3: tests must witness at the event, cover the specialised forms, and cover a heap operand | Tests record the callback, the frame and the local; each runs past specialisation; test 10 is added as the 10a row's oracle. The generator tests check the emitted C and the per-uop properties independently of `runs_code()`. Test 14 checks that a specializer is not counted, and the contract that justifies this is enforced by the guard and the 10a row. |
| Comparison expression; `BUILD_TUPLE` window | Test 5 uses `x + (o < 0)`; test 11 uses `x == (a, b)`. |
| Free-threaded paragraph | Deleted: note 06 §3.4 removed that build. |
| The count script, the source identities, words and slots | The script is in the tree; the record gives the commit and the counts. |
| Documentation of the break | Proposed above; awaiting approval. |
| Flag bits | Stated. |
| Questions: the anchor; landing with a bounded claim; the closing check for the contract | Each goes to a 0.3 row (above). The rung lands with the bounded claim. |

Round 2 (codex, `reviews/compiler-copyprop-1-review-2.md`, verdict revise;
the last round, so the coordinator rules on what stays open):

| Finding | Disposition |
|---|---|
| B1: the specializers' MRO lookups can reach `__eq__` on a type-cache miss; `mro_dicts_unicode()` runs after the lookup | Ruled option (a), on condition that a fixture reaches the path. The fixture changes the type before every call, which forces the miss. Without the check, the probe gave a wrong result in 8, 4, 8, 4 and 1 of 400 calls (class attribute, instance class attribute, subscript `__getitem__`, `__getattr__` hook, `__getattribute__` lookup). Fixed in 4d6fd6e5196: `type_lookup_runs_no_code()` runs before every specializer type lookup, the statistics-only ones included. Test `test_specializer_type_lookup_runs_no_code`; its control (the tree at ecaa09f9267) fails. |
| B2: the reader analysis took stack effects for reads; an unused peek emits no load | Fixed in 9a0281115b1: `inputs_read_after_code()` follows `Stack.pop()`'s `used` rule. Test 14 gains an unused peek that runs code, and checks the emitted order of the call and the load. The control (the previous analysis) fails. The tables do not change. |
| B3: test 10 cannot be the anchor row's oracle | Accepted. Test 10 stays as a semantic regression; its comment names the row and says why it passes. The row's oracle must inject a collection where only the reader's precise frame holds the operand, and must fail with that root omitted. The coordinator writes this into the row. |
| Witness the execution forms; witness the monitoring callbacks | e7cdcac58d0: every rebinding records its frame and the handler executing there. Tests 1, 2 and 3 assert the generic and the specialised handler, and the monitoring test asserts its CALL, PY_START and LINE callbacks. |
| Test 8's barrier was `LOAD_GLOBAL len` | The callee is a cell, so the CALL is the barrier. The control (the kill removed) fails. |
| The measurement split is lexical | Labelled as such in the record. The aggregate counts are exact. |
| An immediate before/after benchmark | Gate 4 ran on the head and on the 41c716c5947 base build (`copyprop.json`, `copyprop-base.json`). |
| Ledger rows and documentation | For the coordinator (PROCESS.md 0.3); the documentation was approved and is committed in fbf27434b0c. |

## As built

The record of rung 1, 2026-10-28. Base: 41c716c5947. Head of
`boil/compiler-copyprop`: the record commit on 1bf1f941c88.

### Commits

| Commit | What |
|---|---|
| 54958dcb119 | design and review round 1 |
| 5ec05e7116b | `_PyIR_RUNS_CODE`, derived per family by the generator; the reader-side borrow condition; generator tests (12 to 14) |
| 9b9bacc24fe | the pass kill; magic 3729; tests 1 to 8, 10 and 11 |
| 9a0281115b1 | the reader-side analysis follows actual reads, an unused peek being no read (round 2, B2) |
| e7cdcac58d0 | the tests witness the frame and the executing handler at each rebinding |
| ecaa09f9267 | specializer contract, part 1: `specialize_dict_hint()` declines dicts whose keys are not all exact str (round 1, B1); test 9 |
| 1f0f58d9f53 | review round 2; notes 06 and 07; `boil/tools/measure/fold_count.py` |
| 4d6fd6e5196 | specializer contract, part 2: `type_lookup_runs_no_code()` runs before every specializer type lookup (round 2, B1); test 9b |
| fbf27434b0c | the library documentation of the per-instruction break |
| 1bf1f941c88 | test_ir's fold test asserts the rule (it pinned the old fold across a global and a call; gate 2 found it) |

### Built against designed

The rule, the derivation and the reader side are as designed. These
changed, each for the reason given:

1. **The specializer contract is enforced, not assumed.** The design took
   it as an invariant that a specializer runs no Python code. Both review
   rounds showed that it did not hold. An immediate reader, which no
   compile-time kill reaches, then read a rebound local when its word was
   dispatched again. Two guards in `Python/specialize.c` (ecaa09f9267,
   4d6fd6e5196) make the contract true for every path found. The
   coordinator ruled on each.
   The MRO-lookup path (round 2, B1) was taken under option (a) and its
   one-hour bound: a fixture reached it within the hour, by bumping the
   type's version before every call so that the specializer's lookup
   misses the type cache and compares a colliding str-subclass key in an
   MRO dict. It gave wrong results on 25 of 2,000 calls over five paths at
   ecaa09f9267, and none after 4d6fd6e5196. The fix is in this rung (test
   9b), so it is not a 0.3 row. Only the checker that would enforce the
   contract is handed to 10a tooling.
2. **The reader-side analysis was wrong as first written.** It took a
   stack effect for a read (9a0281115b1). The tables did not change, so
   no build was affected.
3. **`type_lookup_runs_no_code()` repeats `mro_dicts_unicode()`.** 10a
   rung 2b owns `Objects/typeobject.c`, so the check could not be shared.
   The copy is a recorded hand-off and goes after 10a lands.
4. **The documentation of the break** was added on approval.

### Gate lines

All at 1bf1f941c88, 2026-10-28:

| Gate | Build | Time (PDT) | Result |
|---|---|---|---|
| 0 | build/default | 17:00 to 17:01 | passed |
| 0 | build/debug | 17:01 to 17:07 | passed |
| 2 | build/default | 17:07 to 17:10 | passed, 51,079 tests |
| 3 | build/debug | 17:10 to 17:23 | passed; roots 61,914 collections, 0 fallbacks |
| 4 | build/default | 17:24 to 17:28 | passed (`boil/bench/results/copyprop.json`) |
| 4 | 41c716c5947 build/default | ended 17:02 | passed (`boil/bench/results/copyprop-base.json`) |

An earlier ladder at fbf27434b0c passed gates 0 on both builds and
failed gate 2 on one test, test_ir's fold test, which 1bf1f941c88 fixes.
The 19 copy-propagation tests also passed under `PYTHON_GCVERIFY=kill` on
the debug build at 1bf1f941c88.

The historical record claimed Gate 1 before each code commit, but did
not retain per-commit source/build identities or counts; that claim is
not treated as reconstructed evidence. The takeover reruns the entire
targeted set against the final tests, with source hashes and counts
below. The historical targeted set was: test_compile,
test_compiler_codegen, test_dis, test_sys_settrace, test_sys_setprofile,
test_frame, test_scope, test_peepholer, test_grammar, test_generators,
test_coroutines and test_generated_cases; test_ir from 1bf1f941c88. The specializer commits also ran
test_opcache, test_descr and test_class.

The verification set (kill, ASan and stress) was not run. This rung
changes the compiler's output, the specializer and the format flags, but
not the collector, the roots, the object layout or the handlers' code
(PROCESS.md section 3). The new tests did run under `PYTHON_GCVERIFY=kill`
on the debug build at 9b9bacc24fe, and all passed.

### Negative controls

The following is the historical narrative, not a substitute for retained
source provenance. Its dict-guard revision was invalid: `1f0f58d9f53` is
a child of `ecaa09f9267` and already contains the guard. No removal patch
was retained, so that old dict control is withdrawn. The takeover
controls below supersede both guard controls with exact source hashes,
patches and final-test failures.

- **The pass without the kill** (a local edit, `runs_code = 0`, never
  committed). This is equal to 41c716c5947's pass, and was also run on the
  41c716c5947 build itself before the fix. Tests 1 to 8 fail with
  `Tuples differ: (2, 2) != (1, 2)`, or with `((2, 2), (2, 2)) != ((1, 2),
  (1, 2))` under tracing and monitoring. The two lost-fold tests fail
  with `0 == 0`, because the reader names x's slot.
- **A kill at every op** (a local edit). The four preserved-fold tests
  fail: `3 != 0`, `1 != 0`, `2 != 0`, `3 != 0`. So does test 10's fold
  assertion.
- **Withdrawn dict-hint control provenance** (reported as
  1f0f58d9f53 before ecaa09f9267, which Git disproves). The old account
  reported that test 9 fails: `{'other'}` appears among the
  results. The store probe left 6 of 300 receivers unstored and
  overwrote `other.a`.
- **Without the MRO guards** (the tree at ecaa09f9267, built). Test 9b
  fails. The probe, per path out of 400 calls, gave wrong results:
  class attribute 8, instance class attribute 4, subscript 8,
  `__getattr__` 4, `__getattribute__` 1.
- **Generator controls.**
  - Dropping the instrumented clause fails tests 12 and 13
    (`INSTRUMENTED_POP_ITER`).
  - Dropping the late-read condition fails test 14 (`3 != 2`).
  - Counting specializers fails test 14 (`2 != 3`).
  - The previous analysis, which takes a stack effect for a read, fails
    the peek case of test 14 (`2 != 0`).

### Measurements

**Folds.** `fold_count.py` over the Lib of 5ec05e7116b, extracted with
`git archive` to a fixed directory. The before count is on 5ec05e7116b's
build, whose code is that of 41c716c5947 (the flag only); the after count
is on 9b9bacc24fe's build:

| | before | after | change |
|---|---|---|---|
| `LOAD_FAST_BORROW` words | 223,631 | 233,133 | +9,502 (+4.25%) |
| all words | 4,595,456 | 4,604,958 | +9,502 (+0.21%) |
| temporaries (sum of `co_stacksize`) | 477,857 | 477,995 | +138 (+0.03%) |

The newly unfolded loads, by the first op after the load that has the
flag. This is a lexical diagnostic over the final words. It is not the
pass's own record of what it rejected.

| first op that runs code | change |
|---|---|
| LOAD_GLOBAL | +4,421 |
| LOAD_ATTR | +2,775 |
| BINARY_OP | +1,575 |
| BUILD_SLICE | +224 |
| BUILD_LIST | +186 |
| CALL | +78 |
| UNARY_NEGATIVE | +65 |
| BUILD_SET, FORMAT_SIMPLE, BUILD_MAP, BINARY_SLICE, others | +188 together |

**Instructions (gate 4).** 

`compare.py copyprop-base.json copyprop.json 6d.json`.
The paired base is authoritative for these deltas. Most 6d rows are
within 0.4% of it, but `getattr_property_float` differs by +1.0452% and
`attr_load_poly5` by +0.7135%; comparisons against 6d are not identical
to comparisons against the base.

| group | geomean, head / base |
|---|---|
| kernels | 1.005 (29/29) |
| micro | 1.001 (65/65) |
| memory | 1.000 (6/6) |

The kernel deltas, with the kernel's own newly unfolded loads
(`LOAD_FAST_BORROW` words before and after, counted per file and
classified by the first op after the load that has the flag; scratch
script, not in the tree). A kernel that spends its time in Lib (regex_compile,
deepcopy) is attributed to the Lib count above.

| kernel | instructions | loads before -> after | first stopping op |
|---|---|---|---|
| nbody | +4.0% | 16 -> 33 | BINARY_OP +17 |
| fannkuch | +1.9% | 7 -> 13 | BINARY_OP +5, BUILD_SLICE +1 |
| go | +1.2% | 50 -> 81 | LOAD_ATTR +16, LOAD_GLOBAL +15 |
| chaos | +1.2% | 39 -> 60 | LOAD_ATTR +12, BINARY_OP +9 |
| float | +1.0% | 7 -> 10 | LOAD_ATTR +3 |
| spectral_norm | +0.9% | 18 -> 22 | LOAD_GLOBAL +2, BINARY_OP +2 |
| nqueens | +0.8% | 19 -> 26 | BINARY_OP +5, UNARY_NEGATIVE +2 |
| regex_compile | +0.8% | 22 -> 25 | LOAD_GLOBAL +3; the rest is in `re` |
| deepcopy | +0.6044% | Lib workload | unresolved; the static fold census covers `copy` but is not a causal attribution |
| comprehensions | +0.3081% | not recorded per file | unresolved |
| pickle | -0.3843% | not recorded per file | unresolved |
| deepcopy_memo | +0.8% | 16 -> 17 | LOAD_ATTR +1; the rest is in `copy` |
| xml_etree_generate | +0.4% | 71 -> 75 | LOAD_ATTR +2, LOAD_GLOBAL +2 |
| raytrace | +0.3% | 49 -> 64 | LOAD_ATTR +7, LOAD_GLOBAL +4, BINARY_OP +4 |

The remaining kernels are within 0.3%. nbody's hot loop is arithmetic on locals,
`dx * dx + dy * dy + dz * dz`. BINARY_OP's family shares one format, and
its generic member may run Python code (an `__add__`), so each operand
pending across a BINARY_OP is now its own load.

Micro: class_attr_store +2.8%. `K.v = i` now loads `i` before the
LOAD_GLOBAL of `K` (one more word per iteration, the `x + G` lost fold).
getattr_property +0.7777% and getattr_property_float +1.0457%: the words
of `run` and of the getter are the same on both builds. The float twin
already differs by +1.0452% between 6d and the paired base, supporting
pre-existing variation in that row. It does not establish the cause of
the separate integer-property increase; that attribution remains open.
All other micro rows are within 0.2%.

RSS: the kernel geomean is 0.993. coroutines' 0.797 is a single row in
peak RSS with its instruction count unchanged; it is not attributed. On
memory, the head equals 6d on every row (startup +2.1%, tuples_10m +5.0%,
instances_1m +8.7% against the base run). That agreement suggests a
base-run difference but does not identify its cause; those RSS deltas
and coroutines remain unresolved.

Not tuned. Reintroduced loads explain the rule's structural cost; the
individual rows above are causal attributions only where evidence is
given. Unexplained instruction and RSS changes retain an owner in the
landing-review response below.

### Hazards found, and how each was traced

- **A generic `LOAD_GLOBAL` runs Python code.** Two reproducers, a dict
  subclass's `__getitem__` and a colliding key's `__eq__`, at 12 against
  11. This overturned the brief's "globals stay".
- **A specializer that runs Python code, followed by a second dispatch
  of the same word.** Found by review round 1, reproduced (4 of 200),
  and fixed in two parts. Round 2 found the MRO path, which a fixture
  reached by changing the type before every call to force the miss. Two
  earlier probes had not reached it: a type-cache eviction by pressure,
  and plain instance and hook loads.
- **The suite pinned the defect.** test_ir's
  `test_load_folds_under_a_global_and_a_call` asserted the fold across a
  global and a call as the expected behaviour, so the suite was green on
  the wrong semantics. This is review 2's pattern of checkers and tests
  that agree on the same mistake. Gate 2 found it; the renamed test
  (1bf1f941c88) asserts the rule both ways.
- **The allocation bound.** A sweep's `PyBuffer_Release()` can reach
  `__release_buffer__` (review 2 §5.3). This was not traced further here;
  the claim is bounded on that row.
- **Gate tooling.** Killing the shell loop that ran gate.sh, then gate.sh
  itself with TERM, left gate_run.sh's `timeout` process group and its
  test workers running, until the group was signalled directly
  (`kill -TERM -- -PGID`). This is reported, not fixed; gate.sh is the
  coordinator's.

### Rereading the tests: what each would not detect

| Test | Would not detect |
|---|---|
| 1 call | a lost heap operand (the saved value, 1, is inline); a defect in a call family member other than CALL and CALL_PY_EXACT_ARGS |
| 2 property, getattr | a defect only in a C getset or a member descriptor |
| 3 subscript | a defect only in a C mapping slot (it uses a Python `__getitem__`) |
| 4 reflected | an operand lost across a collecting `NotImplemented` chain (test 10 covers one shape) |
| 5 comparison | cleanup when the comparison raises |
| 6 globals | other mapping protocols of globals, such as `__missing__` |
| 7 settrace | a missing trace event other than the ones asserted (the call of `change`, the line of `attr`) |
| 8 monitoring | events other than CALL, PY_START and LINE; the instrumented-twin clause, which no expression reaches today (test 13 covers it) |
| 9 dict hint | specializer paths other than LOAD_ATTR and STORE_ATTR on an instance dict; the receiver assertion alone does not distinguish phases (the forced-phase test supplies that proof) |
| 9b MRO | class-construction, native `__setitem__` and statistics-only failure lookup paths; a miss produced other than by a version change |
| forced specialization phase | an incorrectly selected runtime adaptive threshold; automatic redispatch; class-construction and statistics-only lookup paths; checker enforcement of other native callees |
| 10 heap operand | a missing reader anchor, because self and opaque C frames keep the operand alive; that is the 10a row's oracle |
| 11 disassembly | runtime operand selection; folds in other shapes |
| 12 synthetic | a native callee misclassified in `call_effects.py`, a return or yield member, a macro that hides a call |
| 13 real handlers | an omission shared with `Properties.escapes` (any macro that hides an escaping call, as `INSTRUMENTED_JUMP` did) |
| test_ir fold | a fold lost under other non-running ops (tests 11 cover four shapes) |
| 14 reader order | a second dispatch after a specializer that runs code (covered by the contract and tests 9 and 9b); operand lifetime |

The per-instruction break has no test. A test that pinned the documented
behaviour would assert the break itself, and this rung leaves that to the
0.2 row's owner.

### Hand-offs

| Item | To | Closes when |
|---|---|---|
| A checker for the specializer contract: classify the callees of `_Py_Specialize_*` | 10a tooling (0.3 row, added) | a checker fixture with a specializer callee that reenters fails |
| An anchor for a reader's folded operand across the reader's own collecting calls | 10a rung 3 (0.3 row, added) | a collection injected where only the reader's precise frame holds the operand fails with that root omitted and passes with it; test 10 stays a semantic regression |
| The allocation bound: no Python code under an op that only allocates | review 2 §5.3's row (10a rung 3; clause added) | that row's acceptance test |
| The per-instruction compatibility break | coordinator (the 0.2 row) | the row; the documentation is committed (fbf27434b0c) |
| `type_lookup_runs_no_code()` repeats `mro_dicts_unicode()` | coordinator or this agent, after 10a lands | typeobject.c exports the function and the copy in specialize.c goes |
| `INSTRUMENTED_JUMP` hides an escaping call, so the instrumented jumps' `HAS_ESCAPES_FLAG` is wrong | the cases generator, at step 8 | tier 0 reads correct flags |
| Gate tooling: a killed gate.sh can leave gate_run.sh's process group running | coordinator | gate.sh's cleanup kills the group on every signal path |


## Landing review response, 2026-10-29 takeover

The review of `bf6635fd0a4` returned **fix first**. The three findings
are addressed together; no compiler or specializer semantic change is
made by this fixup. Integration `7af7ce89618` was merged at `ce7a0a9856b`
before edits. Sources remained quiescent during each build, test and
measurement. All controls below are local source variants, never commits.

### 1. A phase-specific oracle

`_testinternalcapi.specialize_current_instruction(offset, owner, operand)`
validates a word of the calling Python frame and directly invokes its
LOAD_ATTR, STORE_ATTR or BINARY_OP specializer. It never performs the
generic lookup. The test marks only that C call as `specialize`, then
marks the subsequent expression `execute`. At each colliding-key
comparison it records the phase and the caller's actual word. The
fixture forces an attempt without relying on adaptive thresholds; a
class-version change forces a cache miss before each of three attempts.
The generic-phase comparison is a separate positive witness.

The existing receiver-selection regressions remain. The semantic MRO
test now also covers `__getattribute__`, the fifth path claimed by the
old probe. The phase oracle adds dict load/store, class/instance load,
`__getattr__`, `__getattribute__`, `__getitem__`, and MRO store. Each case
uses its own executing code/feedback copy.

### 2. Reconstructed controls and provenance

Retained evidence is in `controls/compiler-copyprop-1/`:

- `dict.patch` removes exactly the instance-dict key-kind guard. The
  final phase test fails twice: dict load reports
  `[('specialize', 8), ('execute', 11)]`; store reports
  `[('specialize', 8), ('execute', 12)]`. Both fail the invariant
  `['specialize'] != []`, independently of receiver-result assertions.
- `mro.patch` bypasses only the MRO guard helper. The other six cases
  each fail that same invariant with `[('specialize', 8)]`: class load,
  instance load, getattr, getattribute, getitem, MRO store.
- `dict-failure.log` and `mro-failure.log` retain the actual assertions,
  subtest names and event words. Word 8 is the fixture CALL; the generic
  attribute/store words are 11/12. The command is gate 1 on
  `test_peepholer -m '*test_specialization_phase_runs_no_code' -W`,
  `GATE_JOBS=1`, on `build/default`. Each variant is rebuilt under that
  build's gate lock before testing. Regrtest `-q` suppresses `-v`; two
  attempts with unsupported `--output-on-failure` and the earlier
  summary-only failures are not the retained assertion evidence.
- `sources.json` records full base commit, final test/fixture/generator/
  metadata SHA256, each production-source hash, each exact patch hash,
  and the Py_STATS configuration. Its `restore` record proves the
  guarded files were restored after both controls and the measurement.
  The final fixup contains those same test and fixture contents.

The historical dict-control revision was invalid and is withdrawn above.
The old per-commit Gate 1 claim cannot be recovered from its narrative;
fresh targeted runs below replace that claim rather than invent counts.

These controls do not prove constructor `__init__`, STORE_SUBSCR
`__setitem__`, statistics-only failure lookups, every module/global
guard, or arbitrary callees' effects. The checker enforcement remains
10a tooling's explicit hand-off. The allocation-time buffer-release
exception and folded-reader missing-root oracle remain 10a rung 3 work;
the pass comment now names the release exception.

### 3. Measurements and disposition of uncertainty

The original instruction/RSS files remain measurements of their named
builds, not new timings of this fixup. The table above now includes
deepcopy +0.6044%, comprehensions +0.3081%, pickle -0.3843%, and both
6d/base exceptions. The integer-property increase is not explained by
the float-property variation. Coroutines RSS and the memory/base RSS
differences remain unattributed.

New allocation and box evidence:
`results/copyprop-review-head-pystats.json` and
`results/copyprop-review-before-equivalent-pystats.json` (under
`boil/bench/`). Both use the same repository-managed GCC release
`build/pystats`, configured `--enable-pystats CC=gcc`, and the unchanged
benchmark runner, hash seed 0, its standard loop counts and differenced
counter protocol. The before-equivalent source variant replaces exactly
`Python/regcfg_passes.c` and `Python/specialize.c` with their
`41c716c5947` contents. It is **not a pristine baseline checkout**:
the final metadata flags remain present but its old pass does not read
them; the test-only fixture and final tests remain present. The exact
variant is `baseline.patch` plus its `sources.json` hashes. Each result
file identifies this controlled comparison explicitly; its ordinary
Git/version metadata alone does not identify the source variant.

| Counter | Head versus before-equivalent, 94 rows |
|---|---|
| Total boxes | exactly equal in 92; logging_format 20 -> 20.044 per loop; logging_simple 20.037 -> 20 |
| Allocations | exactly equal in 92; logging_format 330.001 -> 330.133 per loop; logging_simple 310.112 -> 310.001 |
| Dispatches | 22 rows change; class_attr_store gains exactly 1 per iteration; nbody gains 1,700,040 per loop |

`controls/compiler-copyprop-1/counter-comparison.json` retains every
changed row and exact values. The dispatch increases agree with
reintroduced loads. The small logging counter differences remain
unattributed, without a claim that they are caused by this patch.

Coordinator disposition: correctness landing may proceed with these
bounded attribution gaps. The coordinator owns their investigation at
**checkpoint I**, through handler/runtime work H/7, including deepcopy,
comprehensions, pickle, the integer-property row, both logging counter
rows, coroutines and memory RSS. They are not waived by a future JIT.
The 94-row box metric covers 29 kernels and 65 microbenchmarks. The
runner excludes memory from that metric; the coordinator explicitly
keeps memory allocation/pause evidence at checkpoint I, without an
unreviewed runner extension or a claim that it was measured here.

### Gate survival and final affected checks

The full-suite lines at `1bf1f941c88` remain historical lines at that
revision. `12f407b23a9` merged integration `b57cb04c024`; subsequent
`ce7a0a9856b` merged `7af7ce89618`. Neither changed production compiler,
specializer, handler bodies or handler metadata relative to `1bf1f941c88`.
The takeover changes only a test C fixture, tests, a comment and this
record/evidence. Under PROCESS section 3's rerun rule, the coordinator
ruled that affected targeted modules on both builds, asserted debug-kill
regressions, regeneration and unbuilt checks are required; the merged
integration ladder follows at landing. No old full suite is labelled a
current-head run. This does not close 10a's outstanding root proofs or
extend the historical full-suite verification modes.

Initial affected checks at `ce7a0a9856b` plus the test/fixture/comment
hashes in `sources.json`, before controls: gate 0 default passed 1,924
tests (50 skipped), gate 0 debug passed 1,924 (20 skipped), and gate 1
default passed 1,783 tests (13 skipped) over 16 targeted modules. The
targeted set is test_compile, test_compiler_codegen, test_dis,
test_sys_settrace, test_sys_setprofile, test_frame, test_scope,
test_peepholer, test_grammar, test_generators, test_coroutines,
test_generated_cases, test_ir, test_opcache, test_descr and test_class.
Final guarded checks and the shared verification-state prerequisite
merge are recorded in the following fixup entry.
