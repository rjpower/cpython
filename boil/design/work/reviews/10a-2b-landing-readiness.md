# ROOTS2b landing-readiness inventory

Read-only evidence inventory, 2026-10-29, through the debug matrix/main0
handoff at 18:08 UTC. This is not a landing verdict or a new source review.
No native commands were run and no build-owned helper was changed. ROOTS
remains clean/frozen at `8ce9c57c662bf3a027d01cd0a8bfc45d3d4fb89b`.
Normal debug full has started; the remaining full profiles and gate3 extras
have no completion receipt at this checkpoint. Do not wait for them or infer
their outcome from this inventory.

Follow-up received during handoff: **NOT READY**. The active debug full
reports failures in fork and forkserver `test_threads` modules,
`WithThreadsTestPool.test_release_task_refs`, at _test_multiprocessing.py:3442:
three CountedObjects remain weakref-live. The suite continues unsignaled;
its armer will stop on nonzero completion. ROOTS owns diagnosis. The matrix's
WithProcesses selections remain valid for their exact scope and do not
cover this thread-pool case. Preserve this runtime failure; no exclusion,
optimization rollback or unchanged-source qualification success follows.

All artifact paths below are relative to the ROOTS worktree's
`build/takeover-20261029/`. Current primary PROCESS, WORKLOG and
reviews/10a-2b-full-harness-review.md supply the coordinator's rulings;
the ROOTS work doc and older draft plans contain historical requirements
that need reconciliation in the final as-built record.

The final matrix plan is
`17a8ba8322e01a3f43a3e981adfc9a8e464d3c3f36e1f016e911d4f05893147c`.
I checked all 6,305 source-snapshot file hashes, all 271 native product
paths per default/debug build, both inventories, the expected contract,
four matrix helpers and eleven full-v3 helpers against the retained plans.
All match. The source snapshot digest is
`4b2e7fa4863445ce63a426de5aa61b524353bd5c56d76130b0d0f924cf95d730`;
the plan's source-identity digest is
`8405feb48b29301ec65221b26167d13549824b69f6fa4f1d386604847cf55228`.
Default/debug Python hashes are respectively
`76a7fa5ab8b72de114c0909e4bc6f1b4578efa3823f992bfb281ef7f063ddd97`
and `5df03460168cd1b510ba559fff9e058bf7510f74bc8d107d803943c65373b60e`.

Accepted evidence to carry forward:

| Obligation | Retained result and limits |
|---|---|
| Release twenty-run reclamation matrix | `final-qualification/default-summary.json`: 640 jobs = 620 individual selections plus 20 complete 11-module runs; 41,440 tests, 1,660 allowed skips, 330,157 recorded collections, zero fallback. Each whole-module run has 2,041 tests/83 skips. The contract protects 32 selectors, including retired finding B, independently of allowed skips. |
| Debug O1 twenty-run matrix | `final-qualification/debug-summary.json`: 640 jobs, 41,440 tests, 1,360 allowed skips, 374,245 recorded collections, zero fallback. Results hash `5a561e5b2ac622c0498f526b153a3765123c16713811b259ca964554d76f07e2`. This qualifies the candidate matrix, not global O1 policy. |
| Matrix main-interpreter/counter supplement | Release replay `99e1c5cab4c74a97a0499c00c9450a2380b9e8857ea12f270ec3e9ea40f27589`; debug replay `07d7f4455aa3da053fc95e924741b283d4c28c8ff484937f8700817e285ca1e0`. Each binds 640 retained jobs and 1,480 required PID occurrences, requiring main interpreter 0 and valid nonnegative integer counter domains. These are read-only supplements, not additional runtime coverage. |
| Fresh release full-v3 | Receipt `195c64bee179667ab1b71c08f7b5c122da5c8cf9ce4dfbee1b0e178092d2903d`: 51,130 tests, 3,458 authoritative outer skips, 67 separately classified inner skip events; 564 modules, 565 actual driver/workers, 63,597 recorded collections, zero fallback. Raw returned totals and main0 cohort reconcile. Outcomes are 462 PASSED, 94 SKIPPED and 8 RESOURCE_DENIED, not 564 passing modules. |
| Fresh release stress11 | Receipt `68feb053300b33f745379cc5fe59e6436e4c82139b61eedbfbe3d2089175cbf4`: 3,061 tests/100 skips, all 11 modules, 12 actual driver/workers at off/stress1000, 44,828 recorded collections, zero fallback. Raw returned totals and main0 cohort reconcile. |
| ASan hazard and completion sets | `settled-asan14-summary.json`: 3,616 tests/91 skips, 14 modules, 15 actual driver/workers at kill/stress0, 16,712 collections, zero fallback. Separate sort/functools completion: 359 tests/one `_decimal` skip, three actual driver/workers, 124 collections, zero fallback. Both evidence indexes and adapter hashes match. Exact options are `detect_stack_use_after_return=0:detect_leaks=0`; this is kill coverage, not ASan stress1000 coverage. See the provenance deviation below. |
| Source/checker/compiled controls and repairs | Settled 41-file C check and five old/current checker controls retain their exact diagnostics; current rejects all five. Inline audit, strict stale100 off/kill, release/tailcall v3 stale/walk controls, carrier-origin/list-transfer negative controls, actual cleanup-helper old/fixed controls and shared-heap20 per build remain relevant. Reconcile any earlier source stamp to unchanged functions/current artifacts; do not replace them with historical broad-suite counts. |

