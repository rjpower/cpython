# Focused cadence feedback and causal controls

Held for concrete helper review and coordinator activation. Source is frozen
at `c8cc7718ef3037500aa3902ba5c31acc2b8d7c60`, parent `ef0af95904b8`.
The [exact-source review](reviews/cadence-source-20261031-v1.md) accepts this
candidate for native feedback; it does not establish native behavior.

Use one separately activated 45-minute batch, including builds and cleanup,
within the unchanged 2026-11-01 04:18:11 UTC overall deadline. Downward-clamp
each row to remaining time. Stop at the first unexpected failure, preserve the
outputs and verify cleanup. No automatic retry or qualification follows.

## Positive candidate

Build fresh GCC O1/Py_DEBUG products in `build/debug-cadence`, preserving all
earlier sealed builds. Bind exact configuration, compiler/tool identities,
source, generated files, imports and linked precise-frame audits. In
particular, the new collecting helper must retain its precise frame contract.
Use the accepted machine-slot, build-lock and process-group protocol.

Run the thirteen actual parent methods and twenty-two fresh children named in
the frozen source handoff, through regrtest with parent OFF/N0 and no planned
skips on this supported debug host. Preserve all actual IDs, child modes and
stress observations; dynamic stress is not per-allocation telemetry. Require
the actual minor/major, useful young deaths, old-cycle reclamation, payload,
empty-probe, zero-death, finite-debt, failure/backoff and callback assertions.

The unchanged settrace control uses direct `assert_python_ok` and its natural
isolation. Its observer must report its actual flags, checker mode and hash
seed separately. It does not receive the queue-child startup prelude. Do not
assign unobserved worker or child coverage. Keep normal root-counter evidence
and zero unwind fallback; retain unpinned observations with their actual scope.

## Three isolated source negatives

Each temporary patch gets a clean managed worktree derived from exact c8cc,
its own reviewed commit and fresh debug products. No option is shipped and no
sealed image is patched. Positive candidate execution precedes these controls.

| Source fault | Exact nominated child | Required failure |
| --- | --- | --- |
| Force only automatic minor choices to major | OFF automatic sequence child | `automatic kinds`, with the required leading minor absent |
| Advance M after every successful minor | Repeated-minor finite-major child | `manual minors starved the automatic major` at the frozen final bound |
| Consume H before a young mark succeeds | Retain, minor trace fault stage 3 | `failed attempt consumed minor opportunity` after the fault fires |

Run only the exact source-derived nominated child for each negative, through
the existing gate and typed observer. Preserve its production assignments,
body, prelude and cleanup; bind the extracted bytes to the positive source.
These direct child failures earn causal evidence, with no parent-method or
regrtest pass credit. Do not narrow production loops or suppress other child
calls. Require child exit 1 and the nominated assertion, actual kind/fault/root
witnesses and terminal cleanup. Crashes, missing evidence, import errors or a
different assertion are unexpected failures.

Trace fault stage 3 is protected-heap/domain preflight before page inventory;
stage 2 is inventory and stage 1 is verifier-ledger preparation. Some frozen
source handoff labels call stage 3 inventory; this clarification changes the
description, not the source or nominated preserved-H assertion.

## Decision and remaining work

Accept only the exact focused behavior, source-negative detections and linked
audits actually executed. Broader coherent GC feedback, release cost comparison,
full qualification and landing remain pending. The nursery stays mandatory.
The automatic cadence must separately earn acceptance under the elapsed-first
[design](cadence-design.md); failing it retains the adopted automatic-major
policy without a runtime switch.
