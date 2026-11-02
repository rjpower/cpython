# Documentation rules

Treat this directory as a wiki. A reader should reach the selected design,
current work and next steps from [README](README.md) without reconstructing a
conversation or a chronology of receipt hashes.

| Page | Owns |
| --- | --- |
| `README.md` | Orientation, current milestone and navigation |
| `GOAL.md` | Standing instructions and compaction/restart recovery |
| `PROCESS.md` | Work cycle, extension migration and integration policy |
| `design/runtime-architecture.md` | Overall selected design and detail links |
| Other `design/` pages | Durable subsystem contracts; label proposals and history |
| `design/validation.md` | Testing and feature-admission rules |
| `issues/README.md` | Current board, about one row per experiment |
| Experiment `status.md` | Stage, owner, hypothesis, dependencies, evidence and next action |
| Experiment design/reviews/results | Detailed design, reviews and execution evidence |
| `WORKLOG.md` | About one short line per concluded experiment, linked to its record |

Use existing issue directories and stable descriptive child IDs for independent
increments. Do not create a public document for every agent message, command,
receipt or approval event. Small increments may keep everything in status.md.
The [template](issues/_template/status.md) is a guide, not a demand for empty files.

Rewrite current status when it changes; retain chronology in experiment results
or Git. Update one authoritative record and link it from the board. Distinguish
source review, build, executed runtime tests, commit, integration and measurement.
Include exact source/image identities in results, not every navigation paragraph.

Preserve failures and old evidence. Label superseded plans as historical and
remove them from current assignments. The pre-reset
[worklog](archive/WORKLOG-before-wiki-reset-20261102.md) and
[board](archive/work-board-before-wiki-reset-20261102.md) are preserved verbatim;
their links and assignments are historical, not execution instructions. Do not
rewrite unrelated user work while reorganizing records.

Loom mirrors the wiki for discussion; repository pages own design and execution
state. The `goal` artifact mirrors [GOAL](GOAL.md). Published summaries link to
source pages. After compaction, read GOAL, README, PROCESS and the board, then
inspect actual state instead of reviving an old agent or prepared script.