Both small `*-complete.json` files have the same SHA256
`d68e077bd9d63a41ce9df0bb1f742d0061f5ce65ba9207fa63cd2b04b5dc8362`:
their content is only complete/jobs/global-plan. Build-specific result files,
summaries and product bindings distinguish them. `*-qualified.json` alone
records the first individual/module qualification and is not completion.

The full-v3 plan is
`14739d4144d25df076bded0fa5a174a8ffbcb737fbd70058d30d07b3ab7c6827`;
validator `7a5c69a7a96f951d3a895fa562f11a5063f9b3a506fe2036a841f93bbe4f1945`.
The earlier v2 full run remains native SUCCESS / qualification FAIL. Its
global addSkip hook could not distinguish 67 inner meta-test skips. No
retrospective threshold or module exception repairs that witness. V3's exact
scheduled outer-result identity, weak monotonic tokens, phase/worker binding,
protected raw/outer skip checks, nonempty statistics and main0/domain checks
already cleared review and controls. Preserve the original attempt and all
intermediate plans; do not reopen those resolved semantics by counting only
raw addSkip events.

ASan14 has a narrower native provenance contract than the later completion
run: its plan/index records no pre/post executable or CAPI byte inventory.
Actual worker witnesses bind source, Makefile, sys.version, CAPI origin,
mode/stress and the run/selection. The later completion plan binds 271
current native paths. I independently checked every current byte hash and
file/alias/resolved-target ctime and mtime against that manifest: all match
and all timestamps predate the ASan14 plan start. Latest observed metadata
is epoch `1793289387.9808702`, or **2026-10-29 15:56:27.980870 UTC**;
ASan14 plan start is epoch `1793290078.320878`, or
**2026-10-29 16:07:58.320878 UTC**. The owner reports no intervening ASan
build/config/output writes on frozen8ce; the preceding unfiltered gate log
retains its rebuild and failure. Coordinator explicitly accepts this bounded
operational provenance deviation, with no rerun absent an identity conflict
or source change. This is not a retrospective cryptographic pre/post manifest.
Use the stronger inventory prospectively.

Remaining handoff checklist, with no native work requested by this review:

1. Obtain final v3 receipts for `debug-full`, `debug-kill-full` and
   `stress11-debug`. The frozen guard/armer requires completed debug640,
   its main0 supplement and `candidate-after20`, then stops on any failure.
   Normal full/kill use the complete 564-module roster and only the existing
   remote selector. Stress11 uses the exact four stress selectors plus
   remote, off/stress1000. Debug O1 remains a candidate; integration remains
   Og until the coordinator rules on complete evidence.
2. Complete the approved once-only `stress100k-full` on release after the
   debug full chain, under the existing full-v3 plan. It uses the complete
   roster, off/stress100000, unchanged writer-resize, Writer, unicode
   from_format and test_verify_kill filters plus remote; no test_misc/B or
   ASan exclusion. The ceiling is 7200s/1200s per worker, with the approved
   progress checkpoint. Its filtered success will satisfy the scheduled
   obligation, not retire writer/compiler root debt.
