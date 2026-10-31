# Value and native interfaces

- State: queued
- Updated: 2026-10-31
- Owner: unassigned
- Dependencies: source-bound attribution and a settled callback/root contract
- Branch/head: none assigned
- Worktree: none assigned

## Now

Remaining boundaries include numeric/container/comparison/hash slots, wrapper
argument conversion, Clinic residue and inline numeric receivers. Receiver,
argument, result and stored-object forms are separate contracts. Current
`PyValue` identity and `Py_Is` pointer equality are distinct; aborting object
accessors serve interfaces without error channels. These are boundaries to
reconcile, not automatic bug declarations or current cost measurements.

## Next

Choose one bounded inventory or interface decision from measured consumer cost.
Close generic, specialized, escaped and generated callers together; avoid a
parallel object-form fallback interface. Split an executable child from this
backlog before implementation.

## Completion

Each admitted item has an implemented, retained or deferred disposition, with
useful data/errors/identity, collecting-call lifetimes and matched cost evidence.

## References

- [Values and object protocol](../../design/values-and-object-protocol.md).
- [Receiver audit](reviews/receivers.md).
- [Value-ingress audit](reviews/value-ingress.md).
- [Identity/accessor source](../../../Include/pyvalue.h).
