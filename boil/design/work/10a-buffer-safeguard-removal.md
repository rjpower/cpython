# Final buffer safeguard removal: source cut

Source-only assessment on clean composed candidate
`dc7bd5c19f7563c04318edf0d1698352567f066f`, including sealed consumers
`6275905c8c0` and Unpickler `48b02122321`. NDArray is still an in-progress
source port from `db025699`; its implementation and qualification are not
credited here. No native execution or new exclusion is granted by this assessment.
Reviewed follow-ups now compose as3c6b8daa506: ND baf8, XID9d246659 and
the two-invariant cut424b8a394c7. Actual-union native validation is pending.

## Recommendation

Remove the two silent sweep accommodations on the actual composed owner union,
after closing the same-interpreter XID path below. Keep the existing readiness
and full-closure retention policy. Do not replace it with count coercion, a
callback whitelist, or a generic cleanup slot.

`Objects/memoryobject.c:1211` currently overwrites exports with zero immediately
before asserting zero. Delete the overwrite and its obsolete sweep-order
comment; retain the existing assertion and normal `_memory_release`. The
readiness rule already excludes a memoryview with active outward exports from
sweep: `queue_destroy_hooks` selects ready releases first, then resurrects
every still-dead unready memoryview/managed-buffer closure
(`Python/gc_live.c:1020`). A released memoryview can have no legitimate later
export, because memory_getbuf rejects it and release required zero exports.
This invariant does not depend on an arbitrary number of collection passes.

`Objects/abstract.c:823` currently drops bf_releasebuffer when the exporter has
both DEAD and SWEPT flags. Remove that bypass; require that a non-NULL exporter
is not already swept, checked before dereferencing its type/callback. A debug
assertion is the smallest normal invariant check; a production violation, if a
check is retained there, must fail loudly rather than clear the view and claim
release. Check the conjunction
`(flags & (DEAD | SWEPT)) != (DEAD | SWEPT)`: SWEPT aliases the live MARKED bit,
so neither SWEPT alone nor DEAD alone is the violation. Do not add a blanket
prohibition on
native release functions or classify them as harmless by name. On the supported
ports, callback-bearing acquisitions are consumed before sweep, while transient
views keep their exporters live throughout the explicit release.

This cut is not ready merely because the two lines are small: the composed ND
owner/export proof, XID cut, and relevant native controls must be accepted first.

## Persistent storage inventory: Linux, one interpreter, shared GIL

| Holder/storage | Ownership and automatic disposal |
| --- | --- |
| `_PyManagedBufferObject.master`, `PyMemoryViewObject.view` (`Include/cpython/memoryobject.h:15,33`) | One owned master; each memoryview's copy is a registration, not a second master acquisition. Exact types have active/readiness checks, DESTROY consumption and unready-closure retention. No active exporter is allowed to disappear merely because its consumer is also dead. |
| `_struct.unpackiterobject.buf` (`Modules/_struct.c:2225`) | Sealed one-shot holder; ordinary finalizer closes/inerts it before release; traversal visits exporter. Existing explicit/native/error paths share the transaction. |
| `PyPickleBufferObject.view` (`Objects/picklebufobject.c:10`) | Sealed, non-reacquiring one-shot holder in627590; guarded ordinary finalizer/clear/explicit release, original stable view. |
| `ScannerObject.state.buffer` (`Modules/_sre/sre.h:88`) | Persistent scanner use of SRE_STATE is covered by627590; executing calls root scanner, idle finalization exhausts it before callback. Other SRE_STATE instances are function-local and explicitly finalized. Match objects hold their string, not another persistent Py_buffer. |
| Unpickler input |48b021 removes embedded acquisition, traverses a sealed managed owner and gates active input. Explicit replacement/reinit consumes old owner; automatic clear only drops it. Subclass __del__/reacquisition cannot replace the owner's finalizer or acquisition. |
| `_testbuffer.ndarray` static/base cell (`Modules/_testbuffer.c:96`) | Pending ND port: stable original acquisition belongs to a traversed managed owner; cached/transformed base is borrowed metadata, its separately allocated arrays remain ND-owned. Ordinary outward registrations must mirror owner.exports; redirect uses a separate upstream acquisition. Dealloc must not free the original view metadata or release an owner during sweep. Exact final implementation still needs independent review. |
| `struct xibuffer.view` (`Modules/_interpretersmodule.c:140`) | Raw heap acquisition has an explicit PyGC root, normal XID disposal, and receiving xibufferview finalization. This path remains reachable through same-interpreter queues/channels despite main-only admission. Its receiving-view resurrection hole requires the cut below. |
| `_testbuffer` static `info`/`static_buffer` (`2258`, `2721`) | Test-only borrowed descriptors/static storage, not independent owned acquisitions; FromBuffer explicitly sets master.obj=NULL. Deliberately fragile/noncompliant exporter seams do not establish production buffer ownership. |
| Indirect native consumers such as ctypes.from_buffer | Retain a memoryview through ordinary traced object ownership (`Modules/_ctypes/_ctypes.c:997` KeepRef); no additional persistent Py_buffer to release. Arbitrary user-supplied native addresses and explicit unsafe ctypes manipulation remain their existing contracts. |

Repository declaration and acquisition searches also covered `Include`,
`Objects`, `Modules` and `Python`, excluding generated Clinic duplicates.
Function-local views/Clinic arguments, getargs cleanup, bytes-join/socket buffer
arrays, compression/hash/I/O calls and native test helpers are transient, not
new long-lived owner types. Their explicit success/error release paths and
current opaque-frame/PyMem-carrier roots remain required; this is not a new
precision proof for every helper. `Python/gc_trace.c:35,1225` explicitly scans
PyMem carriers reached from opaque C frames, not arbitrary object-owned arrays.

