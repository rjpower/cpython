# Main-only GC union: final landing evidence review

**Verdict: LAND — execution/evidence closure is clear for the declared Linux main-interpreter profile.**

Qualified source is `4da292b1294280da838eae8b886aca7103f2d014`, with runtime
library/tests at dac6, ordinary native code at3c6 and ASan native code atf945
explicitly separated. The final index is
`build/gc-final-traversal-closure/final-completion.json`, SHA256
`12f1ea31aaced9a55823fcc94da127e5639822932f1361d591935e9db24e8d3a`.
Its276 absolute artifact digests independently match. The completed
profiles, explicit release/ASan composites and corrected closure have no
remaining execution blocker. Independent source approvals are attributed
below; primary merge and post-merge smokes remain coordinator actions.

The original release run reported `test.test_multiprocessing_fork.test_processes`
ENV_CHANGED with `SharedMemory.__del__` calling `close()` / `_mmap.close()`
and raising `BufferError` for exported pointers. Terminal failure index7736d428
and all13 bound files are now independently checked. Native regrtest printed
SUCCESS/native0 with51236 tests/3795 skips, but qualification remains FAIL.
This failure remains immutable. The reviewed dac6 library repair now has
six positive runs and an intended old-policy failure. Its explicit release
and ASan composites are independently clear below; neither rewrites the
original run. The remaining profile and corrected closure evidence is
checked below at its actual source identities.

This is an independent review of the native operator's launch/evidence and
source-to-native composition. The reviewer authored the earlier H3 stats
and S3 snapshot changes, main-only admission, Unpickler, XID and safeguards.
Independent source approval for those changes belongs to the coordinator/
string reviews; this record does not self-clear them. In particular, the
existing H3 source reviews and 7a-2a-snapshot-source-review remain the source
approval records for those components. The coordinator's source-clear
dispositions remain a
landing prerequisite. No native target, configure, make or compiler command
was run by this reviewer. Raw terminal records and unchanged pure host
validators were checked; active result files are not evidence.

## Frozen source and ordinary products

Runtime/build C candidate: `3c6b8daa5065406138879e928c7a6a7ef1fe3aa2`.
Full-run runtime test source: `f9457ef8494a15406c41b6af9af5056a15a59a94`.
Its sole tracked difference from3c6 is the exact reviewed `Lib/test/test_re.py`
external-behavior hunk, verified against the approved patch. All C, headers,
generators, other tests and tool inputs are unchanged across this bridge.
The serving tree is the clean managed `boil-buffer-xid-main-only` worktree,
branch `boil/gc-final-validation`. No image from the contaminated old
`boil-gc-simple` source root is used for these fresh cohorts.

The union composes main-only admission, the one-drainer rule and rooted
active calls, managed-buffer readiness, sealed consumer acquisitions,
NDArray original-master ownership, ordinary same-owner XID memoryviews and
removal of silent buffer-release/count coercions. Source records include
the sealed-consumer, NDArray and XID reviews, and the coordinator-approved
main-only/Unpickler/drainer/safeguard diffs. Earlier safety qualification
remains at its recorded identity; these components require actual union
qualification rather than relabeled older fulls.

Reviewed repair source `0d2116c0573854989ced35dc3053fbcccd953207` additionally
changes only `Lib/multiprocessing/shared_memory.py` and its four tests in
`Lib/test/_test_multiprocessing.py`, exact diffde0f7576. No C/native rebuild
is implied. That intermediate test placement is superseded below.

The test placement in that intermediate repair is superseded by independently
cleared movede73701fd/net7975929e: runtimefe19c47b is unchanged, the entire
`_test_multiprocessing` file is restored to f945, and the four tests move to
`test_memoryview.SharedMemoryLifetimeTests`. They import shared_memory directly
and use track=False/public unlink, avoiding both a tracker subprocess and the
unrelated sanitizer multiprocessing import guard. The final source is dac6;
its bindings, six bounded positive outcomes and remaining full/stress
qualification are checked below. Final source4da adds only audit metadata.

