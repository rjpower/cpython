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

Engineering `df12a12b131384976466310b7a6138417b7a36eb` is committed and
independently reviewed. Canonical plain-yield/list-child transfers reuse the
existing sink machinery after full-chain source/AST proofs. Release/debug each
build 117 modules and pass 582 focused tests / 12 modules. Exactly two generated
final stores change; the other 285 handlers and all metadata remain identical.
The [twelfth report](twelfth-integration-report.md) records current evidence.

The unchanged 29 screen is 0.954489/golden and 1.163732/upstream. Logging preserves
parity in this basic screen at 0.899568/upstream. Four-pair golden repeats give
generators 1.025477, coroutines 0.979644, nbody 0.986605. Only the generator loss
persists among those three; float has a smaller two-pair 1.1% loss. Current
hardware profiles have not been captured and old counters
must not be relabelled35. Cross-campaign changes do not establish causes.

The isolated NULL PRE packet is source-frozen, independently accepted, retained
as a patch and applied into the primary source after timing35. Root is running
serialized release/debug native qualification. The shared query, pathwise
must-NULL facts and VALUE/NULL_ONLY obligations use the existing graph and Plan.
Permission stays ordinary real-LINE PRE/inherited context/empty saved bindings;
all-alias and physical-cell reproof preserve strict adapters. Original
peepholer witnesses and new join/reentry controls remain unqualified until
executed. No native build was created in the isolated worktree.

Next: qualify this merged packet, inventory actual remaining debugger phases,
and admit shared retained-root capabilities from that evidence. Independent
review also records an increased cold resource bound for eligible PRE requests;
large-function qualification must retain that limit explicitly. One frame/call
model remains selected. Golden/upstream controls and sealed evidence stay
immutable, and full suites remain for the coherent combined checkpoint.

## Parallel responsibilities

| Stream | Owner | Current deliverable |
| --- | --- | --- |
| Canonical-home transfer | Sol 6.1 medium A | Integrated canonical35; next same-caught proposal and independent diagnostic peer |
| NULL-only PRE consumer | Sol 6.1 medium B | Frozen NULL PRE consumer; next read-only actual-state diagnostic for remaining jumps |
| Shared physical facts and review | Sol 6.1 medium H | Frozen NULL query/controls; independent native and performance audits |
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
