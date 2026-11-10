# Project status

The frame/call round is closed at engineering candidate `4d666e1b482`.
Logging meets the upstream parity gate. Across all 29 kernels, elapsed geometric
means are **0.934× golden** and **1.175× upstream**. Earlier material golden
regressions no longer repeat; broad upstream parity remains open.

Start with the [report](issues/frame-call-model-20261109/closing-report.md),
[complete tables](issues/frame-call-model-20261109/closing-benchmark-tables.md)
and [next five priorities](issues/frame-call-model-20261109/next-steps.md).
The [closed plan](issues/frame-call-model-20261109/plan.md) records that round.
Runtime39 is now published as `pyjit-golden-20261109-runtime`; the combined
golden branch snapshot is `cb906f9bc52`. The previous golden `0765e9be47f` and
its tags/builds remain immutable. Reported historical golden ratios above
refer to that previous baseline.

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

The active [object/call plan](issues/object-call-20261109/plan.md) starts with
guarded canonical class-value caches and reusable keyword binding plans. Both
producer changes are implemented and regenerated; native validation, actual-diff
review and measurement are pending. Astra reviewed both proposals. The next
lanes are generator-cycle delivery, value-native boundaries and structured
ownership effects. No speedup is claimed for these pending changes.

A targeted class-value lifetime witness crashes both frozen runtime39 and the
upstream control. Runtime39 demonstrably uses the guarded NONDESCRIPTOR path.
The lookup increment acquires and publishes the result before receiver
retirement; this protects lifetime safety without requiring exact CPython
finalizer timing. Native verification of the repair is pending.
Native method/tracing JIT tiers remain unfinished. Coroutine/owner/GDB drafts are
preserved outside this measured image; their temporary worktree is removed.
The [handoff](HANDOFF.md) owns exact local paths and recovery. Unrelated user
work and historical evidence remain preserved.
