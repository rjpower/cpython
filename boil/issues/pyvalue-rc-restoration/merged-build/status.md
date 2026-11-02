# Merged universal-value diagnostic build

- Outcome: make exit 2; 145 objects present, no executable
- Owner: coordinator
- Source: fifteen reviewed commits through `6780ba211ac`, plus the three held object/type files
- Purpose: expose concrete remaining ports; no runtime admission

The existing debug configuration and build directory were reused. Bounded
`make -k -j4 python` finished in 14.8 seconds without timeout. All 7,332 source
manifest paths remained unchanged. The restored private ABI is still boilvalrc1.
No candidate was linked or executed.

The log has 612 error diagnostics across source/header paths, including repeated
and cascading errors; this is not a count of independent defects or a performance
metric. Canonical Dict, marshal and evaluator boundaries compile. Remaining
failures include tuple/list object views, parser/Clinic families, type/object
semantics and bundled consumers. The design/review cycle now targets those actual
sites. Keep generated changes with their producers and avoid a second boxed
container mode to force compilation.

[Identity](../../../../build/pyvalue-merged-build-20261102-v2/identity.json),
[results/grouped errors](../../../../build/pyvalue-merged-build-20261102-v2/results.json),
[raw log](../../../../build/pyvalue-merged-build-20261102-v2/make.log),
[source manifest](../../../../build/pyvalue-merged-build-20261102-v2/source-before.json).
The [first attempt](../first-build/status.md) remains historical evidence.

Next: review and port concrete consumers, repeat affected checks, then link the
composed candidate. Runtime/ABI qualification and matched JIT-disabled timing
require that real executable and rebuilt extensions.
