# GC cleanup and scanning

- State: closed
- Outcome: landed
- Updated: 2026-10-31
- Owner: coordinator for record maintenance; no active runtime operator
- Dependencies: none outstanding for this declared milestone
- Branch/head: retained `boil/gc-native-cleanup` at `0b15b4bbc9b`
- Worktree: retained serving checkout `boil-gc-native-cleanup`; no active job
- Integration: `aee46aacae2`; qualified source/native `0b15b4bbc9b`

## Now

Selective death processing, guarded native suspended-generator cleanup,
direct managed scanning, warning delivery and coroutine initialization repairs
are landed. Independent review clears the explicit source-delta qualification
composite and both postmerge smokes. Predecessor full-suite passes keep their
original identities; original failures remain failed in the linked records.

The completed matched 29-workload comparison reports geometric-mean throughput
of **0.871038× upstream**, with seven faster and 22 slower rows. Same-fork and
profile results describe the combined change, not isolated component gains.
Five serial samples provide descriptive ranges, not confidence intervals.
The finite native measurement batch and its independent review are complete.

The supported boundary is Linux, one main interpreter, the GIL and ordinary
threads. This does not claim foreign lifecycle support, a nursery or complete
interpreter parity.

## Next

No further action is assigned here. Preserve the sealed products, source
identities and evidence paths. Follow-up implementation belongs to the queued
[young-space](../gc-common-young-space/status.md) and
[interpreter-cost](../interpreter-shared-costs/status.md) issues.

## Completion

This milestone closes with reviewed landing, postmerge behavior checks and the
reported finite performance result. New runtime changes require their own
scope and evidence; they do not reopen or relabel these historical executions.

## References

- [Independent landing and qualification review](../../design/work/reviews/gc-cleanup-final-landing.md).
- [Performance results and raw evidence links](../../design/work/gc-post-cleanup-performance.md).
- [Current architecture and recommendation](../../design/work/gc-architecture-next.md).
