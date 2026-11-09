# Debugger entry and frame transfers

This describes the contract integrated at `c1424be11db4`. Full qualification and
the broader transfer model remain open. See [status](../STATUS.md), the
[frame ABI](call-and-frame-lifetimes.md), [observer contract](handler-and-observer-contract.md)
and [repair evidence](../issues/frame-call-model-20261109/qualification-repairs.md).

Debugger requests are cold transactions over actual register frames. Python
does not require CPython's reference-release timing. Conversely, a requested
source position alone cannot create an iterator, restore a shadowed local or
select a handled exception. Such semantic state needs an explicit transfer plan.
Current implementation restrictions are admission limits, not Python language
prohibitions.

## Entry is an observer transaction

Private `sys._debugger_entry(frame, callback)` requires a complete strict active
ancestor of the current Python frame. The current frame is executing the native
entry call and may have C-held arguments; it cannot authorize its own ordinary
instruction-boundary recipe. Inactive frames are rejected before the callback.

The helper captures the real child, saves the existing observer frame/event and
suppresses recursive tracing. It invokes the value-native callback with a
private event discriminator outside the monitoring event domain. It restores
observer/tracing state on success or error. The helper itself does not change
frame owners, continuations or redirect generation; its callback may request a
separately proved redirect. Displaying a caller is separate from
proving a jump into it.

Bdb separates trace preparation from entry. It installs frame hooks before
calling the private transaction, activates tracing inside the suppressed
dispatch, and performs error/quit cleanup there. Its entry hook can reuse a
subclass's interaction hook without emitting an instruction event. Nested
`enterframe` scopes restore their previous value in `finally`. Doctest applies
its output redirection to entry as well as ordinary trace dispatch. Async Pdb
prepares its shim before interaction, enters explicitly on each interaction and
clears async state on all exits; await runs outside the suppressed transaction.

## One analysis and application path

```mermaid
flowchart TD
    request[Requested source line] --> phase[Identify actual frame and observer phase]
    phase --> identity{Exact real LINE identity?}
    identity -->|yes| keep[Preserve owners and exception links; commit generation]
    identity -->|no| analyze[Decode final IR, source metadata and canonical EH]
    analyze --> needs[Derive incoming owner requirements]
    analyze --> context[Derive handled context and saved-owner origins]
    needs --> plan[Select target, preserve homes and saved bindings]
    context --> plan
    plan --> warning[Warn and reprove after callback-capable preparation]
    warning --> detach[Detach surplus owners and restore sources without callbacks]
    detach --> restore[Install saved bindings]
    restore --> commit[Commit continuation, cursors and redirect generation]
    commit --> retire[Close detached owners; perform no later frame writes]
```

`RegisterResumeAnalysis` owns request-local metadata. It uses the same raw EH
decoder and canonical validation as code construction: bounded marked varints,
sorted nonoverlapping ranges, real targets and valid unwind payload homes.
It indexes source records once and builds forward/reverse sparse edges for
normal flow, ordinary exceptions and source-callback errors. Reachability walks
use queues. Allocation products and analysis sizes are bounded before mutation.
No table is serialized, attached to a code object or consulted by dispatch.

Physical requirements use shared read/definition/nullable/outcome contracts.
Ordinary exception requirements come from the pre-success state and are joined
after normal definition kills. Source-callback errors use the actual normal
post-operation outcome. Exception/lasti writes also invalidate whole-code NULL
facts. Immutable constant homes are already available. A nullable operand cannot
authorize an aliased nonnullable reader.

Handled context is derived separately. INHERITED can contain a caller's handled
exception. PUSH_EXC_INFO creates a caught node with its actual saved-previous
owner; matching POP_EXCEPT restores its parent. COPY and SWAP transport value
origins without inventing another dynamic generation. Conflicting ingress is
ambiguous. Ordinary and source-error edges participate in this analysis.

Saved named-local bindings have a separate column per actual LOCAL_MOVE index.
LOAD_FAST_AND_CLEAR creates a token for the saved binding, including genuine
NULL, with a parent from that same local's active chain. COPY duplicates origins
and SWAP transports them. A matching STORE_FAST restores only the youngest save
of its destination local. Different locals may restore in either order; normal
and error paths need not use the same order. Losing every known token home keeps
the obligation active. Ambiguous joins, lost origins and repeated active producers
cannot establish an empty column. Request allocation products are checked before
mutation. These facts come from actual words, not trusted compiler annotations.

`RegisterResumePlan` carries a preserve-home bitmap and saved-local restoration
actions. Its single application path detaches surplus scratch and displaced
local owners, installs bindings, commits state, then closes detached owners.
An actual restored NULL binding is admitted only when each read before a
guaranteed binding definition uses an unbound-binding check. A physical
nullable input is not such a check: NULL can mean deletion or absent optional
self in a handler ABI.
Warning callbacks can redirect the frame: generation detects that intent and
cancels the outer request. Otherwise the plan is reproved against current state.
Closing callbacks never encounter subsequent writes from the old transaction.

## Current supported plans

| Phase or state | Current proof and behavior |
| --- | --- |
| Real LINE identity | Requested resolved line equals the actual current LINE wrapper's line; exact observed frame; no source/error, pending-child or suspended phase. Preserve current PC, all owners and exception links; commit generation. Earlier words sharing that line do not restart the expression or scope. |
| Cancellable PRE operation | A centralized phase factory proves the actual real LINE before its handler. Current expression inputs may be detached when the target needs none; proven saved named locals may be restored from per-local chains. Handled context and independently proved roots must survive. Source-only targets do not acquire this permission. |
| INHERITED context | Existing physical incoming-owner/NULL proof. Bounded FOR-body replay still requires an empty exception table. Ordinary protected try need not create a caught context. |
| Same CAUGHT context | Backward replay preserves actual saved-previous owners and every parent; path/dominance proof excludes read, replacement, pop and reentry. Active exception links remain untouched. |
| Pending ignored Python call | Caller changes its continuation only; overlapping child owners remain intact until successful child completion. Used-result, external and protected child cases still need their own plans. |
| Other contexts/phases | Refuse before mutation until their owner and semantic actions are proved. |

Saved-previous owners must be actual None-or-exception values with surviving
origins. Equal producer PCs do not establish equal dynamic generations. A
same-caught transfer preserves both the target's exception and a callback-owned
exception. It does not implement caught exit.

## Remaining model work

The bounded PRE plan distinguishes disposable expression inputs from saved-local
obligations and restores proven bindings before committing the target. It retires
displaced owners after the whole frame is valid.
Retained outer-with/loop roots need their own availability proof. Source POST,
protected source-only destinations, yield-before-suspension and an actually
suspended frame remain distinct phases with different owner contracts.

Declared terminal providers now commit FOR body and OR_POP outputs before their
callbacks. The constructor checks the generated per-edge physical mapping and
old-owner cuts. A fresh value at an old input's home does not restore its
provenance. This fact does not itself authorize setter SOURCE_POST. That transfer
needs a stopped-state proof of the provider edge, owner lineage and target
context; yield and suspended-frame transfers remain separate.

Caught exit needs the target's actual exception-restoration link. A Python
callback may hold the target's previous context in its own saved-previous cell;
assigning `tstate->exc_info` directly can overwrite the callback and later
restore the wrong target exception. Generator exception-state links add another
storage case. Resolve and prove the actual cell outside child overlap before
transferring an owner. Upstream's observed callback-clobbering behavior is not
the contract to reproduce.

The [active plan](../issues/frame-call-model-20261109/plan.md) owns these next
increments and their proposal, peer review, focused gates and integration.
