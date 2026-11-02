# Current work

Current milestone: complete universal PyValues under RC, then measure the
register interpreter with JIT disabled. Restoration HEAD is `df30266b3b4` on
`boil/pyvalue-rc-universal`: forty-three reviewed source commits; typeobject.c is the
remaining original held file. The latest merged diagnostic build has 166
objects present and failed to link. No restored runtime, qualification or performance pass is claimed.
[State](pyvalue-rc-restoration/status.md), [process](../PROCESS.md) and
[gates](../design/validation.md) own details.

## Now and next

1. The first safe universal-value register-interpreter baseline remains the goal.
   Merged v13 failed before linking: 324 compiler-error observations, 166 objects
   present, all 7,349 inputs unchanged. Focused component passes are not a runtime pass.
2. Both type-reader packets are committed. Sol designs seven bootstrap type
   readers and four remaining Clinic producers; the shared parser engine design
   awaits peer review. Luna inventories remaining itertools reader patterns. Root
   gates the rules, fans out accepted batches, integrates and validates them.
3. The counter foundation and thirteen slot wrappers are committed. Full MRO
   publication needs graph/slot rollback, repeated-receiver and dependency proof;
   the separate spec-constructor envelope awaits a root/peer design gate.
4. Close remaining core/bundled consumer and type correctness blockers, link,
   run focused and broad qualification, then measure against matched upstream
   and interim RC with JIT disabled. Direct-value tuning and JIT follow.

## Active and queued experiments

