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

Third engineering integration `87f13087ed9b97254b983bfb1adef61ae1fcde88`
contains semantic loop/send completion, initialized frame storage, target-described
remote layouts, activation-local source errors and cold LINE handoff. It adds
proved NULL/current-loop debugger recipes, explicit redirect intent and checked
canonical numeric results. Cache format is 64415. Both builds check 117 modules
and pass 320 focused tests, with five skips. All 87 host generator tests pass.
The full release diagnostic ran 54,504 tests: 13 modules failed, reporting
126 failures and 99 errors. This is not qualified or promoted.

The matched two-pair logging/upstream screen is **0.845543**. A four-pair repeat
versus golden has logging 0.589521, nbody 1.056203 and pickle 1.010377. Warmed
nbody counters show instructions 1.033246, cycles 1.053810, branches 1.058609 and
branch misses 0.999944. The extra instruction gap remains; no component speedup
is attributed to numeric publication. The prior seven-workload screen repaired
material unpack, float and deepcopy losses. The complete broad rerun, final
qualification and two six-round parity campaigns remain open. Read the
[third report](third-integration-report.md) for protocols and retained failures;
the [second](second-integration-report.md) and [first](integration-report.md)
reports retain earlier source-specific measurements.

Qualification repair `37a67b9d155` now integrates fused source coverage,
exact protected real-LINE identity and format/layout/frozen-embedding consumers.
Cache magic is 64416. Both profiles check 117 modules and pass 1,027 tests in
eight affected modules; 2,152 valid library files compile without rejection.
The [repair log](qualification-repairs.md) records source/build identities and
qualification limits. No elapsed result is attributed to this cold repair.

Engineering26 `9f677103e00` now integrates explicit entry and its stdlib
consumers, request-local context/owner plans, payload identity and a narrow legacy
projection preserving modern STOP(None). Both builds check 117 modules and pass 131
focused tests; corrected tracing profiling passes 28; 2,154 valid files compile.
The 1,572-test wide gate still fails four actual modules plus an obsolete root
profiling selector (subsequently corrected). No full qualification is claimed.

Integrated `192cfbe8aa0` completes the reviewed LINE invalidation repair: preserve LINE
handoffs when only INSTRUCTION changes, and reset masks for reused LINE identities.
B designs cancellation and saved-local/context restoration separately from
incoming target requirements. The [debugger transfer design](../../design/debugger-frame-transfers.md)
records implemented limits. Remote/GDB source display, general caught exits,
suspension and semantic owner retirement remain open. Ordinary return does not
already clear scratch. The clean fourth engineering screen at 192 has logging/upstream 0.817460
(two balanced pairs). Four-pair golden repeats are logging 0.592805, nbody
1.000054 and pickle 1.017928; the earlier nbody loss does not repeat. The stable
0/2/4 census differs only by 40,007 fewer entered handlers per outer loop, with
nonlinear POWER adaptive retries in both roles. This is not native cost
attribution. All three builds identify 192; stats smoke passes 146. B implements
peer-admitted bounded PRE cancellation and saved-local context analysis; A
proposes scoped cold POST phases. Final qualification and parity campaigns stay
open; no cold-repair speedup is inferred. The [fourth report](fourth-integration-report.md)
records the current screen and retained limitations.

## Parallel responsibilities

| Stream | Owner | Initial deliverable |
| --- | --- | --- |
| Observation and source boundaries | Sol 6.1 medium A | Source coverage, semantic completion events and immediate debugger entry; cross-review of context owners |
| Debugger context and cold contracts | Sol 6.1 medium B | Real-phase identity, protected/suspended context recipes and independent source review |
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
