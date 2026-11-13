# Lifecycle reader census and first consolidation

2026-11-12 source census refreshed against integrated `9136c202e8e` (constructor
`05701020b6a` plus regeneration). The phase consumer `b27a1a21330` is committed
separately and awaits the IR definition/publication ports. The heap-deallocator
candidate has source GO but is not in this checkpoint. NULL remains authority.
This is the port list for the atomic map switch; it does not authorize removing
the arena scanner or treating an empty-return certificate as complete.

`L` means named binding storage (including cells/freevars/constant homes), `N`
the entire admitted logical home domain. A loop over `N` must not decode stale
scratch just because its address is reserved. Address bounds, initialized-value
authority, logical owner obligation, counted GC edge and external marking are
different properties.

## Concrete consumers

| Consumer / existing loop | Actual state read now | Required map/lifecycle port |
| --- | --- | --- |
| `Python/frame.c:_PyFrame_Traverse` → `Python/gc.c:_PyGC_VisitFrameStack` | Typed headers, then every `[0,N)` home, skipping NULL/scalars/indices. GIL `_PyGC_VisitValue` preserves counted-edge policy for subtraction callbacks. | Keep this existing traversal entry as the main collector home selector. Dynamic local binding state plus certified scratch phase/ranges; no stale read. Preserve typed headers and unique counted edges. |
| `gc_free_threading.c:_PyGC_VisitFrameStack` | Same full range, with backend-specific deferred/count subtraction decisions in `_PyGC_VisitValue`. | Use the same selected-home geometry as GIL while retaining the backend policy. Do not turn external transition marking into deferred-count adjustment. |
| `gc_trace.c:_PyGC_TraceVisitFrames` | Active chain, borrowed globals/builtins and `_PyFrame_Traverse`. | Preserve this actual shared consumer rather than add a second map decoder. Caller/child overlap must partition aliases at the published phase. |
| `gc_free_threading.c:gc_visit_thread_stacks` | Full `[0,N)` in reverse, adding only deferred values missing physical counts. | Needs selected homes **and their tags**; an object-only callback loses deferred/count distinction. Do not replace it with an unconditional object visit. |
| `gc_free_threading.c:gc_visit_thread_stacks_mark_alive` | Optional external marking of typed pins and every home, followed by transition chain. | Same active map geometry; retain separate external-mark policy. A shared geometry implementation cannot silently alter subtraction semantics. FT still needs separate qualification. |
| `genobject.c:gen_traverse` | Live/suspended generator delegates to `_PyFrame_Traverse`; cleared generator visits retained code; exception state/origin/name roots remain enclosing-object edges. | Explicit created/suspended/yield-from/error phases; preserve code-only cleared state and exception ownership. Read no uninitialized half-built frame. |
| `frameobject.c:frame_traverse` | Frame-owned heap storage delegates to `_PyFrame_Traverse`; association with a thread/generator frame does not duplicate its home edges. | Retain ownership gate after takeover. An associated observer frame is not another container owner of active homes. |
| `pycore_interpframe.h:_PyFrame_Copy` | Moves and NULLs all `N` homes for escape or generator creation, before rebasing to heap `owned_slots`. | Copy initialized authorized homes, establish destination local validity, and remove source edge authority once. Capture the exact active/suspended/construction phase and alias extent; never copy stale scratch as values. |
| `frame.c:take_ownership` | Copies frame, acquires an independent source code pin, changes container owner, disarms retiring prefix/header authority before callbacks. | Preserve this authority transfer. Code remains available through the last source descriptor read, and destination heap maps must exist before it becomes a traversable owner. |
| `frame.c:clear_slots`, `_PyFrame_ClearLocals` | Reverse full-prefix loop, NULL before each close, then mapping close. | Common release geometry and root progress for heap clear; preserve callback-time named-local writes. The existing API can replace an actual duplicate now. |
| `frame.c:_PyFrame_TryClearExceptCode`, `_PyFrame_FinishClearExceptCode` | Thread normal/error retirement: scratch already NULL, locals prefix; heap/gen retirement: full range. Private `clear_thread_frame_slow` / `clear_generator_slow` own reached records. Association can cause takeover. | Explicit entry reason and remaining map cursor, association/mapping/function/code/exception order, fixed-last code and reservation lifetime. Fast callback-free partial progress must not infer a map from opcode alone. |
| `frame.c:_PyFrame_ClearGenerator`, `genobject.c:gen_clear_frame` | Shared direct-close/dealloc/completion producer, frame then exception retirement with retained code. | Cover untracked generator exception roots, direct unstarted clear, completion result and takeover. Do not pop roots before exception cleanup. |
| `frameobject.c:frame_tp_clear` | Clears observer metadata, then calls existing `_PyFrame_ClearLocals`; code/function remain owned. | Consolidation `6ef8868ac53` is implemented. Port that existing home selector; retain mutable bindings over all L through callbacks, including writes above the descending cursor. |
| `frameobject.c:frame_dealloc` | Untracks/clears weakrefs, then currently clears code/function/mapping **before** reverse home close, followed by observer-owned fields. | Code must survive mapped home interpretation. A blind call into thread/gen retirement is wrong. Freeze heap clear/dealloc callback, mutation and resurrection authority before a shared release implementation. |
| `pycore_interpframe.h:_PyFrame_ClearTemporaries` | Reverse scratch suffix from supplied `first`; each nonNULL home NULLs before close. | Error/source-error/redirect phase selects actual owner obligations; preserve release order and dynamic remaining cursor across callbacks. |
| `bytecodes.c:exception_unwind` | Clears suffix, installs lasti/exception, changes handler PC, then monitor callback. | Exact pre-error/partial-consume and handler-entry maps; lasti is protocol data, not a heap object. RERAISE's stored PC is explicitly not the unwind authority. |
| `frame.c:_PyFrame_CompleteRedirect`, bytecodes InitCleanup | Completes parent redirect after child relinquishes overlap; explicit trampoline cleanup. | A separate explicit executable/phase contract; do not use an ordinary code certificate for static entry or InitCleanup words. |
| `gc_trace.c:scan_thread_struct` | Current value/header regions and whole older chunks conservatively scanned, covering unlinked reservations. | Remove only after active/staged/retiring/heap lifecycle census closes. Preserve independently selected native C/PyMem conservative policy. An address pointing into an arena is not complete arena enumeration. |
| `gc_trace.c:_PyGC_TraceVisitTransitionRoots` | Initialized remaining prefix, typed header mask, generator exception, detached result and optional initialized suffix/range. | Same release recipe and cursor as producer. No code/geometry read after code bit disarms; prefix-only records never infer active-frame geometry. |
| M3 `frame_lineno_set_impl` retirement helper; generic unpack adapter | Detached PyMem retired array / initialized arena output suffix remains live through actual closes. | Preserve prefix authority through nested redirect/collection and iterator/surplus/sequence cleanup. Publish verified active POST before disarm under maps. |
| `ceval.c:stage_binding_frame`, `eval_frame_push_and_init_code`, fallback binder, `_PyEval_VectorValue`; `call.c:_PyEval_Vector` | Full initialized not-current child homes and nullable typed headers through binding/provider cleanup. Provider token disarms at actual publication/retirement; empty outer node survives hook cleanup. | Implemented M3 bridge. Replace full-N staging only with real initialized-prefix/phase capability; never infer it from parent PRE. Keep CODE and dynamic nullable mapping/association authority. Preserve strict-token versus optional-default handoff. |
| `ceval.c:_PyEval_CreateInitFrame`, `create_init_frame_slow`, `clear_init_shim` | Exact entry is callback-free; generic miss moves caller inputs to native workspace and retains full shim/child stages. Flagged shim metadata blocks parent redirect through failure closes. | Implemented in this checkpoint. Static InitCleanup needs its own self/result recipes; staged binder progress and child clobber remain map gates. Do not let generic provider lookup consume flagged shim metadata. |

