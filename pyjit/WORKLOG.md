# Experiment outcomes

One short line per concluded experiment, linked to its detailed record.
[Current work](issues/README.md) owns active status. The
[pre-reset worklog](archive/WORKLOG-before-wiki-reset-20261102.md) is preserved
verbatim; new checkpoint transcripts do not belong here.

- 2026-11-02 — [Wiki reset](issues/wiki-reset/status.md): replaced stale navigation, process/gates and assignments; preserved history, refreshed Loom goal and passed independent documentation/state review.
- 2026-11-02 — [PyValue foundation](issues/pyvalue-rc-restoration/foundation/status.md): committed reviewed dependency checkpoint `359ea4732de`, preserving all later working source; not yet runnable or integrated.
- 2026-11-02 — [Canonical containers](issues/pyvalue-rc-restoration/container-storage/status.md): committed 38 independently reviewed storage/provider files as `edab780a715`; preserved later changes; dependent source, not runnable.
- 2026-11-02 — [Private value ABI](issues/pyvalue-rc-restoration/abi-namespace/status.md): committed reviewed five-file namespace change `359d10f4d4d`; generated configure exact; runtime checks pending.
- 2026-11-02 — [Dictionary composition](issues/pyvalue-rc-restoration/dictionary-composition/status.md): committed 13 reviewed files as `9b4ff426d1a`, including scalar-builtins fix; six legacy compile failures remain explicit.
- 2026-11-02 — [Symtable consumers](issues/pyvalue-rc-restoration/core-consumers/symtable/status.md): committed corrected canonical port `23cec84d282`; actual syntax and seven host controls pass; candidate tests pending.
- 2026-11-02 — [Shared composition](issues/pyvalue-rc-restoration/shared-composition/status.md): committed 44 reviewed path actions as `c21d0a374ee`; three object/type files held for concrete scalar/cache findings.
- 2026-11-02 — [Codegen](issues/pyvalue-rc-restoration/core-consumers/codegen/status.md): committed reviewed two-file port `ebc9740c4d5`; full TU/new-config object and six host controls pass; candidate tests pending.
- 2026-11-02 — [Sys.flags](issues/pyvalue-rc-restoration/core-consumers/sysflags/status.md): committed reviewed port `015eb226fb3`; full TU/new-config object pass; host policy differences preserved, candidate tests pending.
- 2026-11-02 — [First restored build](issues/pyvalue-rc-restoration/first-build/status.md): configure passed, make failed with 141 objects and no executable; grouped 616 repeated/cascading diagnostics, source unchanged, shared boundary ports prioritized.
- 2026-11-02 — [Dictionary API cut](issues/pyvalue-rc-restoration/dict-boundary-closure/status.md): committed reviewed 12-file removal as `18b6053c75f`; Dict/fixture TUs and real Dict object compile pass; 111 explicit caller sites remain to port.
- 2026-11-02 — [Exception consumers](issues/pyvalue-rc-restoration/core-consumers/errors/status.md): committed reviewed port `d1c8041c78c`; syntax/object compile and applicable host controls pass; seven candidate methods unexecuted.
- 2026-11-02 — [Canonical dictionary reads](issues/pyvalue-rc-restoration/value-readers/status.md): committed reviewed five-file increment `99012f782ea`; real Dict/fixture TUs, object and four symbol definitions pass; 13 candidate methods unexecuted.
- 2026-11-02 — [Generic NativeScope adapters](issues/pyvalue-rc-restoration/native-parsers/generic-adapters/status.md): reviewed two-file dependency committed as `1a2adf3da57`; full TU/object pass; caller integration and runtime pending.
- 2026-11-02 — [Scoped parser/constructors](issues/pyvalue-rc-restoration/native-parsers/parser-owners/status.md): reviewed 18-path dependency committed as `55d410ca39f`; producer/Struct checks pass; old parser errors and candidate execution remain.
- 2026-11-02 — [Marshal consumers](issues/pyvalue-rc-restoration/core-consumers/marshal/status.md): corrected reviewed port committed as `69a1b3ea533`; actual TU/object and nine host controls pass; 13 candidate methods unexecuted.
- 2026-11-02 — [Caller transaction](issues/pyvalue-rc-restoration/native-parsers/caller-transaction/status.md): reviewed ten-path increment committed as `6780ba211ac`; evaluator object/50 producer checks pass; eight candidate methods pending.
- 2026-11-02 — [Merged diagnostic build](issues/pyvalue-rc-restoration/merged-build/status.md): make exit 2, 145 objects and no executable; all 7,332 source inputs unchanged; concrete consumer failures remain.
- 2026-11-02 — [Instance/subclass consumers](issues/pyvalue-rc-restoration/core-consumers/abstract/status.md): reviewed two-file port committed as `3296af9ba58`; actual TU/object and 23 host controls pass; twelve candidate methods pending.
- 2026-11-02 — [Canonical cache provider](issues/pyvalue-rc-restoration/typecache/status.md): reviewed six-file dependency committed as `8d5a3d6af1b`; cache/fixture TUs and objects pass; reviewed typeobject delta and mutation/guard/lifecycle remain held.
- 2026-11-02 — [Positional Clinic](issues/pyvalue-rc-restoration/native-parsers/positional-owners/status.md): reviewed 19-path port committed as `be15f9856b5`; 20 wrappers, 270 producer/eight host controls and class/code objects pass; old parser errors and six candidate methods remain.
- 2026-11-02 — [Object composition](issues/pyvalue-rc-restoration/core-consumers/object-attributes/status.md): full pending object/header bridge committed as `e52539e7f68` after final independent review; TU/object and 20 host controls pass; candidate methods remain unexecuted.
- 2026-11-02 — [Tokenizer/bytes readers](issues/pyvalue-rc-restoration/core-consumers/small-readers/status.md): reviewed five-path port committed as `639891a2e81`; three TUs/objects and four host controls pass; nine candidate methods pending.
- 2026-11-02 — [Core mutation](issues/pyvalue-rc-restoration/typecache/core-mutation/status.md): reviewed six-file dependency committed as `7b923d2b935`; Dict/cache/fixture objects build; reviewed typeobject delta and runtime closure remain held.
- 2026-11-02 — [Merged diagnostic v3](issues/pyvalue-rc-restoration/merged-build/status.md): make exit 2, 152 objects present, no executable; all 7,337 sources unchanged; 540 repeated/cascading diagnostics retained.
- 2026-11-02 — [Object C call bridges](issues/pyvalue-rc-restoration/native-parsers/object-call-bridges/status.md): mutually reviewed three-path port committed as `a5563d833a7`; two full TUs/objects and nine host controls pass; 17 candidate methods pending.
- 2026-11-02 — [Porting census](issues/pyvalue-rc-restoration/porting/status.md): historical build census records 540 error observations/425 locations/90 paths, not edits; reviewed ownership-rule batching adopted with cheaper agents.
- 2026-11-02 — [AST sequence batch](issues/pyvalue-rc-restoration/porting/status.md): cheaper-agent one-producer/74-read port independently reviewed and committed as e645682dab5; other generated text exact, syntax diagnostics 81→7 in unchanged constructor, three host controls; candidate execution pending.
- 2026-11-02 — [MRO providers](issues/pyvalue-rc-restoration/typecache/mro-providers/status.md): fifteen-body d0721997 delta accepted after two corrections; twelve host controls, native 38 errors/23 warnings all in unchanged functions; held with remaining typeobject work, candidate unexecuted.
- 2026-11-02 — [Merged diagnostic v4](issues/pyvalue-rc-restoration/merged-build/status.md): make exit 2, 154 objects present, no executable; all 7,339 source paths unchanged; 519 repeated/cascading errors retained.
- 2026-11-02 — [AST constructor](issues/pyvalue-rc-restoration/porting/status.md): Sol-reviewed Luna port committed as b1dce616174; full TU/object pass without diagnostics, four host controls/three older-host skips; all seven candidate tests unexecuted.
- 2026-11-02 — [Merged diagnostic v5](issues/pyvalue-rc-restoration/merged-build/status.md): early probe of AST24 plus unaccepted Clinic39-v1; 434 errors, 160 objects present, all 7,341 sources unchanged, no executable; review corrections remain.
- 2026-11-02 — [List sort assertions](issues/pyvalue-rc-restoration/porting/status.md): independently reviewed exact two canonical-presence replacements committed as 4bf934bb4ec; actual list object builds, runtime pending.
- 2026-11-02 — [Merged diagnostic v6](issues/pyvalue-rc-restoration/merged-build/status.md): early probe of checkpoint25 plus frozen Clinic v2; 430 errors/161 objects present, 7,341 sources unchanged, no executable; Clinic subsequently reviewed/committed.
- 2026-11-02 — [Unified keyword Clinic](issues/pyvalue-rc-restoration/porting/status.md): Sol-reviewed Luna batch committed as 180b4b6d9af, 48 production/three fixture wrappers on one rule; 285 host checks and 27 configured TU probes, no selected-wrapper diagnostics; candidate execution pending.
- 2026-11-02 — [Pure predicates](issues/pyvalue-rc-restoration/porting/status.md): independent source/native review accepted three functions/two files as 69a2750540e; TU errors 6→3/11→10, no selected diagnostics; both TUs still fail and runtime remains pending.
- 2026-11-02 — [Core14 draft](issues/pyvalue-rc-restoration/porting/status.md): REVISE before integration; outside-scope codec brace fails actual syntax, owner cleanup rule and witnesses need finite corrections; original source/failure retained.
- 2026-11-02 — [Core14 readers](issues/pyvalue-rc-restoration/porting/status.md): corrected/reviewed seven-path batch committed as e789685426a; fourteen selected bodies clean, TU errors 53→39, AST unparser TU/object and eight host controls pass; candidate execution pending.
- 2026-11-02 — [Template readers](issues/pyvalue-rc-restoration/porting/status.md): reviewed three-body/two-path port committed as e0dfcd4d7ff; full TU/object pass, 7 errors/2 warnings→0 and three host controls; scalar AST producer exception handled explicitly, candidate execution pending.
- 2026-11-02 — [Merged diagnostic v7](issues/pyvalue-rc-restoration/merged-build/status.md): checkpoint29 plus frozen TypeVar v2/held typeobject; 403 errors/163 objects, 7,342 source inputs unchanged, no executable; TypeVar subsequently reviewed/committed.
- 2026-11-02 — [TypeVar tuple-vararg parser](issues/pyvalue-rc-restoration/porting/status.md): corrected/reviewed four-path port committed as bef49ec8fc9; prior51 keyword/20 positional exact, 311 producer passes/116 fixture skips, native 10→8 with selected wrapper clean; candidate execution pending.
- 2026-11-02 — [Descriptor tuple calls](issues/pyvalue-rc-restoration/porting/status.md): reviewed two-body/two-path port committed as ca594319d0b; full TU/object pass, four errors→0 and three host controls; stable owned vectorcall views reuse existing provider, candidate execution pending.
- 2026-11-02 — [Shared bytes/bytearray join](issues/pyvalue-rc-restoration/porting/status.md): reviewed three-path port committed as 87ec5704d6c; selected native functions clean in both instantiations, eight host controls pass; whole TUs/runtime remain blocked.
- 2026-11-02 — [Merged diagnostic v8](issues/pyvalue-rc-restoration/merged-build/status.md): checkpoint32 plus held typeobject; make exit2, 392 errors/164 objects, 7,342 source inputs unchanged, no executable.
- 2026-11-02 — [ABC selected pairs](issues/pyvalue-rc-restoration/porting/status.md): corrected/reviewed one-body/two-path port committed as de422d7fbb3; selected native errors 6→0/full TU 7→1, nine host controls; candidate execution pending.
- 2026-11-02 — [Clinic helper constructors](issues/pyvalue-rc-restoration/porting/status.md): reviewed producer/seven-constructor port committed as fa398d21618; 21 generated bodies, 690 excluded templates exact, 316 producer passes/116 fixture skips, two host controls, seven native errors removed; runtime pending.

