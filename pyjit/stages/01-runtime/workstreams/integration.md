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

Current state (2026-11-03 17:40): runtime HEAD `6d39221996d`. Debug and release
builds link and run, and 67 shared modules build. The debug suite at 17:31 ran
611 of 619 files: 484 OK, 30 failed, 3 env changed; 48,500 tests, 52 failures.
Two triage agents are fixing the rest. There are no performance results yet.
Commits and next steps are in [HANDOFF](../../../HANDOFF.md).

The next integration packet is the
[PyValue representation packet](../../../design/proposals/pyvalue-representation-packet.md)
(11 steps, accepted after review), which starts after triage and native-boundary
P4. IR v2 follows. The next stage is
[interpreter qualification and measurement](../../02-baseline/README.md).
