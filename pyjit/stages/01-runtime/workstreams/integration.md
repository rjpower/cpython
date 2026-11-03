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
Preserve failures and sealed receipts. Do not stage unrelated work or erase
user changes. Stop superseded assignments and report actual running work.

The [current build](../../../STATUS.md) remains unlinked. Exact source/worktree
identities, protected files and the first commands to run are in
[HANDOFF](../../../HANDOFF.md). The next stage is
[interpreter qualification and measurement](../../02-baseline/README.md).
