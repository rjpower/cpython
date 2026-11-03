## Summary

Reviewed `29a358b492d..e03ab939a4a`, including every changed C/header, handler, generated-code and test hunk.  
The slot conversion largely follows the design; four landing blockers remain.  
Static review only: no builds are present, and `uv` could not acquire its cache lock on the read-only filesystem.

## Blocking findings

1. **Allocation failure can leave ctypes holding a dangling pointer.**

   **Where:** [PyCData_set](/tmp/boilreview/Modules/_ctypes/_ctypes.c:3644) writes native memory before allocating the keeper and installing its traced reference. [Simple.value](/tmp/boilreview/Modules/_ctypes/_ctypes.c:5519) has the same order. If either operation fails, the temporary roots disappear while native memory retains the pointer. A later collection can free the box, making the next read unsafe.

   The **As built — ctypes OOM hazard** [explicitly acknowledges this](/tmp/boilreview/boil/design/work/6d-2.md:833), but defers the fix without an owner or retirement point.

   **Fix:** prepare the keeper and keep-reference update before publishing the pointer, or restore the previous native contents on every failure. Add failure-injection tests for structure fields, array items and `py_object.value`; return from the storing frame, collect under kill mode, and verify that the previous value remains usable.

2. **The recorded gates do not cover the landing head.**

   **Where:** **As built — Gates** records gate 3 at `bddc2ecdbc8`; the [final entry](/tmp/boilreview/boil/design/work/6d-2.md:906) explains carrying it across `facf5bf1ed6` and records gate 2 there. Subsequently, `e03ab939a4a` merges further code, including collector quarantine ordering changes and generator call-effect classifications. No entry covers that merged tree or explains why the earlier gates stand across it.

   The [gate-1 summary](/tmp/boilreview/boil/design/work/6d-2.md:759) also groups results by “each commit” and commit numbers rather than attaching each result to a SHA.

   **Fix:** run the release and level-3 debug gates on the final combined code commit, record its SHA and complete results, and map the earlier gate-1 claims to their tested SHAs. This follows [PROCESS §1, steps 4–5](/tmp/boilreview/boil/PROCESS.md:33).

3. **Promised safety tests are missing or do not establish the claimed failure point.**

   **Where:** **Tests 2, 5 and 6** promise [collecting conversion callbacks](/tmp/boilreview/boil/design/work/6d-2.md:414), descriptor mutation during collection, and failures at compatibility-result and native-field boxes.

   The added [member tests](/tmp/boilreview/Lib/test/test_capi/test_pyvalue.py:623) contain no collecting `__index__` or `__float__` fixture. The [wrapper OOM test](/tmp/boilreview/Lib/test/test_descr.py:6495) merely accepts a `MemoryError` somewhere in the call; it does not prove that the intended box failed. **Review round 4** [acknowledges two untested branches](/tmp/boilreview/boil/design/work/6d-2.md:676), but does not account for the remaining promised cases.

   **Fix:** add fixtures that collect during conversion and descriptor replacement/removal, plus deterministic failure injection at the relevant boxing boundary. Assert the exception, unchanged destination, absence of accidental deletion, and successful subsequent calls after roots unwind. Record any remaining deviations individually with their reasons.

4. **Measurement provenance and attribution remain incomplete.**

   **Where:** **As built — Instructions** [states that the extra nine instructions in `method_lazy_dict` are untraced](/tmp/boilreview/boil/design/work/6d-2.md:825), and that the 10a control measurements are uncommitted. The report compares primarily with rung 1, although **Measurements** specifies the before records.

   The committed [instruction results](/tmp/boilreview/boil/bench/results/6d-rung2.json:6) and [Py_STATS results](/tmp/boilreview/boil/bench/results/6d-rung2-pystats.json:6) identify different dirty source states. They therefore do not identify a reproducible committed source snapshot. The audit additionally [prints checkout HEAD](/tmp/boilreview/boil/tools/measure/box_audit.py:333), which does not establish the binary’s provenance.

   **Fix:** record the exact built source commit or preserved source patch, retain the audit output and 10a control records, and supply before/rung-1/rung-2 comparisons. Trace the unexplained instruction delta to runtime work or code generation and record the evidence.

## Non-blocking findings

- **The new span test needs an ASan outcome.** [test_span_box](/tmp/boilreview/Lib/test/test_gc.py:1956) always expects the collector’s trap text. Under ASan, [quarantine poisons the block](/tmp/boilreview/Python/gc_trace.c:2589), so the header read can report use-after-poison before that diagnostic. Follow the sanitizer-specific assertions already used earlier in this test file.

- **The record contains stale details.** [Tests 11](/tmp/boilreview/boil/design/work/6d-2.md:503) still budgets one allocation for the inline bound-method case; the [implemented budget](/tmp/boilreview/boil/tools/measure/box_audit.py:182) correctly allows two. The [C-mode hand-off](/tmp/boilreview/boil/design/work/6d-2.md:739) also describes the stdio classification fix as pending, although the final merge includes it. Reconcile both entries.

- The reviewed code preserves type invalidation before dictionary updates and subsequent slot updates. The descriptor boxing span holds raw pointers, and the temporary object-form bridges have named retirement rungs. I found no additional shape/version or reference-counting regression in these hunks.

## Questions for the coordinator

- Are there gate and box-audit logs for `e03ab939a4a` that have not been included in the record?
- Which 10a commit will install the receiver span when `method_vectorcall_*` becomes precise, and assert that span in the receiver-box test, as [the hand-off requires](/tmp/boilreview/boil/design/work/6d-2.md:692)?

## Verdict

fix first
---
Landing reviewer: codex, 2026-10-28 05:52 PDT; base 29a358b492d, head e03ab939a4a
