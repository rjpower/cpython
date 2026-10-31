## Summary

The shared lookup plan gives the interpreter and JIT a common resolution boundary.  
Storage, descriptor invalidation, and GC lifetime contracts remain incomplete.  
This review is based on the documents and source; tests were not run.

## Blocking issues

1. **The plan’s inputs and guards cannot describe module lookups.**

   **Where:** 09 §§4, 7; [Python/bytecodes.c](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Python/bytecodes.c:2678).

   **Problem:** All ordinary modules share one root shape, but their dictionaries have different indices and keys versions. `_PyShape_LookupPlan(shape, name, for_call)` cannot derive `_Py_LOOKUP_MODULE.offset`, and `_PyLookupPlan` has no dictionary guard. The global cache also omits `for_call`, although resolution accepts it.

   **Fix:** Either make plans receiver-aware with explicit dictionary guards, or return a module strategy that performs dictionary lookup at execution time. Make binding mode part of the cache key or make plans independent of it. Test one site alternating modules with different insertion orders, then deletion and reinsertion.

2. **Ordinary attribute access does not preserve descriptor binding.**

   **Where:** 09 §4, “Consumers”; [Objects/object.c](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Objects/object.c:1620).

   **Problem:** “METHOD … return the value” returns an unbound function for `f = o.m`. Only the method-call form may return the callable and receiver separately. The enum also lacks a general non-data descriptor case, while DICT needs a class fallback when the instance dictionary misses.

   **Fix:** Specify the complete precedence: data descriptor, instance attribute, non-data descriptor, class value, missing. Define ordinary binding and call-pair production separately over that resolution. Test extracted methods, `staticmethod`, `classmethod`, custom `__get__`, instance shadowing, and missing attributes through both value and pointer APIs.

3. **Receiver-type versions do not cover descriptor mutation.**

   **Where:** 09 §§2, 4, 7; [Python/specialize.c](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Python/specialize.c:660), [Objects/descrobject.c](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Objects/descrobject.c:1893).

   **Problem:** Changing a descriptor class’s `__get__` or adding `__set__` need not modify the receiver’s type. An existing SLOT plan can therefore become wrong without failing its guard. Likewise, `p.__init__(new_getter)` replaces a property’s getter without modifying its owning class. A cached borrowed `fget` can then become unreachable while its advertised guard still holds.

   **Fix:** Include descriptor-type dependencies and property-content validation, or read the current descriptor fields during execution. Function versions alone do not detect replacement of the getter. Define how invalidation deoptimises *active* compiled code after callbacks, rather than merely preventing future entry. Test these mutations after warmup and during a hot loop, with collection between mutation and reuse.

4. **A materialised `__dict__` has no defined storage or ownership path.**

   **Where:** 09 §§2, 6, 9 versus 04 §4; [Objects/dictobject.c](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Objects/dictobject.c:6708), [Objects/object.c](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Objects/object.c:1310).

   **Problem:** Reading `__dict__` supposedly preserves the shape, but the layout removes the dict word. Nothing records the materialised dictionary. Note 04’s design requires both that word and `ma_values->shape`, which §9 deletes. Once mutations enter dictionary mode, it is also unclear how a dict operation finds and updates the owner. `_PyObject_GetDictPtr` additionally returns writable `PyObject **` storage.

   **Fix:** Choose one complete representation. Either materialisation enters dictionary mode immediately, or specify the dictionary pointer, owner relationship, mutation hooks, and traversal/detachment rules. Define the writable-pointer API contract. Test dictionary identity, mutations through both aliases, replacement, and a retained dictionary after its owner becomes unreachable.

5. **The allocation and capacity invariant is missing.**

   **Where:** 09 §§6–7; [Objects/dictobject.c](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Objects/dictobject.c:6667).

   **Problem:** `slots[capacity]` remains, but the capacity field disappears. Equal shapes need not imply equal allocation sizes under note 04’s allocation policy. A cached transition therefore cannot prove its destination slot exists. Non-moving objects cannot simply grow. The literal `header, slots` layout also omits native base payloads.

   **Fix:** Specify allocation size, capacity recovery, slot-base offsets, and overflow conversion for ordinary instances and native subclasses. Transition handlers must prove capacity before writing and preserve the old object on allocation failure. Test differently sized instances sharing a shape, capacity boundaries, native subclasses, and injected failures during dictionary conversion.

