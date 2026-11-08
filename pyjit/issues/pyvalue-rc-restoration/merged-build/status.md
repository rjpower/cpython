# Merged restored-runtime build

Latest: **fails before linking**. There is no restored runtime or timing result.
Root owns integration and validation; see the [board](../../README.md).

## Current evidence

The [v22 probe](../../../../build/pyvalue-merged-build-20261103-v22/results.json)
binds checkpoint56 `6bf198d1ef5` plus held typeobject `ce8c0c45`.
`make -k -j4 python` exits 2 in 36.32 seconds, with 247 error observations,
173 objects and no executable. All 7,362 source inputs stay unchanged. The E
shared-header change triggers rebuilding without reducing the error total.
This probe completed at 2026-11-03 01:46:23 UTC; the documentation handoff does
not claim a newer runtime build.

- [Identity](../../../../build/pyvalue-merged-build-20261103-v22/identity.json)
- [Raw log](../../../../build/pyvalue-merged-build-20261103-v22/make.log)
- [Before inputs](../../../../build/pyvalue-merged-build-20261103-v22/source-before.json)
- [After inputs](../../../../build/pyvalue-merged-build-20261103-v22/source-after.json)

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
semantics still need admission. FORMAT foundation A is committed; backend/
public API/caller closure remains required; E export is committed in56. See [porting](../porting/status.md)
for packet states; runtime workers are stopped for handoff.

## Prior evidence

The [v21 probe](../../../../build/pyvalue-merged-build-20261103-v21/results.json)
binds checkpoint55: 247 errors/173 objects and 7,362 unchanged inputs. Enum/map55
removes six observations; its [comparison](../../../../build/pyvalue-merged-build-20261103-v21/diagnostic-comparison.json)
preserves moved rows and verifies no added diagnostic messages.

The [v20 probe](../../../../build/pyvalue-merged-build-20261103-v20/results.json)
has 253 errors/172 objects at checkpoint54, all 7,361 inputs unchanged. It removes
exactly the final batched error without added diagnostics. The preceding
[v19 probe](../../../../build/pyvalue-merged-build-20261103-v19/results.json)
has 254 errors/171 objects at checkpoint53, all 7,360 inputs unchanged.

The [v18 probe](../../../../build/pyvalue-merged-build-20261103-v18/results.json)
had 280 errors/171 objects at checkpoint52; it removed nineteen datetime errors
and three header warnings without adding diagnostics.

The [v17 probe](../../../../build/pyvalue-merged-build-20261103-v17/results.json)
had 299 errors/170 objects at checkpoint51, with unchanged diagnostics from v16.

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
