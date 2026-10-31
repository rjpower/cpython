# Boil the ocean: start here

A performance fork of CPython: register IR, eight-byte NaN-boxed values,
shapes and tracing without reference counts. The upstream base is
`321925c92c9` (2026-10-23). Interpreter improvements precede JIT work;
roughly 2× on obvious arithmetic is design guidance, not a numerical gate.

**Start with the [work index](issues/README.md).** Each issue's `status.md`
owns its current state, next action and landing identity. Queued work is
distinct from running work. Designs and historical receipts do not assign jobs.

| Need | Read |
| --- | --- |
| What is running, next, landed or deferred? | [Work index](issues/README.md), then the relevant issue |
| Current GC/interpreter recommendation and parity gaps | [Current design and plan](design/work/gc-architecture-next.md) |
| Full latest performance results and their limits | [Performance report](design/work/gc-post-cleanup-performance.md) |
| Runtime architecture and subsystem contracts | [Architecture](design/06-architecture-v2.md), notes 07–14 under `design/` |
| How to organize and update documents | [Documentation rules](DOCUMENTATION.md) |
| How to implement, review, qualify and integrate | [Process](PROCESS.md) |
| Why a significant choice was made | The linked design/decision record; new decisions use `decisions/` when needed |
| Outcomes over time | [Worklog](WORKLOG.md); detailed evidence stays with the linked results |

The coordinator owns integration, the index and cross-cutting documents.
Implementation agents own bounded issues and separate worktrees; independent
agents review significant changes. Update issue status when work starts,
blocks, changes direction or lands, and at hourly checkpoints during active work.

For takeover: read the index and relevant issue records, inspect Git/worktree
state and evidence, then check the actual agents and processes. An old pending
gate or future checkpoint in a historical document is not an active assignment.

Builds and retained evidence live under repository-managed `build/` directories.
Integration is `weaver/python-boil-the-ocean` in
`.worktrees/python-boil-the-ocean`; topic work uses `.worktrees/boil-<name>`.
Machine-specific build flags and limitations belong to the actual build receipts,
not assumptions inherited from a handover.

The [previous handover](archive/readme-2026-10-31.md) and
[previous combined process/status record](archive/process-2026-10-31.md)
are historical snapshots. Existing `design/work/` records and their evidence
remain at their original paths; current issues link them. Loom artifacts are
published views of repository documents, with their source and revision identified.
