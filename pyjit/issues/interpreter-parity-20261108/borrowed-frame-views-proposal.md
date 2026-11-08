# Borrowed call arguments in thread frames: design packet for review

2026-11-08. **Proposal for the next packet, after the current interpreter
batch's qualification and performance checkpoint. No implementation is
selected or authorized by this page.** This extends the
[pinned-source comparison](borrow-frame-followup.md). It retains canonical
eight-byte `PyValue` values in heap containers, fields, cells and value APIs,
and retains explicit ownership. It asks whether a narrow interpreter-only
frame view can avoid one materialized argument snapshot in an eligible Python
call. The retained warmed-shape probe confirms that the actual `MESSAGE`
string and logger are mortal in both interpreters. Independent review of the
selected implementation is required before it begins.

The [first peer review](borrowed-frame-views-peer-review.md) requested a
concrete owner graph and route closure. The [second review](borrowed-frame-views-peer-review-round2.md)
approves **diagnostics and a representation decision only**. Implementation
remains blocked on generated legal-decline edges, a complete source/generated
consumer census, and a chosen variant's exact promotion word operation. The
sections below state required contracts; they do not claim those proofs have
been executed.

## Problem and measured boundary

The candidate's retained warm `LOAD_FAST_BORROW` for `bench_silent` calls
`PyValue_DUP(GETLOCAL)` and places an owning-form value in a temporary call
window. For a mortal object that operation takes a count; for an immortal it
does not. The current register compiler folds some single-reader loads, but
this producer survives once per outer logging call. Its owner is necessary under the
candidate's **current** invariant that every non-immortal object in a live
frame home has its own reference; it is not a law of the eight-byte value
format or of Python semantics. Pinned upstream uses a nonowning
`PyStackRef_Borrow` local load, can transfer that argument to a child frame,
supports local replacement through `f_overwritten_fast_locals`, and promotes
unsafe borrows in `_PyFrame_Copy` on heap escape. The candidate already has
the overwritten-local protection, but does not represent a mortal borrowed
frame home. This is a real design gap, not an observed speedup.

The retained candidate counters show 90 mortal increments and 100 decrements
per workload loop in aggregate. The four source-pinned warmed probes in
E8/warmed-shape-comparison all exited zero on CPU 15 with JIT disabled.
They confirm that `MESSAGE` and the logger are mortal in both interpreters,
that both warmed loops contain `LOAD_FAST_BORROW` at the relevant calls, and
that the candidate retains an argument-load producer in its register IR.
The probe's `MESSAGE` refcount of two is outside the measured interval.
These observations do not identify the owners behind the aggregate counts;
no upstream/candidate dynamic argument-owner comparison has been made.
An argument view may save one owner pair or a word producer only if its
actual route and emitted code allow it. It cannot remove the owner of the
message local, the kwargs dict, the call frames or a native boundary owner.

## Invariants the design must prove

1. **Anchor and identity.** A view names a specific evaluated object, not
   “whatever is now in local slot N.” Until the view is retired or promoted,
   an independent strong owner remains reachable. For a synchronous child of
   an ordinary thread frame, the caller local may be that owner. If a
   frame-locals proxy overwrites it, `f_overwritten_fast_locals` must acquire
   the old object before the slot's owner closes. The view continues to name
   the old object. Direct local writes, reentrant callbacks and any frame
   clear/detach operation need the same proof or must force promotion/decline.
   A GIL alone is not an anchor.
2. **Reference and GC accounting.** A borrowed child slot adds no reference.
   Its close must not decrement, and cyclic-GC `visit_decref` must not
   subtract it as an owning edge. Traversal for marking, frame exposure and
   diagnostics must still see its value and its actual owner. Current
   `Python/gc.c:_PyGC_VisitValue` already distinguishes no-count values from
   counted edges, as upstream `_PyGC_VisitStackRef` does, but current
   frame-home validation assumes a tagged mortal pointer is invalid. Any new
   representation must make the edge count and root relation explicit and
   test them with cyclic objects. A view cannot silently become a
   heap-container edge.
