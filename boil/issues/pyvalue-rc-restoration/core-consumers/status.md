# Remaining core and extension consumers

- Stage: symtable committed; sys.flags and codegen design; later consumers queued
- Owner: coordinator; fresh state_audit implements and wiki_audit reviews
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
[Symtable port](symtable/status.md): implementation complete, actual full-TU
syntax passed and seven semantic cases passed on host 3.13/3.14 as reference
controls. Independent review accepted the corrected private KnownHash lookup;
the coordinator committed `23cec84d282`. Candidate execution remains pending.

Next small consumer increment: two preexisting `PyStructSequence_GET_ITEM`
reads in `Python/sysmodule.c`, exposed by the private ABI syntax check. This is
a separate ownership conversion, not part of the namespace string change. Other
compiler/runtime/bundled increments remain separately queued.

A five-TU [diagnostic probe](../../../../build/pyvalue-core-build-diagnostics-20261102-v1/syntax-diagnostics.json)
found concrete removed container reads in codegen, marshal and errors; frameobject
and namespaceobject passed provisional syntax. These are compile diagnostics,
not a linked build. [Codegen design](../../../../build/pyvalue-codegen-consumers-20261102-v1/design.md)
is under independent review; sys.flags design is assigned to state_audit.
