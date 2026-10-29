# 10a rung 2b: as-built evidence and remaining work

Status, 2026-10-29: not landed. Native products are built from
ce2ac8991eebb1bbb10698ff2d07a79bd4cba452. The enclosing successor changes
only one test's two-line stress eligibility decorator and these records;
it does not rebuild or change the native runtime. Six broad profiles
qualified; the original full N100000 attempt remains FAILED. Tail Walk,
gate 3 closure, independent landing and exact-merge integration are pending.

## Implementation

The call/ceval/type/allocator helpers use explicit owners/spans rather than
conservative C words. The work record retains their individual source and
checker gates. Takeover added these bounded repairs:

- Verifier-only post-B PyMem provenance enrichment. Per-origin traversals
  preserve allocated-block/interior rules, deduplication, cycles, record cap
  and failure-before-kill publication. A/B marking and death policy do not
  change. Deterministic real-helper old/new and zero-seed sort controls are
  independent of the unchanged optimized end-to-end holder fixture.
- `_PyList_AsTupleAndClear` creates the tuple while the initialized list
  prefix is still visible, then clears on success or error. A reproduced
  lifetime failure moved this planned 3a repair into 2b.
- Multiprocessing's cleanup helper drains finalizers before stopping its
  resource tracker, after stopping forkserver. Actual-helper old/new and
  active forkserver/tempdir controls cover the resource dependency.
- Precise classification of `_PyParkingLot_Park`, with all seven real
  borrowed-storage owners left opaque. The pinned real-Park fixture checks
  exact caller slot/value before and after payload publication/unpark; its
  negative removes only that caller slot. Classification override and
  original old-source thread-pool failures are separate controls.
- The checker recognizes the actual DWORD cast in inactive Windows source.
  SemWait's explicit scalar selection preserves optional monotonic fallback,
  single wait, deadline/error/errno behavior; 36 old/new combinations give 72
  copied-body observations, not Windows runtime claims.
- The raw-mutex wrappers have precise markers only. Their bodies/slow-path
  effects are unchanged. The actual ASan linked audit required these markers.

The current raw-mutex bridge preserves literal changed product/section/code
inventories plus focused precision/CFI/root-DWARF evidence, including all four
actual semaphore functions. Five of 286 product paths per build changed; no
whole-machine-code equality is claimed. Historical120-transaction campaigns
per build at 1bfe remain historical. Their explicit carry-forward is supported
by the native bridge and fresh affected/broad checks, not automatic 20 reuse.

## Fresh native evidence

The once-only affected campaign passed 24 transactions across release off,
candidate O1 debug off and actual debug kill. It includes strict stale100,
complete StackWalk, all three original thread-pool selectors, both eval
probes and whole generated-case tests: 243 test executions/three existing
tail-only skips, 1800 stale attempts, 4783 recorded collections/zero fallback.
The three intentional walk unpinned events remain report-only 3c debt.

Qualified profiles below use actual driver/worker mode and stress getters,
exact scheduled/returned identities, outer skip Counters, protected-case
checks, main-interpreter counter witnesses and frozen source/native/helpers.
Full roster is 564 modules. Counts are recorded aggregates of finalization
events, not a claim that repeated(pid,interp) keys are unique lifetimes.

| Profile | Tests/skips | Collections | Fallbacks | Report-only unpinned |
| --- | ---: | ---: | ---: | ---: |
| release full, off/N0 |51,132/3458|63,573|0|2880|
| candidate O1 debug full, off/N0 |51,132/3401|66,943|0|4014|
| candidate O1 debug full, kill/N0 |51,132/3401|66,936|0|4010|
| stress11 release, off/N1000 |3062/100|45,008|0|6524|
| stress11 debug, off/N1000 |3062/64|48,991|0|7948|
| ASan16, kill/N0 |3976/92|16,818|0|3480|

ASan16 is the accepted 14 plus sort/functools in one union. Only the exact
allocator overflow selector and machine remote-exec selector are excluded;
ASAN_OPTIONS remains detect_stack_use_after_return=0:detect_leaks=0. This
does not claim unfiltered ASan success. Stress uses only the four accepted
writer/metadata selectors plus remote-exec; B/test_misc is not excluded.
Writer/PyMem/compiler debt remains open.

Tail make froze its 286 expanded paths before checks. Its strict six-case
stale100 passed at actual tail/off/N0:706 collections, zero fallback and
zero unpinned. Complete tail Walk 11 has not run. Whole tail/retain 8.2 and
step-end gate4 remain 10a4/H1 obligations, not extra 2b matrices.

