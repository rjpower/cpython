# Activation53 architecture review

Review status: approve two bounded experiments with the corrections below;
no performance or implementation approval is implied. Selected source inspected:
`build/worktrees/round51-golden`, HEAD
`0e5e61f4c6bc820f2dd1a5f9ee6f7f1768834254`, under the documented
python-boil-the-ocean checkout. Canonical C is not the measured source. This
review read the packet, canonical frame/IR/observer contracts, applicable agent
guidance and the [official AI policy](https://devguide.python.org/getting-started/ai-tools/).
It performed no runtime edits, builds, executions or benchmarks.

## Corrected thesis and ranking

Construct eligible generative activations directly in their final heap homes;
complete resumed children through one compact canonical terminal. Preserve the
existing cheap ordinary-call path. This is a concrete opportunity to reduce
coroutine creation/completion work, not an explanation of the entire upstream
gap or a promise of generator/DeltaBlue parity.

1. **Primary A: first-word RETURN_GENERATOR direct construction.** Do not
   implement execution of arbitrary creation prefixes in heap storage now.
2. **Primary B: shared hot canonical generator-completion terminal**, with the
   existing observed/error transaction as its cold path. Do not add a stored
   continuation kind or full completion descriptor.
3. **Secondary, deferred: terminal local-owner move.** Admit only with a simple
   no-exposure proof and a measured frequency of mortal local returns. It is
   not required to integrate A or B.

The census counts Python-visible starts, not constructor hits or all actual
resumes. Zero PY_RESUME in Fibonacci means no suspension/restart events; it
still enters newly created coroutines through SEND. Likewise generator creation
is not charged at each yield. Record eligible constructor and exact completion
counts in a separate diagnostic run before interpreting a result.

Exact CALL already overlaps thread arguments and initializes once. Exact SEND
already enters the same evaluator. Ordinary RETURN already transfers canonical
values inline. Source-level wrapper names do not add a second evaluator, and
completion already specializes canonical SEND/FOR internally. Upstream also
uses temporary-frame creation, but copies a smaller live stack range; direct
heap construction is an opportunity to improve this architecture, not merely
to copy an upstream optimization.

The 1.57M generator yield/resume pairs versus 100k completions strongly reject
paying new stores on every resume to avoid a small decode once. A kind byte
would still require A/B/D decoding, while adding ordinary-call producer and
consumer obligations. Reject kind-only B. A full descriptor is worse weighted.
Do not revive round52 forwarding or round51 generator-clear flags.

## A: bounded construction contract

The admitted entry is **physical word zero itself** RETURN_GENERATOR, with a
valid compiler/constructor-proved initial-input recipient and real successor.
“First real instruction” must not silently skip MAKE_CELL, COPY_FREE_VARS,
GET_ITER/GET_AITER, RESUME, instrumentation, or helper words. Reject prefixes;
do not interpret them in a new helper. A genexpr whose iterator acquisition
precedes RETURN_GENERATOR remains on the existing route. Functions with closure
setup before that word also remain there. Generator and coroutine functions
with the proved entry are both candidates; async generators can use the same
allocator contract if their extra fields are initialized, otherwise explicitly
exclude them in this slice.

Use code/plan admission authority, not names or source positions. A bounded
code-entry fact may be established when the executable is constructed; it must
be derived again for code.replace/new code and checked against the live pinned
code selected by the existing function/provider guards. Immutable word zero
proves shape; the current executable/instrumentation state proves bypass is
allowed. A cached function version alone is not permission to bypass observers.

Conservatively decline when PEP523, tracing/profiling, relevant monitoring,
coroutine-origin tracking, LLTrace or enabled DTrace would require creation
entry behavior. Keep all current checks for argument validity, recursion and
provider identity before construction. Do not remove stack-capacity admission
in the first patch merely because the admitted path does not need thread
storage; that is a separate error-behavior change. A pending eval-breaker is
not evidence that GC already ran. Preserve the normal subsequent poll sites.

### Shared interface and exact-call integration

Introduce a cold helper accepting the **explicit executing code**, function,
validated binding description, caller and actual input homes. Its result is
MISS before effects, ERROR with the original inputs still owned, or a canonical
owned generator result after successful input transfer. The helper shares
`_PyFrame_Initialize`, constant initialization and binding rules. It does not
return a fake frame or tag a pointer as an object.

In exact CALL, test the admitted entry before `_PyFrame_ReserveForCall`.
A successful direct result terminates the CALL through its existing recipient
publication and continuation rules; it must not fall through `_SAVE_RETURN_SLOT`
and `_PUSH_FRAME`. A miss continues the existing exact frame setup. An error
uses a generated owner recipe that drains the intact call window exactly once.
This requires an explicit checked emitter terminal, not allocating/error-capable
code hidden inside today's `pure _INIT_CALL_PY_EXACT_ARGS` promise. Update the
producer proof, effects and generated consumers together. Keep ordinary exact
initialization itself unchanged after the miss branch.

Use the same helper for an already validated known positional binding plan
where it naturally returns a finished call through `_PyCallResult {0, NULL}`.
Defaults and empty variadics may be filled in final homes from that validated
plan; do not duplicate the generic argument parser. Keyword plans can be added
only if their existing validation and owner recipe remain equally explicit.
Initial exact plus known positional coverage alone is insufficient to claim
Tree construction coverage: the native value-vector boundary is another
important admitted producer, described below. Report unsupported generic
binding and keyword shapes honestly; they preserve the existing path.

Do **not** change frame-returning `_PyEvalFramePushAndInit` into a union or
pointer-tagged provider. Later generic coverage belongs above frame push, at a
provider boundary already able to return either an object or a child frame,
after binding has established the same entry authority. It can call this same
constructor. Requiring every binding shape in the first patch would force the broad
constructing-state design back into this bounded experiment.

Include the existing native vector boundaries in the first slice:
`_PyFunction_ValueVectorcall` in Objects/call.c and `_PyEval_Vector` in ceval.c
already assemble owned canonical argument arrays before calling the frame-only
API. At that point invoke the same constructor for exact positional count,
no keyword names, optimized code, no explicit locals, no variadics/required
keyword-only arguments, and the same first-word/observer/origin admission.
Require an existing executable initially; cold executable creation can use the
normal path. These are shape guards, not a second generic binder.

Hold the function owner in an explicit local; on MISS pass that same owner and
unchanged argument array to `_PyEvalFramePushAndInit`. On ERROR close those
intact owned arguments and function once, free only the scratch allocation,
and propagate the exception. On success their owners have moved and source
homes are NULL; free the scratch allocation and return the canonical generator
(or its owned object view at the object API boundary). No caller recipient
publication belongs inside this storage helper: the IR adapter publishes into
its caller; native adapters return through their existing API. Preserve native
recursion/stack admission as well as Python recursion semantics, declining near
limits to the current evaluator if necessary rather than silently bypassing its
error checks. Explicit locals and unsupported binding still use the frame API.

This covers an important distinction: exact IR resumes already share the
evaluator, but native construction currently really calls `_PyEval_EvalFrameValue`
or `_PyEval_EvalFrame`. A successful direct construction at these boundaries
can remove that creation-only evaluator entry too. The coordinator identifies
Tree creation at the value-vector boundary; verify candidate hits there before
claiming all 100k creations benefit. Supporting these bounded native producers
does not claim full generic binder coverage.

There is no way to select a new outcome with literally zero ordinary-call
checks while also forbidding a new specialized opcode. Accept and measure one
predictable entry-fact branch in the existing exact handler. Put extra guards,
allocation and binding in its cold admitted arm. No universal transaction,
new continuation stores or per-call metadata allocation is justified. Reuse a
suitable existing plan/code field if possible; do not grow every frame.

### Allocation, GC, code mutation and commit

Selected GIL `Python/gc.c:2009–2105` matters: GC object allocation links an
object and **schedules** GC through the eval breaker; it does not run Python
or collect synchronously. Existing known binding explicitly relies on this
property for an empty dict. The first-word, origin-disabled constructor can
therefore be one audited nonescaping allocation/initialization/commit segment.
Do not insert a poll or callback-capable owner close within it. A proposed
post-GC revalidation/replay transaction solves a nonexistent callback in this
bounded path and adds avoidable complexity. Free-threaded admission may decline
until its separate allocation/concurrency contract is proved.

Nevertheless, make code lifetime explicit. Acquire the executing-code owner
before allocation and pass that code to allocation/type/slot sizing; never
reread `func->__code__` to choose layout after committing to the code. The
original callable home anchors the function until transfer. The code owner can
be transferred to final frame initialization, or temporarily duplicated there
and released once the frame owns it; count any extra refcount pair as added
constructor work. Ordinary calls need not pay it. Function/code replacement
before admission uses current guards; replacement during prior genexpr callbacks
continues through the original prologue route.

A sound commit sequence is:

1. Finish all shape/provider/observer and binding guards without changing inputs.
   Pin code. Allocate the correct exact generator object with all type-specific
   fields valid, and frame state CLEARED. Allocation failure leaves caller homes
   intact. If an allocation primitive returns an untracked object, keep it
   untracked until commit; if it uses the existing tracked CLEARED shell, preserve
   the valid executable and origin fields required by its traverse/dealloc.
2. Initialize **every physical heap home** to NULL. Initialize the header once
   from the pinned code and function, then move each argument from the original
   caller home, NULLing its source, and fill validated defaults/constants.
   No failure or callback is permitted after the first owner move until commit.
   A raw home copy without source NULL violates the caller frontier invariant.
3. Set previous=NULL, owner=GENERATOR, frame state=CREATED, instruction pointer
   to the actual successor of word zero, and incoming `return_slot` to that
   word's proved recipient (including IR_NO_DST). Preserve all other initialized
   frame fields and async-generator fields. Track the complete object if needed.
   Transfer its single result owner to CALL completion.
4. Publish the actual caller destination and winning continuation before any
   displaced-owner close; honor pending redirects and reload continuation after
   callback-capable retirement exactly as ordinary CALL does.

Do not publish the heap frame as `tstate->current_frame` or decrement/increment
recursion merely to mimic a skipped incomplete frame. Preserve the existing
recursion admission threshold, and let actual first resume enter recursion in
the normal way. No exception-chain link exists until actual resume. Origin
tracking declines, so the normal origin walker continues to see its expected
incomplete creation frame. There is no general CONSTRUCTING state in this slice.

No CREATED object may be decrefed on a pre-commit failure: that could emit an
unawaited-coroutine warning or enter close machinery. Before moves, release the
valid CLEARED shell and the temporary code pin with the original exception
preserved. After moves there are no fallible steps before seal. If an implementer
needs callback-capable binding, origin calculation or revalidation after effects,
it no longer fits this proof: decline before those operations or redesign the
phase explicitly. Do not treat a speculative replay as a miss.

A removes temporary thread reservation/header/pop, whole-frame copy and source
clears for unused homes, plus the creation handler transition and transient
current-frame/recursion churn. It retains heap allocation, final NULL
initialization, argument moves, code/function owners and constant/default
owners. For a thread caller it loses zero-copy overlap for arguments but those
arguments had to move into the heap eventually. For a heap caller it can avoid
the extra heap-to-thread-to-heap argument hop. These are static work claims,
not cycle budgets.

## B: canonical completion without new entry state

Keep the dying-owner discrimination and the ordinary return branch. After
`_PyEval_FrameClearAndPop`, route a generator result to **one shared generated
terminal** in the existing evaluator calling convention, or one shared source
body instantiated only at that return join. Avoid a full transaction copy in
every return variant. This is a terminal of the existing evaluator, not another
evaluator or a SEND descent loop.

At this point reload the parent's state. Interpreter-entry completion retains
its current direct recipient route. For a normal parent, derive SEND/FOR and
A/B/D from its pinned immutable word, including the saved physical continuation
used by far adapters. If monitoring or a pending redirect requires the existing
semantic transaction, hand the untouched result owner to the cold helper before
detaching any window. Do not trust pre-child-cleanup observation eligibility:
child finalizers can install hooks or expose frames.

The admitted unobserved path performs exactly the existing semantic commit:
load/detach receiver and cursor, detach SEND's sent home, allow overlapping
SEND destinations, publish result or retain it in C for discard/FOR exhaustion,
and set both real continuation state and semantic owner phase before retirement.
Retire detached owners and displaced locals once, with the same order and
exception behavior as `complete_step_impl`. Reload the actual PC after those
closes. A finalizer installing monitoring after commit affects subsequent
dispatch, not retroactive PRE/POST events. A callback-selected same-PC redirect
still has authority; pointer equality is not proof of no redirect.

Share the commit/retirement implementation with the cold transaction where
practical, so observation fallback cannot drift into a second ownership model.
Retain all PRE/POST failure recipes and real source attribution. In particular,
FOR exhaustion must not overwrite the loop's last item binding. Do not add a
new decode cache or move completion setup to every resume.

The actual saving sought is the C adapter call/prologue, struct status return,
return-to-handler and redundant state reloads on unobserved completions. Decode,
receiver destruction, window detachment and result publication remain. If
compiler output preserves the same call/return sequence, or code growth causes
a loss, the lane has failed its mechanism or performance gate. Merely moving
`complete_step_impl` to a header proves neither improvement.

## Secondary and validation gates

A local owner move is legal only after the last return observation and before
unlinking if no frame object, locals proxy or other retained observation handle
can expose the old local. The existing instrumented return obtains its result
before its callback, which can mutate locals; do not detach that later local
as if it were the captured result. Admit the uninstrumented proved path only,
or retain the existing DUP for that case. Cells are container owners, not
interchangeable with their contents. If this needs broad exposure tracking,
defer it. Inline/immortal returns save no mortal RC pair, so count actual cases.

Before quiet timing, require executed release/debug checks covering admitted
and rejected constructors, positional defaults/variadics, keyword fallback,
genexpr callbacks/errors, closure prefixes, code replacement, mortal constants,
origin tracking, monitoring/tracing/PEP523, recursion failure, allocation failure,
never-awaited warnings, pre-first-send throw/close, GC cycles and frame escape.
For B cover nested SEND and FOR, local/discard/overlap destinations, far adapters,
child cleanup changing hooks, PRE/POST failures, redirects and mortal receiver
finalizers. Add negative emitter/admission tests for changed entry words,
recipients and effect prefixes; debug assertions are not admission authority.

Inspect actual native ordinary CALL as well as admitted construction and B.
Record eliminated loads/stores/calls, newly added common-path instructions and
code size. Diagnostic hits must show which unchanged workload paths execute;
monitoring census alone cannot establish that.

Screen A and B separately against immutable golden51, then their worthwhile
composition, using the packet's balanced target pairs. Confirm meaningful gains
and losses with six pairs; run all29 and a direct upstream comparison before
promotion. Require repeatable coroutine benefit for A/B, acceptable generator
and ordinary-call breadth, and retained logging behavior. A material DeltaBlue
regression cannot be waved away because neither primary lane was intended to
help it. Full diagnostics and independent source/native review belong at a
worthwhile integrated checkpoint. Reject weak lanes rather than accumulating
unmeasured architecture changes. No evidence here establishes a numeric speedup.

## What remains after these experiments

The ordinary-call gap has no demonstrated single cause. Its richer frame header,
canonical-value guards, recipient selection and displaced-owner checks are real
costs, while overlap and initialized-frontier reuse save work upstream performs.
DeltaBlue also pays non-call operations. Attribute whole-workload instruction
excess to calls only after a weighted native-path census establishes that share.
A and B should primarily improve coroutine creation/completion; their plausible
benefit to ordinary small calls is limited to reducing incidental code pressure,
not removing ordinary-call work directly.

For generators, 100k constructions cannot stand in for 1.57M suspensions/resumes.
Even a successful A leaves exception-chain updates, frame/current-state changes,
recursion accounting, input/result recipients and dispatch on those transitions.
The next priority there is a measured breakdown of those existing transitions
and their native register spills, rather than another forwarding/eligibility loop.
Keep the failed round52 experiment as a constraint on such proposals.

Immediate-await coroutines still allocate heap objects and initialize/clear full
activations after A. Deferring that allocation would require a separate escape
and materialization contract across observation, callbacks and suspension; it
is not part of direct final-home construction and should not be smuggled into
this round. Rank future work by the remaining weighted costs after admitted
A/B results: generator resume/yield traffic, ordinary-call header/recipient
traffic, then any broad allocation/materialization design justified by evidence.
