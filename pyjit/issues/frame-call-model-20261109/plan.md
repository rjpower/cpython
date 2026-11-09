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

Engineering `c1424be11db46b5d9e2aca84bcc96ddad6a7ea62` integrates terminal
observer output commits and saved-local restoration on the existing frame ABI.
Generated terminal providers publish final homes and continuation before a
callback; detached owners remain in C through retirement. Activation-owned
scopes identify the actual frame, site, phase, edge and redirect generation.
Restoration uses explicit source-home actions, preserves genuinely unbound
locals, and rejects targets that would read those locals unchecked. Physical
replacement values never resurrect an old saved-owner obligation.

Release/debug builds check 117 modules. Debug passes 363 tests across six core
modules, with one skip; both profiles pass 64 additional observer/frame tests.
The 22 selected release ownership/diagnostic tests pass. Owner probes leave no
survivors after collection, including the formerly stranded FOR result on a
callback error. The constructor accepts 2,156 valid library files with no new
rejections. The current release wide check runs 999 tests with 14 skips and
still fails four modules: monitoring (one error), peepholer (three errors),
sys_settrace (38 errors), and pdb (two failures/two errors). The 21 obsolete
diagnostic mismatches are repaired. These checks do not establish full
qualification; golden remains selected. The [repair log](qualification-repairs.md)
retains exact receipts and failed attempts.

The current clean two-pair logging/upstream screen is 0.936868 (pairs 0.913258
and 0.961087). The separate golden screen has logging 0.653035, nbody 1.041296
and pickle 1.026258. Four-pair repeats give nbody 1.019904 and pickle 1.021510.
Warmed PMU counts show logging/upstream cycles 0.918210 and instructions 1.044970;
nbody/golden cycles 1.040876 and instructions 1.033309. The [sixth report](sixth-integration-report.md)
owns current evidence and limits. Do not combine campaigns or attribute changes
to cold code without an experiment. Final parity campaigns, full suites, API
checks and the unchanged broad screen remain open.

The [next independently reviewed packet](next-provider-stopped-state-peer-review.md)
uses one request-owned stopped-state view for exact committed source edges and
the existing restoration Plan. It will separate physical continuation, normalized
source landing, reported monitoring event and actual owner edge. Extend the
terminal contract to jumps and iterator retirement; keep legacy uncommitted
source hooks unauthorized. The first consumer scope has inherited exception
context, a save-empty target, no incoming target scratch and no token-creating
transfer. Yield cancellation is a separately gated pre-suspension adapter;
retained protocol roots and caught exits require their own proofs. The basic
performance/profile capture is complete. The producer stream is admitted;
the consumer stream first investigates actual each-LINE callback stops: a
native compile inventory finds no source-only records in any of its six
unchanged fixtures. Event ordinals alone do not establish SOURCE_POST.
A bounded 84-row probe confirms the repeated list/set/dict stops occur inside
backward JUMP callbacks. The [amended consumer](stopped-state-actual-jump-amendment.md)
is now admitted: a positively declared owner-neutral committed edge supplies
the shared stopped view and restoration Plan. Public LINE spelling grants no
authority; SOURCE_POST and yield consumers remain separate.

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
