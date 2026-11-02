# Current work

Current milestone: restore universal PyValues under RC, integrate the result,
then measure the register interpreter with JIT disabled. The interim RC images
are qualified; the foundation, containers, dictionary, ABI and symtable have source commits.
Dependent consumers remain in progress; the restored runtime is not built or measured.
See [restoration state](pyvalue-rc-restoration/status.md) for exact tree/evidence
identities. [Process](../PROCESS.md) and [gates](../design/validation.md) govern work.

## Now and next

1. The wiki reset passed fresh reviews. Previous agents stay stopped. Fresh
   state_audit designs the sys.flags port; wiki_audit reviews shared VM/compiler/
   native composition and the coordinator's codegen design.
2. The coordinator commits reviewed source in dependency order and plans the
   first coherent merged build. Source review does not count as integration.
3. Assign fresh implementation owners to disjoint consumer/iterator/parser
   increments. Resolve type-cache correctness separately; validate composition.
4. Run focused candidate tests, applicable broad qualification, then the matched
   interpreter benchmark. Resume JIT validation on the unified value contract.

## Experiments

Each row is one current increment; its linked record owns detail and evidence.
Source-reviewed rows still need commit/integration and runtime validation.

| Experiment | Stage | Owner | Next step |
| --- | --- | --- | --- |
| [Wiki and coordinator reset](wiki-reset/status.md) | Accepted documentation | Coordinator; fresh wiki/state auditors | Complete; use this wiki for new assignments |
| [Canonical value foundation](pyvalue-rc-restoration/foundation/status.md) | Source committed | Coordinator | `359ea4732de`; integrate dependent storage/consumer commits before building |
| [Canonical container storage](pyvalue-rc-restoration/container-storage/status.md) | Source committed | Coordinator; wiki_audit reviewed | `edab780a715`; dependent source, not runnable |
| [Private universal-value ABI](pyvalue-rc-restoration/abi-namespace/status.md) | Source committed | Coordinator; wiki_audit reviewed | `359d10f4d4d`; candidate namespace tests pending |
| [Dictionary composition](pyvalue-rc-restoration/dictionary-composition/status.md) | Source committed | Coordinator; wiki_audit reviewed | `9b4ff426d1a`; six legacy functions remain compile blockers |
| [Compiler metadata dictionaries](pyvalue-rc-restoration/compiler-metadata/status.md) | Source reviewed | Coordinator | Commit the six-function increment with its dependencies |
| [Owned dictionary entry API](pyvalue-rc-restoration/owned-dict-entry/status.md) | Source reviewed | Coordinator | Commit providers; port consumers without borrowed scalar boxes |
| [Selected generic native calls](pyvalue-rc-restoration/native-callers/status.md) | Source reviewed | Coordinator | Commit exact ordinary/KW caller changes; test actual routes in a candidate |
| [Symtable consumers](pyvalue-rc-restoration/core-consumers/symtable/status.md) | Source committed | state_audit; wiki_audit reviewed; coordinator committed | `23cec84d282`; actual candidate tests pending |
| [Scalar builtins bridge](pyvalue-rc-restoration/builtins-bridge/status.md) | Source committed with dictionary | state_audit; wiki_audit reviewed | Included in `9b4ff426d1a`; candidate regression pending |
| [Remaining core and extension consumers](pyvalue-rc-restoration/core-consumers/status.md) | Sys.flags design; codegen design review | state_audit; coordinator | Close concrete removed-API consumers; marshal/errors and broader ports remain |
| [Value-native VM iteration](pyvalue-rc-restoration/iterator-vm/status.md) | Next design | Unassigned after reset | Connect real generic iteration to canonical providers and review observer/reentry behavior |
| [Canonical type cache](pyvalue-rc-restoration/typecache/status.md) | Revise design | Coordinator | Close live-mutation/static-lifetime proof before implementation |
| [Native parsers and registration](pyvalue-rc-restoration/native-parsers/status.md) | Queued | Unassigned after reset | Design one real scoped parser/registration route, then port wrapper families |
| [Restoration integration and interpreter checkpoint](pyvalue-rc-restoration/integration/status.md) | Source checkpoints | Coordinator; wiki_audit reviews | Commit reviewed shared composition, then build/qualify/measure |
| [Method and tracing JIT](jit-e2e-system/status.md) | Deferred to interpreter checkpoint | Coordinator | Reconcile source/runtime with universal values before further native validation |

Earlier restoration components are indexed in the
[component record](pyvalue-rc-restoration/source-components-20261102-v1.json).
New source assignments require actual owners and reviewed scope; old agents,
scripts and deadlines are not resumed from this table.

## Prior outcomes

[RC integration](rc-integration/status.md) is closed and accepted for its declared
profile. [Tracing-GC work](gc-parity-deadline/decision-20261101.md) is closed.
Prior JIT/IR/backend results and rejected experiments remain in the
[historical board](../archive/work-board-before-wiki-reset-20261102.md) and
[historical log](../archive/WORKLOG-before-wiki-reset-20261102.md). Their old
assignments and recommendations are not current instructions.
