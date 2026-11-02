# Native callers, scoped parsers and registration

- Stage: design accepted; two disjoint implementation lanes active
- Owners: state_audit caller/engine; wiki_audit parser/Clinic; coordinator integrates
- Goal: repair real call preparation/publication and port a production constructor family

The [accepted design](design.md), [active grants](implementation-grants.md) and
[extension contract](../../../design/extension-api.md) define one lifetime model.
Publish coherent IN_CALL before preparation can call out. Publish final callable/
self or tuple homes before retiring obsolete preparation inputs; refresh afterward.
Final invocation inputs, displaced output and staged scalar result boxes survive
until actual result/error publication. Parser/converter locals may retire after
last use while IN_CALL remains coherent. No callback, conversion or call replays.

Lane A extends the existing NativeScope and repairs cold/warm/KW/EX/instrumented
preludes, publication, error paths and Python frame transfer through the actual
VM producer. Lane B adds a tuple-only parser lease and shared scoped unpack,
checks counts/shape before boxing, retains keyword results and regenerates the
nine reviewed constructor/init wrappers in seven production products. Struct's
handwritten new and paired init are included. Registered constructor fixtures
exercise real routes and kwargs aliasing; no static-worker shortcut supplies proof.

Both lanes start at source `99012f782ea`. Their files are disjoint; root owns the
shared index, native build freezes, independent reviews and merged validation.
The contract amendment was independently checked against the accepted design.
Further parser families, varpos/recursive converters, other call gateways and
the opaque extension backend remain unfinished. The three object/type files
stay held for their separate findings. Hard compatibility is not a gate.

## Review and evidence

[Independent v3 review](../../../../build/pyvalue-clinic-parser-design-20261102-v1/independent-review-v3.md),
[coordinator review](../../../../build/pyvalue-clinic-parser-design-20261102-v1/coordinator-review-v3.md),
[frozen inputs](../../../../build/pyvalue-clinic-parser-design-20261102-v1/inputs-v3.json).
V1 was rejected as a test-only policy we already intended to replace. V2 required
real early-prelude and witness-route closure. V3 resolves both plus the parser
preflight/materialization sequencing; its original proposal is preserved in the
[design workspace](../../../../build/pyvalue-clinic-parser-design-20261102-v1).

Next: implement, regenerate twice, run host producer checks, freeze actual inputs
for configured-header diagnostics, independently review code/results and commit
useful dependencies. Then validate the merged candidate. No new native-call or
parser runtime pass exists; candidate tests await a linked restored interpreter.
