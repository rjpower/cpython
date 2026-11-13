# Error and unwind selection

Source audit: integrated `863b6d0cb08`. Proposal only; NULL control remains
authoritative. GC's delegated consumer `16f465dc0e5` is separate and still
needs matching IR admission and frames publication before integration.

The next coherent consumer is the actual error processor, not a normal-edge
map reused at an error PC. Capture an emitter-owned exceptional checkpoint
and its immutable word identity before cleanup can reenter. Retain that
selected view and a descending cursor through bridge cleanup, traceback and
monitor callbacks, scratch retirement, exception-slot installation and the
handled callback. Handoff to a normal handler is callback-free.

## Actual boundaries

| Source consumer/producer | Current authority | Required port |
| --- | --- | --- |
| `IREmitter.error_if`, `goto_error`, `error_no_pop` | `goto_error` clears phase and flushes a copied `IRStack` with `error_exit=True`; `ERROR_IF` clears declared inputs, while `ERROR_NO_POP` retains them. | Emit each reached exceptional checkpoint from the actual storage snapshot after its vacate/restore obligations. Neither declared input count nor successful `NORMAL_CLEAR_INPUTS` proves this state. |
| `ceval_macros.h:JUMP_TO_LABEL` | May call `finish_value_bridges` before entering the requested label. A newly failed bridge can change the error destination. | Error selection must precede this callback boundary. Selecting only at `label(error)` is too late. Keep native counted workspace policy unchanged. |
| `bytecodes.c:error` | Frame association/traceback creation and `_PyEval_MonitorRaise` run before scratch cleanup. | Selected homes must already be available to frame copying, introspection and GC during these callbacks. Revalidate actual callback mutations; never revive an invalidated checkpoint. |
| `exception_unwind`, `get_frame_exception_handler` | `CURRENT_OFFSET()` is `next_instr-1`; pending source-error state can substitute an explicit `(handler, exc_slot, lasti)` triple. | Preserve both the actual exceptional origin and selected home state. The triple chooses recovery, not which scratch words are readable. No line/column/source-position inference. |
| `RERAISE`, failed `END_ASYNC_FOR`, `RAISE_VARARGS` | Some jump directly to unwind; RERAISE first changes `frame->instr_ptr` and invokes monitoring. | The producer must select before that callback using its actual handler word. A later selector must not derive origin from mutated `instr_ptr`. |
| `_PyFrame_ClearTemporaries(f, first)` | Clears phase, loads every word in `[first,N)` in reverse, NULLs then closes nonNULL values. | Iterate only readable selected owner obligations, clipped by a descending cursor. Remove authority before a close; skip inactive gaps without loading. Do not drain a home again after a callback rewrites an already passed home. |
| `_PyFrame_CompleteRedirect`, `ceval.c:clear_init_shim` | Also call `ClearTemporaries`: redirect clears scratch after child retirement; the constructor clears its explicit shim including self at home zero. | Keep their actual redirect/constructor capabilities distinct from ERROR. Redirect needs its saved consumed-state recipe; the shim has an actual initialized staged extent. Neither may query a newly assigned PC for the old owners. |
| Handled branch of `exception_unwind` | Clears from `first=exc_slot-lasti`, installs tagged lasti and an exception owner, changes `next_instr`, then calls `monitor_handled`. | Install the two different protocol roles and publish the actual handler-entry shape before monitoring. A failed handled callback is a fresh exceptional ingress, not the old failing-op checkpoint. |
| `exit_unwind_notrace` and child-to-parent propagation | Unlinks child, performs M2 error retirement, then reconstructs parent's `next_instr=instr_ptr+1` and enters `error`. | Keep the existing child-retirement reservation/code lifetime. Parent waiting-call error state needs its own explicit continuation recipe, including actual consumed/promoted call roles. |
| `_PyFrame_GetHomeView`, copy adapter, GIL/FT `_PyGC_VisitFrameStack`, tracing transition visitor | Only created frames currently use selected GC ranges; most error states remain full NULL scans. Transition views have explicit authority and cursor. | All readers of an erroring frame must use the same progressive view. External marking must not become a heap-container subtraction edge, and generic referents must retain readable no-count values. FT requires the existing real quiescence/read capability. |

