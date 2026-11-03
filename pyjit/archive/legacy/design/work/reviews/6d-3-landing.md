## Summary

The inspected call conversions preserve value results, raw receiver roots, and the monitored-result root.  
The landing evidence is incomplete: required gates and several promised test assertions are missing.  
This is a static review of `0f518b81236..085c289b908`; the read-only sandbox prevented `uv` from acquiring its cache lock.

## Blocking findings

1. **The recorded gates do not complete the required ladder.**

   **Where:** As-built [“Gate lines”](/tmp/boilreview/boil/design/work/6d-3.md:566), against [PROCESS §3](/tmp/boilreview/boil/PROCESS.md:163).

   **Problem:** The table records targeted release tests and full debug tests, but no full release suite, `PYTHON_GCSTRESS=1000` stress set, or twenty runs of each eval-loop probe. The ASan line is from `a725b9ab5f2` and explicitly stands “for every file but test_gc”; the later `test_gc` rerun at `ce1d3922b60` uses ordinary debug, leaving ASan coverage open after the verifier change. The committed `gate.sh` does not itself run these additional verification steps.

   **Fix:** Run and record the missing gates at the final code commit, including ASan `test_gc` under kill with the documented option and exclusion. Give every result its commit and explain any earlier result retained under the rerun rule.

2. **The prepend allocation-failure test targets an earlier allocation and permits unexpected success.**

   **Where:** [test_gc.py:2081](/tmp/boilreview/Lib/test/test_gc.py:2081), especially [2090](/tmp/boilreview/Lib/test/test_gc.py:2090); promised in work-doc Tests 5 and claimed in As-built Commit 1.

   **Problem:** `args` is a list. After `set_nomemory(0, 1)`, `m(*args)` must first create its argument tuple in [_MAKE_CALLARGS_A_TUPLE](/tmp/boilreview/Python/bytecodes.c:5152). Failure there does not exercise prepend’s [PyMem_Malloc failure branch](/tmp/boilreview/Objects/call.c:685). The test also prints `ok` if no `MemoryError` occurs.

   **Fix:** Arrange the argument tuple before arming the allocator; the existing `_testcapi.call_with_nomemory` helper supports this. Establish that the failed allocation is the prepend array, require `MemoryError`, and verify a subsequent successful call after restoring the allocator.

3. **The lifetime cases do not establish that argument referents survive collection.**

   **Where:** [test_gc.py:2075](/tmp/boilreview/Lib/test/test_gc.py:2075), compared with work-doc Tests 5 and note 12 §9.

   **Problem:** The forty-argument and `partial` cases contain heap objects, but the callee reads only `len(args)` and keyword names. It never reads positional contents or keyword values. These assertions can therefore succeed without proving the referents remain usable. The result cases also lack an explicit collection between receiving and inspecting the result.

   **Fix:** Collect inside the callee, then inspect every heap argument and keyword value. Explicitly collect before inspecting returned heap results. Retain kill/stress execution, and either isolate the scenarios as designed or document why they share one subprocess.

4. **The box audit omits the promised float matrix.**

   **Where:** [box_audit.py:205](/tmp/boilreview/boil/tools/measure/box_audit.py:205), [recorded results:52](/tmp/boilreview/boil/bench/results/6d-rung3-box-audit.txt:52), and work-doc [Tests 9](/tmp/boilreview/boil/design/work/6d-3.md:363).

   **Problem:** Only the callable-instance row gets a float variant. The `partial`, `itemgetter`, and `attrgetter` rows exercise integer results; the cache rows likewise omit float results. Identity tests cannot detect a transient box that is canonicalised before returning—the reason allocation budgets were requested. The new audit rows also lack corresponding before allocation records.

   **Fix:** Add float-result variants through the same `tp_call` paths and cache modes, with explicit box/allocation budgets. Record comparable before and after measurements, identifying both commits.

## Non-blocking findings

- **The C-mode record is narrower than promised.** [Tests 8](/tmp/boilreview/boil/design/work/6d-3.md:360) requires each changed function, but [the artifact](/tmp/boilreview/boil/bench/results/6d-rung3-cmode.txt:1) omits wrappers such as `wrap_unaryfunc`, `wrap_hashfunc`, and `wrap_richcmpfunc`, plus module adapters. Append their results or name and justify the reduced scope. Their remaining opaque classification is not itself a missing-root finding.

- **The handler comment overstates the boxing removal.** [bytecodes.c:5164](/tmp/boilreview/Python/bytecodes.c:5164) says inline rejection happens “without a box,” while As-built Commit 2 and the audit correctly record two eager boxes. Reword the comment to describe value dispatch accurately.

The kill-record failure and ASan fake-stack hazard have concrete causes and evidence in As-built’s hazard paragraphs. The checker-appeasing helper was removed. I found no changed native object field storing inline bits, new reference-counting behavior, or shape/version-rule violation.

## Questions for the coordinator

- Who owns retirement of the second starred-callee box? [As-built’s hand-off](/tmp/boilreview/boil/design/work/6d-3.md:689) explicitly leaves it unowned; assign a step and retirement condition.
- Are the missing verification runs available outside this record? If so, attach their commit-qualified results before landing.

## Verdict

fix first
---
Landing reviewer: codex, 2026-10-28 08:30 PDT; base 0f518b81236, head 085c289b908
