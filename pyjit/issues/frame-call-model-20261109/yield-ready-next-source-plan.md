# Yield-ready next source plan

Design only after native30 post-review. The actual generated instrumented yield
reads val from window[0], observes, then enters MAKE_HEAP_SAFE/YIELD, which clears
that home and changes generator/frame links. The present event uop checks only
PC movement. This packet would replace that check with typed intent handling;
it would not change ordinary YIELD_VALUE or already-suspended frames.

## One positive pre-switch adapter

Use a dedicated `IR_OBSERVE_YIELD_READY(val)` intrinsic in the event uop. Its
generator validation must prove the complete observed-prefix/unchanged-yield
suffix chain, including actual frame-home input and no owner transfer before
the decision. Recognition is source/flow based, not an instruction-name list.
The static contract must fail closed if a suffix writes a recipient before the
decision, a redirect/error arm falls through, the input is rebound/consumed, or
an unproved helper/extra part appears.

The C adapter `_PyIR_ObserveYield(tstate, frame, site, PyValueBorrow value)`
returns the existing observer result status and current PC. It supplies an exact
YIELD_READY scope before any suspension write. Its site is the real yield word,
its edge is unavailable, and its row is the executing pre-word row. No completed
edge or fictitious destination is declared. The window format keeps the outgoing
owner in a scratch home; this is not a borrowed named-local value that a
f_locals assignment can replace underneath the cached payload.

The adapter owns a duplicate payload across all tools using the existing tool
ordering and callback scopes. Restore each callback scope before closing its
arbitrary return. Release the payload with any callback exception saved, and
only then decide from actual PC and redirect generation. A lazy frameobject at
generation zero is not a redirect. Do not retain YIELD_READY authority across
the payload close; an older outer scope still needs its own exact association.

Unchanged returns to the existing suffix. Redirect reloads slots and dispatches
the latest PC immediately. Error uses the current PC+1 convention expected by
the generated error label. Neither terminating arm may write old output homes,
return_slot, gi_frame_state, previous links, exc_info links, recursion state or
the parent result. The actual generated ordinary yield and existing four float
and terminal providers should remain byte-identical.

## Same stopped-state transaction

B would capture the executing pre-yield row only from this positive scope and
validate exact observed frame, site and original generation. The frame must be
currently running; a stored suspended frame remains unavailable. Reuse the
existing Analysis/Plan, saved-token/no-reentry, NULL-target and caught-context
rules. First cancellation is inherited context with no required target scratch;
do not authorize caught exits, retained-root reconstruction or delegated resume
input from the event name alone. A later actual yield installs its own resume
recipient normally; a cancelled yield installs none.

On a successful setter, source-error invalidation and detach/install/commit
remain the existing transaction. Callback error without redirect uses the
pre-yield EH/owner state. Setter-then-raise uses the committed target. Payload
retirement can reenter; its newer supported redirect wins and must not be
overwritten by the adapter's earlier cached target.

## Files and gates

A owns the observer helper declaration/implementation, the event intrinsic and
generator validation/emission plus host/provider tests. B owns the stopped-state
capture/Plan phase and focused debugger tests. H peers pre-error ownership and
confirms no completed-outcome fact is accidentally advertised. Root alone owns
regeneration, native gates, integration and measurement. Canonical-home result
work must finish its separate source snapshot before either worker edits the
shared generator.

Host positives/mutations must execute the real generator and inspect the typed
arms, proving only unchanged execution reaches suspension. Runtime cases should
retain the original jump-from-yield test, one real yield/resume count, cold/warm
bound and unbound restoration, same-PC intent, outgoing mortal collectability,
multi-tool payload identity, callback error and redirect-then-raise attribution.
Keep a true suspended-frame refusal and delegated/required-owner negative.

Whole END_ASYNC_FOR remains a following semantic outcome needing NEXT/UNWIND;
direct END_SEND/FOR need complete payload/publication/consume contracts. They
must not be pulled into this pre-switch packet or marked terminal at an early
monitor uop. No source change is admitted by this document.
