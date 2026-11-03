# Integrated system review

Independent read-only review of `boil/design/jit-e2e-system.md`, 2026-11-01.
The earlier independent A/B review and receipt remain unchanged.

The integrated design agrees with the final method and tracing proposals:
whole-method native CFG with shared semantic helpers; observed recording;
baseline→LIR and method/trace→one SSA→the same LIR/backend; one Action trampoline,
FrameState, ownership model and active CodeUnit lifetime. It treats canonical
homes as boundary bring-up policy and includes loops, Python/C calls, mixed
objects, errors and suspension fallback before optimizer layers.

The first draft correctly keeps before/after-effect exits, weak/versioned
mutable dependencies, traversed strong pools, active-code retirement and loop
service. Review requested two explicit folds from the component plans:
recording pauses inside opaque callbacks, and live values are materialized
before irreversible effects unless safe allocation-free installation/recovery
is proven. Exactly-once argument/result and recursion transfer is also useful
clarity at EnterFrame/Return.

**ACCEPT the final integrated design** at SHA
`724b2ab88e55394ea6e536887b2aeb6b871ce69e74d802fe02eb620a17912ccc`.
The final text incorporates all three clarifications. No critical architectural
misalignment remains between this document and the final A/B proposals. The
helper generator, trampoline, materializer and backend remain implementation
obligations, not functionality established by this review.

JIT is selected as a system direction. Mechanism experiments and integrated
performance still matter; incomplete arithmetic component results neither prove
production support nor decide the project's existence. The 18-path candidate
remains frozen, unexecuted component source. This review grants no implementation,
native, timing or landing authority.
