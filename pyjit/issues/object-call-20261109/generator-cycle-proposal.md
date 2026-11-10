# Generator cycle: ledger and next experiment

Read-only proposal, 2026-11-10. Source inspected in the execution-boundaries
runtime worktree, based on `4d666e1b482` plus the pending object/call packet.
This lane made no runtime changes, builds, native runs or commits. Native
admission and a new performance baseline remain the coordinator's work.

The closed checkpoint's recursive-generators kernel uses **1.415902× upstream
instructions, 1.442485× cycles and 1.018775× CPI**. Its profile concentrates in
SEND_GEN and YIELD_VALUE. These support removing executed work across the cycle;
they do not allocate the gap to any particular store, helper or frame transfer.
The separate resume micro uses 2.256941× raw upstream instructions and 2.801176×
after separate skeleton subtraction. Neither instruction ratio is a speedup
budget. Sources: [closing counters](../frame-call-model-20261109/closing-report.md),
[measured review](../frame-call-model-20261109/closing39-measured-priority-review.md).

Current41 calibration changes the ordering **for recursive coroutines**. Its
[retained profile](current41-profile-analysis.md) executes Fibonacci(25) through
awaits without a source yield or external asynchronous suspension; no YIELD_VALUE
self samples appear. Calls, coroutine construction/allocation and terminal
completion are prominent. Its 1.379018× upstream instructions and 1.342659× cycles
do not establish a yield-delivery bottleneck. The bounded
[direct-owning entry proposal](owning-generator-entry-proposal.md) can therefore
precede SEND-recipient work for that kernel, subject to creation-shape counts and
compiler-proven call-time prologue semantics. The simple resume micro and the
yield-forwarding tree below remain different evidence cases.

## What one traversal actually does

`bench/internal/micro/generator_resume.py` creates one generator per 1,000 yields.
There is no `yield from`, hence no SEND. One source-level iteration has 1,000
successful outer generator steps and one exhaustion, 1,000 YIELDs, and 1,001
inner range steps. Construction and final retirement occur once, not per yield.
Actual specialized handler counts still need recording.

`bench/internal/kernels/generators.py` traverses a balanced 100,000-node tree.
Each `Tree.__iter__` yields its own value and forwards its descendants through
`yield from`. A source model for one timed traversal predicts 100,000 generator
creations/completions, **1,568,946 YIELDs**, 1,468,946 successful child SEND yields,
and 1,668,946 total generator activations, including completion activations.
The recurrence is `Y(n) = n + Y(n//2) + Y(n-n//2-1)`, `Y(0)=0`; activations
are `Y(n)+n`. These are checked source-model arithmetic, not native counters.
Attribute reads, generator calls and loop/backedge work also execute. Refresh
counts/profiles after the object/call packet; do not assume a named benchmark
isolates a mechanism.

## Cycle ledger

| Phase | Current work and source | Frequency and obligation |
| --- | --- | --- |
| Construct | Exact call binds a thread frame; RETURN_GENERATOR allocates the generator, copies its frame, records the first incoming recipient, pops thread storage, and returns the object (`bytecodes.c:5222`). | Once per generator. Code/function/bindings remain owned; initialization and failure cleanup cannot be skipped. |
| Admit resume | FOR_ITER_GEN checks PEP 523, exact generator type and `gen_try_set_executing`; SEND_GEN additionally accepts exact coroutines and rejects created/non-None sends (`bytecodes.c:1629`, `3904`; `ceval_macros.h:464`). Generic SEND can enter the same primitive. | Each activation. Completion/executing objects must fall back correctly. No unchecked self-resume. |
| Commit resume | `enter_generator` detaches SEND's B+2 incoming owner (FOR supplies None), writes the parent's result recipient and real continuation, links child/exception state, makes it current, checks recursion, and installs/discards the incoming value with `_PyFrame_SetReturnValue` (`ir_generator.py:3901`). | Each activation, including first start and terminal completion. The child must be clearable before callback-capable input retirement; failure after commit unwinds, never retries binding. |
| Continue body | `LOAD_IP`, guarded `IR_SKIP_RESUME`, then dispatch. Skip requires current instrumentation/eval-breaker state; otherwise RESUME runs its checks. The micro also performs its inner range step. | Each activation. An instrumented RESUME is not skipped. Source proves neither an always-free check nor an extra RESUME dispatch every time. |
| Suspend/yield | YIELD_VALUE moves the canonical result, clears its source, advances the saved continuation, restores exception state, leaves recursion, records the child's next incoming recipient, updates profiler anchors, unlinks the child, writes generator-return kind and suspended state (`bytecodes.c:1708`). | Each propagated yield, so repeated at every ancestor in the tree. No fallible operation may be introduced into YIELD. State/link/roots must describe suspension before retirement can reenter. |
| Deliver yield | The existing canonical return sink publishes to the parent's return recipient. `_PyFrame_SetReturnValue` honors pending redirects and discard; the general slot sink installs the result and retires any previous owner (`pycore_interpframe.h:169–201`; `ceval_macros.h:801`). | Each yield, plus an incoming-value installation on every activation. A SEND parent uses B+2, which entry just cleared; other recipients may be locals, temporaries or discard. Count both installation and delivery costs. |
| Forward delegation | After SEND returns a yield, the delegating generator yields it to its parent; its next resume returns to the SEND continuation and loop/backedge. | Each forwarded descendant value. This multiplies the preceding state/publication work; it is absent from the simple micro. |
| Complete/retire | RETURN_VALUE holds its result while unlinking and clearing the child. `_PyEval_CompleteGeneratorReturn` decodes the immutable SEND/FOR site and calls the same `complete_step_impl` used by native completion (`ceval.c:2973–3100`). That transaction observes, detaches the whole input window, publishes SEND's final value, commits the exit, then closes inputs/displaced values and preserves a newer redirect/error. | Once per generator; each delegation edge also consumes one child completion. A FOR completion discards its return value. Clearing generator homes and exception state remains necessary. |
| Native bridge | `gen_send_value_ex` duplicates incoming borrowed arguments, enters evaluation, and reads thread-local `generator_return_kind` to classify yield/return (`genobject.c:286–354`). | Native send/next/throw routes, not every VM-to-VM activation. Do not replace this result protocol casually: the existing field has an explicit no-escaping-call contract. |

