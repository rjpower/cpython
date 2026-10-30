# Pre-JIT roadmap: GC correctness and coherent interpreter design

Coordinator plan, revised2026-10-30. Roughly2× is a **soft design guide**,
especially for obvious arithmetic such as `i += 1`, not a mandatory aggregate
score or a numerical gate before JIT work. The declared GC milestone below
is complete. Publish the detailed interpreter status report and commission
independent design reflection where performance disappoints. Prefer simple,
established techniques and orthogonal operation families over local tuning.

The user's example is a parameterized LOAD_ATTR family with int32/float/
string/object modes, rather than unrelated one-off typed opcodes. Review
which parameters describe stored representation, a result constraint or an
observed specialization, including guards, fallback, boxing, monitoring and
roots. This is a design question for the later reflection, not approval to
duplicate handlers or introduce a second value representation now.

The prepared matched Clang goto/tail/frame-passing experiment remains isolated
and unmeasured. Preserve its builds and bounded runtime evidence; H1/H2 are
not landed, while H3 is landed. Choose subsequent implementation from the
shared-cost evidence and the [source report](pre-I-interpreter-status-and-opportunities.md),
not a guessed gain or a new ABI by implication.

**Current checkpoint,2026-10-30: supported GC integrated75e8.** Qualified4da
contains runtime library/tests dac6, ordinary native3c6 and ASan nativef945.
Final12f1ea31 and postmergef56e0b6b bind the complete qualification and both
1,935-case smokes. The actual merge differs only in15 project Markdown files.
Normal/kill debug fulls each pass51,240 tests; stress2, ASan, explicit
release557+7/ASan15+memoryview assemblies and closure clear independent review.
The original release and traversal failures remain failed; supported callback
and buffer-completion work is no longer a landing blocker. Foreign/adoption,
platform and precision obligations remain separate from this profile.

**Current phase: matched upstream eval-nogcse qualification.** The owned
stock321925 image is preserved. The separate build/eval-nogcse has passed
configure/make/product closure23c9ab25 and independent reviewc51871cb, with
the declared GCC/O3/FP/zero-init policy and CFLAGS_CEVAL=-fno-gcse. Bounded
correctness/origin checks remain pending before any quiet29 refresh. Reuse
the qualified fork O3 image with its actual library/native bindings. No
refreshed performance score or new interpreter ABI implementation is claimed.
The [timing plan](pre-I-qualified-timing-refresh.md)
and [first-slice comparison](pre-I-first-slice-decision.md) keep measurement
and implementation decisions distinct. Roughly2× remains soft guidance.

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

**1. Preserve the completed supported GC boundary.** ROOTS/writers/H3 landed
through d6a23; safetyff6 and supported75e8 then close sweep/snapshot/work-stack,
main-only admission, one rooted drainer, acquisition readiness and finite
consumer/resource lifetime. The [closure plan](gc-closure-plan.md) and
[independent LAND review](reviews/gc-main-only-final-landing.md) retain the
actual source/native/resource composites and causal controls. Do not restart
old120/20 matrices or relabel failed stress100k, release or traversal runs.
Current HTTP normal/kill and trashcan normal/kill/N1000 coverage have their
actual skip/source contracts; no passed100k rerun is claimed.

**2. Keep separate correctness and support obligations explicit.** Linux,
one main interpreter, ordinary threads and the shared GIL define the current
milestone. Foreign/adoption and repeated interpreter deletion, Windows
persistent holders and parallel callback progress are deferred. Further
checker/readers/PyMem/detached-root precision work does not authorize removal
of safe conservative coverage. Live PickleBuffer Python-wrapper re-export is
separate functional debt. No generic cleanup slot or dependency DAG is an
unfinished requirement of the supported profile. Generations, moving
collection and package ports retain their own source/proof/qualification
boundaries; a soft performance goal does not authorize them automatically.

**3. Use landed IR work and obtain trustworthy attribution.** H3 monitoring,
domain/storage and statistics are landed and qualified. Its generated domain
includes prediction paths, and readers distinguish physical handler entries
from semantic events. Categories use generated properties rather than numeric
thresholds. Preserve those interfaces and the existing
[handoff](H3-post-ROOTS-handoff.md); no H3 reimplementation is needed.
Existing register CFG passes
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
The historical ce2ac window was an early diagnostic with unresolved failures,
not correctness or parity qualification. The next quiet release window uses
the qualified75e8 code and the separately built/qualified eval-nogcse upstream
cohort. It does not wait for every proposed optimization or reuse old upstream
origins as proof for a new binary. Rank recurring costs using elapsed profiles
plus boxes/allocated
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
and native profiling tools before adding instrumentation. Landed full-domain handler statistics support dispatch attribution alongside
the existing native symbols and collector counters. A result on one row
does not authorize a row-specific opcode or a custom loop optimizer.

**Historical attribution and qualification checkpoints.** The following
records explain earlier priorities; their source identities and failures remain
historical. Supported GC qualification is now complete at75e8. Refresh the
allocation budget and live-set attribution before choosing collector complexity.

The [first shared-cost attribution](pre-I-shared-cost-attribution.md)
completed all four once-only windows on unchanged ce release. GC consumed
54.0% of coroutine hot time, 47.0% for comprehensions and 30.8% for generators;
Richards has no hot collections. Native software samples corroborate collector/
traversal work, with zero lost events but incomplete ancestry/unresolved
symbols. This prioritizes a sound nonmoving-generations/remembered-set design
and allocation churn over optional dispatch-convention tuning. It does not
prove a generation implementation or predict its gain. H3 correctness and
counting integration was then in progress and has since landed; H1 remains isolated. A
source-only generations design must first establish complete mutator/barrier,
root, weakref and finalizer contracts. Lifecycle defects blocked that checkpoint;
the declared main-interpreter profile is now qualified.

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
pre-sweep Python debug writer was an earlier reentry dependency than stop callbacks;
the landed deferred-output repair closes that supported-profile hazard.

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

**6. Reduce collection and copying costs structurally.** First measure the
existing allocation budget against throughput, memory and pause requirements.
If fresh evidence justifies generations, use a coherent age/mark policy, a remembered set for old-to-young
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

**Coordination.** The coordinator owns comparison policy, reservations and
integration. GC qualification and post-merge smoke are complete. Current work
prepares the matched upstream cohort with accepted configure/make and bounded
correctness/origin protocols; a new quiet transaction requires its own review
and reservation. H1 frame passing has isolated native/runtime evidence but
no elapsed benefit; the informational header-harness limitation stays named.
H2 remains an inert partial decoder, not active maps or clearing. H3 is landed.
No numeric NOARGS ABI or other prospective source recommendation is implemented.

Full suites and perf-sensitive walks serialize because they share perf-map
files; normal admission remains two slots. Preserve predecessor failures and
actual mixed-source evidence identities. Another reviewer assesses code after
an evidence reviewer changes to implementation. Foreign lifecycle, additional
platforms, precision and generations remain separate obligations; there is no
stale supported callback/buffer-completion blocker. Report implemented,
validated and integrated work separately, with hourly and material updates.
