# Preserve common evaluator state

- State: active
- Owner: qualification_review_resumed; coordinator reviews and selects a design
- Updated: 2026-10-31
- Budget: 30 minutes, host-only; 23:14:20–23:44:20 UTC
- Checkpoint: 23:29:20 UTC
- Depends on: [constant-operand elapsed rejection](../interpreter-constant-operands/performance-results.md)
- Decision: investigate concrete common-handler stack work before another opcode increment

## Hypothesis

The rejected constant-operand build adds instruction-pointer and frame reloads
and next-pointer stores in otherwise unchanged common handlers. A focused
generator or evaluator-state change may preserve useful register state without
duplicating the runtime or changing its root/observer contract.

## Experiment

Inspect the preserved matched images, generated C, compiler policy and state
lifetimes. The coordinator reproduced all twelve bounded native captures in
`build/constant-operands-quiet-v1/author-analysis` exactly. This establishes
extra emitted common-path operations, not their share of elapsed cost or the
responsible compiler pass. Return one concrete minimal design with falsifiable
native and elapsed checks. No source implementation, compiler experiment,
target invocation, new workload or timing belongs to this scope.

## Decision rule

Select a prototype only if it addresses actual remaining work and preserves
precise roots, debugging, monitoring and every dispatch entry. Do not infer a
speedup from register assignments or smaller text. Reject an unsupported
register-pinning claim or a broad redesign without a bounded matched question.
The deferred Clang-tail [H1 scope](../h1-tail-dispatch/status.md) is separate.

## Now / next

The thirteen-handler increment remains rejected. Host-only research starts
from that negative result while GC cadence source review and native-feedback
preparation proceed in parallel. The next decision is a reviewed design or
reported rejection; no successor implementation is authorized yet.
