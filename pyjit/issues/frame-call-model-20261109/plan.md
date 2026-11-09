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

The first reviewed engineering increment is committed at runtime `6ef45e391aa`.
It is not qualified or promoted. Release checks 117 modules; the 22-module focus
runs 2,694 tests with 18 modules passing. Subsequent ownership/observer checks
pass GC, weakref, finalization, UUID, frame calls and source-boundary modules.
Useful debugger resumes and one tight-loop source event case remain failing.
Literal fixtures, source context/positions and observed delegation crashes are
repaired. Full release/debug qualification has not run this packet.

The first balanced two-round logging/upstream screen is **1.003646**, near parity
but above the gate. Logging/golden is **0.742426**; unpack/golden **0.833936**.
Other broad losses remain, notably deepcopy_memo **1.092133** and nbody
**1.055594**; the former spectral gain is absent. Warmed logging counters are
1.08061× upstream instructions and 1.01471× cycles. See the
[first integration report](integration-report.md) for exact protocols, failures,
profiles and limits. Do not infer component attribution from the combined image.

The independently reviewed [semantic completion proposal](semantic-completion-proposal.md)
is admitted for runtime/compiler implementation: loop/send outcomes, generator
resumption and physically compact static programs. The
[debugger proposal](debugger-owner-recipe-proposal.md) starts with real loop
anchors and nullable call cells. A measured frame-tail initialization cost
motivates a separate arena-invariant design; it is not yet implementation
admission. Shared-owner and protected/suspended reconstruction work remains open.
The [progress](progress.md) and [latest source review](source-origin-post-review.md)
record current evidence and unresolved gates.

## Parallel responsibilities

| Stream | Owner | Initial deliverable |
| --- | --- | --- |
| Frame/call ABI | Sol 6.1 medium worker | Concrete call-layout, direct binding, ownership/lifetime and lazy argument-container proposal |
| Executable IR and positions | Independent Sol 6.1 medium worker | Compact instruction/CFG and logical-site mapping proposal, plus cold review of the call model |
| Integration and measurement | Coordinator | Resolve shared interfaces, review proposals, run native builds/checks, integrate and measure the merged model |

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
3. Build release first. Run the smallest meaningful semantic/observer/IR tests,
   then logging, spectral and unpack controls; add the other repeated broad
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
