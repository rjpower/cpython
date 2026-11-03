## Summary

The C fixes cover the reported heap edges; no value-layout, shape/version, reference-counting, handler, or generator changes appear.  
Two checker defects remain, and required full gates and before/after measurements are missing.  
Reviewed `72bed4aa822..a4b24326ed9`; execution was blocked by the read-only uv cache, and this checkout contains no builds.

## Blocking findings

1. **Delegation can credit traversal of another object.**

   **Where:** [traverse_audit.py:543](/tmp/boilreview2/Tools/boil/traverse_audit.py:543), consumed at [line 653](/tmp/boilreview2/Tools/boil/traverse_audit.py:653); contradicts [Review item 3](/tmp/boilreview2/boil/design/work/audit-2.md:149).

   **Problem:** `receivers()` treats any assignment whose right-hand side contains the original parameter as an alias of that object. For example:

   ```c
   PyObject *other = ((SideDictObject *)op)->other;
   return PyDict_Type.tp_traverse(other, visit, arg);
   ```

   It accepts `other` as the audited receiver and credits the audited object’s `ma_keys`, although the delegate visits another dictionary. The [negative fixture](/tmp/boilreview2/Lib/test/test_tools/test_traverse_audit.py:511) tests a direct member argument and misses this alias form.

   **Fix:** Accept only proven identity-preserving aliases, and invalidate them after reassignment. Add negative fixtures for member-derived aliases and function results; assert that the omitted base member remains reported and fails `--check`.

2. **Helper coverage depends on set iteration order.**

   **Where:** [traverse_audit.py:590](/tmp/boilreview2/Tools/boil/traverse_audit.py:590), especially the new callee set at line 601.

   **Problem:** Depth-first traversal marks a function permanently seen at its first encountered depth. Consider `root → {a, h}`, `a → h`, and `h → leaf`. If `a` is processed first, `h` is marked seen at depth 2 without expanding `leaf`; the later direct path to `h` at depth 1 is skipped. Thus `leaf`, which is within the promised two-call limit, disappears from analysis. The new set iteration can choose either order, changing findings and potentially making allowlist entries appear stale.

   **Fix:** Traverse breadth first, or track the shortest depth reached and re-expand on a shorter path. Add this diamond-shaped fixture and verify identical findings across hash seeds. This is separate from the acknowledged limitation concerning calls beyond depth 2.

3. **The rung has not completed its required landing gates.**

   **Where:** [As built → Gates](/tmp/boilreview2/boil/design/work/audit-2.md:243).

   **Problem:** The record explicitly defers gates 2 and 3 to integration. [PROCESS §1 step 3](/tmp/boilreview2/boil/PROCESS.md:89) and [§3](/tmp/boilreview2/boil/PROCESS.md:177) require the rung to finish through gate 3 before landing review. Avoiding collector, root, and layout changes limits the additional verification set; it does not waive the ordinary full suites, extension check, regeneration check, or applicable unbuilt-module check.

   **Fix:** Complete and record gates 2 and 3 at the corrected code commit, including root-counter results, regeneration cleanliness, and the required unbuilt-module check. Record any coordinator-approved exception explicitly. The integration quick-test remains a separate step.

4. **Required measurements and attribution are absent.**

   **Where:** [As built](/tmp/boilreview2/boil/design/work/audit-2.md:174), particularly the report totals at line 215.

   **Problem:** The record supplies final report counts and correctness results, but no before/after instruction, allocation, or box measurements required by [PROCESS §3](/tmp/boilreview2/boil/PROCESS.md:213). It also omits the audit-runtime measurement requested in the [design review](/tmp/boilreview2/boil/design/work/reviews/audit-2-review.md:34).

   **Fix:** Record comparable measurements with baseline and final commits, build identities, and commands. Attribute changes caused by the additional traversal edges and expanded helper search. Report unexplained deltas without tuning them away.

## Non-blocking findings

- **Gate provenance is otherwise sound.** The final targeted gates are assigned to `518e342d255`; the sole subsequent commit changes only the record. The earlier tooling run is covered by the final targeted rerun.

- **The Element allowance retains the corrected-away explanation.** [traverse_allow.txt:14](/tmp/boilreview2/Tools/boil/traverse_allow.txt:14) still ties aliasing to `length <= STATIC_CHILDREN`. After spilling and shrinking, storage remains external. Use `children == _children`, matching the as-built disposition.

- **The debug exception visit lacks its regression.** [scanner_traverse](/tmp/boilreview2/Modules/_sre/sre.c:3029) now visits `fail_after_exc`, but the [new scanner test](/tmp/boilreview2/Lib/test/test_re.py:1961) never replaces the pattern’s exception. Add a debug case that replaces it, drops other references, collects, and exercises the existing scanner.

- **Remaining checker limits need a definite owner.** [Hand-off item 3](/tmp/boilreview2/boil/design/work/audit-2.md:294) assigns “whoever next extends the audit.” Name an owner and completion conditions. Include the outstanding header-conflict and non-invoked-function-reference negative fixtures.

## Questions for the coordinator

- Who owns the remaining checker limits, and which rung will close them?
- What regression evidence will close buffer-release hand-off 1, particularly exactly-once release with a retained exporter and an allocating callback? The existing defect is traced and assigned, so I have not reopened it as a new blocker here.

## Verdict

fix first
---
Landing reviewer: codex, 2026-10-28 11:12 PDT; base 72bed4aa822, head a4b24326ed9
