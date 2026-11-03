## Summary

The family-wide barrier is a sound approach to the reported read-order bug.  
Two existing paths invalidate its exemptions: attribute specialization and sweep-time buffer release.  
This is a source review; runtime probes could not start because the read-only sandbox prevented `uv` from locking its shared cache.

## Blocking issues

1. **The specializer invariant is already false, and the reader check misses redispatch.**

   **Where:** “The reader side” and “Hand-offs,” [lines 92–104](/home/power/code/cpython/.worktrees/boil-compiler-copyprop/boil/design/work/compiler-copyprop-1.md:92).

   `_Py_Specialize_LoadAttr()` and `_Py_Specialize_StoreAttr()` reach [`specialize_dict_hint()`](/home/power/code/cpython/.worktrees/boil-compiler-copyprop/Python/specialize.c:790). It accepts an exact dictionary with general keys and calls `_PyDict_LookupIndexAndValue()`. That reaches [`compare_generic()`](/home/power/code/cpython/.worktrees/boil-compiler-copyprop/Objects/dictobject.c:879), which can invoke a key’s Python `__eq__`.

   Consequently, even `return x.a` can be unsafe: a colliding key in the original receiver’s dictionary rebinds the caller’s `x` during specialization; `DISPATCH_SAME_OPARG()` then reads the replacement receiver. Reading the field before the first callback does not help when redispatch reads it again. The proposed first-read check would accept this.

   **Fix:** establish the no-Python specializer contract now. At minimum, decline general-key dictionaries before the lookup in `specialize_dict_hint()`, and audit the other specializer paths. The reader rule must cover rereads after redispatch, not just first use. Classification enforcement cannot remain a future hand-off while the exemption depends on it.

   Add a cold-to-specialized `x.a` regression with a colliding key that redirects `x` to another receiver. Record the comparison and rebinding when they occur; assert the original receiver’s result. Remove the dictionary guard as the negative control. **This test would not establish operand lifetime across collection.**

2. **Allocation is not yet guaranteed to run without Python. S3 is not the only prerequisite.**

   **Where:** “Drain points,” [lines 74–85](/home/power/code/cpython/.worktrees/boil-compiler-copyprop/boil/design/work/compiler-copyprop-1.md:74).

   `no_python` suppresses queue draining, `gc.callbacks` and Python debug output. It does not suppress arbitrary callbacks from deallocation. [`picklebuf_dealloc()`](/home/power/code/cpython/.worktrees/boil-compiler-copyprop/Objects/picklebufobject.c:108) and [`unpackiter_dealloc()`](/home/power/code/cpython/.worktrees/boil-compiler-copyprop/Modules/_struct.c:2232) call [`PyBuffer_Release()`](/home/power/code/cpython/.worktrees/boil-compiler-copyprop/Objects/abstract.c:822), which invokes `bf_releasebuffer`; that can reach Python `__release_buffer__`.

   Thus an allocation-triggered sweep can run Python beneath an instruction classified as allocation-only. In debug builds, the sweep checks may abort instead. This is review 2 §5.3, already assigned to 10a rung 3 in PROCESS §0.3; fixing S3’s output path does not close it.

   **Fix:** name the buffer-release repair as an additional prerequisite for the allocation exemption, with a concrete closing commit and acceptance test. Correct the claim that the listed drain points are exhaustive.

   The collector owner’s test should retain the exporter, force collection inside a claimed allocation-only instruction, and record the executing instruction at release. Assert deferred, exactly-once release and unchanged operand selection. The control should restore sweep-time release. **This would not prove complete heap traversal or all native exporters’ release contracts.**

3. **The test plan does not yet prove the execution paths or the reader exception.**

   **Where:** “Tests,” [lines 159–200](/home/power/code/cpython/.worktrees/boil-compiler-copyprop/boil/design/work/compiler-copyprop-1.md:159).

   A correct numeric result alone can pass if rebinding never happened. Installing tracing or enabling monitoring events does not prove that the intended callback or specialized handler executed. Tests 10–11 can also agree with an incorrect shared classifier.

   **Fix:** require each semantic case to witness the mutation at the callback, identify the target frame/instruction, and assert both the saved-operand result and the changed local. Exercise generic and specialized forms with evidence captured during execution. For monitoring, register and witness the named callbacks.

   Add a heap-valued operand case: rebind its only local, collect, then inspect the original operand’s payload at the reader. Witness the collection and run a missing-root negative control under kill verification; otherwise conservative C-stack retention can hide the defect. **This test would not validate every handler or traversal edge.**

   The generator tests should independently cover every barrier clause, emitted C metadata, and redispatch. Test 12 must reject an unsafe reentrant specializer rather than encode the present exemption as expected behavior.

## Non-blocking issues

The planned tests have these specific limits; put them in the as-built record required by PROCESS §1.3:

| Test | Failure it would not detect |
|---|---|
| 1, call | Premature reclamation of a heap operand; `1` is inline. |
| 2, descriptor | A defect confined to `LOAD_ATTR_PROPERTY`, unless that execution form is witnessed. |
| 3, subscript | A defect confined to `BINARY_OP_SUBSCR_GETITEM`, unless witnessed. |
| 4, reflected operation | Incorrect lifetime through a `NotImplemented` fallback chain. |
| 5, comparison | The first proposed expression calls `__eq__`, not the stated `__lt__`; choose the second expression. |
| 6, globals | Reentrancy inside an immediate reader’s specializer. |
| 7, tracing | Missing instrumentation: ordinary operation callbacks still perform the mutation. |
| 8, monitoring | Missing PY_START/LINE delivery if those events are merely enabled. |
| 9, disassembly | Correct runtime operand selection or lifetime. |
| 10, synthetic families | Loss of the flag during C metadata emission or consumption. |
| 11, real instructions | An unrecognized Python call shared by the classifier and test oracle. |
| 12, reader ordering | A reread after reentrant specialization and redispatch. |

Other corrections:

- Test 9 should use something like `x == (a, b)` to test a fold *across* `BUILD_TUPLE`. `(x, 1)` cannot fold `x` into that window consumer; [`borrow_field()`](/home/power/code/cpython/.worktrees/boil-compiler-copyprop/Python/regcfg_passes.c:180) rejects window formats.
- The free-threaded paragraph describes a build removed by note 06 §3.4. Delete it; it supplies no applicable correctness argument.
- Publish the Lib-count script, exact command and matching before/after source identities. Count restored loads, IR words and frame slots; gate 4 instruction deltas alone do not explain those changes.
- The compatibility break needs the library documentation and ruling required by PROCESS §0.2. Adding only its ledger row at landing leaves that work unspecified.
- “Risks” says the last free flag bit is 16, while the proposal uses 128. Both are currently unused; state the allocation unambiguously.

## Questions

- What anchors a folded heap operand after a reader callback overwrites its local? Specify when the C callee anchors it and whether anything remains live after that callee returns. `IRStack.flush()` retaining a mutable local slot is insufficient by itself; the eval-loop C frame is precise.
- Does this rung wait for the allocation-without-Python prerequisites, or land with an explicitly bounded correctness claim? The current text promises the stronger result.
- Which rung and acceptance check close specializer classification enforcement? “After 10a rung 2b” names an ordering constraint, not a completion gate.

## Verdict

revise
---
Reviewer: codex, 2026-10-28 16:14 PDT
