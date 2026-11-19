# Handlers81: upstream-shaped handlers in one computed-goto evaluator

Status: **approved for rollout**, 2026-11-19. The user accepted the five
decisions in section 8 as recommended and asked to land H0–H5 even if the
first performance gain is modest; there is no performance gate. Work runs on
`pyjit/handlers81` (worktree `BASE/worktrees/handlers81`, from checkpoint78
`99a04733128`), builds `BASE/handlers81-{release,debug}`, evidence
`E59/handlers81/`. It applies the user's rule that behavior upstream
could change between versions (finalization timing, finalizer-visible order
within an instruction, opcode-event granularity) is free to change. Source
references are to checkpoint78 (`99a04733128`) and upstream `3a440b6`.
Evidence is in `E59/handlers81-design/`; paths below are relative to it:

- `notes-q234.md`, `notes-q5.md`: two read-only source surveys, with
  file:line quotes, commit hashes and assembly walks;
- `probe/`: the feasibility probe (sources, scripts, disassembly, results);
- `progress.md`: the run log.

"Measured" means an execution79 ledger (`E59/execution79-call-ledger/raw/`)
or a perf count. "Static" means the shortest path from a handler's entry to
its dispatch jump in compiled code (`probe/fastpath.py`; section 4). All
other numbers are estimates and are marked as such.

## Summary

1. **Handler shape.** Every handler follows one order. Guards come first,
   then the effect. Then it detaches dying inputs, stores its results and
   closes what it released. The periodic check comes last, then dispatch. A
   write stores the new value and then releases the slot's old one, as
   upstream's `STORE_FAST` does. Nothing runs after the first close except
   other closes, the periodic check and dispatch. This deletes `phase_state`
   stores, `_PyValue_Publish` at stores, the destination-kind decode, the
   displaced-local deferral and the re-entrant release protocol.
2. **Backend.** One computed-goto function, as upstream builds with GCC. Cold
   work is in out-of-line helpers that return values or `{frame, next}`
   pairs; no helper takes the address of a handler local. The probe compiles
   336 handler labels into one function with a 40-byte frame (plus six
   pushes), no canary, one indirect jump per dispatch site and no stack
   references on 12 of 14 hot fast paths. The same bodies compiled as GCC
   tail calls run 3–11 instructions longer per handler.
3. **Order.** Value and ownership rules first, in the tail-call build (H1–H3).
   Then the backend switch (H4). Then calls80 in the new shape (H5).

---

## 1. The user's five questions

### Q1. "Why isn't this encoded in the IR?"

It is. Bits 9–12 of the word's handler halfword are `IR_F_LOCAL_A..D`
(`Include/internal/pycore_ir.h:155-159`). `_PyIR_ExecLocalFlags` derives them
in the executing copy. The cost is not in finding the kind. It is in what one
handler body does with it on every execution:

- `IR_CONSUME_INPUT` (`Python/ceval_macros.h:573`) tests the flag. For a
  temporary it takes the value at read time and writes NULL into the slot.
  There are 271 sites in 158 handlers (`notes-q234.md` E).
- `IR_STORE_DST_IMPL` (`ceval_macros.h:651`) tests the flag again for the
  destination, then compares it with `IR_NO_DST`. For a local it calls
  `IR_RETIRE_LOCAL`, which may defer the old value into `ir_displaced`.
- 25 handlers decide the destination twice: `IR_STORE_TEMP_DST` before the
  release, `IR_STORE_DST` after it.

