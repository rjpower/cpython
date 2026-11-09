# Frame/call round: closure and next work

Standing direction: [goal](../../GOAL.md). Actual outcomes: [status](../../STATUS.md).
The [closing report](closing-report.md) owns measurements and qualification;
[the next five priorities](next-steps.md) own proposed follow-up work.

## Frozen candidate

Runtime `4d666e1b482cb4983c7683f91c5922af14d0ab40`, branch
`pyjit/frame-call-model-20261109`. Production code is from `24f4d270ced`; the final commit adds reviewed
explicit refusal/cleanup and immediate-entry tests. The candidate is clean.
Unfinished coroutine, retained-owner and GDB drafts are excluded and preserved.
Golden `0765e9be47f` and matched upstream `3a440b65695` remain immutable controls.

This round redesigned storage, argument/result ownership, executable source
positions and semantic completion as one model. The design pages describe
implemented behavior: [IR](../../design/register-ir.md),
[interpreter](../../design/interpreter.md), and
[frames/calls](../../design/call-and-frame-lifetimes.md).

## Closure gates

1. Complete release/debug full diagnostics and current stats/API checks. Keep
   real runtime failures, optional frame-jump gaps and GDB tooling failures
   distinct. Never claim a green full suite when recorded tests failed.
2. Run unchanged balanced timing for all 29 kernels against golden and upstream.
   Confirm logging with two independent six-round upstream campaigns. Repeat
   material broad losses individually instead of hiding them in an aggregate.
3. Cover all 65 registered micro instruction rows and all six timed memory
   workloads on the three builds with default samples and retained raw counts.
   These supplemental sweeps are diagnostics; their unpaired memory times are
   not a balanced throughput claim. Startup/import RSS stays out of scope.
4. Audit native identities, fixed compiler/affinity/hash/JIT state, input hashes
   and raw results. Profiles must name the image actually recorded. Do not
   relabel older PMU data as current or infer a causal budget from sample shares.
5. Publish independent cold reviews and a ranked five-item next plan. Collate
   repository and Loom navigation, archive historical active artifacts, preserve
   evidence/user work and remove only task-created scratch/worktrees.

The coordinator owns all native runs, integrations and measurement windows.
Workers are now source-review and results-audit only. No new runtime feature
is added while closing this round. Benchmarks, library inputs and builds remain
fixed throughout the measurement window.

## Scope and decision

Logging parity is the round's throughput objective: unchanged JIT-off workload,
two balanced six-round campaigns, geometric mean candidate/upstream at most 1.0
and every pair at most 1.02. Broad performance is reported separately.

GDB pretty printing is developer tooling. Exact CPython display formats,
internal bytecode presentation and prompt finalizer timing are not language or
performance gates. Frame-jump capabilities and source positions have their own
stated API limits; preserve safety/control tests and report unsupported cases.
This boundary follows the user's close-round direction. It does not turn failed
tests into passes or authorize dropped owners and partial commits.

After closure, implement [the ranked next work](next-steps.md) in separate
reviewed experiments. No claim is made that all five will improve performance.
The immutable golden remains the selected control; promotion of this engineering
candidate is a separate explicit decision based on the final report.
