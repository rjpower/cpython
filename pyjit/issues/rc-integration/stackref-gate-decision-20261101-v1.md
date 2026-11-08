# Adopt the ordinary RC qualification scope

2026-11-01 10:03 UTC. **ADOPT the independently accepted whole-profile
scope amendment.** This decision supersedes the mandatory global
`Py_STACKREF_DEBUG` full-suite row in the frozen broad qualification plan.
It changes no runtime, checker, tests, result or collector policy.

The [proposal](stackref-gate-amendment-proposal-20261101-v1.md),
[actual final assessment](stackref-control-final-assessment-20261101-v1.md)
and [independent review](stackref-gate-amendment-review-20261101-v1.md)
establish the limits. The table-backed global diagnostic profile is currently
unsupported/unvalidated and does not block the ordinary RC configuration.
Both upstream builds failed before the three diagnostic methods, so there is
no upstream reproduction claim. The fork's partial checker run stays failed:
all twelve failed modules,43 assertions, unknown causes and two unfinished
modules remain. Nothing is converted to a pass or hidden behind exclusions.

Supported scope remains Linux/x86-64/GIL/GCC computed RC with boxed-slot shapes,
private extension identity and ordinary release/O1-debug configurations. Carry
their accepted full suites, scoped ASan and standard ownership/refleak evidence
only at their actual scopes. Any concrete defect in those configurations is
still blocking. This is not a claim that every ownership path is correct.

Stop the upstream checker repair path. Future support for this diagnostic
profile needs a separately authorized repair and fresh qualification; this
decision schedules neither. The tracing-GC decision remains closed and no
collector selector is introduced.

Mandatory remaining gates are unchanged: actual corrected packaging feedback,
unchanged regeneration, both affected ensurepip/venv suites, public pip foreign
tag rejection and original CPU-extension/refcount interval; independent landing
and exact source/protected-file bridge; two fresh main builds and42 selected
consumer methods per image. No landing or JIT implementation is authorized
by this decision. The matching JSON binds actual adoption UTC and exact inputs.
