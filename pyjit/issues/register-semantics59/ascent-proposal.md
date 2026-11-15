# Checked delegated ascent61

Root approved the revised contract and the implemented source diff. Focused
host source checks pass; generation and native qualification remain pending.
The source packet is isolated in `build/worktrees/parity61-ascent`. Its base is
selected57 `0dcba75bb3df40c32137db453b001f7e8f101de0` in
`build/worktrees/call55-integrated`, read only. Canonical documentation is this
checkout. This packet implements packet 3 of
[interpreter-parity61](../../design/proposals/interpreter-parity61.md), separately
from the producer and direct-creation packets. No full generation, build,
native runtime test or measurement has been performed for this packet.

## Closed change

Change only `Tools/cases_generator/ir_generator.py`'s `suspend_delegated`
emitter, one private inline eligibility helper in `Python/ceval.h`, generated
`Python/generated_ir_cases.c.h`, focused host cases in
`Lib/test/test_generated_cases.py`, and a new
`Lib/test/test_ir_delegated_ascent.py`. No new opcode, frame field, owner kind,
image descriptor, SEND implementation, root walker or reader protocol.
The generator retains the current body for INSTRUMENTED_YIELD_DELEGATED.
Ordinary YIELD_DELEGATED gains a checked loop under
`!Py_DEBUG && !Py_GIL_DISABLED && !WITH_DTRACE`; other configurations retain
the current body. Py_STATS retains one INSTRUCTION_STATS invocation for every
semantically executed YIELD_DELEGATED, including composed parents.

The release loop carries the existing canonical `outgoing` owner through at
most 16 real frame suspensions per handler invocation (at most 15 skipped
parent publications/dispatches). This fixed bound is not a polling interval:
at its limit use the ordinary publication and dispatch, without adding a poll.
No scheduling or interrupt boundary is removed: selected DISPATCH has none,
and YIELD_DELEGATED has no CHECK_PERIODIC/RESUME work between these hops.

The inline helper accepts the still-linked current child and its parent and
returns the parent's actual executing successor word on success. It is a pure,
nonraising read: no refcount operation, allocation, callback, instrumentation
refresh or semantic-plan search. Do not outline the loop or allocate a plan.

## Dynamic admission and trusted invariants

Check eval-frame, trace and profile hooks, including value hooks, once before
entering the loop. They cannot change inside its GIL-held, callback-free
interior. The build exclusions above are compile-time, not repeated guards.

These are the dynamic checks on each proposed parent hop, before detaching
the current child:

1. Parent is generator-owned and its enclosing object is an exact generator or
   coroutine. Parent has no pending_redirect. Retain the initial current
   frame's pending_redirect check at loop entry too.
2. The actual executing word at parent->instr_ptr is SEND_GEN;
   parent->return_pc equals parent->instr_ptr + 1; the actual word at that
   return_pc is exactly ordinary YIELD_DELEGATED. Generic, instrumented,
   custom, virtual and async-generator SEND fail this selection.
3. Let s be saved SEND B. Require parent->return_slot == s+2,
   successor B == s+2 and successor D == s. The object at s must be exactly
   the generator/coroutine containing the current child frame. Require the
   physical recipient at s+2 to be NULL. For the first supported shape require
   successor A == successor B; other valid resume destinations fall back.

Trust existing interpreter/image invariants rather than checking them on each
hop. `Objects/codeobject.c` validates word ranges, operand roles and complete
scratch windows during cold image admission; semantic_delegated_step proves
the SEND receiver/outgoing roles and rejects unproved constructed images.
Thus s, s+2, successor B/D and the admitted A==B destination are scratch, in
bounds and distinct where required. Executing images preserve admitted operand
fields when specializing/instrumenting handlers. No immutable-word copy or
operand comparison is needed. Frame/code ownership pins executing words and
geometry; saved PC alignment/range, base==owned_slots and executing-copy
identity are runtime invariants, not new hot-path byte-corruption defenses.

