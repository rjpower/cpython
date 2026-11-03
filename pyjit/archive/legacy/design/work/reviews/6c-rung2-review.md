## Summary

The shared resolver can remove duplicated attribute logic, and reading the current property getter fixes a lifetime hazard.  
The proposal still conflates generic access with hook dispatch and leaves cache and store contracts incomplete.  
Resolve these gaps before implementation and add targeted tests for them.

## Blocking issues

1. **Executing a GENERIC plan can recurse into the same executor.**

   **Where:** “What changes” items 3–4 and “Decisions” item 4; note 09 §4.4. In [object.c](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Objects/object.c:1621), generic access deliberately bypasses the receiver’s `tp_getattro`. The proposed GENERIC executor calls that slot.

   For a mutable descriptor on an otherwise ordinary class, the slot is `PyObject_GenericGetAttr`: execution recurses immediately. For a class with `__getattr__`, [_Py_slot_tp_getattr_hook](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Objects/typeobject.c:10602) already calls generic access; sending its ABSENT or PROPERTY plan back through the hook also recurses. Explicit `object.__getattribute__` and `object.__setattr__` must continue to bypass overrides.

   **Fix:** Separate outer slot dispatch from execution of generic attribute semantics. Mutable descriptors need dynamic descriptor-slot execution within the latter. Both should share the precedence resolver. Add tests for delegating overrides, explicit base-method calls, missing attributes with `__getattr__`, and descriptor mutation.

2. **`plan.value` has incompatible meanings.**

   **Where:** “What changes” item 5 promises that raw type lookup returns `plan.value`; item 6 retains `LOAD_ATTR_GETATTRIBUTE_OVERRIDDEN`. Note 09 §4.2 puts the custom `__getattribute__` function in `value`, while §4.3 requires the original MRO result.

   Following §4.2 would make `_PyType_LookupValueAndVersion(C, "__len__", …)` return `C.__getattribute__` after the corresponding root plan is cached. Preserving the MRO result instead leaves the specialiser without its stated call target.

   **Fix:** Keep `value` exclusively the canonical MRO result. Obtain the override target from separately identified phase-1 facts, then write it into feedback. Specify how raw lookup populates root entries without recursively resolving hook names or changing its exception contract. Exercise both cache-population orders through the C lookup APIs.

3. **Cache eligibility and phase-1 consistency are underspecified.**

   **Where:** “What changes” items 3 and 5; “Invariants”. Exact `str` is insufficient: [typecache.c](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Python/typecache.c:193) requires an interned name. Version equality also needs an explicit nonzero requirement: [assign_version_tag](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Objects/typeobject.c:1220) can fail, including after a type exhausts its version allowance.

   Phase 1 performs several lookups that may execute equality code against non-exact class-dictionary keys. Those calls can change the MRO, receiver shape, or descriptor after an earlier fact was obtained. The executor-only rooting invariant does not specify protection of those intermediate facts.

   **Fix:** Carry cache eligibility and the observed generation through phase 1; apply note 09’s non-exact-key exclusion; validate all facts before publication. Root intermediate values across further lookups and collection. Add non-interned-name, version-exhaustion, and reentrant-MRO tests, extending [test_type_lookup_mro_reference](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Lib/test/test_descr.py:5535).

4. **The store-plan path is neither fully defined nor transactional.**

   **Where:** “What changes” items 1, 4 and 6; “Tests—Store plans”. The specialiser calls the load-plan API, yet its table includes ADD, which is a store outcome. Load classification cannot determine store behavior: custom read and write hooks are independent.

   Preparation also cannot use `_PyShape_Add` unchanged. [shapeobject.c](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Objects/shapeobject.c:148) returns NULL without an exception for both allocation failure and the shape limit. It requires an existing shared-key index; [store_instance_attr_lock_held](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Objects/dictobject.c:6780) currently inserts that key and writes the value before updating the shape. That contradicts the promised unchanged-object MemoryError test.

   **Fix:** Define a store-plan entry point with SET/DELETE, value kind, and an explicit guard. Specify preparation of a new shared-key index, capacity checks, transition allocation, and distinct limit/OOM results before committing values, insertion order, and header. Cover REMOVE and MISSING, deletion failure, and classes with independent read/write overrides.