In `_TAIL_CALL_BINARY_OP_ADD_INT` the kind tests take about 8–12
instructions, and the displaced-local logic another 13–15, out of about 55–65
(hand count, `notes-q234.md` E). Execution79 cause 2 measures the same thing
in the attribute handlers (69–83 instructions against upstream's 39).

**The design.** The kind costs nothing at run time where the work is not
needed, and one bit test where it is:

- **Destinations: no kind test.** A destination that may be a local gets the
  releasing write: load the old value, store the new one, close the old one.
  The same code serves a local, a temporary and the discard slot. A format
  whose results are always temporaries, such as the method pair of
  `LOAD_ATTR_METHOD_*`, stores plainly. That is a per-format fact, decided by
  the generator, so it costs nothing.
- **Discards: a discard slot.** `IR_NO_DST` becomes one reserved slot per
  frame. A discarded result stays there until the next discard writes over
  it, or the frame dies. This removes the `IR_NO_DST` compare and the
  `IR_MOVE_TO_DST_OR_DISCARD` arm (32 handlers). It also lets `RETURN_VALUE`
  write its result through `return_slot` with no discard case.
- **Inputs: one bit per input that can own a count.** `IR_F_DIES_X` replaces
  `IR_F_LOCAL_X` for inputs. It means "X is a temporary that this format
  consumes". Today every temporary read is a consume, except formats that
  keep an input, such as `FOR_ITER`'s iterator, so the bit follows from the
  slot class and the format's consume role. It is set neither for constant
  slots (calls80 step 3) nor for locals. A handler whose specialized inputs
  are inline values (`BINARY_OP_ADD_INT`, `COMPARE_OP_JUMP_INT`) emits no
  input code at all. Its dying temporaries keep stale inline bits, which own
  nothing. Every other consumer detaches each dying input after the effect:
  load its bits, store NULL. It closes the value after storing its results.
  That costs two instructions when the input is a local. Upstream's
  `PyStackRef_CLOSE` pays the same tag test.

The invariant is the one in the frame header comment
(`pycore_interpframe_structs.h:33-36`): every slot holds a valid value or
NULL. To it this design adds that a dead temporary owns nothing.

**Price of the release-old test.** On a vacant slot it costs three
instructions (load, test, branch; the store is needed anyway). When the old
value owns a count it costs five more (mask test, decrement, branch).
`_Py_Dealloc` is called out of line on zero. Today's destination path costs
two flag tests, plus 13–15 instructions of displaced-local logic for a local.
The probe's `LOAD_FAST_BORROW` (§4) is 24 instructions with the release-old
test included, against 37 today.

**Extra clears.** None in handlers. The consumer's detach is the clear. The
compiler adds only the discard slot.

**Incidental behaviour that changes.** A discarded result lives until the
next discard or the end of the frame. Today it is released at once.

### Q2. `phase_state` and "canonical publication before any release"

**What `phase_state` is.** It is a `uint16_t` at byte 110 of the frame
header (`pycore_interpframe_structs.h:76`). Its low byte is a phase id; its
bits mark PUBLISHED, shadow status, CAPTURED, CREATED, YIELD_DELEGATED and
ERROR_SCOPE (`pycore_ir.h:652-669`).

- **Writers.** `IREmitter.emit_save` (`Tools/cases_generator/ir_generator.py:3586`)
  emits `frame->phase_state = 0;` before every escaping call. That is 1,635
  static stores in 266 of 299 handlers. 27 generic `BINARY_OP` handlers store
  `PUBLISHED | BINARY_PROVIDER`. Cold runtime code sets the lifecycle bits
  (CREATED, ERROR_SCOPE and so on).
- **Readers.** The cyclic GC frame walk (`Python/gc.c:483-520`): PUBLISHED
  runs `_PyIR_ShadowFramePhase` (`Python/ir.c:1097`), a check of the slots
  against the static plan. CREATED and ERROR_SCOPE select a precise view of
  readable slot ranges; every other state scans all slots under NULL control.
  Frame copy and escape (`_PyFrame_GetHomeView`, `Python/frame.c:93-143`)
  and the delegated-yield receiver (`_PyIR_DelegatedReceiverHome`) also read
  it. `f_locals` and tracebacks do not; the evaluator only asserts on it.
- **Is it upstream's `_PyFrame_SetStackPointer`?** By placement, yes: the
  same save-point mechanism emits both (upstream has 332 calls in 232
  handlers). By meaning, no. Upstream's GC needs the stack pointer to bound
  the live slots. A register frame has no stack pointer and every slot is
  valid or NULL, so `phase_state == 0` already gives a safe scan. The stores
  invalidate a publication; they do not make the frame safe.

**Alternative.** Delete every handler store and `emit_save`'s phase
emission. Delete PUBLISHED, `BINARY_PROVIDER`, the shadow check and the
`_PyIR_PHASE_*` plan tables that feed it. The GC always scans all slots. The
lifecycle bits that cold code sets (CREATED, CAPTURED, YIELD_DELEGATED,
ERROR_SCOPE) stay in the field for now; step H1b audits whether "all slots
valid or NULL" lets their readers drop the precise views too. No handler
writes the field. A debug build checks that every slot is a canonical value
or NULL during the GC visit instead.

**`_PyValue_Publish`.** It is defined at `pycore_value.h:164-175`. Inline
forms: an exact int that fits 51 bits is stored inline, and so is an exact
non-NaN float. Anything else is a pointer. A *non-canonical* value is a
pointer to a box of a value that has an inline form. `_PyValue_Publish`
turns such a box into its inline form (`_PyValue_Canonicalize`,
`Python/pyvalue.c:42`). It runs at 250 store sites in 165 handlers, through
`IR_STORE_DST`, `IR_STORE_TEMP`, `IR_PUBLISH_TEMP`, `IR_STORE_TEMP_DST` and
`IR_UNPACK_TO_LOCALS`. A box in a home breaks `is`, which is a bit compare
(commit `308a86c3914`). It also makes inline guards deopt forever.

"Canonical publication before any release" means two things today:

- the result is canonicalized;
- it is written to its temporary home before the receiver is released, so a
  finalizer run by the release sees it rooted (`retire_published_values`;
  `test_ir.py:130`).

**Alternative.** Make canonical form an invariant of storage, enforced where
values are made:

- the object-to-value constructors `PyValue_FromObject{Steal,New,Borrow}`
  already canonicalize (the first probe's method handler shows their two
  type compares, `probe/probe_eval.s`);
- `_PyValue_PointerSteal` has five callers (`Objects/longobject.c:405` is
  one); H1a checks that none can produce a box;
- boxes made by `_PyValueTemps_Box` or `PyValue_AsObject` go to object-form C
  code. They never reach a home, except through a constructor that
  canonicalizes again.

Handlers then store plainly, and the debug store check
(`_PyValue_CheckHome`) stays as the proof. This deletes the five macro
families' `_PyValue_Publish`, the generator's `canonical_result_source`
proofs (`ir_generator.py:6944`) and the `IR_STORE_CANONICAL_*` twins. This is
parity71's option 2. The rooting half of the phrase becomes the shape rule
"store results before any close" (§2, rule 6). It needs no separate step.

### Q3. The re-entrant release protocol

The hazard is real. Releasing the last owner of an object that is not a leaf
runs Python: `__del__`, weakref callbacks, owned destructors. That Python can
read or write `f_locals`, call `gc.collect()` or turn on tracing. Each piece
of the protocol guards one part of this.

| Piece | Where | What it guards | Test |
| --- | --- | --- | --- |
| `_PyValue_CloseMayReenter` | `pycore_value.h:126`, 23 generated sites in 18 handlers | chooses between an inline close (leaf object or not the last owner) and the protocol | |
| `retire_published_values` | `ceval.c:2044`, 18 handlers (attribute loads and stores) | publishes the result before the receiver's release; then reloads `this_instr`, `ir_word`, `slots` | `test_ir.py:130` `test_specialized_attribute_result_survives_receiver_retirement` |
| `ir_retire_last_owner`, `IR_RETIRE_LOCAL`, `ir_displaced` | `ceval_macros.h:620-645`, 136 handlers store to a local | new value in the local before the old one is released; at most one deferred release | `test_ir.py:2055` `test_release_runs_with_the_frame_consistent`; `:2040` released once |
| `finish_displaced_local` | `ceval.c:2051`, 153 calls in 152 handlers | releases the deferred local, then `value_operation_refresh` checks `current_frame` and `ir_words` and restores `instr_ptr`, raising SystemError on failure | |
| `_PyTransitionRoots` | `pycore_transition_roots.h`; pushed in `clear_thread_frame_slow` (`ceval.c:3300`), the error scope and `GEN_*_EFFECT` | keeps a retiring frame's remaining owners and its result visible to the tracing collector and verifier while their releases run | `test_ir_general_return.py:66` reads `get_transition_root_ids()` |

What each guard needs under the policy:

- The **order** "new value in, then old value out" is needed: `f_locals` must
  show a real value. A plain releasing write gives it, as upstream's
  `STORE_FAST` does. It needs no deferral, because nothing follows the close.
- The **reloads and the refresh** guard against a finalizer changing the
  current frame or the executing words. The first cannot happen: a finalizer
  runs in a nested frame and returns. The second cannot happen either: the
  executing copy is fixed for the code object's life (`register-ir.md` 1.10,
  quoted at `ceval.c:1340`), and instrumentation rewrites handler ids in
  place, which dispatch reads fresh. The checks only assert today. If the
  handler touches no slot after its first close, there is nothing to reload.
- The **transition roots** serve the tracing collector and its verifier.
  `register-call-model55.md:357` says so: "GIL cyclic RC already treats
  counted C references as external." A result held in a C local is safe for
  reference counting plus cyclic GC.

**The simplest correct form** is upstream's: store the result, then close,
and call `_Py_Dealloc` on zero. In the register form:

1. detach the dying inputs (bits to C locals, slots NULL);
2. store the results (releasing writes);
3. close the detached inputs and the displaced values, inline, in any order;
4. run the periodic check; dispatch.

