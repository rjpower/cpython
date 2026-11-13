# Project status

Round54 is closed. Published branch `pyjit/round54-golden` and annotated tag
`pyjit-golden-20261112-frame-state` select
`61015a9452d9069646fb5fede9ff963896833b89`: a three-file public frame-to-generator
construction correction with explicit qualification limits. All nine performance
experiments/revisions/compositions are rejected; none enters the checkpoint.
This is a correctness selection with no demonstrated performance lift.

Round55 has completed **architecture research and a reviewed migration plan**. Start with the
[goal](GOAL.md), [plan](issues/call-model55/plan.md), and
[proposed call model](design/proposals/register-call-model55.md). The
[review resolution](issues/call-model55/review-resolution.md) closes all five
initial must-fix findings. Static scratch maps plus initialized locals are the
first candidate; a complete physical-bitmap rival remains required before
representation selection. Implementation is active; the
[execution ledger](issues/call-model55/implementation.md) records current lanes and gates.
No new candidate has earned admission. The proposal
does not replace as-built contracts.

Latest broadly measured candidate `ed08ab64c7e` composes typed pins, common admission,
nested transition roots, staged binding/provider/unpack/constructor ownership,
fixed-last heap teardown, classify-once retirement and the first shared-emitter
phase plans. NULL scratch homes and conservative arena scanning remain authority.
The GIL header remains 112 bytes; exact CALL/RETURN reserve 64/16 native stack
bytes with no phase-table lookup. New phase stores are real preparatory costs.

Both roles pass 675 core tests, nine phase tests and nine selected generator
contract tests. Groups overlap and are not a deduplicated total. The phase
fixtures cover nonempty provider/operand-retirement collection, mutable locals,
actual error cleanup, full 4095-home geometry and public admission routes.
The bounded continuing cross-lane post-review is GO, not a fresh-context audit.

Two-pair elapsed screens remain mixed: versus selected54, DeltaBlue is 5.5%
slower and silent logging 15.2% slower, with a wide logging range. Direct all29
versus upstream has geometric mean **1.165467**, six faster; DeltaBlue is
22.7% slower, generators 46.5% slower, coroutines 23.8% slower, and logging
17.5% faster. Matched PMU finds DeltaBlue still executes **36.8% more
instructions**. No candidate is promoted. The [active report](issues/call-model55/report.md)
retains campaign scopes, pair ranges, profiles and qualification limits.

Full foundation diagnostics at `0851fd51801` run 54,763 tests per role and
find a new allocation-injection crash beside recorded diagnostic limits.
Specialization conflated zero success/failure sentinels. `7c42db43224` requires
positive execution readiness before cache nomination; the unchanged OOM case
and 837 focused cases pass both roles afterward. This does not turn the failed
full diagnostics into a green suite.

Selected-home authority/copy ports `197fd05cb27` pass 84 precise and 588
lifecycle cases per role. Immutable-code generator allocation `8f6567840e7`
fixes a reproduced release/debug crash and passes its four provider-mutation
subcases plus 399 focused cases per role. Newer hot screens still lose
generators/coroutines/logging versus selected54; they have no direct all29
upstream campaign. Certified empty-created copy `9f28c0fa16d` passes 580
lifecycle and five generated-contract tests per role; its hot screen still
loses DeltaBlue 6.1%, generators 3.7% and silent logging 14.6% versus54.
The explicit created marker/getter/collector composition `44383f6baa7` passes
594 lifecycle cases per role. Vacant return recipients `91269fba2ce` pass 288
focused cases per role, but still lose the hot screen. Explicit CALL roles
are merged/generated; initial freezing exposes admission, generator-geometry
and FBARG decoder defects. Geometry, replay, post-method-promotion fallback,
fused-branch cleanup, consumed tails and iterator completion are repaired
through `e75b7088b12`. Independent implicit-child and exceptional-prefix
proofs now pass source perturbation checks. Both release and debug complete
117 modules/79 shared with no import failure. The first 11-module release
focus runs 88 tests: 8 modules pass and 3 expose fixture/return-contract gaps.
A separate debug run passes those 8 modules (41 cases). Corrected CALL fixtures
and the async-loop completion receipt then run 51 release cases: CALL roles and
EH-prefix modules pass; one malformed source-negative fixture and the existing
empty-only return contract remain failures. Their fixes/design are active.
These are failed integration runs, not green qualification.

The five bootstrap plus 26/27 broader source images pass. Successive coroutine
probes move past held-yield and async-exit gaps to a nested async comprehension;
GET_ANEXT's exceptional operand confinement remains unproved. The next packet
uses a general emitted error-footprint capability, not a per-test exception.
Explicit delegated-yield source/lifecycle/remote packets are composed at
`a2653e18206`. Both builds retain 117 modules/79 shared with no failed imports;
the five-module send/suspension/frame/contract/remote focus passes 198 tests
per role, 11 skips. The receiver is an explicit D operand, independent of the
outgoing B value and resume A recipient. Frame capture and local/remote getters
use that same contract. A separate earlier debug run passes 72 overlapping
generator/send/delegated tests. The new hot-set screen completes with stable
identities: versus selected54,
DeltaBlue 1.013629, generators 1.026487, coroutines 1.041078, Richards 0.998792,
comprehensions 1.022872 and silent logging 0.921104. A separate direct-upstream
screen is respectively 1.163485, 1.475615, 1.312553, 1.260601, 1.382747 and
0.696498. Both use two balanced pairs; they are not confirmation or all29.
Logging is faster; the broader gaps remain. Static authority and the complete
physical-bitmap comparison remain open. Bitmap `853bd363ce1` retains its older
five bootstrap plus 25/26 compile-only pass, without full build/tests/timing.
No golden selection follows from this work. Shared storage
with NULL named locals is the reviewed M4 reference, not a measured layout winner.
Round54's [report](issues/frame-state54/report.md),
[tables](issues/frame-state54/benchmark-tables.md) and
[qualification](issues/frame-state54/qualification.md) retain closure evidence.
[HANDOFF](HANDOFF.md) owns paths and recovery. Canonical documentation is this
checkout; its C files are not the measured runtime.

