# Interpreter shared costs

- State: queued
- Updated: 2026-10-31
- Owner: unassigned
- Dependencies: closed [GC cleanup/scanning](../gc-cleanup-scanning/status.md);
  bounded attribution and source ownership need assignment
- Branch/head: none assigned
- Worktree: none assigned

## Now

The register/value interpreter remains the current architecture. Matched
performance is measured, but residual sampled CPU is not yet attributed
specifically to frame, call, native conversion or dispatch overhead. There are
no active attribution jobs, benchmarks or implementation changes for this issue.

The queued investigation starts from existing stacks, instruction counts and
source reviews. Later-row polymorphic attribute matching survives emitted O3
code; row zero is already optimized, and dynamic frequency remains unmeasured.
Other prospective shared boundaries include value receivers/results and
frame/call/resume transitions. Existing argument-window sharing and compiler
folds must not be proposed again as missing features.

[Value/native interfaces](../value-native-interfaces/status.md) retains the
full slot/Clinic and compatibility inventory. [Alias-safe linear string
growth](../../design/work/pre-I-linear-string-concat.md) is another design
candidate: its [proof/owner prerequisite](../../design/work/pre-I-linear-string-proof-owner.md)
and observer/escape contract precede mutation. Neither is an implementation
assignment or an assumed explanation for current costs.

The invariant arithmetic diagnostics retain runtime arithmetic, but do not
measure `i += 1` recurrences. Roughly 2× remains guidance, not an opcode quota,
aggregate gate or promise. The existing profile's timed GC budget alone does
not explain every deficit.

## Next

Assign a bounded attribution question. Reuse existing evidence first; propose
an additional measurement only when needed to distinguish plausible costs.
Then present an orthogonal shared-interface change with roots, exceptions,
fallback and observer semantics explicit. This can run alongside the assigned
young-space track; native measurement admission remains separately coordinated.

## Completion

Close the attribution question with reproducible evidence and either a scoped
implementation proposal or a supported decision to defer it. Any implemented
change needs behavioral validation and matched cost reporting before landing.
Do not substitute a specialized opcode name for a representation contract.

## References

- [Current recommendation and parity budget](../../design/work/gc-architecture-next.md).
- [Measured results](../../design/work/gc-post-cleanup-performance.md).
- [Holistic source review](../../design/work/reviews/interpreter-holistic-review-post-gc.md).
- [Polymorphic emitted-code review](../../design/work/reviews/interpreter-poly-shape-emission.md).