| Experiment | Stage | Owner | Next step |
| --- | --- | --- | --- |
| [Native callers and scoped parsers](pyvalue-rc-restoration/native-parsers/status.md) | Flat kernel/five callers committed; shared backend design frozen | Sol 6.1 peer review queued; root gate | Gate finite engine foundation, then backend/public/caller closure |
| [Core and extension consumers](pyvalue-rc-restoration/core-consumers/status.md) | Call bridges committed; type_new/graph reviewed pending | Coordinator; Sol design/review | C3/MRO, remaining parser/startup/container ports |
| [Porting batches](pyvalue-rc-restoration/porting/status.md) | Six type/four MRO readers committed; next rules in design | Sol bootstrap/Clinic design; Luna itertools inventory; root gate | Independent rule review, bounded bulk edits, compose and rebuild |
| [Canonical type cache](pyvalue-rc-restoration/typecache/status.md) | Recording-only counter foundation committed; full MRO transaction needs revision | Root gate; Sol design/review when assigned | Review spec envelope; resolve graph/slot/reentry proof |
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
| [AST constructor](pyvalue-rc-restoration/porting/status.md) | `b1dce616174`, three paths | Full TU/object pass with no diagnostics; four host controls/three older-host skips; all seven candidate tests unexecuted |
| [List sort assertions](pyvalue-rc-restoration/porting/status.md) | `4bf934bb4ec`, exact two canonical presence checks | Actual list object builds; runtime qualification pending |
| [Unified keyword Clinic](pyvalue-rc-restoration/porting/status.md) | `180b4b6d9af`, 31 paths; 48 production/three fixture wrappers | 285 host checks, 27 configured TU probes/six whole TUs pass; selected wrappers have no diagnostics; candidate execution pending |
| [Pure predicates](pyvalue-rc-restoration/porting/status.md) | `69a2750540e`, three functions/two files | Exact other bytes; native errors 6→3 and 11→10, zero selected diagnostics; both full TUs still fail, runtime pending |
| [Core14 readers](pyvalue-rc-restoration/porting/status.md) | `e789685426a`, six sources/fourteen bodies plus tests | Configured TU errors 53→39; no selected diagnostics; AST unparser TU/object and eight host controls pass; candidate execution pending |
| [Template readers](pyvalue-rc-restoration/porting/status.md) | `e0dfcd4d7ff`, three bodies/two paths | Full TU/object pass, 7 errors/2 warnings→0; three host controls; candidate execution pending |
| [TypeVar tuple-vararg parser](pyvalue-rc-restoration/porting/status.md) | `bef49ec8fc9`, four paths; one additional production wrapper | Prior51 keyword/20 positional exact; 311 producer passes/116 fixture skips; TU 10→8, selected clean; candidate execution pending |
| [Descriptor tuple calls](pyvalue-rc-restoration/porting/status.md) | `ca594319d0b`, two bodies/two paths | Full TU/object pass, four errors→0; three host controls; candidate execution pending |
| [Shared bytes/bytearray join](pyvalue-rc-restoration/porting/status.md) | `87ec5704d6c`, two shared bodies/three paths | Eight host controls; native selected functions clean in both instantiations, whole TUs still fail; candidate execution pending |
| [ABC selected pairs](pyvalue-rc-restoration/porting/status.md) | `de422d7fbb3`, one body/two paths | Nine host controls; full-TU errors 7→1, selected 6→0; candidate execution pending |
| [Clinic helper constructors](pyvalue-rc-restoration/porting/status.md) | `fa398d21618`, one producer/seven constructors/21 generated bodies | 316 producer passes/116 unavailable fixture skips; two host controls; selected native errors zero, four full TUs still fail |
| [Constructor argument boundary](pyvalue-rc-restoration/porting/status.md) | `d2f39118c93`, one previously reviewed held body and its test | Live source unchanged by checkpoint; twelve host controls, candidate unexecuted |
| [Construction readers](pyvalue-rc-restoration/porting/status.md) | `913c15ce15b`, ten readers/caller pins/two paths | Selected errors 11→0/full TU 23→12, warnings unchanged; six host controls; candidate unexecuted |
| [Merged diagnostic v11](pyvalue-rc-restoration/merged-build/status.md) | Probe at checkpoint39 plus held typeobject | All 7,345 sources unchanged; 350 errors/164 objects present; no executable |
| [Operator/LRU readers](pyvalue-rc-restoration/porting/status.md) | `d1a9a80d25b`, fifteen bodies/three paths | Full operator object passes; selected LRU clean, other functools errors remain; seven host controls; candidate unexecuted |
| [Flat-format parser Stage1](pyvalue-rc-restoration/native-parsers/status.md) | `aa7a4335ddf`, kernel/five consumers/three fixtures/six paths | No new native diagnostics; fixture object passes; remaining raw parser closure mandatory; candidate unexecuted |
| [Merged diagnostic v12](pyvalue-rc-restoration/merged-build/status.md) | Probe at checkpoint41 plus held typeobject | All 7,347 inputs unchanged; 332 errors/166 objects; no executable |
| [Six type readers](pyvalue-rc-restoration/porting/status.md) | `0fd003e9c4b`, six bodies/two paths | Selected 6 errors/4 warnings→0; full TU 12→7 errors/23→19 warnings with one first-use relocation; candidate unexecuted |
| [Four MRO-adjacent readers](pyvalue-rc-restoration/porting/status.md) | `df30266b3b4`, four bodies/two paths | Selected 5 errors/3 warnings→0; full TU 7→4/19→16 with two first-use relocations; five carried host controls; candidate unexecuted |
| [Merged diagnostic v13](pyvalue-rc-restoration/merged-build/status.md) | Probe at checkpoint43 plus held typeobject | All 7,349 inputs unchanged; 324 errors/166 objects; no executable |
| [MRO counter foundation](pyvalue-rc-restoration/typecache/status.md) | `5a9d4841a67`, three paths/recording only | Corresponding-layout native comparison has no new diagnostics; counter/runtime and transaction authority pending |
| [Type slot wrappers](pyvalue-rc-restoration/porting/status.md) | `ef48e73fb24`, thirteen bodies/two paths | Selected errors 15→0/full TU 38→23, warnings unchanged; six host controls; candidate unexecuted |
| [Merged diagnostic v10](pyvalue-rc-restoration/merged-build/status.md) | Probe at checkpoint37 plus held typeobject | All 7,344 sources unchanged; 361 errors/164 objects present; no executable |
| [Iterator constructors](pyvalue-rc-restoration/porting/status.md) | `aa46fc4fd01`, three bodies/two paths | Native selected7→0/full TU34→27 errors, zero warnings; seven host passes/one skip; candidate unexecuted |
| [Merged diagnostic v9](pyvalue-rc-restoration/merged-build/status.md) | Probe at checkpoint35 plus held typeobject | All 7,343 sources unchanged; 376 errors/164 objects present; no executable |
| [Merged diagnostic v8](pyvalue-rc-restoration/merged-build/status.md) | Probe at checkpoint32 plus held typeobject | All 7,342 sources unchanged; 392 errors/164 objects present; no executable |
| [Merged diagnostic v7](pyvalue-rc-restoration/merged-build/status.md) | Probe at e0dfcd4d7ff plus then-unaccepted TypeVar v2 and held typeobject | All 7,342 sources unchanged; 403 errors/163 objects present; no executable; TypeVar subsequently reviewed/committed |
| [Merged diagnostic v6](pyvalue-rc-restoration/merged-build/status.md) | Early probe at 4bf934bb4ec plus then-unaccepted frozen Clinic v2 and held typeobject | All 7,341 sources unchanged; 430 errors/161 objects present; no executable; Clinic subsequently reviewed/committed |
| [Merged diagnostic v5](pyvalue-rc-restoration/merged-build/status.md) | Early compiler probe of b1dce616174 plus unaccepted Clinic39-v1 and held typeobject | All 7,341 sources unchanged; 434 errors; no executable; Clinic source still needs review corrections |
| [MRO providers](pyvalue-rc-restoration/typecache/mro-providers/status.md) | Fifteen bodies accepted at d0721997; exact delta held | Twelve host controls; full TU still fails with 38 errors/23 warnings in unchanged functions; 41 candidate methods pending |
| [Merged diagnostic v4](pyvalue-rc-restoration/merged-build/status.md) | Make failed at e645682dab5 plus reviewed held typeobject deltas; 154 objects present | All 7,339 sources unchanged; 519 repeated/cascading errors; no executable |
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
