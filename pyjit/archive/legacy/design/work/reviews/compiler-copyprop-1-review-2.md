## Summary

The family-wide flag fits the single-IR design and addresses intervening reentrancy.  
The specializer exemption remains unproved, and the reader analysis confuses stack effects with actual reads.  
This is a source review; runtime probes could not start because `uv` could not lock its shared cache in the read-only sandbox.

## Blocking issues

1. **The specializer contract still excludes a known reentrant path without establishing its safety.**

   **Where:** “The rule,” specializer contract; [Python/specialize.c](/home/power/code/cpython/.worktrees/boil-compiler-copyprop/Python/specialize.c:662); [Objects/typeobject.c](/home/power/code/cpython/.worktrees/boil-compiler-copyprop/Objects/typeobject.c:5886).

   `lookup_hook_facts()`, the shape-plan lookups, class attribute specialization and binary subscript specialization reach type lookup. A cache miss reaches `find_name_in_mro()`, then general-key dictionary comparison and Python `__eq__`. In `lookup_plan_facts()`, `mro_dicts_unicode()` is checked **after** that lookup. It prevents caching an unsafe result; it does not prevent the callback.

   The unsuccessful probes do not establish an invariant that prevents these misses during specialization. Consequently, the proposed instance-dictionary guard does not justify the blanket `specializing` exemption. Marking the family as running code would also leave the immediate-reader redispatch problem intact.

   **Fix:** establish a non-reentrant lookup path before granting the exemption—for example, check every relevant MRO dictionary before lookup, including metaclass and hook lookups, and decline specialization when the check fails. Include the transitive specializer checker in the exemption’s acceptance gate, with an owner and closing condition for the MRO path.

   Add a fixture that forces a miss at specialization, witnesses the comparison at that phase, and asserts the original receiver’s result. Removing the guard must fail it. This would not establish operand lifetime across collection.

2. **The reader-side analysis does not model when the generated handler actually loads an input.**

   **Where:** “The reader side”; [inputs_read_after_code()](/home/power/code/cpython/.worktrees/boil-compiler-copyprop/Tools/cases_generator/ir_generator.py:423), compared with [Stack.pop()](/home/power/code/cpython/.worktrees/boil-compiler-copyprop/Tools/cases_generator/stack.py:251).

   A uop can declare `(value -- value)` without using `value` in its body. `Stack.pop()` then emits no load. If that uop runs Python and a later uop consumes `value`, the field is first read afterward.

   `inputs_read_after_code()` nevertheless treats the first stack-effect occurrence as the read. When the later uop arrives, the external input is already on its simulated stack, so the function never marks it late. `classify_inputs()` can subsequently classify the consuming use as borrowable.

   **Fix:** track whether each external input has actually been materialized, following the emitter’s `used` and pass-through rules. An unused peek must not settle its read time.

   Extend test 14 with an unused pass-through uop that runs code, followed by a consuming uop. Assert that the input loses its borrow bit and check the emitted load/call order. Restoring the current analysis must fail the test. This would not detect reentrant redispatch or missing GC roots.

3. **Test 10 cannot serve as the promised missing-anchor oracle.**

   **Where:** “Tests,” item 10 and “Hand-offs”; [IRStack.flush()](/home/power/code/cpython/.worktrees/boil-compiler-copyprop/Tools/cases_generator/ir_generator.py:1128); [Python/gc_walk.c](/home/power/code/cpython/.worktrees/boil-compiler-copyprop/Python/gc_walk.c:1).

   Collection occurs inside `X.__add__`, while the original operand remains a precise Python-frame root through `self`. Opaque C frames in the binary-operation chain can retain it afterward. Ordinary kill verification preserves roots from those opaque frames; enabling kill does not remove this protection.

   Therefore the test can pass with the proposed reader anchor absent. It also does not witness a collection during the interval after the callback frame disappears and before the reader’s last use.

   **Fix:** specify the lifetime obligation: which frame or span anchors the saved operand, from mutation of the local through its last use. Keep test 10 as a semantic regression, but replace its hand-off acceptance oracle with collection injection where the operand is held only by the precise activation whose root is being tested. Record that collection at the event and demonstrate failure with that exact root omitted.

   At the C API boundary, distinguish a canonical `PyValue` root from a raw numeric box root: canonicalizing a box can discard its storage identity. This fixture would prove the selected boundary, not every reader or compatibility conversion.

