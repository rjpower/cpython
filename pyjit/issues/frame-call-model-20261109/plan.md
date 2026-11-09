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

Engineering `d7abf803c607b0de3cec58d7e91ba91ff1699792` is committed.
Both profiles build117 modules and pass136 focused tests plus four selected
original/backward/forward/refusal controls each. The preceding NULL36 packet
passed691 tests/13 modules in both roles and repaired three original peepholer
cases. The [thirteenth report](thirteenth-integration-report.md) records both.

Current37 basic logging is0.907800/upstream. Two-pair golden ratios are
logging0.645187, generators1.006499, coroutines0.958126 and nbody0.979884.
Generator37 needs a repeat before declaring its earlier loss repaired. The
current hardware evidence remains36: cycles1.045237/instructions1.020424/
CPI1.024316/golden, six valid100% captures and two records; no37 profile is
claimed. The latest full29 screen is35. Final two six-round logging campaigns
and broad checks remain required at the final source.

Forward same-caught PRE now reuses the existing graph, origins, cuts and Plan,
preserves the old backward proof, and requires fresh PRE/empty saves/exact
caught roots with a normal-edge-only current-to-target survival slice. Finally
payload roots and sibling except arms remain probe-first. The next CALL
success-window NULL query has a finite22-route provider census and independent
source approval; it is applied to the primary runtime for native checks.
The shared contract excludes arbitrary temporary aliases and unknown tails.

The isolated source now has committed36 plus the frozen forward packet. B/H
have reviewed the CALL success-window physical fact separately. Read-only C
matrix probes confirm two ordinary refusals need a NULL fact after completed
CALLs; every provider and result alias must support the proof before admission.
No separate native build exists. Root owns source admission, native jobs,
commits and qualification. Full suites belong at a coherent combined checkpoint,
with no source/build mutation during quiet performance windows.

The aggregate cold-analysis budget is stricter for large eligible PRE requests
than the previous single lane; retain that qualification caveat. Golden and
upstream controls and sealed evidence remain immutable. Current stats, full
release/debug suites, generator repair, final logging gate and cold review
remain open.

## Parallel responsibilities

| Stream | Owner | Current deliverable |
| --- | --- | --- |
| Canonical-home transfer | Sol 6.1 medium A | Canonical35 integrated; forward same-caught implementation frozen for root native gate |
| NULL-only PRE consumer | Sol 6.1 medium B | NULL36 integrated; CALL success-window proof and actual-state diagnosis |
| Shared physical facts and review | Sol 6.1 medium H | NULL36 integrated; independent source/provider and current PMU audits |
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
