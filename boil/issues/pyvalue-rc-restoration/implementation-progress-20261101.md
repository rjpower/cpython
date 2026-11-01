# Implementation progress

The working source is in `build/worktrees/pyvalue-rc-universal`, based on
`95be1d28`. It remains an incomplete dependency state: there is no runnable
restored candidate or performance result. RC and cyclic collection remain
selected. Rebuilding extensions against the new private ABI is intentional;
Python behavior remains required.

## Parallel implementation and review

| Owner | Current result and remaining work |
| --- | --- |
| restart_audit | Foundation frozen early and independently source-accepted: 39 host tests, seven generators/eight products and six selected syntax checks pass. The separate specialized O-call production successor is independently source-accepted with 40 host tests/four syntax checks passing. Its source window closed early; fixture composition continues separately. Wider providers/native/Clinic consumers remain open. |
| coordinator | Tuple/member/cell storage, shared value operations and value iteration independently accepted as partial source. Set checkpoint v5 is independently accepted: six files/eight parsed tests and two passing provisional syntax units. Its source window closed early. Python instance-slot migration is design-only. |
| coroutine_design_review | List/structseq component, fixed native-scope engine and bounded dictionary clear/release increment independently source-accepted. Whole dictionary remains incomplete. Adding a real End-cleanup failure fixture, then designing the next internal dictionary-provider cut. |
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
pyconfig only for diagnostics; they are not candidate builds. The foundation
froze early; its budget is closed. The separate
[O-call successor](o-caller-source-grant-20261101-v1.md) runs from 23:29 to
23:59 UTC. Set's first edit was 22:33:01 UTC, giving a 00:03:01 UTC ceiling.
The dictionary cut retains its original 00:00:46 ceiling; the native-scope
fixture delta retains 00:10:49. No original source budget has been extended.

Measure the unified-value register interpreter with verified JIT-off execution
and existing basic optimizations before returning to JIT performance work. The
prepared comparison uses the same 29-workload roster against interim RC and
upstream, reporting startup, compilation, first use, warm execution and memory
separately. No timing has run yet.

The [opaque extension design](../../design/extension-api.md) and its
[first source slice](native-scope-source-grant-20261101-v1.md) are reviewed
direction and authorized implementation respectively. Actual generated
publication/reload and finalizer ordering remain source review obligations.

## Source checkpoints at 23:49 UTC

Foundation receipt `7cb1d9e6` pins 82 owned paths, seven provider handoffs and
six shared context paths. Independent freeze review `2a49f58d` accepts only
that dependent component. Its boxed common-constant table is a required followup.
Native-scope receipt `59fd5663` accepts the engine and gateway, without claiming
actual VM composition. Corrected O checkpoint v2 (`5940dafe`) checks errors
before reconstructing possibly invalid execution state. Independent review
`26e01bbf` accepts its actual generated production handler; the
[disposition](o-caller-source-disposition-20261101-v1.json) closes the source
window early. Earlier v1 is retained as the counterexample. Corrected runtime
fixtures still require exact binding and eventual candidate execution.

Dictionary clear/release receipt `bfa98144` accepts seven function fragments and
three AST-only language-finalizer tests. It does not accept the unfinished
dictionary or the superseded API-removal script. Set checkpoint v5 patch
`7c439a22` includes NULL1 initialization, value keys and first-error-preserving
release/status propagation. Its eight tests are unexecuted.
Independent set review `4d2d38df` accepts that dependent component; the
[source disposition](set-source-disposition-20261101-v1.json) closes its window.

Required remaining cuts include dictionary providers/mutations and raw consumers,
Python instance slots and allocation, canonical constants, shared VM displaced-
local retirement/error/reload, other native families, parser/Clinic, bundled
consumers and private ABI identity. A passing source component does not close
these dependencies. Full HPy conformance does not gate the internal interpreter
checkpoint.
