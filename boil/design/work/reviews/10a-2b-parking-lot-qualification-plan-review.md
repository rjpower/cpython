# ROOTS2b post-Park qualification scope review

Verdict: **QUALIFICATION_SCOPE_CLEAR**, 2026-10-29. This reviews the bounded
schedule and historical carry-forward ruling. It does not authorize execution
from an unfrozen plan, qualify the final native candidate, adopt O1 globally
or clear landing. No native command or owner-file edit was performed.

Reviewed ROOTS artifact
`build/takeover-20261029/park-prerequisite/postfix-qualification-proposal.md`,
SHA256 `4b87878d2372fd06fc036305f62a0d72117f1326b8f0fa832bc616d317973b33`.
The prior `ca275` proposal remains preserved as `.v1.md`. The source candidate
at review is merge `d8be2e2c36d8d7651b728598f165c9324ae128f4` plus six-file
patch `94d68f30d30935075d045c67a230eff0bbd6b192c32acec00c16fe341dd6bda5`.
The coordinator records the accepted policy in primary `a7d5d6f9e8d`.

The historical carry-forward is explicit and bounded. The legacy 31-selector,
11-module repeated results stay at frozen `8ce`; there is no new-head legacy
20/20 claim or general reuse rule. Correspondence artifact
`old-current-body-correspondence.json`, SHA256
`42350872f16d2c4dcb52f49a78eb7e8d36d6a1316c54fe2bb0078e696abd3c7f`,
binds the reviewed v3 source map. I independently compared existing Python
ASTs: test_gc and test_generated_cases are unchanged after removing their
one new method each. The git diff from `8ce` changes only three C/header files:
the documented parking header, Park/SemWait and the test-only gc_walk part.
The reviewed production diff changes only Park's classification and the
proved-equivalent semaphore feature selection. The ownership proof for all
seven callers and real-Park slot/classification controls support this ruling;
the fresh full chain remains necessary. Old full failures stay failures.

Inventory supplement `postfix-inventory-proposal.json`, SHA256
`e14c8e04eb05e67450989f7a98d8b18dcd43fd239d7b8c1bb8c117709ad93eb5`,
binds the preserved case listing
`be1d4fab40e05e27bcda1e84ec20604e1392958a9536992d2ba9a398d5cd27af`.
I checked all 39 protected IDs are distinct and present in that listing:
the exact old 32, three thread-pool selectors, both eval-probe methods, the
Park fixture and Windows checker test. The older listing report's provisional
38 is preserved and supplemented, not silently rewritten. There are 2,467
unique listed cases across 15 modules. The fresh four-module set lists
`141 + 120 + 120 + 120 = 501`; these are discovery counts, not successful
execution or allowed runtime-skip counts.

Required fresh scope agrees with the coordinator's ruling:

| Scope | Fresh evidence required |
|---|---|
| Repetition, each default and O1 debug build | Each of three thread-pool selectors and both eval methods 20 times in separate fresh workers: 100 individual transactions. Twenty transactions each run the complete three thread modules plus test_gc, four actual workers in normal module order. Total 120 transactions/build; eval methods retain their unchanged internal 20 loops. |
| Pilot stop | Freeze and review the first iteration's five individual selections plus one four-module transaction per build before authorizing the other 19. Derive actual counts, exact skip IDs/reasons and phase/PID/config/seed contracts from this pilot. Do not use 501 as a runtime count threshold. |
| Changed checker suite | Whole test_generated_cases once on default, debug and actual debug kill. Its 65 listed cases were outside the old 11-module matrix; narrow Windows/parser controls do not replace this run. |
| Root/classification controls | Current source/negative checks, full semaphore-body and linked inline audits, real-Park positive/negative/restoration and compiled/detached-register controls. Strict six-case stale100 on release off and debug off/actual kill. |
| Supported ASan and tailcall | Fresh rebuild/source/product bindings. ASan14 plus sort/functools completion under kill/N0 with exact accepted options and allocator/remote selectors; no inherited native-inventory variance. Tailcall gets fresh bounded stale/walk support. Full once-only tail/retain §8.2 remains 10a4/H1 work. |
| Full/verification ladder | Fresh release off full, O1 debug off and kill full, stress11 on each build at N=1000, and once-only filtered release full at N=100000. Preserve exact four approved stress selectors plus remote; no B/test_misc or ASan-only stress exclusion. Existing ceilings and progress check remain. |
| Gate3 closure | cext16 run, including its seven exact existing skips; regen-cases/clinic content equality; traversal audit; 19 successful optional syntax-check receipts with compiler/header bindings and four unsupported-header limitations explicit. |

