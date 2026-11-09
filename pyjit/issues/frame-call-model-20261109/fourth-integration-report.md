# Fourth engineering screen and qualification repairs

Clean measured runtime `192cfbe8aa0d0b593dcc40f26e1c8f70e84b02ae` is **not yet
qualified or promoted**. Golden `0765e9be47f` remains immutable and selected.
Logging is faster than upstream in the latest two-pair screen. Broad correctness
and the final parity campaigns remain required; work continues.

## Current result

GCC 15.2, JIT explicitly disabled, CPU 15, fixed hash seed, unchanged kernels,
seven timed durations after warmup per process and balanced process order.
Source, executable, module and driver identities remain stable across each
campaign. Ratios are candidate/control elapsed time; lower is better.

| Workload | Control | Campaign | Geometric mean | Paired range |
| --- | --- | --- | --- | --- |
| logging_silent | upstream | Two pairs | **0.817460** | 0.798429–0.836945 |
| logging_silent | golden | Four-pair repeat | **0.592805** | 0.590244–0.595484 |
| nbody | golden | Four-pair repeat | **1.000054** | 0.991650–1.021431 |
| pickle | golden | Four-pair repeat | **1.017928** | 0.997762–1.039830 |

Nbody's earlier 5.6% loss on the 87 image does not repeat on this image. Pickle's
initial two-pair 4.5% loss shrinks and varies across the repeat; it remains in the
broad screen and follow-up list. Do not combine comparisons from separate
campaigns or attribute a speedup to cold correctness repairs. These screens do
not replace two independent balanced six-round logging/upstream campaigns with
mean at most 1.0 and every pair at most 1.02.

Evidence is under `build/pyjit-link-wave1/frame-call-model-20261109/` in the
python-boil-the-ocean checkout: `fourth-engineering-upstream/`,
`fourth-engineering-golden/` and `fourth-engineering-golden-repeat/`.
The first repeat command used space-separated cases where the driver requires
a comma-separated list; argparse rejected it before a native process ran.

## What the model now does

Known Python calls bind into an owning frame stack with overlapping argument
windows. Scalar guarded binding plans replace generic preparation on supported
hits. Empty kwargs remain virtual until read or publication. Results name their
recipient or discard directly. A chunk initialization frontier avoids clearing
the same reusable frame tail on every call.

Executable words describe semantic work. Sparse source-edge metadata describes
observation and callback-error destinations. Loop/delegation completion describes
its result, owner retirement and continuation together; generator outgoing and
incoming values have distinct roles. Source errors belong to evaluator
activations, and LINE handoffs track actual LINE configuration changes.

Debugger entry is an explicit cold observer transaction. A request-local analysis
derives physical input requirements, handled-exception contexts and owner origins
from actual final IR and canonical EH. One plan detaches surplus owners, commits
the continuation, then retires detached owners without later frame writes.
It does not add ordinary-dispatch analysis or reproduce upstream's observed
callback-exception clobbering bug.

The [frame ABI](../../design/call-and-frame-lifetimes.md),
[register IR](../../design/register-ir.md), [interpreter](../../design/interpreter.md)
and [debugger transfers](../../design/debugger-frame-transfers.md) own the design.

## Correctness and qualification

The [repair log](qualification-repairs.md) records source-specific checks:

- `37a67b9d155` repairs fused source coverage, protected real-LINE identity,
  compact-format consumers and frozen embedding. Eight affected modules pass
  1,027 tests per profile; 2,152 valid library files compile.
- `9f677103e00` integrates explicit entry and its consumers, bounded cold context
  plans, completion payload identity and narrow legacy projection. Six focused
  modules pass 131 tests per profile; fifty payload rows have no identity
  mismatches. Correct tracing-profiler checks pass 28 tests each. All 2,154 valid
  library files compile; eleven intentionally invalid fixtures remain SyntaxError.
- `192cfbe8aa0` narrows LINE invalidation and resets reused tool DISABLE state.
  Related checks pass 21 plus 51 existing tests per profile. The original
  class-initialization fixture passes base and opcode variants in both builds.
  All three build identifiers match the commit; each checks 117 modules.
  Stats smoke passes 146 tests.

The last wide check belongs to native26: 1,572 tests per profile, with four
actual failed modules—monitoring, pdb, peepholer and sys_settrace. A mistaken
obsolete profiling selector is retained as a harness failure; the corrected
module passes. The last full release diagnostic belongs to 87: 54,504 tests,
13 failed modules, 126 failures and 99 errors. No final release/debug gate is
claimed. Two workers provide reciprocal independent reviews; a fresh external
reviewer could not be spawned because of the persistent thread limit.

## Profiling and next work

The stable existing-counter census `fourth-nbody-census-v2` warms twenty separate
run(1) calls, then captures 0/2/4 outer loops on both stats builds. Selected routes
are stable before/after all captures. Golden enters 17,600,702 handlers per outer
loop; candidate enters 17,560,695. The only differences are removed POP_ITER
(40,005) and POP_TOP (2). All other material route counts match. POWER adaptive
retry counts are nonlinear in both roles and are reported as such. These are
handler counters, not native instruction counts or elapsed measurements.

The first census is retained separately: a single run(20) warmed advance but
left setup specialization active. Older 87 PMU results belong to that older
image, whose elapsed nbody regression no longer repeats. They cannot establish
the current image's cost. Earlier perf captures have zero lost samples but a
HEADER_EVENT_DESC warning and support flat locations only, not a capture timeline.

The next reviewed increment separates cancellable PRE expression inputs from
active saved-local obligations. Losing a saved origin keeps its obligation
active; only matching TOP restoration ends it. The following scoped cold phase
contract must distinguish consumed branch results, C-held results awaiting
publication, yield-before-suspension and an actually suspended frame. Saved-local
restoration, caught exits and retained outer-with roots need explicit actions
and survival proofs, rather than relaxed physical checks.

After coherent repairs: full release/debug qualification, API checks, current
stats, the unchanged 29-workload screen, repeat material losses, and the two
final parity campaigns. Profile a repeated current loss before admitting further
hot-path changes. Startup RSS and a zero-box census remain outside admission.
