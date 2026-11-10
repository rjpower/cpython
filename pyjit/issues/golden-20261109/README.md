# Golden comparison checkpoint, 2026-11-09

The user selected frozen runtime `4d666e1b482cb4983c7683f91c5922af14d0ab40`
as the next golden comparison checkpoint after the
[Astra review](../object-call-20261109/astra-survey.md).
The moving branch is `pyjit/golden-20261108`; the earlier immutable
`pyjit-golden-20261108*` tags and native controls remain unchanged.

This is an engineering comparison baseline, with explicit qualification limits,
not a claim that the full suites are green. The
[closing report](../frame-call-model-20261109/closing-report.md) and
[complete tables](../frame-call-model-20261109/closing-benchmark-tables.md)
own its measured evidence: 0.934192 times the previous golden's kernel elapsed
time, 1.174985 times upstream, and both dedicated logging confirmations below
upstream. No new runtime code is included in this publication.

## Qualification limits

Both full diagnostics ran 54,645 tests. Remaining failures concern GDB tooling,
nine debugger frame-jump capabilities and one debug trace-format assertion.
The Astra review also identified an acquire-after-receiver-retire hazard in two
class-attribute handlers. Initial controls stayed generic and were inconclusive.
A revised eligible list-valued witness now reproduces SIGSEGV in both runtime39
and upstream; runtime39 prints its specialized NONDESCRIPTOR handler before
failing. This is a lifetime-safety issue shared by the controls, not evidence of
an introduced regression. The next lookup increment applies the established
acquire/publish-before-retire contract; its native validation is pending.
The immutable comparison baseline remains unchanged.

## Publication contract

Published immutable tags `pyjit-golden-20261109-runtime` for exact runtime39 and
`pyjit-golden-20261109` for the combined snapshot. The combined snapshot takes
every implementation path from runtime39, removes historical `boil/` documents,
and installs committed canonical `pyjit/` documents. Its merge parents include
the existing golden branch and runtime39, allowing a normal fast-forward push.
Verify all implementation blobs against runtime39 before pushing atomically.
Record resolved local and remote refs in `publication.json` afterward.

The [object/call plan](../object-call-20261109/plan.md) owns subsequent work.
The old baseline remains available for comparisons; a new frozen release build
provides a direct runtime39 control for new candidate measurements.