Between steps 1 and 3 no Python runs. From step 3 on, the frame is complete.
Frame retirement follows upstream's `_PyFrame_ClearLocals`
(`upstream Python/frame.c:100-111`): unlink the frame, hold the result in C,
then NULL and close each slot in turn.

**Deleted:** `_PyValue_CloseMayReenter` and `_PyObject_DestroyMayReenter` in
handlers, `retire_published_values`, `ir_retire_last_owner`,
`IR_RETIRE_LOCAL`, `ir_displaced`, `finish_displaced_local`,
`value_operation_refresh` on handler paths, `emit_ordered_retirement`
(`ir_generator.py:3857`), `release_displaced`, `IR_COMMIT_OBSERVED_DST`,
`IR_MOVE_OBSERVED_DST`, and `_PyTransitionRoots` with its tracing-root walk
(`Python/gc_trace.c:78-150`) and `get_transition_root_ids`. The cold
`clear_thread_frame_slow` and the "slow retirement" path become the plain
clear (execution79: 268 instructions against upstream's 87 for a callee
whose local dies).

**Incidental behaviour that changes:**

- a finalizer run by releasing an input may see the new local binding of the
  same instruction;
- a finalizer that somehow changed the executing words would hit an assert,
  not a SystemError;
- the tracing verifier no longer sees C-held owners. It is optional
  (`register-call-model55.md:357`), and `gc_verify` must accept that, or go.

### Q4. "Exact finalizer ordering for fused instructions"

Python does not require it. The user ruled on 2026-11-19 that finalization
timing and the order in which finalizers see locals within a statement are
free (also `lifetimes-and-language-semantics.md:1-12`). The order survives in
code and in eight tests (`notes-q234.md` D):

- `test_ir.py:1712`: inputs retire before the local store;
- `test_ir_discard_completion.py:31`: CALL+POP_TOP order;
- `test_ir_unpack_transfer.py:73`: unpack releases each old local before the
  next store;
- `test_ir.py:2262`: `FOR_ITER_GEN` recipient before the displacement
  callback;
- `test_ir_home_writers.py:77`: container store before the displaced local;
- `ceval.c:1849-1859`: call with a local destination;
- `test_ir.py:2055`, `test_ir.py:130`: store before release.

**Minimum ordering that keeps the frame consistent** whenever Python runs:

- **O1.** `tstate->current_frame == frame`, and `frame->instr_ptr` is the
  executing word. `ENTER` stores it, as upstream does.
- **O2.** Every slot holds a valid canonical value or NULL. A value leaves its
  slot (NULL stored) before it is closed.
- **O3.** Each owner is closed exactly once.
- **O4.** All of a handler's frame writes precede its first close.

O4 replaces the reloads. O2 plus "store before close" keeps `f_locals` real.

**What deleting the rest removes:** "inputs retire before the local store"
and its call and unpack variants; the one-deferred-release rule; the
per-element release order of unpack-to-locals, which becomes "store all, then
close all old values". The generator loses `emit_ordered_retirement`, the
`value_operation_close3` ordering and `finish_unpack_to_locals`'s
interleaving. Tests 1712, discard-completion, unpack-transfer, 2262 and
home-writers change from exact event lists to consistency checks: each
finalizer sees a valid frame, real `f_locals` and each value released once.

### Q5. Native bridge records and stack canaries

**For the hot path the user is right.** The census has 54 handlers with
`__stack_chk_fail` (`E59/handlers80/c78-per-handler.txt`, recounted from
`ceval.o` in `notes-q5.md`). 43 of them keep a value record, not 37: the
brief's list plus `CALL_INTRINSIC_1/2`, `CALL_METHOD_DESCRIPTOR_NOARGS/_O`,
`LOAD_SPECIAL`, `RAISE_VARARGS`, instrumented twins and generated variants. The record is generated at
`ir_generator.py:7321-7326`:

```c
PyObject *value_boxes[N]; _PyValueTemps value_owners = {value_boxes, 0, N};
value_temps = &value_owners;
```

Its address goes to `_PyValueTemps_Box`, `finish_value_bridges` and the
error path, so the array and struct live in the handler's frame. That adds a
canary and about 12–14 instructions per execution (estimate from two
handlers, `notes-q5.md`). The box is made only if an input is inline and the
callee takes `PyObject *`.

**What they call, and whether value form deletes the record:**