- 2026-11-02 — [Iterator constructors](issues/pyvalue-rc-restoration/porting/status.md): reviewed v2 committed as aa46fc4fd01; selected native errors7→0/full TU34→27, zero warnings; seven host controls/one skip, candidate unexecuted.
- 2026-11-02 — [Merged diagnostic v9](issues/pyvalue-rc-restoration/merged-build/status.md): checkpoint35 plus held typeobject, make exit2/376 errors/164 objects, all7,343 inputs unchanged, no executable.

- 2026-11-02 — [MRO counter foundation](issues/pyvalue-rc-restoration/typecache/status.md): recording-only three-path source committed as `5a9d4841a67`; corresponding-layout native probes add no diagnostics, runtime/transaction pending.
- 2026-11-02 — [Type slot wrappers](issues/pyvalue-rc-restoration/porting/status.md): thirteen-body reviewed batch committed as `ef48e73fb24`; selected errors 15→0/full TU 38→23, warnings unchanged, six host controls, candidate unexecuted.
- 2026-11-02 — [Merged diagnostic v10](issues/pyvalue-rc-restoration/merged-build/status.md): checkpoint37 plus held typeobject, make exit 2/361 errors/164 objects, all 7,344 inputs unchanged, no executable.

- 2026-11-02 — [Constructor boundary checkpoint](issues/pyvalue-rc-restoration/porting/status.md): exact previously reviewed body/test committed as `d2f39118c93`; live source unchanged, twelve host controls, candidate execution pending.
- 2026-11-02 — [Construction readers](issues/pyvalue-rc-restoration/porting/status.md): reviewed ten-reader/caller-pin batch committed as `913c15ce15b`; selected errors 11→0/full TU 23→12, warnings unchanged, six host controls, candidate unexecuted.
- 2026-11-02 — [Merged diagnostic v11](issues/pyvalue-rc-restoration/merged-build/status.md): checkpoint39 plus held typeobject, make exit 2/350 errors/164 objects, all 7,345 inputs unchanged, no executable.

