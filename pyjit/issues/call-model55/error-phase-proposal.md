# Error and unwind selection

Initial source audit: integrated `863b6d0cb08`, refreshed against the current
delegated composition and shared abrupt producer work. NULL control remains
authoritative. The delegated cohort has merged native evidence; it does not
provide general ERROR selection.

The first runtime implementation `c0e9cd3fe6b` and matching ingress
`0a690f32829` are integrated. Root reports both 117/79 builds and all five
ERROR lifecycle tests passing per role at `73ea`, after fixture-only
`3881c3a9a18`. Positive checkpoint generation remains separate.
The runtime executes checkpoint zero under an
explicit initialized NULL-control view: selection precedes bridge finishing,
one cursor spans monitoring and scratch drainage, and the callback-free M2
handoff carries only bindings. Kind 3 supplies metadata, never external owner
edges. The exact bit-14 marker dispatches the common frame selector to that
live scope; copied descriptors retain code-owned/static ranges only. The
foreign lookup requires the GIL or actual STW. A cold executing-thread guard
spans the verifier's HEAD-held recursive traversal; its four bytes occupy
existing padding on the asserted 64-bit layout. No successful instruction
gains a record, lookup or argument. No positive ERROR checkpoint or final
stale-scratch authority is claimed by this extraction. The isolated bitmap
composition uses actual presence selection; its source/native gates remain
open independently of this static NULL-control checkpoint.

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
cleanup, handler installation and handled monitoring. The five-argument ABI
below preserves the current tail-call parameters, returns only the actual next
continuation/disposition, and does not leave a pointer to local
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
conditions are incorporated above. The entry ABI below has IR/frames direction
GO and root approval; actual exceptional producer coverage remains a separate
source freeze.

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

Frozen reached entry:

```c
typedef enum {
    _PyIR_ERROR_HANDLED = 0,
    _PyIR_ERROR_EXIT = 1,
} _PyIRErrorDisposition;

typedef struct {
    _PyInstr *next;
    _PyIRErrorDisposition disposition;
} _PyIRErrorResult;

/* Same translation unit as the actual bridge and exception helpers. */
static Py_NO_INLINE _PyIRErrorResult
_PyEval_ProcessError(PyThreadState *tstate, _PyInterpreterFrame *frame,
                     _PyInstr *origin, uint32_t entry,
                     _PyValueTemps *value_temps);
```

The result occupies 16 bytes on the selected 64-bit ABI. It contains no
descriptor pointer or owner. `entry` has checkpoint ID in bits 0..7 and an
ingress tag in bits 8..11; higher bits are reserved and rejected. The emitter
must reject checkpoint IDs above 255 instead of truncating them. Checkpoint
zero means explicitly UNCOVERED with an independently justified development
NULL-control adapter; it is neither empty nor a static readable certificate.

| Ingress tag | First action and recovery semantics |
| --- | --- |
| RAISE = 0 | Select before bridge cleanup; then traceback/raise monitoring and actual handler lookup. |
| UNWIND = 1 | Select before bridges and enter handler lookup without duplicating traceback or raise monitoring. Explicit origin remains the raising word even if RERAISE changed public lasti. |
| EXIT = 2 | Bypass handler lookup, preserving the existing evaluator-exit tracing distinction. |
| EXIT_NOTRACE = 3 | Bypass handler lookup and the tracing action skipped by the existing notrace label. |
| THROW = 4 | Select the actual throw ingress before `monitor_throw`; then use the RAISE path. A successful saved execution claim does not itself mutate homes. |
| HANDLED_FAILURE = 5 | Select a fresh handler-entry snapshot before another clear after failed handled monitoring. |
| CALLER = 6 | After child retirement, select the parent's actual WAIT/error continuation, including prepared call roles. Successful CALL_DRAIN cannot supply this shape. |
| SOURCE = 7 | Use the actual pending semantic source recovery triple; source locations never select readable homes. Retain the current recovery decoder until its container migration. |

`origin` is the
explicit immutable word identity captured before bridges; `value_temps` is
the existing native workspace, not a new arena owner bank. Enter before
`JUMP_TO_LABEL` finishes bridges. The helper loops through propagation and
owns the view/node until handled continuation or final exit. On return the
handler reloads current frame, homes and continuation; no pointer into helper
storage survives. A failed handled callback selects a new checkpoint before
its next reentrant action. This preserves ordinary tail-call parameters and
adds no addressed record to successful handlers.

