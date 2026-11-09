# Next terminal provider increment

Proposal only while native29 is frozen. No runtime edit, candidate execution or
regeneration is part of this plan. Keep the first packet's generated ordinary
handlers unchanged. The next useful migration is owner-neutral jumps and
POP_ITER, followed by a separately gated pre-suspension yield adapter. Whole
END_ASYNC_FOR and direct END_SEND/FOR remain explicit later providers.

## First source chunk: terminal transfer and iterator retirement

Replace the observer call in INSTRUMENTED_JUMP_FORWARD and the final
_MONITOR_JUMP_BACKWARD uop with the existing positive terminal marker. The
periodic check remains before observation. Refine the single positive marker to
take four arguments: `(reported_source, physical_next, event, owner_edge)`.
Update every current provider and host fixture explicitly; retain no three-arg
fallback. Literal PY_MONITORING_EVENT_JUMP is valid, but never determines the
owner edge. Dynamic or unknown event/edge arguments still fail generation.
These operations have no output or deferred owner.

INSTRUMENTED_POP_ITER must first TAKE its iterator into a checked C owner, mark
the cursor dead, and transfer that iterator through IR_DEFER_CLOSE before the
terminal marker. The final emitter clears consumed homes, commits the real
continuation, observes the branch, publishes the helper result and retires the
held iterator with the callback exception saved. It dispatches the latest PC
after retirement. No standalone iterator close remains after a redirect-capable
callback. POP_ITER's actual owner edge is FALLTHROUGH, despite its reported FOR
BRANCH_RIGHT event. Jumps use TAKEN. The actual owner site is POP_ITER; its
reported FOR source stays a separate coordinate and cannot select ownership or
source-EH state.

The explicit edge is the one fact used by both generated metadata and the
runtime observer call. Validate its literal and supported physical destination
shape together: fallthrough is next_instr; taken is JUMP_TARGET(oparg) or the
existing fused-distance expression. Do not infer edge from LEFT/RIGHT or source
offset. Host fixtures need meaningful distinct target/fallthrough expressions,
not two differently named events both pretending to select next_instr. This
refinement changes no current provider's generated C or metadata facts.

The existing FORMAT_VALUE declaration also represents zero-result committed
operations; it does not promise an output exists. The shared decoder must
confirm this for jump and POP_ITER formats before admission. Consumed input
provenance is cut even when a physical home is reused. No new outcome enum,
runtime handler alias or per-dispatch metadata walk is needed.

Files: bytecodes.c, the marker event proof in ir_generator.py, generator tests,
and focused terminal runtime tests. H independently reviews the actual encoded
read/retire facts. B's stopped-state consumer remains a separate source chunk.
Root compares all ordinary generated handlers and runs native gates.

## SOURCE_POST provider authority

The actual source-edge call-site census has four callers, all in
instrumentation.c:

| Caller | Publication contract |
| --- | --- |
| `_PyIR_ObserveStep` POST | committed window/result, detached C owners isolated; SOURCE_POST allowed |
| `_PyIR_ObserveCommittedBranch` | generator-certified final publication; SOURCE_POST allowed |
| `_Py_call_instrumentation_line` landing | predecessor dispatch has completed, before landing execution; SOURCE_POST allowed |
| Legacy `_Py_call_instrumentation_jump` | later semantic stores/closes may remain; source callbacks still fire, authority UNSUPPORTED |

Thread one explicit publication argument through source_edge_events and
source_line_event. It selects SOURCE_POST versus UNSUPPORTED in the existing
scoped record; it is not a second setter-side authority or a new field. Supply
the actual physical continuation pointer as well. Currently source_line_event
sets spec.next from event.to, which is the normalized real landing and can differ
from an immediate relaxation helper. B should verify the normalized continuation
against record.to while retaining the actual pointer as the scope coordinate.
Do not expand this into an arbitrary jump-chain walk.

The landing-wrapper contract concerns completion, not the predecessor's public
event spelling. All previous ordinary publication has finished before dispatch
can enter this wrapper. B still needs the exact-edge transient origin/save row
and its independent target/NULL safety proofs. Unsupported legacy source hooks
retain display cursors, event delivery and error attribution; masking authority
must not suppress their LINE events.

The first consumer cannot call the current context transfer blindly: token
producers such as PUSH_EXC_INFO and LOAD_FAST_AND_CLEAR can mutate Analysis
parent tables. Positively exclude these creating transitions in the bounded
first rung, or extract a read-only transfer before widening. Copying rows alone
does not make shared parent-table writes safe. This limits consumer capability,
not the publisher's truthful statement that the operation completed.

