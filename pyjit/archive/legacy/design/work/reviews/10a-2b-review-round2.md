## Summary

Caller-owned spans and split keyword unpacking fit the collector’s root model.  
Buffer sizing, raw-pointer lifetimes, slot arrays, and the checker plan still need revision.  
Source review only: `uv` could not acquire its cache lock in the read-only sandbox.

## Blocking issues

1. **The unpack-buffer sizing contract omits the names slot.**

   **Where:** §1, `_PyStack_UnpackDictSlots()` and caller steps 1–4. The stated total includes the prefix, offset slot, positional arguments, and keyword values. The caller also appends `kwnames`, which none of the prefix-table rows includes. Following that contract underallocates by one slot; the debug assertion detects the overwrite, while release builds corrupt memory.

   **Fix:** Define the checked total explicitly as `prefix + 1 /* names */ + 1 /* offset */ + nargs + nkwargs`. State whether `prefix` excludes names. Test each caller at its actual small-buffer boundary, plus overflow through the sizing helper.

2. **Making the bytes constructors precise can remove the source buffer’s only root.**

   **Where:** §2b; [bytesobject.c:166](/home/power/code/cpython/.worktrees/boil-10a/Objects/bytesobject.c:166), [bytearrayobject.c:198](/home/power/code/cpython/.worktrees/boil-10a/Objects/bytearrayobject.c:198). Both constructors retain a `const char *` across object allocation and subsequently copy from it. That pointer can be an interior pointer into an otherwise unrooted bytes object. Conservative scanning currently protects such pointers. The C mode excludes these pointer types from object liveness, so its zero count—and the inline audit using that checker—cannot establish safety.

   **Fix:** Decide how these entries preserve raw buffer owners. Either provide a root mechanism that resolves interior pointers to their owning allocation, or retain conservative treatment for these general C API entries with a named retirement step. Add an injected-collection test whose freshly allocated source survives solely through its data pointer, under kill and ASan.

3. **The creator-side array audit still stops before the slot family.**

   **Where:** §3 prescribes rule 1’s parameter spans; [typeobject.c:10383](/home/power/code/cpython/.worktrees/boil-10a/Objects/typeobject.c:10383) and [typeobject.c:2918](/home/power/code/cpython/.worktrees/boil-10a/Objects/typeobject.c:2918). `slot_sq_item()` creates `ival` and places it in `stack[2]`. An index outside int32 produces a heap object. Its final dispatch does not make `ival` live *after* the call, but the array must preserve it during method lookup and dispatch. `slot_sq_ass_item()` has the same obligation. Parameter spans alone do not satisfy rule 3.

   **Fix:** Extend §2’s array inventory to every promoted slot and its inlined helpers. Put newly created argument values in the creator’s span before lookup. Test a C sequence-slot call with an index above `INT32_MAX`, collecting during method resolution before argument transfer; include a control with that anchor omitted.

4. **Compiler inlining does not make `box_owner_for_descr()` checkable.**

   **Where:** §4’s first bullet; `Objects/object.c` on `boil/6d`; [gc_inline_audit.py:314](/home/power/code/cpython/.worktrees/boil-10a/Tools/build/gc_inline_audit.py:314). The document says inlining makes the append the caller’s own, requiring no span-parameter rule. The checker reads unexpanded source, and the inline audit checks the helper’s body independently. It will still see an append through an unrecognized span parameter and reject it. Marking the helper produces the same problem.

   **Fix:** Move the boxing, append, and injection into the caller’s source, or specify and test a span-parameter contract. State the corresponding implementation for §2’s collecting `VALUES_TO_OBJECTS` conversion; “gets a span form” leaves the same helper-boundary question unresolved.

## Non-blocking issues

- **Correct the allocation model in §1.** [gc_live.c:1439](/home/power/code/cpython/.worktrees/boil-10a/Python/gc_live.c:1439) defers Python callbacks during inline collection. `PyTuple_New()` does not itself run finalizers that mutate `kwargs`. Specify the actual reentrant operation if one is intended; otherwise remove that rationale.
- **Repair the ceval inventory.** `_Py_BuildMap_Values` already consumes values directly. `_PyEval_ImportNameWithImport` is externally declared, and `_PyEval_LazyImportName` has a separate `args[5]` array at [ceval.c:3184](/home/power/code/cpython/.worktrees/boil-10a/Python/ceval.c:3184).
- **Strengthen object-slot fixtures.** Cover a raw float box, use after pop, and overwriting a slot while an alias to its previous object remains live. Those test the proposed provenance contract beyond accepting an output address.
- **Correct the proof claims.** §1’s returned argument pointer currently receives no span provenance in [root_check_c.py:2519](/home/power/code/cpython/.worktrees/boil-10a/Tools/cases_generator/root_check_c.py:2519). Existing liveness therefore does not establish the claimed rejection of reads after pop through that pointer. Add a negative fixture or describe this as dynamically verified.
- **Update Risks.** It still describes deleted 0h and says the zero-error hot helpers need only marks, contradicting §2’s array obligations.

## Questions

- Will `_PyStack_KwNames()` itself be precise? If it remains opaque, which rung retires that new call-layer exception?
- Which span slots are reserved once and reused inside loops, especially matching and unpacking? Specify capacity without appending indefinitely.
- If the scope cut moves item 6 to rung 3, does item 5 move with it, given its explicit dependency?

## Verdict

revise
---
Reviewer: codex, 2026-10-28 05:56 PDT