| Cohort | Actual build source | Terminal capsule | Current product closure |
| --- | --- | --- | --- |
| GCC O1/Py_DEBUG | 3c6 | build4503855f, cleanupf57d66aa | 588 original paths exact |
| GCC O3 release | 3c6 | completionaf7b2765, buildcfa949e1, cleanupd106710c | 588 original paths exact |
| GCC ASan/Py_DEBUG Og | f945 | build0246ade9, cleanupf6922911 | 588 original paths exact |

Both ordinary capsules have configure/make0, correct absolute source/build
roots, fixed configuration/tool hashes and post-gate cleanup. Assertions,
frame pointers, zero-init, shared GIL, nonstats/non-JIT/non-tail policy and
effective optimization are bound by the accepted configuration guards.
Current bytes, symlink/target identities and narrow regular in-build cache
additions were independently checked. The ASan build uses the standard
`detect_stack_use_after_return=0:detect_leaks=0` options. Its target profiles
require actualkill/N0.
The actual sanitizer/Og configuration, all588 product bytes/aliases and
configuration/tool hashes were independently checked. Configure/make and
cleanup passed at f945; all three original sanitizer profiles now pass,
with the successor's changed-path coverage stated separately below.

## Completed bounded evidence

| Evidence | Result | Scope and independent check |
| --- | --- | --- |
| New13, completion95d56f4c | 13 tests/0 skips, 3 passed modules | Actual O1 kill/N0; all37 completion refs checked; raw validator replay |
| Whole9, same completion | 1863 tests/120 skips, 9 passed modules | Actual O1 kill/N0; 10 paired driver/workers;10435 collections, zero fallback,1 report-only unpinned |
| Fresh wholeRe, completion4bd06285 | 172 tests/3 skips | Test sourcef945/native3c6;107 collections, zero fallback/unpinned; raw replay and19 refs checked |
| Four copied negatives, completione859dd0f | Four intended native2/host1 failures | All54 refs checked; exact four omissions, unchanged GC/pickle test bytes; each1test/1failure/0skip |

New13 records212 collections with zero fallback/unpinned. Actual mode,
Py_DEBUG, seed, schedule/worker identity, returned/outer counts and skips,
platform auxiliary pairing, main0 root domains and cleanup agree. The
f945 composite explicitly carries new13 and the eight unchanged whole
modules at3c6 and adds fresh wholeRe; it does not rewrite the old whole9.

The four negative controls exercise drainer admission, scanner inert-before-
release, Unpickler acquire-before-release and NDArray closed-state checking.
Each reaches its named assertion rather than crashing or failing to import.
Driver/worker/auxiliary state is paired actualkill/N0; collections are74,
77,64 and75 respectively, all zero fallback/unpinned. The absent outer
suite-result is the existing failed-suite path. These retained expected
failures prove the selected causal tests, not a positive qualification.

The revised re integration case checks real match data after GC/allocation
pressure, exporter weakref liveness and eventual reclamation/backing resize.
Expected string data is independently constructed so it cannot keep the
scanned string alive. It does not prescribe traversal layout or exact
acquisition counts. Internal GC lifecycle unit invariants remain intact.

## Eight actual profiles and closure

Operator-owned packet: `build/gc-final-qualification` in the serving tree.
Root reviewed the reused recipes and literal vectors. Component binding
`0a31488fe086cf10660a7abcd3f59a12ab4e12dc6af2bed3198a3eabf6ffce06`
distinguishes ordinary native3c6 from test sourcef945 and fresh ASan sourcef945.
Both worker shell copies are0755. Root granted the once-only chain after
current guards and actual ASan capsule binding; the operator owns execution.
Final binding9e8a3071 adds only the three ASan receipts/readiness and corrects
source/native/scope labels plus dependent digests. Ready ASan plan22f30f30,
broad planbb209e4a, closure plandd486fa0 and launch vectors41e6ad36 matched;
jobs, filters and protections are unchanged. The original chain completed
the three sanitizer profiles and stopped at release qualification failure.
Only terminal records are reviewed and credited.

