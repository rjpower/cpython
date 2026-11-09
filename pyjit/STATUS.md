# Project status

The frame/call round is closed at engineering candidate `4d666e1b482`.
Logging meets the upstream parity gate. Across all 29 kernels, elapsed geometric
means are **0.934× golden** and **1.175× upstream**. Earlier material golden
regressions no longer repeat; broad upstream parity remains open.

Start with the [report](issues/frame-call-model-20261109/closing-report.md),
[complete tables](issues/frame-call-model-20261109/closing-benchmark-tables.md)
and [next five priorities](issues/frame-call-model-20261109/next-steps.md).
The [plan](issues/frame-call-model-20261109/plan.md) records closure and scope.
The engineering candidate is unpromoted; golden `0765e9be47f` stays immutable.

Both final logging confirmations pass:0.9067× and 0.8975× upstream elapsed time,
all twelve pairs below0.924×. Coverage includes 29 application kernels, 65 micro
instruction workloads and six timed allocation/collector workloads on all three
builds. Startup/import RSS is excluded. Supplemental memory ratios are unpaired
diagnostics; the collector's roughly 4% golden difference needs a balanced repeat.
Current PMU/profiles identify excess executed work in object/call and generator
paths, rather than a uniform interpreter penalty.

Release/debug/stats builds check 117 modules each; stats smoke passes 150 tests.
Both full diagnostics run 54,645 tests. Release's failed modules are GDB tooling
and nine frame-jump errors. Debug has those plus one trace-format assertion.
Other executed modules pass, including Pdb, monitoring and collector checks.
The API check passes 306 units with zero failing findings or parse errors.
The full suites are not green; tooling formats and unsupported frame jumps are
reported separately from Python semantics and throughput.

Universal PyValues are integrated across interpreter homes, containers,
attributes and rebuilt extensions. Calls use initialized storage, overlapping
windows, guarded binding, explicit recipients and real continuations. Source
boundaries are metadata. Optimization coverage remains incomplete: an
object-form feedback cache currently excludes inline class scalars, and some
native bridges still box canonical values. See [IR](design/register-ir.md),
[interpreter](design/interpreter.md), [frames/calls](design/call-and-frame-lifetimes.md)
and [values](design/pyvalue.md).

The next round prioritizes guarded canonical lookup, generator delivery,
fixed-keyword binding, value-native boundaries and structured ownership effects.
Native method/tracing JIT tiers remain unfinished. Coroutine/owner/GDB drafts are
preserved outside this measured image; their temporary worktree is removed.
The [handoff](HANDOFF.md) owns exact local paths and recovery. Unrelated user
work and historical evidence remain preserved.
