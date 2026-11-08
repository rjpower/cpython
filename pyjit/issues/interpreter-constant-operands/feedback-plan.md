# First native feedback: held for a frozen source review

Prepared 2026-10-31. This plan is **held**, not an execution receipt. The
coordinator must bind the accepted candidate commit, exact case inventory,
helpers and source/product origins before activating it. The host-only
prototype grant does not authorize a target interpreter or build.

## Question and bound

Does the shared operand-source representation survive the actual compiler,
specializer, observer and unwinder, and does O3 remove the nominated work?
Use one sixty-minute feedback batch from activation, within the current
implementation dispatch. Stop at the first unexpected failure, preserve it
and review a bounded correction. An expired batch requires a new decision.
This is first feedback, not qualification or a timing comparison.

Build a fresh GCC O1/Py_DEBUG interpreter first: the register CFG verifier
runs after each pass and after physical slot assignment. Its absence in O3
makes debug feedback useful before inspecting optimized native emission.
Then build GCC O3 for the actual code-generation question. Use the existing
isolated configure/build and gate/worker helper bodies, shared slots and
per-build locks; do not rebuild serving or qualification products.

## Ordered checks

1. Freeze the source and review the complete diff, generated relation and
   host receipts. Run applicable cheap traversal/effect checks before builds.
2. Configure debug with `CC=gcc --with-pydebug 'CFLAGS=-O1 -g'` and build `all`.
   Preserve effective flags, module/import summary and linked root/inline
   audits. Maximum twenty minutes, clamped to the remaining batch budget.
3. Run all actual `test_ir.SmallRhsTest` cases OFF/N0 with seed zero, followed
   by all `test_ir` cases. Bind their discovered IDs rather than estimating a
   test count. Record actual executing workers, modes, roots and exact skips.
   Preserve real behavior, alias/error, entry-zero and code-word controls.
4. Run `test_generated_cases`, `test_dis`, `test_code`, `test_sys_settrace` and
   `test_monitoring` under the same debug source/products and existing worker
   identity protocol. Changed disassembly offsets may need a reviewed golden
   correction after the first actual failure; do not drop those checks.
5. Configure fresh O3 with `CC=gcc`, build `all`, and run the entire new
   `SmallRhsTest` selection. Maximum twenty minutes for the build, clamped to
   the remaining budget. Record source, target library and native identities
   separately for both builds.
6. Inspect bounded native handler ranges and actual emitted words. Require
   removal of the RHS slot load/admission and separate constant dispatch at
   an admitted recurrence, while dynamic-input checks, overflow and generic
   error paths remain. Compare ordinary handler bodies and evaluator size;
   source-level guard absence alone is insufficient evidence.

Use the current instrumented worker and root-counter validation, finite
process-group timeouts and post-gate cleanup verification. Unwind fallback
must be zero. Report-only unpinned diagnostics retain that meaning. Omit the
stress environment variable for N0; never infer a mode from its launch label.
The activated manifest owns literal commands and per-row timeouts.

## Decision

Accept a functioning representation for a separately bounded performance
decision only when the actual behavior and emitted-work gates pass. Reject
or correct a candidate whose observer/fallback representation is incomplete,
whose nominated work survives, or whose code growth defeats the selected cut.
Host generator passes do not stand in for native CFG/observer execution.

The current monitoring control observes warmed specialization followed by
instrumented generic execution and restoration. Wrappers pause adaptive
counters; do not describe it as proof of an executed wrapped specialized miss
without an actual additional witness. The source review still checks every
form-preserving fallback/restoration entry.

Timing requires a preselected eligible recurrence consumer, matched builds
and origins, raw instruction/elapsed vectors and unrelated controls. The old
invariant-local arithmetic diagnostics are not that consumer. A useful
candidate then joins one coherent applicable qualification batch; this plan
does not claim runtime landing or grant an automatic benchmark campaign.
