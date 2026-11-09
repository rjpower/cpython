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

Second engineering integration `3e5cbdb11236ec91f79d8fabc0ceb7ad2ee674cb`
implements semantic loop/send completion, initialized frame storage,
target-described remote layouts, activation-local source errors, epoch-based
cold LINE handoff and a coherent RESUME source barrier. Release/debug each
check 117 modules and pass 207 focused tests. All 2,152 valid library files
compile without rejection; 11 intended syntax fixtures remain. Cache format
is 64415. This source is not fully qualified or promoted.

The first matched two-round logging/upstream screen is **0.974490**; both pairs
are below 1.0. The broad golden screen repairs prior material unpack, float and
deepcopy regressions. Pickle retains about 0.7% overhead. Nbody's loss repeats
at **1.080373** over four pairs; warmed PMU captures show instructions 1.031942,
cycles 1.080012, branches 1.056085 and branch misses 1.000104 versus golden.
Read the [second report](second-integration-report.md) for exact protocols,
profile limits and retained failures. Final full qualification and the two
six-round parity campaigns remain required. The
[first report](integration-report.md) preserves the earlier 6ef experiment.

Next-source work is debugger reconstruction and numeric publication. The
[concrete debugger plan](debugger-recipe-implementation-plan.md) implements
proved NULL requirements before a surviving dynamic FOR anchor. The
[independent numeric review](numeric-hotpath-independent-review.md) admits a
minimal local-first category-order experiment; its benefit remains unmeasured.
The [canonical-result proposal](numeric-canonical-result-proposal.md) requires
constructor and full-result-flow proof before bypassing redundant publication.
Remaining standalone owner retirement requires explicit semantic transitions;
ordinary return does not already clear scratch. Protected/suspended debugger
recipes and remote/GDB active source-cursor display remain open. Six updated
canonical design diagrams render and were inspected.

## Parallel responsibilities

| Stream | Owner | Initial deliverable |
| --- | --- | --- |
| Numeric publication and architectural review | Sol 6.1 medium A | Reviewed local-first store experiment, checked constructor proof, cross-review of debugger owners |
| Debugger owners and cold contracts | Sol 6.1 medium B | NULL/active-loop recipes, safe redirect application, cross-review of numeric changes |
| Integration and measurement | Coordinator | Native gates, coherent commits, logging/broad measurements, profiling, canonical/Loom reports |

Workers propose first and independently review each other's contracts. Runtime
file ownership is assigned after interface review. They do not run shared
native builds or measurements. The coordinator owns commits and integration.
Reviewers inspect actual source and measured evidence after each integration.

## Work sequence

1. Define frame storage, call-site descriptors, argument ownership, ordinary
   and exceptional completion, and logical position transitions as one model.
   Inventory consumers before changing them. Compare the removable work with
   the measured 5,617 extra instructions per logging outer iteration.
2. Implement the reviewed call and executable-position contracts in parallel
   where file ownership is disjoint. Regenerate all producers/consumers and
   cache metadata together. Eliminate the previous packet's ad hoc execution
   variants where the model makes them unnecessary.
3. Build release first. Run the smallest meaningful semantic/observer/IR tests
   and compile the library corpus without importing its modules, collecting
   constructor rejections together. Then run logging, spectral and unpack
   controls; add the other repeated broad
   regressions at the merged integration. Review compiled work and counter
   evidence when measurements leave a gap.
4. Iterate on measured structural costs until logging reaches parity and broad
   regressions are repaired. New designs receive peer review; isolated opcode
   edits do not trigger whole suites. Do not declare success from static work
   counts, sampling shares or one favorable timing pair.
5. Qualify the coherent source in release/debug, run API checks and broad
   performance, then the two parity campaigns. Obtain a fresh cold review and
   update the canonical IR/interpreter/frame designs and documentation chain.

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
