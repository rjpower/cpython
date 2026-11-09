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

Engineering `4a1f70fcef49bccf210180fa69cfc4fb409a80cd` integrates bounded PRE
cancellation, derived per-named-local save obligations and real-LINE identity.
Different locals restore independently; nested saves of the same local remain
ordered. A repeated current source line preserves its actual phase and PC,
including the later with-exit operation on the header's line. The previous
first-word selection could reenter the scope indefinitely; the bounded probe and
unchanged original fixture now pass. The old arbitrary stack-projection capacity
test is ported to prove one successful 101-argument call after cancellation.

Release/debug each check 117 modules and pass 125 focused tests with one skip.
The renamed frame-model identity/large-window gate passes two tests each. The
last wide gate, before that test-only port, runs 999 tests per profile and still
fails four modules: monitoring, pdb, peepholer and sys_settrace. Trace errors
fall from 65 to 38; 21 old diagnostic assertions and the now-ported capacity
expectation also fail. Pdb retains two failures/two errors. These focused and
wide checks do not establish full qualification; golden remains selected.
The [repair log](qualification-repairs.md) and [fourth report](fourth-integration-report.md)
retain earlier identities, failures and screens.

The latest clean two-pair logging/upstream screen at 4a is 0.838496 (pairs
0.812193 and 0.865650). The separate 4a two-pair golden screen has logging
0.585975, nbody 0.999512 and pickle 1.014001. Do not combine campaigns or
attribute a gain/loss to cold changes. Final two independent six-round parity
campaigns, full suites, API checks and the unchanged broad screen remain open.

The [cold checkpoint review](cold-checkpoint28-review.md) ranks eight actions.
Three parallel implementation streams now address saved-local restoration,
outputs committed before observation, and shared encoded POST ownership proof.
The FOR callback-error probe confirms an unpublished result leak on candidate
and upstream; reproducing upstream's bug is not a compatibility requirement.
Exact physical home mapping and old-owner provenance remain separate facts.
The quiet screen is complete and the source freeze is released for the three
reviewed streams under explicit file/interface ownership. The
[debugger design](../../design/debugger-frame-transfers.md)
distinguishes the implemented PRE rules from these next capabilities.

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

1. Complete the reviewed saved-local action and terminal-output contracts in
   disjoint files. Agree generated declarations and shared cold decoders first;
   no provider advertises POST before its outputs and continuation are committed.
2. Use one generator final-output convention and one constructor ownership
   worklist. Preserve unrelated ordinary generated handlers byte for byte.
   Migrate the finite composed/direct provider census explicitly; do not turn
   the old generic callback marker into a terminal action blindly.
3. Run minimal host generator/constructor checks, then focused release/debug
   frame, observer and generator tests. Post-review actual source and results,
   integrate, and take basic logging/nbody/pickle screens. Repair remaining
   restoration/context/phase cases without weakening target availability.
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
