# Merged restored-runtime build

Latest: **fails before linking**. There is no restored runtime or timing result.
Root owns integration and validation; see the [board](../../README.md).

## Current evidence

The [v17 probe](../../../../build/pyvalue-merged-build-20261103-v17/results.json)
binds source checkpoint51 `14fc009d42f` plus held typeobject `ce8c0c45`.
`make -k -j4 python` exited 2 in 3.84 seconds, leaving 170 objects and no executable,
with 299 repeated/cascading errors. All 7,357 source inputs remained unchanged.
Lookup/call50 and scalar boundaries51 are included. The error/warning multiset
is [exactly unchanged from v16](../../../../build/pyvalue-merged-build-20261103-v17/diagnostic-comparison.json);
these increments close semantic domain/error gaps in previously compiling code.

- [Identity](../../../../build/pyvalue-merged-build-20261103-v17/identity.json)
- [Raw log](../../../../build/pyvalue-merged-build-20261103-v17/make.log)
- [Before inputs](../../../../build/pyvalue-merged-build-20261103-v17/source-before.json)
- [After inputs](../../../../build/pyvalue-merged-build-20261103-v17/source-after.json)

Counts include repeated diagnostics and shared-header cascades. They are not
independent edits, a percentage complete, or a performance result. Object counts
include earlier focused builds. Parallel compiler output is not used to invent
function context by joining unrelated lines.

## Next gate

Close concrete core/startup and selected bundled-module consumers under reviewed
porting rules. Root integrates accepted deltas and reruns the merged build. Once
linking succeeds, execute startup, focused value/lifetime/native cases and broad
qualification before matched timing with JIT disabled.

Clinic4 cleared eight selected native errors and produced float/tuple objects;
enum/atexit retain unrelated consumers. Typeobject now compiles without
diagnostics after three separate pickle/state function ports; held scalar/cache
semantics still need admission. FORMAT foundation A is committed; backend/export/
public API/caller closure remains required. See [porting](../porting/status.md)
for live assignments.

## Prior evidence

The preceding [v16 probe](../../../../build/pyvalue-merged-build-20261103-v16/results.json)
had 299 errors/170 objects at checkpoint49; its census remains source-bound there.

The earlier [v15 probe](../../../../build/pyvalue-merged-build-20261102-v15/results.json)
had 314 errors/169 objects at checkpoint47; its raw evidence stays preserved.

The earlier [v14 probe](../../../../build/pyvalue-merged-build-20261102-v14/results.json)
had 317 errors/168 objects at checkpoint45; its raw evidence stays preserved.

The former detailed chronology is retained byte-for-byte in
[attempts through v13](history/attempts-through-v13-20261102.txt).
The [work log](../../../WORKLOG.md) has concise outcomes; raw identities, source
manifests and logs stay in each experiment directory. Earlier failures and
focused passes remain historical evidence, not runtime admission.
