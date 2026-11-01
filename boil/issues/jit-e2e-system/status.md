# Cohesive method and tracing JIT

**M0 source implementation authorized; M1 backend, builds and timing held.**
User feedback36 on Loom revision31 replaces the narrow arithmetic-head tier
proposal with an end-to-end method and tracing JIT, with layered optimizations.
The JIT is the selected strategic direction. Experiments test implementation
choices and optimization hypotheses within that direction; a small prototype
is not required to justify the architecture's existence by winning the full suite.

The accepted RC runtime remains selected. Three earlier frame-entry candidates
remain rejected on their measured results. The unexecuted quickened-block source
is retained as component evidence; its interrupted activation did not take effect.
No tracing-GC work, collector selector or permanent competing backend is added.

Parallel assignments (read-only):

- restart_audit: whole-method baseline, calling conventions, OSR and shared backend.
- coroutine_design_review: tracing/SSA research completed; subsequent review turn unavailable.
- qualification_review_resumed: independent first-slice review and validation contract.
- coordinator: current-source audit, integration, peer review, issue/worklog and Loom.

Use primary LuaJIT and V8 sources, with explicit adaptations for Python RC,
destructors, exceptions, calls, monitoring, suspension and C extensions. Prefer
one frame-state/ownership ABI, shared semantic operations and one optimizing
backend. Distinguish executable interpreter homes from compiler SSA and native
register allocation. Decide the roles of method compilation and tracing together.

The next deliverable is one concrete architecture, its contracts and an ordered
implementation plan. Each milestone must exercise useful control flow and calls,
with focused correctness and diagnostic measurements first. Full-suite acceptance
and expensive qualification remain appropriate for coherent adoption candidates.
The reviewed design will be filed at [the system design](../../design/jit-e2e-system.md)
and published to the existing Loom artifact with code and lifecycle diagrams.

The [integrated architecture](../../design/jit-e2e-system.md) passed
[independent review](design-review-20261101-v1.md); the
[decision](design-decision-20261101-v1.json) pins the exact design and both
research proposals. [Implementation milestones](implementation-milestones-20261101-v1.md)
separate broad baseline coverage, recorded-loop SSA and optimization layers.
First-slice source design is now assigned to restart_audit, with independent
review by qualification_review_resumed. The tracing reviewer encountered provider
capacity on its next turn twice; review was reassigned without changing models
or blocking the work. No source/compiler/native grant is active.

The [first source cut](first-slice-design-20261101-v1.md) passed
[independent review](first-slice-design-review-20261101-v1.md). Its
[validation contract](first-slice-validation-20261101-v1.md) and
[lifetime clarification](first-slice-lifetime-note-20261101-v1.md) are fixed.
[M0 source grant](m0-source-grant-20261101-v1.json): 2026-11-01T16:23:54.586838+00:00
to 2026-11-01T17:23:54.586838+00:00, checkpoint 2026-11-01T16:43:54.586838+00:00.
The outcome is callable generated semantics and a real evaluator Action
consumer, not runtime or performance acceptance. The next source phase adds the
shared LIR/backend and complete loop/call/error native path.
