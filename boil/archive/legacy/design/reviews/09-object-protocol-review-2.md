## Summary

The shared lookup plan and uniform `PyValue` slots give the design a coherent foundation.  
Kind transitions, generic dispatch, descriptor offsets, and cache validity still have correctness gaps.  
This review covers the full document, requested context, and relevant source; tests were not run.

## Blocking issues

1. **In-place generalisation can make existing transitions unsafe.**

   **Where:** 09 §§4.5, 5; [Objects/shapeobject.c](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Objects/shapeobject.c:148).

   **Problem:** Consider `o.x = 1; o.y = 2; o.x = "s"; del o.y`. Generalisation changes the child shape’s promise for `x` to BOXED. REMOVE then returns to its parent, which still promises I32 for `x`. Existing ADD edges can similarly lead from a generalised source to a destination with narrower promises. Updating “that one shape … and nothing else” is insufficient.

   **Fix:** Require every transition to preserve the promises for retained fields. Propagate generalisation through affected shapes or validate and widen the destination before publication. Invalidate every affected plan and compilation dependency. Add tests covering generalisation followed by ADD, delete-last, and hole deletion across shared shapes.

2. **Specialised stores need guards on the incoming value.**

   **Where:** 09 §§5, 7; [Python/bytecodes.c](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Python/bytecodes.c:2879).

   **Problem:** The specified store handlers guard the receiver’s shape and version, then write. Those guards cannot establish that a new value satisfies I32 or F64. After warming `o.x = value` with integers, a float or object can pass the same receiver guards and violate the promise used by unchecked loads. An ADD handler likewise cannot select its destination from the source shape alone.

   **Fix:** Specify incoming-value guards for both replacement and transition handlers, with misses entering GENERALISE or selecting another ADD edge. Tier 1 may omit these only with a proven value kind. Test warmed store sites with int32 overflow, floats, NaNs, bools, and int subclasses.

3. **GENERIC execution can recurse into the same plan machinery.**

   **Where:** 09 §§4.2–4.4, 4.8; [Objects/object.c](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Objects/object.c:1621), [Objects/typeobject.c](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Objects/typeobject.c:10602).

   **Problem:** A mutable descriptor produces GENERIC even when `tp_getattro` is `PyObject_GenericGetAttr`. Executing GENERIC by calling that slot re-enters generic lookup and obtains GENERIC again. Explicit generic APIs also must bypass overrides: `object.__getattribute__(self, name)` and `object.__setattr__` cannot redispatch to the overriding slot. The existing `__getattr__` hook itself calls `_PyObject_GenericGetAttrWithDict`.

   **Fix:** Separate outer slot dispatch from execution of the generic attribute protocol. Treat cacheability independently from the operation: a mutable descriptor needs uncached generic resolution, not redispatch. Preserve explicit-dictionary and AttributeError-suppression contracts through the same resolver. Test delegating overrides, mutable descriptors, and direct generic C API calls.

4. **A member descriptor’s offset is unsafe without an applicability check.**

   **Where:** 09 §§4.3–4.5, 6; [Objects/descrobject.c](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Objects/descrobject.c:164), [Python/specialize.c](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Python/specialize.c:1268).

   **Problem:** An unrelated class can receive another class’s member descriptor: with `A.__slots__ = ("x",)` and `B.__slots__ = ()`, assigning `B.x = A.x` must make `B().x` raise TypeError. The proposed member executor instead reads `A.x`’s offset under B’s shape guard, potentially outside B’s allocation. Stores can corrupt memory. Members marked `_Py_AFTER_ITEMS` also lack a constant object-relative offset.

   **Fix:** Enable direct member access only after proving descriptor-owner compatibility, supported member encoding, offset validity, and applicable audit/read-only flags. Route other members through their descriptor. Preserve AttributeError on deleting an unbound member. Test transplanted descriptors and variable-sized tuple subclasses with members.

5. **`plan.value` has incompatible contracts.**

   **Where:** 09 §4.2 versus §§4.3, 4.6; [Objects/typeobject.c](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Objects/typeobject.c:5888).

   **Problem:** §4.2 puts an overriding Python `__getattribute__` function in `value`. Sections 4.3 and 4.6 require that field to contain the requested name’s raw MRO result, including for GENERIC plans. For a class with `x = 42` and an overriding getter, `_PyType_Lookup(C, "x")` must return 42, not the getter. The proposed root-entry adapter cannot satisfy both contracts.

   **Fix:** Keep `value` as the raw MRO result unconditionally. Obtain the overriding getter from the root entry for `__getattribute__` when selecting its specialisation. Test raw type lookup alongside ordinary lookup on the same overridden class.

