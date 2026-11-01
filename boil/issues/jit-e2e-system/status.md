# Cohesive method and tracing JIT

**M0 semantic behavior accepted; M1 whole-method source implementation active.**
User feedback36 on Loom revision31 replaces the narrow arithmetic-head tier
proposal with an end-to-end method and tracing JIT, with layered optimizations.
The JIT is the selected strategic direction. Experiments test implementation
choices and optimization hypotheses within that direction; a small prototype
is not required to justify the architecture's existence by winning the full suite.

The accepted RC runtime remains selected. Three earlier frame-entry candidates
remain rejected on their measured results. The unexecuted quickened-block source
is retained as component evidence; its interrupted activation did not take effect.
No tracing-GC work, collector selector or permanent competing backend is added.

Parallel assignments:

- restart_audit: M1 whole-method lowering, shared backend and code ownership.
- coroutine_design_review: tracing/SSA research completed; subsequent review turn unavailable.
- qualification_review_resumed: independent M1 source and ownership/ABI review.
- coordinator: current-source audit, integration, peer review, issue/worklog and Loom.

Use primary LuaJIT and V8 sources, with explicit adaptations for Python RC,
destructors, exceptions, calls, monitoring, suspension and C extensions. Prefer
one frame-state/ownership ABI, shared semantic operations and one optimizing
backend. Distinguish executable interpreter homes from compiler SSA and native
register allocation. Decide the roles of method compilation and tracing together.

The reviewed architecture and ordered implementation plan are fixed below.
Each milestone must exercise useful control flow and calls,
with focused correctness and diagnostic measurements first. Full-suite acceptance
and expensive qualification remain appropriate for coherent adoption candidates.
The [system design](../../design/jit-e2e-system.md) is published to the existing
Loom artifact, revision36, with code and lifecycle diagrams.

The [integrated architecture](../../design/jit-e2e-system.md) passed
[independent review](design-review-20261101-v1.md); the
[decision](design-decision-20261101-v1.json) pins the exact design and both
research proposals. [Implementation milestones](implementation-milestones-20261101-v1.md)
separate broad baseline coverage, recorded-loop SSA and optimization layers.
M0 source implementation, independent review and focused runtime validation are
complete. The tracing reviewer encountered provider
capacity on its next turn twice; review was reassigned without changing models
or blocking the work. M1 source implementation is active under the grant below;
candidate builds, runtime execution and timing remain separately gated.

The [first source cut](first-slice-design-20261101-v1.md) passed
[independent review](first-slice-design-review-20261101-v1.md). Its
[validation contract](first-slice-validation-20261101-v1.md) and
[lifetime clarification](first-slice-lifetime-note-20261101-v1.md) are fixed.
[M0 source grant](m0-source-grant-20261101-v1.json): 2026-11-01T16:23:54.586838+00:00
to 2026-11-01T17:23:54.586838+00:00, checkpoint 2026-11-01T16:43:54.586838+00:00.
The source attempt closed early at 16:53 UTC. It produced callable generated
semantics and a real evaluator Action consumer; runtime correctness and
performance remain unvalidated. The next source phase adds the
shared LIR/backend and complete loop/call/error native path.

At 16:43 UTC the checkpoint was partial: 62 generated helpers and 30 passing host
tests were present, but independent review found 289 explicit table rows against
290 valid metadata handlers. The missing generic BINARY_OP base now has an
explicit before-operation fallback reason; successor f4f9c893 preserves the
original dafd9ad6 claim and independent a75820d7 counterexample. Source review
and final fixture work completed before the original cap. The first debug
attempt failed as recorded below; the same eight-method semantic-helper class
remains the runtime check before M1. No 54-case or 29-workload gate is inherited.

Initial frozen source: `49cc47360ccd79c7a89acc8bfa3d546f94afc46c`, 13 paths. The
[independent source review](m0-source-review-20261101-v1.md) and
[source disposition](m0-source-disposition-20261101-v1.json) accept it for that
focused debug validation. Both author and reviewer passed 30 host generator
tests; all six existing ID/metadata products remain byte-identical. This is
shared helper/evaluator infrastructure, with no native JIT execution claimed.

The [first debug outcome](m0-debug-outcome-review-20261101-v1.md) is a compile
failure: 27 preprocessor directives joined the preceding C statement. No runtime
method ran. Supervisor 78057 exited 1; terminal cleanup verified native idle,
all 20 locks, unchanged historical images and removal of the owned cache.

Validated M0 source: `f0a49b6d30a38db705329bcfd5e5f2d6d2413710`. The
[focused correction](m0-directive-source-disposition-20261101-v1.json) adds one
writer line boundary, regenerated whitespace and a regression check. Independent
review passes 31 host tests and confirms two successful syntax-only C checks.
All original evidence remains preserved. The subsequent fresh debug build and
same eight runtime methods passed with zero skips. The
[independent behavior review](m0-debug-outcome-review-20261101-v2.md) and
[decision](m0-behavior-decision-20261101-v1.json) accept this prerequisite for M1.
This proves generated helper/evaluator semantics, with no native JIT or speed claim.

[M1 source grant](m1-source-grant-20261101-v1.json): 17:17:09–18:47:09 UTC,
checkpoint 17:47:09. The outcome is a complete native-method CFG with branches,
backedges, shared helpers, Python/C calls, normal/error continuations and code
ownership. Real emitted entry and compiled-continuation witnesses are required;
compile success alone is insufficient. Implementation and independent review
run in parallel. Host generation and small standalone encoder checks are allowed;
candidate execution and broad qualification follow a separately bounded freeze.
