# Round 52 grouped architectural review

**Recommendation: revise the contract, then implement one bounded candidate.**
The three-word representation and yield-side transfer are sound architectural
choices. They can remove real intermediate home traffic and dispatch without a
second evaluator or new frame fields. Approval to implement is conditional on
resolving findings 1–4 below in the design and shared interfaces. Findings 5–8
are implementation and measurement requirements. This is not approval to promote
an unmeasured candidate or to bypass resume chains.

Reviewed the frozen [design](../../design/generator-delegation.md),
[source audit](source-audit.md), [plan](plan.md), current IR/frame/observer
contracts and selected source at
`/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/round51-golden`,
commit `0e5e61f4c6bc820f2dd1a5f9ee6f7f1768834254` (R). Canonical checkout C's
runtime was not used as the selected source. Applicable guidance and the
[CPython AI policy](https://devguide.python.org/getting-started/ai-tools/) were
read. This review changed only this report; no runtime edits, builds, native
execution or tests were performed. Source references below are relative to R.

## What should remain

Keep DELEGATE's existing three-home window and completion geometry; a real
SUSPEND inside the synthetic throw region; a real RESUME_DELEGATE with an
absolute target; and real frames, exception links and recursion accounting.
A normal send failure and an injected exception cannot share one undifferentiated
DELEGATE recovery PC. The proposed adjacent real words preserve this distinction
without debug-coordinate lookup or an allocated suspension record.

Use the bounded fusion model for the first candidate. Cold CFG analysis may
retain a logical yielded value in B+2 followed by SUSPEND, provided the generated
fast transfer proves equivalent final owners and has no intervening observable
stop. A new general third-edge framework is not required merely to represent
that implementation. Name the suspended transfer contract explicitly, and do
not claim B+2 is physically populated on its fused route. If an actual callback
must see an intermediate state, materialize the ordinary route before it.

## Findings

### 1. Blocking: make shortcut admission a pure, complete decision

The draft lists the right hazards but leaves the executing predicate and the
instrumentation-mismatch exit unspecified. An actual plain SUSPEND handler is
not sufficient: the child can install monitoring while the parent's executing
image still carries its previous handler. Synchronization can allocate or fail;
it cannot be inserted while pretending the parent has SUSPEND's input owner.
`Python/bytecodes.c:176` updates instrumentation at RESUME, and
`Python/ceval_macros.h:793` gives the existing guarded skip. Neither automatically
makes a new yield-side bypass safe.

Freeze a nonescaping `can_propagate_delegate_yield` contract. It must reject a
pending redirect, non-generator parent, wrong active continuation, stale
executing image/instrumentation epoch, active relevant observers and a nonempty
transfer cell. The immutable constructor proof supplies the static triple facts;
the fast path must not scan source metadata or perform a general instruction
decoder. Recheck per parent after any existing hook. A conservative first
candidate with all observation disabled for fusion is acceptable, including
LINE, INSTRUCTION, PY_YIELD, tracing/profile, source handoffs and DTrace/LLTrace.
Checking only PY_YIELD is not acceptable.

On mismatch, first pass the held owner to the ordinary recipient transaction and
resume the real continuation. If stale instrumentation needs synchronization
there, define a cold landing operation with the value already in its real home,
the physical site published and an explicit failure recipe. Merely saying
“dispatch the stale plain word” does not establish newly enabled event behavior.
Do not eagerly synchronize inside the pure predicate. Use existing version and
image authority; add no chain cache. For free-threaded builds, disable this
shortcut unless the corresponding image/epoch synchronization is proved.

### 2. Blocking: establish actual generator ownership before suspension

Matching A/B/C/D and code flags is not enough to justify converting an arbitrary
parent to a `PyGenObject`. `_YIELD_VALUE` currently asserts only that the frame
is not interpreter-owned before using `_PyGen_GetGeneratorFromFrame`
(`Python/bytecodes.c:1708`). That is not a proof that a publicly constructed
code object executes the word in a generator-owned frame.

For new DELEGATE/SUSPEND sites, validate the admitted flags and entry shape
alongside slot extents and triple ingress. More importantly, require actual
`FRAME_OWNED_BY_GENERATOR` before DELEGATE activation and dispatched SUSPEND,
with a safe `SystemError` for malformed thread/interpreter-owned execution,
before any generator conversion or owner movement. The direct predicate also
rejects a non-generator parent. A constructor flag alone is never runtime
authority.

This bounded runtime guard is an acceptable first implementation; a new
all-path frame-context analysis is not a blocker. Constructor checks still reject
locally invalid triples, recipients, targets and exceptional ingress, while the
runtime guard catches paths whose ownership capability was not proved statically.
Mutations of `co_flags`, RETURN_GENERATOR/entry control and normal/exceptional
entry must exercise this boundary without assertion failure or invalid container
conversion. Current `Objects/codeobject.c:1190` validates completion windows and
handler ownership, but `:1340` onward does not establish generator ownership.
Do not make every existing opcode's arbitrary mutation safety part of this work.

### 3. Blocking: specify the terminal owner's phase and hook boundaries

`IR_SUSPEND(value, site, recipient, resume, kind)` is a useful interface only if
its responsibilities are exact. The incoming canonical owner is already detached
from the outgoing home. A fused parent's B+2 is already NULL; a dispatched
SUSPEND takes and clears its actual B before entering the primitive. The primitive
must not take that home again. It assigns the actual resume PC explicitly; the
current `frame->instr_ptr++` would be wrong when entered from DELEGATE.

Keep one owner through the loop and give it exactly one terminal consumer:
ordinary recipient publication/discard or a documented error cleanup. Preserve
current/exc links, recursion exit, return slot, previous-link clearing and the
release-store of suspended state for every frame. No allocation, retirement or
fallible observer may occur in an intermediate parent with SUSPEND recovery
selected and its expected outgoing home absent. The ordinary callback path must
retain the value in the real home until its existing event transaction takes it.

Do not label all retained hooks nonescaping. `LLTRACE_RESUME_FRAME` calls
`maybe_lltrace_resume_frame` (`Python/ceval.h:281`), which queries globals and may
print objects; `DTRACE_FUNCTION_RETURN` resolves names and a line
(`Python/ceval.c:1274`). Retain existing hook ordering and coherent PCs, and
reevaluate eligibility after a hook that can change state. Conservatively
excluding the observed route is simpler than proving those hooks safe inside
an unpublished fusion. Verify last-owner finalizers at ordinary recipient
retirement, GC during callbacks, escaped frame inspection and same-PC pending
redirects. An owned C value remains live without a new GC ledger, but must never
be lost or advertised as a frame owner that is absent.

### 4. Blocking: give RESUME_DELEGATE explicit success, error and redirect exits

The absolute CD encoding fits. The dangerous detail is selecting CD too early.
Generated error routing commonly uses `next_instr - 1`; if it is changed to the
DELEGATE target before instrumentation/periodic/PY_RESUME work succeeds, a failure
can select an unrelated exception recipe. Keep the real RESUME_DELEGATE site and
its ordinary next-word cursor until successful completion of all fallible work.
Only a successful, nonredirected resume selects CD. A committed redirect,
including intent returning to the same address, wins over the encoded target.

Extend both dispatched and skipped-resume paths together. Preserve existing
location/depth flags, epoch/TLBC/Emscripten checks, poll placement, DTrace entry
order and hook-induced cursor changes. In particular, native `send` entry,
exact-child entry, close's depth-one shortcut and injected `throwflag` entry are
separate consumers. The throw path must continue selecting the immediately
preceding SUSPEND without running RESUME_DELEGATE first
(`Python/ceval.c:2298`). Test an error or redirect in PY_RESUME separately from
an injected throw and a provider/send failure.

### 5. High: far completion needs distinct physical and normalized targets

The proposed layout is valid: DELEGATE at s, SUSPEND at s+1,
RESUME_DELEGATE at s+2, adapter at s+3; D=2 reaches that adapter. Keep this.
However, do not extend the existing `instr_length` trick by treating these four
words as one instruction. Current `Python/instrumentation.c:99` groups two
contiguous jump helpers after SEND. Here SUSPEND and RESUME_DELEGATE are real
semantic sites with independent exception/source/events.

Freeze separate concepts: the physical completion continuation is the adapter;
the normalized semantic done target is its CD. The real adapter remains eligible
for INSTRUCTION observation and actual dispatch. Use a dedicated exact-shape
normalizer shared by completion/source/branch readers; do not recursively chase
arbitrary jumps. Keep the adapter's EH and source owner phase consistent with
already committed completion, and keep it outside the synthetic SUSPEND throw
region. Constructor checks and assembler layout must agree on its placement and
incoming edges. Test near/far completion under LINE, INSTRUCTION, completion
callback failure and CLEANUP_THROW, including a completion destination that
aliases an input or is discarded.

### 6. High: generic providers need the same outcomes, not the fast assumptions

The first exact-generator specialization is a sensible scope. Generic send,
virtual iteration and async-generator asend can publish B+2 and dispatch SUSPEND.
They must still complete through the DELEGATE kind and correct error site, never
replay a provider after owner transfer. Keep `_PyFrame_SetReturnValue` for actual
resume/publication transactions, and `complete_step_impl` for completion order.
PEP523 remains an admission boundary; there must be a coherent native-provider
route when it is installed or changes during execution.

Current SEND also has SEND_VIRTUAL and SEND_ASYNC_GEN specializations
(`Python/bytecodes.c:1574–1704`). Removing those products is a potentially material
performance change, not free simplification. Document whether they are retained
as provider refinements or replaced by generic provider execution, and measure
that choice. No additional opcode family is required to retain their semantics.

Keep `_gen_throw` and `gen_close` protocol behavior on their existing paths.
A custom `send` error belongs to DELEGATE; delegated `throw` returning a value
leaves suspension intact; thrown StopIteration takes CLEANUP_THROW's four-home
completion. Missing/raising `throw` and `close`, yielding during GeneratorExit,
async-generator cleanup and coroutine reuse need distinct cases. Preserve
single StopIteration reporting and `gi_yieldfrom`/`cr_await` through the actual
B+2 recipient. Do not interpret the return-kind thread flag as persistent
per-parent delegation state.

### 7. High: bind the logical fusion proof to the generated executable body

The draft's alternative between a bounded fusion and a new general outcome can
be settled now in favor of bounded fusion. Define and verify its final state:
receiver/cursor retained, B+2 NULL, incoming recipient B+2, saved PC s+2, suspended-
delegate state, removed current/exception/previous links and one held value.
The logical yielded edge may populate B+2 for allocation analysis only because
the immediately consuming SUSPEND is certified and unobservable on this route.
Do not expose that logical row as a physical POST observation.

Retain frontend END_SEND/CLEANUP_THROW stack effects until allocation and
completion lowering consume them (`Python/assemble.c:1144`). Make triple
recognition semantic and reject extra ingress; equality of line/column is not
part of the proof. The canonical input proof currently matches the actual yield
body (`Tools/cases_generator/ir_generator.py:4706`). Port it to the terminal
primitive plus its bounded emitter, with mutation tests for missing nulling,
changed PC/recipient, inserted callbacks and weakened observer/redirect guards.
Opcode names or a declaration flag alone cannot authorize canonical transfer or
fusion. This may be implemented as a small dedicated proof, without expanding
the whole observer CFG into a general state machine.

### 8. Medium: require evidence that the intended mechanism earns its checks

The claimed work saved is modest but real: per intermediate yield, one
publication/reload path and one SUSPEND dispatch; on resume, the old jump routing.
All entry/state/link/recursion work remains. Ordinary shallow generators and
FOR_ITER callers may pay new failed-eligibility checks without any propagation.
The reported SEND/YIELD sample shares cannot predict the net gain.

Put the cheapest parent-kind/continuation checks first, avoid decoding immutable
words or source data on every ordinary yield, and inspect emitted native code for
an iterative transfer rather than repeated calls. Use a deep exact-delegation
case to prove the fused mechanism executes, plus unchanged generators/coroutines
and shallow/plain/native-provider cases to expose overhead. These mechanism
checks supplement the agreed unchanged-workload timing; they do not replace it.
Promote only on repeated target improvement and acceptable broad guards. If the
candidate loses, retain the design/evidence, not an unmeasured claim that the
architecture will eventually pay for itself.

## Bounded implementation interfaces and order

Agree these interfaces before splitting workers:

1. One constructor-verified triple/far-adapter description, including its two error contexts,
   plus explicit runtime generator-owner guards at activation/suspension. No per-frame site record.
2. One pure `can_propagate_delegate_yield` operation, reading current dynamic
   authority and returning either a verified actual SUSPEND word or failure.
   No callbacks, allocation, synchronization, owner movement or frame mutation.
3. One generated terminal `IR_SUSPEND` loop with a canonical owned value and an
   explicit real suspension site. It performs each frame's existing suspension
   transaction and then either repeats or publishes exactly once.
4. A literal DELEGATE entry/completion kind using existing owner transactions;
   one RESUME_DELEGATE operation with delayed target selection; one exact far-
   adapter normalizer distinct from instruction grouping.

The transfer structure should be equivalent to this pseudocode, not a second
execution engine:

```text
value := take canonical outgoing home after dispatched SUSPEND observation
site := actual SUSPEND
repeat:
    suspend current generator at site, keeping value owned
    restore actual caller and existing hook order
    next_site := pure admission check on this caller and its continuation
    if next_site exists:
        site := next_site             # B+2 is already NULL; do not take it
        continue
    publish value through the ordinary recipient transaction
    select its actual continuation, respecting pending redirect
    run any required cold observation synchronization at coherent homes
    dispatch
```

Synchronization failures and hook-induced changes need their explicit existing
owner/error transaction; the pseudocode does not grant them a new generic
cleanup path. Generated implementation must retain the canonical proof and
avoid hidden recursion.

Implement the producer, constructor, ordinary dispatched semantics and all
consumers first as a coherent unpromoted checkpoint; then enable the bounded
fusion using the same terminal transfer. This is sequencing, not a request to
ship a rename-only milestone. Sol should review the contracts and actual diffs,
then the coordinator should run focused release/debug qualification, native
mechanism inspection and the existing matched performance gates. Resolve these
findings in one design update; no further Astra loop is required.