## Observers, named bindings and diagnostics

| Concrete reader | Required authority |
| --- | --- |
| `frameobject.c:framelocalsproxy_getval/hasval/getkeyindex`, key/value/item/length iteration and `framelocalsproxy_setitem` | Dynamic local boundness before value reads, cell-home versus cell-content boundness, readonly constant homes and replacement publication before old-owner close. Preserve `f_overwritten_fast_locals` ownership for outstanding borrows. |
| `frameobject.c:frame_get_var`, closure population, `PyFrame_GetVar*`, legacy fast-to-locals interfaces | Same named binding policy, first COPY_FREE_VARS/MAKE_CELL state, virtual `pending_kwargs` materialization and possible boxing failure. No instruction-site scratch map substitutes for mutable local validity. |
| `typeobject.c:super_init_without_args` | Local0 and `__class__` cell availability, including deleted first arg, incomplete-cell and empty-cell distinctions. This direct local reader sits outside frameobject.c. |
| `frameobject.c:register_resume_liveness` physical `null_home` loop, `register_saved_locals_plan`, `register_resume_plan` required/protocol-home reads | Existing observer transfer/origin proof plus actual selected readable state. Stale zero/equal bits cannot restore a missing saved-binding token or NULL protocol. Shared semantic plan must replace duplicate scratch authority, without broadening jump admission. |
| `frameobject.c:frame_lineno_set_impl` restore and scratch-retirement loops | Detach only actual owner homes, publish restored bindings/continuation before close, preserve active-child overlap and nested local mutation. Already outlined retired-array prefix remains live. |
| `genobject.c:_PyGen_YieldFromRef`/`gen_getyieldfrom`, `instrumentation.c` SEND receiver reader and remote asyncio yield-from reader | Certified suspended/selected SEND receiver at `return_slot-2`, not an arbitrary in-bounds home. Retain owner/state/range/address validation and FT read-lock protocol. |
| `ceval.c:value_operation_restore2`, result/method publication, call record refresh/detach/childclobber, step completion | Per-family PRE/staged/POST protocol. Replace old NULL destination tests with explicit inactive/available authority only after phase proof, while reloading mutable local destinations after callbacks. |
| `ceval_macros.h:IR_SLOT`, `GETLOCAL`, consume/store macros and generated handlers | Generator's declared reads/consume/dup/transport/publication events. Static initialized and logical ownership proof must cover all early exits and callbacks, not only collector loops. No generic hot decoder. |
| `object.c:PyUnstable_Object_IsUniqueReferencedTemporary` | Currently walks every scratch home and decodes tags. Must search certified live temporary owners only; a stale duplicate word cannot prove a unique temporary. Preserve actual count-tag test. |
| `frame.c:_PyValue_CheckFrame` | Currently validates every local/scratch word, including pointer type reads. Validate only initialized authorized homes; never dereference dead stale words in a diagnostic. |
| `ceval.h:dump_stack` and lltrace | Currently dumps all local/scratch homes. Show inactive homes as inactive without decoding them; use declared diagnostic phase, preserving exception state and no Python repr calls. |
| `pycore_interpframe.h:_PyFrame_AssertTemporariesClear`, overlap reservation assertions, PopFrame physical departing-suffix assertion | Replace raw all-NULL inspection with certified no-unretired-owner/clobber invariants when NULL stops being authority. No debug-only stale read. The physical reservation is larger than the logical child on some overlaps. |
| `_testinternalcapi:tagged_mortal_slots`, inline locals, `native_scope.c` whole-frame snapshot and arena tests | Use chosen home authority or explicitly inspect raw storage without dereferencing it. Existing 64-home bounded snapshot fixture is not a runtime map-domain cutoff; production plans cover all4095 homes. Preserve a separate canonical NULL shadow control while required. |
| `_testinternalcapi:get_transition_root_ids/get_transition_shadow_status` | Root visitor uses explicit progress; status query observes stored bytes only and remains valid after code detach. No inspection query recreates producer coverage. |
| Remote frames/debug offsets, GDB `iter_locals` and typed header readers | Port validity/physical-span schema when changed; retain torn-sample, sentinel and pointer-range checks. Known GDB quarantine remains; no new disabled tests. Root stack walking that reads no homes needs no scratch scan. |
| Size accounting / direct heap construction / `pystate` entry sentinels | Size-only readers may keep geometry under an independent pin; constructors must establish safe local state before publication. NULL-code interpreter sentinels and half-built cleared generators need explicit nonordinary paths. |