3. **Transfer and escape.** Normal argument evaluation, a direct child push,
   return and unwind must each have one clear transfer/retirement rule. Before
   a child or caller frame is detached, copied to a frame object, suspended as
   a generator/coroutine, yielded across a resumable boundary or exposed to
   an owner that may outlive the anchor, each live mortal view must become an
   owning canonical value or the route must decline. Promotion must happen
   while the anchor is live, before changing frame linkage, and failure must
   leave all original owners and exception state valid. The current candidate
   `_PyFrame_Copy` moves owning words; it cannot be reused unchanged for a
   view. Upstream's `PyStackRef_MakeHeapSafe` is a concrete reference model.
4. **Observation and order.** A callback may inspect/replace `f_locals`,
   trace a call, change code or providers, or raise during preparation.
   Observed argument identity remains the evaluated object. A view must not
   advance its destruction time, hide an owner from a frame snapshot, move a
   reference release across a line/monitor boundary, or weaken the current
   post-callback refresh and sequential retirement. On errors, child setup
   failure and native fallback, the original parent homes remain recoverable
   until the route has committed; no view may be closed as an owner.
5. **Native boundary.** Object-form C calls and compatibility thunks need
   stable `PyObject *` arguments and the established borrow-stack/native
   scope. A frame view cannot be passed as an owning `PyValue` or stolen by a
   native vectorcall. Promote or derive an explicitly scoped owner before
   parent detachment; retain the current boxing rule for inline scalars.
   Value-native APIs may borrow from a live owner only under their declared
   call scope, and any retained result remains owned. A private frame tag
   must be converted to a canonical borrowed API view before a generic value
   operation; its tag must not leak as an ordinary `PyValue` argument.
   Unsupported native routes keep the current owning call transaction.

## Concrete owner and transition proof

### Owner graph for the proposed one-argument pilot

Let `O` be a **mortal test argument** (not yet proven to be the benchmark's
`MESSAGE`), `L` its caller-local home, `W` the physical call-window cell,
`C` the child argument home, and `T` the
frame object's `f_overwritten_fast_locals` tuple. The source is an ordinary
fast local, not a cell or hidden constant. The view records `O`, not a
recipe to reread `L`. The pilot admits only a live `FRAME_OWNED_BY_THREAD`
caller, a non-generator exact Python callee and a positional local source.
The caller cannot execute a direct `STORE_FAST` while suspended in its
synchronous child. The only supported observer write to `L` during that call
is `Objects/frameobject.c:framelocalsproxy_setitem`; any other internal writer
or frame detach requires a pre-view decline or a proven promotion gate.

| State | Counted owner of `O` | Noncounted view of `O` | Required action |
| --- | --- | --- | --- |
| Before/load | `L` (one edge) | None | Capture `O` while `L` is live. Preflight all route exclusions before making a view. |
| Call window and fallible preparation | `L` (or `T` after a supported proxy overwrite) | `W` (zero edges) | Keep `W` and the anchor intact. The pilot permits no effectful preparation after view creation; any broader route must prove each callback or promote first. |
| Child published | `L` or `T` | The same physical cell, now `C` (zero edges) | Transfer the slot role to the child before switching `current_frame`; parent has no second owner of that cell. |
| Child return/unwind | `L` or `T` | `C` until cleared (zero edges) | Clear `C` without `DECREF`; preserve the existing result, PC and exception order. |
| Parent later retires | `L`/`T` until its ordinary close | None | Only now may the last anchor release `O`, unless an escaped child copy was promoted earlier. |

