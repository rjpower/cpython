# Implementation progress

Historical chronology, retained with its original checkpoints and corrections.
The [current status](status.md) and [board](../README.md) own present work.
Previous agents are stopped; their assignments, source windows and compatibility
approval questions below are superseded. Full rebuild/extension porting is
authorized. Runtime source work is held during the wiki reset, not awaiting
compatibility approval. Read later entries as outcomes of their dated stages.

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

## Storage and reader closure,2026-11-02 02:02 UTC

Dictionary storage final v2 `7ab4e86a` passes independent review `f7fc0884`:
36 scoped fragments cover copy/merge, conditional insertion, popitem and managed
storage. Four external object/import handoffs accept only their precise provider
and error-check hunks. Copy DUPs after allocation/reacquisition; detach MOVEs
without resurrecting a refcount-zero instance. Conditional insertion returns
0 inserted or1 existing with an independent requested output; only -1 is error.
Non-inline managed dictionary replacement retains original delayed field-owner
retirement. Twenty-two methods are AST-only; the actual C fixture passes syntax.
Old Dict getters/views/iteration and wider native consumers still prevent whole
dictionary qualification. The storage window closed early.

Reader final `ce914498` passes independent review `c745b682`: four owned
specializer lookups, inline-safe global/lazy handling, guarded attribute reads,
and canonical cold/warmed dictionary reads. Capture scalar PCs before callbacks;
publish/detach inputs and outputs, retire in original order, refresh even with
Temps0. Abort never flushes NULL over a restored aliased input home. Binary
nomination and statistics use canonical types without boxing; historical saves
that require allocation are skipped, while existing rows/current nomination
remain. User-GETITEM nomination and history growth are explicit later work.
Forty-four host checks/seven generators and two actual syntax checks pass;
16 methods are AST-only. The reader window closed early.

The next attribute design includes nomination's real allocation boundaries,
generic method two-output publication, inline class descriptors and managed
stores. The native design extends the accepted engine through real generic
NOARGS/FASTCALL/keyword dispatch, with caller-owned views through publication.
Both remain design-only until independent review and separate source grants.
Full HPy support remains outside the internal interpreter checkpoint.

The repr guard also passes review522be114 (author612dc1da, corrected complete
patcha81845e8). Identity scanning is canonical and cannot invoke user equality;
conditional association reuses a guard installed during allocation reentry.
Leave restores the exact entry exception. Review fixed fixture cleanup for an
unexpected successful duplicate registration. Four methods remain AST-only;
actual Objects/object.c and internal fixture TUs pass syntax. The15-minute
source window is closed. No runtime repr behavior is claimed yet.

Following critical design review, separate attribute (90min/30min checkpoint)
and native vector-engine (60min/20min checkpoint) grants now proceed in
parallel. Generic native VM caller adoption follows the attribute source
freeze, preserving one owner for the VM producer/generation lane. Engine source
alone cannot close its actual Python-to-native route.

## Compiler cache and metadata,2026-11-02 02:16 UTC

Review94d257be accepts canonical cache/interner source709c89c9. Constants,
typed keys and small indices stay canonical; conditional insertion distinguishes
inserted/existing/error. Shared tuple replacement preserves typed equality/hash
and cached hash, without assuming uniqueness or weakening public setters.
Frozensets and slices rebuild owned representatives; the old object merge entry
retains its original pointer until fallible conversion succeeds, then publishes
before retirement. Review corrected negative iterator status handling. Five new
tests cover shared/hash tuples, NaN identity, typed signed zeros, nested code
units, slices and marshal determinism; all18 methods remain AST-only.

The exact changed-body projection passes syntax. A separate ten-minute metadata
cut then replaces seven proven object-only reads (capsules/Unicode/PySTEntry),
reviewf3b04a69 binding7e2340ae. It leaves accepted cache/interner source exact.
The full actual Python/compile.c TU now passes syntax under real headers. Both
windows closed early. Old native dictionary bridges and wider compiler/runtime
execution remain unqualified; syntax closure is not runtime qualification.

## Variable native engine and current review,2026-11-02 02:37 UTC

Partial engine review3f0c4a38 accepts final5c218de9, closing its source window
early. Variable storage extends the same scope and remains drainable after
partial capture/boxing. Typed dispatch checks the actual current vectorcall
target before and after allocation; pre-body NOT_INVOKED decline is distinct
from an executed native failure. Fixed O retains its0/-1 contract. Header
6270ec5d is frozen; three actual syntax TUs and Clinic idempotence pass,22
methods are AST-only. Generic VM adoption must still close the real route;
recursive parsers and typed registration remain separate required work.

Attribute checkpoint56341731 at02:29:09 met its02:32 gate; additive d4fe1085
only strengthens fixtures. Review corrected getter-entered failure retirement
and stale post-cleanup method stores before acceptance. Production and generated
bytes remain frozen while legal managed-store handler/observer witnesses are
reviewed. Seven generators/45 host checks/three actual syntax TUs pass; no
attribute source/runtime acceptance yet.

Root codeobject checkpoint1da89ffd at02:30:51 meets its02:33 gate. Final300b559c
has canonical interning, raw name preflight with constructor/replace error
distinctions, actual inline handler eligibility and seven AST-only tests. Tests
force distinct interned anchors and valid external inline-handler words rather
than assuming warming proves the path. Exact unchanged source projection with
the actual localsplus helper passes syntax; full codeobject still fails old
generated Clinic tuple access. Source is held for independent review.

