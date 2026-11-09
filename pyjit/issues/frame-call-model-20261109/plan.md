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

Engineering `bfd67cbe4b410c9c78292a8eb17849bb24edcb20` is integrated and
independently source/compiled/result reviewed. Proved VM SEND/FOR completion
specializes one C body with canonical input; native/THROW providers retain
publication. The frame ABI, physical continuation and owner/observer contract
are unchanged. Release/debug build117 modules and pass575 focused tests across
11modules each. Completion text grows1194 bytes; no clone or unconditional
normal-path spill bank appears. The [eleventh report](eleventh-integration-report.md)
records the exact evidence and limits.

Four-pair basic campaigns give logging0.801505/upstream (allpairs<0.820), while
golden losses persist: generators1.070491, coroutines1.026246, nbody1.045276.
Current34 normalized counters are cycles/instructions/CPI: generators
1.084226/1.032996/1.049594; coroutines1.029274/0.990849/1.038779;
nbody1.036633/0.986041/1.051308. All18 captures pass at100%running with stable
identities; independent arithmetic agrees. Different campaigns do not isolate
component gains, and no layout cause is established. The full29 screen and
independent two-campaign logging gate remain the32 evidence in the
[ninth report](ninth-integration-report.md).

Two source streams now run in parallel. In the primary integration source, A
implements bounded canonical-home proofs for plain yield and list-child final
publication through existing machinery. Actual SEND_GEN assembly already
coalesces state loads, so state-helper consolidation is not a demonstrated
speedup. In isolated `build/worktrees/null-pre-20261109`, H supplies the shared
cold physical query and B extends existing frame analysis with forward
must-NULL facts and VALUE/NULL_ONLY obligations. Existing strict adapters retain
ALL demands. New permission stays ordinary real-LINE PRE/inherited context/empty
saved bindings, with all-alias and physical-cell reproof. No protocol-policy
ports or broader caught/yield transfer permission are admitted.

Root owns generation/build/tests and reviewed integration into the primary
runtime. The isolated source has no separate build. Native performance windows
require a frozen clean primary source; full suites remain for the coherent
combined checkpoint. Golden/upstream controls and all sealed evidence stay
immutable. The three-option completion/continuation/heap-binding review remains
reference for larger alternatives, not permission for a new ABI.

## Parallel responsibilities

| Stream | Owner | Current deliverable |
| --- | --- | --- |
| Canonical-home transfer | Sol 6.1 medium A | Plain-yield and list-child full-chain source proofs, mutation negatives and focused implementation in primary source |
| NULL-only PRE consumer | Sol 6.1 medium B | Existing frame analysis/Plan extension and tests in isolated source; independent canonical proof review |
| Shared physical facts and review | Sol 6.1 medium H | Cold NULL query/private contract controls in isolated source, independent source/counter audits |
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