## Failed full stress and bounded diagnosis

The original full N100000 run ended naturally with native 2/host 1:
50,722 tests, one interpreter assertion, test_gc timeout and HTTP
ENV_CHANGED. The killed GC worker lacks an end witness; there is no qualified
whole-run counter aggregate. Receipt64448666 and raw artifacts stay intact.

The interpreter test relies on byte threshold 1, with no explicit collection
in its assertion window. Positive stress deliberately replaces that byte
trigger. Unchanged original stress FAIL/off PASS and separately copied windows
(stress 10→10 phases/failure, off 16→17/pass) support that contract mismatch,
not demonstrated post-GC retention. Counts are successful phases charged to
the current interpreter; snapshot allocations affect timing and other
interpreters/failed marks/queued callbacks are outside that claim.

The scheduler remains unchanged. The exact test now uses the existing
threshold-test skipIf/reason under PYTHON_GCSTRESS. Once-only validation:
off 1/0 executes/passes the unchanged body; stress 1/1 records declared
eligibility, not reclamation coverage. All native/config/helper bytes remain
unchanged. No full run is repeated or relabeled because of the decorator.

HTTP's unchanged module control reproduced two closed-fp unraisables.
A separate Python lifecycle observer timed out at 180s and remains incomplete.
Its partial weak-token timeline correlates a colorized test_err response
with fp open at close entry and closed by flush entry on another thread;
who closed it is not observed. Observer allocation/identity limits apply.
No referrer inspection or extra explicit collection ran.

Then the same four unchanged logging cases reproduced HTTPResponse.close→
flush ValueError on both source-bound primary052 and ce2ac, each 4/0 and
native 3/ENV_CHANGED. The same unchanged public trashcan-thread case hit its
automatic 60s deadline on both, with main/producers in existing finalizers.
No custom finalizer, observer, extra collection, raw-memory/C probe or runtime
repair was added to those comparisons. Both lack a worker end/returned JSON;
no aggregate counter credit is assigned. The primary one-off audit's INVALID
receipt remains; its read-only supplement explains regrtest's nonzero path
before from_json, without rerunning the test.

These observations demonstrate inherited exposure/lack of completion under
the declared conditions. They do not establish identical mechanisms,
mathematical deadlock or broad exoneration of 2b. The primary binary comparison
retains historical byte-attestation limits and optional-module differences;
own source/Lib, configuration, current native maps and actual startup state
are bound prospectively. Hashseed0/no-user-site are explicit diagnostic
conditions, not the original full process's unrecorded hashseed.

## Retained artifacts and remaining boundary

Evidence root is
`build/takeover-20261029/park-prerequisite/raw-mutex-design/transition/`.
The summaries bind their raw files/plans/helpers/source/native scopes:

| Artifact | SHA256 prefix | Meaning |
| --- | --- | --- |
| affected/complete-summary.json |cf3d018a|24 fresh native affected PASS|
| full-campaign/stress100k-full.result.json |64448666|original full FAIL|
| interpreter-eligibility/two-check-summary.json |fee5f36c|off body and declared stress eligibility|
| http-comparison/two-build-summary.json |6919760f|inherited HTTP exposure|
| trashcan-comparison/two-build-summary.json |d17bcc22|inherited bounded lack of completion|

Independent review and the coordinator accepted a scoped2b exception,
retaining HTTP/resource ordering and queue progress as final GC/performance
blockers. Original full stress is still FAILED. A concrete successor binding
must be reviewed before tail Walk or gate 3 closure can launch. It must
preserve the frozen a731/f91d helper
bytes and actual old results, changing only the explicit prerequisite/source
binding. Closure still owes sequential 16 cext tests with seven exact existing
skips, unchanged case/clinic regeneration, traversal audit and 19 optional
module syntax receipts with external headers/toolchain bound; four
unsupported modules stay explicit. Current full100k is not green.

Global O1 adoption and independent LAND remain separate. After LAND, the
exact detached merge still requires fresh gate 0 on both builds, current targeted/root
probes, default full gate 2 and debug gate 3 using managed project builds.
Branch evidence is not primary integration completion. No broad profile is
automatically repeated merely for this test-only/documentation successor.

Rung 3a retains writer/PyMem and remaining call-owner work; 3b checker/precision
and 3c active queue payload/saved exception/detach/swap obligations remain.
Queue ordering/progress findings need their own owner and causal design.
Unpinned is report-only 3c debt except strict stale. Before checkpoint I, refresh
quiet elapsed/boxing/root attribution for historical keyword/attribute span
instruction regressions; a future JIT must not be assumed to absorb them.
