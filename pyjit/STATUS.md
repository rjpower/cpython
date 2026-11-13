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

Latest measured candidate `de95475accb` composes typed pins, common admission,
partial transfer/retirement proof, nested roots, staged unpack/binding and
provider handoff, plus classify-once callback-free retirement. It passes
1,487 core tests, 28 C API cases and eight precise provider/reader cases per
GIL role. The groups overlap; they are not a deduplicated total. Cold
source/native review is GO for these bounded ports. Scratch NULLs and
conservative arena scanning remain authoritative.

Its two-pair selected54 screen improves DeltaBlue 6.0% and Richards 3.0%,
but loses coroutines 1.4% and silent logging 8.4% (wide logging range).
Direct all29 versus upstream is 1.162061, with 6/29 faster; DeltaBlue is
12.2% slower and logging 20.6% faster. Matched PMU shows DeltaBlue still
executes 34.1% more instructions. Classify-once retirement substantially
reduces the preparatory cleanup regression, but no general win is confirmed.
No candidate is promoted. The [active report](issues/call-model55/report.md)
retains exact sample scopes, ranges, profiles and qualification limits.
Constructor shim ownership is next in integration; heap deallocation and the
shared-emitter phase work continue. Complete static authority and a physical
bitmap rival remain unqualified. Shared storage with NULL named locals is
the reviewed M4 reference, not a measured layout winner.
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
