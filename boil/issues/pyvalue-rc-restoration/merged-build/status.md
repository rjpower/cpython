# Merged restored-runtime build

Latest: **fails before linking**. There is no restored runtime or timing result.
Root owns integration and validation; see the [board](../../README.md).

## Current evidence

The [v14 probe](../../../../build/pyvalue-merged-build-20261102-v14/results.json)
binds source checkpoint45 `5ece1b27fd0` plus held typeobject composition
`e5602e19`. `make -k -j4 python` exited2 in 3.08 seconds. It left 168 objects
and no executable, with 317 repeated/cascading errors at 263 distinct locations
in 54 source/header paths. All 7,351 inputs remained unchanged.

- [Identity](../../../../build/pyvalue-merged-build-20261102-v14/identity.json)
- [Raw log](../../../../build/pyvalue-merged-build-20261102-v14/make.log)
- [Before inputs](../../../../build/pyvalue-merged-build-20261102-v14/source-before.json)
- [After inputs](../../../../build/pyvalue-merged-build-20261102-v14/source-after.json)
- [Context-free summary](../../../../build/pyvalue-merged-build-20261102-v14/summary.json)

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
enum/atexit retain unrelated consumers. Typeobject still has three errors and
three warnings in pickle/state readers. Its three functions are being ported
as separate Luna packets. The shared FORMAT kernel is code-reviewed; fixtures,
backend/public API/caller closure remain required. See [porting](../porting/status.md)
for live assignments.

## Prior evidence

The former detailed chronology is retained byte-for-byte in
[attempts through v13](history/attempts-through-v13-20261102.txt).
The [work log](../../../WORKLOG.md) has concise outcomes; raw identities, source
manifests and logs stay in each experiment directory. Earlier failures and
focused passes remain historical evidence, not runtime admission.
