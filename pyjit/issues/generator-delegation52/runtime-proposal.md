# Runtime slice

The experimental round 52 candidate implements the revised three-word contract.
The [Astra review](astra-review.md) has a partial proof obligation described
below. Both candidates are rejected on performance; selected golden51
`0e5e61f4c6b` remains unchanged. See the [report](report.md) for final results.

## Instruction and shared interfaces

Frozen instruction names: DELEGATE, DELEGATE_GEN; SUSPEND,
INSTRUMENTED_SUSPEND; RESUME_DELEGATE, INSTRUMENTED_RESUME_DELEGATE.
DELEGATE has SEND's A/B/C/D geometry; SUSPEND has YIELD_VALUE's geometry;
RESUME_DELEGATE uses A for resume flags and CD for an absolute target. Its
format requires a deliberate override: unlike ordinary jumps, the operation
argument remains A while the separately named target is CD. Compiler and runtime
must update encode/disassembly/metadata consumers to this explicit distinction.

Consumer supplies `_Py_Specialize_Delegate` (exact generators/coroutines only)
and `_PyIR_PrepareSuspendLanding(tstate, frame, site)` in instrumentation.c.
The latter returns 0/-1 and synchronizes a stale image only after value
publication. Runtime owns its header declaration. Compiler supplies literal
`_PY_IR_DELEGATE` alongside SEND/FOR step kinds and
`_PyIR_DelegateDoneTarget(words,count,site,physical)` for exact far-adapter
normalization. Neither helper uses source positions for execution authority.

## Ordinary coherent semantics first

DELEGATE retains the existing three-home window, PEP523 and exact type guards,
created/non-None legality, executing exclusion, generic virtual cursor provider,
ordinary `_PyValue_Send` provider and completion transactions. Exact-only
specialization replaces SEND_VIRTUAL/SEND_ASYNC_GEN products for the new family;
the generic provider routes retain their semantics. Their possible throughput
cost remains an explicit measurement question.

Before generic or specialized DELEGATE admission, require actual
`frame->owner == FRAME_OWNED_BY_GENERATOR`; otherwise raise SystemError before
child state/owners change. Dispatched SUSPEND performs the same guard before
TAKE or generator-container conversion. This is a capability guard, not a
substitute for constructor triple/flags/entry proofs.

DELEGATE's exact entry uses the existing canonical input detachment and frame
link/publication transaction, with `_PY_IR_DELEGATE` treated as a send kind.
The parent recipient is B+2, its real continuation SUSPEND, and its executing
PC remains DELEGATE. Generic PYGEN_NEXT initially publishes B+2 and dispatches
actual SUSPEND. Completion calls the shared completion implementation with the
new kind and physical done PC. `_PyEval_CompleteGeneratorReturn` explicitly
admits immutable DELEGATE as well as SEND/FOR; no CALL cost is introduced.

`gen_close` recognizes RESUME_DELEGATE and its instrumented twin when reading
resume flags/depth. `_PyGen_YieldFromRef` retains return_slot-2. Throw and close
protocol forwarding remain unchanged. Saved PC is RESUME_DELEGATE and injected
error uses its preceding real SUSPEND recipe. No chain shortcut handles throw.

## Terminal suspension interface and owner phase

`IR_SUSPEND(value, site, recipient, resume, kind)` is terminal. `value` is one
canonical owning value already detached from the outgoing home. Dispatched
SUSPEND takes and clears its B home before entry. A fused parent's B+2 is
already NULL and must not be taken again. `site` is the actual real SUSPEND;
`resume` is its explicit following real continuation; `recipient` is the next
incoming home; `kind` distinguishes plain/delegated suspension.

The emitter generates one iterative transfer loop, preserving current yield
ordering: select the suspension site/explicit saved resume PC, DTrace return,
restore exception link, leave recursion, install incoming recipient, update
profiled frame, restore actual caller/current frame, clear child previous,
set generator-return kind, release-store suspended state, reload caller slots,
select caller return continuation and run existing LLTrace resume hook. The
current frame and all owner links are coherent at every retained hook.

No direct parent is admitted unless DTrace/LLTrace/observation are disabled for
that fusion. The loop retains its single owner through every iteration. On its
terminal boundary it calls the existing canonical recipient transaction exactly
once, which consumes the owner even on discard/pending redirect. No home is
advertised as holding this value while it exists only in C.

After publication, choose the winning continuation from the recipient
transaction; do not restore a cached PC after pending redirect. If the winner
is the actual SUSPEND continuation, call `_PyIR_PrepareSuspendLanding` with
parent current/coherent and value already in its home. It publishes actual site
and synchronizes epoch mismatch under existing tracing rules. On failure,
next_instr=frame->instr_ptr+1 preserves SUSPEND's input-owner exception recipe;
on success dispatch the refreshed real site, including newly enabled wrappers.
A committed redirect wins, including same-PC intent. The helper cannot grant
fusion and performs no owner movement.

## Pure direct-transfer admission

`can_propagate_delegate_yield` returns the actual admitted SUSPEND word or NULL.
It never allocates, calls Python, synchronizes, mutates frames or moves owners.
Order cheap rejections first:

1. Reject interpreter/thread/non-generator parent, pending redirect or a
   continuation whose executing handler is not plain SUSPEND/delegated kind.
2. Require exact active continuation/site relation, matching B+2 incoming
   recipient, and NULL transfer home. Constructor proves immutable triple and
   all ingress; the runtime does not decode source metadata or scan a chain.