## Shared model and first source slice

Extend the shared emitter event graph at actual exceptional exits and
reentrant cleanup boundaries. Snapshot its actual home/transport state after
TAKE, consume, restore and vacate, before the next callback. Different partial
states need different finite checkpoints. Equal shapes can be interned; one
canonical ERROR phase cannot identify every early exit. Family variants must
agree on a checkpoint contract or report it uncovered. Runtime phase selection
uses emitted literals and already prepared finite state, never an arena read
of a virtual CALL hole.

The cold selector takes the pinned code/image, explicit canonical origin word
and emitted checkpoint. Its result is typed NONE/CANDIDATE/MUST, as for current
home views. The explicit development adapter may select NULL_CONTROL_READABLE;
CANDIDATE never authorizes stale reads. It must not call `_PyCode_GetExec`,
allocate, read homes or reconstruct a checkpoint from `frame->instr_ptr`.
The actual `next_instr-1` remains available in the error handler arguments;
snapshot its identity before a callback can replace it.

Use one reached NO_INLINE error processor to own the view and cursor. It must
start before `finish_value_bridges`, rather than adding an addressed record to
each ordinary handler. Its scope spans traceback/raise monitoring, suffix
cleanup, handler installation and handled monitoring. A concrete ABI is a
follow-on freeze: preserve the current tail-call parameters, return only the
actual next continuation/disposition, and do not leave a pointer to local
descriptor storage after the helper returns. Propagation loops through parent
frames rather than retaining one native helper frame per unwound Python frame.

The existing transition chain can carry the actual error scope as a distinct
metadata-only kind. Its home/header/result external-root authority is zero:
the active frame's traversal supplies its edges, including heap-container
subtraction. Visiting the same active heap homes as external roots would
artificially retain cycles. The node only selects the view and progress used
by those readers; it is not detached ownership. This requires an explicit
common-selector lookup, not two independent frame scans or a retiring kind.
The lookup must find the actual owning thread's scope during cross-thread GC;
current-thread TLS alone is insufficient. Resolve that cold context/lookup ABI
and its FT quiescence requirement before source freeze. Its limit clips scratch, while named
bindings are always read from their current homes. Frame capture must copy
the clipped selected view before scope disarm. Code and any owning arena/heap
storage remain pinned through the last view/range read and monitor callback;
unlink precedes reservation release. Code-owned ranges must not outlive their
pin. This needs source ABI review before adding a kind or selector branch.
An accepted observer redirect must invalidate or replace that scope before
mutating homes; clearing only `phase_state` cannot leave an old linked view
authoritative. Rejected redirects preserve the existing scope.

Before error-to-M2 handoff, transfer the selected view/cursor callback-free or
publish its explicit bindings-only remainder after complete scratch drainage.
Then disarm active selection and let actual retirement own external roots.
Do not drop the view and let M2 reconstruct its old full-N NULL range: that
would read inactive heap scratch under final authority. A saved PC or phase0
cannot recreate the discarded shape. The code pin spans this transfer.

## Constraints that block final authority

- Normal transport receipts currently certify successful absence, not all
  exceptional states. Opaque/provider/helper exits remain explicitly uncovered
  until actual helper ownership and error-result contracts are emitted.
- Dynamic array cleanup needs its real produced/consumed cursor. A static
  checkpoint alone cannot describe arbitrary partial unpack, builder, binder
  or native-result cleanup. Reuse actual prefix scopes or outline the first
  reentrant close; do not add a large record to every ordinary operation.
