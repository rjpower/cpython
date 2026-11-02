# Porting rules and remaining work

We are closing compiler and ownership gaps before the first universal-value
register-interpreter measurement. Source checkpoint44 is `b76f2182354`.
The [latest merged build](../merged-build/status.md) has 324 repeated/cascading
errors, 166 objects and no executable; all 7,349 inputs stayed unchanged.
There is no restored runtime or performance result.

The [board](../../README.md) owns current assignments and accepted checkpoints.
The [component index](../source-components-20261102-v1.json) binds their commits
and evidence. Root coordinates integration and merged validation; Sol 6.1 designs
and reviews rules; Luna analyzes errors and applies approved mechanical batches.

## Current packets

| Packet | Current stage | Next gate |
| --- | --- | --- |
| Seven bootstrap type readers | Committed after independent code/native review | Merged runtime/startup and fault qualification |
| Four remaining Clinic wrappers | v2 requires revision; Luna applies complete v3 correction recipe | Independent actual-code/results review and fresh native gate |
| Shared FORMAT engine | Foundation A v2 design accepted; Luna implements isolated two-file kernel | Compose fixtures, review and native gate; backend/public/caller closure follows |
| Remaining itertools readers | Inventory complete; four tuple-result draft paused | Review reusable canonical copy/update rule; pairwise and zip-longest have separate call boundaries |
| Pickle arguments/state | Smaller per-function recipes in peer review after incomplete bulk attempt | One function per worker, code review, composition and native gate |
| Full type mutation transaction | In revision | Distinguish introduced defects from inherited behavior; use concrete evidence to choose necessary fixes |

Exact designs and inventories:

- [Clinic four-site design](../../../../build/pyvalue-clinic-final-constructors-design-20261102-v1/README.md)
- [FORMAT engine design](../../../../build/pyvalue-format-engine-design-20261102-v2/README.md)
- [Pickle per-function recipes](../../../../build/pyvalue-type-pickle-readers-design-20261102-v2/README.md)
- [Six-function itertools inventory](../../../../build/pyvalue-itertools-tail-inventory-20261102-v1/README.md)
- [Latest family census](../../../../build/pyvalue-v12-error-families-20261102-v1/README.md)

The v12 census predates the latest two type packets. Its 332 observations map
to 278 sites in 58 paths, with 77 warnings counted separately. All eight remaining
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
| `PyTuple_GET_VALUE` | Borrowed canonical value; retain tuple or duplicate ownership before callouts |
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
v2 packet has 284 producer and 5 host-reference passes, but actual code review
found receiver-selection, formatting and evidence gaps. Those results do not
admit v2 or its pending v3 correction. These are dependent source checkpoints,
not runtime admission.

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
