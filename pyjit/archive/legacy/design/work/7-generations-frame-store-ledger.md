# Generations: frame/generator owner and store ledger

Source-only audit, 2026-10-29. No barrier implementation, target execution,
test, build or probe. Clean a285 and the existing four-row timing package
remain unchanged. Baselines are f2a5b3ca33d1 and H3 union e9a1bcc65cf.
Objects/genobject.c, Python/frame.c, pycore_interpframe.h and its structs,
Objects/frameobject.c and the inspected ceval helpers are byte-identical
between those commits. Bytecodes differs in H3 monitoring/property metadata;
the transition bodies below are unchanged. Line anchors below use f2/a285;
e9's first instrumentation addition shifts later bytecode lines by one.
This closes the named source slice's *inventory*, not generational safety.
The collector still has no implemented age/remembered-owner interface.

Authoritative contracts: note11 §11 (whole running/data-stack frame roots,
one owner handoff per suspension/return/escape), generations design §Store
contract and review0c4a74eb (no unsafe SHALLOW promotion, metadata commits
before callback reentry), and the current roadmap's H2 physical-slot maps.
No deviation from those contracts is proposed.

## Actual owner and root boundary

Interpreter-frame structs define f_executable/f_funcobj as strong/deferred
PyValues, f_locals/frame_obj as strong, and f_globals/f_builtins as borrowed.
base/saved_top/previous/instr_ptr/ir_words/ir_feedback are storage/control
pointers, not independent owning Python edges. owned_slots belongs to its
embedding generator/coroutine/asyncgen or PyFrameObject. A thread frame's
base instead points into the thread value stack. Integer state/offsets and
visited/external bits contain no object edge.

Python/gc_trace.c:1377 visit_frame additionally visits the borrowed globals
and builtins as explicit roots. TraceVisitFrames:1418 enumerates every
initialized data-stack frame, linked or not, then non-thread frames actually
on current_frame/previous. Interpreter-owned entry frames expose only their
return slot. Python/gc_live.c:2033 VisitFrameStack currently visits every
co_nslots slot; NULL/dead-temporary clearing is the present validity proof.
ThreadState:1470 separately walks every exc_info/previous_item.exc_value.
A root's direct slot value must be visited even when its heap object is old;
merely marking an old generator referenced by a C span is not equivalent to
fully visiting the generator's slots during a minor.

The concrete *current full-tracing baseline* is stronger: gen_traverse:65
calls Frame_Traverse for every non-CLEARED state, including EXECUTING. The
gen_send_ex2 precise span roots gen before exception chaining/eval entry;
_gen_throw likewise spans gen and its delegated receiver, with temporary
frame-chain links around delegated calls. Current full marking follows those
objects and fields. The pre-link/detach obligations below arise only when a
minor stops recursively tracing old rooted objects. They are not evidence
of an established current missing-root bug.

Consequently FRAME_EXECUTING is **not** a root-membership test: gen_send_ex,
close and throw set that state before ceval links the frame. Conversely a
thread data-stack frame is rooted before chain linkage. A running generator
also remains a heap object for its separate names/origin/finalizer fields;
rooting its frame does not exempt mutations of those fields.

