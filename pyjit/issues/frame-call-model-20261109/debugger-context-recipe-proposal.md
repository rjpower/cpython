# Debugger context recipes after the first two repairs

Proposal during source23 freeze. No runtime edits or worker native jobs.
The NULL and FOR recipes remain the only implemented extensions. This proposal
uses the existing failure inventory, not a claim that the broader gate passes.

## Observed scope

`ownership22-trace-failure-summary.json` records 116 failing subcases: 76 broad
protected/suspended refusals, 13 required-scratch refusals, 21 refusal-diagnostic
mismatches, five extra-exception event cases and one other trace-event failure.
The last two groups need separate observer diagnosis; owner recipes must not
hide them. Existing source tests and expected diagnostic/event assertions stay.

## First extension: identity of an actual real-LINE phase

Before general region analysis, admit a no-flow-change request only when:

- the active observed frame is this frame, without an active child;
- it is an ordinary real LINE callback, not suspended, an error callback,
  a source-only cursor or a deferred completion/return phase;
- the selected *real* target is the same actual instruction about to execute;
- the frame, instruction and redirect generation remain unchanged through
  any fallible preparation.

Preserve every existing scratch home, named local and caught-exception state.
No owner needs to be reconstructed or retired. Do not warn or assign None to
unrelated unbound locals: this request has not entered a different compiler
state. Commit the same executable continuation, clear obsolete LINE handoff,
and increment redirect generation so the observer honors the request.

A nonempty exception table does not invalidate this exact identity proof. It
also does not authorize any other protected jump. `event.to == target` during
a source-only callback is not the proof: the published PC can be a predecessor
and the source event can have a different EH recipe. Initially leave that case
on the existing path rather than treating logical and physical phases alike.

This should address many `JumpFrameModelTestCase.test_jump_to_running_line`
cases without touching structured contexts. Add real-LINE same-PC cases inside
try, except and with, verifying locals and `sys.exception()`; retain a source-only
same-offset negative/control to enforce the phase distinction. A's generation
transaction fix handles successful commit intent independently.

## General context contract

The next transfer is a cold plan with four separate outputs:

1. target incoming physical owner requirements and independently proved NULLs;
2. current owners to preserve, move, restore to named locals, or detach;
3. the caught-exception restoration action, if exiting an active handler;
4. the exact selected executable or suspended-resume phase and its recipient.

An EH table tuple is an *exception-entry recipe*, not the complete current
structured context. Equal handler offsets or equal unwind floors cannot prove
that an active exception, context-manager exit owner or saved comprehension
local survives. Static producer equality cannot prove the current dynamic owner
generation either. Reuse shared ReadsHome/DefinesHome/Outcome/NullableHome for
physical operands, with a pathwise preservation proof for each retained owner.

The compiler still knows region identity and saved local mappings before it
eliminates setup/pop markers. Preserve that information as **cold resumption
metadata**, rather than reconstructing a second frontend program or adding
per-dispatch state. A target context can refer to shared region records:

- ordinary protected try: handler recipe and enclosing region;
- active caught exception: previous-exception owner home and region identity;
- with/async-with: exit callable/optional self homes and enclosing context;
- inlined comprehension: saved local owner homes and restoration destinations;
- suspension: actual resume PC, recipient A/NoDst and suspended exception owner.

Only line/resumption sites and source-only event phases need context references.
Region records can be shared; no old POP/NOP inventory is retained. Exact schema
and size require a compiler census before admission. Extending the versioned
source envelope is preferable to an unchecked private side dictionary if these
facts cannot be derived soundly from the final IR.

## Transfer rules and failure groups

| Transfer | Required proof/action | Existing coverage |
| --- | --- | --- |
| within the same context | Same live structured owners and pathwise survival; target inputs available | try/except/finally body moves |
| into ordinary try | No active handler/with owner is created by entering its body; use target EH recipe | forwards/backwards into try tests |
| into except/finally/with body | Refuse if the target requires an active context absent at source; report the specific context | existing no-jump-into tests |
| out of ordinary try | Drop protection by selecting target context; retire only proven surplus owners | try-to-outside tests |
| out of caught except | Restore caught-exception chain from its actual saved owner, then detach surplus | bare/qualified except exit tests |
| out of with | Preserve existing debugger behavior about bypassing cleanup; retire exit owners without inventing a call to __exit__ | nested with/async-with exit tests |
| out of inlined comprehension | Restore shadowed named locals from the compiler's real saved-owner homes before closing detached values | list/set/dict each-event exit tests |
| suspended redirect | Preserve generator state/exception chain; change real resume continuation and recipient consistently | yield-return jump and async cases |

The matrix is a contract to verify against the unchanged tests, not permission
to guess old debugger policy. In particular, scope exit must not clear child
overlap, call context-manager methods unexpectedly or leave `sys.exception()`
pointing to a detached exception. Arbitrary entry into except/with cannot be
made safe by assigning None.

## Application transaction

Validate the full recipe after warning hooks and any frame materialization.
A nested setter generation change cancels the outer request even at identical
PC. Allocate all cold storage before mutation. Detach surplus owners and take
restoration owners without callbacks, install named-local/context restoration,
commit target phase and generation, then close detached owners. Hold displaced
local/exception owners in the same C-owned array. No outer frame/context writes
follow closing callbacks. Restoration must have a single owner transfer for each
saved value; equal Python values are not interchangeable owner identities.

Pending children retain the separate deferred protocol. Suspended generators
need a dedicated phase constructor: `return_slot` is their next sent-value
recipient, not an ordinary pending-call destination. Do not update only
instr_ptr and leave that recipient describing the old yield. Discarded resume
input uses the existing prepared-resume owner protocol.

## Source consumers and ownership split

- `Python/codegen.c`: with/except/finally region identity and comprehension
  saved-local intent are present here (`_PyCompile_PushFBlock`, LOAD_SPECIAL,
  LOAD_FAST_AND_CLEAR and POP_EXCEPT sequences).
- `Python/regcfg.c` and `Python/assemble.c`: carry that intent through physical
  slot assignment and emit cold recipes alongside real positions; current
  `op->except` and startdepth only describe unwind entry, so do not relabel them
  as full region ownership.
- `Objects/codeobject.c`/marshal: validate cold record bounds, references and
  executable/layout coherence on construction and replace, before analysis.
- `Objects/frameobject.c`: analyze, select and apply the recipe; preserve current
  NULL/FOR helpers as bounded inputs rather than copy their operand rules.
- `Python/bytecodes.c`: PUSH_EXC_INFO/POP_EXCEPT actually transfer caught exception
  ownership; use their exact semantics, not frontend stack depth.
- generator entry/resume helpers and `pycore_interpframe*`: validate suspension
  recipient/exception ownership and pending child phases. Coordinate with A.
- instrumentation/source-error activation: cancel obsolete context on successful
  redirects, including same-PC intent; retain exact source EH on failure.
- remote/GDB: cold schema readers must reject unknown versions; public physical
  offsets and existing source-callback display remain separate.

Before implementation, census the smallest real failure in each row, its final
IR and actual owner homes, then peer the schema/transfer rules. Identity can be
an independent narrow increment. General context metadata should be one coherent
compiler/constructor/application change, not test-name or opcode exceptions.