## Consolidation order and heap-clear proof

First use existing `_PyFrame_ClearLocals` in `frame_tp_clear`, whose duplicate
home/mapping loop has identical callback order. Keep observer-field closes in
their current position. This gives a real consumer to an existing release API
without changing thread/gen code. The helper currently performs an additional
debug whole-frame check; its future map port must validate selected initialized
homes rather than stale scratch.

Next consolidate the GIL/FT `_PyGC_VisitFrameStack` selection behind that existing
entry while preserving backend `_PyGC_VisitValue`. `_PyFrame_Traverse`, generator
and heap-frame traversal, and precise active tracing already consume it. FT
deferred and optional-marking loops need tag-preserving selection as well; an
object-only callback is insufficient for deferred adjustment. Land any shared
home iterator only with these actual consumers, not as an unused parallel API.
Release, copy and diagnostic actions can share selected ranges later without
pretending their callback/order/ownership policies are identical.

For `frame_dealloc`, obtain every required geometry/map fact while code is
owned, keep code fixed-last through home reads and callbacks, and specify the
root/edge handoff before moving code close. A frame object held by a Python
locals proxy or callback is not refcount-zero; deallocation's stronger absence
of ordinary Python access differs from `tp_clear` on an escaped live frame.
That distinction needs tests, not an assumed thread-owner invariant.

