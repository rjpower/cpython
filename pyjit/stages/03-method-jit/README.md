# Stage 3: shared compiler and method JIT

State: planned in [tiers87](../../design/proposals/tiers87.md) (proposed
2026-11-22): a Sparkplug-style baseline compiled from the interpreter's
operation contract, after contract87. An optimizing method tier waits for
evidence that traces leave a gap. The text below predates that plan.

Deliver a cohesive method compiler with layered optimizations, shared typed
SSA/effect semantics, register allocation, backend and recovery. Existing
interpreter IR registers are frame homes; choosing them did not establish an
optimal encoding, hardware-register assignment or optimizing SSA design.

Workstreams cover feedback and specialization, method construction and shared
IR, guards/deoptimization and state recovery, code lifetime/backend, and
end-to-end qualification. Use LuaJIT/V8 lessons as design inputs, not as reasons
to preserve poor existing Python patterns or a one-op-family architecture.

Entry: Stage 2 provides a trustworthy baseline and coherent value/native contracts.
Exit: whole-method execution, opaque-call boundaries, ownership, recovery,
exceptions and observer behavior are validated on the candidate, with measured
end-to-end workloads. Experiments improve specific mechanisms inside the selected
JIT direction rather than repeatedly voting on whether it should exist.

References: [compiler/IR](../../design/compiler-and-ir.md),
[method/tracing system](../../design/jit-e2e-system.md),
[feedback and attribute IR](../../design/jit-ir-feedback/design.md),
[existing experiment](../../issues/jit-e2e-system/status.md),
[current status](../../STATUS.md).
