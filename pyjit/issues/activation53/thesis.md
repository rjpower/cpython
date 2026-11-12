# Activation thesis: final owners and explicit transfers

Proposed for one broad Astra review; not implemented. Baseline is selected
`0e5e61f4c6bc820f2dd1a5f9ee6f7f1768834254`, with unchanged GCC15.2,
CPU15/hash0/JIT-off comparisons. [Plan](plan.md), [call audit](call-audit.md),
[resume audit](resume-audit.md) and [phase evidence](phase-evidence.md) define
scope and current facts. Round52 was rejected and is not the source baseline.

## Thesis

A register machine should construct an activation **in its final owning storage**
and return through an **explicit semantic continuation**. We already implement
much of this for ordinary functions. Generators still inherit a transient
thread-frame construction/move protocol; completion reconstructs SEND/FOR facts
from caller bytecode. The hypothesis is that removing those mismatches can
improve completion-heavy coroutine and ordinary-call workloads while preserving
one owner and observation model. Yield-only fusion did not remove enough work.

This is a falsifiable explanation of opportunities, not proof that activation
cost alone explains all instruction excess. Attributes, guards, instruction
selection and allocator behavior remain workload costs. Universal PyValue is
integrated; the gap is not an unfinished value rollout or a generic boxed call
around every exact call.

## What upstream does better, and what we already do better

Selected exact CALL already reserves/initializes once, overlaps thread argument
homes, acquires code separately from the mutable function and publishes ordinary
returns inline without boxing. Initialized storage is reused without per-call
scratch zeroing; thread return clears locals, not empty scratch. Upstream copies
positional arguments and initializes its local suffix. Repeating those previous
fixes or saying every resume enters another C evaluator would be incorrect.

Selected frames have a richer 14-word header versus upstream's 10-word
pre-locals header, plus canonical-value classification and recipient handling.
Our generator move copies and NULLs every physical register home; upstream copies
only through its live stack pointer and leaves popped source cells alone. The
register initialized-frontier invariant requires that clearing today, so deleting
stores alone is invalid. Generator completion uses an out-of-line adapter and
immutable word/family decoding, while ordinary return already has a direct
recipient. Upstream uses a collapsed computed-goto evaluator in this control;
selected uses generated tail-transfer handlers. Symbol percentages cannot be
compared handler-for-handler across those layouts.

The fresh coroutine profile has zero loss/stable identities and locates 9.28%
of samples in exact CALL, 7.62% RETURN_GENERATOR, 6.45% generator completion,
5.58% local clearing and 4.86% SEND_GEN. These are locations, not removable time
budgets. One unchanged coroutine loop creates/executes 242,785 Fibonacci
coroutines with zero PY_YIELD/PY_RESUME events. Generator traversal instead has
100,000 Tree iterator starts and 1,568,946 yield/resume pairs. The two workloads
need different weighted improvements within the same contract.

## Lane A: final-home generative construction

Choose thread overlap for ordinary functions and generator-owned heap homes for
eligible generative construction before initializing/binding the frame. Share
function/code pinning, argument binding and initialized-home contracts. Remove
the temporary thread reservation/header, full physical-home move and source
NULL loop, and temporary pop. Do not claim to remove generator allocation or
required final-home initialization/refcounts.

The full architecture executes any creation prologue in final storage and seals
the object at RETURN_GENERATOR. This needs an explicit constructing phase: owner
alone currently makes a generator frame complete. Define GC, current-frame,
recursion, origin tracking, callback/error teardown and never-awaited-warning
behavior before implementing that larger phase. Allocation uses the pinned
executing code, not a later mutable function.__code__ reread.

A bounded first slice may admit only a mechanically proved entry whose first
real instruction is RETURN_GENERATOR and decline before effects otherwise.
That avoids moving GET_ITER/GET_AITER, closure setup or observer prologues. It
must still be expressed through the shared activation constructor/binder, cover
ordinary generators/coroutines rather than a benchmark name, and keep all
nonadmitted providers on the existing path. Review must choose whether this
slice is a sound useful specialization or whether the constructing-state model
is necessary now. No new call opcode family or alternate evaluator is assumed.

