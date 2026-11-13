# Project status

Selected54 remains golden: `61015a9452d9069646fb5fede9ff963896833b89`,
branch `pyjit/round54-golden`, tag `pyjit-golden-20261112-frame-state`.
No round55 candidate has earned promotion. The [goal](GOAL.md),
[plan](issues/call-model55/plan.md), [execution ledger](issues/call-model55/implementation.md)
and [report](issues/call-model55/report.md) own the active effort. The reviewed
[target model](design/proposals/register-call-model55.md) is a proposal until
its lifecycle and qualification gates pass.

Current static native checkpoint `979f624cac1` builds all 117 modules,
79 shared, in release and debug. The preceding composition passes 167 focused
cases per role, including all 123 `test_ir` cases. Its semantic CFG correction
closes the newly introduced debugger-jump regression without weakening setter
permissions. The latest exceptional-effect extension passes 13 focused cases
per role. These are overlapping groups, not a deduplicated qualification total.

Source `25585dd6a46` separates required semantic recovery (IRSC1 in
`co_exceptiontable`) from optional source positions (IRSM2 in `co_linetable`).
It includes common admission, execution, observation, disassembly and remote
reader changes. Both native builds complete 117/79; focused integration finds
fixture ports and real admission gaps still to resolve. SEND is now covered,
but an actual predecessor trace finds the remaining async proof loss in two
read-only metadata/instruction projections. Their reviewed shared grammar
correction is current source `9be99bb3d3f`, pending native admission. No
admission exception or additional quarantine has been introduced.

Static scratch NULL homes and conservative arena scanning remain authority.
M4 has not switched. Normal phase programs cover 284/328 handlers, but only
12 exceptional contracts are complete; those counts are different obligations.
The outlined error processor and complete callback/readability coverage remain
open. Exact calls already overlap argument storage, and direct hot caches and
the 112-byte fixed GIL header are retained.

Bitmap native checkpoint `0ee3a61aba3` builds both 117/79 roles and passes
176 focused cases per role, eight skips. Its counted-bit preparation still pays
for NULL maintenance and loses every hot-screen pair. The isolated source lane
is converting the same bytes to one non-NULL presence plane, including roots,
heap capture, staged prefixes and sparse teardown. The ownership composition `20576b58f4b` builds both roles and passes17
targeted presence/provider cases per role. A raw CALL cleanup writer still
needs conversion before qualification. Execution retains initialized NULL values until
actual readers have an explicit policy; a zero presence bit permits no load.
See the [authority assessment](issues/call-model55/bitmap-authority-assessment.md).

The latest static two-pair screen versus54 gives DeltaBlue **1.130007**,
generators **1.008836**, coroutines **1.017062**, Richards **1.032109**,
comprehensions **1.023607** and silent logging **1.278120**. Lower is better.
No broad gain is established. Logging varies substantially across screens;
these are separate campaigns, not isolated source-effect measurements.
A native inventory finds identical logging hot-frame geometry and warm
attribute/call families in candidate and54; hidden constant-local initialization
does not explain this gap. Pair ranges and evidence are in the report.

Latest broad direct-upstream evidence remains `ed08ab64c7e`: all29 geometric
mean **1.165467**, six faster; matched DeltaBlue instruction ratio **1.368099**.
No newer hot screen replaces that breadth or PMU result. The latest full55
foundation diagnostics run 54,763 cases per role and find an allocation-injection
crash. Its positive-readiness fix passes the original OOM test and 837 focused
cases per role, but there has been no green full55 run afterward. Existing
qualification limits below remain explicit. Only the GDB pretty-print class
is quarantined. Current qualification is GIL-only.

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
