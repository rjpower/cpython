# Persistent buffer consumers: native release readiness

Source-only, 2026-10-30. Audited frozen5a45 runtime in
`boil-pre-i-union@5a45f9bb6a73e7351c861faf05624ae4810f6d87`;
primary PROCESS §0.3 and architecture review2 §5.3 name these five families.
No code, build, target, hook or probe was added/executed. Queue2a V2/S3
source/products and historical evidence are untouched.

**Finding:** adding five `tp_finalize` functions is not a complete repair.
The four non-subclassable types permit local finalizers, but Unpickler's
Python subclasses can replace the slot with `__del__`. Reinitialization,
reentrant release and resurrection also require a release-per-acquisition
contract. A native cleanup phase independent of user finalization is the
cleaner full-family direction, potentially reusing CALL_DESTROY scheduling;
its interface and completion semantics still need a coordinator decision.
No new type field/manual type inventory is proposed as approved code.

## Actual release guard and why this debt is real

`Objects/abstract.c:823 PyBuffer_Release` reads view.obj, returns if NULL,
and suppresses bf_releasebuffer only when the exporter is BOTH DEAD and
SWEPT. Otherwise it invokes the callback, then sets view.obj=NULL and
DECREFs. It does not test gc.sweeping, queue a release, or consume the view
before entering Python. Thus the retained-exporter case can run Python
inside sweep; an already-swept wrapper/exporter case silently skips it.
Deferred block frees do not preserve already-destroyed native export state.

For Python exporters, `slot_bf_getbuffer` (`typeobject.c:11418`) creates a
private buffer-wrapper retaining the returned memoryview and exporter.
`_PyBuffer_ReleaseRunsCode` (:11391) recognizes that wrapper and the Python
release slot. `bufferwrapper_releasebuf` may call Python before clearing
its retained fields. `releasebuffer_call_python` (:11526) requires a valid
buffer.obj and uses the existing precise argument/exception span. Do not
set the passed view.obj=NULL and then call that callback with an invalid
view. Do not remove the DEAD|SWEPT guard while other consumers still rely
on it for already-destroyed export state.

## Finite owning-field/lifecycle inventory

| Consumer / source | Owning view and acquisition | Explicit/clear/dealloc and failure behavior | Type/reuse constraint |
| --- | --- | --- | --- |
| PickleBuffer, `Objects/picklebufobject.c:16,68` | Embedded view; constructors set obj=NULL then GetBuffer(FULL_RO); traverse visits view.obj. | C Release(:53), Python release(:189), clear(:100), dealloc(:108) all call PyBuffer_Release; dealloc untracks/clears weakrefs first. Failed GetBuffer leaves no active view under API contract. `raw`/getbuf reject only obj=NULL. | Static, HAVE_GC, no BASETYPE; no reinit/acquire API after creation. Explicit/reentrant release still needs one transaction shared by all paths. |
| Unpickler, `Modules/_pickle.c:1294,7859` | Embedded buffer plus cached input_buffer/len/index/frame state. SetStringInput releases old view when buf!=NULL then GetBuffer(CONTIG_RO). File read/peek paths also call it, not just loads. traverse(:7904) visits buffer.obj. | clear releases buffer, sets buf=NULL and clears Python/native state; dealloc calls clear. __init__(:7965) may clear/reinitialize, under existing running guard. Factory _Unpickler_New(:1805+) zeroes view; GenericNew zeroed allocation supports partial/missing init. `_pickle.loads` creates an internal Unpickler retaining its input view after load until death. | Heap BASETYPE; overriding __del__ replaces tp_finalize. Public load/reinit can acquire again after resurrection. C running guard already blocks reentrant load/init but native cleanup must participate. |
| SRE scanner, `Modules/_sre/sre.c:685,3175` | Embedded SRE_STATE.buffer, plus state.string and pattern; state_init memset zeros state, acquires bytes view, retains string. traverse(:3019) visits both. | state_init error releases acquired view; state_fini(:771) releases it and frees native stacks/marks/pools. Scanner dealloc(:3044) calls state_fini then scanner_clear; scanner_clear only clears pattern. Successful search/match/exhaustion does NOT release scanner's retained view. Local non-scanner state users also call state_fini. | Heap immutable, disallow instantiation, no BASETYPE/no public reinit. Existing executing flag guards calls. Released/resurrected scanner must reject methods before dereferencing state raw pointers; cannot simply release view and leave an apparently live scanner. |
| Struct unpack iterator, `Modules/_struct.c:2221–2365` | Embedded buf, struct owner so, index. GenericAlloc zeroes state; GetBuffer(SIMPLE), length validation, then so publication. traverse includes buf.obj. | Exhaustion clears so BEFORE release(:2284); dealloc(:2232) releases remaining view. Failed GetBuffer/length check eventually deallocs a partial iterator. No tp_clear. | Heap immutable, disallow instantiation, no BASETYPE/reinit. Existing so=NULL gives safe exhausted next/length behavior, a useful consume-before-callback boundary. |
| ndarray consumer, `Modules/_testbuffer.c:207–275,1293` | Embedded staticbuf.base when head==&staticbuf (ND_IS_CONSUMER), distinct from producer linked buffers. GetBuffer sets head only after success; traverse(:246) visits consumer base.obj despite no HAVE_GC flag. | Dealloc frees ND_OWN_ARRAYS metadata then releases base; producer branch frees ndbuf chain. Consumer push/pop reject. init is exposed and currently has no initialized/running/reinit cleanup guard. Re-export may redirect to base.obj or retain ndarray; buffer release decrements producer export counts only. | Static, non-GC, no BASETYPE. Consumer identity is head address, NOT base.obj. Preserve producer cleanup. A finalized consumer's raw buf/arrays cannot remain usable after release; every operation/getset/export path needs a coherent released-state rule. Repeated init currently can overwrite active state: don't treat that as a safe reacquisition protocol. |

