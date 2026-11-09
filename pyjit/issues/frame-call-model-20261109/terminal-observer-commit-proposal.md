# Generator-owned terminal outcome commit

Proposal before source changes. This is the positive provider contract for the
scoped POST observer design, not a new runtime instruction or handler alias.

## Source evidence and existing mechanisms

Generated INSTRUMENTED_FOR_ITER assigns owned step.value to next, calls its LEFT
observer, then publishes cursor/window2 and moves A. OR_POP similarly holds res
through its observer then stores A. Their callback error exits bypass these
stores. The coordinator's filtered weakref probe confirms FOR error strands one
C reference per result on candidate and upstream; control results collect.

IRStack already tracks input ownership, dead variables, defined outputs, window
layout and final publication. Storage.push_outputs rejects undefined results and
live unconsumed inputs; IRStack.final/flush plus generate_handler's final mode
implement normal output placement. Mode STORE_RESULT/window performs a later
window-to-A move and release_displaced runs after that. Existing terminal markers
IR_COMPLETE_STEP and IR_RETURN_TO_CALLER already stop generation and dispatch
without a second generic epilogue. Reuse that structural pattern.

A blind storage.flush before INSTRUMENTED_JUMP is insufficient. It may leave a
folded local unpublished until final, use the wrong window-to-destination mapping,
or execute NoDst close before the owner/continuation commit. It also permits an
old epilogue to store again after redirect. The barrier must be FINAL and own
all remaining terminal work.

## One terminal convention

Use the positive IR_OBSERVE_COMMITTED_BRANCH generator-owned TERMINAL marker
for certified register-handler paths. Its source/destination/event operands
keep their existing meaning. INSTRUMENTED_JUMP remains a nonterminal legacy
provider until its complete outcome is migrated; it receives no new POST authority.
The marker means: this path's operation is complete; every selected output is
defined; remaining nonpeek inputs have already been retired or explicitly
transferred; no semantic statement or uop follows on this path. Generation fails
closed if these facts cannot be established. The emitted C callback is still an
ordinary helper call, not a dispatched cleanup instruction.

Factor one emit_final_outcome routine from existing handler-final publication.
Both ordinary epilogues and terminal observation use its actual slot_layout,
store_mode and generated output_kind; no hand-maintained output mapping. The
observed form defers callback-capable retirement. It must:

1. Push/check exactly this path's defined outputs. Preserve through/peek homes;
   clear consumed scratch homes without touching named-local borrows. Reject
   undefined output, unsupported dynamic array or unknown owner transport.
2. Publish updated owner window/cursor and actual destination. For folded local,
   install the new value and hold the displaced owner in C. For NoDst, detach and
   hold the result in C rather than calling CLOSE inside a store macro. For a
   temporary publish it once. Destination/input aliasing must use the same checked
   detach-before-overwrite contract as ordinary output publication.
3. Publish actual return_pc/continuation. Keep instr_ptr at the semantic site
   during the callback for existing f_lasti/error attribution; typed phase records
   the exact physical next and committed output facts. No guessed source PC.
4. Invoke observation under POST_COMMITTED. On success without redirect publish
   the real continuation PC; on redirect use the committed PC/generation. Then
   retire detached/displaced/NoDst C owners with no later old-slot writes. Reload
   actual PC after supported closing reentry. On error keep the committed owners
   and exact semantic EH contract; close every unpublished C owner exactly once.
5. Dispatch or take the common error adapter. Mark this branch unreachable to
   generation, so ordinary uop/end-handler output stores cannot execute again.

For NoDst/folded local, avoid publishing an intermediate window result merely to
clear/move it if the actual format's first-result mapping permits direct A store.
Other window outputs remain in their declared homes. Existing metadata proves
which output is A versus window tail; do not infer from STORE_DST capability.
The initial implementation can retain harmless already-required canonical window
stores, but cannot leave an extra live alias owner or a dangling temporary.

## Path and declaration proof

Storage already marks assignments in each parsed branch. Invoke push_outputs at
the terminal marker, not at arbitrary source token occurrence. Both OR_POP arms
define res (retained owner versus explicit NULL); FOR body defines next and its
updated cursor. Undefined completion/body outputs from the other outcome must
never be flushed. A marker reached from a nonfinal semantic uop or followed by an
owner operation is rejected until that provider is structurally rearranged.

