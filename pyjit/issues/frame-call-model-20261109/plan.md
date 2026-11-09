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

Engineering `488e85fa3a88aed7d673c154b22d40ba42080a18` is integrated and
independently source/result reviewed. The exact committed-frame view supports
normal nonneutral edges using copied site rows and the existing selected normal
transfer. Old-origin cuts remain separate from physical replacement values.
New permission requires inherited context, empty stopped and target saved-binding
states, and no target scratch demand. PRE/neutral strict recipes are unchanged;
nonneutral POST cannot use their pre-site survivor proof or source-only fallback.
The six legacy tracing window reads now use explicit borrowed views.

Release/debug each build 117 modules and pass 455 focused frame/observer tests
with one skip, including the entire monitoring module. The subsequent allocation
warning cleanup rebuilds 117 modules and passes all seven targeted checks in each
profile. Those later checks do not retroactively rerun the 455 gate. All five
core design pages are synchronized and cross-reviewed. Full qualification and
promotion remain open.

The last full release diagnostic at31 runs 54,563 tests with six failed modules:
monitoring, peepholer, sys_settrace, pdb, gc_verify and tools. Current focused
checks repair monitoring/tools. Isolated GC method and module pass in three
roles; its full-run failure remains unresolved. See the
[eighth report](eighth-integration-report.md) for exact31 evidence.

Current32 meets the logging timing gate: two independent six-round upstream
campaigns give 0.902351 and 0.898412, every pair below 0.937. Both full 29 screens
complete with stable identities: geometric means 0.966319/golden and 1.190384/
upstream. The four-pair golden repeat confirms generators 1.070556,
coroutines 1.086596 and nbody 1.048799. The other four original losses are flat
or faster in the two-pair screen. See the
[ninth report](ninth-integration-report.md) for complete evidence and limits.
Current PMU/profile collection is being prepared; c142 captures remain historical.
Runtime/build/benchmark/Lib freeze continues during the collection.

The [NULL-only consumer proposal](null-only-consumer-interface.md) and shared
physical transfer contract are independently reviewed but not implemented.
Retained roots, caught exits and yield remain separate capabilities. The
[debugger scope review](debugger-transfer-scope-review.md) distinguishes documented
useful jumps from CPython's permissiveness across active context protocols.
No runtime edits are admitted during measurement/qualification freeze.

## Parallel responsibilities

| Stream | Owner | Current deliverable |
| --- | --- | --- |
| API borrow and semantic scope | Sol 6.1 medium A | Six tracing window reads; independent POST review; documented debugger guarantees versus CPython-specific jump permissiveness |
| Stopped-frame view | Sol 6.1 medium B | Reviewed nonneutral committed-edge view, focused controls and unchanged strict adapters |
| Independent model/source review | Sol 6.1 medium H | Actual POST/borrow diff review; precise shared physical transfer facts and NULL requirements |
| Integration and measurement | Coordinator | Focused native gates and commit, then unchanged broad screen before further debugger work; qualification, current PMU and final logging campaigns |

Workers propose first and independently review each other's contracts. Runtime
file ownership is assigned after interface review. They do not run shared
native builds or measurements. The coordinator owns commits and integration.
Reviewers inspect actual source and measured evidence after each integration.

## Work sequence

1. Collect current counters and profiles for the three confirmed losses:
   generators, coroutines and nbody. Use the matched four-pair calibration,
   retained executable identities and sealed FIFO hot-region protocol. Keep
   per-workload instruction/cycle counts distinct from sample locations; old
   source or profiles do not establish the current cause.
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