| Profile | Required actual mode | Status |
| --- | --- | --- |
| ASan16 | kill/N0 | Composite PASS:4026 tests/141 skips;15 old outcomes + fresh MV |
| ASan Unicode7 | kill/N0 | PASS:1037 tests/25 skips |
| ASan owner15: Unpickler7/XID8 | kill/N0, exact15/0skip | PASS:15 tests/0 skips |
| Release coverage | off/N0 | Composite PASS:51240 tests/3794 skips;557 retained +7 fresh; original full FAIL |
| O1 debug full | off/N0 | PASS:51240 tests/3739 skips |
| O1 debug kill full | kill/N0 | PASS:51240 tests/3739 skips |
| Release stress11 | off/N1000 | PASS:3104 tests/148 skips |
| O1 debug stress11 | off/N1000 | PASS:3104 tests/113 skips |
| Closure: cext/regen/traversal/headers | carried cext/regen; fresh corrected traversal/headers | PASS composite; original traversal batch FAIL retained |

Original ASan16 result55d41bbf/cleanup25688a23 is terminal and independently checked:
all artifact hashes and the unchanged raw validator replay agree on16 passed
modules,17 paired driver/workers,184 physical root PIDs,13002 collections,
zero fallback and1 report-only unpinned. Actual Py_DEBUG/sanitizer, kill/N0,
source/config/selection/seed/main0 contracts hold. Quiet execution did not
invoke platform; its auxiliary field is correctly absent. Cleanup verifies.

ASan Unicode7 result015d71ea/cleanup9e507853 is likewise checked: seven
passed modules, eight paired driver/workers,76 root PIDs,718 collections,
zero fallback/unpinned, actual sanitizer/Py_DEBUG/kill/N0 and verified
cleanup. Artifact hashes and raw validator replay agree; auxiliary is absent.

ASan owner15 result0b9a061c/cleanup3ff53fc4 likewise matches its frozen
fifteen distinct positive selectors, three passed modules, four paired
driver/workers and162 collections with zero fallback/unpinned. All15 execute
without a recorded skip under actual sanitizer/Py_DEBUG/kill/N0. Raw
hashes, validator replay and cleanup agree; auxiliary is absent.

The frozen ASan16 log also contains two `<sys>:0 ResourceWarning` messages
for unclosed temporary `stderr.txt` streams and unawaited coroutine/asyncgen
RuntimeWarnings. They are retained, not described as clean stderr. The sole
`stderr.txt` creation source is the forkserver preload test's generated
`sys.stderr = open(...)` without explicit close; this is source attribution,
not proof of the exact warning instance. ASan16 has no reported ENV_CHANGED
or HTTPResponse closed-fp unraisable. Coordinator disposition of those
warnings remains explicit; they do not excuse the original release failure.

### Historical HTTP and trashcan regression coverage

The old stress100k terminal run remains FAIL. Its HTTP module reported
ENV_CHANGED with closed-fp HTTPResponse unraisables; its GC worker timed
out in `test.test_gc.GCTests.test_trashcan_threads`. A partial HTTP observer
associated one response with the colorized logging `test_err`; it did not
identify both original traces. These records are not relabeled by this run.

Current source retains the trashcan body and its working-thread eligibility.
Normal full3 and stress11 select GC, and no declared filter matches this
method. ASan16 and bounded whole9 already return GC PASSED176/21 with no
trashcan/GCTests container skip. That supports execution through the actual
loader/filter/returned-count/skip contract; there is no separate success-ID
journal. Current full/stress outcomes and exact skip associations are
checked below.

Normal full3 selects `test_httpservers` unchanged, including
`RequestHandlerLoggingTestCase.{test_get,test_err}` and
`RequestHandlerColorizedLoggingTestCase.{test_get,test_err}`. Stress11 and
ASan groups do not select HTTP. Final review must reconcile full module
outcomes, these case/container skips and unexpected unraisable/environment
diagnostics. A current ENV_CHANGED or resource-lifecycle unraisable is a
stopper; aggregate counts cannot hide it. No extra target or repeat is
requested, and a current normal/N1000 pass will not be called a stress100k
rerun.

