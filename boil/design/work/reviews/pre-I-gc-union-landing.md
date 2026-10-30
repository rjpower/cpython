# 5a45 incremental GC review

Current disposition: **LAND_INCREMENTAL** for exact5a45 under the existing
coordinator exception, with the independent-H3 boundary below. Round1 is
retained; the terminal round2 closes its pending qualification items.

## Round1: source and completed evidence intake

Candidate `5a45f9bb6a73e7351c861faf05624ae4810f6d87`, relative to qualified
queue base `052d8342e562b1b10d10d26d640c98580616a035`. The complete non-boil
delta is110 paths. That is a different domain from the older67-path
H3-merge-base→ROOTS handoff and70-path primary→ROOTS/bytes inventory.

Verdict at this checkpoint: **GC source/composition scope clear; final
incremental qualification pending terminal stress/closure and coordinator
disposition.** No final GC, performance, upstream parity or LAND verdict.

## Independence and source scope

This reviewer authored H3 statistics/composition and H1, not the GC/ROOTS,
bytes/Unicode writer, reader/history or finalizer-order runtime patches.
H3 source independence relies on the existing coordinator/string reviews
`H3-stage2b3-source-review.md` and `H3-union-source-checkpoint-review.md`;
this is not independent self-review of H3. H1 and its tool-only strrchr
prerequisite are outside candidate5a45 and supply no GC landing credit.

The reviewed ROOTS/bytes source carry-forward is the actual f2 composition,
not reuse of old native receipts as new-head qualification. Its Park caller
contracts, two precise raw-mutex wrappers, PyMem carrier attribution,
list/tuple handoff, cleanup ordering and exclusions retain their component
reviews. The recorded f2 source composition confirmed native equivalence to
writerf5 except the two test-eligibility lines. H3's six shared inputs and
seven regenerated products have their distinct composition review. None of
those old builds qualifies the changed5a45 native cohort.

The additional GC changes were checked against the actual source:

- `Modules/_testinternalcapi.c:4485`: the stack-hit getter preserves the
  initial count and refreshes the borrowed journal immediately after initial
  list allocation; subsequent per-row refresh remains. The history test
  clears within the live Probe's address interval. This closes the confirmed
  move gap under the previously reviewed ordinary-allocation/no-Python/GIL
  contract; it does not prove the historical tuple assertion's cause or
  remove general queue/debug-callback debt.
- Unicode ownership retains the prior independent source review, including
  the21 stable owner lifetimes, InitWithBuffer entry, consumed-resize handoff,
  exactly-once root removal and shared/Windows private-test linkage. The
  private writer layout moved internal and private clients must rebuild;
  public opaque writer layout compatibility is not a claim about private
  consumers. Source and test migration have no new composition conflict.
- `Python/gc_live.c:740`: finalizer eligibility filters DFS seeds and emitted
  finalizer calls, not `order_visit` edges. Nonfinalizing/finished generator
  transit nodes remain traversed from eligible roots. DEAD/FINALIZED state
  and the existing generator-finalizer identity/finished predicate are
  rechecked before queuing. The closed-generator constant-tuple regression
  exercises a real transit edge. This changes neither general sweep order,
  resurrection nor queue-completion semantics, and does not fix their OOM
  or reentry obligations.

No additional integration source blocker was found in this bounded review.
It does not replace every historical component review with a new audit of
all110 paths.

## Completed evidence checked

`completed-evidence-checks.json` records receipt/cleanup/raw-artifact hashes
over24 completed immutable rows: affected19, ASan2 and corrected normal
full3. This includes21 host-only replays of the existing V3 validator and
three strict stale transcript/start/end/N0/main0/counter predicate checks.
The stale rows each have six100/100 cases. No native
test or retry ran during this evidence review.

| Completed coverage | Reported tests / skips | Scope |
| --- | --- | --- |
| Affected19 | Separate per-row totals retained | Own source/products, normal/kill, Unicode/GC/H3/integration, strict stale and reader normal/N1000 |
| ASan16 kill |3983 /92 |16 modules, zero fallback;6651 unpinned report-only |
| ASan Unicode7 kill |1037 /24 |7 modules, zero fallback;6 unpinned report-only |
| Corrected O3 normal full |51181 /3469 |564 modules;63643 collections, zero fallback;2832 unpinned report-only |
| Corrected O1 debug normal full |51181 /3412 |564 modules;67079 collections, zero fallback;4013 unpinned report-only |
| Corrected O1 debug kill full |51181 /3412 |564 modules;67061 collections, zero fallback;4009 unpinned report-only |

