# Frame-exit scratch-owner source audit

Read-only audit of the current bounded frame model. No native probe, build,
regeneration or runtime edit was performed. The coordinator's 660 eventual-owner
probe is useful dynamic evidence, not a replacement for the transfer obligations
below. No new ordinary frame-exit missing-owner path was found in this census.

## Ordinary return

bytecodes.c `_MAKE_HEAP_SAFE` takes the returned home into an owned value.
`_RETURN_VALUE` moves that owner into its C `temp`, marks the DSL input dead,
synchronizes storage and asserts all scratch homes NULL before unlinking the
child. The generator's exact `canonical_return_input` body proof includes this
assertion and transfer chain. `IR_RETURN_TO_CALLER` has no fictitious child
output: it gives the surviving owner to the caller's recipient or shared
generator completion after child retirement. Discard consumes that C owner.

The compiler supplies this empty-scratch precondition. `codegen_return` unwinds
the active fblock stack while preserving only the return value; with/finally/
handler cleanup emits its semantic exit calls and required retirements before
RETURN. regcfg converts the scope exit with no child output; its occupancy
allocator frees only popped/implicit consumed inputs and keeps other live
owners reserved. It does not assume an unread live value has disappeared.
The sink pass removes a POP only for a fresh unique producer result, includes
implicit readers, and requires a discard-capable producer. Retained shared
cleanup is still executable rather than silently forgotten.

This is deliberately not a claim that thread-frame destruction scans scratch.
`_PyFrame_ClearExceptCode` asserts scratch NULL for a thread-owned frame and
closes only its `co_nlocalsplus` homes. Deleting a remaining release or adding
retention would violate this ABI unless the relevant semantic operation first
retires that owner. A blanket frame-clear loop is not justified by this audit.

## Exception and pre-entry failure

bytecodes.c `exception_unwind` clears all scratch on an unhandled exception.
For a handled exception it clears from `exc_slot-lasti`, preserving only the
handler's validated prefix, then installs fresh exception/lasti owners. The
prefix must be consumed by handler execution or a subsequent unwind before
ordinary frame exit. RERAISE's implicit unwind inputs remain represented in the
compiler and shared constructor proof; they are not scalar operand reads.

`exit_unwind_notrace` unlinks the dying child before frame clearing. It cancels
pending caller redirects before and after child retirement, including redirects
attempted during retirement callbacks. The ordinary caller error resumes at its
actual CALL; no return result is fabricated.

Before execution, a fresh frame's scratch is initialized NULL by the reservation
contract. General argument binding uses a separate consumed argument vector;
its error labels release uninstalled argument owners, while installed values
remain in named local homes for thread clearing. The overlapping fast binders
admit bounded positional layouts, transfer into the child's argument locals and
publish a complete child in a callback-free interval. Excess/unmatched argument
binding does not borrow arbitrary child scratch as an untracked argument bank.

## Escape, generator and reuse

When a frame object outlives its executing frame, `take_ownership` calls
`_PyFrame_Copy`: every slot owner moves to the heap frame and the old slot is
set NULL. The executable and special owners are transferred with explicit
source handling. The escaped frame's GC traversal visits all homes; its
deallocator and tp_clear clear all slots. An escape therefore changes the owner,
not whether scratch remains a root.

Generator suspension intentionally retains live homes. YIELD transfers the
outgoing value to the caller, saves the resume recipient and detaches execution
links. Generator completion or abandonment marks the generator cleared and
uses the non-thread full-slot path in `_PyFrame_ClearExceptCode`; generator GC
also traverses all slots. A newly created generator gets its homes through the
same move-and-NULL copy contract. RETURN still checks empty scratch even for
generator frames; the full generator clear is not permission to omit ordinary
semantic consumption.

clear_thread_frame keeps the frame reservation until all local closes finish,
then drops the executable and pops the arena reservation. Reentrant finalizers
cannot reuse the dying storage before retirement completes. An overlapping
child clears/moves its occupied homes before the parent resumes; arena frontier
initialization relies on this NULL-before-reuse invariant rather than erasing
possibly owned initialized homes.

## Constants and scope of discharge

Constant homes are hidden `CO_FAST_CONST` locals inside co_nlocalsplus, validated
by `check_constslots`. They are not scratch exemptions. Initialization DUPs a
mortal replaced constant; thread-local clearing closes it normally. Inline and
immortal constants own no mortal count. Full frame traversal and heap clearing
include them. No borrowed-constant-bank change is assumed.

The audit discharges the existing compiler-produced ownership model for these
paths, subject to its generator and per-outcome contracts. It is not a complete
C-provider verifier or a proof for arbitrary forged code words: the constructor
does not implement a universal all-instruction ownership analysis. Remaining
direct/composed observer providers can still strand a C-held owner or continue
old semantic work after callbacks; those are provider contracts, not something
thread-frame scratch clearing can repair. Their explicit census remains in
terminal-provider-next-packet-census.md. Preserve the debug scratch assertion
and eventual-collectability tests while those providers migrate. No language
claim depends on a historical decref PC or prompt finalization.
