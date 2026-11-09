# Encoded ownership proof for terminal observer errors

Proposal against engineering source `4a1f70fcef4`. A owns terminal emission;
B owns debugger consumers; the independent cold reviewer owns encoded
constructor validation. This document defines their shared POST facts.

The FOR temporary-destination geometry gate is a separate bounded constructor
increment backed by the executed v3 constructor probe. Root independently
reviewed the actual gate and test patches; A and B each explicitly approved
those patches. Root now admits their direct application to
`Objects/codeobject.c` and `Lib/test/test_ir.py`, followed by focused native
validation. The earlier automatic review rejected application because it
considered root admission outstanding. These concrete reviews and this
explicit source/test admission resolve that condition; no indirect application
or workaround is authorized. The broader observer outcome schema below has
A's interface ACK but remains separate from this bounded constructor repair.

## Existing proof and its limit

`_PyIRFormat` supplies roles, physical output kind, `reads`, `writes`, and
`through` (`Include/internal/pycore_ir.h`). `pycore_ir_outcomes.h` supplies
`_PyIR_Outcome`, `_PyIR_ReadsHome`, `_PyIR_DefinesHome`, and NULL-read
capability. The encoded completion proof actually lives in
`Objects/codeobject.c:validate_source_map`, `completion_handler_safe`, and
`validate_completion_words`; `Python/ir.c` supplies word/format/depth access,
not that proof. `Objects/frameobject.c:register_resume_liveness` already sends
source callback errors through the selected **normal** outcome, then adds
ordinary compute-error requirements separately. That phase distinction is
correct. The constructor presently applies its owner proof only to special
completion windows; it does not certify an ordinary consuming branch or the
FOR body after item publication.

The current `_PyIRSlotOutcome` says that an old value was retired, and can name
one new destination. It cannot by itself say whether that destination holds a
value, is definitely NULL, or was never written. Those are different facts:
`JUMP_IF_*_OR_POP` writes explicit `PyValue_NULL` on its drop arm, whereas a
compute error before that assignment has no result. `_PyIR_NullableHome` is a
reader capability, never evidence that a home is NULL.

## One declared edge fact, one cold transfer

Add one generated `uint8_t observer_outcome[2]` to the existing format row,
sourced only from A's checked final terminal commit or the existing
CompleteStep POST owner. A and the cold consumer agreed these exact values:
`_PyIR_OBSERVER_NONE`, `_PyIR_OBSERVER_FORMAT_VALUE`,
`_PyIR_OBSERVER_FORMAT_NULL_FIRST`, `_PyIR_OBSERVER_FOR_BODY`, and
`_PyIR_OBSERVER_COMPLETION`. The edge index is the existing
fallthrough/taken source edge, not a monitoring-event name. `NONE` cannot
authorize a POST debugger phase. A conditional with two observed arms needs a
checked declaration for each arm. `FORMAT_NULL_FIRST` requires one physical
result and a proven explicit NULL assignment before the callback; it must not
be inferred from a missing output. `_PyIR_OBSERVER_FOR_BODY` uses the existing
`_PyIR_Outcome(word, edge)` plus a checked physical mapping for both updated
cursor and item; `_PyIR_OBSERVER_COMPLETION` uses CompleteStep's before/after
owner contract. The cold getter resolves a pristine constructor word through
`fmt->twin` to the certified instrumented row, or reads the canonical row for
a no-twin direct helper such as SEND. This is a single generated declaration,
not two independent opcode tables.
Do not add a handwritten handler-name admission list. The source declaration
and generated row are one fact; composed macros and direct helpers may set it
only after their full semantic path has an admitted terminal commit.

Expose one cold decoder, provisionally `_PyIR_ObservedHomes(word, edge, phase)`,
which returns both **physical home effects** (new value, definitely NULL,
unchanged or unavailable) and **old-origin cuts**. The result is derived from
the declared kind and existing field roles, extents, destination, and physical
output mapping. It is a transfer over homes, not a second unwind recipe. An
output aliased to a retired input makes a valid physical value but does **not**
restore the old owner's provenance. Keep old-origin and physical-validity bits
separate in the same cold proof. Named-local input borrows remain in
their local; only actual temporary consumption clears a home. A folded-local
destination is installed before POST, while its displaced owner is a separate
C owner whose release A proves. `IR_NO_DST` has no result home: the result is
detached and its C owner must be closed once on success, redirect, and error.

