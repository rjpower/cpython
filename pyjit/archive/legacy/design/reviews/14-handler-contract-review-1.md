## Summary

The root map needs a precise meaning at every collection point before clearing can be removed.  
Rules 1–3 conflict with existing frame, generator, and canonical-value invariants.  
This is a source review; the runtime probe could not start because `uv` could not lock its read-only cache.

## Blocking issues

1. **Instruction liveness does not describe all roots inside a handler.**

   **Where:** §2 Rule 2; [Python/ceval.c:2193](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Python/ceval.c:2193), `Tools/cases_generator/ir_generator.py`, `root_check.py`.

   **Problem:** `_PyEval_UnpackIterableValues()` writes fresh objects into output slots, then calls the iterator again. Those outputs are absent from the instruction’s live-in set. Adding live-out slots instead would scan outputs before they are initialised. Similar state changes occur when CALL replaces its window and when `IR_RETURN_EARLY` publishes a result before collecting cleanup. The existing checker treats residence in frame slots as anchoring; that assumption becomes conditional on map coverage.

   **Fix:** Define the root state at each collecting call, including intermediate results and frame transitions. Use the existing span mechanism for dynamically initialised output prefixes, or specify another complete publication rule. Make the generator and checkers enforce membership in the roots actually visited.

   **Proof:** Collect during an unpack iterator’s second `__next__`, after the first result was stored. Record its slot/span coverage and other root provenance at collection; omitting that coverage must fail. This would not test CALL or return transitions, which need their own injected events.

2. **Safepoint-only entries omit suspension and instrumentation states.**

   **Where:** §2 Rules 1–2, §6 question 2; [Python/bytecodes.c:1673](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Python/bytecodes.c:1673), `Python/instrumentation.c`, `Objects/genobject.c`.

   **Problem:** Temporaries survive yields. `for x in make_iter(): yield x` retains the iterator; `make_object() + (yield None)` retains the left operand. `_YIELD_VALUE` also increments `frame->instr_ptr` and later resumes from it: omitting its entry store because it makes no collecting C call can corrupt control flow. Separately, `INSTRUMENTED_INSTRUCTION` can collect before an otherwise non-collecting word for which the compiler emitted no entry.

   **Fix:** Make suspension and frame handoff explicit position-publication points. Define maps for created, suspended, resumed, and dynamically instrumented frames, including the yielded slot versus the not-yet-written send result. Delete §6’s empty-yield premise.

   **Proof:** Collect while each example is suspended, then resume and use the retained object; record the suspended map and roots at collection. Omit the relevant bit as a negative control. These probes would not cover `throw()`, `close()`, `yield from`, or coroutine suspension. A separate INSTRUCTION callback must collect at a normally non-collecting word; that would not prove ordinary execution coverage.

3. **Removing initialisation invalidates consumers beyond the collector reader.**

   **Where:** §2 Rule 2; [Python/frame.c:111](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Python/frame.c:111), `Include/internal/pycore_interpframe.h`, `Python/gc_live.c`.

   **Problem:** `_PyFrame_CheckValues()` reads every slot and can call `Py_TYPE` on its contents. `_PyFrame_Copy()` copies every slot. `_PyGC_VisitFrameStack()` visits every slot, including through generator/frame-object traversal. `IR_STORE_TEMP`, `IR_STORE_DST`, `_PyFrame_SetReturnValue`, and return cleanup assert NULL-based invariants. Dead slots can contain either uninitialised bits or pointers to objects legitimately freed by an earlier collection. Changing only the live collector’s reader leaves crashes and false verifier traps.

   **Fix:** Inventory and convert all slot readers, visitors, copies, and assertions before removing initialisation. Cover frames during argument binding, ownership transfer, exception unwinding, and after completion. Specify which slots contain valid values in each state.

   **Proof:** At an injected collection, poison excluded slots and record that no reader visits them; exercise debug checking and ownership transfer too. This detects forbidden reads, but does not establish that every included slot contains the correct live value.

4. **The stated A/B oracle confuses intended reclamation with missing roots.**

   **Where:** §2 Rule 2’s verification paragraph and §5; [Python/gc_trace.c:2788](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Python/gc_trace.c:2788), note 11 §8.3.

   **Problem:** B reaching an object that A excludes is expected for a correctly dead temporary. Kill mode traps a subsequent invalid use, not the difference itself. Moreover, “mark every slot” cannot precisely traverse uninitialised or dangling values. Existing B gathers skipped conservative words; it does not automatically provide provenance for map-excluded interpreter slots. An opaque C-stack copy can also keep a missed live object in A, masking the defect under kill mode.

   **Fix:** Extend B explicitly to excluded slot words using safe conservative lookup, with code, word/state, slot, and collection provenance. State its limits. Rewrite §5 around event evidence:

   - **Missing-root probe:** establish at collection that the target has no alternate root, then use it afterwards. Removing its map bit must trap. This does not detect excessive retention.
   - **Retention probe:** record whether the target was marked and from which root at that collection; separately record finalizer execution at the queue drain. Marking its dead slot live must change the result. This does not detect omitted live roots.

   Finalizer timing alone cannot distinguish a bad map from conservative retention or delayed callback delivery.

