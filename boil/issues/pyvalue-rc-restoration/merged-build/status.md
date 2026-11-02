# Merged restored-runtime build

Latest: **fails before linking**. There is no restored runtime or timing result.
Root owns integration and validation; see the [board](../../README.md).

## Current evidence

The [v15 probe](../../../../build/pyvalue-merged-build-20261102-v15/results.json)
binds source checkpoint47 `4f2734e77cd` plus held typeobject composition
`542923cf`. `make -k -j4 python` exited2 in 36.12 seconds. It left 169 objects
and no executable, with 314 repeated/cascading errors at 260 distinct locations
in 53 source/header paths. All 7,353 inputs remained unchanged.

- [Identity](../../../../build/pyvalue-merged-build-20261102-v15/identity.json)
- [Raw log](../../../../build/pyvalue-merged-build-20261102-v15/make.log)
- [Before inputs](../../../../build/pyvalue-merged-build-20261102-v15/source-before.json)
- [After inputs](../../../../build/pyvalue-merged-build-20261102-v15/source-after.json)
- [Context-free summary](../../../../build/pyvalue-merged-build-20261102-v15/summary.json)

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

The preceding [v14 probe](../../../../build/pyvalue-merged-build-20261102-v14/results.json)
had 317 errors/168 objects at checkpoint45; its raw evidence stays preserved.

The former detailed chronology is retained byte-for-byte in
[attempts through v13](history/attempts-through-v13-20261102.txt).
The [work log](../../../WORKLOG.md) has concise outcomes; raw identities, source
manifests and logs stay in each experiment directory. Earlier failures and
focused passes remain historical evidence, not runtime admission.