| Publisher / source anchor | Logical owner and store category | Required generation interface/proof |
| --- | --- | --- |
| pycore_interpframe.h:208 SetReturnValue; ceval_macros.h:632 IR_STORE_RETURN/637 IR_RETURN_EARLY | Writes destination frame slot. Running caller/entry return slots are direct roots. A send into a not-yet-linked suspended generator writes that generator's heap storage. | Root-only exemption needs actual frame membership at this point. Pre-link sends need the boundary proof described below, not a universal exemption in SetReturnValue. |
| Header Initialize:268, PushUnchecked:477, PushOverlapping:493, PushTrampoline:522; ceval.c:1966 EvalFramePushAndInit | Function/code/locals/header and initialized arguments go to thread data-stack roots. Overlapping argument windows remain root storage; base does not own the backing stack. | Preserve executable-NULL incomplete-frame convention and initialize slots before first possible allocation. Full root enumerator must scan pushed unlinked frames. Argument setup/exception exits in ceval are a dependent ingress audit, not a heap barrier on each arg copy. |
| bytecodes.c:298 _SWAP_FAST, STORE_FAST fused forms; generated output/result stores; COPY_FREE_VARS:2196 and MAKE_CELL:2121 slot publication | Locals/temporaries of an actually running frame are direct-root writes, even for a resumed old generator. | No per-slot heap barrier while rooted. MAKE_CELL/STORE_DEREF:2190 and frame-proxy PyCell_SET mutate a separate heap cell: that cell still needs the ordinary cell barrier, regardless of running-frame status. |
| interpframe.h:233 Frame_Copy (all header edges and all slots, clearing source slots) | Bulk transfer into destination generator or escaped PyFrameObject. Destination owns new edges; source clears are deletions. | Remember the actual destination if it can already be old; a copy's fresh-looking destination is not proof. It is a nonallocating copy body, so one owner remember can cover publication; constructor/age admission and transfer ordering must be proved at each caller. |
| frame.c:23 MakeAndSetFrameObject | Stores new f in frame->frame_obj; f->f_frame is an alias until ownership transfers. | If the frame is suspended, remember its generator before frame_obj store. Running/data-stack frame_obj is a direct frame root until its later owner handoff. The newly allocated wrapper is a different object, not the owner of a still-generator-owned slot array. |
| frame.c:47 take_ownership, 54 Copy, 55 f_frame switch, 69 allocating TAKE_OWNERSHIP injection, 80 f_back | f receives copied header/slots, then f->f_back receives another frame object. f can be old: it may have escaped long before this transfer. | Remember f at the copy/publication boundary before the injection/back-frame creation; separately cover the later f_back store after that allocation. The first remember cannot be assumed to remain dirty if an intervening minor consumes it. Do not add a redundant raw-frame root instead of naming the heap owner. |

Borrowed globals/builtins and code-owned raw IR/feedback pointers are not
new strong heap fields just because the root scanner visits them. Suspended
frames' function/code traversal and their objects' own strong-field barriers
must preserve the actual owning paths. Those function/code owners belong to
their own ledger; do not manufacture a frame ownership edge to avoid it.

## Every running-to-heap transition

1. **Creation/RETURN_GENERATOR**, bytecodes.c:5395. _Py_MakeCoro allocates the
   destination, then Frame_Copy moves caller header/slots into owned_slots,
   sets CREATED/GENERATOR owner, publishes the generator with IR_RETURN_EARLY,
   and pops the source frame. A coroutine can already have survived collection
   during origin construction before this copy (below). Remember the
   destination before publication/removal of source roots unless a precise
   not-promoted construction region is enforced. The existing early result
   store must remain before popping the source.
2. **Optimized resume**, SEND_GEN_FRAME:1586 and FOR_ITER_GEN_FRAME:3758.
   They write the sent value/None, link exc_info and previous, and _PUSH_FRAME
   makes the target current. These bodies contain no Python call/allocation
   between slot publication and push; prove that whole generated transition,
   including instrumentation/prediction/error variants. None is not a young
   edge, but SEND's arbitrary value can be. No existing generic helper encodes
   this root-membership guarantee.
3. **Generic resume**, genobject.c:267 gen_send_ex2. It writes the send slot
   before ceval's current-frame link (ceval.c:1299). It links exception state,
   anchors gen in a span and may call _PyErr_ChainStackItem (errors.c:748,
   which calls _PyErr_SetObject), then enters ceval. Ceval has a recursion-
   error clear path before its normal link. Do not infer a noncollecting
   interval from FRAME_EXECUTING or from a span containing only old gen.
   Resolve by a proved complete noncollecting path, explicit pre-link live
   slot/frame root, or ordinary owner remember at publication. This is an
   unresolved minor-GC interface, not a demonstrated current full-GC bug.
