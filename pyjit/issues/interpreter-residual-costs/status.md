# Attribute interpreter residual costs

- State: closed
- Outcome: reported
- Decision: integrate the finding into a bounded implementation experiment
- Owner: stats_design_review; coordinator selects integration
- Updated: 2026-10-31
- Budget: decision within 60 minutes of 15:15 UTC; checkpoint at 30 minutes
- Depends on: [landed/measured GC baseline](../gc-cleanup-scanning/status.md)
- Branch/worktree: read-only attribution on qualified source/captures

## Hypothesis

Existing profiles and emitted code identify one shared frame/call/value boundary
whose simplification can remove meaningful work without a second IR or execution
protocol.

## Experiment

Use captured stacks, instructions and source first. Separate allocation/GC from
frame transfer, calls, native conversion and dispatch. Gather another profile only
for a named unresolved question in a quiet window. Verify the actual arithmetic
recurrence path rather than treating invariant addition as i += 1 performance.

## Decision rule

Integrate the attribution into one bounded implementation experiment if a real
shared cost and safe simplification are supported. Reject the proposed explanation
when samples/native code do not support it. Do not force an optimization to meet
roughly 2× guidance; report uncertainty and select another clean hypothesis.

## Now / next

Offline attribution selected [bulk frame transfer](../frame-transfer-bulk/status.md).
The scalar loop accounts for 2.476% of sampled coroutine CPU; this is not a
speedup forecast. The coordinator independently replayed the extraction and accepted the finding.
No new native run was required; residual CPU remains broader than frame/call cost.