Likewise, current_frame/exc_info identity, live acyclic previous links,
FRAME_EXECUTING states and exception previous_item linkage follow from the
checked SEND_GEN entry and each unchanged suspension commit. Keep debug/source
assertions where useful, but do not add per-hop state/type/geometry scans to
re-prove them. The parent comes directly from current child's previous;
the current child is already executing YIELD_DELEGATED. The cursor is retained
unchanged and need not be tested: ascent neither reads nor consumes it.

The dynamic identity/recipient checks prove a live VM SEND entry, not merely
equal slot numbers. The
`enter_generator` emitter at selected `ir_generator.py:5621` takes B+2 and
clears it before publishing return_slot, return_pc, previous and exception
links. Between entry and this return the suspended parent cannot execute.
The exact receiver still owns the child. Redirects fail admission. The final
physical NULL check rejects unsupported constructed states without relying on
uninitialized scratch. Selected storage initializes all scratch; bitmap trials
are not a dependency. The positive delegated-role image admission remains in
force; these checks further restrict it. Invalid public images retain their
existing constructor rejection, rather than becoming an alternate execution
protocol. A valid but unsupported image takes the unchanged ordinary path.

## Loop, exits and owner proof

Keep the first handler's existing take/clear and ownership event. For each
iteration, precheck the next parent while the child is still linked, then
perform the selected suspension sequence for the current frame, in order:

- Commit its original continuation PC (yield site + 1) and phase invalidation.
- Preserve the DTrace return operation in configurations using the ordinary
  body; the composed body is excluded entirely from WITH_DTRACE.
- Restore tstate exc_info, clear the child's exception previous_item, and
  leave Python recursion once.
- Save original YIELD A as its resume return_slot; update the remote profile
  anchor and sequence using _PyThreadState_UpdateLastProfiledFrame.
- Set frame/current_frame to the real parent, detach child's previous, set
  generator_return_kind, publish _PyIR_PHASE_YIELD_DELEGATED, then release-store
  FRAME_SUSPENDED_YIELD_FROM.

On failed admission or the bound, reload slots and next_instr from that real
parent, call existing _PyFrame_SetReturnValue(parent, outgoing) once, honor its
redirected return_pc, run LLTRACE_RESUME_FRAME and DISPATCH. This path retains
local/discard finalization and redirect cleanup after the owner is published
or handed to the existing consuming adapter. There is no speculative parent
mutation to undo.

On successful admission, leave the proven vacant recipient NULL. Set the
parent's phase_state to zero, enter its original YIELD site, retain its original
word and continuation PC, and account INSTRUCTION_STATS before the next
iteration. Reuse the same suspension body with that word's A; do not reuse the
initial handler's operand or const this_instr. No store/reload/take/clear of
outgoing occurs for this parent. Its original interior word remains executable
by ordinary entry and by every failed guard.

The only owner is the initial C-local outgoing, then the final recipient or
existing consuming fallback. Receiver/cursor homes never move. Every skipped
parent has the same post-take NULL home and final header/phase state as ordinary
YIELD_DELEGATED. Semantic owner programs, outcomes, error routes and phase
metadata therefore remain byte-identical: _PyIR_OWNER_YIELD_DELEGATED still
describes each original semantic instruction, not the number of native entries.
No error can originate inside the admitted interior.

## Reader interval and conservative exclusions

No transition root is required for the admitted GIL release loop. This is a
specific exclusion argument: its reads, scalar writes, recursion leave and
profile-anchor update cannot allocate, decref, detach the thread or invoke
Python. Same-interpreter Python/GC/frame-copy readers cannot acquire the GIL
during it. The ordinary handler already holds the taken owner through this
same suspension sequence. Extending that sequence to a bounded number of
frames adds no safepoint. Existing C-local roots remain governed by the native
root policy (`gc_trace.c` register/stack capture); this packet does not claim
that an arbitrary foreign C stack can be traversed without quiescence.