5. **Rule 3 preserves a representation forbidden by the value contract.**

   **Where:** §2 Rule 3; [Include/pyvalue.h](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Include/pyvalue.h), `Include/internal/pycore_value.h`, note 06 §18.1.

   **Problem:** Every exact int fitting int32 is canonicalised inline by `PyValue_FromObject()`. A one-digit exact heap int therefore cannot legitimately appear as a handler operand. Its measured share should be zero by invariant. Retaining compact and inline-only handler forms would preserve a representation leak and consume opcode space that step 8 already needs to free.

   **Fix:** Establish canonical operands at every ingress and simplify the existing integer guards/extractors accordingly. If a legitimate exception exists, name its producer and reconcile the canonical-value contract first. Do not decide representation validity using a one-percent threshold.

   **Proof:** Check the actual operand encoding when C-returned integers enter handlers; deliberately bypass canonicalisation as the negative control. This detects ingress leaks, not arithmetic overflow, bool/subclass handling, or large-int fallback.

6. **Rule 1 conflates frame access, position publication, and observation.**

   **Where:** §2 Rule 1; [Python/ceval_macros.h:92](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Python/ceval_macros.h:92), `Tools/cases_generator/ir_generator.py`, `Python/traceback.c`, `Modules/faulthandler.c`.

   **Problem:** Tail-call parameters contain no frame pointer. Non-collecting handlers still need `frame`: LOAD_CONST accesses code constants; jumps access `ir_words`; feedback accesses `ir_feedback`. Their load cannot disappear solely because they lack safepoints.

   The deopt explanation also overlooks computed-goto `PREDICTED_` labels, which occur after the generic handler’s position store. Finally, asynchronous traceback dumping reads `instr_ptr` outside eval-breaker execution. Python signal callbacks and native signal handlers are different observers.

   **Fix:** Generate frame loads from actual frame dependencies, independently of position stores. Define whether effects apply to concrete expanded handlers or whole families, and cover every alternate entry. Audit asynchronous readers and explicitly decide whether stale diagnostic positions are acceptable.

   **Proof:** Inject collection after a non-collecting specialisation deopts; record and assert the exact word/map key, on computed-goto and tail-call builds. Suppressing publication must fail. Line-only traceback assertions would miss wrong words on the same line; this probe would still not cover asynchronous sampling.

7. **The table’s lifetime and construction contract is missing.**

   **Where:** §2 Rule 2 and §4; [Objects/codeobject.c:346](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Objects/codeobject.c:346), `Python/assemble.c`, `Python/marshal.c`, `Tools/build/deepfreeze.py`.

   **Problem:** A compiler-emitted table must also survive marshal, frozen-code generation, C code constructors, and `code.replace()`. Replacing words or frame size while retaining an old map can turn valid slots into omitted roots or out-of-bounds reads. Encoding is not the only undecided detail.

   **Fix:** Make the map part of the code object’s construction and validation contract. Specify preservation, regeneration, and rejection rules, and bump the pyc magic as PROCESS §3 requires. Define behaviour for missing or inconsistent maps without introducing an all-slots fallback over uninitialised storage.

   **Proof:** Exercise marshal/frozen-code round trips and replacements that change words or slot counts; collect during execution and assert map coverage at the event. Round-trip equality alone would preserve an identically wrong map, so it needs the negative controls above.

## Non-blocking issues

- **Rule 5’s default is unclear.** `configure.ac` defaults `with_frame_pointers=yes`; `CFLAGS_CEVAL` follows core flags, and `_PyEval_EvalFrameDefault` has a GCC attribute forcing frame pointers. Specify precedence for defaults, user flags, sanitizers, and profiling. Verify emitted code actually omits them and that compiled-stack tests attribute expected opaque roots correctly. Zero fallbacks alone would miss incorrect ownership; those tests still cover only exercised layouts.

- **The liveness description overstates reuse.** Note 07 §5 says compaction uses LIFO holding, not liveness. Name how virtual-register sets become final physical-slot sets after compaction and assembly. Record table bytes, lookup cost, compile time, root-phase time, pauses, and RSS alongside handler instructions. The present micro/kernel measurements cannot show whether cheaper dispatch creates more expensive collection.

- **Rule 4’s `gc.get_objects()` rationale is incorrect.** `PyRangeIter_Type` is untracked, and `Python/gc_live.c` filters that snapshot to tracked GC objects. Deferring iterator redesign is reasonable; justify observable iterator state through actual aliases and operations.

## Questions

- What is the map-size limit, and what happens when a valid function exceeds it?
- Does H commit only the interpreter/tier-0 schema, or also tier 1’s locations for spills, virtual frames, constants, and deoptimisation?
- Which clearing operations remain for semantics or root publication, and exactly which retire in rung 2? The current deletion list does not distinguish them.

## Verdict

revise
---
Reviewer: codex, 2026-10-28 17:15 PDT
