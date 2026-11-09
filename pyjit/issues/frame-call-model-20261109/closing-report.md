# Frame/call round: report and analysis

**Logging parity is achieved.** Frozen candidate `4d666e1b482` uses **0.907× and
0.898× upstream elapsed time** in two independent six-round confirmations; every
pair is below 0.924×. Across all 29 kernels it uses **0.934× golden** and
**1.175× upstream** elapsed time. The earlier material golden regressions no
longer repeat, while broad upstream parity remains open.

The round is closed at full SHA `4d666e1b482cb4983c7683f91c5922af14d0ab40`.
Production changes are `24f4d270ced`; the last commit strengthens debugger tests.
The source is clean and no unfinished draft is included. Golden remains an
immutable control; this engineering candidate is unpromoted, with explicit
qualification limits below.

Navigation: [goal](../../GOAL.md) → [plan](plan.md) → [status](../../STATUS.md) →
[IR](../../design/register-ir.md), [interpreter](../../design/interpreter.md),
[frames/calls](../../design/call-and-frame-lifetimes.md), [values](../../design/pyvalue.md).
The [complete benchmark tables](closing-benchmark-tables.md) and
[next five priorities](next-steps.md) carry detailed results and follow-up.

## What changed

One canonical eight-byte value and owning-frame ABI remain. The round places
source/event boundaries in metadata, reuses initialized frame storage, transfers
arguments through overlapping windows, binds common positional Python calls
directly, and materializes eligible empty kwargs only when needed. Calls name
result recipients and real return continuations explicitly. Iteration/delegation
completion and commit-before-close owner transitions share a provider contract.
Bounded producer proofs allow canonical stores without repeated conversion.
Observation and frame-jump proofs stay cold, with explicit reentry authority.

All measurements disable the JIT. Native method/tracing tiers remain unfinished.
The unified value representation is integrated across frames, containers,
attributes and rebuilt extensions. Its optimization coverage remains incomplete:
object-form feedback caches and legitimate owning bridges still cause generic
execution in some important paths. That is a performance-porting task, not a
second representation or a reason to undo the value model.

## Throughput and coverage

GCC 15.2, CPU 15, hash seed 0, JIT off; native revisions, module rosters and exact
ELF/input/extension identities checked before and after. Kernel comparisons use
two balanced pairs per control, seven samples per process. The aggregate weights
each of the 29 kernels once. Logging gets two separate six-round confirmations.

| Result | Candidate/control elapsed ratio |
| --- | ---: |
| 29 kernels / golden | 0.934192 |
| 29 kernels / upstream | 1.174985 |
| logging / upstream, confirmation 1 | 0.906735 |
| logging / upstream, confirmation 2 | 0.897500 |
| DeltaBlue / upstream | 1.619651 |
| generators / upstream | 1.415179 |
| coroutines / upstream | 1.249122 |
| nbody / upstream | 0.841104 |
| float / upstream | 0.807037 |
| spectral_norm / upstream | 0.663817 |

Against golden, 26 kernels have lower mean elapsed time. The three higher means
are JSON loading (+0.84%), deepcopy_memo (+0.17%) and fannkuch (+0.06%): small
screen differences, not established material regressions. The original five
large losses now give unpack 0.818, float 0.992, pickle 0.997,
deepcopy_memo 1.002 and nbody 0.971. These are combined-image comparisons; they do
not attribute a gain to a cold helper or a specific commit.

Supplemental coverage is complete: **65 micro instruction workloads × three
builds**, each with four low/high pairs and registered skeleton subtraction;
**six timed allocation/collector workloads × three builds**, each with a warm-up
and five timed calls. Startup/import RSS-only rows are excluded as requested.
All six sweeps finish with stable identities; raw perf journals match their
reports. Micro instruction ratios are diagnostics, not speedups. Memory times
come from serial, unpaired sweeps and are not a balanced performance claim.

Allocation rows show candidate/upstream times about 0.902 for tuples, 0.970 for
instances and 0.849 for dictionaries. Collector rows show 1.27–1.34/upstream and
about 1.04/golden. Repeat those in balanced order before declaring a new collector
regression. Nominal allocated-GB units represent fixed object counts, not measured
allocator traffic. No collector redesign is proposed.

## Why the remaining gaps exist

Current PMU capture uses the existing FIFO-controlled hot interval, three
counter rounds per role/case and matching retained executables. All 18 stat
captures are valid at 100% event running; all six five-second profiles have zero
lost samples and matching build IDs.

| Kernel | Cycles / upstream | Instructions / upstream | CPI / upstream |
| --- | ---: | ---: | ---: |
| DeltaBlue | 1.654886 | 1.869213 | 0.885338 |
| generators | 1.442485 | 1.415902 | 1.018775 |
| nbody | 0.849243 | 1.128690 | 0.752415 |

DeltaBlue and generators execute substantially more instructions. DeltaBlue's
CPI is better, and generators' CPI is close; extra executed work is the stronger
lead in these cases. Nbody demonstrates why instruction count or “per bytecode”
cost alone is insufficient: it executes more instructions but finishes faster.
The IR and upstream bytecodes also represent different amounts of semantic work.

