# Value and native interfaces

- State: regex extraction design frozen; implementation paused for interpreter/GC priority
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

The [exact-path/source-control design](regex-code-extraction-design-20261101.md)
froze early, SHA256
`2a4a64285b646c26db84fbbb2742077fa25c6fd1879b391ef807da7c2e185a0b`.
It proposes a nonnegative inline-int code-word read with the old fallback and
error order. At the user's clarification, implementation and site follow-up
are paused: holistic interpreter and integrated allocator/collector work take
priority. The small shared baseline still includes regex/json/unpickle as
observations, without selecting a native-interface implementation. No conversion,
callback ABI or float cache is implemented or accepted.

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
