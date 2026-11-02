# Current work

Current milestone: complete universal PyValues under RC, then measure the
register interpreter with JIT disabled. Restoration HEAD is `e645682dab5` on
`boil/pyvalue-rc-universal`: twenty-three reviewed source commits; typeobject.c is the
remaining original held file. The latest merged diagnostic build has 152
objects present and failed to link. No restored runtime, qualification or performance pass is claimed.
[State](pyvalue-rc-restoration/status.md), [process](../PROCESS.md) and
[gates](../design/validation.md) own details.

## Now and next

1. Object, positional Clinic, tokenizer/bytes and C API call bridges are committed.
   Core mutation dependencies have six more files checkpointed; reviewed typeobject
   provider/core/constructor/graph deltas remain held. The MRO providers and one
   immediate public wrapper are frozen for state_audit's code review. A cheaper
   Luna agent completed the first AST batch: one producer rule covers 74 reads;
   root reviewed and committed it. Luna designs the remaining AST constructor
   port. The coordinator integrates against the actual merged build failures.
2. The coordinator prioritizes shared blockers from the latest actual build,
   reviews each design, assigns independent code review and commits useful ports.
   Old agents remain stopped; compatibility approvals are not a gate.
3. Complete remaining type scalar return/call bridges and the separate type-cache
   correctness proof; batch remaining core and bundled consumers under reviewed
   [porting rules](pyvalue-rc-restoration/porting/status.md). Correct owning boxed
   C views are allowed; further direct-value conversion is later tuning.
4. Build the merged source, execute focused runtime tests and applicable broad
   qualification, then run the matched interpreter benchmark. Resume JIT work
   on the unified value contract.

## Active and queued experiments

| Experiment | Stage | Owner | Next step |
| --- | --- | --- | --- |
| [Native callers and scoped parsers](pyvalue-rc-restoration/native-parsers/status.md) | Call and parser dependencies committed | Coordinator | Remaining gateways/parser families and merged runtime witnesses |
| [Core and extension consumers](pyvalue-rc-restoration/core-consumers/status.md) | Call bridges committed; type_new/graph reviewed pending | wiki_audit and coordinator | C3/MRO, remaining parser/startup/container ports |
| [Porting batches](pyvalue-rc-restoration/porting/status.md) | AST sequence batch committed; constructor design next | porting_census (Luna); coordinator reviews | Remaining AST constructor; reuse reviewed batch contracts |
| [Canonical type cache](pyvalue-rc-restoration/typecache/status.md) | MRO14 plus immediate wrapper frozen for code review | state_audit reviews wiki_audit's code; coordinator integrates | Review, then transaction/guards/static/lifecycle closure |
| [Value-native VM iteration](pyvalue-rc-restoration/iterator-vm/status.md) | Tuning after safe baseline | Unassigned | Remove boxing from correct iterator bridges and measure |
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
| [Canonical cache provider](pyvalue-rc-restoration/typecache/status.md) | `8d5a3d6af1b`, six files; one reviewed delta pending | Actual cache/fixture TUs and objects pass; core mutation/guard/lifecycle still required |
| [Positional Clinic](pyvalue-rc-restoration/native-parsers/positional-owners/status.md) | `be15f9856b5`, 19 paths, 20 production wrappers | 270 producer/eight host controls; class/code TUs and objects pass; six candidate methods pending |
| [Object composition](pyvalue-rc-restoration/core-consumers/object-attributes/status.md) | `e52539e7f68`, four paths; full pending file reviewed | Full TU/object and 20 host controls pass; 20 candidate methods pending |
| [Tokenizer and bytes readers](pyvalue-rc-restoration/core-consumers/small-readers/status.md) | `639891a2e81`, five paths | Three full TUs/objects and four host controls pass; nine candidate methods pending |
| [Core mutation component](pyvalue-rc-restoration/typecache/core-mutation/status.md) | `7b923d2b935`, six files; reviewed typeobject delta pending | Dict/cache/fixture TUs and objects pass; remaining typeobject diagnostics and runtime holds |
| [Object C call bridges](pyvalue-rc-restoration/native-parsers/object-call-bridges/status.md) | `a5563d833a7`, three files | Two full TUs/objects pass without diagnostics; nine host controls; 17 candidate methods pending |
| [Generated AST sequence batch](pyvalue-rc-restoration/porting/status.md) | `e645682dab5`, one producer/74 expansions plus tests | Exact other output; same-header syntax diagnostics 81→7, TU still fails in unchanged constructor; three host controls, candidate tests pending |
| [Merged diagnostic build](pyvalue-rc-restoration/merged-build/status.md) | Make failed at `7b923d2b935` plus held typeobject/type_new; 152 objects present | 540 repeated/cascading diagnostics, no executable; later graph/call ports checked separately |
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