Each full has565 actual driver/worker starts paired with ends. Descendant
root records and root interpreter/domain counters remain separate from
those mode witnesses. The full outcomes include462 PASSED,8 RESOURCE_DENIED
and94 SKIPPED modules;51181 is the reported test total, not all tests in
every listed module executing without skips. Existing skip/protection and
whole-module loader evidence supports protected-case eligibility inference;
it is not a newly invented per-test success journal.

The original bytecode/cache launcher failure remains FAIL. Corrected fulls
use writable /tmp bytecode cache, omit truthy N0 stress environment variables
and protect both byte-trigger cases plus the declared bytecode cases. The
fresh fulls replace that coverage, rather than assembling old mixed-environment
results. Added generated bytecode is separately recorded while every original
native/config product remains bound. Actual Clang baseline four-row checks
also passed under their own receipts and coordinator review; they are not
H1 qualification.

## Retained boundaries

The primary PROCESS incremental exception remains authoritative. The old
ROOTS stress100k interpreter/HTTP/GC-timeout profile and f2 stack-hit stress
failure remain failed historical evidence. HTTP finalizer/resource ordering,
queue completion, sweep-order OOM and debug Python reentry are unfinished
correctness obligations; new Queue2 design findings are not repaired by
this candidate or these tests. No held Queue2 probe was retried here.
Report-only unpinned counts are not zero or a proof of complete precise
coverage. Historical ASan SDK/consumed-header/source-path limits remain;
fresh5a45 products are distinguished from any old writer-product reuse.

Stress11 release was reported terminal by the owner after the24-row intake;
its independent raw replay, debug stress and closure remain to be appended.
Do not credit any in-flight row or infer final qualification from file presence.

## Round2: terminal incremental landing disposition

**LAND_INCREMENTAL** is recommended for exact
`5a45f9bb6a73e7351c861faf05624ae4810f6d87` and a coordinator-reviewed
landing that preserves its native/test/tool source. No additional mandatory
qualification item for this declared incremental scope remains open in the
reviewed packet. This is a recommendation to the coordinator, not an action
to merge/publish or a general correctness waiver.

The immutable final index is
`boil-pre-i-union/build/pre-i-qualification/final-receipts.json`, SHA256
`e8f95745e49226cdab8c8f8f0c4c55210f2f9ea7d73ecad0cb5ee3c42d636cdd`.
It binds all seven completed profiles, cleanup, four closure rows, original
failed full and the affected/build references. The independent
`final-evidence-checks.json` checks49 indexed artifact references and both
terminal stress rows through the existing pure V3 validator. All11 modules
pass in each stress row, actual normal/off N1000:

| Stress11 | Reported tests / skips | Collections / fallback / report-only unpinned |
| --- | --- | --- |
| Release |3065 /100 |44771 /0 /6427 |
| O1 debug |3065 /64 |49107 /0 /7871 |

The exact existing resize/writer/format/verify/remote stress exclusions are
retained; these rows do not establish unfiltered writer stress coverage.
Writer qualification instead includes its explicit affected normal/kill,
ASan and complete normal-suite contexts. Fulls retain their exact remote
test exclusion. No new filter was introduced by this review.

Closure checks passed with the original raw artifacts: cext16 with the
exact seven known skips; unchanged regen-cases/Clinic and traversal;19
unbuilt-source header receipts with no warning/failure and four explicit
unsupported modules. The2915 borrowed-header bytes/aliases and C++ compiler
aliases/bytes were independently rechecked. The outer closure return0
follows the frozen helper's post-gate `loaded()` check; the receipt has no
invented `post_cleanup_verified` field. Its source path proves that guard
completed before the receipt was written.

`native-current-check.json` independently rechecks all588 original product
paths per O3, O1 debug and ASan build, including resolved targets/symlinks,
configuration and tool bytes. They are unchanged. The only added files are
six regular in-build generated bytecode files per ordinary cohort and none
in ASan. The source remains clean5a45. There was no new configure/compiler/
target/test execution by this evidence reviewer.

There is no remaining *new integration blocker* in this source/evidence
review. Remaining correctness obligations are the explicitly retained
HTTP/resource-finalizer, queue completion/order, sweep-order OOM and debug
Python reentry debt. Those still block final GC/performance qualification
and require their own work; historical FAIL receipts are not relabeled.
There is no blanket20-repeat or stress100k retry requirement or rare-failure
confidence claim. Platform/SDK and report-only precision limits stay as
stated. H3 independence still rests on the other reviewers' records; no
self-authored H3 source is independently cleared by this review. Any native
source change while preparing the actual landing invalidates this exact
candidate disposition and needs its own bounded reconciliation.
