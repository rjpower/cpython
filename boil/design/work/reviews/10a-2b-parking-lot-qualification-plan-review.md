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

## Round 3: NINETEEN_ONLY_PLAN_CLEAR

Independent source/artifact continuation, preserving the preceding review
at SHA256 `40a38bae78520c3e7c59b38aa7d84d9b1ab7277606a3782a4f890102f14cb66b`.
The coordinator accepted the twelve pilot receipts at clean ROOTS
`1bfe1a57145f8d32c27558a981023146333ee37d` under plan `26a55aa75ff0`.
This verdict clears only the frozen continuation plan for **nineteen new
six-transaction iterations per build**. It does not clear full/support
plans, O1 adoption, integration or landing. No native/build/test/probe
execution or ROOTS source/helper/plan edit was performed by this reviewer.

Artifacts below are relative to ROOTS `build/takeover-20261029/`:

| Artifact | SHA256 |
|---|---|
| `park-prerequisite/postfix-remaining/plan.json` | `0d84e7d24f5bdbbe720a2b27d98ebdabf66891d65f8c04556ec0d936991cf117` |
| `postfix_remaining.py` | `9ace5493e61c2051dc59d962ca619103337b9aaee43376d4e72310a6f08c5763` |
| `park-prerequisite/postfix-remaining/frozen-artifacts.json` | `d8fb6556320118a194e4468e1ecc97098d484860dab83e7fd588c541348d69d6` |
| `park-prerequisite/postfix-remaining/host-controls.json` | `b062bbff3ffb90d4bc6ecd3b9e6db13319f98d9219de37324619fd615d190824` |
| `postfix_remaining_controls.py` | `53dd7b84d8687e1bf62fe216167b3ef1378fec21896e8d0c10d6e6f005a59871` |
| Accepted pilot `postfix-pilot/complete-summary.json` | `4c609efa757eba1701140c05c6859328d17eac4ab8e2b72d7d0bd13641798cbf` |

I checked all thirteen frozen artifact hashes, all 48 iteration-zero
receipt/log/state/root paths, the pilot's 6,322 current source-map hashes
and all 542 current native paths. The native maps, job definitions,
protected set, source identity and six retained helper hashes equal the
accepted pilot's. The new wrapper imports that unchanged pilot's
current-1bfe `loaded()` for its complete source/helper/copied-input guard;
it does not revive an obsolete precommit/8ce guard. Its own plan/helper,
complete 48-path bridge, exact timeout/helper sets and pilot-derived
contracts are checked separately. No pilot helper, plan or receipt was
rewritten. Initial continuation drafts remain preserved.

The pilot supplies iteration zero once. New indexes are exactly 1–19,
with the five isolated selectors and complete four-module transaction
unchanged: 114 new jobs/build, plus the accepted six = 120/build. Each
individual still requires one test/zero skips; each whole-module job must
match the following **executed-pilot** statistics and the complete saved
`Counter((test_id, reason))` for each physical module:

| Module | Default tests/skips | Candidate O1 debug tests/skips |
|---|---:|---:|
| `test_gc` | 141/17 | 141/2 |
| Each of the three complete thread modules | 120/33 | 120/33 |
| Four-module total | 501/116 | 501/101 |

These values are now grounded in accepted runtime receipts, separately
from the earlier coincident 501-case discovery count. No set conversion,
skip-count-only allowance or guessed total replaces the ID/reason
multisets. Different order is allowed; missing/additional pairs, changed
IDs/reasons and multiplicity substitutions fail. The adapter derives the
contracts from immutable pilot receipts and checks that derivation again
before every transaction.

The existing full-v3 validation remains in front of that exact-contract
check: actual paired driver/worker off/extras0/stress0, exact module/filter
selection, PID/config/seed correspondence, authoritative outer-result
identity/statistics/skip events, all39 protected raw/outer/base/subtest/
class/module skip checks, valid nonnegative counter domains, main0 for
every required PID, positive collections and zero fallback. A matching
physical PID represented only by another interpreter still fails. Unpinned
is reported as 3c debt without root-row deduplication or lifetime/lineage
claims. Protected success retains the recorded loader/selection/count
inference; there is no new per-ID success event journal.