3. Reject stale code/image epoch, PEP523 changes, tracing/profile hooks,
   any code monitoring allocation, escaped frame object/source handoff,
   active DTrace hooks and debug LLTrace. Conservative frame-object rejection
   covers its observer cursor/handoff state without new per-frame bookkeeping.
4. Disable fusion entirely under Py_GIL_DISABLED. Recheck once for every parent
   after existing hooks. A failed predicate leaves owner/frame state unchanged.

The exact eligibility checks and generated body implement a bounded transfer
contract; the backend emitter remains a trusted compiler boundary. Plain/shallow yields should reach cheap rejection
before immutable decoding or unrelated frame inspection. The first candidate
may conservatively exclude all debug LLTrace-capable fusion if proving disabled
hook detection is more complex than its value.

## Delayed resume exits

RESUME_DELEGATE keeps its actual site and ordinary site+1 error cursor through
instrumentation refresh, existing periodic policy and optional PY_RESUME event.
It snapshots redirect generation before fallible work. Instrumentation refresh
retries the real resume site; failure leaves site+1 for error attribution.
Only success with unchanged real PC and generation selects CD. A changed PC or
generation dispatches the committed actual PC, including same-address intent.

Its fallible sequencing is implemented in a small shared runtime helper returning
`_PyIRStepResult`, with a thin terminal emitter for plain/instrumented variants.
The helper performs no owner movement. It preserves existing tracing rules and
periodic policy; it does not poll every delegation level. IR_SKIP_RESUME covers
the uninstrumented new operation only after the same epoch/eval-breaker,
TLBC and Emscripten guards. Native evaluator entry and exact-generator entry both
use that shared macro; DTrace entry remains in the existing position. Instrumented
RESUME_DELEGATE always dispatches. Throw entry bypasses resume unchanged.

## Proof and validation

Mechanical checks bind the DSL canonical transfer producer and C eligibility
predicate, with mutation negatives for those checks. IR_SUSPEND is registered
as terminal/frame/sync/escape with an explicit suspension effect. The backend
terminal emitter is trusted compiler implementation: selective generated-output
checks and source/native review, tests and GDB evidence support it, but no
general emitter AST/token mutation gate checks every terminal state write.
Astra finding 7's stronger binding obligation is therefore partial. Peer and
native qualification are required for backend edits; bounded post-emission
contract verification is future work. See the [cold review](cold-review.md).

Golden51 remains selected. Initial candidate `58f9ea6cfee` lost six paired
comparisons by 4.37% for generators, 4.66% for coroutines and 2.67% for
DeltaBlue. Followup `e851719` passes native wrapper-first placement inspection,
but six-pair confirmation loses by 0.95%, 11.30% and 11.85% respectively.
Both candidates are rejected. The [report](report.md) records final qualification
and performance limits.

## Peer resolution

Compiler and consumer peers accept the shared interfaces. Specialization uses
`_Py_Specialize_Delegate(PyValue receiver_st, _PyInstr *instr)`, matching current
SEND's actual value type. Compiler edits only the enum literal in
`pycore_ceval.h`; runtime owns other declarations. Consumer disassembly separates
A's flags from CD's physical jump target.

At the retained LLTrace hook after restoring a caller, that caller still has
its pending-child DELEGATE PC and NULL B+2; it has not selected SUSPEND recovery
or claimed SUSPEND's input owner. This is the same coherent held-result phase
as current yield before recipient publication. A hook may inspect/change that
state; the subsequent pure predicate rechecks it. If observation is active,
ordinary publication materializes B+2 before selecting SUSPEND and invoking
cold landing synchronization. No hook runs with SUSPEND recovery selected and
its expected input absent. The current generator's DTrace hook retains the
existing saved-resume-PC ordering. Active DTrace/LLTrace excludes parent fusion.

## Entry observation repair after native debug qualification

Native INSTRUCTION qualification exposed a stale-epoch entry boundary: an outer
INSTRUCTION wrapper around saved RESUME_DELEGATE could run before the inner
resume handler refreshed instrumentation. The coherent fix uses one
`IR_FINISH_RESUME_ENTRY` boundary for native start_frame and exact generator
entry. Incoming owners and activation links are already committed. It snapshots
real PC/redirect generation with an actual ownership guard, retains DTrace then
LLTrace order, prepares the selected continuation after the last hook, and only
then performs eligible resume skipping. Fresh ordinary _PUSH_FRAME entry stays
unchanged.

Private `prepare_resume_entry` retains a redirect decision, selects the winning
real PC, and synchronizes only when the actual executing operation is an outer
LINE/INSTRUCTION wrapper over immutable RESUME_DELEGATE. Plain and inner
instrumented resume handlers already synchronize before events and need no
duplicate semantic decode/epoch load here. Synchronization performs no owner
movement, CD selection or PY_RESUME callback. Success/error recheck PC and
generation; errors use the winning real site+1, and REDIRECT prevents cached
skip authority, including same-PC intent. Compiler and consumer peers accepted
this contract. Subsequent native inspection confirmed wrapper-first placement
in the entry followup; final qualification remains coordinator-owned.

The no-handler-local-address fixture also gained one narrow exception for the
always-inline conservative instrumented publication: exactly one token sequence
`temp = _PyValue_Publish(&temp); PyValue ir_yielded = temp;`. Occurrence-limited
subtraction preserves rejection of an additional same-line `opaque(&temp)`;
different helpers, arguments, handlers or duplicate publication remain errors.
No runtime publication proof was weakened. Both focused host fixture tests pass.
