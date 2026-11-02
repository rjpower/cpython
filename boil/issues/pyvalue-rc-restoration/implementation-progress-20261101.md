# Implementation progress

The working source is in `build/worktrees/pyvalue-rc-universal`, based on
`95be1d28`. It remains an incomplete dependency state: there is no runnable
restored candidate or performance result. RC and cyclic collection remain
selected. Rebuilding extensions against the new private ABI is intentional;
Python behavior remains required.

## Parallel implementation and review

| Owner | Current result and remaining work |
| --- | --- |
| restart_audit | Foundation, specialized O-call, local/slot retirement, common constants and VM dictionary construction/stores source accepted. The latter has42 passing host checks/seven generators and nine AST-only methods. Implements the next separately reviewed specializer/cold-and-warm reader cut. |
| coordinator | Tuple/member/cell/iteration, set, Python slots, managed initialization before CREATE, common constants and compiler constant/key components source accepted. Runtime remains unqualified. Coordinates remaining compiler cache/interning design and coherent integration. |
| coroutine_design_review | List/structseq, native scope, dictionary clear/release, owning reads, mutation/growth/typed watchers and the value-native constructor source accepted. Implements the separate copy/conditional-insertion/managed-storage cut under corrected zero-refcount rules. |
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
preserves absent-slot AttributeError behavior. The accepted instance-slot source
uses VALUE_EX for Python-created slots. NULL1 and variable size initialize before
reference CREATE callbacks; zero-filled memory is insufficient. Traversal/clear,
member setters and specialized interpreter loads/stores move with that format.
Legacy object fields and __class__ keep their generic descriptor semantics.

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
pyconfig only for diagnostics; they are not candidate builds. Foundation, O-call,
set storage, instance-slot and additive dictionary-provider windows closed early.
Shared retirement and canonical common-constant windows also closed early.
The dictionary mutation/growth window closed early after independent review.
The compiler-constant and VM dictionary windows also closed early. The active
dictionary-storage cut runs from00:59:45 to02:29:45 UTC on2026-11-02, with
concrete copy-versus-detach checkpoint01:19:45. No INCREF0 is permitted on
deallocator-owned detach/clear; live-instance and metadata retention are separate.
Its concrete early checkpointb4323811 is independently reviewed at099f0095;
that accepts only eight early fragments, not the final storage component.
A's separate reader/feedback cut runs01:14:50 to02:44:50 UTC, checkpoint01:44:50.
Its cold generic/warmed dictionary paths must be coherent; error output flush
must not overwrite restored aliased input homes. Two exact C provider handoffs
retain C's original02:29:45 ceiling. Root's key-producer window closed early;
cache/interner followup remains design only until its real provider contracts
and shared-tuple semantic-equality/hash invariants are settled.
Each has its own source grant; no original source budget was extended.

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

## Current correction and source state,2026-11-02 00:20 UTC

The [deallocator contract correction](deallocator-fixture-correction-20261102-v1.md)
retracts the earlier exceptional-DECREF fixture witness claims. tp_dealloc must
preserve the entry exception; debug builds enforce it. Keep the contract and
assertion. Corrected legal unraisable/reentry/primary-preservation fixtures are
independently accepted at receipt `bcd13b6f`:14 native/local and eight set methods,
all AST-only. Set production storage is unchanged. No injected destructor error
is now claimed as a valid scope/operation failure witness.

Owning dictionary provider review `95427515` accepts two new definitions and six
AST-only real set-consumer tests. Legacy getters and the rejected API-removal
script remain untouched. C's next mutation/growth cut retains one original event
classification, sends its one pre-mutation notification after required capacity
allocation succeeds, and re-probes after callback/release boundaries.

Instance-slot review `a7400902` accepts the corrected ten-file source at
`d8879689`; its [disposition](instance-slot-source-disposition-20261102-v1.json)
closes that window. Adapter/member/specializer syntax pass; the full type TU still
fails old wider container API calls. The slot route is an interpreter executing
specialization: existing JIT helpers tier down before its effect.

