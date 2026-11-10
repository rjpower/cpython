# Project status

Candidate `0a35a382909` is committed and passes focused checks, but **performance
promotion is on hold**. It makes class reads and fixed keyword calls much faster
than frozen39 while the full application screen is slightly slower. Start with
the [current report](issues/object-call-20261109/checkpoint41-report.md),
[tables](issues/object-call-20261109/checkpoint41-tables.md) and
[active plan](issues/object-call-20261109/plan.md).

| Current41 elapsed ratio | /frozen39 | /upstream |
| --- | ---: | ---: |
| 29-kernel geometric mean | 1.01024 | 1.18105 |
| logging_silent | 1.05443 | 0.94493 |
| coroutines | 1.06442 | 1.35166 |
| DeltaBlue | 1.03889 | 1.67115 |
| Class-read micro, long confirmation | 0.16914 | 1.02406 |
| Keyword-call micro, long confirmation | 0.63626 | 1.12412 |

Kernel screens use two balanced rounds; micros use six, with fixed longer loop
counts and unchanged bodies. Identities/configuration/affinity/hash/JIT gates
pass. Class/upstream variation does not establish parity. Material application
regressions are being repeated. Static disassembly finds newly outlined frame
construction on three call paths; its bounded inline-policy proposal awaits
review and measurement. It does not explain dominant coroutine recursion.
No new golden promotion is authorized by these results.

The candidate implements guarded canonical class-value caches, safe result
publication, reusable 1–3-keyword binding plans and current-default-shape guards.
Release/debug/stats builds check 117 modules. Focused release/debug runs pass
464 tests each; stats smoke passes 150. Five supported keyword shapes each show
1,000 fast-path hits; four fallback controls show zero. A lifetime witness that
crashes frozen39 and upstream now passes both receiver layouts on release/debug.
Astra's proposal, implementation and post-review find no remaining source blocker.
Full diagnostics and the API checker have not yet rerun on this candidate.

Runtime39 `4d666e1b482` is published as `pyjit-golden-20261109-runtime`; combined
golden `cb906f9bc52` adds canonical docs. The previous golden `0765e9be47f` and
both generations of immutable tags/builds remain unchanged. The
[publication receipt](issues/golden-20261109/publication.json) records remote
readback. The [previous closing report](issues/frame-call-model-20261109/closing-report.md)
owns runtime39's 0.934× previous-golden and 1.175× upstream full29 results,
two logging confirmations around 0.90× upstream, 65 micro workloads and six
allocation/collector workloads. Its full release/debug runs each executed
54,645 tests with known GDB display, nine frame-jump and debug trace-format
failures. They were not green suites and do not qualify current41.

Universal PyValues are integrated across interpreter homes, containers,
attributes and rebuilt extensions. Calls use initialized reusable storage,
overlapping windows, explicit recipients and real continuations. Native
method/tracing JIT tiers remain unfinished. The next design work is a measured
generator-cycle contract, value-native descriptor bridges and structured IR
ownership/effect facts. A global object-layout rewrite is not justified. See
[IR](design/register-ir.md), [interpreter](design/interpreter.md),
[frames/calls](design/call-and-frame-lifetimes.md), [values](design/pyvalue.md),
and [handoff](HANDOFF.md) for exact contracts, paths and evidence.