For a register FORMAT edge, the checked generator commit proves every
non-through temporary input is consumed or transferred and every declared
output has been physically stored. `FORMAT_VALUE` defines the result with a
value; `FORMAT_NULL_FIRST` defines it as NULL. For a window FORMAT edge,
`B..B+through-1` survive; the remaining input homes lose their old owner;
`B+through..B+writes-1` are the actual new window outputs, except that a
folded-local or NoDst result is moved/detached from its window result home
`B+fmt.dst`. This uses the checked generated storage mapping; `writes-through`
does not necessarily identify the moved result. For each admitted
`WINDOW_DST_FIRST` terminal provider whose generated `IR_MOVE_TO_DST`
(`_OR_DISCARD`) moves only for local/NoDst, temporary A must equal
`B+fmt.dst`. Otherwise the physical result remains at another window home.
The first constructor increment enforces this for FOR; the shared decoder
must enforce it for any later admitted window provider.
The same role/extent decoder must reject overlapping, out-of-frame, or
unsupported dynamic layouts; the generator cannot attest an abstract result
whose physical home differs from the encoded word.

For the **compute PRE error** at the same word, use a separate conservative
input fact: through homes remain anchored; every non-through temporary that
may have been consumed is unavailable and *may be non-NULL*; no success output
is presumed. If A can prove a stronger pathwise error snapshot at each actual
error exit, those snapshots may meet at construction. Until then, fail an
encoded recipe that reads a possibly consumed home. A POST declaration must
never erase PRE ordinary-error needs. The source-event handler is checked from
the committed POST edge only; the ordinary exception-table handler is checked
from PRE compute errors **and** POST callback errors where that table can catch
both. Use the actual source `(from,to,edge,handler,exc_slot,lasti)` and marked
exception-table geometry; do not guess a nearest word or turn an event name
into a phase. CompleteStep's STOP callback is PRE, while its branch/source
callback is POST.

Generalize the existing `completion_worklist` to selected physical homes
instead of assuming a contiguous completion window. Retain its old-origin
`unavailable/may_nonnull` facts **without clearing them for the producer's own
replacement output**. Add only the physical fact needed for a callback error:
value, definite NULL, or no committed definition. A later *encoded handler*
definition may discharge an old-origin obligation, as the current completion
walker does. A POST source handler may read a new result in a fresh home, but
an aliased old input remains conservatively unreadable until an explicit
provenance contract says the recipe expects the new value. Neither the source
row nor the format currently says that, so this proposal adds no such escape.
The existing `_PyIR_ReadsHome`, `_PyIR_DefinesHome`, `_PyIR_NullableHome`,
`_PyIR_Outcome`, exception floor/payload writes, and join rule still drive the
forward handler proof. A nullable read is allowed only for a **proved** NULL;
it is not allowed for an absent result or an old-origin cut with an unknown
replacement. Preserve the old completion checks as clients of this same
worklist. No opcode-specific second graph walker, hot frame field, or parallel
EH selection is needed. Keep named-local restoration and origin/context proof
in B's existing Analysis/Plan; physical validity alone does not license a jump.

## Encoded witnesses to require

These are `IR_WORD(handler,A,B,C,D)` field patterns, with scratch beginning at
`nl=2`; `C` is the existing feedback field where applicable. Their source
edges and handler reads must be built as code-constructor tests, rather than
asserted on abstract stack heights.