The terminal release raw records show HTTP PASSED103/8, with all eight skips
specifically requiring SSL; none names the four logging cases/containers.
GC returns PASSED176/35 with no trashcan skip. Source selection/loader and
skip accounting therefore support normal/off execution of these cases.
There is no new HTTP closed-fp report in this module outcome. This is useful
ordinary-mode regression coverage inside a failed whole profile, not a
qualified full or a repeat of the failed100k run. Fresh debug/kill/stress
evidence for the successor is checked below.

### Terminal release failure and bounded repair review

Raw replay after removing only the source-ineligible release debug-hook case
reconciles all564 returned modules and565 paired driver/workers:459 PASSED,
94 SKIPPED,8 RESOURCE_DENIED and3 ENV_CHANGED. The three changed environments
are fork/forkserver/spawn `test_processes`, each reporting an unraisable
BufferError from SharedMemory disposal. The original wrapper stopped first
at a separate bad premise: protecting
`BufferLifetimeTests.test_scanner_explicit_finalize_while_executing` in a
release image despite its `requires debug interruption hook` decorator.
Only release must-run eligibility may be removed; every Py_DEBUG mode,
including stress and ASan, must keep this protection. Failure cleanup
7d341b5b verifies source/config/tools/all three product guards. There is no
positive full verification or qualified root aggregate.

SharedMemory repairde0f7576 is independently SOURCE_CLEAR. Automatic
`__del__` consumes `_fd` before detaching `_buf`/`_mmap` and does not release
another holder's view or call strict mapping close. POSIX mmap owns a
duplicate descriptor (`mmapmodule.c:1952`); a surviving managed buffer roots
the mapping independently. Explicit `close()` is unchanged and still raises
with an exported slice, then succeeds after release. Four external tests
cover discarded views/no unraisable, surviving aliases/slices, strict explicit
close and Linux `/proc` mapping/descriptor disappearance after final release.
Public `sms.unlink()` immediately after creation makes named-resource cleanup
explicit. Final local tests use track=False; the ordinary multiprocessing
integration retains real tracker lifecycles. There are no forced export
counts or new GC loops/protocols.

Seven affected release modules are processes3, misc3 and memoryview (the
latter uses SharedMemoryManager in RacingTest). The557-old-plus7-fresh
assembly required exact original returned/skip/source eligibility reconciliation,
now checked below; no original failed full is relabeled PASS. Debug normal/kill fulls
use fresh successor source. Unicode7/owner15 scopes remain separately
recorded.

Correction to this review's initial ASan inference: the parent package names
do not establish processes-leaf execution. Frozen ASan16 outer records show
fork4tests/4skips, spawn4/4 and forkserver18/4. Each package records its
manager/misc/processes/threads leaf as `unittest.loader.ModuleSkipped` with
the inherited libasan thread+fork deadlock reason. `_test_multiprocessing`
imports shared_memory before this guard but raises before defining the test
classes; import alone is not SharedMemory instance coverage. The prior
claim that these parent package outcomes executed the changed path is
withdrawn. No old processes-leaf ASan coverage or unskipped protected-leaf
coverage follows from their package PASSED states. The fourteen additional
forkserver tests retain their actual preload scope.

The coordinator therefore requires four local, no-fork SharedMemory tests
in `test_memoryview.SharedMemoryLifetimeTests`, using track=False and public
unlink, rather than bypassing this sanitizer guard. `_test_multiprocessing`
has returned exactly to its f945 source. New actual ASan4/0 covers that local
changed path; old multiprocessing parent metadata retains its original
limited scope. Ordinary processes3/misc3 use real track=True integration.
Their fresh six-module release job keeps default resources; a separate
release memoryview job enables cpu and protects the actual manager race.
ASan memoryview keeps cpu disabled to avoid inherited fork exposure. These
source/selection refinements require the terminal evidence below. No parent-package
ASan repeat or skip-guard bypass is required by this review.

The final repair is clean source dac6f98f95362d57752b773c89bf12de2306a089.
Its net diff from f945 contains only shared_memory.py and test_memoryview.py;
_test_multiprocessing is restored exactly. Production disposal is unchanged
from the independently reviewed repair. The four local tests use track=False
and immediate public unlink, without importing the multiprocessing test
guard or starting a resource tracker. The active successor packet is
build/shared-memory-disposal-validation-v2; its frozen launch bindings were
independently cleared separately from this outcome review.

