# Integrated buffer ownership and safeguard removal

Qualified source4da292b1294 is integrated as75e8a1e0c01. Runtime library/tests
remain dac6, ordinary native products3c6 and ASan nativef945; final12f1ea31
and postmergef56e0b6b bind qualification and both integration smokes. The
[independent LAND review](reviews/gc-main-only-final-landing.md) records the
actual composites, failed predecessors and support limits. This ledger covers
Linux, one main interpreter, ordinary threads and the shared GIL; it is not a
universal native-extension or foreign-lifecycle guarantee.

## Implemented invariants

`memory_dealloc` no longer coerces exports to zero. It retains the zero-export
assertion and ordinary release. Readiness selects eligible releases before
retaining each still-unready memoryview/managed-owner closure; a live outward
registration cannot disappear merely because its consumer is also dead.

`PyBuffer_Release` dispatches the real release callback unconditionally after
its existing non-NULL check. A debug invariant rejects BOTH DEAD and SWEPT
before reading the type. SWEPT aliases live MARKED, so neither flag alone is
the violation. The silent bypass is removed; no callback whitelist, forced
count or generic cleanup slot replaces it. Original acquisition metadata
belongs to its sealed owner, not a transformed consumer cache.

The ND and XID source proofs, assertion-enabled native controls, release/ASan
coverage and four copied omissions are complete at the identities below. The
one-file4da traversal allowance documents ND's exact borrowed aliases and
removes the deleted XI wrapper entry; no checker algorithm is weakened.

## Persistent storage inventory: Linux, one interpreter, shared GIL

| Holder/storage | Ownership and automatic disposal |
| --- | --- |
| `_PyManagedBufferObject.master`, `PyMemoryViewObject.view` (`Include/cpython/memoryobject.h:15,33`) | One owned master; each memoryview's copy is a registration, not a second master acquisition. Exact types have active/readiness checks, DESTROY consumption and unready-closure retention. No active exporter is allowed to disappear merely because its consumer is also dead. |
| `_struct.unpackiterobject.buf` (`Modules/_struct.c:2225`) | Sealed one-shot holder; ordinary finalizer closes/inerts it before release; traversal visits exporter. Existing explicit/native/error paths share the transaction. |
| `PyPickleBufferObject.view` (`Objects/picklebufobject.c:10`) | Sealed, non-reacquiring one-shot holder in627590; guarded ordinary finalizer/clear/explicit release, original stable view. |
| `ScannerObject.state.buffer` (`Modules/_sre/sre.h:88`) | Persistent scanner use of SRE_STATE is covered by627590; executing calls root scanner, idle finalization exhausts it before callback. Other SRE_STATE instances are function-local and explicitly finalized. Match objects hold their string, not another persistent Py_buffer. |
| Unpickler input |48b021 removes embedded acquisition, traverses a sealed managed owner and gates active input. Explicit replacement/reinit consumes old owner; automatic clear only drops it. Subclass __del__/reacquisition cannot replace the owner's finalizer or acquisition. |
| `_testbuffer.ndarray` static/base cell (`Modules/_testbuffer.c:96`) | Landed managed owner holds the stable original acquisition; cached/transformed metadata is borrowed, with separately ND-owned arrays. Ordinary outward registrations mirror owner.exports; redirect acquires separately upstream. Dealloc frees only its arrays and drops owner; inactive-owner gates protect data, metadata and re-export routes. |
| `struct xibuffer.view` (`Modules/_interpretersmodule.c`) | Temporary XID acquisition has an explicit root until existing disposal. Same-owner restoration creates an independent ordinary MV registration on the original managed buffer; foreign restoration rejects. The receiving wrapper/type/used-transfer state is removed. |
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

The former receiving xibufferview could finalize while a user finalizer resurrects
the receiving memoryview: all finalizers were queued before delivery. Releasing
the original root/acquisition leaves that resurrected view's cached input
active. Main-only admission does not make this supported same-owner route
unreachable. Readiness of an unrelated receiving managed buffer does not fix it.

Landed cut in `_memoryview_from_xid`:

1. Compare `_PyXIData_INTERPID(data)` with the current interpreter ID before
   accessing the shared object. Reject a foreign owner explicitly.
