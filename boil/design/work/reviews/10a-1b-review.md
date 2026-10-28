## Summary

The direction follows note 11, but the root-safety proof has gaps.  
Generated boxing and generator-return routing need concrete changes before promotion.  
Resolve the callback, metadata, and calling-convention contracts before deleting scans.

## Blocking issues

1. **Generated input boxing contains an unchecked safepoint.**  
   **Where:** §2; [`convert_object_inputs()` and `emit_box()`](/home/power/code/cpython/.worktrees/boil-10a/Tools/cases_generator/generators_common.py:295).

   The generator boxes inputs consecutively, then pushes their roots around calls in the handler body. The second `_PyValue_BoxInline()` can therefore collect the first box. Changing body calls from `escaping` to `collecting` does not cover these generated calls. The original inline slot preserves the number, not its fresh `PyObject *` box.

   **Fix:** Protect previously created boxes during every subsequent generated conversion, including failure cleanup. Add a two-box generator fixture and inject collection during the second conversion; check pointer survival and balanced root pops on failure. A body-call-only boxing test is insufficient.

2. **The early return store can target the wrong slot.**  
   **Where:** §1; `_RETURN_VALUE` and [`_PyEval_GeneratorReturnTarget()`](/home/power/code/cpython/.worktrees/boil-10a/Python/ceval.c:1906).

   `_PyEval_GeneratorReturnTarget()` changes `return_slot` for `FOR_ITER`: yielded values go to the loop target, but the generator’s final return value goes to the iterator window’s cleanup slot. Today this happens before `IR_STORE_RETURN`. The proposed early `_PyFrame_SetReturnValue(dying->previous, temp)` would precede that adjustment and can overwrite the loop variable.

   **Fix:** Select the final destination before storing, then unlink and clear. Specify the ordering separately for ordinary return, `SEND`, `FOR_ITER`, and interpreter entry. Add a warmed, specialized generator loop whose last yielded value differs from its final return value; assert the loop variable remains unchanged after exhaustion. Cover `yield from` and injected collection during cleanup too.

3. **Resume stack effects are required, not an implementation contingency.**  
   **Where:** §1’s “If the compiler … needs `res`”; [`generate_stack_effect_functions()`](/home/power/code/cpython/.worktrees/boil-10a/Tools/cases_generator/opcode_metadata_generator.py:94) and `Python/flowgraph.c`.

   Metadata currently derives pushed counts directly from declared outputs. Removing `YIELD_VALUE`’s output removes the value supplied on resume; removing `RETURN_GENERATOR`’s output changes the initial generator resume state. The `returns` flag currently supplies no replacement stack effect.

   **Fix:** State the required logical effects explicitly and name the shared metadata change that preserves them without generating a second runtime store. Check IR field layouts too: removing outputs can change destination/window encoding, requiring the pyc magic bump mandated by `PROCESS.md`. Add focused resume tests using the sent value in a larger expression, plus generator and coroutine creation tests.

4. **The claimed `_ALLOCATE_OBJECT` borrow contradicts the stated rule.**  
   **Where:** §2, “Weak fields and caches do not qualify” followed by “without code changes”; [`_ALLOCATE_OBJECT`](/home/power/code/cpython/.worktrees/boil-10a/Python/bytecodes.c:4321).

   `init_func` comes from `_spec_cache.init`. [`type_traverse()`](/home/power/code/cpython/.worktrees/boil-10a/Objects/typeobject.c:6836) does not visit that field. It therefore cannot qualify merely as a strong traced field of the anchored type. A valid type-version/lookup invariant may establish a separate path through the MRO and class dictionaries, but the document neither states nor checks it.

   **Fix:** Anchor `init_func` explicitly, or specify and enforce that separate invariant. Do not make caches generally qualify as borrows. Add a negative checker fixture for an untraced cache and a collection test involving inherited `__init__`.

5. **Allocator non-reentrancy remains undecided where the proof needs it.**  
   **Where:** §§2–3; `_PyObject_Init*`, [`new_reference()`](/home/power/code/cpython/.worktrees/boil-10a/Objects/object.c:2480), and the reference tracer callback.

   §2 relies on allocations preserving strong edges; §3 leaves whether the reference tracer may collect as a question. Its documented restriction forbids creating Python objects and changing exceptions, but does not establish an observational callback that cannot collect or mutate existing edges. Tracemalloc’s behavior does not establish the contract for every tracer. Simply spanning the allocation result also needs care: `_PyObject_InitVar()` invokes `_Py_NewReference()` before setting `ob_size`, and constructors initialize other fields later.

   **Fix:** Resolve this before assigning call effects. Either define and enforce a non-collecting, observational tracer contract, or treat the callback path as reentrant and specify safe object construction/publication. Test the supported callback behavior, including an attempted collection. Classify `_PyShape_InitRoot` separately: its current body only writes shape fields.

6. **Marking tail-call handlers does not communicate their calling convention to the walk.**  
   **Where:** §4; [`frame_class()`](/home/power/code/cpython/.worktrees/boil-10a/Python/gc_walk.c:775) and the register ownership loop.

   Section-based classification returns precise with `flags == 0`; `_PyGC_RANGE_PRESERVES_NOTHING` currently comes from registered ranges. Generated handlers use `preserve_none`, but adding `Py_GC_PRECISE` supplies no convention information. A clang-21 assembly probe confirmed that a handler can keep its argument in r12 across a C call without emitting save or undefined CFI rules. The walk would carry that pending value outward instead of assigning it to the handler.

   **Fix:** Supply handler convention information through the walk’s descriptor/range mechanism. Verify ownership on compiled handler-to-C stacks, including generated label handlers. If tail-call work is cut, retain conservative scanning for those handlers until this contract is implemented; a successful build alone does not prove it.

## Non-blocking issues

- **Scope ownership conflicts.** `10a-2.md` assigns `_PyStack_UnpackDict` and the dict-call forms to 1b; this document’s fixed scope omits them. Assign them explicitly to 1b or rung 2’s remainder and update both documents.

- **Complete the entry-path inventory.** §3 omits `collect_from_entry()` itself. Its capture starts the walk in that function, so its frame is scanned today. Name its promotion, and cover `collect_and_run_queue()` whether inlined or outlined.

- **Specify quarantine record lifetime.** §6 requires provenance-bearing traps, but `_PyGC_Quarantine()` currently retains only a block address and collection number; kill mode does not persist the retain-mode record list. Preserve diagnostic records through quarantine expiry and test a delayed use after another collection.

- **Correct the transition-test timing.** §1 says the inline full scan masks failures until item 5. Section classification precedes the activation override, so marking the eval loop in item 4 already disables that protection. Run the relevant negative controls at that boundary.

- **Make measurements reproducible.** §8 should specify repetitions, workload sizes, and instruction counts per operation. Separate generator-root overhead from allocator-span overhead.

## Questions

- Which source owns the authoritative call-effect classifications shared by the analyzer and C mode?
- What exact tests comprise the accepted stress failures? “Finding B” is undefined here, and `10a-2.md` also names the writer failure owned by rung 3.
- If ASan is cut despite the earlier working build, which field/interior-pointer detection claims remain unverified, and who owns completing them?

## Verdict

revise
---
Reviewer: codex, 2026-10-27 23:45 PDT
