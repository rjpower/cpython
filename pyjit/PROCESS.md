# Development process

Start with the [goal](GOAL.md), [active plan](issues/interpreter-performance56/plan.md),
[status](STATUS.md) and [stage map](ROADMAP.md). Use [HANDOFF](HANDOFF.md) when
taking over execution. The coordinator owns integration, merged validation,
measurement and current project state.

The active stage combines value transport, borrowing, calls and specialization.
Astra reviews the architecture as a whole; Sol workers implement and peer-review
bounded components. Root owns integration and native evidence. The active plan
selects which frozen model55 pieces are prerequisites and keeps proposed
contracts separate from selected architecture.

## Rebuild policy

This project rebuilds Python and ports extensions. Hard upstream source or
binary compatibility is not an admission gate. Owning, fallible object bridges
support incremental source ports; update affected callers and document external
edits. Preserve Python behavior, explicit ownership and useful errors.

## An increment

1. **Propose:** describe the problem, change, dependencies, file boundaries and
   decision criteria. Bound uncertain work with a timebox.
2. **Peer review:** independently check semantics, ownership, recovery and the
   proposed evidence. Resolve findings before implementation.
3. **Implement:** update affected producers, generated products and consumers
   together. Follow existing CPython ownership conventions.
4. **Test:** start with focused checks and apply the relevant
   [admission gates](design/validation.md). Record failures and limitations.
5. **Post-review:** independently inspect the actual diff and executed evidence.
   C review includes a release-mode compile.
6. **Integrate and measure:** the coordinator validates the merged tree and its
   composition. Run full diagnostics at coherent checkpoints, not for every
   isolated change. A component pass alone does not establish integration.

Keep designs and reviews concise. Distinguish proposed, reviewed, implemented,
tested, integrated, qualified and measured work. Use commit identities and
retain evidence for decisions. See [documentation ownership](DOCUMENTATION.md).

## Coordination

Astra reviews grouped architectural options. Sol handles component proposals,
implementation and independent review; Terra/Luna handle documentation and
bounded mechanical work. The coordinator owns integration and measurement.

Give each worker a bounded scope, deadline, small-first sequence and progress
file. Use separate worktrees or nonoverlapping file ownership, and agree shared
interfaces before dependent edits. Serialize shared builds and quiet timing
windows. Workers leave changes for coordinator review and integration. Update
status and the affected plan at meaningful transitions.

For callback-heavy changes, describe owner slots, branch and exit flow, transfers
and retirement order. Tests must be discoverable and executed on the candidate
runtime. A standalone prototype is reference evidence, not a candidate pass.
