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

Engineering `24f4d270ced` is committed. Release/debug check117 modules and
pass860 focused tests/14 modules/two skips each. The four original synchronous
jump cases pass unchanged. A release debugger census retains20 trace errors
and two Pdb transcript failures; basic Pdb jump commands pass. See the
[fourteenth report](fourteenth-integration-report.md).

Current38's four-pair golden check passes stable identities: logging0.572018,
generators0.992624, coroutines0.871626 and nbody0.975905. The earlier generator
loss does not repeat. Full release qualification of frozen38 is running. Basic37 gave
logging0.907800/upstream over two pairs. The full29 screen remains35, and counters remain36.
Final qualification and logging campaigns must use the final coherent source.

The isolated source has been fast-forwarded to38 while preserving the reviewed
refusal and Pdb test ports. H owns coroutine first-resume analysis and tests;
A owns only those debugger test ports. B designs a shared retained-owner/context
recipe for remaining ordinary exception/loop transfers. Created-throw ingress
at prologue0 is explicitly excluded from the bounded coroutine NULL proof when
ordinary or source-error recipes cover it. No separate native build exists.
Root owns all native gates, commits and measurements. Workers do not mutate the
primary source or builds during frozen qualification or performance windows.

The aggregate cold-analysis budget remains a qualification caveat. Golden and
upstream controls and sealed evidence stay immutable. No test port may turn an
ordinary capability failure into a permitted refusal without a reviewed
semantic distinction and strong unchanged-state/normal-cleanup controls.

## Parallel responsibilities

| Stream | Owner | Current deliverable |
| --- | --- | --- |
| Debugger test ports | Sol 6.1 medium A | Explicit protocol refusal controls and immediate-entry Pdb position/continuation |
| Retained owner/context model | Sol 6.1 medium B | Shared owner-bundle proposal for ordinary caught/finally/loop capabilities |
| Coroutine ingress and review | Sol 6.1 medium H | Bounded first-resume proof, independent source/result reviews |
| Integration and measurement | Coordinator | Source admission, serialized native gates/commit/basic performance, qualification and final logging campaigns |

Workers propose first and independently review each other's contracts. Runtime
file ownership is assigned after interface review. They do not run shared
native builds or measurements. The coordinator owns commits and integration.
Reviewers inspect actual source and measured evidence after each integration.

## Work sequence

1. Finish the independently reviewed canonical-home packet in the primary
   source, inspect exact generated-handler changes, run focused gates and
   basic performance. In parallel complete the isolated shared NULL query and
   frame consumer under explicit file ownership. Keep one ownership/transfer
   model, source-derived facts and strong negative/reentry tests. No new state
   field or benchmark-specific shortcut is admitted.
2. Fold the independent cold architectural review into the next admission.
   Keep one frame/call/owner model. A new optimization needs a concrete producer
   or call-shape proof and an isolated comparison, not a benchmark-specific path.
3. For correctness, admit only shared capabilities backed by actual failures.
   The three ordinary PRE NULL-only witnesses use forward physical facts and
   independent VALUE/NULL_ONLY demands, with simultaneous transports, all-alias
   checks and conservative unsupported/error states. Do not reproduce arbitrary
   CPython debugger crossings solely to satisfy its accepted jump matrix.
4. Each admitted source packet gets the cheapest meaningful checks, native
   release/debug focused gates, independent post-review, commit and basic
   performance check. Keep full suites for coherent integration points.
5. Run coherent full release/debug qualification and current stats. The current
   API check already passes 306 units with zero failing findings/parse errors;
   repeat it only when subsequent changes justify it. Retain the prior GC negative-control failure; a later pass alone does
   not establish its cause. Resolve documented contract differences explicitly
   and preserve their safety/behavior tests rather than silently weakening them.
6. Complete two independent balanced six-round logging/upstream campaigns,
   current profile evidence and a final cold review. Update canonical designs,
   diagrams if changed, status and repository/Loom report. Logging parity and
   repaired broad performance remain the completion conditions.

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
