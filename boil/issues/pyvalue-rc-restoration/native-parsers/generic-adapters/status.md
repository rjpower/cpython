# Generic NativeScope adapters

- Stage: reviewed dependency committed as 1a2adf3da574fedda7587caafac0f2c8f17953c4
- Author: state_audit; independent review/integration: coordinator
- Scope: two engine/header files; caller integration remains active

Object-vector and tuple-call adapters reuse capture, staging, actual publication,
abort and first-error cleanup in the existing engine. Tuple calls retain their
actual callable/tuple/kwargs. Generic vector capture retains the stable owning
window requirement and has no writable prefix/OFFSET flag. Unsupported Cfunction
dispatch can invoke the generic adapter within the same captured scope.

The full native_scope TU and configured object compile pass without warnings.
Ten preexisting functions remain exact; BeginVector factors shared capture and
End adds two container pin releases. New caller transactions, tests and merged
execution remain unfinished; this is not route/runtime admission.

[Review](../../../../../build/pyvalue-native-call-transaction-20261102-v1/engine-independent-review.md),
[source checks](../../../../../build/pyvalue-native-call-transaction-20261102-v1/engine-checks.json),
[compile evidence](../../../../../build/pyvalue-clinic-owner-scope-20261102-v1/native-objects.json),
[commit](../../../../../build/pyvalue-native-call-transaction-20261102-v1/engine-commit.json).