Every new invocation uses exclusive output creation and refuses any
existing log/root/state/result path. Failure records FAIL where possible
and propagates out of the build loop, with no automatic retry or resume.
The CLI has only prepare/run19, not a full/support armer. Per-job current
source/helper/native pre/post guards, fresh pycache, shared-cache policy,
gate_run slots/build locks and 1,800/1,200-second ceilings remain intact.
The two exact `run19 default` / `run19 debug` host-uv commands in the frozen
REVIEW may use the two existing slots; no same-build waiter is added.

Completion is stronger than reaching the end of a loop. `complete()`
requires every exact build/index/job receipt, head/plan/name/PASS/return
code and exact three-artifact path set; hashes each journal, reloads its
original state/root records and reruns both provenance validation and the
pilot contract. Stored summaries/contracts must equal those fresh audits.
Only after 114 valid receipts and final source/native guards does it create
an exclusive `complete-BUILD.json` containing receipt hashes. Its aggregate
counts cover the nineteen new iterations only; the pilot remains separately
bound iteration zero. Missing, failed or truncated partial rows cannot
produce that marker.

The retained host controls support those source paths: six named roster/
count/ID/reason/multiplicity corruptions reject; skip-order and duplicate
multiplicity positives pass; missing receipt, failed partial row and
truncated JSON each leave completion absent. A real debug four-module
journal passes with three explicitly synthetic provenance rebindings.
These controls were inspected, not rerun, and are not new native coverage.

There is no source blocker to this nineteen-only continuation. Preserve
partial failures and exact products/inputs throughout. Completion receipts
and raw evidence still need the coordinator's outcome review; full,
support-build/variant and O1 decisions remain separate. Do not hold this
bounded continuation for those unrelated future plans.

## Round 4: separate support MAKE_ONLY plan

### V1: REVISE configuration-input freeze

Reviewed make-only plan
`park-prerequisite/support-builds/plan.json`, SHA256
`7e257ecc7a1be55673b1b580c1de34250fb6ca8e12005b034ec29101c77d59db`,
adapter `postfix_support_builds.py`, SHA256
`30251b7f4d76951307724cec5a08885e67a324a70ec22c0963fcd2e8a72ac678`,
twelve-file manifest
`f92bfd18a22b32e913725fbb216e7ce84856b69f54061990bf4368fd4ac740f8`
and host-control report
`ae5cdae8750bb47506ce1b1fd83194a9156ef933bc529922ffa4a66403001e3c`.
Paths are relative to ROOTS `build/takeover-20261029/`.
All twelve artifact hashes, both old support native inventories (271
paths each) and the configured gcc/clang compiler binary hashes match.
No make, candidate interpreter, test or probe was executed by the reviewer.

One blocking gap: after make, `loaded(build)` rechecks clean source,
pilot products/helpers and CC identity, but does not require the support
Makefile and `pyconfig.h` to retain their frozen input hashes. The later
CONFIG_ARGS checks distinguish ASan/debug/tailcall families only; changes
to OPT/CFLAGS or other configuration could be captured as fresh products
rather than rejected. Native outputs are supposed to change, while these
configuration inputs are fixed. Add exact pre/post configuration equality
and a meaningful host-only wrong-configuration rejection before make;
preserve this first plan/adapter/control/manifest when refreezing.

The intended split is otherwise sound: fixed make-j4 under the ordinary
two-slot/build-lock runner; capture complete NEW products inside the lock
after make and before any qualification smoke/test; retain the outer log
and post-cleanup product/receipt checks. Bootstrap/fresh-interpreter build
tools are build execution, not test coverage. The fixed terminal
`-- -m test` host-wrapper marker requests existing slot detection only;
the CLI rejects other forms and has no smoke/test action. No shared runner
change, test-coverage claim, old ASan provenance waiver or runtime-mode
clearance follows. The missing-lock/unknown-build/no-smoke control reports
were inspected, not rerun. **No support make launch clears V1.** The
already cleared default/debug nineteen-only plan is unaffected.

