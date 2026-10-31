# H2 root maps

- State: deferred
- Updated: 2026-10-31
- Owner: unassigned
- Dependencies: a scheduled usable liveness/root-map consumer
- Branch/head: archived `boil/h2-root-effects` at `b3e6847348d`
- Worktree: retired; source and selected evidence retained

## Now

The inert scalar decoder `5df33656912` and parser correction `b3e6847348d`
remain unmerged. The frozen decoder declares 27 supported pristine handlers
and 126 unsupported/unresolved handlers. Successful decoding describes only
its stated temporary-slot inputs and outputs; it is not complete semantic
effects, CFG/liveness analysis, purity or a runtime root map.

No active consumer, runtime map lookup, slot-clearing retirement or weakened
root coverage follows from this work. Existing native tests support the inert
slice at their recorded identities. The original release parser failure and
the corrected test-only bridge remain distinct; their counts are not new
executions on the current runtime.

The current collector keeps its maintained root contracts. H2 has no active
checkout, build, target job or measurement window. The archived decoder is an
input to a future proposal, not an outstanding supported-runtime gate.

## Next

Reopen when a concrete consumer needs usable liveness information. Assign an
owner and review current handler semantics, exceptional transfers and consumer
failure behavior together. Unsupported results must retain the safe existing
path; successful temporary decoding must not be promoted into stronger proof.
Do not activate inert metadata merely to finish an old rung.

## Completion

Qualify a complete, useful producer/consumer slice before changing runtime
root reading or clearing behavior. Its semantic and rooting oracles must
cover the claimed scope. Otherwise retain the archive and remain deferred;
there is no current implementation or performance promise.

## References

- [Independent source review and parser disposition](../../design/work/reviews/H2-root-effects-source-review.md).
- [Handler and root contract](../../design/14-handler-contract.md).
- [Archive and evidence retention](../../design/work/worktree-closure-plan.md).
