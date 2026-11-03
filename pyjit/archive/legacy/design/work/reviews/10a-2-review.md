## Summary

The single-span approach fits note 11 and keeps the root mechanism focused.  
The lifetime proof has gaps around varargs, numeric boxes, load reordering, and vectorcall scratch slots.  
Source inspection and in-memory checker probes support the findings below; revise before promotion.

## Blocking issues

1. **Unread varargs can die before the proposed copy.**  
   **Where:** §1 rule 5 and §2 step 5; [PyObject_CallMethodObjArgs](/home/power/code/cpython/.worktrees/boil-10a/Objects/call.c:1137) and [do_mkstack](/home/power/code/cpython/.worktrees/boil-10a/Python/modsupport.c:235). Method lookup runs before the varargs walk. Formatted calls can allocate or invoke an `O&` converter before reading later object operands. Rooting the built prefix protects neither case: unread register arguments can exist only in the precise entry frame’s vararg save area. A checker fixture that collects before `va_arg()` reports zero errors.  
   **Fix:** Anchor pending object operands before any collecting operation, or explicitly keep the variadic entry frames opaque until their named migration step. Test a collecting method lookup and a `"sO"` build whose first item triggers collection, with the unread object otherwise unrooted.

2. **The checker confuses preserving a value with preserving its box.**  
   **Where:** §0, §1 rules 2 and 4; [root_check_c.py’s identity table](/home/power/code/cpython/.worktrees/boil-10a/Tools/cases_generator/root_check_c.py:152). The checker accepts `_PyValueSpan_Append(&root, PyValue_FromObject(arg)); collect(); consume(arg);`. For an exact finite float, that span contains an immediate; it does not retain the `PyObject *arg` box. This matters when descriptor argument checking creates a boxed receiver in [method_check_args](/home/power/code/cpython/.worktrees/boil-10a/Objects/descrobject.c:283).  
   **Fix:** Require raw pointer encoding when a span protects a concrete `PyObject *`; canonicalize only values destined for value interfaces. Make the checker distinguish those guarantees. Add a negative checker fixture and a kill/ASan test that reads the original finite-float box after collection.

3. **Loading immutable fields does not itself establish an anchor.**  
   **Where:** §1 rule 1a, §3, and §5’s “rule 1a costs nothing”; [cfunction_enter_call](/home/power/code/cpython/.worktrees/boil-10a/Objects/methodobject.c:417). Moving `m_self` into a local before recursion entry merely transfers the rooting obligation to that local. If the builtin is unrooted, immutability cannot keep its receiver alive. Moreover, `cfunction_enter_call()` still reads `func` after the collecting recursion check. The checker rejects both patterns.  
   **Fix:** State that every extracted object surviving another safepoint needs an independent anchor or a rooted owner. Root `self` and `cls` where needed, and separate recursion entry from the subsequent function-pointer load. Retain the zero-cost claim only for paths with no intervening safepoint.

4. **A covered scratch slot loses its old root while overwritten.**  
   **Where:** §1 rule 2 and Risks; both prepend implementations save and overwrite the incoming slot in [call.c](/home/power/code/cpython/.worktrees/boil-10a/Objects/call.c:982). When `args[-1]` contains a live value, overwriting it removes that value from the creator’s span. The saved `tmp` then survives only in the precise callee until restoration. Restoring the slot afterwards does not protect the intervening collection.  
   **Fix:** Put the saved value in the callee’s span before overwriting the borrowed slot. Distinguish newly allocated NULL scratch slots from borrowed populated slots. Test nested prepend calls with collection during the inner call, checking survival and restoration on success and exception paths.

5. **The promotion proof omits collecting header helpers.**  
   **Where:** §1 rule 6 and §2 step 1; [_PyObject_VectorcallValueTstate](/home/power/code/cpython/.worktrees/boil-10a/Include/internal/pycore_call.h:144). This central inline helper holds `callable_o` across vectorcall for result checking; checking it directly reports an unanchored parameter. Yet the build stamp discovers marked `.c` files only, and the inline audit’s source discovery also excludes headers. Marking the outer call functions does not check this body.  
   **Fix:** Include the collecting header helpers explicitly in the worklist and extend both discovery paths to check marked header definitions, or move their collecting bodies into checked `.c` definitions. Add a failing marked-header fixture before promotion.

## Non-blocking issues

- **The interface migration order conflicts with the file ban.** §2 marks `_PyStack_UnpackDict` callers before replacing their escaping out-parameter. Its fourth caller is [ceval.c](/home/power/code/cpython/.worktrees/boil-10a/Python/ceval.c:1990), which this half excludes. Order the shared interface conversion first and explicitly permit that mechanical caller update, or defer the dependent promotions. Avoid introducing a temporary second interface.

- **Several measurements bypass the named paths.** §5’s sort loop uses [PyObject_VectorcallValue](/home/power/code/cpython/.worktrees/boil-10a/Objects/listobject.c:2751); map uses [_PyObject_VectorcallValueTstate](/home/power/code/cpython/.worktrees/boil-10a/Python/bltinmodule.c:1617). Warmed `len()` calls can use `CALL_LEN`. Add C loops that directly exercise the public object APIs with small and heap arrays, keywords, and OFFSET. Measure the span cost rather than asserting store counts.

- **§0 needs accurate call classification alongside parser fixes.** The checker currently treats `va_copy`, `va_arg`, `va_end`, and `PyMem_Free` as collecting. It also classifies `PyMem_Malloc` as collecting despite [the allocator’s explicit non-collecting contract](/home/power/code/cpython/.worktrees/boil-10a/Objects/obmalloc.c:1207). Resolve these with reviewed summaries; otherwise the single heap-span setup encounters artificial rooting requirements.

- **The validation matrix is weaker than the referenced specification.** §6 omits note 11’s ASan checks, although 10a-1 records a working ASan build. Include them. For the weakref test, use a named injection between load and use and assert that it fired; setting stress to 1 alone does not establish that timing. Identify the exact accepted stress failures so an earlier new crash cannot hide behind “known crashes.”

## Questions

- What are the exact replacement signatures and span ownership rules for `_PyStack_UnpackDict`, `_Py_VaBuildStack`, and generator result outputs? Specify who appends, who pops, and how ownership crosses helper returns.
- Does the one-span buffer reserve slots for parameters, results, boxed receivers, and saved OFFSET values? §1 currently describes parameters and argument copies only.
- What precise execution makes each probe object dead before collection? An active `tp_call` argument tuple is deliberately rooted, and `next()` sends `None`; the generator case needs an explicit sequence.
- Which functions and probe cases are mandatory for this half, and which definitively await 1b? Replace “listed here if they do” with a fixed completion list.

## Verdict

revise
---
Reviewer: codex, 2026-10-27 22:15 PDT
