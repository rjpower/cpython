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
The Astra review also identifies a source-level acquire-after-receiver-retire
hazard in two object-form class-attribute handlers. Initial coordinator lifetime
controls pass but stay on generic LOAD_ATTR, so they do not exercise those
handlers and do not disprove the finding. Focused coverage and the established
result-publication contract belong to the next lookup lane. There is no claim
of a reproduced stale-pointer failure or a completed repair.

## Publication contract

Publish immutable tags `pyjit-golden-20261109-runtime` for exact runtime39 and
`pyjit-golden-20261109` for the combined snapshot. The combined snapshot takes
every implementation path from runtime39, removes historical `boil/` documents,
and installs committed canonical `pyjit/` documents. Its merge parents include
the existing golden branch and runtime39, allowing a normal fast-forward push.
Verify all implementation blobs against runtime39 before pushing atomically.
Record resolved local and remote refs in `publication.json` afterward.

The [object/call plan](../object-call-20261109/plan.md) owns subsequent work.
The old baseline remains available for comparisons; a new frozen release build
will provide a direct runtime39 control before new candidate measurements.