3. Freeze/review the separate gate3 extras commands and witnesses before
   execution: actual `-u cpu test_cext` (16-test contract), regeneration plus
   Clinic with unchanged complete source/status/diff content, traversal
   `--check`, and `check_unbuilt.sh` with the declared source/build/headers.
   Full-suite test_cext without cpu does not substitute. The existing draft
   proposes a sequential cext driver; do not append a multiprocessing-only
   `--python` argument to it. Bind whichever approved sequential or isolated
   worker command is actually selected. These extras are not yet qualified.
4. Record optional-module limits exactly. The current build reports ten
   missing modules. Header-only checks are not linked/runtime coverage;
   check_unbuilt's footer also names `_tkinter`, `_dbm`, `_gdbm` and readline
   unchecked. Preserve affected caller review and named limitations instead
   of claiming all optional extensions passed.
5. Close the as-built record: exact gated source/commit and any docs-only
   landing difference, all final plan/helper/native hashes, counts, skips,
   modes, exclusions/removal owners, failure dispositions and remaining
   3a/3b/3c work. Keep qualification-copy registry promotions separate from
   primary availability. Final candidate/source review and integration
   gates remain required; this inventory does not approve the native diff.
6. Keep measurement scope explicit. PROCESS's level4 table assigns the full
   internal benchmark set/step label and attribution to **step end**, not
   every rung; this adds no ROOTS2b gate4 blocker. Preserve the named existing
   item1b/3/item6 rung measurements and identify their actual before/after
   coverage under PROCESS's intermediate-recording rule. Matrix turnaround,
   stressed startup and debug timing are not release performance results;
   do not replace the record with a claim that a fresh full benchmark set
   has run under concurrent gates.

Carry forward these explicit limits. Unpinned is report-only outside strict
stale100: release/debug matrices report 42,466/65,072 visits; release full
2,889; stress11-release 6,519, including 13 driver visits without exact
attribution. Do not call all of them interpreter swaps or claim precise
detached-state roots. Root-stat rows are finalization events, not necessarily
unique physical processes or `(pid,interp)` lifetimes: repeated embedded
initialization can reuse the same pair. Do not deduplicate recorded totals
or assert exact child-fixture lineage that the witness lacks.

The unfiltered ASan gate0 remains FAIL at the known allocator
AddressSanitizerTests.test_buffer_overflow oracle; the successful hazard run
uses only its approved exact selector. Once-only full tailcall8.2 and release
retain diagnostics remain assigned to 10a4/H1 by explicit PROCESS §0.1
handoff, not new2b blockers. Retain requires its own oracle. Shared
verify.py run/gate orchestration is unfinished; build-owned qualification
does not implement it, and the targeted shared launcher's global skip hook
needs separate correction/qualification before future full-profile use.

## Current source preflight, ce2ac versus primary57b5

2026-10-29: **SOURCE_PREFLIGHT_NO_NEW_BLOCKER**, with final qualification,
as-built documentation, exact candidate and integration review pending.
This source-only preflight does not replace the historical failures/results
above and is not a LAND verdict. No candidate command, compiler, test, probe,
checkout or source mutation was performed.

Compared primary `57b5a61a6f26917d8e4878527c396d00cae58198` with frozen ROOTS
`ce2ac8991eebb1bbb10698ff2d07a79bd4cba452`. The selected complete delta below
has binary Git-diff SHA256
`702d075d7cf8e86b2d8128eb52b4aab800ed85106235e00a2bd0a2a3a5c82bcb`.
It contains 62 Include/Objects/Python/Modules/Lib/Tools paths, Makefile.pre.in,
six boil/tools paths and InternalDocs/values.md: 70 total, or 69 excluding
InternalDocs. The earlier 67-path handoff used a different H3 merge-base and
path domain; do not reuse its count for this primary comparison. The three
opt-in diagnostic additions abortbt/cstack/opaque_inliners are retained by
coordinator ruling; they are neither gates nor reliability evidence.

This bounded check builds on the existing 2b/checker, cleanup, Park source/
implementation and raw-mutex reviews and their retained falsifying controls.
The central contracts remain coherent:

- Parameter/result spans use initialized raw slots and pop before freeing
  their backing buffer. `_PyStack_KwNames` owns its names during allocation;
  its three call paths size and rebase their span before later collections.
  `_PyValueSpan_ObjectSlot` gives object out-parameters an actual raw root;
  checker invalidation/merge/pop logic and negative fixtures protect that
  contract rather than treating arbitrary PyObject** casts as anchors.