4. **Yield**, bytecodes.c:1673 _YIELD_VALUE (instrumented form funnels here).
   IR_RETURN_EARLY roots the yielded result in the caller. exc_info is then
   restored and current_frame leaves the generator; previous becomes NULL,
   state becomes SUSPENDED/SUSPENDED_YIELD_FROM. Remember generator *before
   the first root-removal boundary*, covering both retained slots and saved
   gi_exc_state.exc_value. This is one owner operation, not one per slot.
   return_slot/state/instr_ptr updates alone need no barrier.
5. **Return/error**, _RETURN_VALUE:1419 and ceval.c:1902 clear_gen_frame /
   1949 FrameClearAndPop. Return roots its result in the caller, unlinks the
   dying frame, then performs RETURN injection before clear_gen_frame roots
   gen and clears it. An owner handoff must precede that unlink/injection,
   even when most fields will shortly be deleted. A span holding old gen
   after unlink cannot substitute for young-edge closure. Escaped frame_obj
   may take ownership during clearing. Exception unwind/early_exit also reach
   FrameClearAndPop: the predecessor that removes the frame root must obey
   the same handoff, not just the ordinary RETURN handler.
6. **Temporary throw linkage without YIELD**, genobject.c:734 _gen_throw,
   especially 795–829. For recursive yield-from throw or a foreign iterator's
   throw method, it temporarily links the outer frame to current_frame,
   invokes code, then unlinks and may restore the prior suspended state.
   During that call frame introspection can write locals; minors can consume
   an earlier remembered entry. Re-remember before each removal even if no
   YIELD_VALUE executes. Error paths restoring frame_state and method lookup
   do not themselves publish a new strong edge, but the root-membership
   boundary and any external writes still require their own proofs.
7. **close/clear/finalize**, gen_close:541, gen_clear_frame:152,
   _PyGen_ClearFrame:171 and gen_finalize:94. Close may call a delegated
   iterator, bypass resume with the no-unwind close fast path, resume and
   yield illegally, or return through error. gen_clear_frame uses an explicit
   gen span and may transfer to an escaped frame via ClearExceptCode. NULL
   stores/clearing code/name/exc fields are deletions. Any transfer or detach
   must still leave the surviving frame's fields remembered; finalization/
   resurrection does not grant a fresh-young exemption. Finalizer queue and
   domain ownership contracts remain unchanged.

## Other generator-family strong publishers

| Anchor (Objects/genobject.c) | Published edges / logical owner | Construction and post-promotion implications |
| --- | --- | --- |
| make_gen:1278; gen_new_with_qualname:1348 | gi_name/qualname; iframe code/function/locals/frame_obj/slots via copy; exc_value initialized NULL. | NewVar allocation can collect before returning memory. The immediate field-write/track region has no Python allocation, but the complete coroutine constructor extends beyond it. gen_new copies into gen and redirects f->f_frame; source copy clears are not barriers. |
| _Py_MakeCoro:1301 and PyCoro_New:1783; ComputeOrigin:1753 | cr_origin_or_finalizer; origin tuple and per-frame tuples. | Both allocate origin data *after* the coroutine exists/tracks, then assign cr_origin. ComputeOrigin's PyTuple_New and repeated Py_BuildValue are concrete collection points. The tuple's partial-construction stores belong to the tuple ledger too. No generic "new coroutine" barrier exemption. |
| gen_set_name:1014, gen_set_qualname:1045 | Replace gi_name/qualname (also used for coroutines/asyncgens). | Ordinary heap-owner stores on self, including when its frame runs. Type-checking/string ownership does not make a young string old. |
| async_gen_init_hooks:1902 | ag_origin_or_finalizer receives thread's finalizer hook. | Old asyncgen can install a young hook on first use; remember ag before store and before firstiter callback (which can collect). ag_hooks_inited/running_async/closed flags are not object edges. |
| coro_await:1477; async_gen_asend_new:2534; ValueWrapperNew:2616; async_gen_athrow_new:3038 | Wrapper->cw_coroutine; ASend->ags_gen/sendval; wrapped value->agw_val; AThrow->agt_gen/typ/val/tb. | Each has an allocation followed by a short field-initialize/track region, no observed later non-NULL writer in this file. This is a candidate for an exact noncollecting constructor proof, not for a global "immutable/fresh" exemption. Owning wrappers and every field must be safely traceable before they can promote. Their state transitions and destructor clears do not add edges. |
| bytecodes.c PUSH_EXC_INFO:3859 / POP_EXCEPT:1738; errors.c:623 SetHandledException | tstate->exc_info->exc_value can physically be gen->gi_exc_state.exc_value. | While linked, direct exception roots cover new value; after detachment gen owns retained value. Do not treat generic SetHandledException as always thread-only. Suspension remember must cover this field as well as slots; standalone suspended mutation needs an owner-aware interface. previous_item is a borrowed chain pointer, not an owning PyObject edge. |

