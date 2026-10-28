# compiler-copyprop rung 1: a pending load does not survive an op that runs code

Status: design, revised after review round 1, 2026-10-28. Branch
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
- The audit of the other specializers found that their lookups of an
  exact-str name in a type's MRO go through the type cache. On a miss,
  `find_name_in_mro()` compares keys, and a class namespace given to
  `type()` or a metaclass may hold a colliding str-subclass key (CPython
  warns and allows it). Two probes did not reach that path from a
  specializer: an instance attribute under `__getattribute__`,
  `__getattr__` and the attribute's own name, 300 calls each; and a class
  attribute after evicting the type cache, 60 calls. The path is reported
  to the coordinator and not changed here.
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
in the target frame. Every test runs 100 times, past specialisation.

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
    debug build. It is the oracle for the 10a rung 3 row: "a reader's
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
- The type-cache-miss path of the specializers' MRO lookups (see the
  contract above): reported to the coordinator, owner to be ruled.
- The anchor for a folded operand across the reader's own collecting
  calls: 10a rung 3 (a 0.3 row, with test 10 as its oracle).
- The allocation bound: review 2 §5.3's row (10a rung 3), with the added
  acceptance clause.
- Per-instruction events: the 0.2 row (coordinator, at landing) and the
  library documentation (this rung, if approved).
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