6. **Non-transitioning `__slots__` shapes contradict presence and kind promises.**

   **Where:** 09 §§2, 5–6; [Objects/typeobject.c](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Objects/typeobject.c:4321).

   **Problem:** A newly allocated slotted object has unbound fields; deletion makes them unbound again. One fixed root shape cannot prove those fields filled. Nor can it promise I32 or F64 while arbitrary stores are allowed without transitions. Slots-only classes can exceed 30 declared slots and cannot fall back to an exposed instance dictionary.

   **Fix:** Separate fixed offsets from occupancy and kind facts. Either allow shapes to transition while retaining offsets, or specify nullable general-value slots and their extra checks. Preserve member descriptors, inherited slots, duplicate slot names, and pickle state. Test unset/read/delete/reassign, mixed kinds, inheritance, and more than 30 slots.

7. **The stated kind lattice and float encoding are wrong.**

   **Where:** 09 §5; 04 §3; [Include/pyvalue.h](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Include/pyvalue.h:140).

   **Problem:** I32 and F64 are disjoint promises, so `I32 < F64` is not a valid generalisation relationship. Also, inline floats contain double bits **plus `2^49`**, not raw double bits. Treating an F64 slot as a C double would misread it.

   **Fix:** Put I32 and F64 below a general-value kind independently, as note 04 already specifies. Document decoding through `PyValue_Double`. If replacement-only I32→F64 transitions are intended, distinguish those from generalisation of existing values. Test alternating ints/floats across shared shapes, int32 overflow, NaNs, infinities, and signed zero.

8. **The pure resolver contract does not support the proposed store plan.**

   **Where:** 09 §4 signatures and rules, §§5, 7; [Objects/shapeobject.c](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Objects/shapeobject.c:155).

   **Problem:** `_PyShape_StorePlan` receives neither the incoming value kind nor a set/delete operation, yet must select kind transitions. New transitions require allocation, which the resolver forbids. Arbitrary C API names also need handling: the existing MRO lookup can hash names, ready types, and populate caches.

   **Fix:** Define store-plan inputs and outcomes, including deletion, transition preparation, cap exhaustion, and failure. Keep allocation in an explicit preparation/execution phase. State the exact-string and ready-type preconditions for pure resolution and the path for other inputs. Require failure tests proving that unsuccessful preparation leaves values and headers unchanged.

9. **Shape and plan ownership needs an explicit tracing-GC contract.**

   **Where:** 09 §§2, 4, 9; [Python/gc_trace.c](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Python/gc_trace.c:1059), [Python/ceval.c](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Python/ceval.c:938).

   **Problem:** Type ownership and “borrowed while the guard holds” do not establish tracing reachability. PyMem blocks reached from objects are not automatically scanned. Shape names and other owned objects need traversal; borrowed caches need a reclamation policy. Plan execution can box or invoke Python, and eval-loop C locals are excluded from conservative scanning.

   **Fix:** Specify the ownership graph and traversal for shapes, slot values, dictionaries, and caches. Root receivers, active descriptors, boxes, and intermediate results across allocating calls using the existing value-root discipline. Keep borrowed caches from becoming permanent roots; validate stale entries through live receiver metadata before dereferencing them. Preserve instance-before-type destruction. Test forced collections during execution, shape-only names, cached transient types, address reuse, and cache eviction on both builds.

## Non-blocking issues

- **Header comparisons need masking.** In §2, write the actual guard as `(ob_header & SHAPE_MASK) == shape`. GC and weakref flags change independently of layout. Shape publication must preserve them. Record the cost of that actual sequence.
- **Replacing `typecache.c` needs a caller map.** Its current contract is raw MRO lookup, used beneath ordinary and special-method lookup. Explain how `_PyType_LookupValueAndVersion` retains that contract without recursion through the plan cache or a second independent resolver.
- **Strengthen the measurements.** Record boxing, cache/shape memory, read-only dictionary materialisation, five-shape sites, shape-cap exhaustion, transient-type churn, and allocation-heavy workloads. One million uniform instances does not measure those costs. Behavioral tests must accompany plan assertions.
- **Update §9’s gates.** `boil/PROCESS.md` defers ports until phase P. Include its regeneration checks, pyc-magic requirement, and level-4 benchmark record.

## Questions

- Are native descriptor calls and custom `tp_getattro`/`tp_setattro` permanent boxing boundaries, or does a named later step change their signatures?
- Must an escaped instance dictionary avoid retaining its owner? A strong owner backpointer changes weakref and finaliser behavior.
- Note 08’s hack register assigns the string-append rope/builder decision to 6c. Does 6c still own it? If moved, name its retirement step.

## Verdict

revise