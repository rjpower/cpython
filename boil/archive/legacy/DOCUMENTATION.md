# Documentation and work tracking

Adopted 2026-10-31. Start with the [work index](issues/README.md).

## The rule

Each substantial work item has one authoritative current record:
`boil/issues/<stable-name>/status.md`. It answers **what is true now, who owns
it, what happens next, and what closes it**. Rewrite it as state changes;
put chronology and detailed results elsewhere. Keep it short enough to read
without reconstructing a conversation, ordinarily about one screen.

The index is a navigation view of those records. If it disagrees with an issue,
correct the index from the issue and its evidence. Global documents link current
work rather than keeping their own pending-gate and agent-assignment lists.

This addresses observed drift: the old README named both `aee46` and `75e8`
as latest, reported both completed and pending union measurements, and retained
an older performance number as current. PROCESS repeated that state alongside
obsolete assignments. More folders alone would not solve duplicated authority.

## Put each kind of information in one place

| Document | Owns | Update rule |
| --- | --- | --- |
| `README.md` | Orientation and navigation | When entry points or project scope change |
| `issues/README.md` | Active/queued/recently closed/deferred index | With issue transitions; link older closed entries rather than growing indefinitely |
| `issues/<id>/status.md` | Current state, ownership, dependencies, next action, completion/outcome links | At material transitions and active-work checkpoints |
| `design/` | Cross-cutting architecture, invariants and accepted interfaces | When the implemented contract changes; distinguish proposed extensions |
| Issue `design.md` / `plan.md` | Proposed change and implementation sequence | As decisions change; split only when either needs its own substantial document |
| Issue `reviews/` | Findings and dispositions for a named revision | Preserve each review round and link its resolution |
| Issue `results.md` | What was built/found, deviations, qualification scope, measurements and evidence index | During execution; freeze the completed result, add corrections explicitly |
| `decisions/` | Short records of significant shared choices and their rationale | Preserve accepted decisions; link their replacement when superseded |
| `PROCESS.md` | Stable procedures, ownership rules and gates | When procedure changes, not at each landing |
| `WORKLOG.md` | Short dated milestones, decisions and outcomes | Append an event with links, not another copy of gate logs |
| `archive/` | Clearly dated historical snapshots | Preserve history without treating it as current instruction |

Only `status.md` is required for a new item. A small change can keep its scope,
plan and result there. For larger work, link separate files before the status
turns into a design or campaign diary. Existing useful designs/results remain
at their current paths and are linked, not copied into the issue folder.

For new work, the optional layout is:

```text
boil/issues/gc-common-young-space/
  status.md
  design.md       # if no existing authoritative design covers it
  plan.md         # if the implementation sequence needs a separate document
  results.md      # durable outcome and evidence links
  reviews/        # immutable review rounds, when required
```

Names describe an outcome or design question, not an agent, clock time, gate
number or temporary experiment label. The directory name is the stable ID.
Keep its path when it closes. Split a child issue only for independently owned
work with a useful completion condition; link dependencies by issue ID.

## States and outcomes

Use these states in the issue and index:

| State | Meaning |
| --- | --- |
| `queued` | Selected or proposed work; no owner is executing it yet |
| `active` | An owner is executing a stated next action |
| `blocked` | Active work cannot proceed; name the dependency and unblock action |
| `review` | The prepared change/result awaits named review or integration |
| `deferred` | Intentionally outside the current work; record a resume condition |
| `closed` | The completion condition is met or the work is explicitly stopped |

A closed item also names its outcome: `landed`, `reported`, `abandoned` or
`superseded`. This distinguishes a completed investigation from a runtime merge.
Deferred work is not abandoned. Superseded work links its replacement; abandoned
work records why it stopped. No abandoned item is invented to fill a category.

An implementation is **landed** only when it is integrated on the primary
branch and applicable qualification/postmerge obligations are accepted. Record
both the qualified source and integration commit, and link the evidence and
support limits. A topic commit, passing component test, or pushed branch alone
does not establish landing. A status transition neither authorizes new work nor
waives a gate. These rules add no permission or approval step.

## Keeping execution legible

Use the [template](issues/_template/status.md). Record one accountable owner;
name the active agent, branch/worktree and source head when assigned. Clearly
distinguish implementation, validation and integration phases within active work.
List dependencies, one concrete next action, a blocker/resume condition when
needed, and observable completion criteria. Link design, plan, review, results
and follow-ups instead of pasting them into status.

The implementation owner updates its issue at start, handoff, failure/block,
source freeze and review readiness. The coordinator updates integration identity,
the index and cross-issue dependencies. At roughly hourly checkpoints during
ongoing work, verify agent/process state, reconcile the issue records, and send
a short user summary: changes, current work, blockers and next actions. Summaries
link the same records; they do not become another authoritative queue. An idle
item does not need an hourly timestamp-only edit.

Before starting a gate, record the finite question/coverage, source/build/mode,
resource admission and stopping rule in the plan/results. Results link raw
receipts, original failures, accepted carry-forward bindings and cleanup state.
Large outputs stay in established project storage; the committed result identifies
their manifest/hash and retention location. Audit links and source/product
bindings before retiring evidence worktrees. This changes document placement,
not validation requirements; [PROCESS](PROCESS.md) governs those.

Review helpers currently write fixed default filenames beside their input.
Pass the design to design review and the linked results record to landing review;
the latter must identify the governing design. Preserve each output under a unique
round name before another invocation can overwrite it. Link the actual paths in
status; this layout does not claim the helpers already implement round numbering.

An enduring design describes the current contract and clearly labels proposed
changes. Completed implementation details go in results. For consequential
cross-cutting choices, a short decision record states context, decision,
consequences, status and replacement link. Routine fixes need no separate ADR.
Retain useful superseded/abandoned decisions with explicit labels rather than
leaving them as apparent instructions or deleting their rationale.

## Migration and publishing

The initial index covers the landed GC union, the two next performance tracks,
and explicitly deferred H1/H2/foreign-lifecycle work. A separate follow-up group
retains value/native interface closure, verification orchestration, root/checker
reconciliation and deferred H3 tooling consumers. These are substantial threads,
not retrospective issues for every rung, run or reviewer message. Add other
legacy work when scheduled or needed to explain a dependency; the existing
worklog and snapshots remain searchable history.

The old README and combined PROCESS are archived. Current PROCESS retains the
workflow and gates, with the existing finite-campaign policy stated explicitly
instead of the contradictory automatic ten-repeat/daily-repeat instructions.
Existing `design/work/` designs, reviews, evidence capsules and build receipts
stay in place. When a legacy file is touched, label its role and link its current
issue. Move a document only with an inbound-link and evidence-reference audit.

Repository documents are authoritative. Loom artifacts are published views:
identify source path and Git revision, and refresh them after the corresponding
commit. A GitHub issue/board can later mirror this index if useful; keep a single
status authority. Start with Markdown and ordinary review rather than building
a tracking service or maintaining two queues.

## Basis

This applies the reader-purpose separation used by
[Diátaxis](https://diataxis.fr/): explanation and reference serve different
needs from instructions. Our issue lifecycle is a project choice, not part of
that framework. Short, versioned decision records with retained superseded
decisions follow Michael Nygard's
[original ADR proposal](https://cognitect.com/blog/2011/11/15/documenting-architecture-decisions).