### V2: MAKE_ONLY_SOURCE_PLAN_CLEAR

The narrow configuration fix closes V1's blocker. Reviewed frozen plan
`713409750bbf938b9f070c82ead3946192beb57f2c8ef6d7898e30943fe8033d`,
adapter `a6f4ba2bbbeae024b60d7e65c5518ece026b510aeab0e0b55eff89b0de833f13`,
`frozen-artifacts-v2.json`
`091c6213869147899997ad42b7b2f970b051eb197e7055d8f52682f2bf7353e4`
and `host-controls-v2.json`
`a16ad8ab44b103387a013fc88e520557a93eeab8d4f9c280d1702138375bd26e`.
The source of the seven host controls is
`support_build_controls_v2.py`, SHA256
`ea0584387775a71b39f4bf9f2db6c53a30fc8e8025fdb118ae7daa2799899433`.
All thirteen frozen artifacts match. Preserved `plan.json.v1` and
`adapter.v1.py` have the exact first-round hashes recorded above.

The only adapter change is `check_configuration`, called by every
`loaded(build)`: each support Makefile and `pyconfig.h` must match its
original plan hash. Thus pre-make, post-make and post-gate-cleanup guards
all reject configuration drift while permitting compiled outputs to
change. The four new controls alter only the host process's expected-hash
plan-read boundary, one build/file at a time, and reach the precise
`support configuration drift: ...` assertion before make. The original
missing-fd, unknown-build and no-smoke controls remain. These controls
were inspected, not rerun; actual plan/configuration/product bytes were
not changed by them.

I rechecked all 6,322 source inputs, the unchanged 542 default/debug
products, both old support inventories (271 paths/build), compiler binary
hashes and v2 frozen artifacts. Support symlink targets are separately
present in these inventories, and hashing each alias reads its target
bytes. The compiler/configuration/command/native-before maps are unchanged
from V1. The configured C compilers are gcc15 for debug ASan and clang21
for the release tailcall build; the frozen Makefiles retain their full
flags/options/source bindings. This source review executes no compiler,
make, candidate interpreter or runtime check.

Clearance is for the two exact `build asan` and `build tailcall` host-uv
commands in support-builds/REVIEW, with fixed `make -C BUILD -s -j4` and
1,200-second outer ceiling. Each uses the unchanged normal two-slot
gate_run admission and its own build lock. Before make, the inner process
checks fd8 against the actual build-lock inode and fd9 against an actual
normal slot file and obtains nonblocking exclusive flock on each inherited
descriptor. The gate shell holds those locks through the host wrapper's
post-make source/config/compiler/pilot-product checks, NEW product capture,
exclusive build receipt and subsequent cleanup. Its fixed slot-detection
marker is not a test invocation. No same-build waiter, shared runner edit
or production-source/configuration change is authorized.

Normal make's bootstrap/fresh-interpreter executions remain build steps.
On success, the complete NEW runtime-product map is captured under lock
**after make and before qualification**, in `BUILD.build.json`; the outer
`BUILD.launch.json` binds its hash and build log and rechecks source and
the resulting products after cleanup. Existing outputs prevent retry or
overwrite; make/source/configuration/cleanup failures remain failures.
The source/native guards also protect the concurrently running default/
debug matrix, whose sources and products remain frozen.

**STOP at that build receipt.** A later independently reviewed variant
adapter must consume this NEW map and run smoke with `GATE_NO_BUILD=1`,
plus actual native-kind/mode/assertion checks, before claiming any support
runtime coverage. This verdict clears no smoke, hazard, stale/walk, full,
ASan filter/variance, O1, integration or landing result. Current build
receipts and any failures still require coordinator outcome review.
