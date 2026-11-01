# Implementation progress

The working source is in `build/worktrees/pyvalue-rc-universal`, based on
`95be1d28`. It remains an incomplete dependency state: there is no runnable
restored candidate or performance result. RC and cyclic collection remain
selected. Rebuilding extensions against the new private ABI is intentional;
Python behavior remains required.

## Parallel implementation and review

| Owner | Current result and remaining work |
| --- | --- |
| restart_audit | Public PyValue and ownership API, actual frame/call publication, scoped scalar bridge owners and generator changes. Seven generators/eight products regenerate; 38 host generator tests pass. Six provisional syntax checks pass pyvalue/frame/genobject and expose remaining consumers in ceval/frameobject/testinternalcapi. Fixed-arity call emission, guard restart cleanup and tail-call coverage remain under review. |
| coordinator | Tuple/member/cell storage, shared value operations and value iteration. The original 12-file component and new 19-file cell/iteration checkpoint are independently accepted as partial source. Set storage implementation has begun. |
| coroutine_design_review | List/structseq 13-file component accepted as partial source, including sort/mutation/traversal and ownership tests. Dictionary storage/tests have a frozen seven-file partial checkpoint. Designing the explicit opaque extension API and rebuilt-source bridge requested by the user. |
| value_api_consumer_map | Independent source review, callback/failure-path findings and the sealed 660-path native consumer inventory. Source component decisions do not constitute runtime acceptance. |

Component evidence is retained under `build/pyvalue-root-container-checkpoint-20261101-v1`,
`build/pyvalue-root-cell-iteration-checkpoint-20261101-v1`,
`build/pyvalue-register-interpreter-checkpoint-20261101-v1/{list-port,dict-port}`,
`build/worktrees/pyvalue-rc-universal/build/pyvalue-foundation-source` and
`build/pyvalue-rc-native-consumer-map-20261101-v1`.

## Shared ownership interface

| Interface | Ownership |
| --- | --- |
| `PyList_GET_VALUE` / `PyTuple_GET_VALUE` | Borrowed value rooted by the container; unchecked index |
| `PyList_GetValue` / `PyTuple_GetValue` | Checked, independently owned value |
| `PyList_GetItemRef` / `PyTuple_GetItemRef` | Owned object reference; scalar boxing may fail |
| `_PyValueIndexPair` / value iterator helpers | Owned value without scalar boxing; exhaustion/error distinguished by index and exception contract |
| `PyCell_GetValue` / `_PyCell_GetValue` | Owned value; NULL denotes an empty cell |
| `PyCell_NewValue` / `PyCell_SetValue` | Borrowed input promoted before publication |
| `_PyMember_GetValue` / `_PyMember_SetValue` | Owned result / borrowed input; publish replacement before releasing displaced owner |

The public virtual-iteration slot now returns an owned value pair. Tuple, list,
bytes and Unicode providers and the three real interpreter consumers use that
contract. Ordinary object iterator wrappers still box at their explicit C object
boundary; they reserve progress before allocation and roll back only an unchanged
reservation. Tests cover OOM retry and reentrant advancement. The new checkpoint
contains 14 Python test methods, parsed on the host but not run on a candidate.

Cells store strong PyValues and use a swap operation where the caller must
publish or detach another home before release. Their owned object getter is
fallible. Super/class construction preserve its boxing error; a new C fixture
forces getter OOM without changing cell contents. Provisional syntax checks pass
cell storage and the internal test adapter. Bytes/Unicode compilation exposes
unported Clinic wrappers and wider container consumers, not a new runtime result.

Shared hashing/comparison retain inline numeric fast paths. Cold object bridges
pin both inputs before the first allocation. Independent review corrected
exception handling around callback-capable releases: retain the entry exception,
distinguish new operation/cleanup failures and preserve the primary exception.
This source correction is accepted; runtime cleanup-failure coverage is pending.

`_Py_T_VALUE` and `_Py_T_VALUE_EX` distinguish strong value fields from object
pointer fields. Structseq uses the former; absent fields read as None. The latter
preserves absent-slot AttributeError behavior. Every value field initializes to
NULL1 before observation; zero-filled memory is insufficient. General instance
slots still hold object pointers. Their allocator/traversal/destructor migration
must accompany the specialized attribute consumer change.

Dictionary watcher callbacks retain the existing **before-mutation** contract.
The earlier post-mutation proposal was corrected against the actual C API
documentation. Dictionary lookup review also corrected a post-callback key
stability check to use saved canonical bits after table revalidation, avoiding
dereferencing a removed key.

## Native and Clinic closure

Core operations receive borrowed PyValue arrays and return owned values. An
opaque object implementation uses an explicit call-owned bridge: snapshot all
arguments before boxing, retain temporary objects and clean up every exit.
Parser outputs may borrow the owned array during that call. A fabricated boxed
tuple or a permanent scalar-box cache would violate the selected storage design.

The private native entry contract carries the caller's temporary-owner context
explicitly. A wrapper prepares an object result into canonical PyValue and queues
its retired primitive source owner there. The caller publishes the value and live
state before closing that owner. Directly produced values need no retired box.
The complete native/parser/Clinic implementation remains pending.

The user clarified that old extension C API/binary compatibility is not a project
requirement, then requested an explicit opaque API and gradual source migration.
That design work supersedes immediate execution of the rejected dictionary
API-removal script and its approval question. The script remains unexecuted.
See [extension API direction](extension-api-direction-20261101.md) and the
[earlier approval record](extension-rebuild-authorization-20261101.md).

## Next qualification checkpoint

Complete dict/set/instance storage, the core semantic and native-call surfaces,
parser/Clinic and bundled consumers, and `boilvalrc1` import/cache/wheel identity.
Then build and qualify one coherent runtime. Provisional checks use the old debug
pyconfig only for diagnostics; they are not candidate builds. The foundation's
source ceiling is 23:49:40 UTC; set's first edit was 22:33:01 UTC, giving a
00:03:01 UTC ceiling. Neither budget has been extended.

Measure the unified-value register interpreter with verified JIT-off execution
and existing basic optimizations before returning to JIT performance work. The
prepared comparison uses the same 29-workload roster against interim RC and
upstream, reporting startup, compilation, first use, warm execution and memory
separately. No timing has run yet.