In particular, `framelocalsproxy_setitem` may install a value into a named local
already passed by `tp_clear`'s descending cursor. Reusing the ordinary thread
remaining-prefix record would miss that new suffix binding. Preserve the
selected one-pass observable behavior, keep dynamic named-binding authority
over all local homes during callbacks, and leave newly published surviving
bindings owned by the heap frame on return. Under current NULL control a full
initialized-range record can cover this case; future map progress must separate
mutable bindings from the scratch release cursor. Do not repeatedly drain
callback writes without an explicit behavior decision.

Focused bounded consolidation tests should retain frame/code/function access
during local finalizers, mutate both a not-yet-cleared and an already-cleared
local via an escaped proxy, preserve nested clear/collection behavior, check
new owner release on later clear/free, and check weakref/resurrection outcomes.
No change may double-subtract a moved heap edge or let a record outlive its
frame storage. Frames and IR peer this table and the concrete first patch;
root owns merged validation and performance evidence.

## M3 to M4 port queue at 9136c202e8e

P0 means required before any stale-scratch switch. P1 is an independently
reviewable preparation port once the real descriptor/selector is available.
P2 means geometry/schema work only if that representation changes. Readable
means a canonical value, nullable value or tagged control is initialized;
owner means a logical owning edge, not merely nonNULL bits. Runtime tags still
decide counted/deferred edges. Local boundness remains dynamic under the
shared-placement/NULL-local control; choosing local validity adds the named
reader ports below rather than eliminating them.

