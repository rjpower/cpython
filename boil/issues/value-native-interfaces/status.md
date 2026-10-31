# Value and native interfaces

- State: queued
- Updated: 2026-10-31
- Owner: unassigned
- Dependencies: source-bound attribution and a settled callback/root contract
- Branch/head: none assigned
- Worktree: none assigned

## Now

The remaining 6e inventory includes numeric/container/comparison/hash slots,
wrapper argument boxes, Clinic getset residue and inline numeric receivers.
The finite NOARGS proposal is one possible slice, not the whole migration.
Object-self storage, native fields and ports interfaces remain distinct.

Identity, aborting accessors and pointer bounds need reconciliation before
changing contracts. Current `pyvalue.h` deliberately distinguishes value
identity from `Py_Is` pointer equality; `_PyValue_ObjectOrAbort` still serves
interfaces without error channels. These are documented boundaries, not
automatic bug declarations. Historical box counts do not establish current cost.

## Next

Assign a bounded current-source inventory and decision alongside
[interpreter attribution](../interpreter-shared-costs/status.md). Select one
complete shared boundary only when its cost and compatibility consequences
justify implementation.

## Completion

Record each inventory item's implemented, retained or deferred disposition.
Any conversion must close generic, specialized, escaped and generated callers,
preserving data, errors and collecting-call lifetimes, with measured cost.

## References

- [6e inventory, §21.6](../../design/06-architecture-v2.md).
- [Slot interfaces](../../design/12-object-slots-on-values.md).
- [Receiver audit](../../design/work/pre-I-inline-receiver-audit.md) and [value ingress](../../design/work/pre-I-value-ingress-audit.md).
- [Identity/accessor source](../../../Include/pyvalue.h).