Full profiles remain serialized under the existing two-slot/build-lock
scheduler with fresh caches and fixed timeouts. New complete source/native/
imported-helper plans must follow the clean fixup and any integration source
bridge. All selected protected IDs require their planned success; reject
protected base, subtest, class or module skips, missing workers/results,
empty statistics, invalid counter domains or missing main0 cohort coverage.
Use authoritative outer-result skip multisets, retain inner diagnostics and
raw results, and require zero fallback. Unpinned remains report-only 3c outside
strict stale100. These are build-owned adapters; no shared run/gate hook is
claimed. O1 adoption requires the completed new-source evidence and a separate
coordinator ruling. No new benchmark gate is introduced; gate4/checkpointI
remain at their established boundary.

Supporting bounded evidence remains separately named: summary
`632414b6cc84c80be91c64f4e3ee42fe36b14ef2f6f2fa035d030b7db4651816`
reports 71 tests/zero skips, 5857 collections/zero fallback/unpinned; isolated
classification report
`db936988ab4c367e847b98e4e1bcbc8ca50b64c0a124adbfcfa2c53890de1c17`
records the same positive fixture before CORE override, its exact Park-class
failure with caller slot/capture/fallback intact, and successful restoration.
Neither is a final repeated-matrix or full-suite receipt.

No exact new execution plan or pilot receipt was frozen at this checkpoint.
Their independent review remains open; no full profile or remaining 19-repeat
launch follows from this scope verdict.

## Round 2: clean fixup and tested-source bridge

Independent continuation by `stats_design_review`, source/artifact reads
only. The preceding scope review is preserved verbatim; its original
SHA256 is `bcc3b44e8fccfb648cb649cc27b5566cdfd7f87f8e90e4c747f1c77e0d393496`.
No candidate interpreter, build, test or probe was run, and no ROOTS
source/helper/plan was edited.

Verdict for this subsection: **CLEAN_FIXUP_BRIDGE_CLEAR**. This binds the
existing bounded evidence to the clean commit; it is not a repeated/full
qualification result, O1 adoption or permission to execute an unreviewed
pilot plan.

Reviewed clean ROOTS HEAD
`1bfe1a57145f8d32c27558a981023146333ee37d` and
`park-prerequisite/postcommit-tested-bridge.json`, SHA256
`604ef3fb44c0cbda20727e10357c4b94f88023bea8540dd24572d065596cfd85`.
The actual six-file source/test diff from merge `d8be2e2c36d8d7651b728598f165c9324ae128f4`
hashes to the already reviewed patch
`94d68f30d30935075d045c67a230eff0bbd6b192c32acec00c16fe341dd6bda5`.
The implementation-v3 manifest remains
`b82a7c2a14a121d3ec98063a50f096719454462807177fef24ca74d8ee554821`;
all six current inputs match its hashes.

The bridge names precommit bridge
`2999ac96f3673712e312b7c1ebba963c0fc927d645c53c05659f217c2aeb338f`.
Both native-product maps are identical, and every current product hash
matches: 271 paths per default/debug build, 542 total. Every one of the
precommit bridge's 6,322 source-map inputs matches the current source after
substituting exactly its two declared documentation revisions:
`boil/design/11-precise-roots.md` and `boil/design/work/10a-2b.md`.
Both declared current documentation hashes also match. Source status is
clean. These are independent read-only byte comparisons, not new execution.

Old HEAD-bound plans remain historical; their loaded-source guard must not
be relabeled as a guard for this commit. A new complete plan must bind
current source, native products, imported helpers and the bounded scope.
The required first iteration remains five fresh individual transactions
(three thread cases and both eval methods) and one complete four-module
transaction **per build**, then STOP for coordinator review of actual
counts, authoritative skips, root/state witnesses and elapsed time.
The remaining 19 iterations and full chain stay held. The frozen pilot
adapter/plan review follows separately; no pilot execution is cleared by
the bridge alone.

### Round 2 frozen pilot plan: PILOT_SOURCE_PLAN_CLEAR

The new pilot plan and adapter clear source/artifact review for the
coordinator's **first iteration only** on default and candidate O1 debug.
No native pilot has been run by this reviewer. The remaining nineteen
iterations, full profiles, support-build plans, O1 adoption and landing
remain outside this verdict.

Artifacts below are relative to ROOTS `build/takeover-20261029/`:

