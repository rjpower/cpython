# Pre-JIT roadmap: roughly twice upstream interpreter throughput

Coordinator plan, 2026-10-29. The user's new target supersedes the earlier
parity target: aim for roughly 2× upstream CPython's interpreter performance
before starting JIT implementation. Use structural, established techniques
and a small set of coherent execution contracts. This is a target, not a
prediction that the changes below will add up to 2×.

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
An owned exact upstream `321925` build now succeeds and clears independent
product-closure review; runtime qualification is the next measurement input.

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

**1. Finish and land the current precise-root repair.** ROOTS 2b's current
source is `ce2ac8991ee`. Its substantive repairs cover PyMem-origin attribution,
list-to-tuple temporary lifetimes, multiprocessing cleanup and Park retention.
The two latest raw-mutex annotations clear source review and actual linked
audits on release/O1-debug/ASan. Both completed 120-transaction campaigns remain
historical evidence on `1bfe`, with zero fallback. All pre-change products and
journals are preserved.

Finish the bounded comparison of emitted root classifications, unwind rules
and root locations, including the four actual semaphore helpers. Default/debug
function membership and ranges match; ASan intentionally moves only two raw
wrappers into the precise section. Literal code/data changes remain recorded;
selected relocation/string and root/unwind equivalence do not imply whole-image
byte equality. Root decides historical carry-forward from the completed bridge.

Then execute the already proposed fresh affected campaign once per relevant
configuration: release off, O1-debug off and actual debug kill. It includes
strict stale-root probes, complete compiled stack walks and Park caller-slot
controls, the three thread regressions, both eval-root cases and the complete
changed generated-cases module. It is 24 transactions total, not another 20
iterations. The cheap internal stale100 probe remains this landing's existing
hazard check. No new test exclusion or root-policy fallback is introduced.

The landing qualification still requires current-source full release, full
debug normal and full debug kill; the 11-module stress set on both builds at
interval 1,000; the once-only filtered release full at interval 100,000; and fresh ASan kill with stress off
coverage of the 14 hazard modules plus sort/functools. The ASan union may run
as one 16-module campaign with the same exact existing allocator exclusion.
That consolidation changes no covered module or allowed failure. Finish cext,
unchanged regeneration, traversal audit and declared optional-module/header
checks. Preserve unsupported modules and existing exclusions explicitly.
No benchmark or unrelated GC redesign becomes an additional 2b landing gate.

An independent landing review and primary integration smoke follow. Full
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

H3 integration and performance analysis can proceed after 2b while independent
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

As soon as the upstream diagnostic/correctness environment and a stable fork
candidate are ready, take a quiet release measurement window. Do not wait for
every proposed optimization before obtaining the first honest wall-time
baseline. Rank recurring costs using elapsed profiles plus boxes/allocated
bytes, GC root/mark/sweep time, survival, dispatch mix, spills and code size.
This determines the next implementation priority; no imaginary per-technique
speedup budget is compounded into a 2× promise.

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

Checkpoint I targets roughly 2× upstream on the declared application aggregate
with defensible individual results and memory/pause tradeoffs. If the proposed
techniques fall short, publish the shortfall and choose the next measured,
principled intervention with the user. Do not redefine the workload, count JIT
speedups toward the target or start JIT implementation to conceal a deficit.

**Coordination.** The coordinator owns this plan, measurement choices, shared
interfaces and integration. Subagents own bounded implementation and independent
review. Current assignments remain ROOTS comparison/affected qualification,
independent bridge/baseline review, and baseline diagnostic/correctness helpers.
Next native ownership is H3 union/statistics, then bounded H1, with 6e and GC 3
parallel where interfaces permit. Full root maps and call-window changes follow
their actual dependencies. Report evidence and blockers hourly, with clear
distinctions between implemented, validated and landed work.