Destination cardinality becomes one commit PER REACHABLE PATH, rather than the
current shared dst_stores list counting source sites. Two mutually exclusive
terminal arms may each commit once; two commits on one path remain an error.
The generator must not weaken its existing single-displaced-owner invariant.
Use hygienic injected identifiers: scan/reserve declarations or generate unique
names, with mutation tests for DSL locals matching helper result names.

All residual C owners must be accounted for: output moved into home, original
input already retired, preserved peek still frame-owned, or explicit deferred
retirement record. Unknown C-held values refuse generation, not a runtime test.
Callbacks inspect already-published locals. Detached owners remain counted C
roots through callback/error; GC need not see an invented frame slot.

## Provider migration, not a permanent exclusion list

Consuming POP_JUMP and fused compare/contains/is forms have no unpublished
result at their selected observation and use the positive zero-output terminal marker.
OR_POP moves its selected result into the terminal commit. FOR body commits its
cursor/result/local before LEFT. Completion RIGHT/SEND continue using their
existing CompleteStep positive outcome, sharing the same cold observer record.

Legacy macro users that currently close an input or assign an output AFTER
INSTRUMENTED_JUMP must move that semantic work before the terminal marker, or
explicitly transfer its owner into the barrier's deferred record. For example
POP_ITER has an iter close after the marker, and legacy END_SEND assigns val and
closes receiver afterward. They cannot silently use a generic flush and claim
committed state. Their eventual release-word elimination is a separate compiler
lowering goal; this convention neither renames them nor adds new cleanup words.

An alternative split compute+terminal-observer microop is useful where existing
DSL branch structure cannot prove final outputs. Both microops belong to ONE
semantic handler and emit ONE terminal C sequence; no additional IR word or
dispatch is allowed. Prefer a single marker with path-sensitive storage proof
when current parsed outputs already suffice (FOR and OR_POP).

## Error and metadata contract

The observer error occurs AFTER output commit, unlike a compute failure before
it. Shared constructor/debugger ownership consumers need the exact callback
outcome and must not erase pre-error requirements with success definitions.
Validate the actual encoded handler's ordinary/source EH recipes under both
compute PRE and observer POST states, using shared physical output/retirement
facts. Existing completion-only validation is not a blanket branch proof.
No second unwind mechanism or nearest-word recipe guessing is introduced.

Last-owner displaced-local or NoDst cleanup happens after state and continuation
are valid. This is a chosen safe register ownership boundary; exact CPython
reference-release timing is not a Python language requirement. A callback's
newer redirect must win over the terminal adapter and its later closes.

## Implementation ladder and required evidence

First host generator tests: FOR fixed-window output/local/NoDst; OR_POP both
arms; consuming branch no-output; path-local one commit vs duplicate same path;
undefined output, live owner, unknown dynamic array, wrong final uop, injected-name
collision and suffix-after-marker negatives. Assert emitted output publication
precedes observation and no slot/dst stores follow observer/redirect/error.

Native ordinary regressions: filtered FOR LEFT callback error results become
collectable after gc (no prompt-finalizer claim); OR_POP retained/drop behavior;
folded local callback sees new binding; NoDst result, cursor and receiver state;
callback raises, setter redirects, same-PC intent, and displaced-owner reentry
preserve committed target/locals. Keep original monitoring/trace expectations.
Then constructor negative owner recipes and broad observer/generator tests.

A owns generator/bytecodes/runtime terminal owner adapter; B owns shared cold
outcome/EH/debugger consumers. Exact helper/deferred-owner schema requires B/root
peer before code. Root owns regeneration, all native runs and measurements.

## Concrete first interface for independent review

The first implementation changes generated instrumented terminal paths only.
Factor Python emission of the existing final-output sequence, with its ordinary
mode preserving the exact current output text. An observed mode substitutes
nonclosing destination transfer and terminates the path. The coordinator must
compare regenerated output: unrelated ordinary handlers must be byte-for-byte
unchanged. This does not change the public instruction set or ordinary final
store macros.

Use a generated C scope with hygienic names and these roles:

