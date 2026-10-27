## Summary

The frame-class split and shared JIT safepoint table give the design a clear direction.  
Removing the scan backstops exposes concrete gaps in frame transitions and PyMem lifetimes.  
The root checker, quarantine verifier, and unwind contract need revision before implementation.

## Blocking issues

1. **Frames outside the chain still contain live roots.**  
   **Where:** §§4.3, 7; [Python/ceval.c](/home/power/code/cpython/.worktrees/boil-10a/Python/ceval.c:1943), [Python/bytecodes.c](/home/power/code/cpython/.worktrees/boil-10a/Python/bytecodes.c:1419), and `Python/frame.c::take_ownership()`.

   `_PyEvalFramePushAndInit()` initializes a frame before publishing it on `current_frame`. `initialize_locals()` stores newly allocated `**kwargs` and `*args` objects in those unlinked slots, then makes further collecting calls. Neither the frame chain nor a precise C-frame scan protects them.

   Returning has the inverse problem: `_RETURN_VALUE` consumes the result’s input slot, unlinks the frame, and holds the result in `temp` across `_PyEval_FrameClearAndPop()`. That path can allocate through `take_ownership()` when it materializes `f_back`. Visiting `entry.stack[0]` does not help: the return value has not reached it yet.

   **Fix:** Root initialized construction slots until publication, and root the return value from input consumption through its store into the caller. Specify these transitions explicitly. Add collections forced during argument initialization and during a return with retained frame objects.

2. **“Borrowed from an anchored value” is insufficient across reentrant calls.**  
   **Where:** decision 3, §§2, 4.2; [root_check.py’s borrowing rules](/home/power/code/cpython/.worktrees/boil-10a/Tools/cases_generator/root_check.py:385).

   A rooted list does not keep a previously borrowed element alive after a callback clears the list. The same applies to mutable fields and caches. `FT_ATOMIC_LOAD_PTR_ACQUIRE` establishes ordering; it does not establish ownership or continued reachability. The proposed general anchoring rule is therefore stronger than the demonstrated `_ALLOCATE_OBJECT` case supports.

   **Fix:** Permit borrowed anchoring only when the owning edge is guaranteed to remain intact throughout the call. Otherwise root the borrowed object itself. State who protects arguments for the entire callee execution, including arguments the caller never uses again. Add a callback that removes the borrowed reference, collects, and returns to its user.

3. **The proposed C checker inherits assumptions that are false for general C.**  
   **Where:** §2; [root_check.py::find_declarations()](/home/power/code/cpython/.worktrees/boil-10a/Tools/cases_generator/root_check.py:208), `anchored_expr()`, and `analyzer.py::NON_ESCAPING_FUNCTIONS`.

   Today every `PyValue *` local is assumed to point into rooted slots, and declaring `_PyValueRoot` is enough to classify its contents as held. Neither proves that an array is registered or that a root remains pushed. Dereferencing an arbitrary pointer is also accepted as anchored. These assumptions would accept precisely the unregistered arrays and prematurely popped roots that C mode must reject.

   Adding `switch` alternatives does not address C parameters, out-parameters, cleanup gotos, fallthrough, or mutation of the presumed anchor. The existing non-escaping list also contains allocating functions; it is not a non-collecting list.

   **Fix:** Define C mode’s supported subset and reject unsupported constructs. Track root registration and lifetime, pointer provenance, and anchor invalidation. Separate “cannot run Python” from “cannot collect.” Require negative tests for unpushed/popped roots, PyMem arrays, overwritten anchors, parameters, and cleanup paths before promoting functions.

4. **The PyMem conversion lacks a complete inventory and span-lifetime rules.**  
   **Where:** §§6.2, 7, rung 3; [Objects/listobject.c](/home/power/code/cpython/.worktrees/boil-10a/Objects/listobject.c:3022), [Python/ceval.c](/home/power/code/cpython/.worktrees/boil-10a/Python/ceval.c:1011).

   `_PyList_AsTupleAndClear()` is an additional concrete site: it sets the list size to zero before `_PyTuple_FromValues()` allocates. Its elements then survive through the PyMem scan, which the proposal removes for this opaque core frame. Rooting the list alone is insufficient.

   A span also cannot indiscriminately cover an array’s capacity. `_PyObjectArray_FromValues()` initializes elements while boxing can collect; sorting allocates replacement merge storage and maintains separate key/value ranges. Reading uninitialized entries as precise pointers can fail; retaining a freed span base can read freed memory.

   **Fix:** Add the list-to-tuple site—preferably clear the list after copying—and specify initialized lengths, buffer replacement, ownership transfer, and failure cleanup for every span. Preserve raw box pointers rather than canonicalizing them into inline values. Section 7 must also allow opaque functions to retain spans: after disabling their PyMem scan, they cannot simply “drop the push.”