| Group | Handlers | Callee and fix | Record deleted? |
| --- | --- | --- | --- |
| Input is always an object on compiled code | `POP_EXCEPT`, `PUSH_EXC_INFO`, `RERAISE`, `END_ASYNC_FOR`, `CLEANUP_THROW`, `WITH_EXCEPT_START`, `IMPORT_NAME`, `MATCH_KEYS`, `GET_ANEXT`, `BUILD_TEMPLATE`, `SEND` (4 forms), `MAKE_FUNCTION`, `SET_FUNCTION_ATTRIBUTE`, `LOAD_FROM_DICT_OR_*`, `CALL_INTRINSIC_2`, and others (21) | `assert(PyValue_IsObject(x))` in the recognized form (`analyzer.py:1131`); callees unchanged | yes |
| Inline input is a user error | `GET_AITER`, `GET_AWAITABLE`, `DICT_UPDATE`, `DICT_MERGE`, `LIST_EXTEND`, `SET_UPDATE`, generic `UNPACK_SEQUENCE`/`UNPACK_EX`, `RAISE_VARARGS` (9) | inline pre-check raises the existing TypeError from the tag; `PyDict_Update`, `_PyList_Extend`, `_PySet_Update`, `_PyEval_UnpackIterableValue` unchanged | yes |
| Generator cannot see the guard | `IMPORT_FROM` | already calls `_PyEval_ImportFromValue` | yes |
| Small value form | `CHECK_EXC_MATCH`, `CHECK_EG_MATCH` (`_PyEval_CheckExcept*TypeValid`), `MATCH_CLASS` (`type` as a value), `LOAD_SUPER_ATTR` and twin (`super_call`'s `global_super`), `BUILD_INTERPOLATION` (`_PyInterpolation_BuildValue`), `LOAD_SPECIAL` (pass the type) (7) | new value-form entries | yes |
| Object-form native boundary | `CALL_METHOD_DESCRIPTOR_NOARGS/_O` (`ml_meth(PyObject *, PyObject *)`), `CALL_INTRINSIC_1` (`PRINT`, `ASYNC_GEN_WRAP`), `INSTRUMENTED_END_FOR/END_SEND` (`monitor_stop_iteration`) (5) | the box moves into a cold helper that owns it, as `super_call` does (`ceval.c:1030-1050`); `value-api.md`'s thunk layer at the C API boundary | yes, from the handler |

So value form deletes the record from all 43 handlers. The type of
`_PyValueTemps` survives only inside the five cold helpers, and goes when
method descriptors get value-form entries.

**Other address-taken locals** (`notes-q5.md` Q2):

- `CALL_KW_PY`: `PyValue keywords[3]` in the inlined `call_bind_known_python`
  (`ceval.c:4401`). Calls80 deletes the staging ("keyword operands go
  straight to their cached destinations"); until then, outline it.
- 10 `GEN_*_EFFECT` handlers: `_PyTransitionRoots roots` (40 B), plus
  `PyValue remaining[5]` and `_PyTransitionPrefix prefix` in
  `GEN_COMPLETE_EFFECT` (`ceval_generator.h`). Q3 deletes the roots. The
  bodies become out-of-line helpers returning `{status, next}`.
- Not present: out-parameters (`PyObject *res; f(&res)`) and `_PyStackRef`
  arrays. Pair structs come back by value (`_PyStatusValue`, `_PyCallResult`).
  Keep it so.

After H3 no handler takes the address of a local. That is a precondition of
the computed-goto backend: the probe's first draft passed `&next_instr` to one
cold helper. GCC then kept `next_instr` in memory for the whole function and
added a canary (`progress.md`, 13:46 entry).

---

## 2. Emission rules

These rules apply to every handler. The generator
(`Tools/cases_generator/ir_generator.py`) emits them; `bytecodes.c` keeps the
uop bodies. Hot sets only order the work.

1. **Entry.** `frame->instr_ptr = next_instr; next_instr += 1;`. Decode
   fields from `word` where used. No per-handler C locals other than C
   values: no `value_temps`, no `ir_displaced`, no `ir_own_*`.
2. **Guards and deopt.** All guards precede every effect. A failed guard does
   `goto PREDICTED_<family>` with `word` unchanged; the family head reruns
   the word. Nothing to undo.
3. **Reads borrow.** Inputs are read in place as `PyValueBorrow`. No take at
   read. A local read stays valid across Python: a proxy write keeps the old
   value alive (`values77.md` rule 1). Temporaries are invisible to Python.
4. **Effect.** Inline when short. Otherwise one call to an out-of-line
   function that takes values and returns a value, a status, a
   `_PyStatusValue` or a `{frame, next}` pair, in registers. No handler local
   has its address taken. Fields needed after a call are decoded before it.
   Object-form callees sit behind cold helpers (Q5).
5. **Detach.** After the effect, each input with `IR_F_DIES_X` set moves to a
   C local, and its slot becomes NULL. An input that the handler stores
   elsewhere (attribute value, call operand, method self, return value) is
   *taken*: moved if dying, `PyValue_DUP` otherwise.
6. **Store.** Results go to their slots. Where the format allows a local or
   the discard slot, use the releasing write: `old = *slot; *slot = v;`. Where
   the format's results are always temporaries, use a plain store (debug:
   assert the slot owns nothing). Values are canonical by construction; no
   `_PyValue_Publish`.
7. **Release.** Close the old values and the detached inputs with
   `PyValue_XCLOSE`: an inline test and decrement, then `_Py_Dealloc` out of
   line on zero. Upstream's `_PyLong_ExactDealloc`-style specializations stay.
   From the first close on, the handler reads and writes no slot.
8. **Periodic check.** Where upstream checks (backward jumps, after calls,
   `RESUME`), the last effect before dispatch is
   `if (eval_breaker & _PY_EVAL_EVENTS_MASK) { if (_Py_HandlePending(tstate)) goto error; }`.
   The instruction is complete, so an error unwinds from a completed word and
   nothing reruns. This is handlers80's livelock, removed by construction.
9. **Errors.** `goto error`. Before step 5 the inputs are still in their
   slots; after step 6 the results are stored. C-owned values are closed
   before the jump. The `error` label calls the cold `_PyEval_ProcessError`,
   which unwinds by the exception table and clears scratch slots, and
   returns `{frame, next}` or NULL to leave the function.
10. **Dispatch.** `word = *next_instr; goto *targets[word & IR_HANDLER_MASK];`
    and nothing else. It compiles to five or six instructions, inside GCC's
    `max-goto-duplication-insns` (8), so GCC keeps one indirect jump per
    site (§3).
11. **Specialization.** The family head decrements its counter. At zero it
    calls the cold specializer, then re-dispatches the same word, before any
    effect. Specialization may allocate and run the GC: the frame is
    untouched at that point.
12. **Instrumentation twins.** The event call comes first. It is a cold
    helper; the frame is consistent because nothing has run. Then the twin
    does `goto` to the base body's label. `INSTRUMENTED_LINE` calls the line
    helper and jumps to the original handler from the side table with the
    same word. Opcode events keep IR-word granularity (calls80 decision).
13. **Frame transfers.** Call, return, yield and send write `frame`, `slots`
    and `next_instr` in the handler. Heavy steps (generator creation, frame
    clear, generator return routing) are helpers that return `{frame, next}`.
    `IR_GEN_TAIL` and the tail-called generator targets
    (`ceval_generator.h`) become such helpers.
14. **Facts.** A per-format fact is free: the generator picks the code. A
    per-site fact is a word bit, tested at run time (`IR_F_DIES_*`, call
    roles). Never a handler variant per site combination.

Deleted from the generator by these rules: `emit_save`'s phase stores,
`IR_CONSUME_INPUT`, `IR_TAKE`, `IR_STORE_DST_IMPL` and its seven variants,
`IR_RETIRE_LOCAL`, `release_displaced`, `emit_ordered_retirement`,
`boxes_into_record`, `canonical_result_source`, `dying_frame_save`, the
`value_temps` parameter of `PROCESS_ERROR_AT`, and the `finish_value_bridges`
test in `DISPATCH`.

---

## 3. Backend: one computed-goto function

### The constraint

Upstream's handler shape is an inline fast path with cold calls. GCC has no
`preserve_none`. Under `no_callee_saved_registers` a tail-called handler
keeps the state in argument registers, which any call clobbers. So *any*
call, even `_Py_Dealloc` on a cold zero-count path, makes GCC move the state
into call-preserved registers at entry and back before the tail call.
Execution79 measures 6.7 register shuffles and 1.9 stack adjustments per
handler entry, 7.3–9.4% of upstream's instructions per kernel.

Handlers80 tried to keep tail calls and make bodies call-free:

- it deferred releases into exit continuations (`tstate->ir_exit` mailbox,
  r9 for one object);
- it tail-called error, periodic and completion exits.

Handlers with any call fell only from 312 to 258 of 333. `_Py_Dealloc` alone
remained in 144. Kernel instructions fell 0.4–2.4%. Always-executed code
grew from 271 KB to 311 KB, and instance attribute loads got 6–7
instructions slower. The shape the user wants, upstream's, needs cold calls.
GCC tail calls tax every handler that has one. In one function the state
lives in call-preserved registers for the whole activation, and a cold call
costs nothing on the fast path.

### Why the earlier computed-goto builds lost, and what differs now

| Build | Result | Cause (from its review) | What the new shape does |
| --- | --- | --- | --- |
| IR evaluator before A3 (`register-ir-v2-a3.md` §7) | 3,304 B frame, 7.6 stack references per word (upstream 1.3); tail calls ran in 0.78–0.88 of its time | Not isolated. Consistent with: address-taken value records, `&displaced` and roots in the one function (A3 later counted 200 canaries in the tail-call link and fixed them per helper, §3.4); `CFLAGS_CEVAL` workarounds (`-fno-gcse`, `-finline-max-stacksize`) | No address-taken locals (H3); cold work outlined; no flags |
| Round60 engine (`engine-post-review.md`) | 4,472 B frame; frame, tstate, PC, word, slots reloaded from stack slots; one indirect jump with 1,131 jumps to it; +55 KB text; lost 18.6–60% | Not isolated by the review. It records the same ownership and publication contracts as selected57, a changed inlining decision on SEND, and a shared dispatch block that reloaded the PC, built a 16-byte constant in `xmm0` and spilled PC+8. The source of that era also redefined state after each refresh (`this_instr = frame->instr_ptr; ir_word = ...; slots = ...`), as checkpoint78 still does | Dispatch is five or six instructions, so GCC duplicates it per site; no reloads after releases (rule 7); bodies lean before the switch |

The merged dispatch has a specific mechanism. GCC factors computed gotos
into one block during optimization. `pass_duplicate_computed_gotos` copies
the block back to each predecessor only if it has at most
`max-goto-duplication-insns` instructions: 8 in GCC 15.2
(`gcc -Q --help=params`). Upstream's dispatch fits, and it has 257 indirect
jumps (checked: `upstream-release-devlibs-3a440b6/Python/ceval.o`,
`_PyEval_EvalFrameDefault`, 13,702 lines). Round60's did not, which is
consistent with its single jump. The reviews did not isolate the cause of
the spills. The probe shows that lean bodies with no address-taken locals
keep the state in registers at the scale of the full handler set.

### The probe

Sources are in `probe/v2/`: `handlers.inc` (handler bodies), `common.h`
(rules 5–7 as `detach_if`, `take_input`, `slot_write`), `cg.c` (one
computed-goto function) and `tc.c` (the same bodies as
`no_callee_saved_registers` musttail functions). They are compiled with
checkpoint78's `ceval.o` flags and real headers (`probe/cc.sh`). The handler
set: `LOAD_FAST_BORROW`, `LOAD_CONST_SLOT`, `STORE_FAST`,
`LOAD_ATTR_INSTANCE_VALUE`, `LOAD_ATTR_METHOD_WITH_VALUES`,
`BINARY_OP_ADD_INT`, `COMPARE_OP_JUMP_INT`, `STORE_ATTR_INSTANCE_VALUE`,
`CALL_PY_EXACT_ARGS` (operand word, child at the arena top),
`RETURN_VALUE`, `JUMP_BACKWARD` (periodic), `LOAD_GLOBAL_MODULE`,
`BINARY_OP_SUBSCR_LIST_INT`, `CALL_BUILTIN_O` (a hot native call, then the
periodic check), seven family heads and `error`.

Findings:

- **Frame.** `sub $0x28,%rsp` plus six pushes. No canary. State is in
  registers: `r13` frame, `r14` slots, `r15` tstate, `rdx` next_instr, `rcx`
  word. GCC keeps the last two in caller-saved registers and saves them only
  around calls.
- **Scale.** 16 copies, 336 labels, in one function (`probe/v2/cg_x16.c`):
  the same 40-byte frame, 225 indirect jumps (one per hot dispatch site plus
  entry), and identical fast paths in copies 0, 8 and 15 (597 instructions
  for the 14 hot handlers). Family heads went to `.text.unlikely` on their
  own, because they call `cold` helpers.
- **Stack references** on the fast path: 0 in 12 of 14 hot handlers; 4 in
  `RETURN_VALUE` and 5 in `CALL_BUILTIN_O`, around their calls.
- **Against tail calls.** The same bodies as GCC tail calls are 3–11
  instructions longer per handler (577 against 680 over the 14). The
  difference is register moves and `rsp` adjustments (`probe/fastpath-results.txt`).
- **The lesson of the first draft.** One address-taken state variable
  (`&next_instr`) put that variable in memory in every handler and added a
  canary.

**Limits.** The probe has 14 hot bodies, copied, not 330 distinct ones. Its
bodies leave out statistics, free-threading atomics (no-ops in this build)
and some real work. Examples: `LOAD_GLOBAL_MODULE`'s builtins guard,
`CALL_PY_EXACT_ARGS`'s `RESUME` skip, and generator returns, which go to a
helper. Heavy handlers (generators, generic calls) must follow rule 4 for
the result to hold. That is what H4's stop checks test (§6).

### Decision

**Computed goto, one function, GCC.** Detail:

- **Cold helpers return to dispatch through the handler.** A helper returns
  a status, a value or `{frame, next}`. The handler assigns and does
  `DISPATCH()` or `goto error`. No helper jumps into the evaluator.
- **Keeping dispatch jumps separate.** Rule 10. A shape check counts the
  evaluator's indirect jumps after each build: there must be at least one
  per handler label in `.text`.
- **Delete with the tail-call backend:** `TAIL_CALL_PARAMS`, `TARGET` as a
  function, `Py_PRESERVE_NONE_CC`, `py_tail_call_funcptr` and
  `instruction_funcptr_handler_table`, `JUMP_TO_LABEL`'s tail call,
  `IR_GEN_TAIL`, `CFLAGS_HANDLERS` (`-fomit-frame-pointer`, `-ffixed-rbp`)
  and `PY_TAIL_CALL_CC` in `configure.ac`/`Makefile.pre.in`, and the
  `enter_handlers` shim. The evaluator keeps the frame pointer, as upstream
  does.
- **Nested evaluation** costs one evaluator activation per re-entry from C,
  as upstream. The probe's activation is 96 bytes (40-byte frame plus six
  pushes and the return address). The real one will be larger; upstream's
  frame is 360 bytes. `TestRecursion` and the C stack limit are checked
  in H4.
