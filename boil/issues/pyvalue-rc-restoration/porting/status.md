# Porting rules and remaining work

We are closing compiler and ownership gaps before the first universal-value
register-interpreter measurement. Source checkpoint53 is `5b993d71b1e`; the latest merged build describes that checkpoint plus held typeobject.
The [latest merged build](../merged-build/status.md) has 254 repeated/cascading
errors, 171 objects and no executable; all 7,360 inputs stayed unchanged.
There is no restored runtime or performance result.

The [board](../../README.md) owns current assignments and accepted checkpoints.
The [component index](../source-components-20261102-v1.json) binds their commits
and evidence. Root coordinates integration and merged validation; Sol 6.1 designs
and reviews rules; Luna analyzes errors and applies approved mechanical batches.

## Current packets

| Packet | Current stage | Next gate |
| --- | --- | --- |
| Seven bootstrap type readers | Committed after independent code/native review | Merged runtime/startup and fault qualification |
| Four remaining Clinic wrappers | Corrected v3 committed as checkpoint45 | Candidate runtime/fault qualification after linking |
| Shared FORMAT engine | Foundation A committed as checkpoint47 | Backend/export/public/caller closure design and dependent implementation |
| [Itertools readers](../core-consumers/itertools/status.md) | Six-function checkpoint53 committed; native 27→1 unchanged batched error | Review/apply final batched recipe, then full TU/object |
| Enum/map readers | Three-body enum2/map_reduce design frozen with corrected proposed test | Independent design review, Luna application, native/code review |
| [Datetime readers](../core-consumers/datetime/status.md) | Twelve-function checkpoint52 committed, including three undiagnosed varargs reads | Linked candidate qualification |
| Pickle arguments/state | Three-reader checkpoint46 committed | Candidate runtime/fault qualification after linking |
| GenericAlias tuple consumers | Reviewed eight-body checkpoint49 committed | Candidate runtime/fault qualification |
| Type attribute object bridge | Checkpoint48 committed; both full TUs/objects pass | Linked candidate behavior/fault qualification |
| Method lookup/call envelope | Checkpoint50 committed | Execute eight candidate contracts; six ordinary reference controls pass |
| Scalar type boundaries | Checkpoint51 committed; full TU/object and five host controls pass | Candidate qualification, then dependency extraction/MRO/error/lifecycle closure |
| Held type foundations/cache correctness | Causal classification accepted; nine canonical foundations accepted in place | Extract exact reviewed dependencies; close remaining slot/MRO/guard/lifecycle defects |

Exact designs and inventories:

- [Clinic four-site design](../../../../build/pyvalue-clinic-final-constructors-design-20261102-v1/README.md)
- [FORMAT engine design](../../../../build/pyvalue-format-engine-design-20261102-v2/README.md)
- [Pickle per-function recipes](../../../../build/pyvalue-type-pickle-readers-design-20261102-v2/README.md)
- [Six-function itertools inventory](../../../../build/pyvalue-itertools-tail-inventory-20261102-v1/README.md)
- [GenericAlias inventory](../../../../build/pyvalue-genericalias-inventory-20261102-v1/README.md)
- [Accepted GenericAlias design and implementation recipes](../../../../build/pyvalue-genericalias-readers-design-20261102-v1/README.md)
- [Accepted type/cache causal matrix](../../../../build/pyvalue-typecache-causal-triage-20261102-v1/README.md)
- [Current family census](../../../../build/pyvalue-v16-error-families-20261103-v1/README.md)

The v16 census has 299 observations at 246 sites in 52 paths; 31 warnings are
separate. Tuple readers contribute 178 observations, list readers 49 and
dictionary lookup/iteration 34. Generated Clinic paths have zero errors; seven
handwritten atexit consumer errors remain. The v17 diagnostic multiset is exact
to v16, with only the undiagnosed type source and two test paths changed;
[comparison](../../../../build/pyvalue-merged-build-20261103-v17/diagnostic-comparison.json).
V18 removes nineteen datetime error observations and three header warnings, adding no
diagnostics; all other diagnostic rows are unchanged. The source-bound census
remains historical, with this [exact delta](../../../../build/pyvalue-merged-build-20261103-v18/diagnostic-comparison.json).
V19 removes another 26 errors through itertools53, including four array errors
reported through header expansions. All other diagnostics are unchanged in its
[comparison](../../../../build/pyvalue-merged-build-20261103-v19/diagnostic-comparison.json).
The earlier 22-error itertools inventory missed those four plus the remaining
batched header error. The actual full-TU baseline has 27 errors.
A [four-scope inventory](../../../../build/pyvalue-core-reader-inventory-20261103-v1/README.md)
now maps builtin/exception/enum/atexit diagnostics to concrete next packets.

The historical v12 census predates the latest type and Clinic packets. Its 332 observations map
to 278 sites in 58 paths, with 77 warnings counted separately. All eight then-remaining
Clinic errors map to four producers: reversed_new, float_new, tuple_new and
atexit_register. None matches the earlier fixed tuple-keyword rule. The itertools
inventory maps 22 observations to 21 getter expressions in six functions.
Neither observations nor sites are independent edits or a percentage complete.