- 2026-11-02 — [Operator/LRU readers](issues/pyvalue-rc-restoration/porting/status.md): corrected v3 committed as `d1a9a80d25b`; full operator object passes, selected LRU clean with other functools errors remaining, seven host controls; candidate unexecuted.
- 2026-11-02 — [Flat-format parser Stage1](issues/pyvalue-rc-restoration/native-parsers/status.md): kernel/five callers/three fixtures committed as `aa7a4335ddf`; configured diagnostic signatures unchanged, fixture object passes, candidate unexecuted; remaining parser closure mandatory.
- 2026-11-02 — [Merged diagnostic v12](issues/pyvalue-rc-restoration/merged-build/status.md): checkpoint41 plus held typeobject, make exit2/332 errors/166 objects, all7,347 inputs unchanged, no executable.
- 2026-11-02 — [Six type readers](issues/pyvalue-rc-restoration/porting/status.md): corrected v2 committed as `0fd003e9c4b`; selected six errors/four warnings→0, full TU 12→7/23→19 with one first-use relocation, five host passes/one skip; candidate unexecuted.
- 2026-11-02 — [Four MRO-adjacent readers](issues/pyvalue-rc-restoration/porting/status.md): corrected v2 committed as `df30266b3b4`; selected 5 errors/3 warnings→0, full TU 7→4/19→16 with two first-use relocations, five carried host controls; candidate unexecuted.
- 2026-11-02 — [Merged diagnostic v13](issues/pyvalue-rc-restoration/merged-build/status.md): checkpoint43 plus held typeobject, make exit2/324 errors/166 objects, all 7,349 inputs unchanged, no executable.
- 2026-11-02 — [Seven bootstrap type readers](issues/pyvalue-rc-restoration/porting/status.md): committed as `b76f2182354`; selected 2 errors/13 warnings→0, full TU 4→3/16→3 with one first-use relocation, six host controls; candidate unexecuted.
- 2026-11-02 — [Clinic4](issues/pyvalue-rc-restoration/porting/status.md): corrected v3 accepted and committed as `5ece1b27fd0`; 287 producer/five host passes, selected8→0, float/tuple objects pass, enum/atexit still fail; candidate unexecuted; v2 REVISE preserved.
- 2026-11-02 — [Pickle3](issues/pyvalue-rc-restoration/porting/status.md): corrected per-function packets composed and committed as `defd38874bf`; full type TU/object 3 errors/3 warnings→0 and eight host controls pass; failed bulk/API attempts preserved, candidate unexecuted.
- 2026-11-02 — [FORMAT foundation A](issues/pyvalue-rc-restoration/native-parsers/status.md): corrected composition committed as `4f2734e77cd`; no new native diagnostics and full fixture object passes; nine candidate methods and backend/public/caller closure pending.
- 2026-11-02 — [Merged diagnostic v14](issues/pyvalue-rc-restoration/merged-build/status.md): checkpoint45 plus held typeobject, make exit2/317 errors/168 objects; all 7,351 inputs unchanged, no executable.
- 2026-11-02 — [Merged diagnostic v15](issues/pyvalue-rc-restoration/merged-build/status.md): checkpoint47 plus held typeobject, make exit2/314 errors/169 objects; all 7,353 inputs unchanged, no executable.
- 2026-11-03 — [Type attribute bridge](issues/pyvalue-rc-restoration/core-consumers/type-getattro/status.md): checkpoint48 `dd26d8e34ca`; protected materialization and optional-caller error propagation, both full TUs/objects pass and four host controls; exact index projection preserves held changes, candidate unexecuted.
- 2026-11-03 — [GenericAlias closure](issues/pyvalue-rc-restoration/core-consumers/genericalias/status.md): checkpoint49 `7a6cba14ed0`; eight functions composed, full TU/object15 errors/2 warnings→0/0, ten host passes/one older-version skip; original corrections/crash evidence retained, candidate pending.
- 2026-11-03 — [Merged diagnostic v16](issues/pyvalue-rc-restoration/merged-build/status.md): checkpoint49 plus held typeobject, make exit2/299 errors/170 objects; all7,355 inputs unchanged, no executable.
- 2026-11-03 — [Lookup/call envelope](issues/pyvalue-rc-restoration/core-consumers/type-scalar/status.md): checkpoint50 `3a3ee2ac368`, ten functions/two forwarding dependencies; full held TU/object pass, six ordinary host controls and two mandatory candidate contracts with recorded upstream differences.
- 2026-11-03 — [Scalar boundaries](issues/pyvalue-rc-restoration/core-consumers/type-scalar/status.md): checkpoint51 `14fc009d42f`, five functions; full held TU/object and five host controls pass; canonical provider rename projected exactly, runtime/fault qualification pending.
- 2026-11-03 — [Merged diagnostic v17](issues/pyvalue-rc-restoration/merged-build/status.md): checkpoint51 plus held foundations, exit2/299 errors/170 objects; all 7,357 inputs unchanged; diagnostic multiset exact to v16, no executable.
- 2026-11-03 — [Datetime closure](issues/pyvalue-rc-restoration/core-consumers/datetime/status.md): checkpoint52 `67210b9cf67`, twelve functions/23 getter uses; full TU 19 errors/3 warnings→0 and object passes, seven host controls; candidate unexecuted.
- 2026-11-03 — [Merged diagnostic v18](issues/pyvalue-rc-restoration/merged-build/status.md): checkpoint52 plus held typeobject, exit2/280 errors/171 objects; all 7,358 inputs unchanged, exactly nineteen datetime errors removed and no added diagnostics; no executable.
- 2026-11-03 — [Itertools next readers](issues/pyvalue-rc-restoration/core-consumers/itertools/status.md): checkpoint53 `5b993d71b1e`, six functions/21 getter uses; native 27→1 unchanged batched error, ten host controls; no full-TU/object/runtime pass.
- 2026-11-03 — [Merged diagnostic v19](issues/pyvalue-rc-restoration/merged-build/status.md): checkpoint53 plus held typeobject, exit2/254 errors/171 objects; all 7,360 inputs unchanged, exactly 26 errors removed with no added diagnostics; no executable.
- 2026-11-03 — [Batched writer](issues/pyvalue-rc-restoration/core-consumers/itertools/status.md): checkpoint54 `a714cb97c14`; final itertools full TU/object clean, four new host controls; candidate pending.
- 2026-11-03 — [Merged diagnostic v20](issues/pyvalue-rc-restoration/merged-build/status.md): checkpoint54 plus held typeobject, 253 errors/172 objects, all 7,361 inputs unchanged; no executable.
- 2026-11-03 — [Enum/map readers](issues/pyvalue-rc-restoration/porting/status.md): checkpoint55 `f7f53c043fc`, enum full TU/object clean, builtin12→11 errors, four host controls; v1 scope violation restored/excluded, guarded v2 independently accepted.
- 2026-11-03 — [Merged diagnostic v21](issues/pyvalue-rc-restoration/merged-build/status.md): checkpoint55 plus held typeobject, 247 errors/173 objects, all 7,362 inputs unchanged; six fewer errors with raw row drift preserved, no executable.
- 2026-11-03 — [Export lifetime E](issues/pyvalue-rc-restoration/native-parsers/status.md): checkpoint56 `6bf198d1ef5`, fixture TU/object clean, getargs twelve errors unchanged; fourteen fixture methods unexecuted.
- 2026-11-03 — [Merged diagnostic v22](issues/pyvalue-rc-restoration/merged-build/status.md): checkpoint56 plus held typeobject, 247 observations/173 objects, all 7,362 inputs unchanged; no executable.
- 2026-11-03 — [Documentation hierarchy](issues/documentation-hierarchy/README.md): renamed project root to pyjit; project/stage/workstream/experiment views and exact coordinator handoff, frozen evidence and user changes preserved; no runtime-source change.
- 2026-11-03 — Process change: propose, peer review, implement, test, post-review, integrate; designs and reviews are short markdown, Git commits are the identity (no seals or receipt JSON), workers never commit, every C review includes a release-mode compile; see [PROCESS](PROCESS.md).
- 2026-11-03 — Type graph/MRO/cache projection: checkpoint57 `5e977ec8cbb`, admitted after independent review; no runtime execution.
- 2026-11-03 — Wave 1 core ports: `337dd518abb`, `5c250397c43`, `494e53f657a`, `bd1520443fd`, `abc8631ce60`; about 50 files moved off deleted borrowed getters, each independently reviewed; new helpers `_PyTuple_ReplaceTakeRef`, `_PyDict_NextBorrowed`, `_PySet_NextBorrowed`.
- 2026-11-03 — PyValue immortal tag: `271f4e435e5` keeps the no-count tag in `PyValue_NewRef`, removing a second encoding of None/True/False.
- 2026-11-03 — Marshal: `6ea1e52db4a` stops the set writer on iteration error.
- 2026-11-03 — Wave 2 extension modules: `62fee2c1656`, `c7194ad146e`, `570b81e0e7a` (stdlib accelerators, pickle, ctypes); not yet linked or run.
- 2026-11-03 — System modules: `0b7f39711d0` (remote debugging, select, socket, curses, termios, posixsubprocess, resource, multibytecodec); not yet linked or run.
- 2026-11-03 — Decision: [native-boundary design](design/native-boundary.md) replaces explicit `_PyArg_OwnerScope` with a per-thread borrow stack, withdraws the four frozen FORMAT templates, deletes `PySequence_Fast_ITEMS`/`GET_ITEM`; packets P1 to P4; under independent review.
- 2026-11-03 — Designs written: [pyvalue](design/pyvalue.md) (12 gaps, under review) and [register IR](design/register-ir.md) (in progress; copy-propagation `f_locals` deviation noted on the interim build).
- 2026-11-03 — Build state: debug (merged v23) and new release build `pyvalue-rc-release-20261103` fail only in `Python/getargs.c` for the core; no executable, no runtime test, no timing. Upstream control `3a440b65695` built and passes `test_int`.
- 2026-11-03 — Test and C API modules: `0ab5bc42f61`; wave 2 done.
- 2026-11-03 — Register IR G1 fixed: `c66a0b29253` stops copy propagation folding a load across code that may run Python; pyc magic 64402.
- 2026-11-03 — PyValue tag fixes: `6a7b3769d62` (ERROR/wrap unowned, one immortality test, `__slots__` traversal), PyValue outside the limited API, stable ABI manifest.
- 2026-11-03 — Native boundary P1 and P2: `acbdf45627d` (per-thread borrow stack, getargs, scopes).
- 2026-11-03 — First runtime fixes: `192694b1523` (AST arena owns strong refs; CALL specialization with inline self; managed-dict slot).
- 2026-11-03 — Eval-loop C frame cut from 35.6 KB to 5.5 KB (upstream about 360 B): `ca8a9d6d827`.
- 2026-11-03 — Remaining modules ported (decimal, ssl, sqlite, zstd, ...): `e704ab282d9`.
- 2026-11-03 — Bring-up fixes: `7f6501d9669`; `_PyType_Lookup` and `PySys_GetObject` deleted; type cache admission; register IR limits raise `SyntaxError`.
- 2026-11-03 — Native boundary P3 and review-1 follow-ups: `56ff66a596b`, `643c69c7244`, `6d39221996d` (Clinic on the borrow stack, `_PyArg_OwnerScope` deleted, borrow scope tests); the first alone does not build.
- 2026-11-03 — First link and run: debug and release builds link and run; 67 shared modules build (decimal, tkinter, ssl, sqlite, zstd headers not installed). Debug suite at 17:31: 611/619 files run, 484 OK, 30 failed, 3 env changed; 48,500 tests, 52 failures. No performance results yet.
- 2026-11-03 — Decision: [pyvalue](design/pyvalue.md) reviewed ([review 1](design/reviews/pyvalue-review-1.md)); D1 to D8 recorded, D3 int51 chosen from the [upstream int-width histogram](bench/results/pyvalue-int-width-20261103.md).
- 2026-11-03 — Decision: [native-boundary](design/native-boundary.md) accepted with changes after [review 1](design/reviews/native-boundary-review-1.md); P4 in progress, post-review running.
- 2026-11-03 — Decision: [register-ir](design/register-ir.md) accepted after [review 2](design/reviews/register-ir-review-2.md) (IR-D1 to IR-D4; one limits rule; names past 4,095 lower to an intrinsic; 8-byte feedback offsets).
- 2026-11-03 — Proposal: [PyValue representation packet](design/proposals/pyvalue-representation-packet.md) accepted after [review](design/reviews/pyvalue-representation-packet-review-1.md); 11 steps, about 42 agent-hours, after triage and P4.
- 2026-11-03 — Landed: representation step 5 (`0bf3ad80ecf`, release contract and retire path; post-review found no bugs, two risks fixed first in step 6) and value API V1 (`f3a20271f26` to `48bb4d6d1d3`: box counters, enforcement macros, `make check-value-api`, `PyValue_AsKnownObjectBorrow`/`Steal`, `pyvalue.h` internal, `PyValue_ERROR` removed). Suites: run=51,511, failures=12 in debug and release.
- 2026-11-04 — Decision: [register IR v2 packet](design/proposals/register-ir-v2-packet.md) accepted (revision 3) after [review 1](design/reviews/register-ir-v2-packet-review-1.md) and [review 2](design/reviews/register-ir-v2-packet-review-2.md): interpreter structure first (activation record, PC out-parameter deleted, tail calls as the only backend after 9b), D4 in two steps, then the format steps; about 117 agent-hours. register-ir.md, native-boundary.md and pyvalue.md updated (`24042ea0492`).
- 2026-11-04 — Landed: step 5 review fixes, representation steps 6a-6c (save/restore sites 766 to 117) and the `PyDict_Pop` status fix (`c4b01b23a54` to `c8a0b4dc54c`). Suites: run=51,511, failures=12 in debug and release.
- 2026-11-04 — Measured: [re-measure after step 6c](bench/results/remeasure-20261104.md): kernels 2.27x upstream (2.64x before step 5, 14% less time), pyperformance 2.53x; exception save/restore 22.3% to 8.2% of the before time; loop cost unchanged and again the largest bucket.
- 2026-11-04 — Landed: representation step 7 (`ab07912b7cb` to `1a9d0a6fbbe`; status APIs, residue, defences back to upstream form, `_PyValue_RestoreException` deleted, perf watcher fix; post-review found no bugs) and register IR v2 step B0a (`54825c08a1a`, the generated instruction contract table; compiled output unchanged). Suites: run=51,538, failures=11 in debug and release.
- 2026-11-04 — Landed: representation step 8 (`5640c46361c` to `f6508007b94`): canonical construction, `PyValue_Is` as a bit compare (the deleted fallback: 0 true results in 3.59 billion calls), the home checker as a store-counting debug assert, object-level value identity. Post-review found one bug (generic `SEND` stored an auto-boxed input), fixed before commit. Suites: run=51,548, failures=11 in debug and release.
- 2026-11-04 — Landed: register IR v2 A1 and A2 (`44eb4e716dd`, `335b86b71e9`) with fixes (`c8637fe7c6b`): the value record only in the 115 handlers that box; `next_instr` by value in the 12 helper families. Release loop 406 KB to 303 KB; kernel time against `54825c08a1a` A1 0.983, A2 1.024, both 0.986 (A2's gain is for A3a's tail calls). Compiled output unchanged. Suites: run=51,552, failures=11 in debug and release.
- 2026-11-04 — Landed: register IR v2 A5a (`3c7ac067f7f` to `3e28e1755bd`): no frame slot holds a borrowed reference; census 510.5M violating stores before, 0 after; post-review found a fail-open gap in the generator's in-place rules (copies into C variables, object steals), fixed before commit. Release loop 308,756 to 279,995 B; kernels 0.939-0.949 of the base. Suites: run=51,557, failures=11.
- 2026-11-04 — Landed: representation steps 9a and 9b (`bf0639df819`, `80bb9c84701`): int51 inline ints in the runtime, interpreter and trace JIT; numeric guards inline-only; magic 64404; review fuzzer matched upstream; kernels 0.964 of the base. And `5de7bd62ef9`: release order around calls matches upstream, five fork tests updated, 9 of 11 long-standing failures fixed. Suites: run=51,571, failures=2 (test_capi refcounts, open for B5).
- 2026-11-04 — Decision: the two `test_capi` refcount failures stay visible (no test guard) until IR v2 B5 decides the call forms; a local passed to a call reads `sys.getrefcount` 2 because the window owns its copy under D4.
- 2026-11-04 — Measured: [second re-measure](bench/results/remeasure-20261104b.md) at `5de7bd62ef9`: kernels 1.73x upstream (2.27x before), pyperformance 1.83x (2.53x); save/restore gone; the loop is 44% of the gap and the largest bucket; words unchanged (0.805). RSS method fixed for page-cache residency (measurement.md).
- 2026-11-04 — Decision: [A5b typed borrow](design/proposals/register-ir-v2-a5b.md) accepted (revision 3) after [review 1 and review 2](design/reviews/register-ir-v2-a5b-review-1.md): `PyValueBorrow` behind same-name `_Generic` reader macros, `-Werror=incompatible-pointer-types` for every in-tree unit, the native scope's argument borrow removed in A5b-3, the proven-inline release rule deferred; four steps, 21.5 h. Design docs synced (`1b8284a9813`).
- 2026-11-04 — Landed: value-form container calls (`2f572ea0ea8`): 11 sites stop boxing inline inputs; post-review found no must-fix, and its should-fix items (a str-key slowdown, a generator borrow-check hole, test gaps) were fixed first. nqueens boxes 541k to 40k per loop. Compiled output unchanged. Suites: run=51,613, failures=2.
- 2026-11-04 — Landed: register IR v2 A5b-1 (`a6002e5854b`): `PyValueBorrow`, the `PyValue_Borrow` view and same-name `_Generic` reader macros; the incompatible-pointer flag for in-tree units; negative compile tests in the suite. Post-review found no must-fix; three should-fix items fixed first. The merge onto `2f572ea0ea8` failed to compile at one new `MakeHeapSafe(PyValue_Borrow(...))`, as designed. Suites: run=51,615, failures=2.
- 2026-11-04 — Decision (user): no box inside the core (value-api.md R27, `82fb8439404`); the PyValue encoding is private to the value headers (R26, `5a859343798`); the fork's JIT leaves the build until the JIT stage (`faddfd1454d`); broad changes are made in bulk (script plus compiler-driven fan-out to parallel Sonnet workers), and A5b-4's renames move into the first value-API wave.
- 2026-11-04 — Landed: upstream CPython's JIT remnants removed (`36aa4a0345a`); `sys._jit` reports the fork's JIT. Suites: run=51,617, failures=2.
- 2026-11-04 — Decision: [A3 tail-call proposal](design/proposals/register-ir-v2-a3.md) accepted (revision 2) after [review 1](design/reviews/register-ir-v2-a3-review-1.md): the JIT's interpreter hooks leave in A3.1 and A3b is dropped; `preserve_none` on Clang and `no_callee_saved_registers` on GCC; the G9 work list is the object-file canary list (200 functions, 56.7% of entries at HEAD; 133 after two helper fixes); four steps, 30 agent-h, about 16.5 h of wall clock.
- 2026-11-04 — Decision (user): delete the fork's JIT source in A3's first step (its own commit; history keeps it). The A3 plan (30 agent-h, 16.5 h wall) is too heavy and keeps upstream's handler shape; revision 3 makes handlers free of addressable locals by construction, leaves the box area to R27, and lands in two bulk-style steps.
- 2026-11-04 — Decision: [value-API route to R27 and V2a-0](design/proposals/value-api-v2a0.md) accepted (revision 2) after [review 1](design/reviews/value-api-v2a0-review-1.md): reviewed mechanism steps (V2a-0, V2a-1a, M1b, M2, M3a/b, M4, G) then compiler-driven waves (W1, W2 core, W4-0 module homes, W4 Modules, W5) on parallel Sonnet workers; value slots inline in `PyTypeObject` as opaque storage; the R27 gate attributes boundary boxes by the code address crossed into; `PySliceObject` moves to internal (M4 starts now); SOABI `boilvalrc1` to `boilvalrc2`. Estimates under reconciliation.
- 2026-11-04 — Decision: A3 revision 3 after the user's review: handlers have no addressable locals by construction (values in and out, the operation transaction deleted, memory work out of line, lasting state in slots, a generator lint); the record shrinks to 128 B (link and entry frame); the box area stays as R27 debt. Two landings, 16.5 agent-h, about 6 h of wall clock.
- 2026-11-04 — Decision: value-API plan revision 3: wave estimates are arithmetic from the counts (script share, 40 call sites per worker-hour, slot and field rates, pilot, builder and cleanup); about 225 agent-h (was 357) and the R27 gate at about T21 of wall clock (was T32); critical path V2a-0, M1b and M2, W2 with I3, W5; A3 has 4 h of slack.
- 2026-11-04 — Landed: register IR v2 A5b-2 (`fc981364c56`), generator views and owners; post-review found three fail-open generator rules (S3 heap stores, S7 casts and temps, TAKE killing one alias), all made fail-closed before landing; no generated file changed by the fixes. And value API M4a (`df1503353d7`). Suites: run=51,621 and 51,631, failures=2.
- 2026-11-04 — Landed: value API V2a-0 part 1 (`e8cb83c42be`) with the SOABI bump (`6ce98fe4790`). Measured: the A5b-3 fan-out (8 Sonnet workers with per-file compile checks, 8 Sonnet diff checkers) fixed 186 of 194 compile-error sites in 3.6 minutes of wall clock; the 9 left need header retypes outside the workers' files; one checker finding (a dropped reentrancy assert in `dictobject.c`).
- 2026-11-04 — Landed: value API M4b (`ca5b20c64d9`), slices as value fields; and register IR v2 A3 step 1 (`7b33e876eb3`..`15859d9a592`): JIT deleted, tail calls the only backend, activation record, canary functions 200 to 133. Suites at `15859d9a592`: run=51,627, failures=2 on gcc debug and release.
- 2026-11-04 — Landed: A3 activation-record assert fix (`eab52a937a6`); register IR v2 A5b-3 (`c71a3ca6aa2`), the typed borrow's heap side, MakeHeapSafe family deleted, R26 lints; value API V2a-0 part 2 (`f7d374b1f92`), dispatchers, slot adapter, thunks, box kinds. Suites at `c71a3ca6aa2`: run=51,630, at `f7d374b1f92`: run=51,681, failures=2 on gcc debug and release. Proposals written for M3, V2a-1a, M1b+M2 and G; peer reviews running.
- 2026-11-04 — Landed: register IR v2 A3 step 2 (`0bf1b0486a5`): handlers have no addressable locals (G9), the generator lint refuses an address taken of a handler local, box-area handlers pinned at 97. `enter_handlers()` works around a GCC 15.2 `no_callee_saved_registers` miscompile (risk R13; reproducer `build/pyjit-link-wave1/a3/gcc-ncsr-bug.c`).
- 2026-11-04 — Landed: `c6fb9beb79f` fixes the two test_capi failures (test_is_unique_temporary, test_unicode.test_new) by comparing `sys.getrefcount` with a fresh local. The suites run with 0 failures since. Supersedes the earlier decision to leave them visible until B5.
- 2026-11-04 — Landed: A5b-4 (`763c8ececd5`): final ownership names (`PyValue_DUP(PyValue_FromObjectBorrow(o))`, `PyValue_XDUP`, `PyValue_XCLOSE`, `PyValue_FromObjectSteal`), home copies typed `PyValueBorrow` with 56 commented exceptions, A5b-3 review lint fixes.
- 2026-11-04 — Landed: value API W1b (`c66a6339ccf`): container object forms compatibility-only; 1,064 sites in 86 files (814 by script or generator, 234 by Haiku and Sonnet workers); 44 sites in abstract.c, descrobject.c, object.c and typeobject.c deferred. New tools `value_sweep.py` and `wave_files.py`.
- 2026-11-04 — Landed: value API M3 (`b9d3e3a3c9b`): value vectorcall, `METH_VALUE` (0x0400), SOABI `boilvalrc3`, Clinic `value_emission`; `_testclinic_value` waits for V2a-1a. Suites at `b9d3e3a3c9b`, gcc debug and release: run=51,810, failures=0; `make check-value-api` 169 units, 0 failing findings, 5 deferred.
- 2026-11-04 — Decision: model policy. Every agent launch sets model and effort. Opus at high effort only for critical design and subtle failure debugging, never fanned out; Sonnet at medium effort or Haiku for porting, finishing, reviews and doc syncs. In the W1 pilot Haiku's mechanical batch matched Sonnet's site for site. Finishers run the full suite only through `tools/integrate.sh` and release-compile their own tree (release builds compile `#ifndef Py_DEBUG` code; two landings broke on this).
- 2026-11-04 — Decision: [register IR v2 B numbering](design/proposals/register-ir-v2-b-numbering.md) accepted. Pinned handler numbers let the B steps develop in parallel: critical path about 16.5 h against 34.5 h sequential; B-pin goes first.
- 2026-11-05 — Landed: 15 runtime commits `9f40967b7e3` to `9563654f360` (table in [STATUS](STATUS.md)): value API G-1, V2a-1a, M1b, M2, W1 batch-10, I2, I1, G (`check_r27.py`), V2a-1a-iii; register IR B-pin, B1, B2 (no `NOT_TAKEN`, -1.74% static words), B7 (constant slots), B4 (feedback memory -24% on Lib); `9fdea3b9310` regenerates `test_frozenmain.h`. Magic 64405 (B2), 64406 (B7), 64407 (B4). Full suites last passed at `01e8f3bc4ea`: 51,977 tests each on release and debug, 0 failures; later landings ran step tests and a smoke set.
- 2026-11-05 — Decision: no hardening against hand-crafted code objects or marshal streams. After the interpreter checkpoint, `co_code` and `.pyc` return to stack bytecode with the IR as a hidden cache ([proposal](design/proposals/stack-bytecode-recovery.md)). Full suites run only at integration points.
- 2026-11-05 — Correction: B4 built one `LOAD_ATTR` head, not two; [register-ir.md](design/register-ir.md) G10(c) and §1.11 updated (B2 monitoring).
- 2026-11-05 — Landed: register IR B6 (`4eeb6ea2ff2`): a compare and its conditional jump on one line become one `COMPARE_OP_JUMP`, `CONTAINS_OP_JUMP` or `IS_OP_JUMP` word; 12,989 pairs fuse on Lib, 118 stay apart. Magic 64408.
- 2026-11-05 — Landed: register IR B5 (`1fc13423e46`): discarded call results have no destination and every `PUSH_NULL` is elided (17,461 words to 0 on Lib; `POP_TOP` 38,503 to 30,021). Magic 64409. The fold is partial: a call that ends its line keeps its `POP_TOP`, because bdb stops after `pdb.set_trace()` (FOLLOWUPS).
- 2026-11-05 — Landed: B7 fix (`56906c53125`): a pushed frame writes its self slot only when the call has a self; a specialised call from a coroutine had erased a hidden constant (asyncio `test_shield_effect` segfault). `64eab46d9da` drops `PUSH_NULL` from `test_code`'s artificial-instruction list.
- 2026-11-05 — Landed: value API I4 (`1996cbbb080`): attribute, `super` and import handlers call the value dispatchers; `import x` makes no box.
- 2026-11-05 — Landed: value API I3 (`b904ab8f014`): subscript store and delete, `FOR_ITER`, `SEND`, `GET_LEN`, `MATCH_KEYS` and `MATCH_CLASS` call the value dispatchers. `5c94f78a582` (INTERPRETER_EXIT) makes the evaluation loop return `PyValue`; `e94ed139c64` adds box-site tests for I1 to I4.
- 2026-11-05 — Landed: value API W4-0 (`1a9b96b1000`): fields of eight modules are value homes (deque, itertools, `_pickle`, `_functools`, `_operator`, `_queue`, `_asyncio`, `_elementtree`).
- 2026-11-05 — Landed: register IR B5b (`8eee8081ca0`): unpack writes consecutive locals (2,408 of 3,343 unpack sites on non-test Lib); `LIST_APPEND`, `SET_ADD` and `MAP_ADD` read items from fields; -5,415 static words. Magic 64410.
- 2026-11-05 — Integration point at `8eee8081ca0`: full suites, release and debug, ran 52,178 tests each. Two failures, both stale tests, fixed in `a1ecc0eb4f3`.
- 2026-11-05 — Landed: value API I5 (`d686623e3fb`): call handlers, monitoring and tracing run on values; the monitored kernel makes 0 boxes (was 4,000). Release probe: `max(3,4)`, `items.index(7)`, `len(items)` and `round(2.5,1)` take 8% to 21% longer per call until W2 converts their callees; measure at the checkpoint.
- 2026-11-05 — Result: G2 open question 9 counted ([register-ir.md](design/register-ir.md) G2). A fixed-dst rule removes 9 to 14% of class tests at no word cost; half the tests guard sources. The decision stays between A4 and B3.
- 2026-11-05 — Landed: value API W2a (`ce9289e2ddb`), core Clinic with value emission; `61872dc3152` lets the specializer reach `METH_VALUE` callees again (`CALL_BUILTIN_FAST`, `CALL_METHOD_DESCRIPTOR_FAST`, and `CALL_LIST_APPEND` for `list.append`); `62bfae99844` checks the arity of `METH_VALUE|METH_NOARGS` and `METH_VALUE|METH_O` in the trampolines.
- 2026-11-05 — Landed: value API W2b marks (`f30173f3de3`) and `_PyValue_IndexExact` (`1e19778c783`): `PyNumber_Index` keeps upstream's exact-int result; `_PyNumber_Index` stays `_PyValue_Index`.
- 2026-11-05 — Landed: value API W4a (`229065a3c2c`), the accelerator modules. Post-review (`build/pyjit-link-wave1/review-w4a.md`) fixed in `28453e14972`; the lint's compile database now records sharedmods, so it covers 203 units, up from 182. `71566e96043` fixes the stats build, which had broken silently after W2a; every landing check now also compiles an `--enable-pystats` build.
- 2026-11-05 — Landed: `d63c129b95f`: `METH_VALUE` splits on `METH_KEYWORDS` as `METH_FASTCALL` does. The trampoline rejects keywords for positional `METH_VALUE`; Clinic emits no keyword check; `METH_VALUE|METH_METHOD` requires `METH_KEYWORDS`. Adds the dict unhashable-key messages. Step tests and smoke set on debug and release, all passing, 0 lint findings (`index-exact` run=6,003; `w4a-review-fix` run=10,421; `w4a-review-core` run=8,546).
- 2026-11-05 — Ruling (coordinator): no file-local helper may rebuild a marked object function. Object exits are written inline at the site, and value-native consumers are preferred.
- 2026-11-05 — Ruling (coordinator): one macro per check, with the failure value as an argument; no twin `_V` macros.
- 2026-11-05 — Ruling (coordinator): argument checks (arity, keywords) live in the call trampolines with upstream's messages. Never in generated code with a rebuilt name, and never by rewriting messages after the call.
- 2026-11-05 — Landed `f84d73164fa`: required thread alias and mmap/CJK review fixes; independent review clean, three builds and 4,202 tests per debug/release build passed ([closure plan](issues/checkpoint-closure/plan.md)); posixshmem Clinic diagnostics accepted and optional sizeof fidelity kept separate.
- 2026-11-05 — Landed `98a26bb6218`: W5 kernel paths, fail-closed R27 evidence checks and first startup ports; three builds, lint and 8,826 tests per debug/release build passed. Kernel deltas are zero; startup and full static/suite closure remain open ([closure plan](issues/checkpoint-closure/plan.md)).
- 2026-11-05 — Landed `4b2a6926d2a`: value-native marshal and AST decoding plus two I/O ports; reviewed error-path ownership, three builds, lint and 5,937 tests per debug/release build passed ([marshal proposal](issues/checkpoint-closure/marshal-proposal.md)).
- 2026-11-05 — Landed `58db4324fec`: LOAD_NAME, I/O audit and exception value homes; three builds, lint and 5,561 tests per debug/release build passed. Windows source reviewed, runtime unverified there ([closure plan](issues/checkpoint-closure/plan.md)).
- 2026-11-05 — Landed `e983958bede`: 139-file mechanical sweep, direct-slot ports/checker and first Decimal conversions. Three builds, lint and 9,511 tests per debug/release build passed. Merged qualification found and fixed nullable public bytes/Unicode join guards; the static inventory and indirect-slot audit remain incomplete.
- 2026-11-05 — Landed `4fffee855c9`: four startup-box fixes, selective Clinic generation and 65 Decimal macro-family methods. Three builds, lint, 483 Clinic tests and 6,649 tests per debug/release build passed. Linux no-op startup is zero-box; cold kernel compilation remains nonzero. Fresh kernel and full-suite stats attempts retain raw evidence; the checkpoint is not admitted.
- 2026-11-05 — Landed `f81ebbd646d`: further Decimal Context/instance/rounding/classmethod entries, six manual module ports and CJK callback continuations. Three builds, lint and 6,001 tests per debug/release build passed. The frozen `4fff` stats census ran 54,270 tests with 31 failures in eight files and failed R27; [triage](issues/checkpoint-closure/fullsuite-4fff-census.md) records remaining boxing/accounting debt and the diagnosed handler-306 stats array overrun.

