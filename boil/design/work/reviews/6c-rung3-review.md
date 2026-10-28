## Summary

[The proposal](/home/power/code/cpython/.worktrees/python-boil-the-ocean/boil/design/work/6c-rung3.md) gives shapes a coherent storage model and retires the rung-2 adapter.  
The specified clearing, non-exact-name, and variable-size paths contain correctness holes.  
Revise those paths and the collector validation before implementation.

## Blocking issues

1. **Clearing can invalidate the allocation guarantee after `__class__` assignment.**  
   **Where:** §§4–6; [Objects/typeobject.c](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Objects/typeobject.c:7391), [Objects/dictobject.c](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Objects/dictobject.c:7149).  
   **Problem:** §4 permits a six-slot allocation to acquire a type with capacity 30, relying on terminal dictionary mode. §§5–6 then reset cleared objects to the new type’s root shape. Subsequent additions can write beyond the original allocation. Clearing through `_testcapi.clear_managed_dict()` makes this reachable without resurrection.  
   **Fix:** Preserve dictionary mode when clearing a dictionary-mode object; touch only slot 0. Reset to root only from shape mode. Test small-to-large and large-to-small class changes, followed by clearing, stores, traversal, and repeated class changes.

2. **A non-exact-string load cannot execute DICT against shape storage.**  
   **Where:** §§3–4; [Objects/lookupplan.c](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Objects/lookupplan.c:92), [Objects/object.c](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Objects/object.c:1374).  
   **Problem:** After `o.x = 1`, `getattr(o, StrSubclass("x"))` must find `x`. The proposal classifies this as DICT but converts only on stores. Slot 0 still contains an attribute value, not a dict. The current adapter handles this by materialising storage; deleting it removes that protection.  
   **Fix:** When execution reaches instance storage for a non-exact name, convert before dict lookup. Preserve data-descriptor precedence and overridden hashing/equality semantics. Test both Python and C API loads, including subclass names with custom `__hash__`/`__eq__` and allocation failure.

3. **The computed dict word overlaps `ITEMS_AT_END` payloads.**  
   **Where:** §5; [Objects/typeobject.c](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Objects/typeobject.c:5761), [Modules/_testcapi/heaptype.c](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Modules/_testcapi/heaptype.c:1319).  
   **Problem:** `PyObject_GetItemData()` places items at the final `tp_basicsize`. Increasing that size by eight shifts the item array. For eight-byte items, `_PyObject_VAR_SIZE(tp, n) - 8` then points at the last item, not reserved dict storage. `_testcapi.HeapCCollection` supplies an existing reproducer base.  
   **Fix:** Give `Py_TPFLAGS_ITEMS_AT_END` types a positive dict offset in the fixed portion, before their items. Reserve negative computed offsets for layouts whose items precede subclass fields. Cover Python subclasses and C types requesting managed dicts, with zero, one, and several items; check payload integrity through mutation, clear, collection, and deallocation.

4. **Removing the pre-header does not remove debug allocator offsets.**  
   **Where:** §2; [Python/gc_trace.c](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Python/gc_trace.c:426).  
   **Problem:** “Every GC object then starts at its block” is false with debug allocator hooks. `heap_offset()` includes a surviving `debug_offset`, and `tag_offset` also excludes MEM/NOSCAN pages. Deleting both without specifying their replacements risks missed roots or interpreting non-object blocks as objects.  
   **Fix:** State the new mapping explicitly: object address equals mimalloc block address plus the uniform debug offset, restricted to the two object heaps. Keep allocator user pointers distinct from underlying blocks during freeing. Verify conservative interior roots and sweep under both `mimalloc` and `mimalloc_debug`, including release builds using debug hooks.

5. **NOMEMORY lacks a contract at non-raising entry points.**  
   **Where:** §§3–4 and §7; [Python/specialize.c](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Python/specialize.c:1113), [Python/bytecodes.c](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Python/bytecodes.c:1926), [Objects/object.c](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Objects/object.c:1303).  
   **Problem:** `_PyShape_PrepareAdd` now sets MemoryError, but `_Py_Specialize_StoreAttr` returns void and its caller immediately redispatches. Leaving that exception pending violates the specialiser’s contract. Separately, `_PyObject_GetDictPtr` currently suppresses materialisation failure; the proposal’s general conversion-failure rule says MemoryError is raised.  
   **Fix:** Make speculative preparation failure clear its MemoryError and unspecialise; let generic execution perform required preparation and report failure. Preserve `_PyObject_GetDictPtr`’s NULL-without-new-exception behavior and unchanged storage on failure. Inject failures through both entry points, including a warmed store preparing a missing edge.

6. **The proposed collection test does not collect during conversion.**  
   **Where:** Tests; §4; note 09 §4.7; [Python/gc_live.c](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Python/gc_live.c:1296).  
   **Problem:** Collection from “a descriptor in the same statement” occurs outside the ordinary conversion executor. It does not prove that the provisional dict survives or that copied values remain reachable before publication. Conservative stack retention can also mask missing roots.  
   **Fix:** Add a deterministic collection point after provisional-dict construction and before publication. Retain the receiver, name, and incoming-value rooting required by note 09; root the provisional dict separately. Exercise object-valued attributes and a newly supplied value, and assert that publication performs no allocation or callback. Cover both allocator modes.

## Non-blocking issues

- **Size accounting remains incomplete (§2).** Updating `object.__sizeof__` misses native overrides such as [list’s](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Objects/listobject.c:3297) and [dict’s](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Objects/dictobject.c:4623). Use one shared accounting rule and test native subclasses with shape storage.
- **The deletion schedule contradicts itself (§§8–9, Decisions, Risks).** Split-table removal is commit 3 in the sequence but commit 4 elsewhere; the storage commit is numbered 1 but called commit 2 under Risks. `PyDictValues` cannot disappear while retained split-table code still dereferences it. Separate removal of instance use from removal of the struct, and name one definite retirement commit.
- **Record wall time and per-type memory.** Instructions and million-instance RSS do not establish dependent-load latency or shape/cache overhead. Restore note 09 §9’s wall-time, shape-memory, plan-cache-memory, and transient-type measurements.
- **Strengthen new-handler tests.** Exercise class mutation and version exhaustion after warming POLY, METHOD_SHAPE, NONDESCRIPTOR_SHAPE, MEMBER, and WITH_HINT sites. Keep specialised code alive while transient types die and addresses are reused. Include inline ints, signed zero, infinities, and boxed NaNs in conversion tests.
- **Update debugger validation (§8).** The computed-dict reader in [Tools/gdb/libpython.py](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Tools/gdb/libpython.py:535) also needs the integer digit-count rule. Add `test_gdb` and external-inspection coverage alongside the reader changes.

## Questions

- What should `__sizeof__` report after cross-capacity `__class__` assignment, when the current type’s capacity differs from the original allocation?
- Is `LOAD_ATTR_SHAPE_POLY` deliberately SLOT-only? Note 09 §7 also routes polymorphic method calls through it; reconcile that promise with this rung’s narrower handler.
- How does the feedback entry encode a fixed versus computed dict location alongside its hint and keys version? [Its current layout](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Include/internal/pycore_feedback.h:113) aliases `hint` and `dictoffset`.

## Verdict

revise