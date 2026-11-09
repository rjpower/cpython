# Scoped semantic phase for observed callbacks

Proposal only during the source192c timing freeze. No runtime changes or native
worker jobs. Build on the existing request-owned Analysis/Plan and committed
redirect generation. No executable boundary, second offset domain or ordinary
per-dispatch metadata work is proposed.

## Actual source distinctions

The consuming instrumented POP_JUMP bool/None forms consume their temporary
input before INSTRUMENTED_JUMP. The jump wrapper already has the exact semantic
site and native destination. Its public LEFT/RIGHT name is not enough: ordinary
LEFT is fallthrough, while SEND completion LEFT is a completed semantic outcome.
The physical next pointer may first name a real relaxation helper; keep that
pointer separate from the normalized destination reported to monitoring tools.

Generated INSTRUMENTED_JUMP_IF_*_OR_POP is different: it holds res in C across
the callback and unconditionally IR_STORE_TEMP(A,res) afterward. Advertising a
committed POST state there would let a redirect clear/replace A and then have the
old handler overwrite it. These handlers remain unavailable until their actual
outcome is published before observation and the terminal adapter has no later
store on redirect/error. This is a source-level ownership condition, not an
opcode-name exception to general POST admission.

PY_YIELD is currently emitted before _YIELD_VALUE. The outgoing value remains
owned in its window; gi_frame_state is still executing, current/previous links
and exception/recursion state are unchanged. The generator has not yielded yet.
A debugger redirect can cancel that pending yield, but must dispatch immediately
without switching frames or publishing a resume recipient. A truly suspended
frame has already transferred all of those states and needs another protocol.

## One cold scoped record

Introduce one internal observer-context record containing borrowed runtime
coordinates only:

- observed frame and immutable real site;
- exact physical next pointer and semantic edge/outcome when present;
- a typed phase: PRE_WORD, POST_COMMITTED, YIELD_READY, STEP_READY, SOURCE_POST
  or UNSUPPORTED; each producer must satisfy its owner contract;
- entry redirect-generation snapshot, with absent frameobject treated as0.

Store a pointer to the currently scoped record beside the existing cold
observed_frame thread state; do not add fields to interpreter frames. The record
lives on the observed C adapter's stack. Entry/exit saves/restores the previous
pointer, including nested callbacks and explicit recursive tracing. A callback
for which no exact phase is supplied publishes UNSUPPORTED, so an outer record
cannot accidentally authorize an inner event. Existing observed_frame and event
gates remain authoritative for active ancestry; phase.frame must match the
requested frame and actual observed phase. Private debugger entry keeps its own
pending-child contract and never pretends to be PRE_WORD.

The record owns no Python values and cannot outlive its synchronous callback
adapter. Code/frame ownership already pins the coordinates. No serialization,
GC roots, suspended-frame field or monitoring-configuration domain is added.
Monitoring epoch changes do not change immutable register ownership; the setter
still rechecks actual reader/handler capability and redirect intent normally.

A cold getter copies this record into B's RegisterResumePhase factory. The
setter must not infer an outcome merely from what_event, a nearby instruction,
or a modern callback's mutable payload. Existing Analysis/Plan computes owner
requirements from the exact phase and target. It retains named-local obligations,
caught roots and unknown-context refusal. A valid runtime snapshot is necessary,
not a substitute for target availability or protocol proof.

## Producer contracts and staged admission

| Producer | Typed phase / ownership | Redirect completion |
| --- | --- | --- |
| Direct real LINE | PRE_WORD; handler has not consumed inputs | Existing Plan commits target, LINE wrapper dispatches target. |
| Source LINE | SOURCE_POST with actual from/to/edge and recorded EH | Existing generation-aware source transaction honors target and cancels old cursor/error recipe. |
| Consuming branch | POST_COMMITTED; consumed temp homes NULL, named inputs unchanged, no unpublished result | Jump helper returns committed target, no later scratch/result store. |
| FOR/SEND completion POST | POST_COMMITTED; window detached, SEND result already published, detached C owners isolated | CompleteStep retains its single owner and reloads actual PC after closing detached values. |
| Completion PRE STOP | STEP_READY; result C-owned, old receiver/window still anchored | Existing CompleteStep discards held result on redirect before reusing cached input. Not ordinary PRE cancellation. |
| PY_YIELD before switch | YIELD_READY; outgoing value still frame-owned, frame still executing | New typed yield adapter returns REDIRECT before MAKE_HEAP_SAFE/YIELD transition. |
| OR_POP retaining result | UNSUPPORTED until output commit is repaired | No admission while unconditional post-callback store exists. |
| Actual suspended frame / ordinary return / unknown hook | UNSUPPORTED | Keep existing refusal until its complete protocol is proved. |

First useful POST rung: consuming branches with a positively proven committed
outcome and target availability, using B's no-active-save cancellation policy.
Retain all caught roots; do not generalize the existing FOR-pair survival proof
across exceptional/context boundaries. Use actual successor facts, not the old
site's incoming consumed-predicate requirements. Facts at the landing may be a
conservative meet; actual source/outcome agreement is checked as in source26.
No generic inference that every input is killed is allowed. Where a cold outcome
helper needs explicit retirement facts, derive physical temp homes through the
existing role/operand decoder and exclude named-local borrows.

