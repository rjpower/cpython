# Native call and parser implementation boundaries

The coordinator and independent peer accepted the frozen v3 design. The caller
grant is active; the parser grant is closed after commit. These
disjoint boundaries apply on `build/worktrees/pyvalue-rc-universal`, starting
from source commit `99012f782ea`. Neither agent may stage or commit. Root owns
integration, shared builds and admission. Hard compatibility is not a gate.

## A — state_audit: caller transaction and NativeScope

Own only these source boundaries:

- Include/internal/pycore_native_scope.h and Python/native_scope.c: reviewed
  generic object-vector/tuple-call adapters and shared capture/result internals.
- Python/ceval.c: named value-operation/native-vector coordination and bounded
  call-preparation helpers; no unrelated arithmetic/provider changes.
- Python/bytecodes.c: named method-expansion, monitoring, tuple preparation,
  cold/warm/EX/instrumented call and Python-input-transfer operations.
- Tools/cases_generator/ir_generator.py: handler entry, refresh/error and
  publication producers; corresponding generated native products together.
- Tools/cases_generator/analyzer.py: the named transaction markers' actual
  callout, error, synchronization, terminal and frame-switch properties in
  escaping_call_in_simple_stmt, has_error_without_pop, always_exits,
  switches_frame and compute_properties as needed. Keep unrelated uppercase
  macro behavior unchanged and prevent unsupported Tier 2 admission.
- Lib/test/test_generated_cases.py: TestCallTransactionMarkers and necessary
  helpers checking analyzed properties and generated control flow. Narrowly
  update TestJitHelpers.test_real_call_return_and_error_order and
  test_object_iteration_and_keyword_adapters for the actual replacement
  transaction boundaries; preserve unrelated assertions and test coverage.
- Modules/_testinternalcapi/native_scope.c and its Clinic product: bounded real
  suspended-frame/window observer; preserve existing fixtures.
- Lib/test/test_pyvalue_native_call_transaction.py: focused real call witnesses.

Use one transaction before the first preparation callback. Publish final input
homes before retiring obsolete preparation locals. Keep final invocation inputs
through result/error publication. Refresh success and error paths; no replay
once preparation can have effects. Drain C-only preparation owners before Python
inlined frame transfer. Reuse NativeScope and actual publication, not a new engine.

The analyzer amendment covers IR_PREPARE_CALL_METHOD, IR_PREPARE_CALL_TUPLE,
IR_MONITOR_CALL, IR_REFRESH_CALL and IR_INVOKE_CALL. Keep the pure
IR_BEGIN_CALL_EFFECTS distinct. Declare semantics directly; do not add dummy
calls, dead error branches or stale metadata flags. Check automatic escaping-call
emission against the IR emitter's own refresh/publication handling. Regenerate
all affected products normally and retain before/after inputs.

## B — completed: scoped parser and production constructors

Own only these source boundaries:

- Include/internal/pycore_modsupport.h and Python/getargs.c: explicit tuple-only
  owner lease/scoped unpack plus reachable metadata/error helpers. Pin/count
  first; shared cheap preflight before allocating/materializing values.
- Tools/clinic/libclinic/parse_args.py: structural family selection, scope setup,
  arguments and common result/error cleanup. No converters.py/varpos migration.
- The nine reviewed wrappers and seven input/product pairs: memoryview,
  mappingproxy, ParamSpecArgs/Kwargs, os.sched_param, ZoneInfo, Struct/new+init,
  BinaryReader init. Production inputs otherwise get checksum-only changes;
  Modules/_struct.c s_new is the sole handwritten constructor body port, with
  format cleanup on allocation failure and preserved subclass dispatch.
- Modules/_testclinic.c and its generated header; Lib/test/test_clinic.py and
  applicable clinic.test.c output; Lib/test/test_pyvalue_clinic_owners.py.
  Use real registered constructors and the reviewed nested VectorcallDict alias
  route, not static implementation calls or an unported FASTCALL helper.

Generator closure amendment: the same owned _testclinic.c input also emits
Modules/clinic/_testclinic_depr.c.h and _testclinic_kwds.c.h. Root inspected and
admits their generated pycore_value.h include/checksum changes. This adds no
handwritten scope or parser family; retain their normal regenerated products.

Parser locals stay owned through last use/converter cleanup and may retire in
coherent IN_CALL. Keep an implementation result through cleanup; retain primary
errors and reject success with an exception. No wrapper ABI cast or hidden scope.

## Integration and gates

Both lanes use the [accepted design](design.md) and
[extension lifetime contract](../../../design/extension-api.md). The A/B
interfaces share an invariant, not scope storage. Ask the coordinator to review
any necessary out-of-scope helper; no user compatibility approval is needed.
Keep the three held object/type files unchanged. Do not hide unrelated compiler
failures with fake declarations, pointer casts or a different configuration.

Record actual changed hashes, producer regeneration and second-run idempotence,
host producer tests and intended candidate witnesses. Coordinate native checks
with root so all consumed source/header inputs are stable. Label residual TU
failures and unexecuted candidate tests precisely. Freeze useful endpoints for
independent code/results review; root commits reviewed dependencies and validates
the merged slice. Full runtime admission still requires a linked candidate.

[Peer review](../../../../build/pyvalue-clinic-parser-design-20261102-v1/independent-review-v3.md),
[coordinator review](../../../../build/pyvalue-clinic-parser-design-20261102-v1/coordinator-review-v3.md).

## Python transfer amendment

The [accepted transfer amendment](python-transfer.md) extends A to private
code-specific provider helpers and the initialize_locals(code) seam. One code
snapshot determines locals policy, frame layout/executable and binding through
EX allocation. Consumed copies protect parent homes on failure; actual child
handoff precedes retiring parent duplicates. Refresh child entry at known numeric
PC 0 before first dispatch; rollback never adjusts unentered recursion accounting.
The constructor-init-shim and broader binder cleanup remain separate holds.

B closed at 55d410ca39f; A’s two engine files are committed as 1a2adf3da57.
Only the remaining caller/producer/observer scope is still an active source grant.
