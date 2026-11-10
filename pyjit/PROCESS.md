# Development process

Start with the [entrypoint](README.md), [current status](STATUS.md) and
[stage map](ROADMAP.md). Use [HANDOFF](HANDOFF.md) when taking over execution.
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

1. **Propose:** write a short design in `design/` or an experiment directory:
   the problem, the minimum useful change, dependencies, file boundaries and
   decision criteria. Use a timebox where uncertainty warrants one.
2. **Peer review:** an independent reviewer checks semantics, ownership,
   recovery and whether the increment answers the question. Fold findings into
   the design before implementation starts.
3. **Implement:** update affected producers, generated products and consumers
   together within the agreed scope. Ports use idiomatic upstream CPython
   ownership code.
4. **Test:** run the cheapest meaningful checks first, then applicable
   [admission gates](design/validation.md). Keep failures and limits visible.
5. **Post-review:** an independent reviewer inspects the actual diff and the
   executed evidence. Every C review includes a real release-mode compile in
   the release build directory; debug-only builds missed release-only uses of
   deleted getters.
6. **Integrate:** the coordinator commits reviewed work, merges, and validates
   the merged tree. Component reviews do not prove their composition works.
   Call a feature integrated only after that. Label an unbuildable checkpoint
   as such.
   - A landing runs `tools/integrate.sh <tag> <tests>`: regenerate, build
     debug and release, the lint, then the step's own test list plus a fixed
     smoke set on both builds. Breakage in a core area (bytecode, the
     compiler, the value API) shows up in that set; it is not subtle.
  - The full suites (`tools/full.sh`, 25 to 40 minutes for both) run only at
     coherent batch integration points and before the checkpoint recap. Focused
     tests and basic performance checks follow individual landings. A
     full-suite failure is bisected over the batch's commits.

Designs and reviews are short markdown files. Git commits are the identity of
a change: there are no SHA seals, author seals or receipt JSON. Cite a commit
hash. Do not overwrite a review; add a new file for a new round.

Selected architectural goals remain goals. Experiments improve concrete
mechanisms inside them; they do not repeatedly vote on whether an unfinished
universal-value runtime or JIT should exist.

## Model roles

Current user direction (2026-11-10): Astra reviews a broad architecture or group
of designs once; Sol handles component proposal/code review, implementation and
debugging. Do not use Astra in the inner loop. Terra/Luna may handle bounded
mechanical ports and documentation. DeltaBlue is the next ordinary-call target;
incremental mostly-beneficial landings may proceed with documented tradeoffs.
Cycle/layout investigations are secondary to removing structural work.

Current packet direction (2026-11-08): use **Sol 6.1, medium effort** for bulk
work and implementation, with independent proposal/code review and coordinator
integration. This supersedes the older default role allocation for the active
[execution-boundary packet](issues/execution-boundaries-20261108/plan.md).

The earlier default allocation (2026-11-05) replaced the previous provider's names:

- **Terra, medium effort:** implementation, ownership-sensitive ports and finishing.
- **Luna, medium effort:** bounded mechanical ports and documentation sync.
- **Sol:** important design decisions, independent critical review or specific
  debugging problems; do not use it for bulk migrations.
- **Coordinator:** assigns work, reviews results, commits, integrates and
  reports status.

Workers never commit. They leave changes in the working tree and report; the
coordinator reviews and commits.

## Agent briefs

Every brief carries:

- a time budget and a deadline;
- a small-first ladder: the cheapest check first, then larger ones;
- a progress file, updated at least every 30 minutes;
- a scope cut: what to finish first if time runs short.

## Parallel work and communication

Use separate worktrees or explicit nonoverlapping file ownership. Agree shared
interfaces before dependent edits. Independent review and preparation can run
beside implementation; shared builds and quiet timing windows serialize. The
coordinator owns integration and final validation.

Check agents' actual state; a queued message is not a running assignment. Stop
obsolete agents and close assignments before replacing them.

For bulk ports, review a reusable ownership rule and representative examples
once, then assign bounded file groups to cheaper agents or a mechanical
rewriter. Workers report cases outside the rule rather than inventing lifetime
policies. Review the actual batch diff and focused checks before committing.
Cluster the failed merged build into root causes, including shared headers and
generators, and distinguish cascades from independent sites. Regenerate
products from their producers. See the
[porting rules and census](issues/pyvalue-rc-restoration/porting/status.md).

For callback-heavy C ports, the recipe must spell out owner slots, branch and
exit flow, typed transfers and retirement order. Assign one function per packet
unless several functions follow the same mechanical flow. Verify newly named
project APIs against actual declarations. Test controls must be discoverable by
the repository's runner; a passing standalone prototype is reference evidence,
not proof of candidate test discovery or execution.

Update STATUS and the affected workstream at meaningful transitions and keep
the user informed. Give proactive status reports about two hours apart during active work, covering
completed changes, actual running assignments, blockers and the next executable
milestone. Record last/next report times and inspect completion notices before
waiting again. Never describe a completed agent or a timer record as running
background work. Durable scheduled delivery needs an actual scheduler.

Keep about one line per concluded experiment in [WORKLOG](WORKLOG.md); designs,
review iterations, commands and raw results belong to that experiment. Follow
the [documentation rules](DOCUMENTATION.md).