A dedicated exact `_PyIR_PHASE_ERROR_SCOPE = (1u << 14)` marker dispatches
common selection to kind 3. It cannot combine with captured, created or
delegated suspension markers. Lookup first checks the current thread's chain; foreign
thread lookup requires the GIL or actual interpreter stop-the-world and
searches that interpreter's thread chains by exact frame identity. Merely
having PUBLISHED set, or using the collector's current TLS, is insufficient.
An unstopped FT foreign-frame lookup reports NONE: a suspended-generator
read claim does not protect another thread's active native ERROR scope.
Existing root walks that already know the
owning thread should supply that context to the same cold lookup, rather
than repeat a thread search. No generic lookup runs on an ordinary inline
instruction.

Concrete lookup proposal: `_PyFrame_GetErrorHomeView(owner_hint, frame,
out)` copies the selected view and clips it by the linked node's cursor.
`owner_hint` is either the actual thread already known by a root walk or
NULL; it is not a substitute for quiescence. A NULL hint first searches the
current thread's chain, then `interp->threads` under the GIL or actual STW.
Match exact kind and frame identity through the whole nested chain. No home
read, callback, allocation or new pin is performed. The result retains only
code-owned/static ranges, not a pointer to the helper's stack descriptor.
Callers still need their existing frame-read capability before inspecting
the phase marker. Missing/duplicate/unprotected context returns NONE rather
than reconstructing geometry from PC or code size.

Thread-list locking is a separate capability. The verifier holds HEAD_LOCK
through root enumeration and recursive `drain()`, where arbitrary `tp_traverse`
calls can reach a foreign frame. Publish a cold nested HEAD-held depth on the
executing thread around each actual HEAD-held traversal region, restore its
prior depth on every exit, and clear it before HEAD_UNLOCK. The lookup uses
unlocked enumeration only under that explicit guard; otherwise it takes
HEAD_LOCK itself. An owner hint proves neither lock ownership nor quiescence.
Do not infer ownership from a mutex's locked bit. No Python callback may run
while this traversal guard is published. Review the exact thread struct
placement and all held traversal regions before source freeze; charge any
padding/size increase and cold entry/exit stores. This adds no hot frame field
or tail-call argument.

At terminal drainage the helper passes a bindings-only view and current
cursor into actual M2 retirement before unlinking the metadata node. Current
THREAD `TryClearExceptCode` and association initialization assume scratch
already empty and select only L; heap/generator paths select N. This is not
a general scratch-retirement contract. The error handoff must explicitly
preserve its selected extent through association takeover and all remaining
closes; code remains fixed-last. General RETURN needs the same explicit
extent sharing, separately from the empty-return fast form. No proposed
error selector authorizes a full-N load from stale scratch.
Withdraw the ERROR marker and its borrowed frame/view authority before code
close or reservation pop. If an exhausted node remains linked during a
callback, it must be explicitly disarmed and unselectable; arbitrary STW
timing cannot make a stale frame pointer discoverable again. Accepted
mutation needs its actual replacement shape before any writes; NONE cannot
stand in for a cleanup recipe. Initial NULL-control adapters must prove their
actual owned initialized interval, particularly on parent waiting-call ingress.

Cost to price: one error-only helper call and literal checkpoint, one
40-byte linked metadata node plus a 16-byte helper-local view, and cold
owning-thread lookup where needed. Checkpoints/ranges add code-owned storage.
There is no new hot frame field, ordinary handler record, count operation or
tail-call argument. Native prologue/spill effects still require root's
generated/native inspection; source placement alone does not prove no cost.

### First producer and file boundaries

Frozen literal names are `_Py_TRANSITION_KIND_ERROR = 3` and
`_PyIR_PHASE_ERROR_SCOPE = (1u << 14)`. The frame marker is exact, not combined
with CAPTURED, CREATED or delegated suspension. These consume spare existing
bits; they are not implemented or reusable permissions yet.

GC owns the transition-node assertions/visitor, the cold owning-thread lookup
and common-selector port in `frame.c`, and the outlined processor in `ceval.c`.
Keep that processor in the existing translation unit so it can use the real
bridge and exception helpers without exporting a parallel implementation.
IR owns the checkpoint event/selector, literal entry emission and all changes
to `bytecodes.c` error/unwind labels. Frames peers mutable binding/copy and
accepted-redirect ports. Root composes these as one used producer/consumer;
do not land an unused kind or lookup independently.