Second useful rung: YIELD_READY cancellation under the same no-active-save,
same-context, target-availability Plan. It changes no generator links/state on
redirect. The unchanged original test_jump_from_yield should replay the body
before the one real yield, not manufacture a resume or yield event. Postpone
true suspended-frame redirection and delegated-resume reconstruction.

## Yield adapter and errors

Replace the current PY_YIELD handler's PC-only test with a typed observed-result
adapter: CONTINUE, REDIRECT or ERROR. Compare both actual PC and generation after
callback and after releasing the owned event-argument copy. A same-PC setter is
still a committed redirect; never continue with the previously copied val after
its home may have been detached. New frameobject creation at generation0 is not
intent. On REDIRECT reload slots and dispatch immediately, with no _YIELD_VALUE
recipient/state/recursion/exception-link writes. The event's argument copy is
owned by the existing _Py_call_instrumentation_arg path, so later tools retain a
valid payload even if the frame home is cancelled.

On ERROR without redirect, retain the actual pre-suspension YIELD EH recipe and
owned window. On successful redirect then raise, use the committed real target
PC/EH (and no cancelled source recipe), exactly as existing generation-aware
source errors do. The generated error adapter uses published frame PC+1 where
its shared label expects next_instr-1. No parallel unwind engine is added.

Branch errors similarly retain the actual consumed/post state. The mandatory
constructor proof must admit its ordinary/source EH requirements from that
outcome; an error is not permission to resurrect the consumed predicate. FOR/
SEND retain their existing explicit PRE/POST status, owner cleanup and EH proof.
None of these records makes callback errors recoverable by guessing a nearest
word or restoring a stale cursor.

All application paths allocate/reprove first, detach surplus/restore actions,
commit the target/cursors/generation, then close with no later old-transaction
writes. Observed adapters return the latest actual target; finalizer reentry
must not be overwritten by a cached next. If a provider cannot prove that rule,
it remains unavailable. Unsupported external-finalizer ancestor redirects do
not become supported merely because a callback now has a typed phase.

## Truly suspended frames

Do not classify PY_YIELD observation as already suspended. Actual suspension
means previous link detached, gi_exc_state link detached, recursion unwound and
return_slot naming the future input recipient (or NoDst). Redirecting that state
needs a checked new resume-input destination, resume/start event ordering,
exception-context attachment and generator state transition. Delegation also
has receiver APIs that inspect that recipient. None can be repaired by replacing
instr_ptr alone. Keep suspension refusal until a separate full recipe is admitted.

## Interfaces, consumers and first gates

A owns scoped observer adapters/instrumentation plus bytecodes/generator yield
status and provider commit audits. B owns Phase getter consumption, exact normal/
source outcome facts and Analysis/Plan application in frameobject. Put the
shared record in an internal header; root admits its storage/API before edits.
Audit every existing observed_frame save/restore site: generic callback runner,
legacy optimized LINE, source LINE and private debugger entry. Every site either
supplies a valid phase or explicitly masks it as unavailable.

Keep original monitoring branch, tracing yield and debugger transcripts unchanged.
Add LEFT/RIGHT consuming branch redirects with non-idempotent body effects;
None/non-None predicate operands and retained local borrows; callback-owned
exception context; consumed predicate's last-owner reentry; same-PC setter and
setter-then-raise at a reachable unprotected target. Prove OR_POP retained owner
is refused until commit is implemented. For yield, test original replay, one real
yield/one resume, event payload lifetime, same-PC redirect, callback error and
unchanged generator exception/recursion links. Distinguish a truly suspended
frame negative. Then root runs monitoring/sys_settrace/code/source-reentry and
generator/coroutine suites before any broad performance qualification.

## Required scope/lifecycle refinement after independent review

B correctly requires record scope to match observed_frame and what_event exactly.
The callback runner must publish/restore all three together around each actual
callback, restoring them BEFORE closing its arbitrary result. The owned event
argument copy is later closed by _call_instrumentation_arg after that inner
record has ended. Direct legacy/source LINE and private debugger entry need the
same paired scope. A record cannot remain installed merely because a provider
still has detached C owners; restored outer event identity is not inner authority.

Pass an immutable provider phase SPEC through the broadcaster. It captures the
original PC/generation before the broadcast; each callback's scoped record keeps
that original generation, not a fresh snapshot after a preceding callback may
have redirected. Later tools may still receive the same event payload, but must
not regain old outcome facts from a newly captured generation. Include exact
event identity and require phase.frame==observed_frame==requested frame in the
factory. Source callback's existing stop-on-redirect policy stays unchanged.