- Public-header precise marks move to pyport with a core/ELF/compiler guard;
  generated and explicit helper interfaces change together. The unused
  `_Py_Check_ArgsIterable` callable parameter is removed at declaration,
  caller and generated uses. No H1 frame convention or H3 schema change is
  smuggled into this source delta. Their later union still needs revalidation.
- Park's seven production callers are four lock/event/once/RW sites in lock.c,
  SimpleQueue get, thread-state suspension and interpreter-finalization wait.
  They remain opaque. Their address/expected storage survives the complete
  wait; SimpleQueue's object-bearing output is the caller's writable `item`
  slot, covered by its opaque pinned stack until reattachment. Park holds no
  Python object roots. A future precise queue-get caller needs an explicit
  output root; neither this repair nor the held Queue2 work grants that change.
- Park alone becomes precise. The two raw mutex wrappers have unchanged
  scalar/pointer bodies, exact precise markers and reviewed slow-path/platform
  closure. The semaphore refactor chooses one monotonic or realtime wait;
  its retained feature/timeout/error controls preserve errno/result behavior.
  Real-Park caller-slot/negative classification controls and actual linked
  CFI/root-location comparison remain distinct evidence.
- PyMem attribution supplements B using allocated carrier edges with
  per-origin cycle/alias deduplication. It does not replay arbitrary traverse
  graphs or change real marks. `_PyList_AsTupleAndClear` allocates/copies while
  the list still visits its items, then clears size. These repairs have
  retained carrier/list-transfer controls; report origin order is not fixed.
- Multiprocessing test cleanup drains finalizers after forkserver stop and
  before resource-tracker stop, preserving both socket and unregister ordering.
  Earlier thread-pool and standalone-finalize failures remain retained; they
  are not erased by the currently fresh affected/full campaigns.
- The removed source decorators restore real reclamation assertions. Current
  full-v3 protection rejects skips of the 39 protected selectors/containers;
  source retirement is distinct from historical matrix coverage. Writer,
  compiler-metadata, ASan allocator and remote-exec limitations remain exact.
  `collect` stays explicitly outlined below walk entry; queue `run_queue_ex`
  remains opaque for its saved exception/live payload. Nothing here retires
  3a/3b/3c, zero-init, transitive PyMem or other conservative protections.

One nonblocking bookkeeping remnant is `OUTLINED_OPAQUE['vectorcall_call_dict']`:
that function is now merged into precise `_PyVectorcall_Call`, so the old
entry names no current function. It supplies no current native bypass. Treat
its stale explanation as later checker inventory cleanup, not a reason to
rewrite frozen source during qualification. The narrowed hashtable effects
now leave callback-bearing operations collecting, strengthening that boundary.

The final review must bind the exact as-built candidate/docs delta and fresh
seven-profile, extras and conditional tail-walk receipts, including failures,
skips, actual kinds/state, product inventories and report-only unpinned limits.
Old1bfe120 remains historical with the accepted ce2ac native bridge. O1 policy,
landing and primary integration remain coordinator decisions after that review.

Complete selected path inventory:

