# Interpreter round 51 report

The selected **3+4 checkpoint, `0e5e61f4c6b`**, takes **1.4% less time than
prior50** across 29 unchanged kernels on the geometric mean. It remains **15.7%
slower than upstream**. The coordinator accepts this incremental performance
tradeoff; full diagnostics and independent review are complete with established limits.
The [golden runtime](https://github.com/rjpower/cpython/tree/pyjit-golden-20261111-runtime)
is published on branch `pyjit/round51-golden`.

Six-pair direct comparisons with prior50 show coroutines taking 10.7% less time,
spectral norm 10.2% less, silent logging 10.8% less and Richards 3.3% less.
Comprehensions are flat. DeltaBlue takes 3.7% more time and generators 1.3%
more. A separate six-pair check confirms deepcopy_memo takes 2.0% more time,
with five slower pairs. Logging has one slower prior50 pair and a wide
0.827254–1.038904 range; its upstream confirmation is faster in every pair.

The all-29 mean is 0.985700 versus prior50, 0.982829 versus golden39 and
1.156535 versus upstream. Spectral norm remains 24.5% slower than golden39.
These direct campaigns are not interchangeable, and their ratios must not be
multiplied. The [tables](benchmark-tables.md) retain every kernel, control,
confirmation range and component result. Broad parity remains open.

## Selected implementation

Exact calls now select their final owning homes, reserve storage once and
initialize one header. The initialized thread path keeps argument overlap;
cold helpers fill uncertain storage or move heap-caller arguments. The same
frame ABI still owns binding, continuation, observation, unwind and retirement.
Native exact CALL shrinks from 1,410 to 973 bytes with one initialization region;
that static change does not establish the cause or size of application gains.

Integer ADD and SUB preserve canonical results across the inline/boxed-overflow
join into the existing final store. A bounded whole-producer proof validates
input guards, scaled overflow, boxing preconditions, NULL error propagation and
unchanged input retirement. It neither whitelists arbitrary boxing nor widens
other arithmetic. Native normalization arms disappear; required owner retirement
and allocation errors remain. The [interpreter](../../design/interpreter.md),
[IR](../../design/register-ir.md) and
[frame design](../../design/call-and-frame-lifetimes.md) define these contracts.

## Five reviewed experiments

Astra supplied one [grouped architectural review](astra-options.md). Sol agents
followed proposal, independent peer review, implementation, focused testing and
independent post-review in separate subtrees. Root integrated and measured them.

| Option | Mechanism | Component source | Decision |
| --- | --- | --- | --- |
| 1 | Cache guarded inline-values displacement | `22f4ce1508e` | Discard: screen did not meet breadth criterion |
| 2 | Publish canonical list/tuple iteration results directly | `0d0dbb5fbc9` | Discard: adding it to 3+4 lost on multiple kernels |
| 3 | Share exact-call reservation and initialization | `d1353a16f3d` | Retain |
| 4 | Preserve canonical integer add/subtract results | `4bb16138d64` | Retain |
| 5 | Reuse empty-scratch proof at normal generator return | `e9886d5b1c3` | Discard: confirmed DeltaBlue regression |

All component release builds, focused checks and independent source reviews
completed. Component reviews are not merged qualification. Option2's incremental
trial improved comprehensions by 2.7% but regressed the other measured cases;
it is absent from the selected source. Retired experiment worktrees/builds were
removed after retaining commits, patches, configuration, native binaries and raw
evidence. Existing controls and the selected runtime remain intact.

## Remaining performance gap

Post-warmup PMU diagnostics show candidate/upstream instructions per workload
iteration of 1.352487 for DeltaBlue, 1.414550 for generators and 1.388163 for
coroutines. Their user-cycle ratios are 1.173937, 1.443516 and 1.249676. Silent
logging uses 0.953890 times the instructions and 0.730284 times the cycles,
with a wide cycle ratio range of 0.657228–0.775115. All 24 counter intervals
ran at 100%, with stable identities. These counts establish extra executed work,
not its removable fraction or a hardware explanation of component changes.

Four sampled profiles have stable identities and zero lost samples. About
2,500 samples per profile rank generator SEND_GEN at 27.41% and YIELD_VALUE at
13.92%, versus RETURN_GENERATOR at 2.39%, completed-generator return at 1.44%
and frame-local clearing at 1.36%. DeltaBlue ranks exact CALL at 11.17%,
shape-local load at 9.26% and method-with-values load at 7.09%. Upstream's
collapsed evaluator prevents direct per-handler percentage comparisons.

The next investigation targets the resume/yield transition contract before
more cleanup changes. A proved empty redispatch may be avoidable, but provider,
owner, frame, exception, recursion and observer state still require a complete
contract. A constant-operand redesign needs a weighted hidden-home cost census;
feedback already uses directly addressed scaled offsets. The
[next-plan assessment](next-plan-assessment.md) records source constraints and
negative evidence; the [plan](plan.md) sets the next gates.

## Verification and limits

The selected release/debug builds each check 117 modules. Each passes 821
focused tests with nine skips plus five targeted generator tests. The strict
value API gate passes 306 units with zero findings or parse errors. Merged
regeneration is clean. Independent review of source, native code, focused and
full diagnostics, raw timing and PMU evidence approves the increment with its
stated tradeoffs.

Full diagnostics run 54,689 filtered tests in each build. Release retains nine
`test_sys_settrace` debugger-jump errors; debug retains those nine plus one
`test_lltrace` formatting failure. No additional failing module group appears.
The same groups/counts occur in prior50; its quiet logs do not establish
case-for-case equivalence. The four enabled GDB groups pass in both builds.
These are failed diagnostics, not an unfiltered full pass. The qualification
runner excludes exactly
`test.test_gdb.test_pretty_print.PrettyPrintTests.*`, including passing methods
in that class. This is a coverage gap, not a repair. Debugger-jump checks and
LLTrace remain enabled. See the [qualification policy](qualification-policy.md)
and [independent integration review](integration-review.md).

Measurements use unchanged kernels, GCC15.2, CPU15, hash0 and JIT disabled;
quiet timing/profiling finished before full tests. Two balanced pairs screen all
29 kernels; six confirm selected targets and suspicious regressions. A rebuilt
bf810 control served component experiments; final comparisons use the actual
prior selected image. Native size or source inspection alone is not a speedup.
[Status](../../STATUS.md) owns admission, and [HANDOFF](../../HANDOFF.md) gives
runtime/evidence paths and exact source identities. Universal PyValues are
integrated; native method/tracing JIT tiers remain unfinished.