Ordinary YIELD already has a checked canonical transfer and avoids number
boxing/publication conversion. `_PUSH_FRAME` following IR_ENTER_GENERATOR in the
DSL does not establish a second executed frame push: the primitive dispatches
terminally, and the generated handler ends there. Do not count source macros as
native operations. Release/debug branches and compile-time tracing macros also
need the retained ELF, not a source-line tally.

## Three bounded changes: ordering depends on the workload

1. **First yield/delegation candidate: a proven empty SEND recipient.**
   SEND resume detaches B+2 before entering the child, yet the yielded-result
   sink uses the general old-owner load/test/close path. Represent EMPTY_TEMP
   versus GENERAL/DISCARD in the existing recipient descriptor, with no frame
   growth or independent flag, and use one sink implementation. Start admission
   only at SEND's static cleared-slot proof; retain pending redirects before
   consuming the descriptor. A suspended child's incoming recipient is separate
   from its parent's yielded-result recipient. Every constructor, copy, debugger,
   remote reader and `gen_yf` accessor must decode the same representation; do
   not expose tagged bits as a slot number. This is an internal ABI port, not
   a one-line publisher bypass. The [earlier recipient survey](../frame-call-model-20261109/return-recipient-options-after36.md)
   maps its risks, but its old36 text sizes are not current evidence.

   First gate: record how often that proof applies and count recipient updates
   **and** deliveries. Then compare compiled entry plus yield plus completion,
   including tag decode, stores, branches, total text and spills. Reject if the
   new classification work offsets the removed old-owner work. Do not read
   code/nlocals on every resume to save one load. This candidate targets recursive
   delegation; a SEND-only admission cannot explain or fix the FOR-only micro.

2. **Conditional next experiment: fold one plain delegation continuation.**
   If dispatch counts show that compiler-generated SEND → YIELD → resume/backedge
   forwarding is material, recognize only that exact uninstrumented island and
   reuse its real continuation and common result transaction. Remove one proven
   forwarding dispatch or redundant transfer, not an ownership/state transition
   by assumption. No inlining arbitrary child bodies, new completion ABI, copied
   binder or per-frame delegation flag. Instrumentation, debugger redirection
   and non-generator providers retain their existing route. The source/observer
   mapping and suspension/throw contract must remain reconstructible. This is
   a separate lowering/handler proposal requiring independent review; it is not
   yet an implementable optimization inferred from a profile label. Reject if
   the pattern is uncommon, observer machinery grows, or text/spills erase the
   dispatch saving.

3. **Creation-only alternative: bind directly into owning generator storage.**
   A narrow exact-generator provider could allocate its real owner and initialize
   the existing frame ABI there, avoiding thread-frame reservation/copy/pop.
   Preserve prologue execution, argument failures, cell/free-variable setup,
   exception/recursion rules and GC-visible ownership; code flags alone do not
   prove a harmless entry sequence. This has little leverage on the 1,000-yield
   micro. The tree model predicts roughly 16 activations per creation, but that
   does not establish allocation/copy's cost share. Require measured creation
   counts and retained samples in reservation/copy/allocation before designing
   the complete bounded entry contract. Do not bundle it with recipient work.
   For current41's creation-heavy recursive coroutine kernel, this is now the
   preferred first design candidate; [its bounded contract](owning-generator-entry-proposal.md)
   starts with compiler-proven empty-prologue, exact-positional entries and
   preserves the existing binder, owning storage and immediate-result ABI.

## Evidence gate before choosing implementation

Root should collect one stats-only census keyed by code/site and static producer:
SEND versus FOR, first versus suspended entry, actual specialized hits/fallbacks,
resume skips versus real RESUME dispatches, local/temp/discard recipients,
recipient writes, incoming installations, yielded deliveries, completions,
creations, native entries, and redirect/observation cases. Counters must neither
root values nor add frame flags. Use those counters for frequencies; use a quiet
uninstrumented executable for paired elapsed/PMU comparisons. Validate the
source model against actual handlers rather than adding temporary counters to
the final performance binary.

Then root retains matching source/ELF identities, inspects the complete affected
handler/helper text and spills, and apportions instruction samples across entry,
body/range steps, yield/delivery, completion and construction. Sample proportions
locate work; only a bounded changed-route comparison can establish a saving.
Keep coroutine/send/throw/close, created non-None, recursion failure, PEP 523,
monitoring, frame escape, locals/discard/alias, owner finalizers and newer
redirects in focused admission checks. Preserve one completion ABI and committed
state before retirement. Python's lack of an immediate-finalization guarantee
does not weaken lifetime safety, and exact CPython destruction timing is not a
performance or semantics requirement.

The first executable milestone is a workload-specific census. Yield/delegation
work needs a complete recipient read/write map; recursive-coroutine entry work
needs validated prologue/argument/caller-owner counts and a construction ledger.
An independently reviewed packet is justified only if its complete ledger
predicts a net saving. No optimization is authorized by this proposal, and no
fraction of either instruction gap has been attributed to a proposed change.