Profiles place DeltaBlue work in exact Python calls and attribute handlers;
generators concentrate in SEND_GEN and YIELD_VALUE. Nbody concentrates in float
arithmetic and list subscripting. Sample shares locate code; they are not
causal fractions of the slowdown or promised speedup budgets.

Source inspection gives a concrete optimization hole. In class_attr_load,
`o.k` reads an unshadowed inline scalar stored on its class. Descriptor
classification returns INLINE_VALUE, but instance specialization only admits
that case when an instance shadows it; otherwise it declines. Adjacent cached
non-descriptor paths assume an object-form descriptor. The witness uses 4.54×
upstream raw instructions, or 8.83× after subtracting each runtime's skeleton.
That is not an 8.83× elapsed slowdown, and its share of DeltaBlue is not isolated.

Other actionable evidence is generator delivery, fixed keyword binding and
object-form boundaries. The resume microcase has 1,000 yields per creation, so
creation-only tuning is insufficient. A fixed three-keyword call uses 1.985×
upstream raw instructions. The cell getter boxes inline 3000 through an object
API before later canonical publication; its witness uses 2.028× raw instructions.
The str_join microcase includes 1,000 append calls per amortized join, so its
name does not identify Unicode joining as the cause. Existing dictionary
first-probe handling must be measured before adding another cache.

## Functional qualification

| Check | Result |
| --- | --- |
| Release/debug/stats builds | 117 checked modules each |
| Final release full diagnostic | 54,645 tests; GDB 16 failures and nine frame-jump errors |
| Final debug full diagnostic | 54,645 tests; GDB 21 failures, one trace-format failure and nine frame-jump errors |
| Stats smoke | 150 tests pass |
| Value API check | 306 translation units; zero failing findings and parse errors |

The full suites are **not green**. Their remaining failures are explicit:
GDB tooling, a debug trace check expecting an instruction removed by lowering,
and nine unsupported frame-jump capabilities. Other executed modules pass, including
Pdb, monitoring and collector checks. Standard platform/resource skips remain
(74 module files skipped, 11 resource-denied). The historical GC negative-control failure
did not recur; these passes do not establish its original cause. API notes
(91 home-copy, 151 slot-read, one unchecked) remain diagnostics, not failures.

GDB had been absent, so its optional tests previously skipped. Installing it
exposed a tooling port: printer registration expects the old value type, and
tests expect the old id() C arguments. Exact display is not a language or
throughput gate. Matching CPython destruction timing or its full accepted jump
matrix is not the goal either. Four unchanged synchronous jump witnesses pass;
eleven deliberately refused protocol crossings have unchanged-state and normal
cleanup controls, and both Pdb backends have real entry/continuation tests.
No failed ordinary jump case has been silently counted as a pass.

## Review and next five fixes

Two independent V8-style design/implementation assessments, then a measured
priority review, support keeping owning homes, explicit continuations and
transactional retirement. They criticize duplicated ownership-effect knowledge
and inherited object-only optimization boundaries. Reviewers participated in
earlier workstreams; these are independent closing assessments, not blind
reviews by external engineers. The session's thread limit prevented adding
another fresh reviewer. No reviewer claims V8 affiliation.

The evidence-weighted five priorities are:

1. Finish guarded canonical-value lookup, starting with inline class scalars.
2. Simplify the complete generator resume/yield/delegation cycle.
3. Extend reusable binding plans to fixed keyword shapes.
4. Remove avoidable object bridges at value-native getset/native boundaries.
5. Derive emitter ownership effects once and retire owners at semantic outcomes.

The [next plan](next-steps.md) gives source evidence, risks and falsifiable gates.
It demotes unmeasured direct-heap creation, broad layout tuning and debugger
capability expansion. No further runtime work is included in this round.

## Cleanup and evidence

Coroutine ingress, retained-owner bundles and the partial GDB port are excluded.
The coroutine packet passed release but debug asserted on a synthetic handler
to RESUME; its exact patch was reversed. Complete drafts and identities remain
in `E11/deferred-after39-source/` and the separate authoritative coroutine patch.
The task-created isolated worktree and branch were removed after verification.
Unrelated user work, historical worktrees and sealed evidence remain unchanged.

The current PyValue page describes actual encoding/typed ownership; its complete
old page is archived. Loom has one current goal, plan, status, report and linked
core designs/review/data. Six old active artifacts are preserved with their
complete one-revision histories in the repository archive before active cleanup.

E11 is `build/pyjit-link-wave1/frame-call-model-20261109/` under
python-boil-the-ocean. Final raw campaigns are `closing39-kernels-{golden,upstream}`,
`closing39-logging-{1,2}`, `closing39-supplemental-v2`, and
`closing39-gap-perf-{stat,record}`. Full logs are `closing39-full-{release,debug}`;
stats/API logs start `closing39-`. The initial supplemental preflight failure
measured nothing and is retained separately. See
[complete tables](closing-benchmark-tables.md),
[independent raw/PMU/profile audit](final-performance-audit.md),
[measured review](closing39-measured-priority-review.md), and
[source-backed addendum](final-micro65-priority-addendum39.md).