If the proxy replaces `L`, its `add_overwritten_fast_local` first creates a
tuple owner of the *old* `O`; only then does it close `L` and install the new
value. If tuple allocation fails, the setter leaves `L` owning `O` and the
call reports that failure. The child view still names `O` in either case.
The active caller frame must keep its frame object and `T` alive while the
view exists. `frame.clear()` rejects an executing thread frame, but the
review must also prove that GC `frame_tp_clear`, frame deallocation and line
relocation cannot clear `T` or `L` while this child view is live; otherwise
the pilot needs its own support owner and loses the intended refcount saving.
For a cyclic `O`, the active graph is thread-root → caller frame → `L` → `O`
with one counted frame edge, plus `C` ⇢ `O` with zero counted edges. After a
proxy overwrite, the support path is caller frame object → `T` → `O` with
one counted tuple edge; the old `L` edge is gone. GC marking may inspect the
view, but `visit_decref` must subtract **zero** for it. If a retained child
frame receives an owned copy, that new frame contributes exactly one counted
edge and no view edge. Tests must use a tracked `O` in a cycle, invoke GC
during the child and after each transition, and check identity, collection
and expected edge counts. This graph is the admission proof, not a claim that
all caller locals can always anchor a view.

### Monotonic promotion and route closure

The only view-to-owner conversion point is a frame-slot promotion operation
while its anchor is live. It reads the captured `O`, checks that the slot
still denotes that object, takes exactly one strong reference, publishes the
owned canonical word in the **same** slot, and only then clears that slot's
view role. For side metadata, the untagged mortal pointer can take its count
before the role clears. For a private no-count tag, `PyValue_DUP` on the
*tagged view word* is wrong: current code would treat it as no-count. That
variant must reconstruct an untagged canonical borrow from the captured
`O`, take one count from that object, then publish the owning word. The first
pilot admits a mortal nonnumeric object; exact numeric objects that would
canonicalize inline are excluded until a separate conversion rule is
reviewed. A mismatch between captured `O`, slot bits and the current anchor
is a failed proof, not permission to dereference the slot. For a valid
mortal pointer, taking the count does not allocate, call Python or raise;
the operation must preserve any pending exception. Promote all
live views in deterministic slot order before changing frame linkage or
copying. If a selected representation needs side-storage allocation, allocate
and validate it *before* installing the first view; failure leaves the
original owning route untouched. Promotion itself must have no fallible step.
There is no rollback after a successful promotion: subsequent failures use
the existing owning-slot unwind, each added owner closing once. Debug checks
verify the anchor, object identity, slot role and reference delta before and
after conversion. A scheme that can fail halfway through promotion without
these states is rejected.

`Python/frame.c:_PyFrame_MakeAndSetFrameObject` creates an object pointing at
the running frame but does not copy its slots. `Objects/frameobject.c` reads
such slots through the proxy; it must return an owned read of `O` and honor
the view role, while its setter follows the support rule above. Retaining a
frame object at child retirement calls `Python/frame.c:take_ownership`, which
must promote every child view before `_PyFrame_Copy` moves words and clears
source slots. The frame-object allocation occurs earlier; if it fails, no
view role changes. `Python/bytecodes.c:RETURN_GENERATOR` allocates through
`_Py_MakeCoro` before `_PyFrame_Copy`; `Objects/genobject.c:gen_new_with_qualname`
allocates the generator before its copy. The pilot excludes both, so reaching
either with a view is a contract failure; a later extension must promote after
successful allocation and before copy. A resumed generator or coroutine may
hold only canonical owners, never a view tied to a thread caller. A retained
frame, suspension, stack switch, JIT/native handoff or any parent-frame
detachment while a dependent child view exists likewise requires promotion
before the anchor can disappear; the first pilot declines before such a
route. The first fallible operation *after* promotion uses existing owning
cleanup and error propagation. A raising finalizer on an overwritten local
must still run only when the support tuple/frame later releases that owner,
not while publishing the child or changing the call's fault PC. Raw
`sys.getrefcount` may differ because the proposed view omits a count; object
identity and the last-owner destructor ordering are the required behavior.