- 2026-11-05 — Landed `d6eae4b1c1c`: complete generated handler-domain stats, strict per-dump metadata and calibrated pystats fixtures. Three builds, lint and 4,640 tests per debug/release build pass; the allocator-debug handler-306 overrun reproducer is fixed. All 29 kernels produce valid coverage, with zero loop deltas but two to 1,175 startup boxes; R27 remains open. The reviewed 21-file cold-path/Decimal/iterator batch is in full qualification.
- 2026-11-05 — Landed `6aec17eefeb`: 28-file cold-path, Decimal and iterator batch with null-aware exits and retained owner diagnostics. Three builds/lint and 6,291 focused tests per profile pass after full-suite findings (fixture mismatches and profiler abort) were repaired. Raw predecessor count has eight strict-zero kernels; final R27/full qualification remains open. Replacement upstream/interim controls now include development-header-dependent modules; no new performance result yet.

### 2026-11-05 — Context, iterator and numeric integration

Committed runtime `3f480256304`: 25 files, including the complete Decimal core
mark and reviewed constructor/context/slot ownership, six itertools producers,
long-range value homes, numeric/bisect consumers, cold binary folding and
allocator-independent stats owner metadata. Full release/debug each ran 54,301
tests. Corrected the new negative-lo test assertion and exported the existing
debug paired-slot validator; the focused 106 tests and arithmetic probes pass
on both profiles. Debug module roster is 117 with zero missing/import failures.
Landing now checks that roster explicitly. Raw kernel capture has nine fully
zero kernels; all 29 loop deltas are zero and the strict gate still fails.
The next struct/property/Unicode/inventory batch is in merged qualification.
The local closure plan is updated. Updating the Loom plan copy was rejected by
automatic approval review as an export of internal implementation details;
that remote copy remains at revision 7. No alternate export was attempted.