- **Not chosen:** keeping tail calls and making bodies call-free (handlers80,
  measured above), and clang's `preserve_none`. A clang-21 build moved kernel
  instructions by −1% to +4% (`execution79-instruction-gap.md`), and the
  project measures with one GCC configuration.

Parity71's option 5 ("resident core", deferred) asked for effect
continuations to be removed from the CFG first, and every core↔effect
crossing priced. Rules 4 and 13 do the first: effects are calls that return,
and there are no continuations. The probe prices the crossing at zero
register moves for a cold call. For a hot call the price is the argument
setup plus saving `rdx`/`rcx` (4–5 stack references in `CALL_BUILTIN_O`).

---

## 4. Before and after for the hot handlers

Instructions on the shortest path from entry to dispatch. The "static"
columns use one tool (`probe/fastpath.py`) on checkpoint78's `ceval.o` and on
the probe objects. It takes the cheapest arm at each branch (immortal or
vacant values). It disagrees with checkpoint78's measured ledgers by 0–12%.
"Probe" figures are the 336-label build. Upstream figures are measured
ledgers.

| Handler | c78 measured | c78 static | Probe CG | Probe TC | Upstream measured |
| --- | ---: | ---: | ---: | ---: | ---: |
| `LOAD_FAST_BORROW` | 37 | 37 | 24 | 28 | 14 |
| `LOAD_ATTR_INSTANCE_VALUE` | 69–83 (`LOAD_ATTR_SHAPE*`) | 68 | 48 | 54 | 39 |
| `LOAD_ATTR_METHOD_WITH_VALUES` | 51 | 45 | 50 | 53 | 30 |
| `BINARY_OP_ADD_INT` | about 55–65 (hand count) | n/a¹ | 37 | 43 | n/a |
| `COMPARE_OP_JUMP_INT` | — | 66 | 39 | 39 | n/a² |
| `STORE_ATTR_INSTANCE_VALUE` | 66 (`STORE_ATTR_SHAPE`) | 74 | 53 | 60 | 48 |
| `CALL_PY_EXACT_ARGS` (2 args) | 156 | 146 | 88 + 10–20³ | 98 | 139 |
| `RETURN_VALUE` + frame clear | 143 | 52 + helper | 39 + helper | 39 + helper | 143 |
| `JUMP_BACKWARD` | 22 | 23 | 17 | 25 | 18 |
| `LOAD_GLOBAL_MODULE` | 58 | 54 | 38 + 5–8³ | 47 | 41 |