| Artifact | SHA256 |
|---|---|
| `park-prerequisite/postfix-pilot/plan.json` | `26a55aa75ff08c8c31bc6196ed2d8784757053a77212216883295394ab0fd608` |
| `postfix_pilot.py` | `1d62448e4c88641bf2e0791159a9b3f774c59ef1fbbfe1bcecc9aae2468e00b1` |
| `park-prerequisite/postfix-pilot/frozen-artifacts.json` | `00e2302ab22598d812b17bd2563bece229fb95e7e667745d61b49694e2db3f34` |
| `park-prerequisite/postfix-pilot/host-controls.json` | `25b76e7fd4fe2e51c61fd89371cef5f8288825631a875649da6514ae2f797b3d` |
| `postfix_pilot_controls.py` | `f54a672b6725c36b1be0aca417ff91d047fa97da3e3a8ee22c1839d2268d4ab2` |

I independently checked all fourteen frozen artifact hashes, every one of
the plan's 6,322 source-map hashes, all 542 native paths and the two retained
state/root inputs to the host controls. The plan's native maps exactly
match the clean-commit bridge. The five reused helpers are unchanged:
`full_assert_state.py`, `full_worker.sh`, `full_result_observer.py`,
`full_contract_v3.py` and its transitive `module_contract.py` import.
The plan has its own current-source `loaded()`; no old HEAD-bound loaded
guard is imported. Copied registry/inventory/protection/bridge/policy
inputs are frozen; registry availability is not promoted.

The exact admission is the two `pilot default` and `pilot debug` commands
in `postfix-pilot/REVIEW.md`, using the host uv/Python wrapper and shared
UV cache. Each CLI iterates its frozen six jobs once, refuses existing
outputs and stops on failure. It provides no repeated/full mode. Every
child runs through existing gate_run slots/build locks, a fresh pycache,
1,800-second outer ceiling and 1,200-second worker timeout. Distinct builds
may use the existing two slots; do not queue a second same-build pilot.

Each build gets five isolated `-j1` transactions: one exact selection for
each thread-pool regression and each eval method, with one actual test and
zero skips required. Both eval methods keep their existing internal loops.
The sixth transaction is `-j8` with the four complete physical modules,
one whole module per actual worker, normal module order and no positive
match filter. The only negative selector is the established machine remote
case. This is six transactions/build now, toward the separately ruled
120/build schedule, not a new 120-job launch.

All 39 protected IDs are unique and present in the copied inventory.
Four-module coverage contains 14 of them, and each individual job selects
its exact protected case. The complete 39-ID set still guards raw and
authoritative outer skip events. Base/subtest/class/module skips are
rejected; all four returned modules must be PASSED with nonempty valid
statistics. Actual worker PID/config/seed/module/filter and paired
driver/worker start/end off/extras0/stress0 witnesses must reconcile.
Authoritative outer-result tokens, statistics and skip multisets must
match actual returned results; inner events stay separate. Root domains
are checked before addition, every required PID needs main-interpreter
counter evidence, collections must be nonempty and fallback zero.
Unpinned remains report-only outside the strict stale controls.

`selected_protected` is descriptive selection metadata, not a separately
consumed per-test success journal. The validator uses the entire protected
set for skip rejection. Protected-case execution is inferred from the
unchanged actual loader, exact module/selector filters and returned outer
counts (one test/zero skips for each individual), with complete-module
selection for the four-module transaction. It is not an explicit event
receipt naming every successful protected test ID. This is sufficient for
the bounded pilot review; do not advertise stronger per-case event evidence.
The main0 requirement also rejects an otherwise matching physical PID
represented only by foreign/subinterpreter root records; a PID alone is
insufficient, as the retained corruption control demonstrates.

The eleven host-only corruption receipts agree with their frozen driver:
missing schedule/result, outer-config mismatch, empty selection,
subinterpreter-only PID evidence, negative counter, selection narrowing
and protected base/parameter/bracket/class skips are rejected. Their
positive uses a retained real bounded fixture with four explicitly
synthetic provenance rebindings. This establishes validator behavior,
not new runtime or thread-pool coverage; the controls were read, not rerun.

The four-module listing's 501 remains discovery information. The pilot
retains exact observed per-module counts, outer skips/reasons, PID/seed/
configuration, complete journals, hashes and elapsed time without applying
501 as a runtime threshold. **STOP after these six jobs per build.** The
coordinator must review those actual results, legitimate skip allowances,
state/root witnesses and turnaround before any remaining nineteen. Any
failed or skipped protected case remains a failure; no filter or automatic
resumption is approved. Full normal/kill/ASan/stress/extras plans require
their subsequent frozen review and remain held.
