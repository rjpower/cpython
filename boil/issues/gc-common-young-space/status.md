# Common young-space roadmap

- State: closed
- Outcome: superseded
- Decision: split into bounded experiments
- Updated: 2026-10-31
- Owner: coordinator

The previous all-at-once milestone is replaced by independently reviewed
experiments: [minimal nursery](../gc-nursery-minimum/status.md),
[coverage and reclamation](../gc-nursery-coverage/status.md), then
[reduced old-object work](../gc-nursery-old-work/status.md). Each integrates,
rejects or narrows its own hypothesis. This planning change does not claim a
nursery implementation or alter the landed baseline.
