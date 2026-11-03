# Project wiki and coordinator reset

- Stage: accepted documentation after independent review
- Owner: coordinator; reviewers: fresh wiki_audit and state_audit agents
- Goal: make selected design, actual state, process and next steps readable from
  one entrypoint, without reconstructing historical checkpoints.
- Scope: documentation, Loom goal/status and stale coordinator monitor; no runtime
  source edits or native execution.

## Design

Keep existing wiki paths. README is the entrypoint, GOAL records standing user
direction, PROCESS owns compatibility and the increment cycle, architecture
links to IR/interpreter details, validation owns admission, and the board has
one row per experiment. Detail stays in experiment directories; WORKLOG has one
short concluded-outcome line per experiment. Archive superseded verbose records
verbatim and preserve unrelated user work.

## Findings and implementation

The audit found stale tracing-GC architecture, obsolete tracing-only gates,
stopped agent assignments, missing recent source acceptances and a 1,954-line
checkpoint transcript. The state audit found uncommitted restoration source and
no restored build/runtime/timing; 28 recent source/context hashes match reviews.
Previous agents were interrupted; two fresh audit agents replaced them.

Evidence: [wiki audit](../../../build/wiki-reset-20261102-v1/wiki-audit.md),
[state audit](../../../build/wiki-reset-20261102-v1/state-audit.md), and
[verbatim archive manifest](../../../build/wiki-reset-20261102-v1/archives.json).

## Checks and next step

Fresh reviews accepted content/process and actual-state accuracy after two
wording fixes. Link/archive/protected-file/diff checks pass. The Loom goal was
read back exactly. See [results](results.md) and [review](review.md). No runtime
source changed or native suite ran for this documentation increment.

Next: the coordinator assigns fresh owners and commits/integrates reviewed
source increments, validating their merged result before broadening work.