When a Plan commits, its final target is a complete PRE landing before detached
closes run. A changed generation therefore invalidates OLD site/outcome facts.
A provider with a proved no-post-redirect-store contract may admit a derived
PRE_COMMITTED_LANDING at actual instr_ptr only if pending_redirect is clear,
instr_ptr==return_pc, source/error cursors are clear and active-frame/context
gates pass. Reanalyse target ownership from there. This preserves a finalizer's
newer direct redirect during Plan retirement. It is not a claim that the old
operation's output still exists. Pending child, private entry, suspension and
unsupported providers cannot acquire this landing authorization by comparison
alone. A fresh nested observed callback supplies its own authoritative record.

The record pointer is borrowed from a live synchronous C scope. Audit evaluator
activation entry/exit, generator current_frame switches and thread-state swap/
clear in addition to callback runners. No ordinary frame switch updates the
record: accessor validates current-thread active ancestry and scope association;
yield's record ends BEFORE suspension/previous-link detachment. Nested evaluator
entries save/restore their own callback scopes. If an activation/stack switch
replaces the owning C evaluation chain, the old record is unavailable until its
own scope is restored; never dereference a record from an inactive/destroyed C
stack. The exact cold activation guard/reset hook is a prerequisite interface
review, not permission for an unchecked raw pointer assumption.

Generic consuming branches are not certified by the existing completion-only
constructor proof. The next implementation must define and validate their exact
post-consumption owner/error facts using the shared cold physical contract. Do
not cite existing FOR/SEND completion acceptance as a general proof.

The additional INSTRUMENTED_FOR_ITER body census has the same unpublished-output
problem as OR_POP: item stays C-owned through LEFT and is stored/moved afterward.
The coordinator confirmed an error-path retained-owner leak with weakrefs and
collection on candidate AND matched upstream. Treat that as a real owner bug,
not as acceptable compatibility. The terminal commit proposal addresses all
such providers structurally; no permanent unsupported handler list is intended.

## Concrete storage/API refinement for peer

Prefer storing one nullable pointer on the existing private `_PyEvalActivation`
rather than adding a thread-state pointer. Initialize it to NULL before publishing
every activation. The pointee is an immutable C-stack record containing frame,
real site, physical next, original redirect generation, exact event, typed phase,
and exact outcome/edge or source-record identity where applicable. The provider
captures the spec before broadcasting; callback scopes do not refresh its intent
snapshot between tools.

Proposed cold APIs, declared with the record in an internal header:

```
typedef struct {
    struct _PyEvalActivation *activation;
    const _PyIRObserverSpec *previous;
} _PyIRObserverScope;
void _PyEval_EnterObserverContext(PyThreadState *tstate,
    const _PyIRObserverSpec *context, _PyIRObserverScope *scope);
void _PyEval_LeaveObserverContext(PyThreadState *tstate,
    _PyIRObserverScope *scope);
int _PyEval_GetObserverContext(PyThreadState *tstate,
    _PyInterpreterFrame *frame, int event, _PyIRObserverSpec *result);
```

Enter captures the current activation and previous pointer in the caller's scope
and publishes the supplied record on that activation. The synchronous callback
runner installs a real local record, calls the callback, then Leave restores the
saved pointer together with observed frame and event BEFORE
closing its arbitrary result. Entry with no evaluation activation cannot authorize
an active target; no record is published. Restore must return to the same owning
activation after the synchronous callback, with a debug assertion on that scope.
Private debugger entry and every callback without a provider proof install a
non-NULL UNSUPPORTED record. NULL means no scope on this activation; it must not
mean an unsupported inner callback.

Get walks the active activation chain to its first non-NULL record. This permits
ordinary Python callback code to enter another activation while the outer
observed operation remains stopped. The first record must match exact requested
frame, current observed_frame and exact what_event; UNSUPPORTED masks all outer
records. Copy the validated record into request-owned storage, then apply original
generation/active ancestry/actual phase checks. Never return an unchecked borrowed
pointer to the debugger plan.

This uses the existing evaluation chain for C-stack switching. A record on an
inactive chain is unreachable to the accessor; activation teardown restores its
previous link before its local storage dies. Generator suspension does not retain
the record: yield observation ends before frame/exception/recursion-link changes.
A new ordinary frame dispatch performs no observer state write.

Cold review notes an important restored-outer-scope case: nested callback result
retirement occurs after restoring outer observed frame/event/record. The outer
record is usable only if its provider is still in a proved phase and all exact
association/generation gates pass. Ordinary user execution inside an ongoing
outer callback can already run such finalizers. A successful change to the target
invalidates the old generation and requires a newly proved committed landing or
refusal; it never revives the old outcome. If no outer scope exists, or that scope
is UNSUPPORTED, retirement cannot acquire inner authority. This lifecycle must be
tested with nested callbacks returning mortal results, same-frame and different-
frame targets, and explicit recursive tracing.

This storage/API is still under B/root peer review. It supersedes the tentative
thread-state pointer placement above; no runtime field or helper has been added.

B independently approves the activation-owned scope direction for planning:
first-nonNULL lookup stops on UNSUPPORTED, exact callback association is paired,
and owning activation is captured for Leave. The fresh cold reviewer finds the
storage viable with explicit outer-scope/close-time association checks described
above. Neither review admits runtime/header changes without the provider ownership
and encoded-error proof.