```
PyValue ir_terminal_destination_owner = PyValue_NULL;
_PyIRObserverResult ir_terminal_observation;
```

`ir_terminal_destination_owner` owns either the displaced local value or the
ignored result, never both: local and NoDst are disjoint destination categories.
A temporary destination contributes no deferred owner. Existing `ir_displaced`
may supply this owner only after the path proof establishes there was no earlier
separate destination commit. The output publication adapter captures the old local owner and installs the new
value in one callback-free block; for NoDst it detaches the result without closing it.
A window result is first published through the existing canonical window mapping,
then moved/detached according to actual A mapping. Other window outputs remain in
their homes. Publication itself uses existing exact numeric canonicalization;
its conversion does not invoke arbitrary Python callbacks.

An input that the provider must retire after commit uses an explicit DSL ownership
transfer marker, provisionally `IR_DEFER_CLOSE(owner)`. It removes that one known
owner from the generator's live set and places it in a hygienic C local. It emits
no close until the terminal adapter. This is needed for POP_ITER's receiver and
legacy END_SEND's receiver while those definitions remain. The marker is not a
runtime instruction. Deferred owner count is statically determined per path;
close code is emitted directly, not interpreted from metadata or a runtime cleanup
program. Duplicate transfer, unknown owner origin, live untransferred input or a
second destination owner rejects generation. A copied C spelling of an owner
must be proved an alias/transfer, not counted as another owned reference.

The cold observer API is provisionally:

```
_PyIRObserverResult _PyIR_ObserveCommittedBranch(
    PyThreadState *tstate, _PyInterpreterFrame *frame,
    const _PyIRWord *site, _PyIRWord *next, int event,
    const _PyIRObserverSpec *spec);
```

The result is typed CONTINUE/REDIRECT/ERROR plus actual `next`; it owns no Python
value. The immutable spec identifies the exact post outcome and initial redirect
generation. The helper scopes it per actual callback together with observed frame
and event, restores that scope before callback-result disposal, and retains
existing event/tool order. It neither closes provider owners nor writes their
homes. Existing CompleteStep calls the same observer transaction with its own
already-established outcome; ordinary CALL return is unaffected.

Generated terminal order is: transfer all outputs and selected deferred inputs;
set `return_pc = physical_next`; observe with `instr_ptr` still at the semantic
site; select the committed continuation (or the semantic error site); then close
the destination/deferred owners exactly once. Set the selected `instr_ptr` before
the first potentially reentrant close. After closes, read the actual frame PC,
never restore a cached continuation, and emit no old output/window stores. A
redirect followed by another redirect from a close therefore keeps the latter.
The adapter's `_PyValueTemps` are exact numeric input boxes by the existing
`pycore_value.h` contract; they are released too, but do not authorize arbitrary
owner transfers or require a new callback ownership protocol.

On observer error, retain the exception while closing deferred owners using the
existing exception-preserving finalizer rules. No-output branch errors retain
consumed-input state; FOR body errors have item/cursor/destination published.
The shared constructor proof must validate these POST EH requirements separately
from compute-PRE requirements before these handlers are admitted. This closes
the confirmed leaked C item rather than suppressing the callback error.

One additional audit obligation is explicit: source callback errors can install
an activation-owned source-error recipe before later deferred closes run. If a
supported close-time setter commits a newer target, that recipe must not override
the committed target's EH. The revised commit-time invalidation contract below
handles this before reentry, rather than clearing a potentially replaced recipe
after retirement. Do not rely solely on the frameobject display cursor.

The first native regression is the coordinator's filtered FOR LEFT error case:
three produced mortal values become unreachable and collectable after the calls,
with no assertion about immediate destruction. Add OR_POP error/redirect and
folded-local callback publication tests. Existing consuming branch and generator
completion tests must remain unchanged. Scoped POST setter admission is a second
consumer step after provider publication and encoded EH proof are established;
it is not needed to reproduce or repair the leak itself.

### Exception and deferred-close completion

Use the established exception-preserving cleanup pattern: when observation
returns ERROR, take the raised exception with `PyErr_GetRaisedException()`, close
all explicitly transferred owners with no exception pending, then restore that
same owned exception with `PyErr_SetRaisedException()`. Destructor failures retain
their normal unraisable handling; the callback's exception remains the error to
unwind. This operation neither reconstructs old inputs nor changes exception
links. CONTINUE/REDIRECT paths close with the ordinary no-error state.