- 2026-11-05 — Landed: runtime `812cba08f4b` ports dictionary constructors
  and `math.dist`; three 117-module builds, lint and 6,193 focused tests per
  release/debug profile pass. The frozen `b9888ea3b40`-labelled raw capture has
  15/29 startup-inclusive zero-box kernels and zero loop deltas on all 29;
  its embedded build identity is predecessor-dirty `7baf96ed017`, documented
  in [status](STATUS.md). A selected legacy attribute-owner probe reports zero
  invalid owners in two dumps. R27 and full-suite qualification remain open;
  no new performance result exists. See the live [closure queue](issues/checkpoint-closure/plan.md).

- 2026-11-06 — `af82c03c4e5` integrates operator/factorial value continuations
  and exact process-accounting rules; three builds and 5,768 tests/profile
  pass. Full runs each exercised 54,311 tests and found the operator call-slot
  crash, whose correction is under merged validation. The new full stats
  [census](issues/checkpoint-closure/fullsuite-af82-census.md) records 14,211,307
  debt boxes and zero invalid owners/drops; admission and timing remain open.

- 2026-11-06 — `66981c963be` integrates the operator crash repair and value
  continuations for formatting, I/O, direct construction, dictionary methods
  and SQLite: three 117-module builds and 6,684 tests/profile pass. Then
  `357baeea402` closes regex subscription/getattr with 4,633 tests/profile and
  upstream differential probes. The exact-source raw census remains 15/29
  zero kernels, all zero loop deltas; no elapsed-time result is claimed.