A's shared local/slot kernel final receipt `73e95037` fixes the v1 C-only result
leak on refresh failure and includes intrinsic error metadata. Independent review
`d4e78148` accepts the final kernel. D verified153 emitted folded-local publication sites have
only pure writes/commit through dispatch. Generic input/field retirement remains
required: actual STORE_ATTR_WITH_HINT still has unchecked releases and can use
stale executing words after a legal finalizer changes instrumentation. This is
an explicit counterexample for the next consumer closure, not a whole-VM claim.

Common-constant review `a0f5c131` accepts root source `7dc3c2f6` and generated
closure `c8a4ae8f`: the14-entry table now holds inline -1, both generated loads
use DUP, shutdown guards scalar values, and compiler object getters return owned
references. Two selected syntax checks and41 host generator checks pass; five
runtime tests are AST-only. [Component dispositions](source-components-20261102-v1.json)
retain exact acceptance and retraction limits.

## Dictionary mutation acceptance and consumer checkpoints,2026-11-02

Mutation review `56bbc658` binds author receipt `db99c46e`:43 fragments across
ten context files, comprising33 core functions, five callbacks and five
notification-argument conversions only. It accepts canonical mutation/growth,
raw post-boxing selection revalidation and `_PyDict_FromValueItems`.
Object pop boxes before removal, so boxing OOM consumes nothing. The new C
fixture models actual table-address reuse and once-only equality;14 methods
remain AST-only. Watcher/fixture syntax passes; Module Clinic and the whole
dictionary translation unit remain unqualified. Copy, managed storage,
setdefault/popitem algorithms and legacy native consumers remain required.

VM checkpoint `bc738ea9` covers BUILD_MAP, generic/specialized dictionary
assignment and hinted stores with pre-callback PC capture and forced reload.
It has42 passing host checks/seven generators and nine AST-only runtime methods.
Final receipt `d1606aba` corrects hinted failure order by restoring selected
temporary owners to their original homes before dict-pin close and real unwind.
Independent review `57d1a09c` accepts the selected source and closes its window.
Stable IDs/formats are distinguished from required derived effect metadata
changes; no candidate execution is claimed.

Compiler checkpoint v2 `71666fa3` uses normalized canonical identity words and a
full-word hash, pure tuple complexity traversal and owned value compaction.
Seven AST-only tests cover actual CFG folding/compaction and checked setter
ownership/publication. Flowgraph/fixture syntax passes; the list TU retains old
Clinic and two sort truthiness errors. The exact-operation host hash model has
1279 occupied buckets and maximum chain6 for2048 integral float keys; this is
source evidence, not a performance measurement. Independent reviewfc3237a4
accepts this dependent component; its window closed early. Recursive compiler
cache/final interning and wider codeobject consumers remain actual dependencies.

The independent typed-key producer now also passed review97607e27, binding
corrected receipt6f64aa7c. Recursive tuple/frozenset keys use canonical values;
the old object entry is an owned, fallible adapter. Review replaced ordinary
frozenset iteration with fresh New(NULL)/AddValue to avoid hidden scalar boxing.
Six new tests are AST-only, including a one-shot allocation failure/recovery
fixture at a real32-slot tuple allocation. The actual fixture and exact unchanged
producer-body projection pass syntax; full codeobject retains older Clinic,
interning and reader failures. Its window closed early. Compiler cache/final
interning and wider codeobject closure remain required.

Managed constructor source now closes the CREATE publication gap: reviewb11eaaad
binds receiptc8047215. The allocator installs the type pointer, initializes
declared and managed fields, then calls `_PyObject_Init[Var]` for the sole type
INCREF and reference publication. The existing observer checks full managed
capacity/header/root shape/absent dictionary and every NULL1 slot without
allocation or exception changes. Two new tests are AST-only; actual fixture
syntax passes, full type/native/runtime qualification remains open. The separate
15-minute source window closed early. This changes no C-owned initializer body.
