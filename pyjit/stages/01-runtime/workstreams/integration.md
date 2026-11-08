# Coordinator integration

Parent: [runtime integration](../README.md). Owner: top-level coordinator.

The coordinator owns live application, the shared index, dependency-ordered
source commits, merged builds, qualification and progress reports. A worker
handoff is an input to this loop, not the end of the project.

1. Inspect actual source, index, processes and worker state.
2. Choose a finite blocked mechanism from the failed build or a concrete runtime contract.
3. Obtain Sol design/review; fan out approved mechanical work to Luna or a rewriter.
4. Inspect writer targets, run artifact generators with stability guards, and run cheap meaningful gates.
5. Obtain independent actual code/results review; integrate and commit only the accepted delta.
6. Validate the merged tree; select the next failure until linking and startup succeed.

Expensive broad qualification and quiet timing belong to coherent candidates.
Preserve failures and reviewed commits. Do not stage unrelated work or erase
user changes. Stop superseded assignments and report actual running work.

Current state (2026-11-08): runtime `0765e9be47f` is frozen as the
[golden baseline](../../../issues/golden-20261108/README.md). Full release/debug
qualification and measured outcomes at functional `0267d428808` are complete.
The earlier linking/triage loop is finished; its old instructions remain in
Git history. No old triage assignment should be restarted from this page.

The [cold review and next plan](../../../issues/golden-20261108/next-steps.md)
own subsequent design and performance experiments. Keep the golden tag
immutable. The coordinator qualifies each coherent batch and compares it
against this baseline before admission. Exact source/build paths and preserved
local changes are in [HANDOFF](../../../HANDOFF.md).