¹ The tool's shortest path leaves through the deopt dispatch.
² Upstream has no fused compare-and-jump; its `COMPARE_OP_INT` plus
`POP_JUMP_IF_*` are not in the ledgers.
³ Work the probe leaves out (estimate): `RESUME` skip and the remaining
header fields; the builtins-version guard.

The "after" figures are estimates for the real handlers: the probe plus
0–15%. Every entry falls, except `LOAD_ATTR_METHOD_WITH_VALUES`. That one
does the same work as today (guards, DUP of the method, move or DUP of self,
two plain stores), minus the convention. Upstream's is 20 instructions
shorter because it moves a *borrowed* self without a count. Calls80 decided
against borrowed arguments; this is that decision's price, about 6
instructions per method load (estimate).

### Before and after, in source

`LOAD_ATTR_INSTANCE_VALUE`. Before (`generated_ir_cases.c.h:14781`, after its
guards):

```c
attr = PyValue_XDUP(found);
ir_own_B = IR_CONSUME_INPUT(ir_word, B, owner);         /* kind test, take */
PyValue receiver = ir_own_B;
frame->phase_state = 0;
IR_STORE_TEMP_DST(ir_word, A, attr);                    /* kind test 1 */
{
    int retire_needed = _PyValue_CloseMayReenter(receiver);
    if (!retire_needed) { PyValue_XCLOSE(receiver); receiver = PyValue_NULL; }
    if (retire_needed) {
        int retire_failed = retire_published_values(tstate, frame, next_instr, NULL,
            receiver, PyValue_NULL, (IR_IS_LOCAL_FIELD(ir_word, A) || IR_A(ir_word) == IR_NO_DST) ? attr : PyValue_NULL);
        if (retire_failed < 0) { PROCESS_ERROR_AT(RAISE, 0, next_instr - 1); }
        this_instr = frame->instr_ptr;
        ir_word = FT_ATOMIC_LOAD_UINT64_RELAXED(*this_instr);
        slots = _PyFrame_Slots(frame);
    }
}
/* _PUSH_NULL_CONDITIONAL */
if (IR_IS_LOCAL_FIELD(ir_word, A) || IR_A(ir_word) == IR_NO_DST) {
    IR_STORE_DST(ir_word, A, attr);                     /* kind test 2, publish, displace */
}
if (!PyValue_IsNull(ir_displaced)) {
    frame->phase_state = 0;
    if (finish_displaced_local(tstate, frame, next_instr, NULL, ir_displaced) < 0) { ... }
}
DISPATCH();
```

After (`probe/v2/handlers.inc`):

```c
PyValue dead_b = detach_if(slots, word, IR_F_DIES_B, IR_B(word));
slot_write(&S(A), PyValue_DUP(found));     /* store, then close the old value */
PyValue_XCLOSE(dead_b);
DISPATCH();
```

The others, after (guards as today, then):

- `LOAD_FAST_BORROW`: `slot_write(&S(A), PyValue_DUP(S(B)));`. Calls80 folds
  most of these into operands.
- `LOAD_ATTR_METHOD_WITH_VALUES`: `slots[A+1] = take_input(B);
  S(A) = PyValue_DUP(fb->method);`, plain stores (the format's results are
  temporaries).
- `BINARY_OP_ADD_INT`: `slot_write(&S(A), _PyValue_FromIntScaled(sum));`. No
  input code; on overflow, deopt to the family head, which boxes.
- `COMPARE_OP_JUMP_INT`: the compare and a `next_instr += IR_DS(word)`. No
  stores, no input code.
- `STORE_ATTR_INSTANCE_VALUE`: `*field = take_input(B);` then close the old
  field value and the detached owner. A first store to an empty field takes
  the family head (insertion order and shape transition, out of line).
- `CALL_PY_EXACT_ARGS` (calls80 design O): guards; reserve at the arena top;
  `child->base[i] = take_operand(ops, i)` for each operand, unrolled per
  operand word; header stores; `frame = child; slots = base;
  next_instr = code start`. No release, no publication. Before:
  `call_transaction_begin`'s recheck loop, `CALL_EFFECTS` fallbacks,
  `call_push_moved_exact_args`, `ir_displaced` and the window NULL store.