## Yield-ready contract

Add a narrow `_PyIR_ObserveYield(tstate, frame, site, PyValueBorrow value)` helper
returning the existing typed observer result: unchanged, redirect or error. It
is called only from _YIELD_VALUE_EVENT. It supplies YIELD_READY at the actual
YIELD site, with the executing frame and original generation, before
_MAKE_HEAP_SAFE or _YIELD_VALUE changes any owner or frame/generator link.
Its coordinates describe the pre-word row; there is no fabricated completed
suspension edge or target destination.

The helper owns a duplicate event payload across all selected tools, preserving
existing callback order. The original outgoing value stays frame-owned until
the typed decision. Each tool scope is restored before its arbitrary result
close, as in the current common callback runner. The yield scope must not remain
installed across the payload close. Any outer scope must still pass exact
frame/event/generation validation; it cannot inherit YIELD_READY authority.
Save a callback exception around payload retirement and recheck actual PC and
generation after that retirement so a newer supported redirect wins.

On unchanged success, the generator continues through the existing canonical
value transfer and suspension unchanged. On redirect it reloads/dispatches the
latest PC and skips the whole MAKE_HEAP_SAFE/YIELD suffix: no old outgoing store,
return_slot update, gi_frame_state publication, previous/exc_info detachment or
recursion exit is allowed. On error it uses the published PC+1 adapter expected
by the generated error label. An unredirected failure retains pre-yield EH;
setter-then-raise uses the committed target and invalidated old source recipe.

B enables this phase only through the request-owned stopped-state view and the
same reviewed Analysis/Plan constraints. No actual suspended frame, pending
child result, caught-context exit or delegated-input reconstruction is admitted.
The existing scalar marker proof does not certify suspension. A separate exact
generator contract must prove that the typed redirect/error arms terminate and
that only the unchanged arm reaches the existing yield suffix.

Files: observer header declaration, instrumentation.c helper, bytecodes.c event
uop, generator validation/emission if a dedicated typed marker is required,
host mutation tests and focused yield cancellation tests. Avoid changing the
ordinary YIELD_VALUE generated block. Agree B's exact phase-view interface before
implementing the helper or enabling setter authorization.

## Later whole outcomes

END_ASYNC_FOR requires a complete semantic provider, not a marker inserted in
_MONITOR_END_ASYNC_FOR before matching/reraise. Model NEXT versus UNWIND with a
typed terminal action. On UNWIND an exception owner remains held until it is
installed as the raised exception; no fake normal next pointer represents this
action. Redirect/error retire that owner with the correct active exception.
Actual END_ASYNC_FOR owner/source-EH site remains distinct from reported SEND.
The subsequent monitor_reraise callback must not inherit stale POST authority.
This action schema needs a separate peer before source work.

Direct END_SEND should use the same result/window commit as common SEND
completion and the same StopIteration payload constructor. Direct END_FOR keeps
its input frame-owned on STOP error; successful redirect cancels its old consume.
Neither can be certified merely because canonical lowering usually elides them.
The current direct monitor_stop_iteration payload construction defect is part of
their migration, not a reason to copy a known bug into a new provider.

## Focused admission ladder

1. Host positive no-output JUMP and POP_ITER deferred-owner emission; assert
   reported BRANCH_RIGHT plus owner FALLTHROUGH reaches the POP_ITER source
   record. Reject unknown events/edges, mismatched destination shapes,
   post-marker semantic work, duplicate or early-exit deferred owners. Confirm
   existing provider facts and ordinary generated handler byte equality.
2. Jump LINE/source-edge order and callback errors; POP_ITER eventual iterator
   collectability, callback error preservation and safe close-time inspection.
3. Yield host mutation negatives for suffix execution after redirect/error,
   stale output use and ownership transfer before decision.
4. Ordinary-source yield cancellation counts one actual yield/resume, preserves
   outgoing payload identity across tools, allows same-PC redirect intent, and
   releases cancelled mortal output eventually. Callback error/redirect-then-
   raise and latest close-time redirect need exact source attribution checks.
5. Root native release/debug focused observer, generator, frame and completion
   gates; reclassify original tracing/debugger failures before selecting the
   following provider. Do not update expected transcripts to conceal lost events.

## Peer disposition

H approved the zero-output metadata/decoder direction, including empty selected
home sets and both POP_ITER input cuts. B identified the initial event-to-edge
conflation and approved the refined four-argument marker with destination-shape
proof. That first draft error is corrected above; no source used it. Both peers
retain separate admission for the yield adapter and stopped-state consumer.
