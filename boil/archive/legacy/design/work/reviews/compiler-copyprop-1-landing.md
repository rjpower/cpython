## Summary

The family-wide barrier, pre-lookup guards, and repaired reader analysis match the recorded fix.  
Landing is blocked by gaps in the specializer tests, invalid control provenance, and incomplete measurements.  
This was a source and artifact review; runtime checks could not start because `uv` cannot lock its shared cache in the read-only sandbox.

## Blocking findings

1. **The specializer tests assert receiver selection rather than the promised no-Python invariant.**

   **Where:** [test_specializer_runs_no_code](/tmp/boilreview/Lib/test/test_peepholer.py:1414), [test_specializer_type_lookup_runs_no_code](/tmp/boilreview/Lib/test/test_peepholer.py:1456), and the as-built “Rereading the tests” entries at [lines 563–564](/tmp/boilreview/boil/design/work/compiler-copyprop-1.md:563).

   Both tests record that a comparison happened while the target Python function ran. Neither distinguishes specialization from the subsequent generic lookup. Their result assertions can pass when specialization never runs, or when a specializer runs Python without producing the particular redispatch error being tested.

   That distinction matters because [uop_runs_code()](/tmp/boilreview/Tools/cases_generator/ir_generator.py:407) unconditionally exempts `specializing` uops. The required invariant is that specialization runs no Python code. Correct attribute results are a consequence of that invariant, not proof of it.

   The MRO test also omits the `__getattribute__` case whose failure is reported in the [negative-control record](/tmp/boilreview/boil/design/work/compiler-copyprop-1.md:433). Its committed cases cover class attributes, instance class attributes, `__getattr__`, and `__getitem__`.

   **Fix:** add an internal C fixture or C-only phase marker that identifies specialization at the comparison event. Force the specialization attempt and cache miss; assert no Python callback during that phase. Removing each guard must produce a witnessed callback and fail the invariant assertion. Keep the receiver-result assertions, add the claimed `__getattribute__` case, and state the remaining untested guard paths explicitly.

2. **The validation record does not identify a valid dict-guard negative control.**

   **Where:** as-built “Negative controls,” [line 429](/tmp/boilreview/boil/design/work/compiler-copyprop-1.md:429), and “Gate lines,” [line 404](/tmp/boilreview/boil/design/work/compiler-copyprop-1.md:404).

   The record calls `1f0f58d9f53` the tree “before ecaa09f9267” and says its build lacked the dict-hint guard. Git shows that `1f0f58d9f53` has `ecaa09f9267` as its parent, and its `Python/specialize.c` already contains the guard. No local removal patch is identified. The stated revision therefore cannot establish the claimed control.

   Gate 1 is also summarized as having run before each code commit, without individual tested revisions, builds, or results. The later merge is absent from the gate-survival explanation. I verified that production code is unchanged after `1bf1f941c88`; the imported test changes only alter documentation text, so this does not establish stale production-code gates.

   **Fix:** identify the actual guard-free source tree, including any local patch and the final test version used, and retain its failing assertion. Rerun the control if that evidence cannot be recovered. Add commit/tree-specific Gate 1 results and explicitly explain why the gates at `1bf1f941c88` stand through the landing merge.

3. **The measurement record omits a regression, contains a false comparison claim, and lacks allocation and box counts.**

   **Where:** as-built “Measurements,” [line 475](/tmp/boilreview/boil/design/work/compiler-copyprop-1.md:475), [line 505](/tmp/boilreview/boil/design/work/compiler-copyprop-1.md:505), and [PROCESS §3](/tmp/boilreview/boil/PROCESS.md:298).

   Recalculation from the committed JSON finds:

   - `deepcopy` rises **0.6044%**, from [3,883,713.9975](/tmp/boilreview/boil/bench/results/copyprop-base.json:247) to [3,907,188.3675 instructions per loop](/tmp/boilreview/boil/bench/results/copyprop.json:247). It is omitted from the attribution table despite “the others are within 0.3%.”
   - The claim that every `6d` instruction row is within 0.4% of the base is false: `getattr_property_float` differs by **1.0452%**, and `attr_load_poly5` by **0.7135%**.
   - The float-property comparison supports pre-existing variation in that row; it does not establish the cause of the separate integer-property increase. Likewise, the record explicitly leaves the coroutines RSS change unattributed.
   - Both new result files contain instruction and RSS measurements only. PROCESS requires allocation and box counts too; their omission is neither recorded as a deviation nor justified.

   **Fix:** correct the comparisons, include the omitted rows, and distinguish demonstrated causes from unresolved variation. Supply allocation and box comparisons against the before records. For unexplained changes, retain the uncertainty and obtain a named owner and coordinator disposition rather than claiming attribution is complete. No performance tuning is requested.

## Non-blocking findings

- The metadata change is focused: all **245** changed entries gain only `_PyIR_RUNS_CODE`; removing that addition makes the entire metadata file byte-identical to the base. Borrow masks, field roles, destinations, and feedback metadata remain unchanged.
- I found no added `PyObject *` field holding inline values, reference-counting operation, or change to shape/version rules. The guards precede the affected lookups.
- The allocation-time buffer-release exception and folded-reader root obligation remain explicit [PROCESS §0.3 hand-offs](/tmp/boilreview/boil/PROCESS.md:104). Test 10 is correctly described there as a semantic regression, not the missing-root oracle. These bounds remain open.
- The duplicate MRO helper has an owner and retirement condition in the [as-built hand-offs](/tmp/boilreview/boil/design/work/compiler-copyprop-1.md:584). The stronger allocation claim in the [pass comment](/tmp/boilreview/Python/regcfg_passes.c:163) should nevertheless name the recorded buffer-release exception.
- The blanket “every test runs 100 times” statement is inaccurate: tracing and monitoring run once, and the heap regression runs 20 times. Record these deviations and their reasons.

