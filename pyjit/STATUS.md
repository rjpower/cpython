# Project status

The selected DeltaBlue checkpoint is runtime 48 `5559f60093e`: canonical guarded
class values and owned classmethod pairs. Both improvements are committed and
pushed. Source is restored to 48 on `pyjit/deltablue-checkpoint-20261110`.
[Report](issues/deltablue-call-model-20261110/checkpoint-report.md) →
[tables](issues/deltablue-call-model-20261110/benchmark-tables.md) →
[next plan](issues/deltablue-call-model-20261110/plan.md).

## Performance

Final confirmations use six balanced pairs per target; the full 29-kernel upstream
screen uses two per unchanged kernel. GCC 15.2, CPU 15, hash 0 and JIT disabled.
Lower elapsed ratio is better:

| Case | Selected 48 / starting checkpoint 46, six pairs | Selected 48 / upstream |
| --- | ---: | ---: |
| DeltaBlue | 0.739181 | 1.225425, six pairs |
| Logging silent | 0.872129 | 0.758002, six pairs |
| All 29-kernel geometric mean | not measured | 1.179430, two pairs |

DeltaBlue takes 26.1% less time than starting checkpoint 46 but 22.5% more
than upstream. Logging parity holds in every confirmation pair. Broad upstream
parity remains open; the mean is essentially flat against the earlier checkpoint 46 screen.
Separate campaigns do not provide an exact causal decomposition. See the
[full tables](issues/deltablue-call-model-20261110/benchmark-tables.md).

The complete conditional leaf experiment is **rejected**, not selected. It
elides 25,504 real activations per DeltaBlue iteration yet runs 15.2% slower on
that target and 21.3% slower on logging than frozen 48. Its generic transaction
repeats facts/decoding and builds a dynamic owner ledger. Compiling away its
call-site gate/execution leaves DeltaBlue +2.0% and logging +9.0%; the ordinary
residual is open. The [experiment report](issues/deltablue-call-model-20261110/integration49.md)
retains counters, matched profiles, failures and ablation results.

## Qualification

Selected release/debug build 117 modules. Component focused checks and the
306-unit strict value API gate pass. Both coherent full diagnostics
run 54,697 tests. Release/debug report the established 16/21 GDB display failures,
nine unsupported debugger-jump errors per role, and one debug trace-format
failure. The stale disassembly failures are absent. Do not
call the full suite green. GDB formatting and exact CPython frame/finalizer
transcripts are separate from Python behavior and throughput.

The rejected experiment separately passes 908 focused tests in each role and
the API gate; its debug path declines elision. No full promotion run was warranted
after its performance screen failed. Stats identifies48, builds 117 modules and passes 150 smoke tests.
Exact commands, identities and evidence are in [HANDOFF](HANDOFF.md).

## Next decision

The single [Astra review](issues/deltablue-call-model-20261110/astra-architecture-review.md)
is complete. Sol reviews the inner loop; Luna/Terra own bounded mechanical
updates. The [C2 design](issues/deltablue-call-model-20261110/C2-proposal.md) has
peer approval as a future direction: select leaf CALLs during specialization,
cache immutable feedback units, consume checked-entry facts and simplify common
unary retirement. The [completion review](issues/deltablue-call-model-20261110/C2-completion-review.md)
identifies the shared committed-caller/observer boundary required to remove the
ledger entirely. C2 is unimplemented and has no measured speedup. Do not expand
grammar or change the frame ABI before that execution contract beats ordinary
calls. The coordinator owns integration and matched measurements.

Universal PyValues are integrated across runtime/containers/frames and rebuilt
extensions. Native method/tracing JIT tiers remain unfinished. Historical
runtime39 `4d666e1b482`, combined snapshot `cb906f9bc52`, upstream and previous
golden controls remain unchanged. Prior
[object/call46](issues/object-call-20261109/checkpoint46-report.md) and
[logging closure](issues/frame-call-model-20261109/closing-report.md) retain their
historical results and limits. Current contracts:
[IR](design/register-ir.md), [interpreter](design/interpreter.md),
[frames/calls](design/call-and-frame-lifetimes.md), [values](design/pyvalue.md).