## Selected implementation and performance

Universal eight-byte PyValues span the runtime and rebuilt extensions. The
register interpreter is implemented; native method/tracing JIT tiers remain
unfinished. Exact calls already overlap arguments in one frame stack, reserve
and initialize once, and publish ordinary returns inline. Warm arena storage
uses an initialized frontier. Generator resume dispatches inside the evaluator;
hidden constants initialize at construction, not every resume.

Final round54 measurements use the actual selected61015 checkpoint, unchanged
workloads, GCC15.2, CPU15, hash0 and JIT disabled. Against golden51, all29
(two balanced pairs) has geometric mean **1.006381**, 11/29 lower; separate
six-pair hot confirmation has mean **0.996669**. Neither establishes a general
speedup. Logging silent differs substantially by scope: all29 is 1.138347,
while hot6 is 0.978879 with range 0.876076–1.005017. Keep both observations.

Against upstream, completed all29 mean is **1.156543**, 6/29 faster. Valid
six-pair confirmation is `final54-hot6-confirm2-upstream`:

| Case | Selected54/upstream elapsed | Pair range |
| --- | ---: | ---: |
| Generators |1.450207 |1.424808–1.472757 |
| Coroutines |1.254281 |1.240417–1.265182 |
| DeltaBlue |1.154663 |1.134484–1.184544 |
| Richards |1.280930 |1.229266–1.407654 |
| Comprehensions |1.390876 |1.347206–1.511459 |
| Logging silent |0.712216 |0.671358–0.779797 |

Richards, comprehensions and logging have wide ranges. The initial
`final54-hot6-upstream` failed its load guard and is retained but excluded;
its partial results are not confirmation. All accepted campaigns keep stable
clean source/native identities before and after. Do not pool campaigns or
multiply independent comparison ratios.

Final matched PMU instruction ratios versus upstream are 1.353135 for
DeltaBlue, 1.413895 for generators and 1.386260 for coroutines; cycles are
1.210496, 1.434569 and 1.249114. Both stat groups complete 18/18 valid
invocations; record completes 6/6, with stable identities and no lost records.
Flat profiles and instruction differences identify repeated work, not exclusive
removable cycle budgets. See the [tables](issues/frame-state54/benchmark-tables.md).

## Frame-state decision

Keep the 112-byte GIL header, direct feedback/environment caches, direct locals
mapping owner, NULL homes and initialized frontier. The measured F+N+L layout
has an 80-byte header but loses its screen; recovering omitted state and eager
mapping records cost work. A bitmap migration must cover every physical-home
reader/producer and reentrant phase, not just teardown.

R's two-pair all29 mean improves most cases, but its six-pair coroutine loss is
6.9%; F loses 4.1% there. Validated S2 removes four repeated geometry checks
and a reached intermediate NULL store, but hot coroutine timing is 2.4% worse.
These are rejected experiments, not selected alternate contracts. Full tables
and independent source/native reports retain all sample scopes and limits.

The public-constructor correction acquires the frame association owner, sets
first-input discard, constructs coroutine origin before moving owners, and
initializes subtype state before publication. Both-role native review finds
normalized hot CALL/SEND/YIELD/RETURN and ordinary construction unchanged.
Focused checks pass 808 tests per role (10 release skips, 9 debug); the strict
value API gate passes 306 units. The candidate's own failure probe checks 48
allocation points per subtype per role with stable code/mapping owners.
Full diagnostics and direct timing are complete. The checkpoint is selected
with the limits below; the full-run profiling finding remains open.

## Qualification limits and retention

Selected51 prior full diagnostics run 54,689 filtered tests per role: release
retains nine debugger-jump errors; debug those nine plus one LLTrace failure.
Only `test.test_gdb.test_pretty_print.PrettyPrintTests.*` is quarantined.
Current final diagnostics run 54,692 tests per role. Release has the same nine
debugger errors and an unstable GC-verifier failure; debug has those nine,
LLTrace and one profiling failure. Verbose debugger/LLTrace repeats have the
same failure identities as selected51. Six
independent targeted runs fail four times on each of selected51 and the new
candidate for the GC-verifier case. Profiling targeted, tracing-module and
complete-package reruns pass both builds, but its original full-run finding is
unexplained, not proven preexisting or fixed. These failures are recorded,
not quarantined or called a green full suite.
All current qualification is for the measured GIL configurations.

All nine rejected round54 branches are published. Verified retirement evidence
retains their patches, commit logs and 18 archives containing 3,042 native and
configuration files. Only their 27 task-owned source/build directories were
removed. Selected54, golden51, upstream and older controls remain.
Rounds52/53 also retain their published rejected branches and verified archives;
see their reports rather than historical source copies.

Round55 reopens bitmap/static-map/NULL-home tradeoffs within a holistic
call/frame architecture, including complete lifecycle and recurring costs. No
bitmap or native-JIT answer is predetermined. The broad Astra review and
independent design post-review are complete; Sol high-effort lanes now implement
the staged plan. The coordinator owns native evidence and measured selection.
