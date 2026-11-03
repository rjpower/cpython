# Native callers, scoped parsers and registration

- Stage: prior adapters committed; flat-format kernel/five callers committed; shared backend design
- Owner: coordinator assigns remaining scopes and validates composition
- Goal: repair real call preparation/publication and port production constructor families

The [accepted design](design.md), [completed grants](implementation-grants.md),
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

- [Caller transaction](caller-transaction/status.md), `6780ba211ac`:
  cold/warm/KW/EX/instrumented preparation, native publication and selected-code
  Python frame transfer. Actual evaluator/fixture TUs and evaluator object pass.
  Fifty host generator checks pass; eight candidate methods remain unexecuted.

## Remaining work

Root owns the shared index, coordinated native checks, independent reviews and
merged validation. The merged diagnostic build still has no executable. Neither
the caller nor parser runtime routes are admitted by these source checkpoints.

Further parser families, varpos/recursive converters, exact-argument preparation,
the constructor-init shim, broader binder cleanup, other gateways and the opaque
extension backend remain unfinished. Object and abstract consumer repairs are
committed; typeobject.c remains partly held. Hard compatibility is not a gate.
The earlier native/parser source grants listed above are closed.
The [positional Clinic increment](positional-owners/status.md) is reviewed and
committed as `be15f9856b5`. Fixed-keyword and constructor-helper producer rules
are also committed; the [board](../../README.md) owns their current checkpoints.

The explicit caller-owned flat-format kernel and five consumers are committed
as checkpoint41, `aa7a4335ddf`, after independent actual-code review and root
composed native checks. Three registered fixtures cover object/UTF8 leases,
unsupported formats, and typed/nullable output. Native diagnostic signatures
are unchanged; the actual fixture object builds. Candidate fixtures have not run.
Sol designs the shared keyword/nested/resource backend engine next. This first
kernel does not close the old raw parsers: keyword/nested/resource backends,
public opaque scopes, producer and mechanical caller migrations remain mandatory.
[Exact grant](../../../../build/pyvalue-flat-format-scoped-design-20261102-v1/coordinator-grant.md),
[baseline closure](../../../../build/pyvalue-flat-format-scoped-design-20261102-v1/baseline-closure.md).

## Design evidence

[Independent v3 review](../../../../build/pyvalue-clinic-parser-design-20261102-v1/independent-review-v3.md),
[coordinator review](../../../../build/pyvalue-clinic-parser-design-20261102-v1/coordinator-review-v3.md),
[frozen inputs](../../../../build/pyvalue-clinic-parser-design-20261102-v1/inputs-v3.json).
V1 was rejected as a test-only policy we intended to replace. V2 required real
early-prelude and witness-route closure. V3 resolves both and the parser
preflight/materialization sequencing. Earlier proposals remain in the
[design workspace](../../../../build/pyvalue-clinic-parser-design-20261102-v1).

[Object C call bridges](object-call-bridges/status.md) are now reviewed and committed,
with actual call.c/fixture TUs and objects passing. The
[startup type constructor](../core-consumers/type-new/status.md) is committed
as checkpoint38, followed by ten construction readers and caller pins in39.
General keyword/nested/resource format-parser ownership and its consumer ports
remain design/implementation work.

Shared FORMAT foundation A v2 is committed as checkpoint47 `4f2734e77cd`
after independent code/native review:
[finite A/B/C packets](../../../../build/pyvalue-format-engine-design-20261102-v2/README.md).
A is a shared stable lease/resource foundation; B/C adapt the actual backends.
Public scope, raw-wrapper retirement and mechanical consumer closure remain
mandatory before claiming parser closure. No source or host pass follows from
this checkpoint. The accepted revision specifies the same-scope Finish/End
contract, chunk-only resources and a typed fixture destructor. Full fixture
TU/object passes without diagnostics; getargs retains its twelve baseline errors.
The nine-method matrix is source-reviewed and syntax-checked, not candidate-run. B also needs an actual per-export buffer lifetime contract;
owning view.obj alone does not retain released export storage. The four final
Clinic wrappers are committed as checkpoint45 after corrected v3 code/native
review; they do not widen the flat parser whitelist or depend on the new
foundation.


The [E export packet](../../../../build/pyvalue-format-export-E-composed-20261103-v1/README.md)
now passes the configured header-overlay gate: getargs retains exactly twelve
errors with no new diagnostic signatures; the expanded fixture TU/object are
clean. Five added candidate methods preserve all original tests. Independent
actual review is active; no new fixture behavior has executed. Sol has frozen
[B1 positional templates](../../../../build/pyvalue-format-B-positional-design-20261103-v1/README.md)
and [B2 resource/converter templates](../../../../build/pyvalue-format-B-resources-design-20261103-v1/README.md),
with C keyword and public/caller closure still in design. These signature-changing
overlays stay isolated until the whole dependent contract is ready; E alone does
not close the parser or retire its raw wrappers.