| Word and edge | Committed POST physical state | Required positive / negative handler witness |
| --- | --- | --- |
| `IR_WORD(INSTRUMENTED_FOR_ITER,6,4,fb,+2)`, LEFT/fallthrough | `4` iterator remains; `5` updated cursor owns its new value; `6` owns item. The old cursor origin at `5` is still cut. | Source handler may read fresh item `6` when its floor preserves it. Reject a source/ordinary handler reading `5` as the old cursor; the current source row has no declaration that it expects the replacement cursor. |
| `IR_WORD(INSTRUMENTED_FOR_ITER,1,4,fb,+2)`, LEFT, folded local `A=1` | Local `1` owns item; `5` owns cursor; window result `6` is definitely NULL; old local owner is deferred C state. | A nullable read of fresh, NULL `6` may pass after POST unwind if still preserved. A nonnullable read of `6` fails. A source handler reading local `1` sees the committed binding; reading old cursor `5` fails. |
| `IR_WORD(INSTRUMENTED_FOR_ITER,IR_NO_DST,4,fb,+2)`, LEFT | `5` owns cursor; `6` is definitely NULL; ignored item is a detached C owner. | POST error must close that C owner; a handler requiring item at `6` fails. |
| `IR_WORD(INSTRUMENTED_FOR_ITER,7,4,fb,+2)`, LEFT, `A=7` temporary | Current `IR_MOVE_TO_DST_OR_DISCARD` leaves item at `6`, while current `_PyIR_Outcome` reports `dst=7`. | Reject this encoded mismatch unless another constructor geometry gate already excludes it. Do not accept an EH read of phantom `7`. |
| `IR_WORD(INSTRUMENTED_JUMP_IF_TRUE_OR_POP,7,4,target_hi,target_lo)`, taken | `4` consumed/NULL; `7` owns the retained condition. | POST source handler may read `7`; reject a handler reading old owner at `4`. A PRE truth-error handler cannot assume `7`. |
| Same word, fallthrough | `4` consumed/NULL; `7` is **defined NULL**, not missing. | Admit a NULL-capable read of fresh home `7` only when that actual reader allows it; reject a nonnullable read. A PRE handler cannot infer that NULL assignment. If `A=B`, old-origin cut still rejects the read despite the new NULL fact. |
| `IR_WORD(INSTRUMENTED_POP_JUMP_IF_TRUE,4,fb,target_hi,target_lo)`, either edge | `4` consumed/NULL; there is no output. | Reject a source or ordinary handler requiring old `4`; accept a path whose EH floor overwrites `4` before use. |

For FOR, `_PyIR_Outcome`'s existing body entry currently retires only `B+1`
and reports `dst=A`. A sound common decoder must also represent the physical
window result `B+2`: retained when `A=B+2`, definitely NULL after a folded
local transfer or NoDst detach. Check both a source-only EH recipe and an
ordinary exception-table recipe covering the same word. The ordinary provider
failure occurs before item publication, so a recipe that reads item `6` is
invalid even if the POST source recipe may read it.

The geometry rule belongs in `validate_completion_words` as well as the POST
decoder: for a FOR word, `A` must be a named local, `IR_NO_DST`, or exactly
`B+2`. No other temporary destination is implemented by the current emitted
`IR_MOVE_TO_DST_OR_DISCARD`; metadata must not bless one. A bounded constructor
probe can compile `def f(xs):\n    for x in xs: pass`, find its FOR word, and
replace only that word's `A` with `B+3` using
`test.support.bytecode_helper.encode_words`, increasing `co_stacksize` if
needed. `code.replace` must reject the mutant without executing it; the
unaltered word and `A=B+2` control must construct. An `A=IR_NO_DST` witness
requires a real encoded discard-capable FOR site, or a separately constructed
valid source recipe; simply changing an arbitrary loop's A would also change
its intended dataflow. Keep its existing `C` feedback field and signed `D`
distance unchanged in all mutants.

## Admission and review gates

1. A proves terminality and physical output ownership for every declared
   edge, including full composed/direct legacy providers. A declaration is
   absent when later semantic work remains, an output is undefined, a C owner
   is unaccounted, or an emitted store can run after callback/redirect.
2. B decodes the declaration and checks each encoded source/ordinary EH edge
   with the existing owner lattice, including aliases, local/NoDst destination,
   exception floor, payload writes, and joins. The negative words above must
   fail construction. The debug phase getter consumes only declared POST edges
   and then retains B's target/context analysis.
3. Root compares generated ordinary handlers outside the declared set and
   runs constructor, monitoring, debugger, release/debug, and native ownership
   qualification before treating this as implemented. This document is an
   interface proposal, not evidence those gates have passed.
