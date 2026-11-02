# Native callers, scoped parsers and registration

- Stage: generic adapters and scoped constructors committed; caller transaction in implementation
- Owner: state_audit caller/producer; coordinator reviews and integrates
- Goal: repair real call preparation/publication and port production constructor families

The [accepted design](design.md), [remaining grant](implementation-grants.md),
[Python transfer amendment](python-transfer.md) and
[extension contract](../../../design/extension-api.md) define one lifetime model.
Publish coherent IN_CALL before preparation can call out. Publish final callable/
self or tuple homes before retiring obsolete preparation inputs; refresh afterward.
Final invocation inputs, displaced output and staged scalar result boxes survive
until actual result/error publication. Parser/converter locals may retire after
last use while IN_CALL remains coherent. No callback, conversion or call replays.

## Reviewed checkpoints

- [Generic NativeScope adapters](generic-adapters/status.md), `1a2adf3da57`:
  real object-vector and tuple-call routes reuse existing capture/publication.
  Actual native_scope.c TU and object pass; caller admission remains separate.
- [Scoped parser and constructors](parser-owners/status.md), `55d410ca39f`:
  tuple-only owner leases, cheap preflight before materialization, nine wrappers
  in seven production products and real registered constructor witnesses.
  Struct compiles. Other full-TU diagnostics fail in unchanged consumer bodies;
  fourteen candidate methods remain unexecuted. Host producer/reference checks
  and their limits are recorded with the checkpoint.

## Active increment

State_audit implements cold/warm/KW/EX/instrumented call preparation, invocation,
publication and Python transfer through the actual VM producer. One selected code
snapshot must determine binding and frame layout through allocation. Consumed
copies protect parent homes before handoff; the child becomes current before
parent duplicates retire. Analyzer metadata must expose the transaction markers'
real callouts, errors and frame transfers. Its bounded amendment and focused
tests are part of the grant; generated products must come from their producers.

Root owns the shared index, coordinated native checks, independent reviews and
merged validation. Freeze useful endpoints for code/results review and commit
reviewed dependencies. No new native-call or parser runtime pass exists; all
candidate witnesses await a linked restored interpreter.

Further parser families, varpos/recursive converters, the constructor-init shim,
broader binder cleanup, other gateways and the opaque extension backend remain
unfinished. Three object/type files stay held for separate findings.
Hard compatibility is not a gate. The completed parser lane has no active source
grant; wiki_audit now works on the type-cache design proof.

## Design evidence

[Independent v3 review](../../../../build/pyvalue-clinic-parser-design-20261102-v1/independent-review-v3.md),
[coordinator review](../../../../build/pyvalue-clinic-parser-design-20261102-v1/coordinator-review-v3.md),
[frozen inputs](../../../../build/pyvalue-clinic-parser-design-20261102-v1/inputs-v3.json).
V1 was rejected as a test-only policy we intended to replace. V2 required real
early-prelude and witness-route closure. V3 resolves both and the parser
preflight/materialization sequencing. Earlier proposals remain in the
[design workspace](../../../../build/pyvalue-clinic-parser-design-20261102-v1).