Frozen release-shm4, debug-shm4 and asan-shm4 each pass4 tests with zero
skips, actual off/kill/kill respectively and N0. Each has two paired
driver/worker witnesses, zero fallback and zero report-only unpinned;
collections are62/72/72. This review rehashed each result's raw artifacts,
checked its cleanup link and replayed the unchanged raw validator with all
four selected IDs protected. These local successes prove the declared
SharedMemory path in each actual cohort. They are separate from the
processes/misc integration, whole memoryview outcomes, causal control and
release/ASan assemblies checked below, and the four remaining profiles.

All six bounded positives are now independently checked. Fresh processes3
and misc3 return814 tests/79 skips, all six PASSED with3374 collections;
O3 memoryview with cpu enabled returns178/18,139 collections and its
manager race protected against a skip. ASan memoryview with default
resources returns178/19,136 collections. Each has zero fallback/unpinned,
actual declared mode/N0 and verified cleanup. Their frozen raw validator
replays agree with the result summaries.

Release assembly c13fdc23 binds20 artifacts. Every retained returned row
matches its original raw record; the557 retained and seven fresh rows are
disjoint, with the explicit test_memoryview/test.test_memoryview alias.
The564 rows total51240 tests/3794 skips and462 PASSED/94 SKIPPED/8
RESOURCE_DENIED. Resources differ for the fresh cpu-enabled memoryview
cohort; no uniform new full run or per-module old root aggregate is claimed.

ASan16 assembly2b81264e binds12 artifacts:15 exact retained rows and one
fresh memoryview row, totaling4026/141 and16 PASSED. Parent multiprocessing
skip limitations remain explicit. New ASan4 is distinct changed-path
evidence, not added again to these totals. Old Unicode7/owner15 retain
their true source and native identities.

The old-policy causal audit dcb15ebe is also checked against raw trace,
structured returned result and paired off/N0 witnesses. The unchanged
discarded-view test fails exactly at `automatic disposal reported an
exception` with captured BufferError,1 test/1 failure/0 skips, native2 and
host1. Its47 collections have zero fallback/unpinned and post-cleanup
guards pass. Native failure is preserved; no positive verify was applied.
Terminal index58723869 binds48 files and separates this expected failure
from its six positive outcomes.

The remaining recipe is frozen separately at
build/gc-final-remaining-qualification: helper diff ad9d7cbf, ready vectors
db7353d7 and closure plan a363bc6a. Only prerequisite digests/readiness and
launch authorization metadata were rebound after the coordinator's source
review. The native operator owns execution and outputs; this reviewer
checked no active results. Its four actual profile results and cleanup are
checked below; the later audit correction has its own closure bridge.

The first fresh remaining profile is independently clear: debug-full
result8d550cf2/cleanupe052976b and handoff61e60f19. Raw validator replay
with all274 declared protections agrees on564 returned states,462 PASSED/
94 SKIPPED/8 RESOURCE_DENIED,51240 tests/3739 skips and565 paired
driver/workers. Actual mode is off/N0, ordinary native3c6/current source
dac6;60794 recorded collections have zero fallback and3 report-only
unpinned. Raw artifacts and cleanup links match. No matching unraisable,
closed-buffer or closed-fp trace appears in this frozen log.

HTTP returns PASSED103/8; its eight outer skips are exactly SSL cases.
The previously implicated HTTP logging methods are neither filtered nor
skipped. GC returns PASSED176/21; test_trashcan_threads has only its existing
working-thread requirement and is neither filtered nor in the complete
outer skip list. These are loader/selection plus module-result inferences,
not a per-success-ID journal or a claim that old stress100k passed. The
same raw records provide nine smoke modules1935/50 with an exact matching
outer skip counter, including30 main-only capability skips.