Before the used producer/consumer source freeze, IR must supply actual emitted
origin/checkpoint entries for error, direct-unwind, recursion/throw and failed-handled
ingress. `error` and `exception_unwind` cannot both run the same traceback or
raise-monitor action. Preserve current monitor ordering, the source recovery
triple and the original RERAISE origin separately. `monitor_handled` failure
is a fresh selection before the next clear. The helper return must identify
handled continuation versus complete evaluator exit; no descriptor pointer
may escape the helper stack in either case.

After scratch is drained, actual error retirement receives an explicit
bindings-only capability before scope disarm and frame unlink. Do not merely
choose the old no-result wrapper by its name: that wrapper currently assumes
its scratch precondition. If a disarmed metadata node remains linked through
the retiring call, set frame NULL and all authority/pointers empty before any
callback; root inspection must report it as an empty node rather than hide
it. Prefer callback-free pop/handoff when the used M2 entry can accept the
saved descriptor directly. Reuse the general-return extent mechanics where
they express the real bindings-only interval; do not add an extra hot return
argument or a PC-based decoder.

First validation uses the actual active-frame selector/collector during
traceback creation and an error finalizer, a nested ERROR scope on another
frame, and an actual cross-thread STW root walk where supported. A transition
external-root-ID query cannot prove active selection: kind 3 owns no external
edges. Add explicit code/view lifetime and metadata disarm assertions at M2
handoff, plus the existing live named-binding mutation and capture controls.
The first producer's exact checkpoint coverage still needs source freeze; the
five-argument entry, result, eight ingress semantics and lookup/lifetime rules
above have direction GO. No unused selector or checkpoint alias may land alone.

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

## Next cold admission increment: real exceptional predecessors

The first producer/metadata component is committed as `93519833847`, with
IR's used cold EH consumer integrated separately. One shared source grammar
now supplies both operand-footprint and exceptional-route projections; the
old helper table/parser is removed. Root reports both-role native passage at
`e90`. The follow-on absence-effect source below is separately reviewed and
awaits merged generation/native checks.

The current loop in `normal_transfer_proof` enqueues an EH predecessor before
every protected word, including words with no path to error processing. This
can erase retained absence at an inert jump or constant publication. The
captured 54-word coroutine in
`E55/call-abrupt-writer-rejections/coroutines-image.log` has 12 EH intervals
and five source records, all with handler -1. Those records select no source
recovery target. They do not justify treating every protected word as an
ordinary error ingress.

Neither `_PyIR_PURE` nor absence of `_PyIR_RAISES` is a positive no-error
certificate. Actual generated dispatch has two additional routes:

* `DISPATCH` and `JUMP_TO_LABEL` finish native value bridges only when an
  actual boxing record has a nonzero count. Nonboxing handlers generate
  `value_temps = NULL`. Bridge failure can enter `error` without a DSL ERROR.
* Final local-result publication calls `release_displaced` and then
  `finish_displaced_local`. Its old-value close can reenter and its refresh
  can fail. A scratch constant result can therefore be inert while the same
  handler with a local recipient is an exceptional producer.

Keep exceptional route metadata separate from the sole normal ownership
program. Proposed `_PyIRFormat.error_routes` is a uint8 immediately after
`call_input_holes`. Source layout accounting puts it in existing padding
before `shadow`: reads/writes/through shift one byte, while shadow, twin,
events and total size stay unchanged. Root must verify actual C offsets.
No frame or code-object field is added.

| Route bit | Required producer evidence / word predicate |
| --- | --- |
| COMPLETE = 1 | Complete parsed source effect grammar and receipt coverage for every emitted exit, publication adapter and terminal. Absence of observed errors alone is insufficient. |
| DIRECT = 2 | An actual error/direct-unwind, poll, monitor, checked helper failure, child-error propagation or other nonconditional error route exists. A source branch need not always take it. |
| BRIDGE = 4 | Actual input boxing can leave a record for emitted terminal bridge finishing. Enabled only for that emitted handler's boxing capability. |
| LOCAL_DST = 8 | An actual displaced-local publication/refresh adapter can fail. Enabled only when its independently validated encoded recipient names a binding, not from generic output geometry. |

Zero means UNKNOWN, not no error. Additional child/WAIT capabilities can be
introduced only with their used source receipt; the first increment need not
certify them. A cold query, conceptually
`semantic_has_exceptional_predecessor(image, pc)`, returns true for UNKNOWN
or any enabled route. COMPLETE with no enabled route skips only the ordinary
EH enqueue at that word. Source recovery records, observer ingress, recursion
entry and external throw remain independently represented.

