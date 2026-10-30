# Pre-JIT roadmap: GC correctness and coherent interpreter design

Coordinator plan, revised2026-10-30. Roughly2× is a **soft design guide**,
especially for obvious arithmetic such as `i += 1`, not a mandatory aggregate
score or a numerical gate before JIT work. Finish the outstanding GC fixes
first. Then publish a detailed interpreter status report and commission
independent design reflection where performance disappoints. Prefer simple,
established techniques and orthogonal operation families over local tuning.

The user's example is a parameterized LOAD_ATTR family with int32/float/
string/object modes, rather than unrelated one-off typed opcodes. Review
which parameters describe stored representation, a result constraint or an
observed specialization, including guards, fallback, boxing, monitoring and
roots. This is a design question for the later reflection, not approval to
duplicate handlers or introduce a second value representation now.

The prepared matched Clang goto/tail/frame-passing timing plan is parked
during GC implementation. Preserve its existing builds and bounded runtime
evidence. After GC, report the actual IR/handler/value/call/GC state and
costs before implementing the broader interpreter cleanup. Read-only quality
review and the prospective improvement list now proceed in parallel with GC
qualification at the user's request. Rank clean operation families, value
bridges, common frame/call costs and dispatch by source evidence and expected
breadth; identify what fresh measurements would confirm each proposal.
Recommendations remain provisional until matched timing after GC landing.

Current GC checkpoint,2026-10-30 11:35 PDT: safetyff6 is landed. Final
main-only/drainer/buffer source3c6b8daa506 is complete, including ND and
same-owner XID ports and the two safeguard removals. Component O1 evidence
passes through explicit test-only corrected composites; final-union native
qualification remains: fresh O1 new13/whole9 pass, but broader profiles and
closure are pending. An unknown ambient in-source build blocked its first
make before compilation. Preserve it and qualify in a clean existing source
worktree with a fresh owned O1 build. Four causal controls and the behavioral scanner successor now pass their
bounded checks; release/ASan/full/closure and landing precede fresh timing.
The [provisional source report](pre-I-interpreter-status-and-opportunities.md)
now inventories the implementation and seven ranked shared opportunities.
The [closure plan](gc-closure-plan.md) and [owner ledger](10a-buffer-safeguard-removal.md)
name the exact boundary. No refreshed performance score exists yet.

**How we judge the result.** The primary score is the geometric mean of
upstream elapsed time divided by fork elapsed time across the declared
application/kernel suite, currently 29 workloads. Roughly 2× means roughly
half upstream's elapsed time. Publish every workload, including regressions;
do not improve the score by dropping difficult cases or averaging in tiny
microbenchmarks. Report allocation-heavy workloads, string size scaling,
startup, peak RSS and GC pauses alongside that score. Extend the application
suite before optimization if coverage is missing, and freeze that decision.

Both runtimes must use their own source, standard library and compatible
extensions. Declare compiler, target ISA, frame pointers, tailcall, PGO/LTO
and training policy before a comparison. Keep controlled attribution and
production-policy comparisons distinct. The first owned upstream build is
GCC/O3 with frame pointers and no PGO/LTO/JIT; the fork's `-fno-gcse` remains
disclosed and requires upstream sensitivity or a fully matched eval-policy
cohort before a parity/speedup claim. If production configuration changes,
build an appropriate upstream comparator too. Py_STATS explains mechanisms;
nonstats release elapsed time decides whether an optimization helps.

Historical 6d instruction ratios are 1.161 for kernels, 0.693 for micro and
1.699 for memory workloads. They establish neither current elapsed parity
nor progress to 2×. The old baseline's source/library binding is incomplete.
An owned exact upstream `321925` build and initial origin/correctness
qualification now pass independent review. All50 upstream actual
benchmark-driver origin rows also pass independent review.

The first release elapsed diagnostic finished15:30 PDT on frozen unlanded
ce2ac: upstream/fork geometric mean **0.828243** across all29 kernels,
22 slower rows and seven faster. This is about1.207× the upstream elapsed
time in geometric-mean terms. Largest regressions are coroutines0.365,
comprehensions0.528 and generators0.592; strongest wins are float1.923,
Richards1.897 and nbody1.603. The [timing record](pre-I-first-quiet-timing-plan.md)
retains all29 rows/290 values and independent actual-scope review.
Five serial samples after warmup are an initial estimate. Unresolved GC
stress failures, fork-driver origin limits, fork-only -fno-gcse and incomplete
consumed-SDK proof prevent correctness, parity or target qualification.
Do not automatically rerun the suite or claim the ratio explains its causes.