## Non-blocking issues

- **Witness execution forms.** “100 calls, past specialisation” does not establish that `LOAD_ATTR_PROPERTY` or `BINARY_OP_SUBSCR_GETITEM` executed. Capture the target code, offset and executing handler during the mutation event. Register and witness the named PY_START and LINE callbacks; enabling those events alone supplies no evidence.
- **Avoid a masking barrier in test 8.** In `x + len(s)`, `LOAD_GLOBAL len` already kills the pending load. Bind `len` to a local before the expression, then remove the CALL family’s barrier as a negative control.
- **Correct the measurement’s attribution claim.** [fold_count.py](/home/power/code/cpython/.worktrees/boil-compiler-copyprop/boil/tools/measure/fold_count.py:48) scans final physical slots without region boundaries, local-write kills or borrow-field eligibility. It also checks the reader before the line-change kill. Its aggregate counts are useful; its stopping-op split is not the pass’s rejection history. Record rejection reasons at the compiler decision, or label this split as a lexical diagnostic. Include an immediate before/after benchmark comparison alongside the historical `6d.json` comparison.
- **Reconcile the ledger and documentation.** The checked `PROCESS.md` lacks the claimed specializer-checker and operand-anchor rows, and the allocation row lacks the added “no Python under allocation-only ops” clause. The documented collector bound is an explicit ruling; I am not reopening it. Record its actual acceptance condition, and make the two library-reference edits required landing work once ownership is approved.

The proposed tests retain these limits:

| Test | Failure it would not detect |
|---|---|
| 1: call | Reclamation of a heap operand; the saved integer is inline. |
| 2: property/getattr | A defect confined to a specialized member unless that member is witnessed. |
| 3: subscript | A defect confined to a C mapping slot or an unwitnessed specialized member. |
| 4: reflected operation | Loss of an operand across a collecting `NotImplemented` fallback chain. |
| 5: comparison | Incorrect cleanup or operand preservation when the comparison raises. |
| 6: globals | Reentrancy inside an immediate reader’s specializer. |
| 7: tracing | Missing property tracing events when only the rebinding function’s call event is asserted. |
| 8: monitoring | Missing CALL-barrier handling masked by LOAD_GLOBAL; missing unwitnessed PY_START/LINE delivery. |
| 9: specializer | MRO-cache-miss and STORE_ATTR paths; comparison “while f ran” does not identify specialization. |
| 10: heap operand | A missing reader anchor while `self` or opaque C frames retain the operand. |
| 11: disassembly | Incorrect runtime selection, collection safety or callback delivery. |
| 12: synthetic families | Incorrect native-callee effect classifications. Add independent return/yield and generated-form cases. |
| 13: real handlers | An omission shared with `Properties.escapes` or hidden inside a macro. |
| 14: reader ordering | Unused pass-through inputs as written; after repair, redispatch and lifetime defects remain outside its scope. |

The allocation-bound acceptance test must witness the executing instruction at release, prove deferred exactly-once delivery, and fail when sweep-time release is restored. It would not prove every native exporter’s release contract.

## Questions

- Who owns the MRO specialization path, and which landing gate closes its no-Python contract?
- Does 10a’s operand-root work guarantee callee anchoring throughout the reader, caller anchoring between calls, or both?
- Are the required library documentation changes approved, and will the missing ledger entries be present before landing?

## Verdict

revise
---
Reviewer: codex, 2026-10-28 16:31 PDT