Debug-kill-full resultb1391e8f/cleanupc8ffa48e and handoffdc9972f4 also
independently clear. The same274-protector raw replay agrees on51240/3739,
564 returned states462 PASSED/94 SKIPPED/8 RESOURCE_DENIED and565 paired
driver/workers. Actual kill/N0 has60771 recorded collections, zero fallback
and3 report-only unpinned. Raw and cleanup hashes match. HTTP again passes
103/8 with only its eight SSL skips, and GC passes176/21 without a
trashcan_threads or debug scanner-hook skip. No matching unraisable or
closed-buffer/fp trace appears. Both full modes therefore establish current
normal/kill coverage at their declared resources; neither relabels the old
stress100k run or supplies its missing per-success-case journal.

Release stress11 resultf3b9c680/cleanup3803f212 and handoff9de6ff22 also
independently clear. Raw replay agrees on11 PASSED modules,3104 tests/148
skips,12 paired driver/workers and35801 collections, zero fallback and33
report-only unpinned under actual off/N1000. GC returns174/36 without a
trashcan_threads skip. The scanner interruption-hook case is visibly
skipped for its existing Py_DEBUG requirement; only release must-run
protection omits it. Debug stress retains that case protected. HTTP is not
selected by stress11. Artifact/cleanup links match and no matching resource
unraisable or closed-buffer/fp trace appears.

Debug stress11 result8a7c8cbd/cleanup08457125 and handofff5db4bf8
independently clear as well. Raw replay agrees on3104/113,11 PASSED
modules,12 paired driver/workers and37982 collections with zero fallback
and11 report-only unpinned under actual off/N1000. GC returns174/23. The
debug scanner interruption case remains explicitly protected and unskipped,
as does trashcan_threads under its existing loader/selection contract.
HTTP is not selected. Raw and cleanup hashes match; no matching resource
unraisable or closed-buffer/fp trace appears. All four fresh profiles are
now independently clear.

Closure stopped at the traversal audit, return1. Cext passed16 tests with
the exact seven declared skips and regeneration passed; unbuilt/header
checks were not run. The frozen traverse.log hash7227ee08 reports exactly
two problems: ndarray's head->base.obj/staticbuf.base.obj no longer have a
recognized direct release/traverse, and the removed XIBufferViewType_spec
still has an allowance. The native window is released; no retry or source
change is credited here.

Source inspection supports a narrow metadata repair in the existing allow
file: ND's producer buffers initialize base.obj=NULL; its consumer cache
copies the original managed owner's master, ndarray_traverse visits owner,
and mbuf_traverse visits master.obj. The two cached paths are borrowed,
not independently owned references. The obsolete XI type is absent.
Removing its stale row and documenting these exact ND paths does not
require a generic checker exemption or runtime change. The corrected
traversal/header results are checked below; the failed batch remains a
failure.

The independently reviewed metadata-only correction is clean source
4da292b1294280da838eae8b886aca7103f2d014, exact diff548487a3. Its sole path
is Tools/boil/traverse_allow.txt; the key is the C type variable NDArray_Type,
while the checker diagnostic displays its Python name ndarray. All runtime,
test and checker-algorithm bytes remain dac6. The successful broad and
component evidence retains dac6 identity; corrected traversal and previously
unrun header checks require an explicit source bridge rather than relabeling
the failed closure or rerunning unchanged native/full work.

Corrected continuation packet751dc211/source bridgec47d9604/plan6aa87735
contains no profile jobs, only traversal then unbuilt. Its literal commands,
worker0755 modes and unchanged lock/control bodies were independently
cleared. The actual continuation returns0: traversal has its expected empty
success log, and unbuilt emits exactly19 expected receipts, no warnings,
and four declared unsupported modules (_tkinter, _dbm, _gdbm, readline).
Raw result/plan/log/outer links match. Cext16/exact7 and unchanged regen
remain carried at actual dac6; no successful stage was rerun.

After terminal cleanup, this reviewer independently rehashed all original
588 products per ordinary debug/release/ASan cohort, their configuration,
resolved targets and aliases, each recorded tool file, all2915 borrowed
headers and the current source/recipe inputs. All match. The only additional
ordinary build files are six regular nonsymlink in-build cachepyc files per
cohort; ASan has none. Serving source is clean4da. No compiler, target or
guard helper was executed for these independent file checks.

