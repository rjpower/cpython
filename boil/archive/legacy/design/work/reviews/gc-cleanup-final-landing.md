# GC cleanup composite landing review

Disposition: **LAND** the reviewed composite at source/native/runtime `0b15b4bbc9b48313d9487f6b6a2911f7e7f4bfa3`. Actual integration and two postmerge smokes remain separate obligations. This is not a claim that full suites ran on the successor.

The final authority is [final-completion.json](../../../../../boil-gc-native-cleanup/build/gc-native-cleanup-broad-v2/final-completion.json), SHA256 `50054725c871a7d71a0eceb71db1d8373d335837aff8ed73e6dec208cb7a27eb`. All 382 absolute artifact references were independently rehashed. The previously reviewed terminal raw journals were replayed with the unchanged validators; closure logs, commands, receipts and postguards were checked separately. No reviewer native job or source mutation was used.

## Source and authorship boundary

The successor differs from `33944d24e551829a06a3c84c3c5e2c38d6acb463` in exactly two files: two shared generator-constructor origin/finalizer-slot NULL stores before GC tracking, one context comment, and a 27-line external origin/data regression. Later origin assignments and zero-depth branches remain unchanged. This fixes publication of a traced but uninitialized coroutine field while origin-tuple allocation collects. The regression arms stress1 only around construction, restores prior stress/depth, and checks origin metadata and usable returned data.

Independent native-cleanup, selective-category, warning-context and composition reviews remain applicable. Managed-scanner source approval belongs to the coordinator and independent source reviewer; this review does not self-clear its author's scanner implementation. This lane independently checks the operator's build, launch, raw-outcome and composite evidence, including the constructor fix.

## Explicit coverage assembly

| Source | Actual evidence | Outcome |
| --- | --- | --- |
| `33944d24` | O3 normal full; O1 normal full; O1 kill full | Each 51,255 tests; skips 3,795 / 3,739 / 3,739. Each roster: 462 PASSED, 94 SKIPPED, 8 RESOURCE_DENIED of 564. |
| `33944d24` | O3 stress11, off/N1000 | 3,115 tests / 148 skips; all 11 PASSED. |
| `0b15b4bb` | Four incremental build/audit capsules | GCC O3, GCC O1/Py_DEBUG, GCC Og/ASan/Py_DEBUG, Clang21 O1/Py_DEBUG tail: make and cleanup0; 116 modules / zero import failures each. |
| `0b15b4bb` | Origin regression and affected whole4 | Origin1/0 in O3 off and O1 kill. GC/generators/coroutines/asyncgen: 456/35 O3 off and 456/21 O1 kill, all four PASSED each. |
| `0b15b4bb` | Fresh tail ownership | Exact ownership1/0 in actual kill mode; 293 current dispatch handlers and ordinary native-clear body/section/covering CFI checked. |
| `0b15b4bb` | Corrected O1 stress11, off/N1000 | 3,116/113; all 11 PASSED; 38,251 recorded collections, zero fallback, 43 report-only snapshot visits. |
| `0b15b4bb` | Fresh ASan16, kill/N0 | 4,038/141; all 16 PASSED; 13,095 recorded collections, zero fallback, three report-only snapshot visits. |
| `0b15b4bb` | Closure4 | C extension16/exact7 skips; regeneration0; traversal0; exact19 borrowed-header receipts with four declared unsupported modules; outer0. |

The carried full3 and release stress are **339 results**, not successor passes. Their raw results and the original failed debug-stress profile remain unchanged. The approved local-initialization delta is covered by four fresh compilation/audit capsules, a byte-identical causal probe, the focused and whole4 runs, the previously failed debug-stress profile, ASan16 and closure. No full3 or release-stress repeat was substituted or implied.

The identical probe fails on sealed339 with the named coroutine-field fatal (native134), then succeeds on the fresh successor with origin and returned-data assertions (native0). The origin regression is protected and unskipped in focused, whole-module, stress and ASan coverage. Quiet broad journals establish selected, unskipped passing-module execution rather than individual success-ID records.

## Closure and retained identities

All four current original product maps were independently checked after terminal closure: debug594, release594, ASan588 and tail588 entries; configuration and tool hashes match, and no extra cache files were present. All 2,915 borrowed-header bytes and resolved aliases match the frozen closure plan. Serving source is clean at exact0b15. Old339 images remain independent, byte-verified retention copies under preservation manifest `7276117e`; they are not claimed runnable relocated builds.

Original linked-audit failures, the established ASan overflow-selection failure, the later host-invalid ASan smoke and its offline validation supplement, and the real339 debug-stress crash remain preserved. Corrected receipts do not rewrite those outcomes. The final index's expansion of a historical `.log` suffix into its absolute raw path changes only reference serialization, not the historical index or test execution.

## Limits and next gate

Scope remains Linux x86-64, main interpreter, shared GIL and ordinary threads, with existing unsupported-context and platform limits. ASan leak and fake-stack detection are disabled. Parent multiprocessing wrappers retain their actual skip scope and do not establish skipped leaf-process coverage. Isolated warning children carry no blanket descendant-kill claim. Report-only `unpinned` counts are valid detached-thread snapshot visits without a pinned C-frame extent, followed by conservative scanning; they are not object counts or nursery pin metrics.

The four unsupported header-check modules remain `_tkinter`, `_dbm`, `_gdbm` and `readline`; limited/stable C API and MSVC-dependent skips remain explicit. No universal precision, arbitrary RAW-hook safety, performance benefit or new score is established here.

No evidence blocker remains for this explicitly scoped composite landing. Keep serving source and capsules frozen. Bind the actual integration commit/tree and exact documentation-only difference, then run the two already reviewed normal off/N0 postmerge smokes once. Performance stays held until actual postmerge evidence seeds fresh29 origins and a separate quiet reservation is granted.