Before invoking `call_transaction_push_python_fallback`,
`call_transaction_native`, `call_record_monitor`, PEP 523 or an instrumented
path, promote `W` while the parent anchor is verified live, then discard the
view role and use the current owning transaction from that point. This is the
single conversion boundary for all post-view declines. The pilot preflights
route exclusions, but ordinary function-version, code, recursion and
stack-space guards may still decline after view creation. Every such legal
guard exit must promote `W` while its anchor lives, then resume the existing
owning fallback/deoptimization at the original PC and with the original
exception state. A fatal assertion is reserved for a corrupt view/anchor
record, not a legal cache miss. For a later
general-call route, a failed final guard may decline only after promotion;
an error from refresh retains the original exception and follows the
existing `-1` cleanup, without replaying monitoring. The fallback binder's
`PyValue_XDUP` copies, the native vectorcall's owning window and ordered
`call_transaction_detach`/`retire` then see ordinary owners. No view reaches
an object-form C argument vector or a consuming provider.

## Representation alternatives, with no winner yet

| Question | Private tagged frame borrow | Canonical word plus explicit frame-view/anchor metadata |
| --- | --- | --- |
| Frame slot | Give a mortal borrowed pointer a private tag distinct from an owning value, or redefine the existing no-count tag in a frame-only domain. Heap homes still receive only canonical owned/immortal/inline values. | Keep the eight-byte value word canonical and identify selected frame slots as nonowning views in a side table or typed frame-window descriptor tied to a stable anchor. Heap homes remain ordinary values. |
| Current conflicts | Bit 0 currently means immortal/no-count in homes; bit 1 is invalid/wrap in debug. `PyValue_CheckValid`, `_PyValue_CheckHome`, `PyValue_DUP/CLOSE`, pointer classification, fast exact-word comparisons and generator assumptions would need a precise frame-view domain. Reusing a tag without those changes is unsound. | Raw `PyValue_CLOSE`, generic frame clear/copy and GC traversal currently assume a word is independently owned. Every route touching a view must consult metadata; stale or lost metadata is a correctness fault. A separate slot role must not become a hidden second heap-value representation. |
| Anchor and escape | A tag reports nonownership but does not itself identify or retain the anchor. Additional proof or metadata is still needed for local replacement and cross-frame lifetime. | The descriptor can name source frame/slot and original object, with an explicit relation to the supporting owner. Slot replacement still needs the overwritten-local support or promotion. Metadata storage, move and invalidation may cost more than one incref. |
| Generator/JIT/observer impact | Value accessors and generated handlers must classify the tag before close, duplicate, identity, box and heap write. JIT must not treat tagged views as canonical heap values. | Frame access, copies, unwinds, GC and generated call/result handlers must carry the slot role and anchor. JIT and out-of-process readers need a documented frame-layout contract while heap encoding stays fixed. |

The two alternatives are hypotheses. A hybrid is possible only if it has one
coherent owner rule and does not create an untracked heap representation.
“Tag the borrow and leave the rest unchanged” and “store a raw borrowed
pointer in an owning home” both fail the current contract. `PyValue_Is` masks
the current no-count bit for pointer identity, but exact-word tests and
canonical heap storage still require review; identity alone does not settle
ownership. Prefer the simpler representation only after an actual consumer
inventory, generated-code cost and peer review.

### Consumer migration map; exhaustive census still required

The pilot must admit a view only from a checked local-load producer into one
checked direct-call consumer. The table identifies source families and known
consumers; it is **not yet an exhaustive proof**. Before choosing a
representation, scan every frame-slot read/write/move/clear, generated
handler, GC visit, frame exposure/copy and native/JIT handoff, and record
the conversion or rejection at each reachable boundary. A view reaching an
unlisted generic handler is a failed design proof, never an implicit “owned”
default.

