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
slower and silent logging 15.2% slower, with a wide logging range. Direct all 29
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

The newer CALL composition includes explicit input roles, independent child
transport, exceptional-prefix and async-exit receipts, and a delegated YIELD
whose receiver D is independent of outgoing B and resume recipient A.
Local/remote getters, throw, close and capture use that operand contract.
`a2653e18206` passes 198 targeted tests per role, 11 skips. Its two-pair hot
screen versus selected54 is DeltaBlue 1.013629, generators 1.026487, coroutines
1.041078, Richards 0.998792, comprehensions 1.022872 and logging 0.921104.
A separate direct-upstream screen is respectively 1.163485, 1.475615,
1.312553, 1.260601, 1.382747 and 0.696498. These are screens, not all 29 or
confirmation. Earlier copy/created/recipient ports and their limits remain
in the [report](issues/call-model55/report.md).

Checked home writers at `50f1124fd7a` pass both full 117/79 module builds and
37 writer/CALL/transfer cases per role. Its separate two-pair selected54
screen is respectively 1.033000, 1.013630, 0.997996, 0.987920, 1.040724 and
1.006214; no broad gain is established. It still pays for NULL maintenance
and arena scans. Five bootstrap and 26/27 broader compile probes pass; the
remaining async source needs complete exceptional-producer and redirect/error
contracts beyond GET_ANEXT's now-certified operand footprint.

Current static source/native `ff14b98be4b` adds per-edge NULL results,
general RETURN and independently certified EMPTY, plus direct tuple/list
steal-success receipts. Both builds complete 117/79 with no failed imports.
Six focused modules pass 67 cases per role after four fixture corrections; Expanded `test_ir` has stale CALL/EMPTY fixtures and a
source-debugger jump regression: selected54 passes the identical fixture in
both roles, while candidate fails. This remains partial qualification.
Forged EMPTY negatives and actual remaining-owner assertions stay intact.
Its two-pair hot screen versus54 is1.039263,1.043308,1.052147,0.984842,
1.007455,1.004977; direct-upstream1.230703,1.504706,1.326683,1.230958,
1.356191,0.728430. No broad gain. Source admission compiles 682/684 images,
including all 189 stdlib entries. The remaining two images share async EH gaps.
Known-home source 3b4547380d0 builds both 117/79 roles and passes 36 focused
cases per role; its selected54 hot screen loses broadly (logging 1.199787). It carries existing
value/physical vacancy facts through common commits instead of rediscovering
them in B.

Bitmap `bf7d6bf0d20` also builds both 117/79 roles and passes 150 focused
tests per role, eight skips, including physical invariants and capture.
Its nine copied instruction-body guards have been replaced by shared checked
home operations. The NULL-control rival's two-pair selected54 screen is
respectively 1.335421, 1.222776, 1.333615, 1.210015, 1.279180 and 1.603326.
All pairs are slower. This prices current bookkeeping plus old NULL work;
it does not evaluate a complete stale-scratch rival. The
[writer ledger](issues/call-model55/writer-closure-ledger.md) and
[authority assessment](issues/call-model55/bitmap-authority-assessment.md)
own its remaining gates. Neither representation has switched authority or
earned golden selection. Shared storage with initialized NULL named locals
remains the reviewed M4 reference, not a measured layout winner.
Current B `0ee3a61aba3` composes matching known-home commits, builds both 117/79
and passes 176 focused cases per role, eight skips. Its separate selected54
screen is1.327228,1.228186,1.350701,1.217027,1.250380,1.587998; every pair
loses. Root selects single non-NULL occupancy as the next full-rival direction,
with exact interfaces pending peer freeze; no authority switch is claimed.
The semantic CFG correction and actual exceptional-route producer/consumer
compose at static e90c6a4b7f9, with native qualification running.

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
workloads, GCC15.2, CPU15, hash0 and JIT disabled. Against golden51, all 29
(two balanced pairs) has geometric mean **1.006381**, 11/29 lower; separate
six-pair hot confirmation has mean **0.996669**. Neither establishes a general
speedup. Logging silent differs substantially by scope: all 29 is 1.138347,
while hot6 is 0.978879 with range 0.876076–1.005017. Keep both observations.

Against upstream, completed all 29 mean is **1.156543**, 6/29 faster. Valid
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

R's two-pair all 29 mean improves most cases, but its six-pair coroutine loss is
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