- 2026-11-06 — `42696e9fcca` integrates owned value type assignment and its
  lock-held dictionary helper: three 117-module builds, lint and 4,499 tests
  per release/debug profile pass. Upstream differential probes cover
  metaclasses, displaced-value finalizers and slot updates. Process-accounting
  producer/reader composition and paired construction remain in progress;
  full qualification and current timing remain outstanding.

- 2026-11-06 — `84edb605d31` lands strict launch capture and completed-process
  identity (4,276 tests/profile plus real paired-process probes).
  `71016b30e45` lands direct value new: bounded 100-call int and float probes
  each fall from 100 boxes to zero; true C entries pass. `1ab005e79b8` lands
  init/constructor providers and four manual API aliases after 4,541 tests per
  profile. A private ordinary-class probe still finds 100 boxes/100 calls;
  actual metatype dispatch and direct wrappers are corrected in the next
  reviewed composition. Full qualification `12246` now covers those changes,
  float formatting, descriptor lookup and native child receipts together.

- 2026-11-06 — Landing `30553` passed regeneration, lint, three 117-module
  builds and 430 AST/call tests per release/debug profile. Fourteen focused
  startup/hot-zero/identity probes report zero boxes; 412 integer cases, 17
  kwargs/descriptor/recursion cases and two direct-C `object()` checks match
  upstream on both profiles. Full suites and strict R27 remain outstanding;
  no current timing is claimed.