Windows `_overlapped` union acquisitions (`Modules/overlapped.c:98,117`) and
`_winapi.Overlapped.write_buffer` (`Modules/_winapi.c:151`) remain explicitly
unsupported in this Linux milestone. No claim is made for external extension
holders that neither trace nor explicitly root their acquisition owner.

## XID: reuse the main interpreter's managed buffer

The old receiving xibufferview can finalize while a user finalizer resurrects
the receiving memoryview: all finalizers were queued before delivery. Releasing
the original root/acquisition leaves that resurrected view's cached input
active. Main-only admission does not make this supported same-owner route
unreachable. Readiness of an unrelated receiving managed buffer does not fix it.

Smallest source cut in `_memoryview_from_xid`:

1. Compare `_PyXIData_INTERPID(data)` with the current interpreter ID before
   accessing the shared object. Reject a foreign owner explicitly.
2. Return `PyMemoryView_FromObject(shared->view.obj)` for the same owner.
   `memory_getbuf` made view.obj the original exact memoryview; its temporary
   export prevents release while the new view is constructed.
3. Leave shared.used zero. Existing XID disposal removes the root, releases the
   temporary acquisition and raw-frees its storage. No ownership transfer into
   xibufferview occurs in the supported path.

`_PyXIData_NewObject` calls only the factory (`Python/crossinterp.c:1019`).
Both queue_get (`Modules/_interpqueuesmodule.c:1199`) and channel receive
(`Modules/_interpchannelsmodule.c:1944`) dispose XID on success and failure.
`_xidata_clear` invokes its free callback, which disposes the unused shared
buffer exactly once. Factory failure must not change used or take disposal
ownership. An internal caller must still follow its existing XID dispose rule.

FromObject on an existing memoryview shares its managed buffer and copies that
view's metadata (`Objects/memoryobject.c:817`). New and original registrations
can release independently; either can outlive the other without another raw
wrapper finalizer. This preserves slicing, format, shape, strides, readonly
state and mutable backing rather than copying data. The unused foreign wrapper
type may be removed as separate cleanup; it must not remain a foreign fallback.

## Minimal tests and causal controls

Use existing module seams: ordinary MV export/release and dynamic-export
readiness; struct/PickleBuffer/scanner callback cycles; the seven Unpickler
methods; ND's real transform/export/closed-owner cases; and same-interpreter
channel/queue transfer. The ND synthetic count block/unblock retention/OOM seam
proves policy mechanics only, not a real acquisition graph.

Add a same-owner transfer control with independent original/received release,
nontrivial view metadata and mutable backing; then collect a cycle whose user
finalizer resurrects the received view and verify its bytes remain usable.
Existing channel send_buffer/outlives-sender cases remain eligible. Failed
construction/disposal needs source closure and existing allocation-failure
coverage, not a new global fault hook. Foreign admission is explicit; no real
foreign interpreter is created just to exercise the unsupported branch.

The two removal invariants must run enabled in an actual assertion cohort;
release and ASan qualification then exercise real callback/count balance on the
coherent union. Keep all historical failures. No skip or new repeat campaign is
needed. The inherited live PickleBuffer Python-wrapper re-export failure stays
separate functional debt; new cycle tests do not prove raw()/memoryview works
for that live wrapper.

For the remaining union's four causal checks, prefer **one copied negative
union, one build, four isolated case processes**, with disjoint source changes:

| Omission | Existing isolated case and required failure |
| --- | --- |
| Replace interpreter queue-draining admission with the former per-thread running_queue guard; keep active-call roots unchanged | `test_gc.SingleDrainerTests.test_worker_collect_returns_without_overtaking`: named "inner callback overtook outer callback" assertion, with bounded worker completion. |
| Remove only scanner_finalize's pre-release start=NULL | `test_gc.BufferLifetimeTests.test_scanner_cycle_release`: named "release must observe an exhausted scanner" assertion. Existing global backing prevents stale-storage manufacture. |
| Add `_Unpickler_ReleaseInput(self)` before preparing the new owner, leaving remaining code/tests unchanged | `test_pickle.CUnpicklerBufferOwnerTests.test_failed_replacement_keeps_old_acquisition`: original acquisition-failure ValueError is caught, then the external no-release assertion detects premature consumption. It never reads the invalid old cache. |
| Remove only ndarray_tobytes' active-owner gate | `test_gc.BufferLifetimeTests.test_ndarray_cycle_release_and_resurrection`: named "released ndarray remained readable" assertion. Global backing stays live during the callback read; no stale-storage crash is needed. |

These cases do not activate one another's defect: the drainer case uses ordinary
Python finalizers, the scanner case has no Unpickler replacement/concurrent
drainer, and the Unpickler case has no scanner or second collector thread. The ND
case has no scanner/Unpickler/concurrent drainer; only its tobytes gate is
omitted.
Keep each exact native failure and structured named assertion separately;
crash, timeout, import, compile, helper or unexpected error is no credit.
Use the accepted wrapper and offline failure accounting, not a new runner.
One configured tree with successive tiny commits would also isolate them but
adds three make/checkpoint phases without a demonstrated attribution need.
Freeze the actual four-omission negative for review only after the positive union
is fixed; none of these commands or patches is authorized by this note.
