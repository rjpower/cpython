# Remaining core and extension consumers

- Stage: symtable, codegen, sys.flags, errors, marshal and abstract consumers committed; further ports queued
- Owner: coordinator assigns scoped ports and independent review
- Goal: remove accidental boxed/borrowed assumptions while making extension
  ports possible and incremental.

Use the existing 660-path native consumer inventory and exact dictionary caller
ledger; do not repeat a broad census instead of porting concrete consumers.
Completed compiler ports cover symtable and codegen metadata. Remaining runtime
scopes include frame-local proxies, module clearing and structseq metadata.
Generated AST, lifecycle/type and bundled module consumers also remain.

The [dictionary API cut](../dict-boundary-closure/status.md) removed borrowed
entrypoints. Owning GetItemRef, NextEntryRef and SetDefaultRef support rebuilt
source ports; remaining old calls need explicit edits. Source/binary compatibility
is not a gate. Earlier prepared scripts did not execute and must not be mistaken
for the current diff.

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

[Codegen](codegen/status.md) and [sys.flags](sysflags/status.md) are source committed
after independent review; both compiled in the new debug build. Candidate
execution remains pending. The [errors port](errors/status.md) is also committed
and compiles as a real object; its candidate tests await a linked interpreter.
Further compiler/runtime/bundled increments remain queued.

A five-TU [diagnostic probe](../../../../build/pyvalue-core-build-diagnostics-20261102-v1/syntax-diagnostics.json)
found concrete removed container reads in codegen, marshal and errors; frameobject
and namespaceobject passed provisional syntax. These are compile diagnostics,
not a linked build. [Codegen design](../../../../build/pyvalue-codegen-consumers-20261102-v1/design.md)
was implemented, reviewed and committed. The subsequent [full build](../first-build/status.md)
now supplies concrete grouped diagnostics; use it instead of repeating the inventory.

[Marshal port](marshal/status.md): reviewed two-file checkpoint committed as `69a1b3ea533`;
actual full TU/object and nine host controls pass. Candidate tests await linking.

[Instance/subclass consumer port](abstract/status.md): design independently accepted;
reviewed two-file source checkpoint committed as `3296af9ba58`; actual TU/object
and 23 host controls pass. Candidate tests remain unexecuted.

Active [positional Clinic increment](../native-parsers/positional-owners/status.md):
20 generated wrappers reviewed and committed as `be15f9856b5`; producer checks
pass, native diagnostics remain recorded. The
[object attribute bridge](object-attributes/status.md) needs the finite cleanup
corrections identified by independent whole-file review. State_audit designs the
next startup/parser port; wiki_audit implements object corrections before core
type-cache work.