Every new or changed test has the following limits. The specializer gaps are blocking finding 1; the other limits describe the tests’ actual scope.

| Test | Failure it would not detect |
|---|---|
| [`test_call`](/tmp/boilreview/Lib/test/test_peepholer.py:1237) | Reclamation of a heap operand; the saved integer is inline. Other call-family members are untested. |
| [`test_property`](/tmp/boilreview/Lib/test/test_peepholer.py:1247) | A defect confined to C getsets or member descriptors. |
| [`test_getattr`](/tmp/boilreview/Lib/test/test_peepholer.py:1258) | A defect confined to an overridden `__getattribute__` path. |
| [`test_subscript`](/tmp/boilreview/Lib/test/test_peepholer.py:1268) | A defect confined to a native mapping slot. |
| [`test_reflected_binary_op`](/tmp/boilreview/Lib/test/test_peepholer.py:1279) | Operand loss across a collecting `NotImplemented` fallback chain. |
| [`test_comparison`](/tmp/boilreview/Lib/test/test_peepholer.py:1290) | Incorrect cleanup or operand preservation when comparison raises. |
| [`test_global_in_dict_subclass`](/tmp/boilreview/Lib/test/test_peepholer.py:1300) | A defect confined to another mapping path, such as `__missing__`. |
| [`test_global_colliding_key`](/tmp/boilreview/Lib/test/test_peepholer.py:1312) | Reentrancy during an immediate reader’s specialization. |
| [`test_under_settrace`](/tmp/boilreview/Lib/test/test_peepholer.py:1329) | Mutation performed by the tracer itself; mutation here comes from the called functions. It also lacks repeated instrumented execution. |
| [`test_under_monitoring`](/tmp/boilreview/Lib/test/test_peepholer.py:1364) | Mutation performed by PY_START or LINE callbacks; those callbacks only record delivery. Other event families remain untested. |
| [`test_specializer_runs_no_code`](/tmp/boilreview/Lib/test/test_peepholer.py:1414) | Python execution during specialization that does not produce the tested receiver error; it cannot identify the comparison phase. |
| [`test_specializer_type_lookup_runs_no_code`](/tmp/boilreview/Lib/test/test_peepholer.py:1456) | The same phase violation, plus the omitted `__getattribute__`, MRO store, class-construction, and statistics-only paths. |
| [`test_folded_operand_outlives_rebinding`](/tmp/boilreview/Lib/test/test_peepholer.py:1504) | A missing reader anchor: `self` and opaque C frames retain the operand. It does not witness collection in the unprotected interval. |
| [`test_fold_under_constant`](/tmp/boilreview/Lib/test/test_peepholer.py:1548) | Incorrect runtime arithmetic or operand lifetime despite correct source-slot metadata. |
| [`test_fold_under_local`](/tmp/boilreview/Lib/test/test_peepholer.py:1553) | Incorrect treatment of the second operand; only the first source is asserted. |
| [`test_fold_under_cell`](/tmp/boilreview/Lib/test/test_peepholer.py:1558) | Incorrect runtime cell loading or collection safety. |
| [`test_fold_under_tuple_and_is`](/tmp/boilreview/Lib/test/test_peepholer.py:1566) | Runtime tuple/identity errors or collection hazards; it checks operand placement. |
| [`test_no_fold_under_global`](/tmp/boilreview/Lib/test/test_peepholer.py:1574) | Reading the wrong value at runtime despite retaining a load. |
| [`test_no_fold_under_attribute`](/tmp/boilreview/Lib/test/test_peepholer.py:1579) | The corresponding runtime selection error across attribute execution. |
| [`test_runs_code`](/tmp/boilreview/Lib/test/test_generated_cases.py:1076) | Incorrect native-callee classifications; isolated return/yield cases and generated NOFB forms are absent. |
| [`test_borrowed_input_read_before_code`](/tmp/boilreview/Lib/test/test_generated_cases.py:1135) | Reentrant specialization and redispatch, or missing GC roots. Its specializer exemption is assumed. |
| [`test_runs_code_covers_every_handler`](/tmp/boilreview/Lib/test/test_generated_cases.py:1169) | A call omitted by the shared analyzer properties, including calls hidden in macros. |
| [`test_metadata`](/tmp/boilreview/Lib/test/test_generated_cases.py:1200) | Incorrect native execution or compiler consumption of otherwise correct emitted metadata. |
| [`test_load_folds_under_a_constant_only`](/tmp/boilreview/Lib/test/test_ir.py:1078) | Incorrect operand preservation under rebinding: its runtime cases never mutate the local. |

## Questions for the coordinator

- Can the original control log identify the actual guard-free tree, or must that control be rerun?
- Who owns any property or RSS variation that remains unexplained after the measurement record is corrected?
- Is postponing allocation and box measurements an approved scope cut? No such ruling appears in this record.

## Verdict

fix first
---
Landing reviewer: codex, 2026-10-28 17:43 PDT; base b57cb04c024, head bf6635fd0a4