Source discovery remains564 modules; this is a source selection count, not
runtime execution. The normal fulls retain the whole roster and existing
byte-trigger/bytecode protections. Source-defined main-only skips remain
visible. Only the explicitly unavailable `test_stress_shared_heap_callbacks`
was removed from must-run protection; no extra selection filter was added.
The inherited remote-exec and ASan allocator exclusions and four stress
filters are unchanged. Quiet profiles validate platform auxiliary records
only if invoked. N0 variables are omitted, bytecode writes enabled and each
target uses a writable disposable `/tmp` cache. Global two-slot/build locks
and shared perf-map serialization remain. Stop on unexpected failure;
there is no retry-green or additional qualification campaign.

## Retained failures and limits

All original failures remain failures: scanner oracle attempts and their
test-only corrections; old environment/cache/helper failures; the unknown
ambient in-source build and failed make precondition; the O3 zero-process
log-path error; and the ASan pre-gate missing native-directory rejection.
The latter started no helper/configure/compiler or test; creating the owned
empty build directory does not earn native credit. Preserved dc7 payloads
are evidence, not runnable relocated images.

The support boundary is Linux x86-64, shared GIL, one main interpreter with
ordinary threads. Public subinterpreter creation rejects explicitly; no
subprocess emulation or foreign-adoption qualification is claimed. Windows
Overlapped/_winapi persistent buffer owners remain unsupported. External
extensions must follow the supported ownership/root API; useful future
NumPy ports are not qualified by these core tests. Live PickleBuffer Python-
wrapper re-export remains separately named functional debt.

Unpinned counts are report-only; zero fallback is required. Borrowed-header
closure and historical ASan SDK/source-path limitations remain explicit;
this is not exhaustive system-header, platform, leak-sanitizer or arbitrary
buffer-graph coverage. Historical HTTP/queue/shutdown failures and scope
exceptions remain in the coordinator record. Process-exit retention is not
permission to leak normal-runtime acquisitions or claim safe repeated
`Py_FinalizeEx` in a live embedding host. Generations, moving collection,
foreign lifecycle and new cleanup/completion frameworks are outside this
landing. Final evidence indexing remains separate from the preserved
individual run outcomes and source approvals above.

## Post-merge addendum: integrated75e8

The coordinator integrated qualified4da as
`75e8a1e0c01a69b0e34f667c4dac0ad837da4c68`, tree
`8a309fd40dd0fcf5ac0d7e8b1242050da73b5e6d`. Independent Git inspection
confirms exactly15 declared project Markdown paths differ; every other
tracked byte equals4da. This does not claim native images built from primary:
runtime library/tests remain dac6, ordinary native code3c6 and ASan f945.

The final smoke packet changes only actual merge/qualification-index bindings
from its reviewed held template; helper, two literal vectors, jobs and exact
skip expectations are unchanged. The21 input hashes and final12f1 index
match. Expectations were derived from actual qualified nine-module returned
rows and outer skip counters: release1,935/80 and debug1,935/50.

Both once-only off/N0 smokes and cleanup independently clear. Release result
ed2fc200/cleanup174c3bff replays to nine PASSED modules,1,935/80, ten paired
driver/workers and773 collections, zero fallback/unpinned. Debug result
4a1ec4a9/cleanupc99b16ef replays to nine PASSED modules,1,935/50, ten paired
driver/workers and812 collections, zero fallback/unpinned. Exact skip counters,
source/configuration/native/integration bindings and raw artifact hashes
agree. No active result was credited and no target was run by this reviewer.

`build/gc-main-only-integration/final-receipts.json`, SHA256
`f56e0b6b85c2bd07e06687a0ce1957f28011ecca5d48547e6dcd30bde843f49b`,
binds20 absolute references, all independently rehashed. All run/verify
return codes are0 and no native job remains. The coordinator pushed75e8
after this independent integration clearance. Original failed runs, composite
source/resource identities, authorship exclusions and support limits above
are unchanged. This addendum closes primary integration; it supplies no
refreshed performance result or broader platform/lifecycle claim.