Traversals ground the owner list: gen_traverse:65 visits names, live iframe
or retained code of a CLEARED generator, coroutine origin and exc_value;
async_gen_traverse:1884 additionally visits ag_origin_or_finalizer. Wrapper,
ASend, wrapped-value and AThrow traversals visit their listed owning fields.
Cleared generators still own code; do not use a finalizer-noop shortcut to
remove that edge. No callbacks or exception-object subfields were re-audited
here: exception traceback/context/cause and stop-value stores belong to the
exception ledger, even when generator helpers invoke their setters.

## Escaped-frame and external-local publishers (required adjacent surface)

Objects/frameobject.c:279 framelocalsproxy_setitem writes fast[i] when no
cell is present; cell writes go through PyCell_SET. Resolve the slot's owner
from the actual interpreter frame: thread/current-chain roots, suspended
GENERATOR owner via existing GetGeneratorFromFrame, or owning PyFrameObject
when its frame is in _f_frame_data. A proxy's PyFrameObject wrapper can point
at generator-owned storage and therefore is not necessarily the slot owner.
There is no existing general owner-resolution/remember interface for this
three-way distinction; do not pretend Frame_GetFrameObject supplies it
(frame.c asserts against recreating an owning frame object's wrapper).

The same function may allocate add_overwritten_fast_local's tuple first
(:250–274) and stores it on f_overwritten_fast_locals to preserve borrowed
reads; that is a distinct f-owned edge. It then publishes a new slot value.
Recheck actual owner/age after allocating preparation, not a cached freshness
assumption. Extra-locals dict installation (:336–341) needs a barrier on f,
while later dict key/value writes need barriers on that dict. Frame locals
lazy creation (:1117 and :2453) writes frame->f_locals after PyDict_New;
frame_init_get_vars:2334 publishes closure cells into a potentially suspended
frame. frame_settrace:2034 writes f_trace on f. take_ownership writes f_back.
These are ordinary heap writes even if the API caller is an opaque C frame.

f_locals_cache currently has NULL initialization and clearing only in this
file; do not invent a non-NULL publisher. The full frame traversal:2116 also
visits f_back, f_trace, f_extra_locals, f_locals_cache and overwritten locals,
then iframe fields only for FRAME_OWNED_BY_FRAME_OBJECT. Local proxy creation
(:468–497) owns a separate proxy->frame constructor edge. PyFrame_New:2289
creates a function/code exec before frame allocation, initializes embedded
storage nonallocating and publishes its frame owner; that exact region is
another candidate constructor proof. f_lineno handling's writes of None
and slot clearing (:1962/:1978) are immediate/no-edge stores or deletions;
its tracing/exception/cell/container effects retain their separate owners.

## Constructor and H2 closure gates

Allocation is real: gc_live.c:2360 gc_alloc -> MallocWithType ->
PyObject_Malloc; obmalloc.c:1227 counts/collects **before** object allocation.
new_reference:2694 sets the object map and calls CREATE reftracer. Incomplete
object bodies must stay protected by the existing construction/SHALLOW
contract; GC_TRACK alone does not prove every traversed field initialized.
Specifically make_gen tracks before origin_or_finalizer is assigned in the
coroutine paths, which allocate ComputeOrigin in between. Promotion therefore
needs an explicit complete-object proof, not the tracked bit or a SHALLOW
bit which some other path happened to clear. No claim is made that this
existing full-collector construction convention is broken.

Likewise frame.c's historic MakeAndSetFrameObject comment says allocation
will not collect. It is not a valid proof against the current object-allocation
trigger. Existing owner can promote while the new wrapper is allocated;
publication afterward needs the actual owner rule. The newly returned wrapper
itself has no intervening allocation in its small initializer, subject to the
existing reftracer/no-Python/no-object-allocation contract. The CREATE hook has an actual existing noncollecting guard:
pycore_object.h:68 raises in_tracer and gc_live.can_collect excludes it;
its documented contract also forbids changing existing edges. The listed
small wrapper initializers then contain only field stores/NewRef/GC_TRACK,
which is a concrete noncollecting region from allocation return through
initialization. This can support a narrow constructor exemption once new
allocation is guaranteed young and safe publication is specified; it is not
an exemption for the allocating coroutine-origin region or later owner
stores. Remaining allocator/lock/publication assumptions must be bound in
the eventual implementation proof, not inferred from the tracked bit.

H2 is planned, not implemented. Today all slots are initialized and dead
ones cleared; roots and suspended traversal both visit all slots. H2 must
supply trusted physical-slot maps for final IR positions including synthetic
jumps, exceptions and suspend/resume states, plus initialized-prefix and
call-window overlap rules. The map at YIELD (instr_ptr incremented before
detach), return, throwflag entry and a CREATED frame must agree with each
root/handoff. Frame_Copy cannot blindly turn stale un-cleared dead words
into live heap edges after clearing-store removal. All roots, suspended
traversal, escaped-frame/local introspection and copying need the same valid
map/initialization interpretation, including retained Python-visible locals.
H3 metadata/property changes do not provide these maps. No H2 dependency is
invented for current full-slot generations; the existing representation can
be the first implementation if its store/promotion/root contracts close.

Unclosed requirements for this slice are therefore precise:

1. A nonallocating owner-remember operation and actual owner resolution for
   suspended/escaped storage, with age checked at publication, not before an
   allocating preparation step.
2. A complete handoff list covering yield, return/error/early exit, temporary
   throw linkage, constructor copy and take_ownership, placed before the
   earliest corresponding root removal/possible collection.
3. A proven pre-link generic-send region or explicit root/remember strategy;
   FRAME_EXECUTING and a root of old gen are insufficient evidence.
4. Per-constructor initialization/publication and safe-promotion evidence,
   especially coroutine origin allocation. Preserve SHALLOW keep-young until
   that evidence exists; commit age/remembered changes before callback reentry
   so a later dirty write is not erased.
5. Ordinary barriers for generator names/hooks, escaped-frame fields/proxy
   fast locals and cell/container/exception owner dependencies; opaque roots
   are not a substitute for old heap mutation coverage.
6. Independently reviewed full-young reachability oracle/omission controls for
   resumed-old-frame writes before next yield, temporary throw detach, escaped
   old frame takeover, pre-link send and post-construction/promotion stores.
   These are future semantic controls, not launches authorized by this audit.

Inventory completeness is limited to the listed files and adjacent root,
proxy, allocator and error-entry interfaces inspected here. JIT/external frame
APIs, remote debugger mutation, argument binding variants, function/cell/
exception/container internals, port writers and free-threaded concurrency
still require their own ledgers. No Queue2/S3 work or blanket family-wide
"covered" status is implied.
