# Project status

Activation 53 is implementing two reviewed experiments from golden51: eligible
final-home generative construction and a shared canonical completion terminal.
Source/native audits and one broad Astra review are complete; lane proposals
and independent peer review precede implementation. No candidate has a native
build, qualification result or measured win yet. The [active plan](issues/activation53/plan.md),
[design](design/activation-transfers.md) and [review resolution](issues/activation53/review-resolution.md)
record the contracts. The selected runtime below is unchanged.

Round 52 is closed: **both generator-delegation candidates are rejected**.
Golden51 remains selected and unchanged. One broad Astra review, incorporated
feedback, implementation, focused qualification, profiling and independent cold
review are complete. The [experiment report](issues/generator-delegation52/report.md)
and [full tables](issues/generator-delegation52/benchmark-tables.md) record the
results. The first candidate loses 4.4%/4.7%/2.7% versus golden51 on generators,
coroutines and DeltaBlue in six pairs; its entry revision loses 1.0%/11.3%/11.9%.
The revision's all29 geometric mean is 1.020473 versus golden51 and 1.172901
versus upstream. Neither is promoted. Full suites were not run after the
performance gate failed. Experimental code and evidence are retained; task-only
builds/worktrees are retired. The [round 52 next plan](issues/generator-delegation52/next-plan.md)
called for weighted transition evidence before another shared resume/transfer
proposal.

Round 51 selects shared exact-call reservation/initialization and canonical
integer ADD/SUB publication at runtime `0e5e61f4c6b`. All five Astra options
were implemented and independently reviewed in separate subtrees. Cached layout
displacement, canonical iteration and normal-generator cleanup were rejected
on measured breadth or composition. Profiling, full diagnostics and independent
review are complete. The [golden runtime](https://github.com/rjpower/cpython/tree/pyjit-golden-20261111-runtime)
is published on `pyjit/round51-golden` with the documented limits below.

Start with the [goal](GOAL.md), [active plan](issues/activation53/plan.md),
[report](issues/interpreter-round51/report.md),
[benchmark tables](issues/interpreter-round51/benchmark-tables.md) and
[architecture](design/runtime-architecture.md). [HANDOFF](HANDOFF.md) owns exact
source/build paths and execution state. Canonical documentation is in this
checkout; its C files are not the measured runtime.

## Performance

Unchanged kernels, GCC 15.2, CPU 15, hash 0, JIT disabled. Ratios are elapsed
candidate/control, lower is faster. Individual campaigns are direct comparisons;
do not multiply their ratios.

| Case | / prior checkpoint50, six pairs | / upstream, six pairs |
| --- | ---: | ---: |
| DeltaBlue | 1.036881 | 1.159832 |
| Richards | 0.966903 | — |
| Comprehensions | 1.000125 | — |
| Generators | 1.012945 | 1.445513 |
| Coroutines | 0.893073 | 1.267666 |
| Spectral norm | 0.897685 | — |
| Logging silent | 0.891657 | 0.775668 |

All 29 kernels, two balanced pairs, have geometric means of **0.985700 versus
prior50**, **0.982829 versus golden39** and **1.156535 versus upstream**.
This is incremental progress, not broad parity. Logging is faster than upstream
in every confirmation pair; its direct prior50 pairs have a wide
0.827254–1.038904 range, including one loss.

Keep the regressions visible. DeltaBlue is 3.7% slower than prior50 in the
longer confirmation and 4.5% slower in the broad screen; generators are 1.3%
slower. The screen finds deepcopy_memo 2.7% slower than prior50; six pairs confirm
a 2.0% cost, with five slower pairs. Spectral norm remains 24.5% slower than
golden39 despite its improvement this round. The tables retain every kernel and control.

Post-warmup PMU diagnostics show native-instruction ratios of 1.352487 for
DeltaBlue, 1.414550 for generators and 1.388163 for coroutines versus upstream.
Their user-cycle ratios are 1.173937, 1.443516 and 1.249676. Logging executes
0.953890 times the instructions; its cycle pairs vary substantially. These are
whole-workload counts, not removable fractions or proof of a hardware cause.

## Selected design

Exact calls choose final owning homes before one storage reservation and one
header initialization. Initialized thread callers retain argument overlap;
cold helpers fill an uncertain storage suffix or move heap-caller arguments.
Code and function remain separate owners; continuation, observation, binding
and retirement retain the shared ABI.

Integer ADD/SUB publication preserves canonical output across the proven inline
and boxed-overflow join. A bounded whole-producer proof checks guards, scaled
overflow, allocation failure and unchanged input retirement. It does not
whitelist arbitrary boxing or generalize to other arithmetic.

Universal PyValues are integrated throughout runtime and rebuilt extensions.
Native method/tracing JIT tiers remain unfinished. Source coordinates do not
control ordinary dispatch or call admission; compiler line restrictions still
need an observer reconstruction contract. Current contracts:
[IR](design/register-ir.md), [interpreter](design/interpreter.md),
[frames/calls](design/call-and-frame-lifetimes.md), [values](design/pyvalue.md).

## Qualification and next work

Release/debug builds each check 117 modules. Focused qualification passes 821
tests with nine skips plus five generator tests in each build. The strict value
API gate passes 306 units with zero findings or parse errors. Independent merged
source/native, timing/PMU and full-diagnostic review approve this increment.
Full diagnostics run 54,689 filtered tests in each build. Release retains nine
debugger-jump errors; debug retains those nine plus one LLTrace failure. No
additional failing module group appears. The prior quiet logs establish the
same groups/counts, not individual-case equivalence. These are failed diagnostics
with documented limits, not green full suites.

The runner quarantines only the GDB pretty-printer class; other GDB groups,
debugger-jump checks and LLTrace remain enabled. Quarantine is a coverage gap,
not a fix. Remaining established debugger/trace failures must stay visible.
Golden39, upstream and prior50 remain immutable controls.

Activation 53 source and native audits are complete, with phase evidence and a
[thesis](issues/activation53/thesis.md) submitted for one broad Astra review.
No source candidate or measured build win exists yet. The yield-only round52
experiment failed its performance gates; its mechanism worked without delivering
throughput. A constant-operand redesign needs a weighted constant-home cost
census first. Feedback already uses direct scaled offsets; removing a nonexistent pointer table is not an option.
See the [source assessment](issues/interpreter-round51/next-plan-assessment.md).