| Priority / state | Exact reader or writer | Authority and concrete port |
| --- | --- | --- |
| P0, escape/creation | `pycore_interpframe.h:_PyFrame_Copy`; `frame.c:take_ownership`; `bytecodes.c:RETURN_GENERATOR`; `genobject.c:gen_new_with_qualname` | Copy only readable selected homes, transfer each owner once, and establish destination local state and lifecycle descriptor before tracking/publication. Source alias authority ends once; source code pin lasts through descriptor reads. Destination inactive words need no decoding. The current unconditional N-home move is unsafe with stale scratch. |
| P0, failing active frame | `bytecodes.c:error`, `exception_unwind`, `exit_unwind_notrace`; `pycore_interpframe.h:_PyFrame_ClearTemporaries` | Select actual failing/partially consumed phase before traceback/monitor callbacks. Descending cleanup selects remaining owners, not all nonNULL reserved words. Handler entry installs readable tagged lasti and exception at declared homes, then publishes its new shape. Keep CURRENT_OFFSET authority distinct from a RERAISE-modified `instr_ptr`. |
| P0, unlinked retirement | `frame.c:_PyFrame_TryClearExceptCode`, `_PyFrame_InitClearRoots`, `_PyFrame_FinishClearExceptCode`, `_PyFrame_ClearGenerator`; `ceval.c:clear_thread_frame_slow` | Replace all-N heap/gen and scratch-NULL assumptions with actual remaining owner ranges/cursor. Preserve dynamic named bindings through takeover, result and generator exception roots, fixed-last code and reservation lifetime. Existing prefix records are initialized control, not already a static map. |
| P0, generator resume/result | `genobject.c:gen_send_value_ex`, `gen_close`, `_gen_throw`; `pycore_interpframe.h:_PyFrame_SetReturnValue`, `_PyFrame_PublishReturnSlot`; `ceval.c:_PyEval_CompleteGeneratorReturn` | Distinguish created, suspended, delegated, throw/error and completion states. An inactive scratch recipient has no old owner to load; synthesize that fact from the recipe. A named recipient still reloads its actual current binding after callbacks. Preserve exception-chain attachment and temporary current-frame links during delegated throw. |
| P0, suspended receiver | `genobject.c:_PyGen_YieldFromRef`, `gen_getyieldfrom`; `instrumentation.c:_PyIR_ObserveStep`; `remote_debugging/asyncio.c:get_awaited_coro_address` | Certify the retained SEND receiver role before reading `return_slot-2` or `IR_B(site)`. Existing state/bounds/address checks remain necessary. A reserved in-bounds slot is not initialized authority. Remote value sampling additionally needs its stopped-state/read-lock protocol; location-only inspection does not. |
| P0, heap/active collection | `gc.c:_PyGC_VisitFrameStack`, `gc_free_threading.c:_PyGC_VisitFrameStack`; callers `_PyFrame_Traverse`, `gen_traverse`, `frame_traverse`, `_PyGC_TraceVisitFrames` | Use the existing selector entry for dynamic L plus certified scratch. Keep associated frame objects from duplicating active/generator home edges. Preserve subtraction versus marking tag policy. The committed phase hook is a reached comparison only, not this map-only selector. |
| P0, FT active collection | `gc_free_threading.c:gc_visit_thread_stacks`, `gc_visit_thread_stacks_mark_alive` | Port both full-N loops to the same selected geometry while retaining each PyValue tag. Deferred missing-count adjustment and external mark remain different actions. Require actual world quiescence for phase reads. |
| P0, staged construction | `ceval.c:stage_binding_frame`, `eval_frame_push_and_init_code`, provider scopes, constructor shim scope; `_PyEval_UnpackIterableValue` | Binding currently initializes the full child extent to NULL. If that fill disappears, advertise only actual initialized binding/scratch prefixes and typed headers. Unpack owns its installed suffix through iterator/sequence closes; publish active POST before disarm. Parent/child aliases and clobber capacity must partition exactly once. |
| P0, native fixed adapter | `native_scope.c:_PyNativeResult_Publish/_Abort/_PyNativeScope_End`; `ceval.c:native_call_publish_result`, `native_call_finish`, `native_call_builtin_o` | PRE retains source window arguments; success detaches inputs, installs window result and then closes native snapshots. Encode those actual publications and failure shapes, including dynamic local destination. Fixed NativeScope workspace is native C ownership, not a new arena bank; no blanket C-root registration follows. |
| P0, debugger transfer | `frameobject.c:register_resume_liveness`, `register_saved_locals_plan`, `register_resume_plan`, `frame_lineno_set_impl`; `frame.c:_PyFrame_CompleteRedirect` | Remove the positive assumption "entry initializes scratch to NULL" and physical-null authorization of unproduced homes. Use matched readable owner/NULL protocol roles plus existing origin/token proof; never relax jump admission. Retired-array prefix persists through close. Mutable locals stay dynamic; accepted redirect publishes target before callbacks. Constructor staged barrier remains explicit. |
| P0, hot protocol consumers | `ceval.c:call_record_refresh`, `call_transaction_detach`, `call_bind_known_python`, exact argument movers; generated CALL/SEND/FOR families | Elided PUSH_NULL cannot make `window[1]` readable. Certified no-self form supplies virtual NULL without a load; method form reads an initialized receiver. Replace inactive scratch destination tests and clobber assertions through generated roles, not a hot generic decoder. |
| P1 before switch, diagnostics | `frame.c:_PyValue_CheckFrame`; `Python/ceval.h:dump_stack`/lltrace; `object.c:PyUnstable_Object_IsUniqueReferencedTemporary`; temporary/overlap/PopFrame assertions | Select readable homes before tag/type decoding. Uniqueness searches actual temporary owners, not stale duplicates. Display inactive positions without dereferencing them. Replace physical all-NULL assertions with no-unretired-owner and clobber proofs; no new disabled checks. |
| P1/P2, named observers | Locals proxy get/set/iteration; `frame_init_get_vars`, `frame_get_var`, `PyFrame_GetVar*`; `typeobject.c:super_init_without_args`; `Tools/gdb/libpython.py:PyFramePtr.iter_locals` | With NULL locals, retain current dynamic L policy, cell-home/content distinction, `pending_kwargs` materialization and overwritten-borrow owners. A validity alternative must port every reader before changing L initialization. Existing GDB quarantine stays documented; no new quarantine. |
| P2, no home read | `remote_debugging/frames.c` stack/location walk; code/frame/gen size accounting and debug offsets | Keep typed pins, NULL sentinels, range/torn-sample checks and malformed-location degradation. Update offsets only for actual schema changes. These readers need no scratch decoder merely because frame geometry exists. |
| Final P0 gate, conservative arena discovery | `gc_trace.c:scan_thread_struct`; native C/PyMem roots remain separately selected | Remove current value/header and older-chunk scans only after all active/staged/retiring/suspended/escaped states above have producers and readers. Keep native roots. Arena address reachability is not complete enumeration and cannot cover an unlinked initialized prefix by itself. |