6. **Zero and exhausted versions are absent from the validity contract.**

   **Where:** 09 §§2, 4.3, 4.6–4.7, 5; [Objects/typeobject.c](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Objects/typeobject.c:985), [version allocation](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Objects/typeobject.c:1220).

   **Problem:** Equality with `shape->type_version` is insufficient when both versions are zero. Zero means invalid; some custom MROs cannot receive a version, classes stop receiving versions after the per-class limit, and counters can exhaust. `PyType_Modified` currently returns early for an unversioned type. Giving one of its shapes a fresh kind version could therefore create a guard that subsequent type mutation never invalidates.

   **Fix:** State that zero never authorises caching or specialisation. Define how shape generations coexist with type-version assignment, uncacheable MROs, and exhaustion; exhaustion must disable speculation. Kind changes must also notify compiled dependencies, rather than merely changing the compared word. Add deterministic tests using version-control hooks.

7. **Removing the pre-header leaves some dictionary storage undefined.**

   **Where:** 09 §6 and §9 rung 3; [type creation](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Objects/typeobject.c:4363), [managed dictionary access](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Include/internal/pycore_object.h:319).

   **Problem:** Variable-sized Python subclasses currently have `Py_TPFLAGS_MANAGED_DICT`, no inline values, and `tp_dictoffset == -1`. That value is a sentinel for pre-header storage, not an ordinary offset they can “keep” after the pre-header disappears. Native bases with their own dictionary field are a different case.

   **Fix:** Specify allocation and dictionary-pointer placement for managed types without inline slots, including int, tuple, and bytes subclasses and variable-sized extension types. A conventional computed dictionary offset is one option. Update creation, allocation, traversal, clearing, and `_PyObject_GetDictPtr` together. Test several payload lengths and collection through both attribute and C API access.

8. **MODULE is not yet a complete execution strategy.**

   **Where:** 09 §§4.3–4.4; [Objects/moduleobject.c](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Objects/moduleobject.c:1260).

   **Problem:** “Receiver’s dict, then the class” omits data-descriptor precedence, module-level `__getattr__`, and lazy-import resolution. For example, a dictionary entry named `__class__` must not shadow the type’s data descriptor. Resolving receiver-specific indices fixes the previous cache-key problem but does not preserve module semantics.

   **Fix:** Define MODULE execution in terms of `_Py_module_getattro_impl`’s complete contract, sharing generic resolution beneath it. Add tests for module `__getattr__`, dictionary entries colliding with data descriptors, lazy values, and module subclasses, alongside the alternating-module test.

## Non-blocking issues

- **Failure atomicity:** §4.5 says TO_DICT converts and then stores, but also promises unchanged slots and header on every failure. The triggering insertion can fail after conversion. Stage the insertion in the provisional dictionary before publishing it, as [the existing overflow path does](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Objects/dictobject.c:6816).

- **Collector integration:** §4.7 should name allocation from the unscanned heap for borrowed plan-cache storage. Omitting `tp_traverse` does not prevent [conservative scans from following PyMem blocks](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Python/gc_trace.c:1636). Also enumerate the incoming store value and provisional dictionary among execution roots. Stress collection with only those roots keeping objects alive.

- **Dictionary handlers and cache replacement:** §9 deletes WITH_HINT handlers without naming their DICT replacements. Specify their actual guards; a dictionary load still needs dictionary-layout and presence validation. Also define replacement of stale `(shape, name)` entries: [current insertion ignores an existing key](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Python/typecache.c:126).

- **Measurements and consistency:** Record wall time alongside instruction counts to expose dependent-load latency, plus generalisation fan-out and fixed-capacity waste. A plain six-slot instance requests at least 56 bytes, so note 08’s “small instance 16 bytes” retirement proof needs updating. Note 06 §3.3 and its 6c row also retain superseded dictionary-materialisation and `__slots__` descriptions.

## Questions

- Does the plan cache store load plans only? Cached store plans would also need to distinguish SET/DELETE and incoming value kind. Which outcome represents SET on an object with no instance storage?
- After `__class__` assignment between types with different capacities, is the allocation-capacity invariant explicitly waived because dictionary mode is terminal? Which size do clearing and accounting use?
- What representation and aliasing rules will resolve `str +=` in rung 4? Ownership is named, but the decision remains open.

## Verdict

revise