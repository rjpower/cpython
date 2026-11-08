# Review 2: register IR v2 implementation packet

Reviewed: [register-ir-v2-packet.md](../proposals/register-ir-v2-packet.md),
revision 2, at `6452354690e`, against
[review 1](register-ir-v2-packet-review-1.md). Source read: `48bb4d6d1d3` through
`git show`. Nothing was built or timed. One preprocessor check of compiler
attributes was run. Date: 2026-11-04.

The coordinator's decisions on findings 3, 7 and 8 were not reopened: A3c
leaves one backend, A3a–A3c land together after 9b, and D4 is step A5 after
representation step 8.

Line numbers without a file name refer to the proposal at `6452354690e`.
`gen.h` is `Python/generated_ir_cases.c.h` at `48bb4d6d1d3`.

## Verdict

**Accept with changes.** Revision 2 deals with 17 of the 20 findings in full,
and the text matches each stated disposition. Findings 3, 5 and 9 are partly
resolved. The new activation record is the right idea: the computed-goto loop
already shares one scratch area among all its handlers. But its contents are
under-counted, and its rules are not stated. A5 covers only part of D4.

Changes wanted before implementation:

1. Size the activation record and list everything it holds: 37 boxes, not
   three, and the other locals that handlers pass to out-of-line functions
   (R2-1).
2. Make the record the activation's one structure: the entry frame, the borrow
   floor and the JIT state go in it. State its access, lifetime and ownership
   rules (R2-2).
3. Bring A5 up to pyvalue.md D4: the typed borrow, the debug checks and the
   producer list. Give the typed borrow hours and a place after 9a, or say
   why it is deferred (R2-3).
4. Put A2's debug checks where the deleted writes were, on the writer side,
   and on frame switches (R2-4).
5. B5 renumbers too (R2-5). The smaller corrections are R2-6 and R2-7.

## Review 1 findings: disposition check

| # | Sev. | Status | Where in revision 2 | Note |
|---|---|---|---|---|
| 1 | major | Resolved | 42; 65–75; 198–245; 1084–1086 | Timing leads and static counts are labelled as diagnosis. One wording error (R2-7) |
| 2 | minor | Resolved | 255–273; 957; 1137–1139 | Lower bound named; object path given; weights' gap listed |
| 3 | blocking | **Partly** | 44; 545–574; 1071–1076 | The step exists and is independent of G2. Its content omits half of D4 (R2-3) |
| 4 | major | Resolved | 45; 164–180; 505–543; 932–933 | Out-parameter deleted in 12 families. Assert placement: R2-4 |
| 5 | major | **Partly** | 46; 594–626 | Transaction records move to the record. Other by-address locals and the box-area size are missing (R2-1) |
| 6 | major | Resolved | 22–26; 578–584; 628; 366–367 | Confirmed: Clang 21 has `preserve_none` and `musttail`; GCC 15.2 has `no_callee_saved_registers` and `musttail` |
| 7 | major | Resolved | 48; 658–670; 928–929; 1065–1067 | |
| 8 | major | Resolved | 49; 381–399; 417 | `bytecodes.c` listed (385, 616); unforced orders dropped or labelled |
| 9 | major | **Partly** | 50; 732, 753, 769, 780, 794, 805 | Magic bump in every format step. The renumbering list omits B5 (R2-5) |
| 10 | major | Resolved | 1011–1035 | Sums to 97 (41 + 53 + 3). R2-1 and R2-3 add work |
| 11 | minor | Resolved | 339–367 | All six items corrected; caveat removed |
| 12 | minor | Resolved (scope cut) | 586–591; 1148–1165 | |
| 13 | minor | Resolved | 443; 465–476; 1097–1103 | |
| 14 | minor | Resolved | 443; 454; 477–480 | |
| 15 | minor | Resolved | 137; 713–724; 1119–1124 | Omits the allocator's own "two live" check (R2-7) |
| 16 | minor | Resolved | 127; 736–737; 749–750; 1091 | |
| 17 | minor | Resolved | 672–689 | |
| 18 | minor | Resolved | 496–502; 627–638; 806–811; 733–737 | |
| 19 | minor | Resolved | 1044–1128 | Every listed item and missing edit is present |
| 20 | minor | Resolved | 328; 513 | |