```text
Include/cpython/listobject.h
Include/cpython/pylock.h
Include/cpython/tupleobject.h
Include/internal/pycore_call.h
Include/internal/pycore_ceval.h
Include/internal/pycore_dict.h
Include/internal/pycore_gc_trace.h
Include/internal/pycore_gc_walk.h
Include/internal/pycore_interpframe.h
Include/internal/pycore_lock.h
Include/internal/pycore_object_alloc.h
Include/internal/pycore_parking_lot.h
Include/internal/pycore_pystate.h
Include/internal/pycore_tstate.h
Include/internal/pycore_value.h
Include/pyport.h
Include/pyvalue.h
InternalDocs/values.md
Lib/multiprocessing/util.py
Lib/test/_test_multiprocessing.py
Lib/test/test_asyncgen.py
Lib/test/test_capi/test_watchers.py
Lib/test/test_context.py
Lib/test/test_ctypes/test_callbacks.py
Lib/test/test_descr.py
Lib/test/test_gc.py
Lib/test/test_generated_cases.py
Lib/test/test_memoryview.py
Lib/test/test_set.py
Lib/test/test_tools/test_gc_inline_audit.py
Lib/test/test_weakref.py
Makefile.pre.in
Modules/_ctypes/cfield.c
Modules/_testinternalcapi.c
Modules/_testinternalcapi/gc_walk.c
Objects/call.c
Objects/cellobject.c
Objects/classobject.c
Objects/descrobject.c
Objects/dictobject.c
Objects/exceptions.c
Objects/genobject.c
Objects/listobject.c
Objects/methodobject.c
Objects/moduleobject.c
Objects/object.c
Objects/obmalloc.c
Objects/setobject.c
Objects/tupleobject.c
Objects/typeobject.c
Python/bytecodes.c
Python/ceval.c
Python/ceval.h
Python/ceval_gil.c
Python/ceval_macros.h
Python/context.c
Python/gc_live.c
Python/gc_trace.c
Python/generated_ir_cases.c.h
Python/parking_lot.c
Python/pyvalue.c
Tools/build/gc_inline_audit.py
Tools/cases_generator/call_effects.py
Tools/cases_generator/root_check_c.py
boil/tools/abortbt.c
boil/tools/cstack.sh
boil/tools/gate.sh
boil/tools/measure/stale_probe.py
boil/tools/opaque_inliners.py
boil/tools/verify.manifest.json
```

## Actual stress100k failure checkpoint; landing held

2026-10-29: the current `ce2ac` once-only stress100k-full has an actual
`test_interpreters.ListAllTests.test_destroyed_by_gc` failure at test_api.py305:
interpreter34 is still in `[0,34]`. Worker3667921 returns FAILED with 179
tests/one failure/eight skips. Worker3667375's test_httpservers returns
ENV_CHANGED with 103 tests/zero failures/eight skips and two unraisables:
HTTPResponse.close459 → IOBase.close → HTTPResponse.flush472 → fp.flush
raises ValueError on an already-closed file. Both actual workers started
off/extras0/stress100000. The full transaction was still active and left
unsignaled at this source-read checkpoint; its logs/journals are partial,
not a frozen final manifest. No final PASS or LAND is possible from this
checkpoint, and all-seven-PASS walk/closure prerequisites remain unsatisfied.

The interpreter case at293–305 uses gc_threshold(1) without explicit collect.
The positive-stress branch of gc_live.c1745 counts to stress_every and returns
before ordinary scheduling; threshold1 alone does not force a pass under
N100000. This is a concrete scheduling hypothesis, not a proven diagnosis.
The whole-module main-interpreter count121 cannot prove a collection in that
critical window. First distinguish no collection, a reachable original cycle
and collection with delayed finalizer/ID-reference destruction. The path is
weakly cached Interpreter → __del__/_decref → _interpreters.decref →
_PyInterpreterState_IDDecref; a later list_all wrapper can also acquire a ref.
Do not infer a Park retention or queue-completion bug from the remaining ID.

For HTTP, the trace proves a not-yet-closed response has a non-None but closed
fp, not which operation closed it. Source/test files in this path are unchanged
from primary. The test's explicit check_status_and_reason reader cleanup sets
response.fp=None before reader.close, so that helper alone does not explain
the observed state. Keep explicit close/test lifetime, missing reachability
and finalizer dependency/completion order as separate hypotheses. The existing
finalizer_order promises referrer-before-transitive-referent queue order;
order and completion across owner threads are distinct. No retained raw event
timeline selects one cause. Main0 count252 and zero fallback/unpinned do not
resolve resource ordering. Current release/debug normal/debug kill fulls had
both modules PASSED with the same test/skip counts; that remains their scope,
not a disposition of this failed stress transaction.

Only after terminal preservation and a coordinator ruling should a bounded
diagnostic observe critical-window collection/finalizer state and actual
HTTP response/fp identity, explicit close/finalizer order and thread. Use
case-local existing hooks rather than a new queue executor or generic held
Queue2 probe. No native diagnostic, code change, exclusion or retry was run or
authorized by this review. The earlier source-preflight finding remains a
bounded source check; it never asserted completion of runtime qualification.

## Terminal stress100k failure; source and evidence preserved

