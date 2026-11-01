# Value and native interfaces

- State: regex extraction hypothesis design active; implementation held
- Updated: 2026-11-01
- Owner: restart_audit; coordinator selects and integrates
- Dependencies: source-bound attribution and a settled callback/root contract
- Branch/head: none assigned
- Worktree: none assigned

## Now

The [Fable native audit](../fundamental-runtime-design/fable-native-audit-20261101.md)
selects regex code-list extraction as a candidate rather than inferring boxing
dominance from stdlib benchmark names. Historical regex conversions are high;
json_loads/unpickle recorded zero inline boxes in that older census. Counts do
not supply current time, fresh allocations or per-site attribution.

A host-only exact-path/source-control design runs 02:27–02:47 UTC, checkpoint
02:37. The shared computed-stats preparation supplies the proposed current
regex/json/unpickle observations. No conversion, callback ABI or float cache is
implemented or selected. The integrated reporting journal preserves actual
native-PC rows for this decision.

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