## Required baseline versus later tuning

Universal PyValue describes stored language values. An owning temporary
PyObject view at a C boundary is valid: it can box an inline scalar, stays alive
through its users and is then released. Every C local need not become value-native
before measuring. All extensions are rebuilt and ported; hard compatibility is
not a gate. See [process](../../../PROCESS.md).

| Required for a safe baseline | Can follow the first measurement |
| --- | --- |
| Remove reads/casts treating value storage as object pointers | Avoid boxing in correct owning C bridges |
| Replace removed borrowed accessors with owners or scoped leases | Value-native builtin/extension operations |
| Retain parser outputs and native arrays through their users | Faster iteration and direct-value bridges |
| Preserve errors and coherent state through callbacks and cleanup | IR, dispatch and register-allocation tuning |
| Close relevant type/cache mutation and invalidation defects | Further JIT implementation and optimization |
| Port and qualify the selected core/bundled profile | Other documented platform/extension profiles |

Extensions outside the selected baseline profile remain recorded ports. They
are not silently qualified or required to become value-native first.

## Reusable rules

These rules apply only under a named, reviewed packet with exact inputs.

| Pattern | Rule and automation boundary |
| --- | --- |
| Owned generic item | Owning fallible getter; immediate NULL/pending check; keep through callbacks and protected cleanup |
| Proven type/name metadata | Validate canonical kind before pointer access; source owner spans pure reads; selected owner spans callbacks |
| Tuple/native argument window | Reviewed owner-scope object view; End after implementation and converter/resource cleanup; never cast value storage |
| Dictionary lookup | Preserve found/missing/error distinction and actual owner; pending error cannot become missing |
| Mutable iteration | Owned snapshot or reviewed revalidation at actual callback points; API renaming alone is insufficient |
| Occupied value write | Prepare new owner, publish coherent state, retire displaced owner; retain selected target as required |
| Generated products | Change producer, regenerate, prove excluded output exact, regenerate again for idempotence |

Decoder names have different domains:

| Operation | Required proof and result |
| --- | --- |
| `PyValue_AsPyObjectBorrow` | Heap-object value only; pure borrowed decode rooted through use |
| `PyValue_AsPyObjectNew` | Heap-object value only; adds a reference, never boxes inline values |
| `PyValue_AsObject` / `PyValue_AsPyObjectNewBoxed` | Fallible owned object view including inline values; check NULL and pending error |
| `PyTuple_GET_VALUE` | Borrowed canonical value; retain tuple or promote with NewRef/MakeHeapSafe before callouts |
| `PyValue_DUP` | Duplicates an already owning value; does not promote a mortal borrow |
| `PyValue_NewRef` / `PyValue_MakeHeapSafe` | Promote borrowed heap values to independent owners; the latter consumes its input |
| `_PyTuple_GET_OBJECT` | Proven heap metadata only; no generic scalar fallback |

Escaped parser outputs, recursive/resource conversion, descriptor lookup and type
publication need explicit contracts. Correctness does not follow from a grep
replacement, a cast, successful compilation or a host reference test.

## Batch lifecycle and evidence

Freeze the rule, bodies/producer, hashes, dependencies, owner and cheap acceptance
checks. Obtain independent design review, then give Luna or a mechanical rewriter
the finite edit recipe. Report unmatched cases rather than guessing. Review actual
code and raw results independently; root integrates only the selected delta,
validates the composition and commits. Serialize native/shared builds while
independent implementations use isolated copies. Broad qualification and quiet
timing wait for a linked candidate.

Recent accepted packets include operator/LRU (`d1a9a80d25b`), flat FORMAT Stage1
(`aa7a4335ddf`), six type readers (`0fd003e9c4b`) and four MRO readers
(`df30266b3b4`). The operator object and parser fixture object build. Bootstrap7
(`b76f2182354`) follows those packets: selected 2 errors/13 warnings→0, full type
TU 4→3 errors/16→3 warnings, with exactly one first-use relocation. Six host
controls pass; startup/fault/candidate behavior remains unexecuted. The Clinic
v2 packet required receiver, formatting and evidence corrections. Corrected v3
is committed after independent review: 287 producer/five host passes, selected
eight native errors→0 and full float/tuple objects; enum/atexit still fail.
These are dependent source checkpoints, not runtime admission.

Preserve failed attempts and corrections. The type-reader design-control log
originally claimed four passes but contains three passes/one error. The failed
log, adjusted controls and author/peer corrections remain in that experiment.
The super-lookup owner ledger's claim that type dictionaries cannot invoke key
equality callbacks is also explicitly corrected, with a host reparenting witness.
Candidate execution of that witness remains required.

The [historical checkpoint notes](history/checkpoint-notes-through43-20261102.txt)
preserve the former page byte-for-byte. Detailed patches, reviews, commands,
raw logs and source identities stay in each experiment directory; concluded
outcomes have one line in the [work log](../../../WORKLOG.md).