Independent review rejects post-close matching by activation/frame/record:
close-time nested observation could install a new recipe with the same tuple.
Redirect generation does not identify a recipe installation. That first design
is withdrawn before implementation.

The revised proposal invalidates old recipes AT the successful setter commit,
before any detached-owner callback. Add a cold ceval API that walks the existing
evaluation-activation chain and clears only pending entries whose source-error
frame is the exact redirected target frame. All such recipes describe a phase
before this committed redirect; they must not override its new real target EH.
Entries for other frames are untouched. Perform this operation in the existing
callback-free target/cursor/generation publication block. A recipe installed by
any subsequent close-time callback remains intact: there is no post-close clear.
A failed setter does not clear recipes. This needs B/root peer admission; it adds
no activation counter, interpreter-frame field or second unwind mechanism.

Defined NULL is a real output in OR_POP's drop arm. Its assignment remains in the
path storage state and is committed as NULL; absence of an assignment is rejected.
An owner transferred into a published home is not also closed by the adapter.
All deferred locals are nulled/detached from the adapter's own ownership state
before each close, so a nested observer cannot cause the same C owner to be
retired twice. No close helper may restore a cached semantic PC afterward.

## Provider census required before source admission

The macro name is not proof of terminality. Actual current DSL call sites are:

| Provider | Current callback boundary | Required structural treatment |
| --- | --- | --- |
| Instrumented forward/backward JUMP | No pending result | Terminal commit after any poll/semantic work. |
| POP_JUMP bool/None/None-negated | Input already consumed or dead | Zero-output committed outcome, with exact taken/fallthrough retirement facts. |
| Fused `_INSTRUMENTED_JUMP_IF_COND` | Boolean dead; previous compare suffix must also be complete | Prove the entire composed handler chain ends here, not only this uop body. |
| OR_POP arms | Selected result held in C | Publish defined owner or explicitly defined NULL before observation. |
| FOR body | Cursor/item not fully published | Publish complete body outcome and destination before LEFT. |
| INSTRUMENTED_POP_ITER | Receiver close follows marker | Detach receiver into explicit deferred owner before terminal observation. |
| `_MONITOR_END_ASYNC_FOR` + `_END_ASYNC_FOR` | First uop observes; second still matches exception, retires inputs or reraises | Current marker is NONTERMINAL. A blind replacement would silently skip semantic work. Restructure the full success/error outcome or reject terminal generation; preserve the exception/reraise event contract. |
| INSTRUMENTED_END_SEND | Calls `_Py_call_instrumentation_jump` directly, then assigns result and closes receiver | Direct-helper site must migrate to the same committed-outcome adapter; replacing marker calls does not certify this handler. |
| INSTRUMENTED_END_FOR | Calls `monitor_stop_iteration` directly, then closes an input | The input remains frame-owned during callback, unlike the leaked FOR C result. Audit redirect before the later consume; an error alone need not leak it. Classify this as a PRE observation until a complete outcome proves otherwise. |
| CompleteStep PRE/POST | Existing owned completion protocol | Supply exact phase spec to shared callback scope; keep PRE old-window and POST detached-window facts distinct. |
| PY_YIELD event | Value still frame-owned before switch | Dedicated YIELD_READY transaction; never terminalize the following suspension transition after a redirect. |

Also inspect the macro implementations themselves and direct legacy/source LINE
paths; a callback hidden behind another helper is not certified by a text search
for INSTRUMENTED_JUMP. Host census tests should enumerate all actual marker and
direct-jump-helper providers and fail on an unclassified new provider. The proof
must use parsed composition/owner state at each site, not a permanent runtime
handler exclusion list. No source implementation is admitted until this census
has a coherent per-provider treatment and independent peer approval.

The source-error invalidation proposal distinguishes pending continuation
publication from an actual redirect commitment. A pending-child request that can
be cancelled on exceptional child exit must NOT clear an old recipe merely when
the setter publishes its request. Perform invalidation in actual CompleteRedirect
commit before callbacks, or prove a narrower admitted phase has no such recipe.
Ordinary and identity actual-PC commits invalidate before their first close.

