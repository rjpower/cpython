## Summary

Reviewed `3c4e421c83f..2cfb3b7e909`; no handler or cases-generator hunks changed.  
One blocking gap remains: several dict-backed getsets still box, contrary to rule E.0, without a recorded deviation.  
Inventory reproduced; focused debug tests: 10 passed, one skipped. Full gates were assessed from the record.

## Blocking findings

1. **Complete the dict-backed accessor conversion and test its allocation contract.**

   **Problem:** Classmethod and staticmethod annotation accessors still box around object-form dictionary helpers. `type_abstractmethods` likewise boxes its dictionary result before canonicalising it. These are value-storage paths, covered by [rule E.0](/tmp/boilreview/boil/design/work/6d-4.md:314), rather than native object-field residues. “Where the work differs from the plan” records no exception for them.

   **Where:** [classmethod accessors](/tmp/boilreview/Objects/funcobject.c:1616), [staticmethod accessors](/tmp/boilreview/Objects/funcobject.c:1894), their [shared helpers](/tmp/boilreview/Objects/funcobject.c:1408), and [type_abstractmethods](/tmp/boilreview/Objects/typeobject.c:1538).

   **Evidence:** On the recorded Py_STATS binary, 1,000 warmed operations produced 1,001 int boxes and 1,002 allocations for each classmethod/staticmethod annotation read or write, and for `C.__abstractmethods__` reads. Module/type annotation controls produced only one box and two allocations—the fixed overhead. The [new audit cases](/tmp/boilreview/boil/tools/measure/box_audit.py:268) omit these paths, so identity tests and the passing audit do not detect the gap.

   **Fix:** Convert the shared helpers to take/return `PyValue`, using value dictionary and attribute APIs; convert `type_abstractmethods`’ lookup too. Preserve deletion and error behavior. Add uncached-int and float audit cases with zero-box/allocation budgets, then update the as-built record and affected gates at the fixing commit.

## Non-blocking findings

- **The unbuilt-module check can report success after a compiler failure.** [check_unbuilt.sh:58](/tmp/boilreview/boil/tools/check_unbuilt.sh:58) discards the compiler’s exit status and judges only filtered diagnostic text. A failed invocation without matching text prints `ok`. Capture and check the compiler status independently, while continuing to reject warnings.

## Questions for the coordinator

- Are the pre-audit boxes in [type_set_module](/tmp/boilreview/Objects/typeobject.c:1465) and [type_set_doc](/tmp/boilreview/Objects/typeobject.c:1944) intentional residues? Their storage is dictionary-backed; if auditing requires those boxes, name that boundary and its reason in the record.
- The new architecture §21.5 carries forward the rung-1 deadlock, but its [hazard record](/tmp/boilreview/boil/design/work/6d-1.md:540) establishes where the parent waited, not why the child hung. Is this an explicitly accepted open investigation? Name its owner and required evidence.

The rung-4 codec hazard is properly traced and reproduced on the control. The end-gate record names commits and explicitly explains retained runs after the codec fix; the final commit changes only records/results.

## Verdict

fix first
---
Landing reviewer: codex, 2026-10-28 11:06 PDT; base 3c4e421c83f, head 2cfb3b7e909