All five traversals retain their exporters in the current collector; this
is not a missing-traverse proposal. SRE_STATE's buffer API is shared with
stack-local match/search/split/substitution calls: do not postpone all
state_fini calls or change those explicit releases. ndarray's producer,
ND_OWN_ARRAYS, redirect/slicing and dynamically replaced ndbuf paths are
separate from the one consumer-held Py_buffer and must stay correct.

## Exactly once means acquisition → release, not once per object

Before a callback can allocate, collect, yield or reenter consumer methods:

- Publish releasing/inert consumer state. Use existing state where enough
  (unpack iterator so=NULL; Unpickler running; scanner executing), otherwise
  one finite private state field and all affected access checks are needed.
  Reentrant explicit cleanup must see consumed/releasing state and return
  without invoking the same view again. Do not rely only on the queue's
  finalized bit: explicit release does not consult it.
- Keep consumer, exporter and all Py_buffer-referenced shape/stride/format/
  internal storage rooted and valid until bf_releasebuffer returns. Existing
  owner traversal and precise spans are the mechanisms; no new root system.
  ndarray currently frees owned arrays BEFORE release; a callback-safe
  path must retain those arrays until the callback is finished.
- Consume exactly that acquisition, then clear buf/cached pointers and
  finish native cleanup once. A callback cannot install a new acquisition
  that an old release's tail then clears. Reject mutation while releasing,
  or specify a real detached transaction with stable metadata and roots.
- A copied Py_buffer is not automatically a complete solution: metadata
  may point into the original view/storage, and a callback expects obj
  valid. If copying is chosen, prove those pointers/storage remain valid;
  don't zero/free/reinitialize the original metadata across the callback.
  A consumer-local releasing guard can preserve the original view address
  and avoid changing the global PyBuffer_Release ABI/semantics.

After completion, PickleBuffer stays released; unpack iterator stays
exhausted. Scanner and ndarray need explicit safe inert states if a
callback finds/saves the consumer. Scanner cannot reuse stale start/end
pointers; ndarray cannot export/use a released data pointer. Guard all
public paths, not just the finalizer. Unpickler's later successful reinit
or input replacement is a NEW acquisition requiring new cleanup eligibility.

## Compare the existing protocols before choosing an interface

`object.c:172 PyObject_CallFinalizer` sets FINALIZED before dispatch and
calls only the actual type's tp_finalize. Type inheritance copies the slot
(`typeobject.c:8863`), while `__del__` uses slot_tp_finalize (:11304,11766).
An Unpickler subclass's __del__ can omit super().__del__; no base cleanup
is guaranteed. Calling a C base finalizer from slot_tp_finalize would also
miss a C subtype's different finalizer and couple user finalization to
mandatory native destruction. Removing BASETYPE would break ordinary
Unpickler customization, not merely migrate an internal layout.

GC-tracked objects keep FINALIZED after resurrection (`gc_trace.c:2151`),
so a resurrected/reinitialized Unpickler can acquire a new buffer without
another tp_finalize call. Non-GC ndarray's bit is cleared when marked
reachable; that does not make its reinit/inert-state paths safe, and does
not cover becoming dead again before a subsequent reachable mark. Do not
rearm user __del__ merely to release a new native buffer.

Existing native-hook machinery is distinct:

- `_Py_TPFLAGS_DESTROY_HOOK` is selected by type_ready_destroy_hook
  (`typeobject.c:9373`); live_destroy_hook_of/has_destroy_hook
  (`gc_trace.c:2059,2090`) inspect known kinds and NOTIFIED. Current buffer
  support is exact memoryview/managedbuffer only, through
  `_PyMemoryView_DeallocReleasesWithCode`, not these five consumers.
- `queue_destroy_hooks` (`gc_live.c:933`) queues ALL eligible dead hooks
  before resurrecting their object graphs, sets NOTIFIED on successful
  node allocation, then routes existing CALL_DESTROY. run_destroy_hook
  (:386) dispatches existing watcher/extras/module/capsule/memory handlers.