## Attribute/code-object acceptance and next cuts,2026-11-02 03:17 UTC

Final attribute292c9f3d passes reviewb4a0e4e6 and closes its window early.
The final readiness pin correction is source-bound; three actual TUs pass,
while the additional typeobject TU preserves41 old errors and their line-map
proof. Twenty-one methods remain AST-only, with prior16 unchanged.

Codeobject final0aadc5b2 passes independent C reviewa864f8e6 plus D's additive
fixture correctionca1fc0a9, closing its window early. Prior production review
c80058b4 remains valid, but its v2 encoding-witness claim is withdrawn: the
actual encode_words helper rejects specialized string names. Final tests use
numeric IDs203/204 from actual metadata; source-derived peer host controls
reject the old path and preserve numeric operands. Seven methods are AST-only.
Canonical shared/hash tuple ownership and the accepted typed-key producer
remain unchanged. Whole Clinic/free-threaded/native/runtime remains unqualified.

Generic native caller handoff093ea6f9 plus addendumf63863ed passes design
reviewc11d2231. Its new60-minute source grant includes selected cold feedback
noallocation, safe kind checks and bounded inline TypeError, plus actual
ordinary/KW caller publication/cleanup/refresh. Other callable/instrumented
admissions remain mandatory later work. A completed attribute turn was explicitly
resumed using followup_task after queued messages alone did not restart it;
no implementation budget ran during that idle interval.

C's dictionary reader checkpointce4bd36d arrived02:58:08, before03:03:40,
with actual first edit02:48:11. Final187744d6 is now held for independent review:
34 fragments, seven new methods/29 AST-only total. Only six legacy borrowed
boundaries remain in actual Dict diagnostics; fixture syntax passes. Source
completion is not accepted until the final exact review is sealed.

Root's type-cache design is held before source grant. The existing pointer
cache cannot store inline values and resize can retain a table across allocation.
Further source audit found pre-notify watchers can repopulate a class cache
after invalidation but before commit. Initial invalidation alone is not a valid
lifetime/version proof. A mutation/admission barrier must cover all supported
type/base/MRO/clear paths and already-installed guards before implementation.

Dictionary final187744d6 subsequently passed independent reviewe9a446cc,
closing its source window early. Independent function census verifies exactly
34 fragments and all other existing Dict bodies/names unchanged; prior22
fixture ASTs remain exact,29 total. Repr Writer_Discard now preserves the
primary exception. The actual fixture passes syntax; full Dict still fails
six retained borrowed bodies. Value-native iterator VM adoption and the explicit
legacy API/core-caller migration remain required. No candidate execution.

## Native caller and canonical consumer checkpoints,2026-11-02 03:48 UTC

A generic native caller checkpoint6f45ad14 arrived03:27:00.488936 before
03:30:41. Exact review corrects fallback callable/names homes before consumption,
cold inline auto-restoration and the final event-poll seam: first forced refresh
and error gate, original poll once, second forced refresh/error gate, finalmove.
The separately recorded fixture amendmenta2a8c1de adds an actual-VM one-shot
hook witness for +1 fallback versus observer redispatch. Final source review
is pending under the unchanged04:10:41 ceiling; no runtime route is claimed.

The new retained-boundary script was rejected before execution, then rejected
once more through the same review path with the exact latest user authorization.
The second review requires specific approval of GetItem/Next scalar error and
borrowed SetDefault behavior after its downstream risk is disclosed. That
question is pending; no source/header/backups/start receipt from either attempt.
The held script/equivalents will not run. Distinct additive-only designf30310e3
passed review and separate execution approval: actual first edit03:44:44.893642.
It adds two owned providers/declarations/fixtures with all old contracts exact;
original03:52:40 checkpoint/04:32:40 ceiling remain. Core and bundled consumers
must still migrate. See the authorization-context and separate source grants.

Root compiler metadata checkpoint61a1d727 at03:46:25.540899 precedes03:55:07.
Exactly six bodies plus two includes are independently checkable against the
preserved baseline; all other source is exact. Direct inline integer arithmetic
and independently owned canonical keys/values replace borrowed scalar reads.
Post-allocation size checks and pre-store bounds protect tuple/cell offsets.
Full actual compile.c, assemble.c and flowgraph.c syntax passes. Four new test
expectations pass host-reference controls, not candidate execution; independent
source review is pending. Symtable/codegen/marshal/AST consumers remain required.

Typecache v2c172b13e remains design-only HOLD after review37695118. A live-write
ledger additionally identifies annotation getters/setters and static shared-tag
initialization/teardown obligations. No cache source grant exists. Loom62 was
published/read back;63 corrects only its UTC header from03:49 to actual03:47.
Latest63 has51,544 characters/five diagrams. No coherent restored runtime/timing.

Subsequent explicit user direction resolves the compatibility question: this
is a rebuild of Python and all extensions; use judgment for possible incremental
ports, with no hard compatibility gate. The additive bridge remains separate;
apply the reviewed remaining legacy-only delta against that frozen baseline,
not the stale combined script. No further compatibility confirmation is needed.

Compiler61a1d727 subsequently passed independent review39a69223 and closes its
source window early. The reviewer independently reproduced exact six-body/two-
include scope and bound all three passing actual syntax checks. Host fixture
controls remain distinct from candidate runtime; other consumers remain required.