- 2026-11-06 — Runtime HEAD `6a0e4c12c17` (constructor/format), following
  `05286694774` (native child accounting), is the current frozen source. All five
  build-info objects match their respective source HEADs; module rosters are
  116/116/117. Both full profiles passed 54,351 tests with no failures
  (release: 2,531 skips; debug: 2,469). The strict whole-source R27 census
  failed with 11,028,818 core/core-pending/unresolved boxes and incomplete
  identity/accounting. The 87-record, 29-kernel capture has 18 startup-inclusive
  raw-zero kernels and 29 zero loop deltas, but fails strict admission. The
  matched-build preflight passed but is ineligible as a measurement. No elapsed
  timing exists.
- 2026-11-06 — Synchronized live design and handoff summaries to the final 6a
  functional, census and kernel receipts. Diagnostic timing is active, with no
  result published; historical qualification failures remain documented.
- 2026-11-06 — The matched JIT-off 6a diagnostic campaign completed: kernel,
  pyperformance and peak-RSS candidate/baseline factors are recorded in the
  report and follow-up queue. R27 remains failed; no qualified performance or
  checkpoint claim follows from this diagnostic run.
- 2026-11-06 — Follow-up priority is steady-state throughput, with RSS
  secondary. The initial profile samples identify hypotheses only; the deeper
  performance analysis remains pending a separate coordinator report.