The producer is the actual shared emitter graph. Reuse the structured typed
expression/helper grammar, extending it to return an effect summary even for
an empty ERROR-token set; current `operand_error_sources` returns None for
that case and cannot certify inertness. Checked primitive projections, value
moves, constants and encoded jumps may compose. Helper summaries distinguish
no-arena-write from no-error/control-transfer. Raw frame/window aliases,
unknown calls/macros and untyped addresses or terminals lose COMPLETE.
Native metadata projections and both structured `Py_GIL_DISABLED` arms have
explicit typed contracts. No instruction name, successful drain or infallible annotation
substitutes for this check.

Expected explicit ERROR/direct-unwind routes must equal receipts recorded
after their actual copied-stack flush, as in ordinary abrupt-operand coverage.
Implicit boxing failure has separate receipt keys. Final output publication,
displaced refresh, polling, monitor wrappers, bridge finishing and terminal
dispatch each have an explicit effect in the same graph. DEOPT/redispatch must
close over the actual target family or lose COMPLETE. A terminal bypassing
recorded finalization poisons coverage. Union enabled route bits and intersect
COMPLETE over every family member/twin/redispatch target; a quiet sibling
cannot hide an error route.

For enabled producers, keep pre-normal exceptional enqueue and the independent
abrupt floor. Import only prior NO_HOME below both EH cleanup and complete
abrupt-write floors; classification alone supplies no preservation. RESUME's
instrumentation/pending calls and CLEANUP_THROW's reraise/monitor branch are
real producers. Successful completion is not their partial-error footprint.
No INIT, physical NULL, readability, completeness or pending tuple crosses
this ingress.

The next helper contracts must close these actual routes without copied
opcode bodies. Instrumentation takes a code pin and interpreter; periodic
checking takes tstate, never an arena address. Its callback contract permits
named-binding mutation but needs an explicit observational-frame boundary.
The resume oparg can exclude its periodic branch at/after AFTER_YIELD_FROM;
that does not exclude a twin's monitoring path. CLEANUP_THROW's actual
`IR_COMPLETE_STEP` supplies typed step-kind input/result bounds, independently
of its normal completion receipt. Its `monitor_reraise` argument must be
observational for scratch. The source grammar validates those exact helper
interfaces and all error/monitor routes; aliases or changed arguments revoke
the footprint. No whole-body clone or opcode-specific abrupt flag is planned.

The follow-on shared effect is `PRESERVES_SCRATCH_ABSENCE`, distinct from
`NO_ARENA_WRITE` and UNKNOWN. CLOSE/XCLOSE, pending calls and observation may
change bindings or vacate scratch through a debugger redirect. The setter
moves saved scratch only into bindings and NULLs unpreserved scratch; it does
not install a new scratch owner. Nested Python callbacks allocate outside the
active parent prefix, and an executing generator cannot resume itself.
Only encoded operand/recipient transport can destroy prior NO_HOME. This is
an absence proof, not permission to read a cleared or stale cell.

Source control flow rejects ERROR or direct unwind after adopting a changed
`next_instr`. Actual RESUME tests errors before adopting `frame->instr_ptr`.
A handler that can box and change origin loses both census and operand
coverage because terminal bridge failure would need the new origin.
`IR_COMPLETE_STEP` supplies its own reached status-error/terminal identity;
its encoded window/result confinement is independent of normal completion.
Source observer recovery remains a separate unknown ingress, including its
own exception recipe. No helper name alone supplies that recipe.

The second unknown-ingress pass uses the same producer classifier for
unreached words. It must not manufacture an unknown edge from a positively
inert word and undo the first pass. An unreached or reached UNKNOWN producer
still injects unknown; source recovery seeds remain independent. Existing
bounded queue/EH indexing and one reusable vector suffice. This adds cold
constant-size metadata and no per-PC allocation or native hot store.

Freeze tests cover protected inert jumps and scratch constants versus local
publication; boxing/bridge siblings; poll/monitor/helper errors; one missing
route/terminal receipt; independent unknown source recovery; and a prior-live
prefix negative. Renaming preserves primitive-derived behavior; injecting an
opaque helper or changing a target revokes COMPLETE. Use actual captured
coroutine ingress plus all public image routes. This is removal of false
predecessors under cold admission, not runtime ERROR readability or a full
M4 switch. IR and root give direction GO on the metadata/receipt model;
actual source changes still need bounded helper/producer freeze.
