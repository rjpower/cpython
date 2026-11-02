# Current work

Current milestone: complete universal PyValues under RC, then measure the
register interpreter with JIT disabled. Restoration HEAD is `3296af9ba58` on
`boil/pyvalue-rc-universal`: sixteen reviewed source commits, two held object/type
files. The latest merged diagnostic build has 145 objects but failed
to link. No restored runtime, qualification or performance pass is claimed.
[State](pyvalue-rc-restoration/status.md), [process](../PROCESS.md) and
[gates](../design/validation.md) own details.

## Now and next

1. State_audit designs the fixed-arity positional Clinic increment from actual
   constructor diagnostics. The instance/subclass consumer is committed.
   Wiki_audit implements the accepted seven-path type-cache provider increment;
   mutation/guard/lifecycle dependencies remain separate runtime holds.
2. The coordinator prioritizes shared blockers from the latest actual build,
   reviews each design, assigns independent code review and commits useful ports.
   Old agents remain stopped; compatibility approvals are not a gate.
3. Complete object/type scalar return/call bridges and the separate type-cache
   correctness proof; port the remaining core and bundled consumers.
4. Build the merged source, execute focused runtime tests and applicable broad
   qualification, then run the matched interpreter benchmark. Resume JIT work
   on the unified value contract.

## Active and queued experiments

| Experiment | Stage | Owner | Next step |
| --- | --- | --- | --- |
| [Native callers and scoped parsers](pyvalue-rc-restoration/native-parsers/status.md) | Three reviewed dependencies committed | Coordinator | Remaining gateways/parser families and merged runtime witnesses |
| [Core and extension consumers](pyvalue-rc-restoration/core-consumers/status.md) | Positional Clinic design; instance/subclass committed | state_audit; coordinator reviews | Owning fixed-arity constructor views; remaining getargs/AST/bundled ports |
| [Canonical type cache](pyvalue-rc-restoration/typecache/status.md) | First provider increment in implementation | wiki_audit; coordinator reviews | Canonical cache/admission and seven-path review; later mutation/guard/lifecycle closure |
| [Value-native VM iteration](pyvalue-rc-restoration/iterator-vm/status.md) | Next design | Unassigned | Connect real generic iteration to canonical providers and review observer/reentry behavior |
| [Integration and interpreter checkpoint](pyvalue-rc-restoration/integration/status.md) | Build diagnostics → ports | Coordinator | Close concrete compile/link blockers, rebuild, qualify and measure |
| [Method and tracing JIT](jit-e2e-system/status.md) | Deferred to interpreter checkpoint | Coordinator | Reconcile source/runtime with universal values before further native validation |

## Reviewed checkpoints and outcomes

These source commits are dependent and unqualified; their tests still need a
linked restored runtime. Primary-worktree documentation is separate.

| Experiment | Source commit or outcome | Remaining validation |
| --- | --- | --- |
| [Wiki reset](wiki-reset/status.md) | Documentation accepted; `34486c7090f` | Maintain current assignments and Loom mirrors |
| [Canonical value foundation](pyvalue-rc-restoration/foundation/status.md) | `359ea4732de` | Merged runtime |
| [Canonical storage](pyvalue-rc-restoration/container-storage/status.md) | `edab780a715`, 38 files | Merged container/lifetime routes |
| [Private ABI namespace](pyvalue-rc-restoration/abi-namespace/status.md) | `359d10f4d4d`, five files | Runtime cache/extension identity |
| [Dictionary composition](pyvalue-rc-restoration/dictionary-composition/status.md) | `9b4ff426d1a`, 13 files; includes [builtins fix](pyvalue-rc-restoration/builtins-bridge/status.md) | Legacy closure, real dictionary/watcher routes |
| [Symtable](pyvalue-rc-restoration/core-consumers/symtable/status.md) | `23cec84d282`, two files | Candidate semantic tests |
| [Shared VM/compiler/native composition](pyvalue-rc-restoration/shared-composition/status.md) | `c21d0a374ee`, 44 path actions | Three excluded object files; merged native/VM routes |
| [Codegen](pyvalue-rc-restoration/core-consumers/codegen/status.md) | `ebc9740c4d5`, two files | Candidate semantic tests |
| [Sys.flags](pyvalue-rc-restoration/core-consumers/sysflags/status.md) | `015eb226fb3`, two files | Candidate snapshot/reentry tests |
| [Dictionary API cut](pyvalue-rc-restoration/dict-boundary-closure/status.md) | `18b6053c75f`, 12 files | Initial inventory: 50 core/startup, 37 bundled and 24 C test/example sites; later ports reduce it |
| [Exception/unraisable consumers](pyvalue-rc-restoration/core-consumers/errors/status.md) | `d1c8041c78c`, two files | Candidate matching/hook tests |
| [Canonical dictionary readers](pyvalue-rc-restoration/value-readers/status.md) | `99012f782ea`, five files | Four real symbols and Dict/fixture TUs pass; 13 candidate methods unexecuted |
| [Generic NativeScope adapters](pyvalue-rc-restoration/native-parsers/generic-adapters/status.md) | `1a2adf3da57`, two files | Actual TU/object pass; caller integration/runtime pending |
| [Scoped parser/constructors](pyvalue-rc-restoration/native-parsers/parser-owners/status.md) | `55d410ca39f`, 18 paths | 267 producer checks; Struct object pass; other old parser errors retained |
| [Instance/subclass consumers](pyvalue-rc-restoration/core-consumers/abstract/status.md) | `3296af9ba58`, two paths | Actual TU/object and 23 host controls pass; twelve candidate methods pending |
| [Caller transaction](pyvalue-rc-restoration/native-parsers/caller-transaction/status.md) | `6780ba211ac`, ten paths | Actual evaluator object; 50 host producer checks; eight candidate methods pending |
| [Marshal consumers](pyvalue-rc-restoration/core-consumers/marshal/status.md) | `69a1b3ea533`, two files | Actual TU/object and nine host controls pass; 13 candidate methods pending |
| [Merged diagnostic build](pyvalue-rc-restoration/merged-build/status.md) | Make failed at `6780ba211ac`; 145 objects, no executable | 612 repeated/cascading diagnostics; required consumer ports remain |
| [First diagnostic build](pyvalue-rc-restoration/first-build/status.md) | Configure passed; build failed; 141 objects | 616 repeated/cascading diagnostics across 96 source/header paths to resolve |

Earlier component reviews, including compiler metadata, owned dictionary entries
and selected generic native calls, are indexed in the
[component record](pyvalue-rc-restoration/source-components-20261102-v1.json).
Their implementation is checkpointed; component review does not prove runtime
composition. New source assignments need actual owners and reviewed scope.

## Prior outcomes

[RC integration](rc-integration/status.md) is closed and accepted for its declared
profile. [Tracing-GC work](gc-parity-deadline/decision-20261101.md) is closed.
Prior JIT/IR/backend results and rejected experiments remain in the
[historical board](../archive/work-board-before-wiki-reset-20261102.md) and
[historical log](../archive/WORKLOG-before-wiki-reset-20261102.md). Old assignments
and recommendations are not current instructions.