5. **The quarantine verifier does not detect every use and can change the graph it verifies.**  
   **Where:** §8.3; [Python/gc_trace.c](/home/power/code/cpython/.worktrees/boil-10a/Python/gc_trace.c:1525), `Python/gc_live.c`, and note 09 §2.

   Replacing a shape/type does not trap direct structure-field or cached-buffer reads. Even a `Py_TYPE` comparison merely observes the replacement type unless an explicit check is added. A missed root can therefore be used without invoking a trapping slot.

   Header replacement also changes subsequent traversal, heap classification, and deallocation. The document does not explain how original referents and shapes remain protected, or how quarantine interacts with weak references, finalizers, and destroy hooks. Keeping the original header in a side table does not by itself solve those interactions.

   **Fix:** Separate access detection from retention accounting. Specify original-metadata traversal and referent retention, logical death and callback handling, and instrumentation for direct reads. Before using “zero missed roots” as proof, demonstrate intentional missed roots detected through a field read, a cached interior pointer, a slot call, and finalization.

6. **The walker needs a coherent, persistent starting context.**  
   **Where:** §3; [capture functions](/home/power/code/cpython/.worktrees/boil-10a/Python/gc_trace.c:965), `Python/ceval_gil.c::PyEval_SaveThread()`, and the detach path.

   Adding the caller’s PC and CFA to the existing capture is insufficient. The captured registers are taken inside the capture helper; its prologue has already changed `rbp`. A caller PC combined with that register set describes two different frames. On detached threads, the capture helper and `PyEval_SaveThread()` can return before another thread walks them.

   The document also leaves return-address normalization unspecified. Looking up the address after a call can select the next unwind row or even the next function at a boundary; DWARF describes this distinction in [§6.4.4](https://dwarfstd.org/doc/Dwarf3.pdf).

   **Fix:** Define exactly which frame the snapshot represents, recover its complete register context before transient frames return, and specify call-site PC lookup. Bound and validate CFA progression and save-slot reads before trusting them. Test actual compiled mixed stacks, each saved register, detached threads, and `preserve_none` handlers; a synthetic ownership test alone does not exercise compiler unwind metadata.

7. **Load address is not a descriptor-cache generation.**  
   **Where:** §3, “Descriptors.”

   A DSO can unload and another mapping can reuse its address. The proposed load-address check then accepts an old descriptor for different code. Wrong CFA/save-slot rules can omit roots. Cached pointers into unloaded unwind data are also invalid; [glibc explicitly requires callers to ensure that unwind data remains valid](https://sourceware.org/glibc/manual/latest/html_node/Dynamic-Linker-Introspection.html).

   **Fix:** Invalidate using a loader-change generation or rebuild the relevant cache when mappings change. Give JIT descriptors equivalent unregister/invalidation rules. Add an unload/reload test with address reuse and different frame layouts.

## Non-blocking issues

- **The narrowed scope needs reconciliation.** Note 06 §6 and note 08 §6 promise conservative scanning only for extensions. Decision 4 retains most core frames. Update the adopted plan and retirement claims explicitly. Decision 5 is also presented as settled while §13.1 still asks whether to adopt it.
- **Tier 1’s root contract is incomplete (§5).** Maps must cover live compiler temporaries and owned object constants as well as interpreter slots. Specify whether mapped locations replace ordinary frame-slot scanning, how stale physical slots are handled, and how code constants remain reachable.
- **Entry frames require a special visitor (§4.3).** Their executable is `None`, and several header fields are deliberately invalid in debug builds. Visit the designated return slot explicitly; do not route them through ordinary frame traversal and slot-count calculation.
- **Measurements need cold and worst-case costs (§9).** Record descriptor-cache memory, cold decoding, deep C stacks, detached-thread counts, and pause distributions. The warm-step estimate does not establish those costs.

## Questions

- Is extension identity deliberately determined by linkage? The same stdlib module receives different PyMem protection when built in versus shared.
- What prevents an unchecked opaque helper from being automatically inlined into a precise function, including under LTO?
- Which failed verifier or ownership checks prohibit a rung from landing at the hard stop?
- Will sensitive root transitions receive deterministic collection injection? Allocation intervals of 1,000 and 100,000 do not ensure those transitions are exercised.

## Verdict

revise