5. **The handler table assumes storage guarantees that rung 2 does not have.**

   **Where:** “What changes” item 6 and the opening layout promise.

   A dictionary-mode shape can still have valid inline values and no dict after shape exhaustion. A materialised dict can instead alias those values through a split table. See [dictobject.c](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Objects/dictobject.c:6708). Consequently, DICT cannot mean “a dict exists”, and today’s `LOAD_ATTR_WITH_HINT` cannot handle every DICT plan.

   Likewise, METHOD can be valid for an instance lacking `m` even when another instance added `m` to the shared keys. Today’s [specialize_attr_loadclassattr](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Python/specialize.c:1464) asserts that the name is absent from those keys. Deleting `instance_has_key` does not remove that handler precondition.

   **Fix:** Specify the current-storage adapter and handler eligibility checks. Decline unsupported specialisations or adapt their guards; retire this adapter explicitly in rung 3. Test shape exhaustion without materialisation, split-dict aliasing, and one instance shadowing a method while another does not.

6. **Direct member execution needs explicit safety criteria.**

   **Where:** “What changes” items 1 and 6; “Risks”. Naming audited and `_Py_AFTER_ITEMS` members as risks does not define when `member` may authorize a direct load or store.

   Existing checks in [specialize.c](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Python/specialize.c:1067) distinguish `_Py_T_VALUE_EX` from native pointer and numeric members, validate the descriptor’s owner type, reject audited or variable-offset loads, and reject read-only stores. Bypassing these can read unrelated memory, suppress audit events, or write encoded integers into native pointer fields.

   **Fix:** Define separate direct-load and direct-store eligibility conditions; route other members through their descriptors. Preserve repeated-deletion errors from [structmember.c](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Python/structmember.c:323). Add native-member cases and a member descriptor attached to an unrelated class.

7. **The non-exact-string rule changes descriptor precedence and side effects.**

   **Where:** “What changes” item 3 says such names materialise the dict and yield DICT.

   Current [generic_getattr_value](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Objects/object.c:1660) executes a data descriptor before attempting materialisation. `getattr(obj, StringSubclass("x"))` must therefore still reach a property or member first, including on objects without dictionaries. Premature materialisation introduces observable allocation failures and layout changes.

   **Fix:** Make these names uncached while retaining ordinary descriptor precedence. Materialise only when execution reaches instance storage. Also preserve the load/store distinction for set-only descriptors: load precedence requires `tp_descr_get` as well as `tp_descr_set`; stores require the setter. Add string-subclass/property tests and a C set-only descriptor test.

## Non-blocking issues

- **The free-threaded gate is stale.** “Decisions” item 6 cites note 02, but architecture §3.4 drops that build, and [configure.ac](/home/power/code/cpython/.worktrees/python-boil-the-ocean/configure.ac:1991) confirms its removal. Delete this gate unless restoring free threading is an explicit scope change.
- **Classifying twice proves repeatability, not purity.** Compare initialized fields rather than padding, and use allocation/user-code assertions if the debug check is intended to enforce those prohibitions.
- **Add `test_type_cache` to the targeted ladder.** Extend it for collisions, resize, negative entries, and the same name under several shapes. The existing suite already covers cache contracts and version exhaustion.
- **Strengthen lifetime tests.** Keep specialised code alive after dropping transient types and use weak references to check reclamation. Collection loops alone do not establish address reuse. Extend property replacement to NULL and non-function getters.
- **Complete the measurement record.** Resolve whether cache bytes are recorded here or deferred to rung 5. Record boxes per loop alongside instructions, especially for class values and native descriptor boundaries; separate cold cache construction from warm lookup.

## Questions

- When a store transition does not exist, what event creates it before `STORE_ATTR_SHAPE_ADD` is installed, given that the specialiser may no longer prepare transitions?
- Does `lookup_plan(module, name)` return MODULE or GENERIC, and where is MODULE executed? Retaining the module specialiser does not settle note 09 §4.3’s generic-module contract.
- In rung 4, how will an insert-only table replace a stale `(shape, name)` plan after in-place kind generalisation changes only the shape version?

## Verdict

revise