Ordinary CALL cannot pay a large new transaction or repeated preparation to
support this case. Prefer call-plan or code-entry authority already available
at admission; specify actual common-path checks. Initial exact-only admission
must have a clear route for generic/known-plan users of the same constructor,
rather than a parallel ownership model. Preserve PEP523, monitored/traced calls,
argument-error behavior, code mutation, coroutine origins, pre-first-send
throw/close, allocation failure and deep recursion behavior.

## Lane B: explicit completion authority

Existing caller return_slot/return_pc carry ordinary return authority. A compact
continuation kind CALL/SEND/FOR/INTERPRETER_EXIT could record producer-known
operation class alongside them, fitting current x86-64 header padding. Every
producer, copy, takeover, exceptional entry and redirect needs a defined rule.
The saved PC remains a real executable site pinned by the code owner; source
positions remain attribution only. Incoming send recipient and outgoing child
continuation remain distinct facts.

The bounded kind-only version removes dying-owner/interpreter-owner inference
and immutable handler-family lookup on completion. It still reads immutable
A/B/D once at completion, retains window detachment/owner retirement, and adds
one literal store at each entry. A full descriptor also removes operand decode
but adds more stores/space on every resume; that tradeoff is poor for many-yield
frames unless weighted evidence earns it. Do not introduce another metadata
lookup and call it caching improvement.

An alternative is a compact canonical completion terminal beside generated
return, sharing the cold observer/error tail. It can remove adapter call/return,
prologue/status transport and repeated loads. It must not duplicate the full
transaction into every handler or push ordinary return through generic recovery.
Admission is after child cleanup, because that cleanup can install hooks or
redirect the parent. Astra should choose between these variants, or reject B
if saved work does not justify entry overhead. One coherent experiment is
preferable to an unbounded combination queue.

## Smaller secondary option

A dying unexposed frame can potentially move a returned named-local owner rather
than DUP it and close the same local during clearing. Exposed frame locals must
retain their values; observation/GC/proxies and finalizer reentry need an actual
exposure proof. This removes an RC pair, not the local scan, and adds an exposure
guard. It is subordinate to A/B and should be implemented only if review and
weighted evidence show a clean shared terminal ownership contract.

## Ownership, observation and rejection gates

- Every input/held result has one declared owner. The code anchor is independent
  of mutable function state. Allocation/failure paths cannot replay providers or
  leave a constructing object runnable, warning-producing or GC-invalid.
- Publish coherent current/exception/frame/recursion state before any escaping
  operation. Commit the real winning continuation before owner retirement;
  callbacks and same-PC redirect intent retain authority. No persistent chain
  cache or assumed stable observation epoch across escaping work.
- Preserve generic provider behavior and register recipient classes including
  local, temporary, discard and overlapping homes. Far completion adapters and
  throw/close/PRE/POST failure recipes remain real control paths.
- Test actual candidate release/debug paths: genexpr iterator errors/reentry,
  closures/code mutation, defaults/keywords/variadics, mortal constants, origins,
  warnings/async hooks, frame escape/GC, deep recursion, initially non-None send,
  local replacement, nested SEND/FOR, throw/close, monitoring/tracing changes
  during cleanup and redirects. Add only missing focused coverage.
- Inspect compiled ordinary exact calls as well as the changed path. Establish
  actual eliminated work and added branches/stores; assembly size/sample shares
  alone do not establish success. Keep trusted compiler boundaries explicit.
- Separate diagnostic counting from quiet timing. Screen against actual golden51,
  confirm material target results, run all29 and direct upstream. Full suites
  follow a worthwhile candidate. Reject lanes without repeated benefit and
  acceptable breadth; preserve their source/evidence without golden promotion.
