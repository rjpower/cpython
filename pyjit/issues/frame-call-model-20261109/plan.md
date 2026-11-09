# Frame and call model: logging parity

The user requires logging parity, repaired broad regressions and a coherent
register interpreter model. Python's data model governs behavior; historical
CPython destruction positions are not a constraint. This work continues the
existing integration checkout and universal-value runtime.

## Starting point and completion

Runtime source is `build/worktrees/execution-boundaries-20261108`, switched
in place from `4b3d739d867` to branch `pyjit/frame-call-model-20261109`.
Canonical documentation remains in the coordinator checkout. Golden runtime
`0765e9be47f`, its builds and the matched upstream build are immutable controls.
The [previous candidate report](../execution-boundaries-20261108/report.md)
records the starting qualification and regressions.

Completion requires:

- Unchanged `logging_silent`, JIT disabled, matched GCC configuration and native
  identity checks: two independent balanced six-round campaigns with geometric
  mean candidate/upstream at most 1.0 and every pair at most 1.02.
- Repair the five repeated broad regressions against golden. Run the unchanged
  29-workload screen at coherent integration points and investigate repeatable
  material losses individually rather than hiding them in an aggregate.
- Compact executable register instructions. Source/event/exception positions
  are metadata; empty source boundaries are not dispatched or decoded. Calls
  name result recipients directly, including discard. No post-return decoding
  of an inherited release operation.
- Reviewed, implemented frame/argument/result ownership and validity rules,
  including exceptions, generators, re-entry, frame escape, locals inspection,
  debugger jumps and observation enabled while a frame is active.
- Appropriate focused checks, release/debug full qualification at the coherent
  final source, independent source/result review, current design diagrams,
  and a published report/plan in the repository and Loom.

No benchmark, logging-library or upstream-control edits, second compiler,
permanent experiment switches or weakened correctness assertions.

## Current execution

Engineering `417a8bd6e2e3` integrates exact actual-owner/physical-next
terminal facts and committed-jump stopped-state restoration. It preserves
existing strict owner recipes and repairs the six unchanged each-LINE fixtures.
Each release/debug profile checks117 modules, passes343 focused tests with one
skip, five generator methods and the selected twelve-test gate. The999-test
wide gates still fail monitoring1, peepholer3, sys_settrace24 and pdb2 failures/
2 errors. Full qualification remains open. See the
[seventh report](seventh-integration-report.md).

Engineering `3cde040a1afd` additionally reuses canonical local-home duplication
through the shared final store. Independent actual source/result review approved;
25 focused tests pass in each profile and committed builds check117 modules.
Only two generated load handlers change; 285 other bodies, four numeric bodies
and metadata stay unchanged. Four balanced before/after pairs give ratios of
round medians nbody 0.970773, logging 1.034605 and pickle 1.011179. Logging slows
in every pair. A coherent full release diagnostic is running before more model
edits. See the [eighth report](eighth-integration-report.md).

Clean417 short screens give logging/upstream0.864255; separate golden
logging0.618457,nbody1.048030,pickle0.978917. Retained c142 PMU/profile evidence
is historical: logging/upstream cycles0.918210/instructions1.044970;
nbody/golden cycles1.040876/instructions1.033309. Native31 still needs direct
upstream measurement. Do not combine campaigns or infer dynamic gain from static assembly.

The [next shared capability proposal](nonneutral-post-and-null-requirements-proposal.md)
addresses actual committed conditional callbacks and proved NULL-only incoming
target requirements. It reuses the selected normal transfer and shared Plan;
COPY/SWAP requirements transport simultaneously, required aliases dominate and
fresh NULL seeds are bounded to ordinary nonsuspending entry. Shared physical
transfer facts require independent review before implementation. General source,
retained protocol roots, caught exits and yield remain separately gated. No
runtime edits are admitted during native measurement/qualification freeze.

## Parallel responsibilities

| Stream | Owner | Initial deliverable |
| --- | --- | --- |
| Output commit and observer phases | Sol 6.1 medium A | Generator terminal outcomes, activation-scoped phases and exact-frame source-error invalidation |
| Frame restoration | Sol 6.1 medium B | Saved-local actions, retained protocol roots and shared Plan application |
| Encoded ownership proof | Independent Sol reviewer | Physical output mapping, constructor POST/error proof and cross-review |
| Integration and measurement | Coordinator | Native gates, coherent commits, logging/broad measurements, profiling, canonical/Loom reports |

Workers propose first and independently review each other's contracts. Runtime
file ownership is assigned after interface review. They do not run shared
native builds or measurements. The coordinator owns commits and integration.
Reviewers inspect actual source and measured evidence after each integration.

## Work sequence

1. Integrate explicit owner-edge terminal facts, source-publication authority,
   terminal jumps and iterator retirement under the reviewed shared generator
   convention. Preserve unrelated ordinary handlers. Actual owner site and
   reported monitoring source are separate coordinates.
2. Diagnose each failing debugger phase at its actual PC; admit the stopped-state
   consumer only with a concrete witness and positive publication authority.
   Reuse the existing Analysis/Plan and constructor decoder. Complete retained
   protocol roots, caught exits and yield cancellation through reviewed recipes;
   do not turn the generic callback marker into a terminal action blindly.
3. Use minimal host checks, then focused release/debug frame/observer/constructor
   gates. Independently review the actual diff and results, integrate, and take
   basic logging/nbody/pickle screens. Use current profiles to select throughput
   work without weakening target availability or changing the workloads.
4. Run full release/debug suites and API checks at the coherent integration,
   rebuild stats and run the unchanged 29-workload screen. Repeat material losses
   and profile the current native image before another hot-path optimization.
5. Complete both six-round logging/upstream campaigns, independent cold review,
   canonical designs/diagrams and the repository/Loom report. Do not promote an
   engineering screen or a static handler count into a parity claim.

## Measurement and recovery

Evidence belongs under `build/pyjit-link-wave1/frame-call-model-20261109/`.
Reuse the existing candidate release/debug/stats builds configured against
the integration source; do not modify frozen golden products. Host drivers use
`uv run --no-project python -B`, sanitized native environments, CPU 15, fixed
hash seed, JIT-off checks and fixed workloads. Native builds/tests and quiet
timing serialize. Preserve failed attempts and source identities.

Earlier candidate source, reports and measurements remain historical evidence.
Current specifications will describe the new model directly. Update this plan
and STATUS from actual progress; a prepared command or agent assignment is not
an executed result.