- CALL errors must preserve promoted SELF/KWARGS and consumed-window facts
  from prepared scalar state, including failures after wrapper retirement.
  Canonical entry ABSENT roles are not a post-effect error description.
- Binary restore phase 7 is useful existing evidence, but rejected restoration,
  frame/PC mutation and bridge failure cannot inherit its positive shape.
- `semantic_handler_entry` currently seeds the retained prefix unknown and
  marks both exception and lasti as MAY_OWNER. For selected authority, join
  actual exceptional predecessor retained-prefix facts, install tagged lasti
  as readable unowned protocol data, and install exception as an owner. No
  current unknown seed may be relabelled MUST.
- Recursion-entry failure, generator throw entry, source-event failure,
  completion-helper failure and failed handled monitoring are separate real
  ingress producers. The ordinary raising-op recipe cannot stand in for them.

Cost: no generic query or new tail-call argument on successful ordinary
instructions. Error branches publish a finite literal/variant and reach one
cold selector. The error helper owns one bounded scope plus a 16-byte view;
per-code checkpoint rows/ranges are interned and count toward code size.
Checkpoint IDs must fit the existing phase domain explicitly. State joins and
allocation checks stay bounded by admitted home extent, not fresh compiler
vreg count. Callback-phase coverage remains a separate M4 gate: entering this
processor cannot retroactively certify earlier callbacks in the failing helper.

First validation: actual error/traceback/bridge callbacks inspect the active
frame's selected IDs, not just the external transition-ID helper; descending
finalizers mutate passed and remaining named homes;
RERAISE's saved lasti differs from actual origin; lasti-only ranges contain no
counted edge; exception/handled-monitor failure reenters with a fresh shape;
CALL promotion and partial unpack/builder failures exercise real cursors.
Compare against NULL control and report unsupported truthfully until every
required producer and reader is ported. Root owns native execution and switch.

IR and frames lifecycle peers give direction GO: checkpoint before bridge
cleanup, explicit origin, one selected cursor and separate lasti/exception
roles. Frames' metadata-only active selection and explicit M2 transfer
conditions are incorporated above. Detailed ABI remains pending source freeze.

## Bounded scope ABI for the next freeze

Reserve transition kind 3 for active ERROR selection. Reuse the existing
40-byte node and its `homes`/`remaining` fields, with a reached helper's
16-byte view. Here `home_authority` describes the active frame's readable
selection, not an external owner. Header mask, result and prefix authority
are zero. The external transition visitor must explicitly skip this kind;
active frame traversal and heap-container subtraction use the common selected
view. Push validation must distinguish this metadata node from an owning
retirement node: its code lifetime is backed by the active frame's real pin,
not a fictitious CODE owner bit.

Proposed reached entry: `_PyEval_ProcessError(tstate, frame, origin,
checkpoint, value_temps)`, returning the existing two-word `_PyIRStepResult`.
The checkpoint packs only an emitted finite phase/variant. `origin` is the
explicit immutable word identity captured before bridges; `value_temps` is
the existing native workspace, not a new arena owner bank. Enter before
`JUMP_TO_LABEL` finishes bridges. The helper loops through propagation and
owns the view/node until handled continuation or final exit. On return the
handler reloads current frame, homes and continuation; no pointer into helper
storage survives. A failed handled callback selects a new checkpoint before
its next reentrant action. This preserves ordinary tail-call parameters and
adds no addressed record to successful handlers.

A dedicated cold ERROR-scope phase marker can dispatch common selection to
the owning node. Its bit and exact entry signature remain unallocated until
IR/frames review. Lookup first checks the current thread's chain; foreign
thread lookup requires the GIL or actual interpreter stop-the-world and
searches that interpreter's thread chains by exact frame identity. Merely
having PUBLISHED set, or using the collector's current TLS, is insufficient.
An unstopped FT foreign-frame lookup reports NONE unless a real frame read
claim also protects the node. Existing root walks that already know the
owning thread should supply that context to the same cold lookup, rather
than repeat a thread search. No generic lookup runs on an ordinary inline
instruction.

