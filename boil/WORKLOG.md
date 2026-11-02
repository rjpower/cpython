# Experiment outcomes

One short line per concluded experiment, linked to its detailed record.
[Current work](issues/README.md) owns active status. The
[pre-reset worklog](archive/WORKLOG-before-wiki-reset-20261102.md) is preserved
verbatim; new checkpoint transcripts do not belong here.

- 2026-11-02 — [Wiki reset](issues/wiki-reset/status.md): replaced stale navigation, process/gates and assignments; preserved history, refreshed Loom goal and passed independent documentation/state review.
- 2026-11-02 — [PyValue foundation](issues/pyvalue-rc-restoration/foundation/status.md): committed reviewed dependency checkpoint `359ea4732de`, preserving all later working source; not yet runnable or integrated.
- 2026-11-02 — [Canonical containers](issues/pyvalue-rc-restoration/container-storage/status.md): committed 38 independently reviewed storage/provider files as `edab780a715`; preserved later changes; dependent source, not runnable.
- 2026-11-02 — [Private value ABI](issues/pyvalue-rc-restoration/abi-namespace/status.md): committed reviewed five-file namespace change `359d10f4d4d`; generated configure exact; runtime checks pending.
- 2026-11-02 — [Dictionary composition](issues/pyvalue-rc-restoration/dictionary-composition/status.md): committed 13 reviewed files as `9b4ff426d1a`, including scalar-builtins fix; six legacy compile failures remain explicit.
- 2026-11-02 — [Symtable consumers](issues/pyvalue-rc-restoration/core-consumers/symtable/status.md): committed corrected canonical port `23cec84d282`; actual syntax and seven host controls pass; candidate tests pending.
- 2026-11-02 — [Shared composition](issues/pyvalue-rc-restoration/shared-composition/status.md): committed 44 reviewed path actions as `c21d0a374ee`; three object/type files held for concrete scalar/cache findings.
- 2026-11-02 — [Codegen](issues/pyvalue-rc-restoration/core-consumers/codegen/status.md): committed reviewed two-file port `ebc9740c4d5`; full TU/new-config object and six host controls pass; candidate tests pending.
- 2026-11-02 — [Sys.flags](issues/pyvalue-rc-restoration/core-consumers/sysflags/status.md): committed reviewed port `015eb226fb3`; full TU/new-config object pass; host policy differences preserved, candidate tests pending.
- 2026-11-02 — [First restored build](issues/pyvalue-rc-restoration/first-build/status.md): configure passed, make failed with 141 objects and no executable; grouped 616 repeated/cascading diagnostics, source unchanged, shared boundary ports prioritized.
- 2026-11-02 — [Dictionary API cut](issues/pyvalue-rc-restoration/dict-boundary-closure/status.md): committed reviewed 12-file removal as `18b6053c75f`; Dict/fixture TUs and real Dict object compile pass; 111 explicit caller sites remain to port.
- 2026-11-02 — [Exception consumers](issues/pyvalue-rc-restoration/core-consumers/errors/status.md): committed reviewed port `d1c8041c78c`; syntax/object compile and applicable host controls pass; seven candidate methods unexecuted.
- 2026-11-02 — [Canonical dictionary reads](issues/pyvalue-rc-restoration/value-readers/status.md): committed reviewed five-file increment `99012f782ea`; real Dict/fixture TUs, object and four symbol definitions pass; 13 candidate methods unexecuted.
- 2026-11-02 — [Generic NativeScope adapters](issues/pyvalue-rc-restoration/native-parsers/generic-adapters/status.md): reviewed two-file dependency committed as `1a2adf3da57`; full TU/object pass; caller integration and runtime pending.
- 2026-11-02 — [Scoped parser/constructors](issues/pyvalue-rc-restoration/native-parsers/parser-owners/status.md): reviewed 18-path dependency committed as `55d410ca39f`; producer/Struct checks pass; old parser errors and candidate execution remain.
- 2026-11-02 — [Marshal consumers](issues/pyvalue-rc-restoration/core-consumers/marshal/status.md): corrected reviewed port committed as `69a1b3ea533`; actual TU/object and nine host controls pass; 13 candidate methods unexecuted.
- 2026-11-02 — [Caller transaction](issues/pyvalue-rc-restoration/native-parsers/caller-transaction/status.md): reviewed ten-path increment committed as `6780ba211ac`; evaluator object/50 producer checks pass; eight candidate methods pending.
- 2026-11-02 — [Merged diagnostic build](issues/pyvalue-rc-restoration/merged-build/status.md): make exit 2, 145 objects and no executable; all 7,332 source inputs unchanged; concrete consumer failures remain.
- 2026-11-02 — [Instance/subclass consumers](issues/pyvalue-rc-restoration/core-consumers/abstract/status.md): reviewed two-file port committed as `3296af9ba58`; actual TU/object and 23 host controls pass; twelve candidate methods pending.