2. Return `PyMemoryView_FromObject(shared->view.obj)` for the same owner.
   `memory_getbuf` made view.obj the original exact memoryview; its temporary
   export prevents release while the new view is constructed.
3. Existing XID disposal removes the root, releases the temporary acquisition
   and raw-frees storage. The wrapper type, its finalizer and used-transfer
   flag are deleted; there is no unsupported foreign fallback.

`_PyXIData_NewObject` calls only the factory (`Python/crossinterp.c:1019`).
Both queue_get (`Modules/_interpqueuesmodule.c:1199`) and channel receive
(`Modules/_interpchannelsmodule.c:1944`) dispose XID on success and failure.
`_xidata_clear` invokes its free callback, which disposes the unused shared
buffer exactly once. Factory failure does not take disposal ownership. An internal caller still
follows its existing XID dispose rule.

FromObject on an existing memoryview shares its managed buffer and copies that
view's metadata (`Objects/memoryobject.c:817`). New and original registrations
can release independently; either can outlive the other without another raw
wrapper finalizer. This preserves slicing, format, shape, strides, readonly
state and mutable backing rather than copying data. The foreign wrapper type
is removed, with no fallback.

## Completed tests and causal controls

Completed controls use existing module seams: ordinary MV export/release and dynamic-export
readiness; struct/PickleBuffer/scanner callback cycles; the seven Unpickler
methods; ND's real transform/export/closed-owner cases; and same-interpreter
channel/queue transfer. The ND synthetic count block/unblock retention/OOM seam
proves policy mechanics only, not a real acquisition graph.

Same-owner transfer controls cover independent original/received release,
nontrivial view metadata and mutable backing, plus a cycle whose user finalizer
resurrects a usable received view.
Existing channel send_buffer/outlives-sender cases remain eligible. Failed
construction/disposal needs source closure and existing allocation-failure
coverage, not a new global fault hook. Foreign admission is explicit; no real
foreign interpreter is created just to exercise the unsupported branch.

The two removal invariants ran enabled in the assertion cohort; release and
ASan qualification exercise real callback/count balance on the coherent union.
All historical failures remain. No extra skip or repeated campaign was needed. The inherited live PickleBuffer Python-wrapper re-export failure stays
separate functional debt; new cycle tests do not prove raw()/memoryview works
for that live wrapper.

The union's four causal checks used **one copied negative union, one build,
four isolated case processes**, with disjoint source changes:

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
Actual copied negative35c92 uses the four disjoint omissions above. Indexe859dd0f
binds54 references: each isolated case reaches its named assertion with1 test,
1 failure, zero skips, actual kill/N0 and intact cleanup guards. Crashes or
harness failures are not credited. This is completed causal evidence, not a
new execution grant.


## Python resource wrapper found by full qualification

The f945 release full exposes SharedMemory.__del__: discarded native-only
memoryview slices can remain queued when automatic destruction requests
strict mmap.close. The six C acquisition-owner ports remain unchanged; this
is an additional Python resource-wrapper lifetime assumption. Reviewed
repair0d2116 drops SharedMemory's view/mapping references and closes its own
POSIX descriptor, leaving the managed acquisition to keep mappings usable
until their last views finish. Explicit close remains strict for live exports.
No forced count, skipped release, nested collection or completion DAG is added.

Controls validate discarded-view cleanup without unraisables, surviving data,
strict explicit close/retry and eventual Linux mapping/descriptor retirement.
Local track=False objects supply sanitizer unit checks; actual tracked
processes/misc modules and the ordinary memoryview manager race cover the
integration without bypassing the existing ASan fork guard. The final repair
is dac6: four local track=False tests in test_memoryview
avoid the sanitizer fork guard without bypassing it. All six positive runs
pass, and the old-policy worker fails the intended captured-BufferError
assertion (terminal58723869/all48 references). Explicit release557+7 and
ASan15+memoryview assemblies preserve the old source/resource contexts;
normal/kill debug fulls each pass51,240 tests, both stress rows and closure
pass, and integration75e8 has both1,935-case smokes. Final12f1 and postmergef56e
retain all original FAILs and exact native/library/metadata identities.
No supported callback/buffer-completion blocker remains from this ledger;
foreign/adoption, platform and precision obligations stay separate.