At terminal drainage the helper passes a bindings-only view and current
cursor into actual M2 retirement before unlinking the metadata node. Current
THREAD `TryClearExceptCode` and association initialization assume scratch
already empty and select only L; heap/generator paths select N. This is not
a general scratch-retirement contract. The error handoff must explicitly
preserve its selected extent through association takeover and all remaining
closes; code remains fixed-last. General RETURN needs the same explicit
extent sharing, separately from the empty-return fast form. No proposed
error selector authorizes a full-N load from stale scratch.

Cost to price: one error-only helper call and literal checkpoint, one
40-byte linked metadata node plus a 16-byte helper-local view, and cold
owning-thread lookup where needed. Checkpoints/ranges add code-owned storage.
There is no new hot frame field, ordinary handler record, count operation or
tail-call argument. Native prologue/spill effects still require root's
generated/native inspection; source placement alone does not prove no cost.

## First cold admission increment: retained absence

The captured `CoroutineTest.test_await_11.<locals>.foo` image has an exception
edge from YIELD word 9 to CLEANUP_THROW word 15 (slot 7, no lasti). CALL_KW at
word 13 needs the already empty home 3 below that cleanup's base 4. The upfront
unknown handler seed loses it. Root's captured IRSM has only one observer
record, SEND 8 to YIELD 9 with no recovery handler; it adds no handler-15
ingress. Evidence: `call-last-admission/full-decoded.txt` and
`coroutines-source.bin` under E55.

Proposed distinct `_PyIR_ABRUPT_PREFIX` format flag uses spare uint16 flag
space. It is cold metadata, not an appended normal ownership event: existing
sole-normal-program consumers must retain their contracts. The generator
sets it only from an actual terminal suspension receipt plus complete parsed
body/IO and audited resume/throw/monitor helper footprints. Family/twin
intersection removes it if any path is unexplained. For the first suspension
contract the abrupt scratch-write floor includes both outgoing B and any
scratch resume destination A; a named A cannot authorize scratch writes. This
must include the resumed/throw path and monitoring failure, not merely normal
YIELD output. Unknown helpers, changed arguments/body/continuation, new scratch
aliases or missing family receipts revoke it. Future producer classes need
their own actual emission receipts within this same footprint interface.

Before each protected word's normal transfer, enqueue its exceptional
predecessor separately. Construct the existing handler-entry state, but carry
only incoming SEM_NO_HOME below both the handler cleanup floor and the
positively certified complete abrupt-write floor. Do not carry INIT, physical
NULL, nonNULL, owner or pending delegated-role facts. Uncovered abrupt
producers contribute unknown prefixes. Source-observer recovery is a separate
ingress; without its own emitted footprint it also contributes unknown.
Successful normal cleanup receipts confer no exceptional preservation.

Remove unconditional unknown seeds for reached protected predecessors, which
would erase every positive join. After the reachable fixed point, seed unknown
for still unreachable protected/source recovery ingress and process the queue
again. This preserves structural hardening of unreachable handler code without
inventing a reachable positive predecessor. Both phases use the same bounded
must-intersection worklist; a later unknown predecessor can only remove facts.
Use a sorted exception descriptor index and binary lookup, or an O(words)
index, rather than rescanning the full table at every word/replay. One reusable
exception-state vector is enough; do not allocate per-word home bitvectors.
Codes without exceptional ingress take the existing allocation/scan path.

Tests must pair retained prior absence with a prior-live prefix negative,
overlapping/out-of-footprint mutation and missing source-recipe controls;
multiple exceptional predecessors must intersect, including an uncovered arm.
Exercise real await plus delegate throw/recovery and all public construction
routes. No new runtime phase, readable-home authority, physical NULL fact,
normal fill, fallback or M4 switch is part of this increment. This ABI and
worklist change require peer/root freeze before source edits.