- 2026-11-06 — Completed the source-line review and selected hardware-counter
  analysis for deltablue, logging_silent and comprehensions. The 54 counter
  runs, 18 empty calibrations and six user-only DWARF captures passed evidence
  review with unchanged identities and no lost samples. Candidate retired
  instructions are 2.047x/1.903x/1.628x upstream; CPI is comparable or lower,
  while cache counters are too noisy for attribution. The review proposes
  matched Clang A3 R1/Q2, bounded attribute/accessor, Python-call and
  frame-traffic experiments. R27 remains open; P0–P4 precede implementation.
- 2026-11-06 — User narrows throughput work to GCC and removes absolute
  zero-box admission as a requirement. The [slowdown summary](issues/checkpoint-closure/slowdown-summary-6a0e4c12c17.md)
  separates unfinished API migration from measured hot-loop overhead, and
  workload instruction counts from per-bytecode costs. Current work prioritizes
  generated publication/helper overhead, call/frame traffic and useful ports;
  earlier strict R27 outcomes remain historical diagnostics.
- 2026-11-07 — Integrated and cold-reviewed the calls/frame/publication batch
  at runtime `7780e32e0e2`, following `8a8fdc91368` and `ba37cc10058`.
  Both full profiles ran 54,361 tests with zero failures; all three builds
  report 117 modules. The completed GCC-only campaign gives elapsed factors
  1.263136/1.054877 against upstream/interim over 29 kernels and
  1.084920/0.902391 over the declared 12-result subset. Selected-loop native
  instructions fall 2.2%/6.7%/1.4% from 6a, but remain
  2.002x/1.775x/1.605x upstream. Host variation prevents a convincing broad
  elapsed-gain claim; B's known-NULL reuse benefit remains unproven. Retained
  source/build identities, 54 counter runs and six loss-free profiles drive
  the [report](bench/results/interpreter-throughput-20261106.md) and
  [follow-up](issues/interpreter-throughput/follow-up.md). Runtime source
  remains clean; no second compiler or new zero-box census was used.
- 2026-11-07 — A fresh [holistic source assessment](issues/interpreter-throughput/holistic-assessment-20261107.md)
  finds coherent ownership but unfinished execution-path simplification; it
  prioritizes call preparation, shared phase/output facts, measured handler
  work, write-only call bookkeeping cleanup and the JIT recovery interface.
- 2026-11-07 — Completed R1–R5 interpreter architecture and cleanup through
  runtime `0953757f4e7`: dead call state removed, consuming fallback outlined,
  direct immortal empty-varargs home, shared checked first-result publication
  contract and bounded canonical shape-result store. Independent proposals,
  postreviews and cold reviews resolved unsafe declaration forms before
  admission. Both full profiles passed 54,380 tests with zero failures;
  release/debug/stats check 117 modules and value API lint passes 306 units.
  Direct balanced R4/R5 paired whole-kernel factor is 0.992103; keep accepted
  with local-read regressions documented. Final candidate/upstream factors
  are 1.247431 on 29 kernels and 1.048074 on the selected twelve-result subset;
  candidate/interim factors are 1.044650 and 0.878774. All 54 counter runs and
  six zero-loss profiles pass independent evidence review. Selected native
  instructions remain 1.960x/1.720x/1.590x upstream with lower CPI. The
  [report](bench/results/interpreter-architecture-20261107.md) records noise,
  regressions and the remaining gap; the [follow-up](issues/interpreter-architecture-20261107/follow-up.md)
  prioritizes shape/local emission, call-route counts and a separate first
  recovery slice. Native JIT recovery remains planned. Removed only the
  task-created contract worktree/branch and R4 build hardlink after retaining
  the final patch and exact ELFs; runtime source remains clean.
- 2026-11-08: [V8 comparison checkpoint](issues/interpreter-parity-20261108/comparison-checkpoint.md): eight recommendations, competing structural proposals and independent reviews; pinned logging baseline 1.657289x upstream, 161 dispatches/20 frames per loop. Runtime remains qualified R5; implementation selection is open.

- 2026-11-08 — [Interpreter parity batch](bench/results/interpreter-parity-20261108.md): selected V8 mechanisms qualified through functional `0267d428808`, comment-only successor `0765e9be47f`; both suites 54,394/0 failures, logging 1.38× upstream and 0.8402× retained R5, direct 29-kernel GM 0.9810 with spectral +8.6% tradeoff; next borrowed-frame lifetime/representation investigation.
- 2026-11-08: froze and published the golden interpreter; consolidated source-checked IR/interpreter specs with Mermaid diagrams, archived stale boards, and recorded nine cold-review recommendations in the [next plan](issues/golden-20261108/next-steps.md).
- 2026-11-09 — [Execution-boundary candidate](issues/execution-boundaries-20261108/report.md): reviewed `4b3d739d867`, release/debug full passes 54,421 each (debug retry, unexplained prior profiling failure retained); logging flat, spectral −16%, five broad regressions repeat, performance promotion withheld. Golden stays selected; next is component ablation and a call/owner cost ledger. Corrected immediate-finalization assumptions in the lifetime policy.
- 2026-11-09 — [First frame/call integration](issues/frame-call-model-20261109/integration-report.md): unqualified `6ef45e391aa`, direct guarded binding/virtual kwargs/compact positions; two-round logging 1.00365× upstream and 0.74243× golden, unpack 0.83394× golden; other broad losses and debugger/completion gates remain. Counters reduce logging's instruction excess by about 80%; repeated frame-tail initialization motivates separate review.

- 2026-11-09 — [Second frame/call engineering integration](issues/frame-call-model-20261109/second-integration-report.md): 3e5cbdb1123 passes 207 focused tests per build; logging/upstream screen 0.97449, nbody/golden repeat 1.08037; full qualification and final parity remain open.
- 2026-11-09 — [Third frame/call engineering integration](issues/frame-call-model-20261109/third-integration-report.md): 87f13087ed9 passes 320 focused tests per build; logging/upstream screen 0.84554, nbody/golden repeat 1.05620; full release diagnostic finds 13 failing modules, grouped for context/source-event and format repairs; promotion remains open.
