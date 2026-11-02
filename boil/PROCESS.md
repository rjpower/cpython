# Development process

Start with the [entrypoint](README.md) and [work board](issues/README.md).
The coordinator carries the project through implementation, integration and
measured outcomes, maintains current state, and updates the user.

## Rebuild policy

This is a Python rebuild, including rebuilding and porting extensions. Hard
upstream source/binary compatibility is not a gate. Do not ask the user again
to approve backward-compatibility breaks within this scope. Use judgment to
make ports possible and incremental: owning, fallible object bridges support
ordinary source ports, followed by the opaque extension API. Update affected
core and bundled callers and document required external edits. Preserve Python
behavior, ownership and useful errors.

Existing CPython object layouts, C APIs and JIT patterns are inputs to assess,
not designs we must preserve. Replace poor patterns with reviewed alternatives.
Keep one selected runtime design; avoid permanent switches and duplicate
representations merely to preserve optionality.

## One useful increment

1. **Design:** state the problem or hypothesis, minimum useful change,
   dependencies, owner, file boundaries and decision criteria in its experiment
   directory. Use a timebox where uncertainty warrants one.
2. **Review the design:** an independent reviewer checks semantics, ownership,
   recovery and whether the increment answers the question. Resolve findings.
3. **Implement:** update affected producers, generated products and consumers
   together within the agreed scope.
4. **Test:** run the cheapest meaningful checks first, then applicable
   [admission gates](design/validation.md). Keep failures and limits visible.
5. **Review code and results:** independently inspect the actual diff and
   executed evidence. Revise and rerun affected checks when needed.
6. **Commit:** record the useful increment or reject it with its evidence.
   Commit dependent source checkpoints promptly; label an unbuildable checkpoint
   as such rather than claiming a runnable feature.
7. **Integrate:** the coordinator merges reviewed commits, resolves shared
   changes and validates the merged tree. Component reviews do not prove their
   composition works. Only then call a feature integrated or admitted.

Selected architectural goals remain goals. Experiments improve concrete
mechanisms inside them; they do not repeatedly vote on whether an unfinished
universal-value runtime or JIT should exist.

## Parallel work and communication

Use separate worktrees or explicit nonoverlapping file ownership. Agree shared
interfaces before dependent edits. Independent review and preparation can run
beside implementation; shared builds and quiet timing windows serialize. The
coordinator owns integration and final validation.

Give agents concrete work and completion conditions. Check their actual state;
a queued message is not a running assignment. Stop obsolete agents and close
assignments before replacing them. A restart or old plan does not renew work.

For bulk ports, review a reusable ownership rule and representative examples
once, then assign bounded file groups to cheaper agents or a mechanical rewriter.
Each group records its rule, source identity, file ownership, exceptions and
results. Regenerate products from their producers. Workers must report cases
outside the rule rather than inventing lifetime policies. Review the actual
batch diff and focused checks before checkpointing; the coordinator validates
composition with shared builds. Full qualification belongs to coherent merged
candidates, not every repeated call-site edit. See the
[porting rules and census](issues/pyvalue-rc-restoration/porting/status.md).

Use Luna for compiler-log analysis, identifying repeated patterns and applying
approved bulk edits. Use Sol 6.1 for solutions and design review. Cluster the
actual failed merged build into root causes, including shared headers and
generators; distinguish cascades from independent sites. Each design produces
bounded implementation packets with file ownership, rule, dependencies and
acceptance checks. The coordinator schedules them, integrates reviewed results,
rebuilds, and repeats until the selected runtime builds and runs. Do not give
Luna open design work or stop when a worker finishes its current packet.

Update the board at meaningful transitions and keep the user informed. Give
proactive status reports about two hours apart during active work, covering
completed changes, actual running assignments, blockers and the next executable
milestone. Record last/next report times and inspect completion notices before
waiting again. Never describe a completed agent or a timer record as running
background work. Durable scheduled delivery needs an actual scheduler.

Keep about one line per concluded experiment in [WORKLOG](WORKLOG.md); designs,
review iterations, commands and raw results belong to that experiment. Follow
the [documentation rules](DOCUMENTATION.md).
