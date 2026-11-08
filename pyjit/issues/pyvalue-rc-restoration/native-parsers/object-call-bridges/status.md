# Object C API call bridges

- Stage: mutually independent production/fixture review; committed as `a5563d833a7`
- Production author: coordinator; reviewer: state_audit
- Fixture/test author: state_audit; reviewer/integrator: coordinator

Fourteen call.c functions now compose canonical tuple/dictionary storage with
existing owning views and method temporaries. Tuple arrays stay owned through
invocation. Keyword arrays use an owning selected snapshot without a dictionary
cursor across allocation/boxing. Partial allocation, result and owner retirement
preserve the first error. Method callers use the actual canonical provider contract.

Dispatch is selected after preparation. Ordinary calls respect current vectorcall
flags/tp_call; public PyVectorcall_Call retains its deliberate no-flag protocol.
The latter cannot fall back recursively through its own tp_call trampoline. Ordinary
tuple fallback preserves original tuple/dict inputs. MakeTpCall selects current
tp_call after constructing containers. No call/converter effects are replayed.

The fixture removes its raw tuple-layout cast, retains scopes in two existing
users and registers four actual C API witnesses. Twenty-one other production
bodies, fifteen other fixture bodies and generated Clinic output remain exact.

Two actual full TUs and configured objects pass with zero diagnostics on 310 stable
inputs. Nine existing-CAPI host controls pass without skips. All seventeen candidate
methods remain unexecuted, including eight dedicated new witnesses. The initial
explicit-None host fixture failure is retained. No linked/runtime/performance pass.

[Design and review resolution](../../../../../build/pyvalue-call-object-bridges-20261102-v1/design.md),
[production review](../../../../../build/pyvalue-call-object-bridges-20261102-v1/production-code-review.md),
[fixture review](../../../../../build/pyvalue-call-object-bridges-20261102-v1/coordinator-fixture-review.md),
[actual checks](../../../../../build/pyvalue-call-object-bridges-20261102-v1/native-v1.json),
[commit](../../../../../build/pyvalue-call-object-bridges-20261102-v1/commit.json).