Reaching the illustrative2× from this historical diagnostic would require
about2.41× that fork's throughput; this is not a required work quota. The
[shared-cost record](pre-I-shared-cost-attribution.md) includes
a conditional Amdahl calculation: even removing all measured GC work leaves
the three slow rows around or below upstream parity if non-GC work is fixed.
GC reduction and the common frame/call/value/interpreter changes must both
contribute. This is a planning model, not a speedup forecast.

**Why twenty was not an optimal default.** Note 11, section 8.2, required 20
passes both individually and in full-module order. The rule was carried into
the newer five-selector/four-module campaign, without a measured failure
probability or stopping model. Both old 640-job campaigns passed; the full
debug suite then exposed a thread-pool case outside that selected coverage.
The original failing cases, root trace and classification negative supplied
the causal evidence for the Park fix. Repetition added confidence but did
not substitute for coverage or an invariant-specific oracle.

Future campaigns start with a reproducer that fails on the old code, a
negative control that breaks the claimed invariant, and a successful fixed
case. Run affected modules once in each relevant configuration, and run
broad coverage early on the milestone candidate. Costly repetition needs a
named race/retention hypothesis, relevant schedule/stack/build variation,
recorded exposure and a bounded stopping rule. Stop and diagnose on failure;
do not retry until green. Cheap inner stress loops may remain where they
exercise a specific hazard; they are not equivalent to repeated whole-module
campaigns. Twenty all-green runs have no special statistical status: even
under independent trials, a 1% failure probability is missed about 82% of the
time. Our runs are not established independent trials.

Reuse established launchers and source/build manifests. Group related checks
into reviewable campaigns; avoid creating a new wrapper/provenance framework
for each optimization. Receipts keep their real source identities. A reviewer
may accept unchanged historical evidence with an explicit source/native
argument, while current-candidate correctness gates stay fresh. Never rename
old tests as new-head passes. This revises future repetition policy, not past
receipts or test outcomes.

**1. Finish and land the current precise-root repair.** ROOTS 2b originates
at `ce2ac8991ee`; the current combined successor is5a45 (README tracks its
qualification). Its substantive repairs cover PyMem-origin attribution,
list-to-tuple temporary lifetimes, multiprocessing cleanup and Park retention.
The two latest raw-mutex annotations clear source review and actual linked
audits on release/O1-debug/ASan. Both completed 120-transaction campaigns remain
historical evidence on `1bfe`, with zero fallback. All pre-change products and
journals are preserved.

The completed bounded comparison covers emitted root classifications, unwind
rules and root locations, including the four actual semaphore helpers.
Default/debug function membership and ranges match; ASan intentionally moves
only two raw wrappers into the precise section. Selected relocation/string
and root/unwind equivalence do not imply whole-image byte equality. Root
accepted historical carry-forward under that bridge.

The fresh affected campaign now passes once per relevant configuration:
release off, O1-debug off and actual debug kill. Its24 transactions cover
strict stale-root probes, complete compiled stack walks and Park caller-slot
controls, the three thread regressions, both eval-root cases and the complete
changed generated-cases module. They are not another20 iterations. The cheap
internal stale100 probe remains this landing's existing hazard check. Fresh
tail build and six strict stale100 cases also pass; bounded tail StackWalk
waits for fulls because perf-map cleanup interferes. No new test exclusion
or root-policy fallback is introduced.

