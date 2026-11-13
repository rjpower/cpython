# Lifecycle reader census and first consolidation

2026-11-12 source census over integrated `e233c2f05d8`, merged into GC lane as
`97850931449`; the four-file CValue cleanup changes no frame-reading policy.
Later unpack/binder adapters are identified separately below. NULL remains
authority. This is the port list for a full map switch, not permission to remove
the arena scanner or treat the first empty-return certificate as complete.

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
| `frameobject.c:frame_tp_clear` | Clears observer metadata, then duplicates reverse all-`N` home loop and mapping close. Code/function remain owned. | First concrete consolidation: invoke existing `_PyFrame_ClearLocals` after metadata closes. No new wrapper or representation change is needed for this exact portion. |
| `frameobject.c:frame_dealloc` | Untracks/clears weakrefs, then currently clears code/function/mapping **before** reverse home close, followed by observer-owned fields. | Code must survive mapped home interpretation. A blind call into thread/gen retirement is wrong. Freeze heap clear/dealloc callback, mutation and resurrection authority before a shared release implementation. |
| `pycore_interpframe.h:_PyFrame_ClearTemporaries` | Reverse scratch suffix from supplied `first`; each nonNULL home NULLs before close. | Error/source-error/redirect phase selects actual owner obligations; preserve release order and dynamic remaining cursor across callbacks. |
| `bytecodes.c:exception_unwind` | Clears suffix, installs lasti/exception, changes handler PC, then monitor callback. | Exact pre-error/partial-consume and handler-entry maps; lasti is protocol data, not a heap object. RERAISE's stored PC is explicitly not the unwind authority. |
| `frame.c:_PyFrame_CompleteRedirect`, bytecodes InitCleanup | Completes parent redirect after child relinquishes overlap; explicit trampoline cleanup. | A separate explicit executable/phase contract; do not use an ordinary code certificate for static entry or InitCleanup words. |
| `gc_trace.c:scan_thread_struct` | Current value/header regions and whole older chunks conservatively scanned, covering unlinked reservations. | Remove only after active/staged/retiring/heap lifecycle census closes. Preserve independently selected native C/PyMem conservative policy. An address pointing into an arena is not complete arena enumeration. |
| `gc_trace.c:_PyGC_TraceVisitTransitionRoots` | Initialized remaining prefix, typed header mask, generator exception, detached result and optional initialized suffix/range. | Same release recipe and cursor as producer. No code/geometry read after code bit disarms; prefix-only records never infer active-frame geometry. |
| M3 `frame_lineno_set_impl` retirement helper; generic unpack adapter | Detached PyMem retired array / initialized arena output suffix remains live through actual closes. | Preserve prefix authority through nested redirect/collection and iterator/surplus/sequence cleanup. Publish verified active POST before disarm under maps. |
| Proposed binder outer stage | Initialized not-current child homes and typed headers across binding and outer EX cleanup. | Never infer from parent PRE. Root through actual child publication or gapless M2 failure retirement. Native-provider/PEP523 post-return entry remains a separate open gate. |

## Observers, named bindings and diagnostics

| Concrete reader | Required authority |
| --- | --- |
| `frameobject.c:framelocalsproxy_getval/hasval/getkeyindex`, key/value/item/length iteration and `framelocalsproxy_setitem` | Dynamic local boundness before value reads, cell-home versus cell-content boundness, readonly constant homes and replacement publication before old-owner close. Preserve `f_overwritten_fast_locals` ownership for outstanding borrows. |
| `frameobject.c:frame_get_var`, closure population, `PyFrame_GetVar*`, legacy fast-to-locals interfaces | Same named binding policy, first COPY_FREE_VARS/MAKE_CELL state, virtual `pending_kwargs` materialization and possible boxing failure. No instruction-site scratch map substitutes for mutable local validity. |
| `typeobject.c:super_init_without_args` | Local0 and `__class__` cell availability, including deleted first arg, incomplete-cell and empty-cell distinctions. This direct local reader sits outside frameobject.c. |
| `frameobject.c:register_resume_liveness` physical `null_home` loop, `register_saved_locals_plan`, `register_resume_plan` required/protocol-home reads | Existing observer transfer/origin proof plus actual selected readable state. Stale zero/equal bits cannot restore a missing saved-binding token or NULL protocol. Shared semantic plan must replace duplicate scratch authority, without broadening jump admission. |
| `frameobject.c:frame_lineno_set_impl` restore and scratch-retirement loops | Detach only actual owner homes, publish restored bindings/continuation before close, preserve active-child overlap and nested local mutation. Already outlined retired-array prefix remains live. |
| `genobject.c:_PyGen_yf`, `instrumentation.c` SEND receiver reader and remote asyncio yield-from reader | Certified suspended/selected SEND receiver at `return_slot-2`, not an arbitrary in-bounds home. Retain owner/state/range/address validation and FT read-lock protocol. |
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

Frames and IR source peers report GO for this table and the first
`frame_tp_clear` consolidation, with the obsolete clear-helper name corrected.
Both retain deallocation as a separate fixed-last/mutation/resurrection gate.
The bounded first source port is implemented and frames postreview GO:
`frame_tp_clear` now calls existing `_PyFrame_ClearLocals`; its observer-field
close order and retained pins are unchanged. `test_frame.py` checks one-pass
local mutation on both sides of the cursor, nested clear and collection, later
release of the surviving binding, and code access during callbacks. Host AST
and diff checks pass; root owns execution. No `frame_dealloc` change is included.

## Proposed separate deallocation increment

The smallest deallocation port retains its existing function/mapping-before-home
close order and observer-field order, but moves code close after the home sweep:
untrack/weakrefs; clear function and mapping; call `_PyFrame_ClearLocals` while
code remains owned; clear code; clear back/trace/extra/cache/overwritten fields;
free. The helper's second mapping clear is NULL in this path. This shares the
actual home sweep without repurposing thread/generator ownership or changing
live `tp_clear` one-pass mutation behavior. No source edit is authorized by this
paragraph alone; peer the code-pin and access proof before implementation.

The required deallocation access proof is narrower than a general claim that
finalizers cannot mutate frames. A Python callback or locals proxy holding this
frame would contribute a strong reference and prevent refcount-zero entry.
The frame is untracked and its weakrefs cleared before owner closes, it is not
the active thread/generator frame, and takeover gives heap storage its own
owner. A cycle that resurrects an escaped frame during `tp_clear` instead uses
the live-frame path already tested. A native engine cannot resurrect a
refcount-zero object through an invalid borrowed pointer as a supported way to
write new locals. Confirm these conditions on actual source and focused tests;
do not apply this argument to live `tp_clear`.

A useful fixed-last regression fixture creates a function from a distinct
`code.replace()` image, captures a heap-owned returned frame containing a
finalizer, drops the external function and then the frame, and observes weak
code/function references inside that local finalizer. Code must remain alive
while the function is already released; both disappear after deallocation.
The finalizer holds weak references only, so the fixture does not accidentally
prevent the route being tested. Add weakref/cycle resurrection checks separately
if source review finds a supported access path beyond this proof. Native
conservative heap policy remains explicit; this does not supply a new arena
root or justify scanning stale heap scratch under maps.
