## Summary

The in-place slot conversion and monitored-result root fit the architecture.  
Two paths still retain internal boxing without a clear retirement plan.  
This assessment is based on the proposal, context documents, and source inspection.

## Blocking issues

1. **§E’s limited edit leaves callable boxing before value dispatch.**

   **Where:** §§B/E; [`_MAKE_CALLARGS_A_TUPLE` and `_DO_CALL_FUNCTION_EX`]( /home/power/code/cpython/.worktrees/boil-6d/Python/bytecodes.c:5145); [generated handler](/home/power/code/cpython/.worktrees/boil-6d/Python/generated_ir_cases.c.h:5264).

   **Problem:** Changing the three calls does not remove the existing `PyValue_ObjectBorrow(func)` conversions. The generator treats these as object inputs and inserts boxes, including in `_MAKE_CALLARGS_A_TUPLE` before it checks whether argument conversion is needed. Thus an inline callable still allocates before §B rejects it; allocation failure can produce `MemoryError` first. The generated code protects these boxes, so this is not an unchecked-pointer crash.

   **Fix:** Extend §E to remove unnecessary callable object inputs. `_Py_Check_ArgsIterable` does not use its callable parameter; remove that parameter and update its callers. Use value predicates for callable type checks. Confine any boxes required by monitoring to that branch and root them raw. Test inline int and float callees through cold and warmed starred calls, with zero callable boxes on the unmonitored path.

2. **The `lru_cache_call` recipe preserves an avoidable object-result layer.**

   **Where:** §D, Tests 9 and Measurements; [`uncached_lru_cache_wrapper`](/home/power/code/cpython/.worktrees/boil-6d/Modules/_functoolsmodule.c:1284), [`infinite_lru_cache_wrapper`](/home/power/code/cpython/.worktrees/boil-6d/Modules/_functoolsmodule.c:1296), and [`lru_cache_call`](/home/power/code/cpython/.worktrees/boil-6d/Modules/_functoolsmodule.c:1731).

   **Problem:** `PyValue_FromResult(self->wrapper(...))` canonicalises after boxing. With `maxsize=0`, the helper calls `PyObject_Call` and boxes every inline result despite storing nothing. With `maxsize=None`, misses box through `PyObject_Call`, and hits box through the object dictionary accessor. The bounded cache’s native `link->result` field explains a kept box; these other paths have no such requirement. Neither appears in the residue list.

   **Fix:** Convert the internal wrapper interface and uncached/unbounded paths to values, boxing only where a native field requires it. Alternatively, explicitly retain these paths as measured residues with a named retirement step. Add separate allocation and box budgets for `maxsize=0`, unbounded hits/misses, and bounded hits/misses.

## Non-blocking issues

- **Test 3 contradicts the chosen API.** `PyVectorcall_Call` retains `PyObject *callable`; it cannot accept an inline `PyValue`. Test inline rejection through `PyObject_CallValue` and `PyObject_VectorcallValue`. Test `PyVectorcall_Call` with a real vectorcall-capable object and verify its value result.

- **The receiver allocation-failure test needs a reliable trigger.** Tests 4 proposes `set_nomemory`, but `6d-2.md`’s round-4 record already explains that number freelists can bypass it. Specify how the new test reaches `_PyValue_BoxInline`’s failure branch, or record inspection as the validation limit.

- **§H underspecifies length-hint semantics.** In [Objects/abstract.c](/home/power/code/cpython/.worktrees/boil-6d/Objects/abstract.c:101), preserve acceptance of bools and int subclasses, `NotImplemented`, suppressed call-time `TypeError`, negative-result errors, and `Py_ssize_t` overflow. Name these cases and add `test_iterlen` to the targeted ladder.

- **Measurements need allocation counts and merge attribution.** Zero conversion boxes can conceal number objects created immediately before canonicalisation. Record allocations for the new call/wrapper rows, and compare against a baseline containing the same 10a changes; rung 2 already demonstrated that those merges materially affect instruction counts.

## Questions

- Is `ga_call`’s numeric-result box intentionally retained? [Its call and `set_orig_class` helper](/home/power/code/cpython/.worktrees/boil-6d/Objects/genericaliasobject.c:648) still require an object result, as does `ga_vectorcall`. Specify a shared value implementation or name the residue’s owner.

- Which 10a merge supplies the descriptor-call spans and check classification required by the final tree? Record that dependency and the resulting frame classes before claiming the lifetime tests prove the precise paths.

## Verdict

revise
---
Reviewer: codex, 2026-10-28 04:49 PDT
