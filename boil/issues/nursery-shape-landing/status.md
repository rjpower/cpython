# Qualify the nursery and shape simplification together

- State: active
- Owner: string_design; coordinator integrates; roots_audit reviews
- Updated: 2026-10-31
- Budget: 90 minutes from first native launch; checkpoints at 15, 30 and 65 minutes
- Depends on: [nursery](../gc-nursery-minimum/status.md),
  [shape admission](../shape-admission-reuse/status.md)
- Branch/worktree: boil/nursery-shape-qualification;
  ../boil-nursery-shape-qualification
- Head: e9e10d5b503eaf219797a5db44766eeab6fad4dc
- Phase: fresh O1/Py_DEBUG build running
- Execution: started 16:06:20 UTC; deadline 17:36:20 UTC on 2026-10-31

## Hypothesis

The independently reviewed opt-in nursery and shape-family simplification can
preserve the supported runtime contracts together, without further source
changes or separate full qualification campaigns.

## Experiment

Combine only GC candidate 3a473612157 and interpreter candidate c46f1bef67f on
the current documentation head. Prove the source union, freeze products and run
one applicable [validation batch](../../design/validation.md). Carry each
candidate's source/feedback evidence under its actual identity; refresh native
coverage for the combined source. Include actual opt-in minor execution in
supported modes. Keep the nursery disabled by default.

## Decision rule

Integrate after the applicable combined-source checks and independent review
pass. Stop on an unexpected failure or the budget limit, retain the result and
reject this landing attempt pending a separately bounded correction or remaining
qualification batch. No performance claim or automatic repeat-to-green.

## Now / next

Coordinator independently proved the exact thirteen-file union and 6,903
unchanged tracked entries. Independent review cleared the [finite plan](plan.md)
and its source/product boundaries. The first fresh build is running; checkpoints
are 16:21:20, 16:36:20 and 17:11:20 UTC. There is no combined qualification pass or runtime
landing yet. Coverage/reclamation and old-scan reduction remain separate work.