| Boundary and actual source | Tagged alternative must do | Side-metadata alternative must do |
| --- | --- | --- |
| `Python/regcfg_passes.c`, `Python/regcfg.c`, `Python/flowgraph.c`; `Python/bytecodes.c:LOAD_FAST_BORROW`, `_INIT_CALL_PY_EXACT_ARGS`; `Tools/cases_generator/analyzer.py`, `ir_generator.py`, `ir_metadata_generator.py`; `Python/generated_ir_cases.c.h` | Permit a private view tag only for a temporary producer whose sole checked consumer is the direct call. Classify its tag before generated `DUP`, `CLOSE`, guard, identity or fallback. Promote before any intervening wrapper and on every legal guard decline. | Emit the same producer/consumer proof and transfer the slot-role/anchor record with the physical window cell when it becomes a child local. Copies, swaps, compaction and other users are rejected before view creation unless the role moves with the word; legal declines promote first. |
| `Include/internal/pycore_value.h`: `PyValue_IsObject`, `PyValue_DUP`, `PyValue_CLOSE`, `PyValue_Is`, `_PyValue_IsSameWord`, `_PyValue_CheckHome`, `PyValue_CheckValid`; `Include/internal/pycore_interpframe.h:_PyFrame_PushOverlapping`, `_PyFrame_PushUnchecked`, `_PyFrame_SetReturnValue`, `_PyFrame_ClearTemporaries` | Define a frame-view domain and conversion accessor. Ordinary value operations and heap stores receive a canonical owner or a canonical borrowed API view, never the private tag. Return/result slots remain owning. | Keep the word encoding unchanged; slot-role-aware helpers replace raw close/copy/traverse at admitted frame sites. Result slots remain owning. Detect a role/word mismatch before ordinary value helpers run. |
| `Python/ceval.c:call_transaction_overlap_python`, `call_transaction_push_python_fallback`, `call_transaction_native`, `call_transaction_detach`, `call_transaction_retire`, `clear_thread_frame`, `_PyEval_FrameClearAndPop`; generated general call | Promote before any generic fallback, monitor/native provider or owning cleanup. No private tag reaches `_PyValue_VectorcallTstate` or the binder. | Clear role as part of promotion before these same calls; do not duplicate/retire the view through raw `PyValue` arrays. |
| `Python/frame.c:_PyFrame_Traverse`, `_PyValue_CheckFrame`, `clear_slots`, `_PyFrame_ClearExceptCode`, `take_ownership`; `Python/gc.c:_PyGC_VisitFrameStack`, `_PyGC_VisitValue` | Recognize the private tag on frame traversal; visit the object for marking but subtract zero counted edges. Close without decrement; promote before copy. | Consult role for every admitted slot. Traverse the object but subtract zero; clear role with slot, and promote before copying. A stale role/slot generation is detected, not silently treated as owning. |
| `Objects/frameobject.c:framelocalsproxy_getval`, `framelocalsproxy_getitem`, `framelocalsproxy_setitem`, `add_overwritten_fast_local`, `frame_lineno_set_impl`, `frame_clear_impl`, `frame_tp_clear`, `frame_dealloc`, local export; `Objects/genobject.c:gen_traverse`, `gen_new_with_qualname`; `Python/bytecodes.c:RETURN_GENERATOR` | Proxy reads return owned `O`; overwrite support preserves old `O`. Prove active-frame clear/line paths cannot retire the anchor. Generator and retained-frame copies either see no tag by preflight or promote before copy. | Proxy reads consult role, return owned `O`; metadata follows only admitted overlap. Prove the same anchor protection. Any generator/copy path either sees no role or promotes before copy; no role is stored in the heap frame. |
| Native value/object adapters, JIT entry and out-of-process frame readers | Pilot runs JIT-off and rejects native/provider routes before view creation. Future JIT/native consumers need an explicit tag classification and owner conversion before a view can enter their ABI. | Same rejection now; a future frame-layout contract must expose role/anchor metadata to JIT/readers or force promotion at handoff. |

For a tagged variant, a word-only move carries the nonowner bit but not its
anchor; the checked call must establish that relation in the child. For a
metadata variant, the role is keyed to the slot's frame generation and moves
atomically with its physical cell through overlap. A trace read leaves the
role intact; a clear removes it; copy first promotes; compaction may move a
potential view only if its compile-time slot mapping is reflected at runtime.
Debug builds compare the recorded `O` with the slot word and the active
anchor on every transfer and after observer entry. The independent peer
review must approve a complete generated-consumer scan and a per-site
disposition; this table is the starting inventory, not an exemption for a
missed consumer.