## New findings

### R2-1. Major. The activation record is mis-sized and holds only part of what handlers pass by address

- **Where.** A3a, lines 594–616 ("the box area (three boxes) and every
  transaction record"; "the remaining `&res` and `&left` become
  slot-addressed").
- **The box area.**
  - Box capacity varies by handler, from 2 to 37 (`value_owners =
    (_PyValueTemps){value_boxes, 0, N}` in gen.h).
  - The computed-goto loop already shares one area for all handlers: "One
    scratch area serves every handler", `PyObject *value_boxes[37]` (gen.h:12–14).
  - The record needs the maximum, 37 boxes (296 B), and the generator should
    emit that maximum as a constant.
- **Other locals passed to out-of-line functions.** About 140 handlers take a
  local by address. The record's table covers the transaction records. This
  count is mine: a source scan of gen.h, keeping locals declared in the
  handler and passed to a function that is not `static inline`. It leaves
  these out:
  - `&attr` and `&old_value` passed to `retire_published_values` in the nine
    specialised attribute handlers: `LOAD_ATTR_INSTANCE_VALUE`, `_MODULE`,
    `_SHAPE`, `_SLOT`, `_WITH_HINT` (gen.h:13226, 13552, 13971, 14063) and the
    four `STORE_ATTR_*` (from gen.h:18540).
    - These are the hottest handlers that take a local by address: 15% of
      entries on chaos, 22% on deltablue, 30% on float, 27% on go, 21% on
      raytrace and 24% on richards (`checkpoint-20261103-C-kernels-boxes.json`).
    - The call is on the cold local-destination path. But a local whose
      address escapes gets a stack home and a canary for the whole function.
  - Further `held_*` names passed to `value_operation_close3` and
    `value_operation_restore2` in about 6 handlers: `held_dict`, `held_sub`,
    `held_value`, `held_container`, `release_value`, `retained_dict` and
    `watched_dict`.
  - `&null_or_index, &item` in `FOR_ITER` and `INSTRUMENTED_FOR_ITER`
    (`_PyForIter_VirtualIteratorNext`, gen.h:9991, 11337).
  - Out-parameters of runtime APIs that upstream also has:
    - `_PyMapping_GetOptionalItem2(…, &err)` (4 handlers, gen.h:14246, 14692);
    - `PyUnicode_Append(&temp, …)` (gen.h:931);
    - `_PyDict_MergeUniq(&dupkey)`;
    - `get_exception_handler(&lasti)`;
    - `_PyEval_LoadGlobalValue(&v)` and `_PyEval_GetIter(&index_or_null)`;
    - `native_call_finish(&c, &s)`.
- **Change.**
  - State the rule: every local that a handler passes to an out-of-line
    function lives in the record, unless the callee is changed to take and
    return values.
  - List the remaining sites with a disposition for each.
  - State the record's size. It is per activation, so it adds C stack per
    nested evaluation.
  - Raise A3a's estimate by the extra sites (about 3–5 h, a judgement).
  - The `tcframes.py` target (lines 619–626) is the right check. It will fail
    unless these sites are covered.

### R2-2. Major. The record should be the activation's one structure, with stated rules

- **Where.** Lines 606–614 ("lives in the entry function's frame and is reached
  through `tstate` (pushed and popped at eval entry and exit)"), and 647.
- **Per-activation state at HEAD already lives in two places:**
  - the C frame of the loop function: `_PyEntryFrame entry` (ceval.c:2206),
    the JIT locals (2209–2214) and, under computed goto, the shared box area;
  - the thread state: `_PyEval_EvalFrameDefault` saves and restores
    `borrow->eval_floor` around the loop (ceval.c:2461–2476).

  The proposal adds a third link through `tstate`, beside the floor. One
  structure per activation, owned by `_PyEval_EvalFrameDefault` and linked once
  from `tstate`, would hold all four: the entry frame, the outer floor and its
  pending request, the JIT context, and the scratch. Without that, the record
  is a second home for activation state that already has one.
- **The box area and the borrow stack.** They are both per-activation, both
  reached from `tstate`, and both hold owned box references. They differ in
  lifetime: a handler's boxes die at its dispatch, while the borrow stack's
  entries die at scope exit. value-api.md VA4 also keeps slot adapters off
  the stack. The separation is defensible, but the proposal should state it,
  and native-boundary.md §2 should mention the record.
- **Rules the proposal must state:**
  - **Explicit access.**
    - A handler loads the record pointer from `tstate` once, on the path that
      uses it, and passes it to its helpers as an argument, as `value_temps`
      is passed today.
    - No helper reads `tstate`'s record itself. A C function called from a
      handler does not enter a new activation, so if it found the record
      through `tstate` and boxed into it, it would overwrite the live
      handler's boxes.
    - This also keeps AGENTS.md's "explicit ownership".
  - **Lifetime.**
    - Handler fields are dead at every dispatch. In debug builds, assert an
      empty box count at dispatch.
    - The JIT fields persist across handlers, so separate them from the
      handler fields in the struct.
  - **Ownership.** The record holds owned values only, which is true today:
    `PyValue_MakeHeapSafe` and `PyValue_NewRef` at gen.h:65–66 and
    13959–13961. This makes it consistent with D4 if the record counts as a
    home.
  - **No scan.** Delete "A collector could scan this record" (line 612).
    Handlers leave stale copies there; `attr` is still held after
    `IR_STORE_DST`. Free threading is a non-goal: pyvalue.md:379 says
    "free-threaded builds `#error`".
  - **Stack switching.** The pointer refers to the C stack, like the entry
    frame that `tstate->current_frame` reaches. Stack-switching extensions
    (greenlet) must save and restore it. Add it to the ports notes.
- **Cost of the indirection.**
  - One dependent load from `tstate` (in a register under A3a) on each path
    that uses the record. Today such a handler pays for a frame and a canary
    instead: 9 prologue instructions in `tcvt`'s `LOAD_FAST_BORROW`
    (line 321).
  - Record fields have the same type as frame slots, so the compiler must
    assume a slot store can change them. Use a `restrict`-qualified pointer,
    or copy fields into locals between calls.
  - Under Clang, a parameter would cost nothing, since 12 registers are free.
    Under GCC, the sixth register is held for A4.
  - Count the record-using entries in the §8 measurements.
- **What was checked and is sound.** One record per activation is safe for
  nesting, suspension and re-entry:
  - Handler C state is already dead at every dispatch; the shared
    computed-goto scratch depends on that today.
  - Every path that runs Python code (`__del__`, trace and monitoring
    callbacks, `gen.send` from C, PEP 523 hooks) enters
    `_PyEval_EvalFrameDefault`, which would push its own record.
  - A suspending generator or coroutine crosses a dispatch, so it carries no
    handler state.
  - The JIT context already lives per activation (`pycore_jit.h:50–52`,
    "never copy an activation context"), so moving it in changes nothing.

### R2-3. Major. A5 implements half of D4

- **Where.** A5, lines 545–574; §10 (6 h); §3 line 384.
- **What pyvalue.md D4 decides** (pyvalue.md:513 and 793–815), and what A5
  includes:

  | D4 item | In A5? |
  |---|---|
  | No home holds a borrow; bit 0 in a home means immortal | yes |
  | "Borrows live only in C locals, as a distinct C type that store APIs reject" | **no** |
  | Remove `MakeHeapSafe` from setters | **no** |
  | Debug asserts: "tagged implies immortal" in `_PyGC_VisitValue`; heap safety on stores | **no** for temporaries. `IR_STORE_DST` already asserts it for locals (`ceval_macros.h:714`) |
  | "List the producers of borrows that reach homes and drive them to zero" | **no** |
  | `_Py_VISIT_VALUE` in `traverse_slots` | **no** |
  | Merge the `DUP`/`NewRef` and `CLOSE`/`Close` pairs | **no** |

- **Why the typed borrow matters here.**
  - A5 makes `PyValue_BorrowAt` return the slot bits, and drops the tag from
    `IR_LOAD_BORROWED`, `IR_BORROW_KNOWN` and `IR_DROP_BORROW` (lines
    555–557).
  - A borrowed local then has the bits of an owned value. Today the tag makes
    a `PyValue_CLOSE` on an in-place input a no-op. After A5, the same close
    would decref the local.
  - The distinct C type is D4's replacement for that safety net. Without it,
    A5 removes the safety net and puts nothing in its place.
- **Missing checks.**
  - Representation step 8 turns the home checker into an assert
    (representation packet line 523).
  - `_PyValue_CheckFrame` (`Python/frame.c:102–135`) already walks every
    slot in debug builds.
  - A5 should make both reject a mortal object with bit 0 set in any slot.
    That is the check that finds a missed producer. §9.2's "debug refcount
    checks" (line 996) would find it only later and elsewhere.
- **Cost, estimate and order.**
  - The 44 → 28 figure (lines 413, 566–567) comes from `rep`, which takes no
    count. That is right on the inline-scalar path. On the object path, A5
    adds an incref on the load and a decref in the consumer, and the loop's
    −10% leaves out that code. §8 measures object paths after A5 (line
    957), so the figure need only be labelled.
  - pyvalue.md says the typed borrow "touches every core signature that
    takes a borrowed value". That is not in the 6 h. It would overlap 9a's
    renames: `PyValue_FromInt32` in 13 files and `PyValue_InlineInt` in 18.
- **Change.** Either:
  - **split the step**:
    - A5a, after step 8: homes, counts, the producer list, the frame and home
      checks, and `traverse_slots`; about 6 h.
    - A5b, after 9a: the typed borrow, the `MakeHeapSafe` removal from
      setters, and the pair merges; hours stated.
  - **or** state that the typed form is deferred, with its reason and its
    step.

  Add `Python/frame.c` and the home checker to A5's files. Add a test where
  a trace function rebinds a local through `f_locals` between its copy into
  a call window and the call. The callee must still see the original object.

### R2-4. Minor. A2's invariant assert is not where a violation would show first

- **Where.** Lines 533–535 ("asserted after each reentrant call"), and §9.2
  line 994.
- **Problem.**
  - `value_operation_begin` already asserts `frame->ir_words == exec->words`
    before each operation (ceval.c:832, also 2105). The new check covers only
    "after".
  - "After each reentrant call" is not a site list that the generator can
    emit.
  - The check misses paths with no reentrant call in the resumed frame.
    When an inlined callee returns or yields, or an exception unwinds into
    the caller, the caller resumes from its own `instr_ptr`. If the copy had
    been replaced while the callee ran, the caller's handler never made a
    call, so no check would fire.
- **Change.** Put the check in three places:
  - Turn each deleted write and refresh into an equality assert at the same
    site: `ceval.c:1584–1587`, 1816, 2125 and 2172–2175. That is exactly
    where the old code would have repaired a moved PC.
  - Writer side: `_PyCode_ClearExec` (`ir.c:331–337`) asserts that it runs
    only from dealloc. The only writers of `_co_exec` are `codeobject.c:573`
    (initialisation), `ir.c:323` and `ir.c:337`.
  - In the debug tail-call build, check the invariant next to `frame ==
    tstate->current_frame` in `PRE_DISPATCH_GOTO`, which covers frame
    switches.
- **Also state:**
  - whether the `frame->instr_ptr = exec->words + current` restores at the
    same sites stay. Line 515 removes only the `ir_words` and
    `ir_feedback` refreshes;
  - that the instrumentation-version assert must allow an error being set.
    `instrument_all_executing_code_objects` stops at the first failure
    (`instrumentation.c:2040–2042`) and leaves later code objects stale;
  - that the invariant makes the JIT's `code->_co_exec == exec` re-checks
    dead (`jit_runtime.c:358–403`, `jit_record.c:84`). Delete them in 9b or
    A3b.

### R2-5. Minor. B5 removes a handler, so it renumbers

- **Where.** Line 50 ("renumbered in … B2, B4, B6"); B5, lines 774–787.
- **Evidence.** B5 implements G4 item 2, under which "the handler goes" for
  `PUSH_NULL` (register-ir.md G4). B5 changes the handler set, so it must
  renumber like B2, B4 and B6.
- **Change.** Add B5 to the list and to its step text.

### R2-6. Minor. A4 lands and may then be reverted

- **Where.** Landing order, line 405; A4, lines 684–688.
- **Problem.**
  - A4 lands before B2, but its decision needs B3's prototype, and B3 comes
    after B2.
  - "Otherwise B3 lands and A4 is reverted" means one form lands and is then
    replaced.
- **Change.** Build A4 and B3's prototype off-tree once A3 has landed,
  decide by G2's rule, and land only the winner. Open question 9 is counted
  first.

### R2-7. Minor. Smaller corrections

- **Line 69.** "replacing the triage's 2.6× on build F". The triage's 2.60 is
  the 29-kernel F/U geomean (triage line 241). On the same 12 kernels it was
  2.51 (review 1, finding 1). 2.42 is a newer build on a different harness,
  not a correction of 2.6.
- **B1, lines 719–721.** The allocator has its own "two live temporaries"
  `SystemError` (`regcfg.c:823`), besides the verifier's at 1279. Name both.
- **A3c, lines 658–670.** A3c removes the last fallback backend. Name every
  build configuration the project runs, so that each is known to build with
  `musttail`. A3a covers debug and `Py_STATS`.
- **Lines 172 and 611–614.** The free-threaded remarks have no consequence
  while free-threaded builds `#error`. Drop them or mark them as such.
- **C8, line 335.** "5.2× with A1, step 8 and D4" is the `vt` + `rep` variant,
  which also includes A2's `ni`.

## Estimate and order

- **The estimate.**
  - The per-step table sums to 97 (phase A 41, phase B 53, measurements 3).
  - It now matches review 1's basis for A3a, A3b, A4, B0a, B4, D4 and the
    dual-CI work.
  - R2-1 adds about 3–5 h to A3a. The typed borrow (R2-3) is not counted.
  - With both, about 100–115 h. This is a judgement.
- **The order** is sound:
  - A1 and A2 come after step 7, which edits their helpers and macros.
  - A5 comes after step 8, which makes construction canonical.
  - A3 comes after 9b, so no representation step runs on two backends.
  - The format steps come after A3, so each changes one dispatch form.
  - Two exceptions: the typed borrow should follow 9a (R2-3), and A4 should
    not land before its decision (R2-6).
- **One overlap is unstated.** B0a starts after step 6c and edits
  `assemble.c` and `specialize.c`. 9a reviews both for int32 assumptions
  (representation packet: `assemble.c` 4 sites, `specialize.c` 14). The
  edits are small, but the packet should name the overlap.

## Checked and found sound

- The dispositions marked "Resolved" above, at the cited lines.
- The tools claims (lines 22–26). A preprocessor check of `__has_attribute`:
  - `clang-21`: `preserve_none` and `musttail`;
  - `gcc` 15.2: `no_callee_saved_registers` and `musttail`.
- §0.2's executing-copy facts:
  - `_co_exec` has three writers: `codeobject.c:573` (NULL at creation),
    `ir.c:323` (created once, under the NULL test) and `ir.c:337` (cleared).
  - `value_operation_begin` asserts the identity that A2 relies on
    (ceval.c:832).
- Turning the refresh into an assert. `instrument_all_executing_code_objects`
  (`instrumentation.c:2031–2049`) instruments the code of every frame on
  every thread's stack, below `FRAME_OWNED_BY_INTERPRETER`, before
  `set_events` returns. The error path is the exception (R2-4).
- One record per activation, for nested evaluations, suspension and re-entry
  (R2-2, last item).
- The record's values are owned, so they agree with D4.
- B1's line numbers (`regcfg.c:790`, 820, 841, 883, 887, 962, 1279), and the
  existing knob `_PyIR_MaxForward` (`assemble.c:700`) that `_PyIR_MaxSlots`
  copies.
- §3's file table against the representation packet's steps 8, 9a and 9b.
- §2's corrected rows, including the register rule for state across C calls.
  Under `preserve_none`, r12–r15 carry the parameters and survive calls into
  ordinary C. Under `no_callee_saved_registers`, rbx and r12–r15 are free to
  hold them.
