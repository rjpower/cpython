## Summary

Caller-owned spans and deleting the temporary keyword-call wrapper fit the architecture.  
Root ownership, checker contracts, and the regression proof need revision.  
This is a source review; `uv run` was blocked by the read-only cache.

## Blocking issues

1. **Zero checker errors do not justify adding only the precise mark.**  
   **Where:** §2 and Risks; [Python/ceval.c:703](/home/power/code/cpython/.worktrees/boil-10a/Python/ceval.c:703). `_PyEval_CallExitWithException` builds a local argument array containing the traceback. Its existing comment explicitly relies on that array being conservatively scanned. Marking the function precise removes this protection. A C `__exit__` can clear the exception’s traceback, collect, then read the traceback argument. Rule 1’s parameter liveness does not cover this creator-side obligation.

   **Fix:** Inventory every argument array built by the promoted helpers and slots, including zero-error functions. Put these arrays in spans before dispatch. Add a precise C consumer that mutates the original owner and collects before reading the argument. Correct the claim that group a needs only marks.

2. **The unpacking contract omits positional-array owners before the first allocation.**  
   **Where:** §1, lines 113–122; [Objects/call.c:399](/home/power/code/cpython/.worktrees/boil-10a/Objects/call.c:399) and [Python/ceval.c:1976](/home/power/code/cpython/.worktrees/boil-10a/Python/ceval.c:1976). The proposed prefix names callable and `kwargs`, but `_PyStack_UnpackDict` allocates `kwnames` before copying positional arguments. `_PyVectorcall_Call` must keep its source tuple alive during that allocation. `_PyEvalFramePushAndInit_Ex` similarly needs `callargs`, plus `func` and `locals` used afterwards. An interior array pointer in a precise frame provides no conservative root.

   **Fix:** Specify each caller’s prefix explicitly. Include the tuple owner before unpacking, and distinguish raw object anchors from canonical values. Inject collection at the names-tuple allocation with the source tuple otherwise unrooted.

3. **0h does not define a sufficient interprocedural proof.**  
   **Where:** §1, “The C mode”; [root_check_c.py:2363](/home/power/code/cpython/.worktrees/boil-10a/Tools/cases_generator/root_check_c.py:2363). A `_PyValueRoot *` parameter and a matching size-macro name do not establish how many slots a callee initializes, what it guarantees on failure, or whether the macro arguments match the actual fill. The current checker explicitly does not track indices. The proposed rule also needs to accept the fixed small array guarded by a size comparison, although that array is not itself dimensioned by the callee’s macro.

   **Fix:** Define checked effects for the named fillers: required remaining capacity, initialized suffix on success, failure state, and returned-pointer provenance. Specify how push and rebase receive actual capacity; their current interfaces do not. Preserve the existing overflow check when moving allocation sizing into callers, including prefix and names-slot overhead. Add negative fixtures for mismatched counts/prefixes, reads after failed fills, and calls without a verified fill contract.

4. **The required “fails before” keyword test is masked by existing conservative protection.**  
   **Where:** §1 Tests; [Python/gc_trace.c:1195](/home/power/code/cpython/.worktrees/boil-10a/Python/gc_trace.c:1195). Plain kill mode still follows PyMem arrays from opaque frames. Before conversion, `vectorcall_call_dict` retains the unpacked array this way. A shared `_testinternalcapi` callee holding the array pointer can provide the same protection. Clearing `d` therefore does not demonstrate the claimed failure.

   **Fix:** Describe this as a hazard exposed by promotion or scan retirement. Use a vectorcall-capable consumer whose test range is explicitly precise, assert its classification and zero fallbacks, and eliminate other scanned copies. Demonstrate failure with the span deliberately omitted, then success with it present. Ordinary `kill:pymem` alone does not suppress extension-frame protection.

5. **The ceval out-parameter conversion needs an interface decision.**  
   **Where:** §2’s prescription to use 0e; [Python/ceval.c:569](/home/power/code/cpython/.worktrees/boil-10a/Python/ceval.c:569) and [Python/ceval.c:3248](/home/power/code/cpython/.worktrees/boil-10a/Python/ceval.c:3248). Calls such as `PyObject_GetOptionalAttr` and `_PyModuleSpec_GetFileOrigin` return through `PyObject **`. Inherited 0e specifies `PyValue *` slots; appending the result afterwards does not make the escaping address acceptable to the checker. The value getter planned by 6d is also an undeclared dependency of work scheduled “now.”

   **Fix:** Specify checked raw-object output slots within the existing span mechanism, or name the prerequisite value APIs and order the conversions after them. Do not bridge the mismatch with unchecked pointer casts. Cover missing, successful, and exceptional outputs, including preservation of the original box.

## Non-blocking issues

- **Allocation claim:** §1’s “common case allocates nothing” is false for nonempty keywords: `PyTuple_New(nkwargs)` remains. State that the argument-buffer PyMem allocation disappears, and measure those allocations separately.
- **Failure coverage:** Add allocation failures at heap-buffer creation and names-tuple creation, exact small-buffer boundaries, and assertions that cleanup restores the root-chain head.
- **Measurement attribution:** §6 compares a pre-6d baseline with a result containing 6d merges. Add measurements immediately before and after the relevant 10a commits on the same merged base.
- **Validation coverage:** Preserve note 11 §8.2’s requirement to run affected tests both alone and in module order. Record the recommended tail-call probe and walk checks; GCC results do not exercise that calling convention.

## Questions

- Which exact 22 functions comprise §4’s hand-off? The cited `6d-2.md` at `8543842849f` contains no “C mode: hand-off to 10a” table.
- If 6d reaches its hard stop without landing the required rung, which buildable commit supplies the bodies, and who owns subsequent edits?
- Which single step owns retirement of the opaque macro-generated slots: rung 3 or 10b? Name it and list the functions.

## Verdict

revise
---
Reviewer: codex, 2026-10-28 05:37 PDT