The terminal receipt is
`64448666783338eaa394a2055d3f2d3c6b89094ee5e464368d67050576513190`:
native exit 2, qualification FAIL, 1,394.577 seconds. Its three raw artifact
hashes match. The transcript reports 50,722 tests, one assertion failure and
3,418 skips, with test_interpreters FAILED, test_httpservers ENV_CHANGED and
test_gc timed out after 1,200 seconds. Those transcript totals are not a
validated complete-roster or root aggregate. Driver3645774 has an end witness;
test_gc worker3666394 has scheduled/start witnesses and no end, returned or
suite-result witness. The validator's `unpaired actual state` failure correctly
rejects that missing terminal worker evidence. It is not an adapter false FAIL.

The timeout trace names test_gc.test_trashcan_threads. Main is in C.__del__430
at the test's time.sleep459; one producer is in __del__423/make_nested439 and
the other in sleeper_gen409/__del__430/make_nested439. The test's producers
continue until its exit list is changed by start_threads' cleanup, after the
main with-body returns. run_queue_ex drains until queue_head is empty, while
these finalizers release the GIL. This permits a liveness-cycle hypothesis:
main may keep draining producer work before it can request producer shutdown.
The trace does not prove that hypothesis, queue ownership, completion ordering
or a native lock deadlock. A separately approved case-local progress/entry/exit
chronology could distinguish it; no held Queue2 probe or new executor is
authorized by this observation. HTTP lifetime/order and the interpreter's
threshold scheduling remain separate unresolved cases.

At this read-only terminal check, all 6,322 current source-bridge hashes,
all three expanded 286-path native maps and their resolved-target/link
identities match; Git is clean at ce2ac. The full runner rechecks its current
source/native guards before validating raw results. No source drift explains
away these failures. Landing, tail Walk and closure prerequisites remain held.
No diagnostic, retry, source/native edit or interpreter command was executed
by this reviewer.

## Interpreter diagnostic plan review: failure-code FIX_FIRST

Frozen diagnostic plan `7e553ce2` / runner `2b896958` / probe `8aed9d39` /
manifest `15f77430` is **FIX_FIRST**, before any target command. The four-job
scope, terminal-failure prerequisite, exact copied body, seed/hash distinction,
counter limits and existing source/native guards are bounded appropriately.
The original regrtest and copied unittest paths have different failure codes:
`Lib/test/libregrtest/results.py` defines EXITCODE_BAD_TEST=2 and the unchanged
driver propagates it; the copied probe explicitly exits 1 on assertion failure.
The runner's unconditional acceptance of only 0/1 would label an intended
original-stress assertion failure INVALID_OR_UNEXPECTED. Its prose also calls
that original failure native1. Require exact per-path outcome/code consistency
(original PASSED0/FAILED2, copy PASSED0/FAILED1), retaining both native failures
as failures with qualification=False. A host-only mapping control can check
the two valid failure codes and reject cross-path or unexpected codes. No
runtime retry, production change or expanded diagnostic is required. No probe
was executed by this reviewer.

## Interpreter diagnostic successor: four-job plan clear

Verdict: **FOUR_PROBE_PLAN_CLEAR**, subject to the coordinator's execution
grant, for plan `d478fceb3171f856f058b842582ee6dc009847d3bbe4627257e5bfec2050bf7a`,
runner `51df97f74d45054e1d91abfc2e899bc9b1b2a6055d8ca64963eb0796d904852c`,
unchanged probe `8aed9d39b3c750cba85da2bb3ebf35ecad76ad8232f40ad0fb8dcc6ff20381a3`,
manifest `cb2310978ebd8522c9b5169cc7b73f7cb936c40dd2530a974d66b12f3b14be8e`
and host controls `0ef6a60b101896717a4e58f5e9fbde537d9ae6f2b6d02fa9350eca39a5e1ae42`.
All ten indexed hashes match. The complete unexecuted predecessor is preserved
under frozen-v1. The narrow diff checks exact per-path outcome/exit codes;
four valid combinations and six wrong-kind/unexpected-code negatives are
recorded. The command now uses --no-randomize: actual cmdline.py574–577 first
enables randomization for --randseed, then applies that explicit override.