## First bounded experiment and files to inspect

The retained warmed, pinned-source comparison now establishes the upstream and
candidate `logging_silent` call words and actual `MESSAGE` mortality. Next capture
pointer/owner provenance and the candidate route's attributable
message-object increment/decrement delta. The aggregate 90/100 counters
cannot substitute. The logging route meets the warmed producer and mortal
object preconditions, but is not an established owner-saving pilot until the
attributable owner pair and complete consumer census are proved. A tiny `f(x)`
that the compiler folds is not such a
pilot. If the exact-call shape has no retained producer, use another proven
shape or defer the pilot; do not credit a no-op source change.

The first implementation packet, if independently accepted, is GIL-only,
ordinary thread-frame to ordinary thread-frame, one positional mortal
argument from a live caller local into an exact Python callee. It excludes
generator/coroutine children, keyword/variadic binding, native providers,
PEP 523, monitored or instrumented entry, free-threaded execution and JIT
entry before view creation. A checked producer may create a view only for a
temporary destination whose one consumer is the selected direct call, with
no intervening arbitrary operation. The call's checked prefix must prove
function/code/stack capacity before it transfers the view to the child.
The checked terminal either publishes the child with the role transferred,
or promotes the slot and enters the existing owning route. All unselected
handlers see only owners. The generator must reject a different operand
shape, additional reader, callback, owner-retiring suffix, unexpected
wrapper or unproved terminal; metadata stays conservative for exceptions,
frame switches and observers. Runtime instrumentation changes after producer
selection need a gate *before* any replacement wrapper or observer executes:
either the producer proves that no such change can occur until its consumer,
or a view-aware dispatch entry promotes first. A call guard reached only
after a wrapper is too late. Ordinary tracing of the accepted child must
observe the original argument identity and the support-owner lifetime.

The bridge to `logging_silent` is separate and conditional. Its ten retained
loads feed `CALL_PY_GENERAL`, whose `call_transaction_overlap_python` creates
fresh `CO_VARKEYWORDS` kwargs and refreshes after a possible callback. One
possible extension carries a view through that allocation only after proving
that callback-driven local replacement, frame change and every fallback leave
the `L`/`T` anchor valid; on any decline it promotes before the consuming
binder/native route. Another extension uses a separately reviewed
[no-callback fresh-dict proof](followup-analysis.md) on a GIL-held dict
freelist hit with no active reference tracer; it promotes before ordinary
`PyDict_New()` on a miss or other decline and keeps all current final guards
there. Neither extension is chosen. The second also needs dict-specific
hit/miss counts; aggregate freelist counts do not establish coverage. If
`MESSAGE` is immortal, or neither bridge has bounded callback/fallback
proof, the pilot is a correctness exercise only and does not support a
throughput continuation for this target.

## Decision and rejection criteria

Before implementation, obtain a peer-reviewed owner graph and closed
consumer inventory for the selected route, plus source-level proof that a
borrowed edge cannot outlive or evade its anchor. The diagnostic must count
eligible, declined, promoted and fallback calls by route; attribute the
message-object owner delta and GC counted edges; and identify the retained
producer in warmed IR. Then use focused tests for local replacement through `f_locals`, callbacks
and tracing, cyclic-GC edge counts, frame-object retention, generator and
coroutine rejection/promotion, exception unwinding, native fallback and
ordered destructor observations. Release/debug generated code and actual
GCC must show a simpler accepted path without adding comparable metadata,
spill or branch cost. A later target-specific extension needs dict-creation
callback coverage and fresh balanced timing controls. Reject or defer the
mechanism if the anchor cannot be made explicit, any escape/GC route needs an
unbounded case list, candidate heap canonicality or native ownership weakens,
or the actual accepted route saves no meaningful work. The current
six-commit batch, its performance verdict and its selected universal-value
direction remain separate from this proposed packet.