Current-source full release, full debug normal and full debug kill now pass
51,132 tests each with zero fallback. Both11-module stress sets pass3,062;
ASan16 kill passes3,976 under the exact existing allocator filter/options.
The final filtered release full at interval100,000 is terminal FAIL: an
interpreter assertion, a20-minute trashcan-thread timeout and HTTP unraisables.
The timed-out worker prevents a qualified aggregate root-counter result.
The [triage](10a-2b-stress100k-triage.md) separates threshold scheduling from
actual collection/retention and HTTP lifecycle causes. No new exclusion or
retry is introduced. Four reviewed once-only interpreter diagnostics now
separate stress-trigger semantics from collection/retention. The resulting
two-line threshold-test eligibility annotation passes its normal case and
declared stress skip; the scheduler is unchanged. Existing public HTTP cases
reproduce the closed-fp error on primary052 and ce, and the unchanged trashcan
case times out on both at the bounded diagnostic deadline. These establish
inherited exposure, not mechanism equality or absence of interactions.
Independent review supports a scoped incremental-landing exception, accepted
by the coordinator16:09. The failed full stays failed with no aggregate.
HTTP/finalizer ordering and queue progress block final GC/performance
qualification. A successor bound to the actual test/docs source and unchanged
ce native bytes may now finish tail StackWalk, cext, unchanged regeneration/
traversal and declared optional-header checks. Old frozen plans stay intact.

Exact candidate review and primary integration smoke follow. Bytes3a's
registered owner-slot patch and focused positive/causal-negative/stress/ASan
results clear coordinator review. Compose it with ROOTS2b into one detached
candidate and run one fresh mandatory union ladder, including normal and kill
fulls and applicable verification checks. Two consecutive near-identical
integration ladders would add little attribution beyond the retained focused
receipts and frozen parents. This changes scheduling, not coverage. Full
suites serialize where shared host state requires it; independent bounded
work can use the other normal machine slot. O1 keeps debug assertions and
symbols and remains a candidate until its required normal/kill gates finish.
The old 20-iteration matrices are not automatically restarted.

**2. Separate the remaining correctness debt from performance development.**
Landing 2b does not finish all GC work. Track writers/PyMem corruption (3a),
checker/precision residue (3b), active queue payload/saved-exception and
detached/swapped thread-state roots (3c), and dependent-finalizer completion
ordering. Fix actual crashes and missing-root hazards before calling the
runtime ready for its performance checkpoint. The queue-completion lane has
an existing automatic-classifier hold; do not route around that hold or
describe it as missing user authorization.

H3 integration and performance analysis can proceed after this checkpoint while independent
GC fixes continue. H2's removal of clearing stores waits for the complete
reader/span contract from rung 3. Generations and finalizer delivery must use
the resulting stable root/lifecycle semantics. Moving collection and package
ports are separate work; the 2× target does not silently authorize a moving
nursery or a broad ABI rewrite.

**3. Integrate the ready IR work and obtain trustworthy attribution.**
After ROOTS lands, compose the frozen H3 monitoring/domain/storage candidate
with primary. Review the five shared files semantically and regenerate the
handler body; a textual merge is insufficient. Run affected monitoring,
exception, compiler and root controls on the actual union, then its required
landing checks. Use the existing
[handoff](H3-post-ROOTS-handoff.md) rather than reopening the whole design.

Fix handler statistics to cover the complete generated domain, including
prediction paths. Distinguish physical handler entries from semantic operation
events. Remove inherited numeric category exports through generated semantic
properties, not arbitrary opcode thresholds. Existing register CFG passes
already perform copying/folding/compaction; this is not a plan to rebuild that
compiler. Audit remaining effect metadata and output copies against observed
hot code, especially operations that can invoke Python.

Reuse one authoritative description of operands, windows and effects for the
compiler, final-IR liveness and later escape proofs. Extend existing generated
metadata where allocation, reentrancy, exceptions or aliasing need a stronger
contract; a format/borrow mask alone is insufficient. Avoid parallel hand-written
opcode classifications or a separate optimizer for each consumer.

Readiness includes the actual benchmark driver's setup/run import and native
dependency closure in the [baseline plan](pre-I-baseline-postbuild-plan.md);
the startup diagnostic and correctness suite alone do not establish it.
As soon as that environment and an immutable fork candidate are ready, take
a bounded quiet release measurement window. The coordinator may use ce2ac's
normal-full-qualified release as an early diagnostic before landing, with its
unresolved stress failures explicit. This provides no correctness, parity or
target qualification. Focused diagnosis and ready GC fixes take priority;
a noisy window must not block source work. Do not wait for
every proposed optimization before obtaining the first honest wall-time
baseline. Rank recurring costs using elapsed profiles plus boxes/allocated
bytes, GC root/mark/sweep time, survival, dispatch mix, spills and code size.
Historical ROOTS2b instruction records also show keyword-call (+7.1% and
+14.0% on two rows) and attribute-span (+2.8–13.1%) costs. Preserve their old
source identities and refresh elapsed/box/root attribution on the stable
candidate. The old suggestion that step8 will replace those paths is not a
prospective pre-JIT disposition. These measurements determine the next
implementation priority; no imaginary per-technique speedup budget is
compounded into a 2× promise.