The first immediate unsafe old assumption is `_PyFrame_ClearTemporaries`'s
`if (!PyValue_IsNull(ref)) PyValue_CLOSE(ref)` over a reserved suffix: an inactive
stale pointer would be closed as an owner during unwind. `_PyFrame_Copy` can
instead propagate that stale word into a heap container before collection or
deallocation. Elided-call self and `_PyFrame_PublishReturnSlot` are additional
ordinary-path reads that a collector-only port cannot repair. Do not test these
by executing deliberately malformed or stale images before the atomic switch;
use bounded cold shadow probes and admitted source fixtures.

Independent next packets after heap deallocation and first emitter comparison:

1. Frames: copy/escape/RETURN_GENERATOR and public frame-to-gen construction,
   with actual generator/heap copy fixtures and selected-home geometry.
2. Frames/root: unwind suffix cleanup plus handler entry and retirement cursor;
   agree generated error-phase boundary with IR before editing bytecodes.
3. GC: existing GIL/FT frame selector plus the two FT active loops, preserving
   tags and existing policies. Land shared selection with these real consumers.
4. Frames + GC observer lane: suspended receiver/resume destinations, then cold
   debugger transfer/diagnostics/uniqueness; remote value reader follows the same
   suspended role while location-only paths remain independent.
5. IR: complete emitted phases and certified no-self CALL form across compiler,
   admission and generated helpers; active adapters consume the same capabilities.

These packets can be source-reviewed separately while NULL control remains.
Root integrates overlapping headers/bytecodes and owns the final all-phase,
all-reader switch. The queue does not select placement or claim emitter coverage
that has not landed.

Frames and IR source peers report GO for this table and the first
`frame_tp_clear` consolidation, with the obsolete clear-helper name corrected.
Both retain deallocation as a separate fixed-last/mutation/resurrection gate.
The bounded first source port is implemented and frames postreview GO:
`frame_tp_clear` now calls existing `_PyFrame_ClearLocals`; its observer-field
close order and retained pins are unchanged. `test_frame.py` checks one-pass
local mutation on both sides of the cursor, nested clear and collection, later
release of the surviving binding, and code access during callbacks. Host AST
and diff checks pass; root owns execution. No `frame_dealloc` change is included.

## Separate private heap-deallocation component

The actual two-file candidate and its source GO are recorded in
[heap-dealloc-proposal.md](heap-dealloc-proposal.md) and
[heap-dealloc-peer.md](heap-dealloc-peer.md). It uses the existing full home/header
record before weakref callbacks, function/mapping-before-home order, five moved
raw native metadata owners in their prior order, and fixed-last code. It does
not repurpose thread/gen reservation logic or alter live `tp_clear` mutation.
The fixture checks precise lower-home/code roots, weakref order, nested records
and native metadata lifetime. Root owns integration and native validation;
mapped selection and phase reset remain explicit later ports.