- `RETURN_VALUE`: `retval = take_input(A)`; unlink; clear the dying frame (one
  helper, upstream's order); `slot_write(&slots[frame->return_slot], retval)`.
  A discarded result goes to the discard slot. Generator frames call one
  helper that returns `{frame, next}`. Before: `_PyFrame_SetReturnValue`'s
  displaced-local path, `phase_state`, `_PyEval_FrameClearAndPopGeneralWithResult`
  and the tail-called generator route.

---

## 5. Ownership and observation

| Change | What keeps the frame consistent when Python runs |
| --- | --- |
| Borrowed reads, detach after the effect | Inputs stay in their slots through any escaping effect, so GC sees them and the unwinder clears them on error. A borrowed local survives a proxy write (`f_overwritten_fast_locals`, `values77.md` rule 1) |
| Releasing write | New value stored before the old one is closed (upstream `STORE_FAST`; `test_ir.py:2055` still holds). The finalizer sees the new binding in `f_locals` |
| Releases last, no reloads (O4) | The current frame, `instr_ptr` and the executing words cannot change under a finalizer; the handler touches no slot after its first close |
| No `phase_state` | GC scans every slot; each is canonical or NULL (O2). C-held values are external references to cyclic GC |
| Canonical by construction | Constructors canonicalize; debug `_PyValue_CheckHome` at every store and in the GC visit |
| No transition roots | Upstream's frame clear: unlink, hold the result in C, NULL then close each slot |
| Discard slot | Holds a valid owner; GC and frame clear see it; never shown in `f_locals` (above `co_nlocalsplus`) |
| Computed goto | No semantic change. `frame->instr_ptr` is stored at entry as before; tracebacks and `f_lineno` read it |
| Instrumentation twins | Events fire before any effect. A callback that enables tracing takes effect at the next dispatch, which reads the rewritten id |
| `sys.settrace` | Line and opcode events from the twins at IR-word granularity (calls80 decision). Jumps set by a trace function apply at the event, before the word's effects |
| Generator suspension | Dead temporaries own nothing; a suspended frame keeps its live values and the discard slot. `YIELD_VALUE` follows rules 5–7 |

---

## 6. Migration plan

Steps in dependency order. Each step deletes the form it replaces. Gates are
correctness. The shape checks in H4 are stop-and-diagnose checks on the
mechanism, not performance gates. Agent-hours assume Sonnet workers on units
of 10–20 minutes, one careful implementer per core unit, and one cleanup
pass.

| Step | Content | Deletes | Test ladder | Agent-hours (estimate) |
| --- | --- | --- | --- | ---: |
| H0 | Shape tool: from `ceval.o`, frame size, canaries, indirect jumps per handler, fast-path length and stack references for the hot set (`probe/fastpath.py` grown into `Tools/pyjit/handler_shape.py`). Baseline on checkpoint78 | | Runs on c78 and the probe objects | 1–2 |
| H1a | Canonical storage: plain stores in the store macros; audit the five `_PyValue_PointerSteal` callers and the box producers | `_PyValue_Publish` at stores, `IR_STORE_CANONICAL_*`, `canonical_result_source`, `87f13087ed9`'s proof tests | (1) `(yield) is 5`, `x = 2**40; x is 2**40` round-trips, float NaN. (2) `test_tagged_values`, `test_generated_cases`, `test_ir*`, `test_long`, `test_float`, `test_json`. (3) Full suite at H2 end | 4–6 |
| H1b | `phase_state` out of handlers; GC scans all slots; debug slot check in the GC visit | `emit_save` phase stores (1,635), PUBLISHED, `BINARY_PROVIDER`, `_PyIR_ShadowFramePhase` and its plans; audit of the lifecycle bits' readers | (1) `gc.collect()` in `__del__` during a generic `BINARY_OP`. (2) `test_gc`, `test_generators`, `test_coroutines`, `test_frame`, `test_ir*`, `_testinternalcapi` probes updated | 3–4 |
| H2 | Ownership rules 3, 5, 6, 7 in the generator; `IR_F_DIES_*` from `_PyIR_ExecLocalFlags`' successor; discard slot in `Python/regcfg.c` and the assembler; frame clear in upstream's order | everything in Q3's deleted list; `IR_CONSUME_INPUT`, `IR_TAKE`, `IR_STORE_DST_IMPL` family, `IR_MOVE_TO_DST_OR_DISCARD`, `_PyIR_UnpackToLocals` interleaving, `value_operation_close3` ordering | (1) `x = o.a` where the old `x` has `__del__` reading `f_locals`; `o.a.b` with a dying receiver; `f()` discarded. (2) The eight Q4 tests rewritten as consistency checks; `test_gc`, `test_weakref`, `test_finalization`, `test_sys_settrace`, `test_frame`, `test_generators`, `test_ir*`. (3) The ownership-review77 harness on a debug build with `PYTHONMALLOC=debug -X dev` and `gc.set_threshold(1, 1, 1)`. (4) Full suite, release and debug | 10–14: generator core 4; compiler discard slot and flags 2; about 60 hand-written uop fixups in `bytecodes.c` as 10-minute units, 3; tests 2; cleanup 1–2 |
| H3 | Bridge records and canaries: asserts and pre-checks (31 handlers), seven value forms, five cold boxing helpers; `CALL_KW_PY` keyword staging out of line; `GEN_*_EFFECT` bodies as helpers | `_PyValueTemps` in handlers, `value_boxes`, `finish_value_bridges` in `DISPATCH`, `value_temps` in `PROCESS_ERROR_AT`, `boxes_into_record` | (1) `except 5`, `[*5]`, `{**5}`, `a, b = 3`, `raise 5`, `case 5():`, `t"{5}"`, `(5).bit_length()`, all raising today's messages. (2) `test_exceptions`, `test_except_star`, `test_patma`, `test_unpack`, `test_unpack_ex`, `test_import`, `test_asyncgen`, `test_coroutines`, `test_tstring`, `test_descr`, `test_super`. (3) Shape check: zero `__stack_chk_fail` in handlers | 6–8: 54 units in parallel, about 3; value forms 2; cleanup 1–2 |
| H4 | Backend: `TARGET` becomes a label, `DISPATCH` a computed goto, `JUMP_TO_PREDICTED` a goto, `PROCESS_ERROR_AT` `goto error`; generator transfers (`ceval_generator.h`) return `{frame, next}`; twins jump to base labels | the tail-call backend list in §3 | (1) `f(x, y)` loop; a generator; a coroutine; recursion to the limit; `sys.settrace` and `sys.monitoring` smoke. (2) `test_sys_settrace`, `test_monitoring`, `test_generators`, `test_coroutines`, `test_asyncio` (subset), `test_capi`, `test_ir*`, `TestRecursion`. (3) Full suite, release and debug | 6–10: macros and generator 3; generator-transfer helpers 3 (careful); cleanup 1–2 |
| H5 | Calls80 steps 1, 2a–2c and 3, written in the new shape: operand fill is `take_operand`; push inline; allocation, vectorcall and specialization are helpers returning `{frame, next}` or a status. Calls80's step 0 (handlers80) is replaced by H1–H4 | per calls80 §9 | per calls80 §9 | calls80's 23–34, less about 2 (no tail-call continuations) |
| H6 | Hot-code layout (execution79 order step 4) | | | separate |

Total for H0–H4: about 30–44 agent-hours; with fan-out about two working
days of wall clock (estimate). H5 follows at calls80's estimate.

**Why this order.** H4 needs H1–H3: round60 shows what the switch does to
protocol-heavy bodies. H1–H3 are each correct and measurable in the
tail-call build. They will also cut some calls there; how many is not
predicted. H5 after H4 writes the call handlers once, in their final form.
The opposite order (calls80 first, in tail-call form with continuations)
would build handlers80's continuation shape and tear it down. That is a
stopgap, so it is not proposed.

**What survives from handlers80** (`pyjit/handlers80`, parked):

- the rule "publication first, then releases in site order" becomes rule 6
  then rule 7, without the site order;
- error exits through one label and helper become rule 9;
- `handler_calls.py` and the kernel instruction harness (`instr.sh`,
  `perstat/`) become measurement tools.

**Deleted from handlers80:** the deferred releases, the `tstate->ir_exit`
mailbox, `_PyEval_ExitTail`, `_PyEval_ExitOneDead`/`ExitOneDisplaced`, the r9
argument, the periodic tail (and with it the livelock), the int/float exit
tails, and the generator's `defers_releases`/`defer_blocker` analysis with
`IR_DEFER_REPORT`.

### Measurement plan

After each of H1a, H1b, H2, H3, H4 and H5:

- **Instructions per loop**, exact, for the five execution79 kernels
  (DeltaBlue, raytrace, coroutines, deepcopy_memo, richards), by perf-stat
  N-differencing (`E59/handlers80/instr.sh`). These do not depend on layout,
  so each step is attributed exactly.
- **Per-call ledgers** for execution79's cases (`f(x, y)`, `o.m(x)`,
  `o.store(x)`, attribute cases) by gdb trace.