- Discovery occurs after finalizer batching/resurrection. Thus a consumer
  with an ordinary user finalizer is kept intact through that call; native
  cleanup can occur at its later actual death. This is a useful existing
  separation. A finalizer-resurrected consumer is not cleaned prematurely.
- The mechanism is once-per-OBJECT; NOTIFIED is not generically cleared on
  resurrection. Native buffer cleanup needs acquisition rearming or a
  separately proved irreversible-closed consumer contract. Merely adding
  five type checks to discovery and dispatch leaves that issue unresolved.

The clean full-family option to REVIEW is an inherited native pre-dealloc
cleanup capability, independent of Python __del__, using the existing
CALL_DESTROY batch/root/owner transport. Actual callback storage, eligibility
and rearming need a focused type-interface decision: there is no such
generic slot today. Compare a native callback/capability against the
existing finite dispatch pattern; don't add a new queue, per-package
registry or another manual parallel type table. If an actual type field is
chosen, it needs layout/slot-construction/inheritance/C-subtype and rebuilt
extension review. Using tp_clear indiscriminately is not equivalent:
clear has broader field teardown semantics and currently has no mandatory
independent queued pre-dealloc phase; calling every tp_clear as native
destruction would silently change unrelated types.

## Completion and ownership prerequisites

Both approaches queue on the CONSUMER object's owner via existing owner
routing. The subsequent buffer callback runs on that same thread, as an
explicit release does; it is not automatically rerouted to the Python
exporter's owner. Shared-heap cross-owner callback policy must remain
explicit. `_PyBuffer_ReleaseInInterpreter` takes an explicit interpreter;
it is not automatic owner inference for these calls.

A release callback may block with the GIL released, use live exporter
resources or collect. Mere rooting/FIFO invocation cannot preserve resource
completion. Existing dependency design makes FINALIZE/LEGACY predecessors
and DESTROY waiters. If native buffer cleanup uses CALL_DESTROY, that is
not enough by itself: a consumer native release and its exporter's own
native release can both be queued (all hooks are queued before resurrection).
Their callbacks must not overlap in an order that destroys a needed export
resource. Review whether native cleanup becomes a completion predecessor,
or prove a narrower phase rule. Do not silently classify every watcher or
weakref callback differently. This is the same exact per-call dependency
mechanism, not justification for a second release scheduler.

Close/fork/cancel must preserve that protocol and rooted view lifetime;
these remain Queue2a lifecycle prerequisites. Adding local finalizers or
new cleanup discovery is not final GC correctness while they are open.

## Smallest coherent source slices and checks, still ungranted

A genuinely small independent option is ONE unpack-iterator lifecycle
change: reuse its exhaustion transaction (so=NULL before release) in an
idempotent tp_finalize; retain dealloc fallback for already released/partial
instances. No subclasses/reacquisition exist. This is not a five-family
repair and should not pre-empt a uniform interface decision if it would
just be rewritten. Other local finalizers require the state work above;
Unpickler remains blocked by user-finalizer replacement/reacquisition.

Full-family implementation files would be the five consumer C files,
plus the chosen existing type/collector hook interfaces in
`Objects/typeobject.c`, `Python/gc_trace.c`, `Python/gc_live.c` and only
necessary type/header/slot construction declarations. `Objects/abstract.c`
need not change its global guard for a consumer-local transaction. SRE
scanner-only state may need `Modules/_sre/sre.h`. Tests belong in existing
`test_buffer`, `test_pickle`/`pickletester`, `test_re` and `test_struct`.
This is a source boundary proposal, not permission for that diff.

Use a retained pure-Python exporter whose __buffer__ returns a memoryview
and whose __release_buffer__ records identity/count, allocates and calls
gc.collect. Each family gets acquire/drop/repeated-collect checks, retained
exporter/payload, explicit release/exhaustion then death, acquisition/error
cleanup, and reentrant cleanup. Unpickler includes no-super __del__, load
input replacement, reinit and resurrection/reacquisition; scanner/ndarray
include saved-consumer inert behavior; ndarray includes own-array metadata
and redirect/producer controls. Keep exception preservation and exact once
counts, not just process exit. Existing test_buffer Python protocol tests,
re.test_keep_buffer/scanner lifetime, struct arbitrary-buffer/iteration and
pickle bad-init/buffering/oob coverage provide the affected suite context.

An old build may miss release or execute Python from sweep; don't promise
one particular old symptom without evidence. A deterministic copied source
control can consume/detach the view but omit its release callback, so the
retained-exporter assertion fails at a named missing-release count without
intentionally executing Python in sweep. A separate old-source comparison,
if granted, must preserve any swept-exporter skip/guard/crash honestly.
No raw-address fixture or new test hook is needed for this contract.
Actual release/debug-kill/causal outcomes and owner/dependency coverage
require their own reviewed source/launches; none is claimed here.