The first elapsed result makes allocation/GC, frame creation/resume and
call/value movement the immediate attribution priorities. Profile the shared
mechanisms in coroutines, generators and comprehensions, with a fast numeric
or object workload as a control. Separate setup from timed work and distinguish
whole-process root counters from the hot window; reuse the existing root/pause
and native profiling tools before adding instrumentation. Full-domain handler
statistics will improve dispatch attribution, but existing native symbols
and collector counters need not wait for that landing. A result on one row
does not authorize a row-specific opcode or a custom loop optimizer.

The [first shared-cost attribution](pre-I-shared-cost-attribution.md) now
completes all four once-only windows on unchanged ce release. GC consumes
54.0% of coroutine hot time, 47.0% for comprehensions and 30.8% for generators;
Richards has no hot collections. Native software samples corroborate collector/
traversal work, with zero lost events but incomplete ancestry/unresolved
symbols. This prioritizes a sound nonmoving-generations/remembered-set design
and allocation churn over optional dispatch-convention tuning. It does not
prove a generation implementation or predict its gain. H3 correctness and
counting integration continues; H1 stays a bounded later experiment. A
source-only generations design must first establish complete mutator/barrier,
root, weakref and finalizer contracts. Known lifecycle defects remain blockers.

As of 17:50, GC candidate f2a5 includes ROOTS and bytes-writer roots. Its
16 early jobs pass; release coverage is assembled from 563 qualified original
module cohorts plus one corrected-environment whole test_cmd_line. The original
full remains failed from two launcher-environment effects. Both debug fulls
pass51,134 tests with zero observed fallback. Release stress11 then fails
one stack-hit type assertion; debug stress, ASan and extras have not started.
Historical records match reused numeric addresses, and the reader separately
holds a borrowed journal pointer across a potentially collecting allocation.
Neither finding alone establishes the observed failure cause. Preserve the
red run, diagnose once, repair the actual contract and qualify affected work;
no retry-to-green or new exclusion follows. H3 union e9a1 has owned Py_STATS/O3/Og
builds, real producer/reader checks and eight bounded smoke/H3/stale passes;
three integration jobs subsequently pass on e9. The next combined source5a45
adds the source-reviewed reader/test and Unicode ownership repairs plusa285;
one fresh O3/O1-debug ladder qualifies that actual source. At19:07, all19
affected jobs and both ASan groups pass, with zero observed fallback.
Matched Clang goto/tail bounded runtime and stale-root checks also succeed;
tail ownership executes. Release full starts the five-profile serial ladder,
followed conditionally by the final C-extension/regeneration/traversal/header
audits and independent landing review. This source is not yet qualified or
landed. In parallel,
[finalizer-order eligibility](pre-I-finalizer-order-proposal.md) is implemented
and [semantically checked](pre-I-finalizer-order-checkpoint.md) on O3 and Og
kill at a285 in isolation: avoid DFS roots/order entries for finalizers that
cannot run, retaining dead transit traversal. The copied reversal negative
fails its intended assertion. The later matched four-row diagnostic gives
parent/candidate ratios1.250/1.055/1.070/1.033, with all40 values retained and
known qualification/layout/sample limits. It supports carrying this small
change into broad qualification; it does not refresh the upstream score. This addresses avoidable death-pipeline work,
separately from old-live scanning. The
[generations direction](7-nonmoving-generations-design.md) clears review but
requires the closed store/constructor ledger and concrete promotion/commit
interfaces before activation. The [frame/store ledger](7-generations-frame-store-ledger.md)
now identifies pre-link send, yield/return/throw detach and escaped-frame owners;
it is a future-minor obligation, not evidence of a current full-GC defect.
Its allocator/domain ledger now identifies the
young-only enumeration boundary, address reuse and abandoned heaps. The existing
pre-sweep Python debug writer is an earlier reentry dependency than stop callbacks.

