# Remaining core and extension consumers

- Stage: next design; no active source owner during the wiki reset
- Owner: coordinator assigns fresh disjoint compiler/runtime/bundled increments
- Goal: remove accidental boxed/borrowed assumptions while making extension
  ports possible and incremental.

Use the existing 660-path native consumer inventory and exact dictionary caller
ledger; do not repeat a broad census instead of porting concrete consumers.
Next compiler scope includes symtable/codegen metadata; runtime scopes include
exceptions, frame-local proxies, module clearing and structseq metadata.
Marshal, generated AST, lifecycle/type and bundled module consumers also remain.

Six retained dictionary bodies still need the selected boundary semantics;
temporary scalar borrows cannot implement them under RC. Owned GetItemRef,
NextEntryRef and SetDefaultRef support rebuilt source ports. Unchecked old calls
need explicit edits; source/binary compatibility is not a gate. Earlier prepared
scripts did not execute and must not be mistaken for the current diff.

Split independently owned increments with exact files, ownership/error handling,
focused tests and review. Change generated AST/Clinic products through their
producer. Coordinate shared API changes and commit useful reviewed increments.

References: [boundary design](../../../../build/pyvalue-dict-borrow-boundary-design-20261102-v1/proposal.md),
[caller ledger](../../../../build/pyvalue-dict-borrow-boundary-design-20261102-v1/core-callers.md),
[extension design](../../../design/extension-api.md).
Next: choose and review the first concrete port, then implement/test/review/commit.
