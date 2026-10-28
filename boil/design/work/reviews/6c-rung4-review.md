## Summary

Slot kinds preserve one PyValue representation, and generalising the field owner’s subtree is sound.  
The cache changes need stronger validity and lifetime rules; stale feedback can also disable polymorphic loads.  
The string builder has an aliasing hole and lacks a complete storage and validation contract.

## Blocking issues

1. **A4 weakens the check that the MRO result is still current.**

   **Where:** A4.1; [`lookup_plan_facts()`](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Objects/typeobject.c:5985).

   Replacing `version == shape->type_version` with two nonzero checks loses more than the unwanted equality between type and shape versions. `find_name_in_mro()` can run key comparisons and replace the MRO. Such code can leave the receiver type with a fresh version and an entirely string-keyed MRO; the proposed predicate could then cache the old result under the new shape version.

   **Fix:** Require the lookup’s returned version to equal the live `tp_version_tag`, independently require a nonzero shape version, and validate the classification’s shape version at insertion. Add a reentrant key-comparison test that changes bases and assigns a new type version during lookup.

2. **A6 treats a borrowed shape/version pair as permission to dereference the shape.**

   **Where:** A6, “Loads”; note 09 §4.7; [`pycore_feedback.h`](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Include/internal/pycore_feedback.h:112).

   Feedback does not retain the receiver, type, or shape. After collection, checking a version through the stored shape pointer is already a use-after-free. Conservative C-stack scanning does not establish that lifetime. The current interpreter validates against the **live receiver’s** shape; a JIT compiling from feedback has no equivalent guarantee.

   **Fix:** Record the observed load kind in `_PyFbAttr` and the corresponding poly row. The JIT may use that snapshot only behind the runtime shape/version guard, or obtain the kind through an explicitly rooted live receiver. State how compilation establishes dependency lifetimes without making borrowed caches roots.

3. **Generalisation leaves obsolete versions occupying polymorphic rows.**

   **Where:** A4, A6–A7; [`fb_record()`](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Python/specialize.c:491).

   Rows are distinguished by version as well as shape. Generalising several fields creates several observations of the same shape and consumes the four-row budget. A site with only two live shapes can become permanently MEGA because of invalidations, after which `specialize_shape_poly()` declines it. The proposed cacheability fix does not address this.

   **Fix:** Replace an obsolete row when observing the same live shape again, rather than appending another version of it. Do not inspect other borrowed shapes to prune them. Test a two-shape load across enough field generalisations to exceed four historical observations, and require it to remain polymorphic.

4. **Exempting the whole `+=` instruction from sealing permits shared-string mutation.**

   **Where:** B, “SEAL”; [`specialize_binary_op()`](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Python/specialize.c:2248), [`binary_op_value()`](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Objects/abstract.c:1993).

   After building an unshared `s`, `s += rhs` can call `rhs.__radd__(s)`. That method can retain `s` and return it unchanged. The instruction is exempt from SEAL, so a later exact-string append can mutate the retained alias.

   **Fix:** Exempt only the successful exact-string append path. Seal before every fallback that can expose either operand to user code or a C implementation. Cover cold execution and guard failures, including a reflected method that retains and returns its left operand. Also reconcile eligibility with the existing handler’s support for `s = s + t`.

5. **The builder’s capacity and growth contract is unspecified.**

   **Where:** B, “Growth”; [`PyASCIIObject`](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Include/cpython/unicodeobject.h:149), [`_PyUnicode_ResizeCompact()`](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Objects/unicodeobject.c:1077), [`PyObject_Realloc()`](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Objects/obmalloc.c:1581).

   Existing compact strings store logical length, not capacity. An unshared bit does not say how much slack remains. The document also leaves unspecified growth beyond capacity, overflow, allocation failure, rooting during allocation, and `s += s` when growth relocates the buffer. Existing resize code changes logical length and can invalidate pointers.

   **Fix:** Specify one allocation/growth helper: how capacity is recovered under normal and debug allocators, checked geometric sizing, logical length and terminator updates, operand/result roots, self-append handling, and publication only after success. Revise the unchanged-layout claim if necessary. Add forced-collection and allocation-failure tests at growth and widening boundaries.

## Non-blocking issues

- **A5 misstates the writer audit.** [`publish_dict()`](/home/power/code/cpython/.worktrees/python-boil-the-ocean/Objects/shapeobject.c:454) writes a dictionary pointer into slot 0, potentially replacing an I32/F64 value, before changing the header. Document this as a commit sequence with no intervening call or safepoint; it should not generalise the old field.

- **A7 needs the new failure cases.** Add generalisation with an unversioned type, shape-tag counter exhaustion, and class mutation after generalisation. Exercise incompatible stores through the value API, compatibility C API, and `object.__setattr__`, followed by collection with a boxed referent held only in the slot. Existing type-version exhaustion tests do not prove the new allocator’s exhaustion behavior.

- **A4.3 needs a disposition.** “Recorded as an imprecision” leaves the store-site policy knowingly unenforced. Either accept and amend that policy, or name the retirement step and proof. A separately recorded scalar type version could identify the observation’s type without dereferencing a dead shape.

- **B understates the cost of sealing.** Reading `len(s)` on every iteration seals the string and forces the next append to copy the prefix; the cost can remain quadratic. Measure that pattern separately. Prove the intended append case with a size sweep, copied bytes or allocation counts, wall time, and peak memory—not one large input. If B is implemented, extend the targeted ladder to Unicode, compiler/IR, frame, tracing, and monitoring tests.

- **Clarify A6’s tier terminology.** The architecture calls the SSA JIT “tier 1”; “loads do not change in tier 1” appears to mean the interpreter.

## Questions

- Which scope governs B? Note 08 §3 already records a coordinator ruling deferring the escape proof to step 9, while this document awaits a ruling and schedules an implementation commit. Reconcile them. Note 08’s “trace proves” wording also conflicts with the method-JIT architecture.
- Where does SEAL insertion run relative to copy propagation, store elimination, and register compaction? Define eligible locals and how preserved temporaries, cells, implicit reads, and both operands of self-append are covered.

## Verdict

revise
---
Reviewer: codex, 2026-10-27 20:46 PDT