Remote readers can sample during ascent and do not require the outgoing home:
`Modules/_remote_debugging/frames.c` reads frame identity, code, PC and previous;
keep every actual pop and profile-anchor sequence change.
`Modules/_remote_debugging/asyncio.c:get_awaited_coro_address` requires the
delegated phase and validates the immutable YIELD at continuation-1 before
reading D's receiver. Each frame publishes that phase only after its complete
header/home commit. Its receiver remains owned and unchanged; its B home is
NULL exactly as in selected57. `Objects/genobject.c:gen_yieldfrom_ref` uses the
same finite role through _PyIR_DelegatedReceiverHome. No reader treats B as the
receiver, and no result owner is falsely advertised as a frame edge. Frame
copy/traversal keeps the current selected NULL-control fallback.

Debug fallback is deliberate: LLTRACE_RESUME_FRAME calls
maybe_lltrace_resume_frame, which calls PyDict_ContainsValue(globals, ...).
A mixed-key globals table can run equality even when frame->lltrace is zero.
Skipping that lookup is not proved safe by a zero flag. Debug qualification
therefore covers ordinary fallback only; release tests must exercise ascent.
WITH_DTRACE and free-threaded admission are deferred. If source peer finds an
additional concurrent owner reader or callback, stop this packet for review;
do not silently add a new root protocol or weaken a reader.

## Minimal falsifiers and coordinator gates

Host generator cases check that only the certified ordinary delegated emitter
gets the loop; the instrumented twin remains ordinary; alternate-frame hops
use the newly decoded original word/PC; metadata owner programs and phase
selectors remain unchanged. Retain mutation tests for the delegated transport
and SEND entry source contracts. Root owns full generation and native builds.

Runtime fixtures use warmed exact nested yield-from chains of depths 2, 8 and
40, asserting yielded object identity, send values, StopIteration.value,
handled exception state and stable recursion capacity after repeated cycles.
Depth 40 crosses the 16-suspension bound. Add aliased/self-cyclic yielded
objects, abandonment+GC, throw/close and reentrant finalizers after publication.
Inspect every suspended gi_yieldfrom/cr_await link and frame locals; cover a
nested await chain that actually emits delegated yield, rather than assuming
the existing coroutine benchmark does. Trace/profile/monitoring changes from
the leaf, debugger redirect, custom iterators and constructed role/recipient
mismatches must retain ordinary behavior and never compose that edge. Reuse
existing IR image construction support; malformed images may be rejected at
admission instead of reaching execution. Run existing remote async/frame-cache
tests plus sampling of the nested workload during repeated yields.

Release fixture outputs alone do not establish admission. Root must capture
the actual native loop-back edge for the warmed depth-8 fixture and verify
more than one real parent suspension per native YIELD_DELEGATED entry, using
a separate diagnostic breakpoint/uprobe or instrumented evidence build. Also
capture depth-40 bound exits and hook/unsupported-shape fallback. Retain hop
lengths as diagnostic evidence, never infer them from monitored opcode visits.
No permanent per-hop counter/API is proposed.

Native falsifier: compare selected57 and candidate two-hop and longer ascent
paths, including the once-per-entry hook gate and every dynamic hop guard,
not just the successful loop body. Price receiver/type/word loads, identity
and vacancy tests, bound/exit branches and any compiler duplication of those
guards. Cold admission facts must not become hot plan lookups or repeated
geometry validation. The candidate must remove repeated
_PyFrame_SetReturnValue, parent result reload/clear and handler transfer while
retaining all per-frame stores above. Charge inlining/text growth, stack frame,
spills, extra word/receiver loads and branches. Reject comparable staging or
guard cost, unsafe readers, or broad elapsed regressions. Root runs the eight
fixed workloads against selected57 and any separately selected combination;
the losing round59 packets and engine60 are not controls or prerequisites.