**4. Simplify representation and value flow.** Advance 6e canonical ingress,
inline numeric receivers and remaining slot/Clinic bridges. Keep exact int32
values canonical so integer guards need one inline representation; preserve
bool/subclass and boxed-NaN semantics. Remove demonstrated box/unbox round trips
at call and C-API boundaries. Require measured box-site removal and broad
release benefit, not just a faster isolated helper. This lane can overlap
handler work where file ownership and interfaces are distinct.

**5. Make the generated interpreter cheaper under one protocol.** First run
the bounded H1 frame-passing experiment against an identical parent build,
including all handler entries, predicted dispatch, error paths and callbacks.
Keep it only if release elapsed time improves without harmful spills/code-size
growth. It is the smallest historical handler cost, not the central 2× theory.
Then simplify canonical numeric guards and fix demonstrated register-base
spills. Generated properties and coherent calling conventions apply to all
handlers; do not add benchmark-specific variants. Keep per-word diagnostic
position publication and eval-breaker behavior.

After precise readers/spans are ready, H2 derives physical-slot liveness from
the final IR, including synthetic jumps and normal/exception/suspend edges.
Validate construction and live-root/retention negatives before removing GC-only
clearing stores. All frame readers must respect the map; arbitrary code
construction cannot supply a trusted bitmap. Measure map size, construction
cost and lookup overhead. Use that common liveness/alias contract to place call
arguments directly in initialized windows and eliminate copies safely. Typed
float truth reuses the existing branch semantics. New operations must express
reusable semantics and remove broad costs; no fused-pair or handler-count goal.

**6. Reduce collection and copying costs structurally.** Introduce nonmoving
generations using a coherent age/mark policy, a remembered set for old-to-young
heap edges and an explicit root-scanning contract. Cover all value/container
APIs that can create those edges. This targets repeated full-heap work
without requiring movement. Barrier-omission negatives must fail; demonstrate
throughput, retained memory and pause distributions on short-lived and live-set
workloads. Design the barrier together with the settled value/container model.
Do not assume that a generation label alone improves pauses or throughput.

Advance linear string concatenation through its existing proof/ownership plan:
shadow proof first, private-capacity storage and observer closure next, growth
enabled last. Agree header/capacity layout with generations before storage edits.
A proven fresh nonescaping string can grow geometrically without
changing aliased immutable strings. Validate size scaling, aliases, allocation
failures and observer effects. This is an asymptotic optimization, with its
own complexity cost; implement when string copying is a material workload cost.

**7. Re-measure and decide the next structural change.** After each coherent
landing, compare against its parent and refresh the application score in quiet
windows. Keep individual regressions visible, including generators/coroutines,
deepcopy, comprehensions and range spills. Use profiles to distinguish remaining
GC, representation, call and dispatch costs. Revert or omit inconclusive local
optimizations; revise a structural design when measurements identify a larger
problem. Do not chase tiny instruction wins while a larger runtime cost remains.

Checkpoint I reports defensible individual results and memory/pause tradeoffs.
Roughly2× guides reflection, especially on simple arithmetic; it is not a
required aggregate result. A shortfall commissions a subagent to inspect the
design for clean general improvements. Preserve the workload and distinguish
interpreter results from future JIT results. Report the design tradeoffs and
choose the next coherent change without an open-ended score-chasing loop.

**Coordination.** The coordinator owns this plan, measurement choices, shared
interfaces and integration. The GC operator owns the current5a45 broad ladder
and final audits. The handler owner has completed Clang baseline validation
and has compiled H1 release/debug code; its debug classifier correction passes
a fresh audit on unchanged native products. Candidate runtime/root checks
and matched elapsed comparison follow. The inert H2 scalar decoder completes
its bounded native validation,
including the test-only generated-table parser correction. Its owner has moved
to a source-only Queue2a safety/dependency design pass. The handler owner now
reviews GC evidence independently outside their H3 authorship. No current H2 consumer
uses these partial effects as complete semantics or root maps.

Full suites and perf-sensitive walks serialize because they share perf-map
files. Other admitted builds may use the second normal machine slot; quiet
timing waits for native validation and a reserved window. Preserve frozen
predecessor failures rather than relabeling them as successor passes. Finish
independent reviews before a reviewer changes roles to implementation, and
have another reviewer assess that implementation. Queue completion and
generations remain separate correctness/design obligations; the current
incremental GC landing does not close them. Report evidence and blockers
hourly, with implemented, validated and landed work distinguished.