- **Shape** from H0's tool: evaluator frame bytes, canaries, indirect jumps,
  static fast path of the hot set, stack references on fast paths.
- **Stack references per word** from the traces (upstream 1.3; c78 1.5).
- Timed geomean over the standard set, recorded, not gated.

Early stops (stop and diagnose; no flag, padding or compiler switch):

- after H3: any handler with a canary, or a hot handler with an
  address-taken local;
- after H4: evaluator frame above 1 KB; a canary in the evaluator; fewer
  indirect jumps than handler labels in `.text`; stack references per word
  above 2.5; or the hot set's static fast paths not at least 5% shorter than
  after H3. Each points at a known cause: an address-taken local, an
  oversized dispatch block, a body that breaks rule 4, or state redefined
  after a call;
- after H4: kernel instructions not lower than after H3 in at least four of
  the five kernels. Diagnose with the per-handler fast paths before any
  further step.

### Expected effect (estimates)

As shares of upstream's instructions per kernel, against execution79's
excess (DeltaBlue +29.6%, raytrace +15.6%, coroutines +30.3%, deepcopy_memo
+29.5%, richards +8.6%):

| Kernel | Convention (H4; execution79 cause 1) | Operand and result protocol (H1–H3; cause 2) | Total, before calls80 |
| --- | ---: | ---: | ---: |
| DeltaBlue | −7 to −9 | −5 to −8 | −12 to −17 |
| raytrace | −6 to −7 | −3 to −5 | −9 to −12 |
| coroutines | −6 to −7 | −2 to −4 | −8 to −11 |
| deepcopy_memo | −6 to −8 | −2 to −4 | −8 to −12 |
| richards | −6 to −7 | −2 to −4 | −8 to −11 |

The convention column assumes H4 removes 80–100% of execution79's
measured shuffle share. The probe removes all register shuffles and `rsp`
adjustments on fast paths without a hot call. The protocol column assumes the
hot handlers lose 20–35% of their instructions (the static table above:
−30% summed over 11 handlers with comparable figures), and that handler
bodies are about a third of each kernel's instructions. That share is a
guess (±50%); H0's baseline measures it. Calls80's own estimates (−0.35% to
−5.4%) come on top. On these estimates richards would reach about upstream's
count, and the others would keep 5–20% of excess. Calls, generators and
retirement remain the largest items.

---

## 7. How calls80 and handlers81 combine

- Calls80's call and return handlers are written to section 2's rules. Its
  "read once, first" rule is rule 3 plus rule 5. Its "commit" point
  (`tstate->current_frame = child`) needs nothing between reservation and
  commit that can run Python. Rules 4 and 13 keep allocation and
  specialization before the reservation, in helpers.
- Its operand fill ("local DUP, temp move, constant slot tag test") is
  `take_operand`. Constant slots carry no `IR_F_DIES` bit.
- Its "Handlers80 compliance" paragraph (tail-called continuations) is
  replaced. Allocation, vectorcall and specialization are helpers called
  from the handler, returning `{frame, next}` or a status.
- The discard slot replaces `return_slot == IR_NO_DST`. `RETURN_VALUE` has
  one store path.
- Order: H1–H4, then calls80 steps 1, 2a–2c, 3 (H5). Calls80 step 1 (child at
  the arena top) does not depend on the backend and could run in parallel
  with H1–H3 if staff allow. Its new code would then follow rules 3–7 in the
  tail-call build and survive H4 unchanged.

---

## 8. Decisions for the user

Decided 2026-11-19: all five as recommended. The discard slot replaces
`IR_NO_DST`; the eight Q4 tests become consistency checks; the transition
roots, `gc_verify`'s C-held owner walk and `get_transition_root_ids` are
deleted; H4 switches to computed goto; the method-load gap is accepted.

1. **The discard slot.** A discarded result lives until the next discard or
   frame exit (incidental timing). The alternative keeps an `IR_NO_DST` test
   (two instructions) in every handler that can discard.
2. **Finalizer order within an instruction.** Eight tests change from exact
   event lists to consistency checks (Q4). Inputs' finalizers may see the new
   local binding.
3. **The tracing collector's transition roots** go (Q3). `gc_verify` and
   `get_transition_root_ids` lose C-held owners, or are deleted.
4. **The backend switch** (H4) deletes the tail-call build and
   `CFLAGS_HANDLERS`. The single GCC configuration stays.
5. **Method loads** stay about 20 instructions above upstream's. About 6
   of those are the count on self, the price of calls80's "no borrowed
   arguments"; revisit only if measured.

## 9. Risks and open questions

- **Scale of real bodies.** The probe's copies are lean and identical.
  Generator, generic-call and exception handlers are larger, and any of them
  that keeps C values live across a call raises register pressure. H4's stop
  checks catch it. The fix is always to outline (rule 4).
- **Lifecycle bits of `phase_state`.** CREATED and ERROR_SCOPE select precise
  slot views in frame copy and GC. Whether "all slots valid or NULL" lets
  those readers scan everything needs the H1b audit. If not, the bits stay
  as cold state; handlers still never write them.
- **`IR_F_DIES` derivation.** It must follow each format's consume roles, not
  only the slot class: `FOR_ITER` keeps its iterator. The generator already
  knows the roles (`IRStack.consume`).
- **Hot native calls** save `rdx`/`rcx` around the call (4–5 stack
  references in the probe). If the shape tool shows this matters,
  `next_instr` can be recomputed from `frame->instr_ptr` after the call.
- **Estimates.** The kernel table rests on execution79's shares and on a
  guessed handler share of kernel instructions. H0 replaces the guess with a
  measurement before H1 starts.
