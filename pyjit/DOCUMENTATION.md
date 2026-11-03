# Documentation ownership

Use a hierarchy of decisions and evidence. Readers should not need an experiment
chronology to answer a project-level question.

| Level | Authoritative pages | Contents and owner |
| --- | --- | --- |
| Project | README, STATUS, ROADMAP | Outcome, critical path, stage states and next milestone; coordinator |
| Stage | stages/*/README.md | Deliverable, dependencies, entry/exit gates and workstream links; coordinator/stage owner |
| Workstream | Stage workstream pages | Current packet, stage of work, blocker, evidence and next gate; workstream owner |
| Experiment | issues/* and retained build experiment directories | Exact design, review, implementation, commands, failures, results and decision; assigned worker/reviewer |
| Durable contracts | design/*, GOAL, PROCESS | Selected architecture, standing direction and operating rules |
| Handoff | HANDOFF | Exact operational snapshot, pending artifacts, hazards and next execution sequence; coordinator |
| Log | WORKLOG | About one short line per concluded experiment; detail stays with the experiment |

STATUS is the current aggregate authority. Do not copy its counts and assignments
into every navigation page. Stage pages link to it. A workstream owns its local
next step and links a bounded packet; an experiment owns the detailed evidence.
Update the relevant parent when an outcome changes. Archive or Git retains the
chronology; do not keep completed agents listed as running.

Distinguish proposed, reviewed, implemented, tested, committed, integrated,
qualified and measured. State what was actually executed and on which runtime.
An author template, reference test or C compile is not a candidate runtime pass.
Stages close through their exit evidence, not a subjective percentage.

Keep exact identities in experiment records and HANDOFF rather than the CEO
view. Before compaction or a coordinator change, update HANDOFF and reconcile
it with actual Git, processes, build outputs and worker states. A prepared
script is not an executed action. A report due-time is not a scheduler.

The former documentation root was renamed from boil to pyjit. There is one
canonical entrypoint, with no old-directory alias. Preserve archived/history
files, raw results, sealed reviews and their original paths inside provenance.
Older scripts and immutable artifacts may still name the old directory; adapt
an unsealed launcher or new copy rather than changing a sealed receipt. Branch,
ABI, schema and build-directory names are separate identities and retain their
recorded names.

Loom mirrors project pages for discussion; the repository owns execution state.
Its goal artifact mirrors GOAL. Preserve older revisions when updating mirrors.
See [process](PROCESS.md), [handoff](HANDOFF.md) and [current status](STATUS.md).