### Pinned cold invalidation interface

B's independent peer approves this revised direction, including the pending-child
restriction. Proposed declaration in `pycore_ceval.h`:

```
void _PyEval_InvalidateSourceErrors(PyThreadState *tstate,
                                   _PyInterpreterFrame *frame);
```

Implementation belongs in ceval.c: iterate the active `eval_activation` chain;
for an entry whose `source_error_frame == frame`, clear that pointer and reset
its record index. Do not inspect/clear a different frame's recipe. This operation
allocates nothing and invokes no callbacks. The activation links already bound
its lifetime and walk; no pointer escapes the synchronous helper.

There are three exact commit hooks:

1. `frame_setlineno`'s existing identity commit after its overflow/phase checks.
2. Its actual ordinary redirect commit, after all allocation/proof/warnings and
   before any detached-owner close. Do not invoke on its pending-call branch.
3. `_PyFrame_CompleteRedirect`, after child retirement, in the callback-free
   publication block and before `_PyFrame_ClearTemporaries`. Mere publication of
   a pending request does not commit: exceptional child exit cancels it and must
   retain an otherwise relevant old recipe.

Failure paths perform no invalidation. There is no post-close recipe cleanup.
This pin requires coordinator admission of the shared hooks before code edits.

Regression obligations: keep nested-source-error preservation and ordinary
same-PC setter-then-raise target-attribution tests; add an admitted terminal
callback-error/deferred-owner redirect case once that actual phase exists. Prove
an unrelated outer-frame recipe survives nested target invalidation, and a recipe
installed after commit survives subsequent cleanup. Pending-child exception
cancellation must not erase the prior recipe. Do not forge unsupported finalizer
setter phases merely to obtain a positive test; if an ordinary-source witness
cannot reach a case, test the actual cold helper through narrowly scoped existing
internal test support and label its scope. Existing original tests remain intact.

## Smallest coherent provider implementation boundary

Do not register a global terminal replacement for every existing
INSTRUMENTED_JUMP occurrence. The complete census disproves that shortcut.
Introduce a positively checked DSL marker, provisionally
`IR_OBSERVE_COMMITTED_BRANCH`, for paths whose final storage/owner state and
no-following-semantics proof succeeds. It emits the shared observed callback
adapter, not an execution alias or additional IR instruction. Existing providers
without this declaration advertise no POST authorization. Migrate them by their
actual complete outcome; the census remains an explicit open obligation, not a
permanent runtime exclusion list or a claim of final completion.

The first useful increment covers FOR body, OR_POP selected arms and already-
consuming branch outcomes. It repairs the confirmed C-owner leak and stale output
stores, while root can verify unchanged ordinary handler text. Constructor/error
proof and cold phase admission cover exactly these declarations. CompleteStep can
share the callback scope under its existing distinct PRE/POST contract. Other
providers migrate before final completion qualification; no existing API behavior
is silently declared unsupported merely to pass a test.

For END_ASYNC_FOR, the proposed complete instrumented operation first computes
whether the held exception matches, without changing ownership. Its matched arm
uses a normal terminal continuation after detach/publication. Its nonmatched arm
needs a terminal RERAISE action: after the existing RIGHT observation, CONTINUE
installs its held original exception, emits monitor_reraise and unwinds; REDIRECT
retires that exception and uses the committed target; ERROR retires it while
preserving the callback exception. All C owners and POST EH requirements must be
proved. This preserves RIGHT-before-reraise ordering without skipping the second
semantic uop or adding an executable cleanup word. The NEXT versus UNWIND action
is emitted by the provider/generator; it is not a generic metadata interpreter.
This specific migration still needs independent semantic/encoded proof review.

The adapter must also distinguish three existing real coordinates: actual owner
site (for END_ASYNC_FOR), reported branch source (SEND), and physical continuation.
Current `_Py_call_instrumentation_jump` accepts both instr and src, but uses src
for source-edge lookup as well as reporting. The new provider cannot assume the
reported callback source is its ownership site or its compiler source-map key.
Pin that key against B's actual compiler map, retain reported monitoring offsets,
and pass the explicit coordinates to the shared adapter. No second public offset
domain is introduced.
