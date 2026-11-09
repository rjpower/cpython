# Experiment records

Use [project status](../STATUS.md) for the program view and the
[stage map](../ROADMAP.md) for outcomes and dependencies. These directories
hold detailed experiments; their presence does not mean they are active.

| Record family | Parent workstream |
| --- | --- |
| [Frame/call model and logging parity](frame-call-model-20261109/plan.md) | [Interpreter baseline](../stages/02-baseline/README.md) |
| [Execution boundaries and static operand modes](execution-boundaries-20261108/plan.md) | [Interpreter baseline](../stages/02-baseline/README.md) |
| [Golden checkpoint, cold review and next steps](golden-20261108/README.md) | [Interpreter baseline](../stages/02-baseline/README.md) |
| [Universal-value restoration](pyvalue-rc-restoration/status.md) | [Runtime integration](../stages/01-runtime/README.md) |
| [Consumer rules and ports](pyvalue-rc-restoration/porting/status.md) | [Consumer workstream](../stages/01-runtime/workstreams/consumers.md) |
| [Type/cache admission](pyvalue-rc-restoration/typecache/status.md) | [Type-system workstream](../stages/01-runtime/workstreams/type-system.md) |
| [Scoped parsers](pyvalue-rc-restoration/native-parsers/status.md) | [Native-boundary workstream](../stages/01-runtime/workstreams/native-boundaries.md) |
| [Merged build evidence](pyvalue-rc-restoration/merged-build/status.md) | [Integration workstream](../stages/01-runtime/workstreams/integration.md) |
| [Method/tracing system](jit-e2e-system/status.md) | [Method stage](../stages/03-method-jit/README.md) |
| [Qualified interim RC](rc-integration/status.md) | [Baseline stage](../stages/02-baseline/README.md) |

[Tracing-GC work is closed](gc-parity-deadline/decision-20261101.md).
Other old issue directories and archived boards are historical evidence,
not live assignments. Git retains the earlier checkpoint-by-checkpoint board;
the [component index](pyvalue-rc-restoration/source-components-20261102-v1.json)
and [short log](../WORKLOG.md) retain commit/evidence references.

New experiments use the [template](_template/status.md), have one parent
workstream, and keep design, independent review, implementation, raw checks,
decision and commit evidence together. The coordinator validates their composition.