Only original-stress, original-off, window-stress and window-off, in that
order as separate commands, are cleared. Each remains a fresh diagnostic
under normal admission/build and full-serialization locks, exact terminal
failure/source/native/tool guards and no-overwrite receipts. Original failure
is native2, copied failure native1; both remain failures with qualification=False.
N0 failure or an unexpected error/skip/witness/state stops evidence acceptance.
The two counter snapshots measure successful root phases charged to the
current interpreter only; they do not prove target graph/finalizer completion.
The copied observer changes allocation/import timing. Seed991443983 is bound,
but new hashseed0 differs from the full's recorded randomized-hash flag and
does not replay its process history. No HTTP/trashcan repair, full rerun,
Queue2 probe, exclusion or production change follows from this plan. No
target, host control or diagnostic was executed by this reviewer.

## First diagnostic: native failure reproduced; harness invalidated

Only original-stress ran under the reviewed V2 plan. Native exit 2 reproduced
the exact assertion with one test, one failure, zero skips: interpreter1
remains in [0,1]. Driver3727483 and worker3727486 each have start/end records;
scheduled/returned worker PID, seed991443983 and config `5b1278ce` agree.
Actual state is off/extras0/N100000 with the declared new hashseed0. The
original result remains qualification=False / INVALID_OR_UNEXPECTED because
the diagnostic required an absent suite-result. Its three recorded raw hashes
match. This harness error is distinct from the native assertion failure.

The source explains that absence: full_result_observer.capture_result emits
suite-result only after test_func returns; single._run_suite105 instead raises
TestFailedWithDetails, caught at229 into the returned FAILED result. The
diagnostic-only validator may require outer results for PASSED while permitting
absence for this exact FAILED/native2/one-test/one-failure/zero-skip/no-error
case with complete driver/worker pairing, selection, seed and config evidence.
Any present outer must still agree. Keep full-v3 helpers/qualification rules
unchanged. Validate the preserved failure offline into a separate record;
do not rerun original-stress or overwrite its invalid harness receipt. The
remaining three jobs require the corrected frozen plan and coordinator grant.
No reviewer target/native command or diagnostic was run.

## Diagnostic V3: remaining-three plan clear

Verdict: **REMAINING_THREE_PLAN_CLEAR** for plan `ff85cad6`, runner `86bbe01c`,
manifest `feb77e44`, host controls `6c4dfe32` and offline supplement `ed54cbe8`.
All eleven manifest hashes match. The narrow validator now accepts absent
outer only for the exact target FAILED/native2/one-test/one-failure/zero-skip/
no-error result with complete driver/worker pairing, selection, seed and config.
PASSED still requires an outer snapshot; any present outer must agree.
Host negatives reject passed-without-outer, wrong failure count, missing
worker end and a present outer's wrong seed. Shared full-v3 helpers and
qualification rules are unchanged.

The separate read-only supplement matches the retained native failure and
source catch path; the original `86008c1a` harnessINVALID receipt is preserved.
The CLI now admits only original-off, window-stress and window-off; it rejects
original-stress before execution. The coordinator may grant those three
separate commands. This does not rewrite or rerun the failed original, make
any native failure a correctness PASS, or disposition stress100k. No target
or host control was executed by this reviewer.

## Four actual diagnostic outcomes

Verdict: **ACTUAL_DIAGNOSTIC_SCOPE_CLEAR** for summary
`bb50b7d365daf89ef4f0f3fa8c4d9f3909714e7541750c522bce3ec95a0fd9a6`.
The original-stress nativeFAIL2/harnessINVALID receipt and separate offline
supplement remain unchanged. Original-off passes one test/zero skips with
paired driver/worker and agreeing returned/outer stats. Copied stress fails
the same assertion, ids [0,1], with charged main0 root phases 10→10; copied
off passes, ids [0], with phases 16→17. The remaining three raw artifact sets
match their receipt hashes. Their start/end identities, off/extras0/actual N,
recorded seed/hash policy and main0/nonnegative/zero-fallback root domains
agree; native source/product guards complete after the commands.

This supports a threshold-trigger eligibility distinction under the declared
stress override: gc_live1745 replaces ordinary threshold scheduling, and
test_gc1224/1289 already treats byte-trigger expectations separately. It does
not show a target retained after a charged mark, exclude other-interpreter
marks or queued finalizers, or reproduce the full's unknown randomized hash
history. Copied getter allocations perturb timing. Keep all four outcomes
distinct from correctness qualification and preserve the full FAIL. A narrow
test-eligibility adaptation requires its own reviewed source grant; no
scheduler change, blanket filter or uncalibrated repeats follow. HTTP and
trashcan liveness remain unresolved. No reviewer native command was executed.
