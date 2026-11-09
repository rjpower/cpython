# Canonical ownership transfers: source proof

2026-11-09. Source-only proof for the proposed ABI increment; no native receipt.

## Return

IRStack's input comes from a canonical frame home. The checked
`_MAKE_HEAP_SAFE; _RETURN_VALUE` chain takes that owner without changing bits,
holds it in C while retiring the child, and publishes it to the caller.
The instrumented chain similarly holds the taken value across its callback.
`canonical_return_input` checks both chains' actual token assignments and fails
closed to ordinary publication if their value provenance changes. It does not
infer canonical bits for native or SEND results by instruction name. Host full
generation chooses canonical return publication for exactly RETURN_VALUE and
INSTRUMENTED_RETURN_VALUE. Their explicit source is still owned until the
recipient consumes it, including IR_NO_DST. Frame cleanup cannot release that
separate C owner.

## Window to local

Before the edit, a source scan of all 33 generated IR_MOVE_TO_DST and
IR_MOVE_TO_DST_OR_DISCARD sites found IR_PUBLISH_TEMP on the same window cell
before each move. This includes native call specializations and FOR_ITER.
IRStack final flush publishes each window scalar output; a preexisting window
cell is already a canonical frame home. The final move therefore needs no
second exact-long/float classification or number canonicalization.

The new move asserts the source home invariant, copies the owning bits, clears
the old cell and retires the displaced local as before. Discard still clears
the source cell before release. This does not remove canonical publication at
raw/native production, or assume that an arbitrary C PyValue is canonical.
Callbacks between initial publication and the move cannot introduce a
noncanonical frame home through supported APIs. Their existing revalidation
and continuation rules remain necessary and are not weakened by this change.

## Unpack

The three tuple/list specializations copy PyValues from canonical container
homes; TWO_TUPLE additionally passes through IR_PUBLISH_TEMP. They now pass an
explicit compile-time canonical fact to the inline local mover and the
out-of-line finish_unpack_to_locals fallback. Both move the source owner,
clear the window cell and preserve displaced-local retirement and errors.
If a previous retirement fails, the fallback closes each remaining window
owner without storing it, as before.

Generic unpack remains conservative. Its _PyValue_IterNext object adapter uses
PyValue_FromObjectSteal, but the direct value iternext slot can return a PyValue
without local normalization. This pass does not assume that every external
value-slot provider satisfies an unstated stronger provenance contract.
Generic UNPACK_SEQUENCE/UNPACK_EX retain publication on transfer. No tuple/list
alias was added merely to select this fact: existing producer identity supplies
it to the shared mover.

## Scalar sinks

A fresh single-result producer may use IR_NO_DST. Before input retirement,
temporary publication skips that sentinel, keeping the result owned in C.
Failure cleanup treats the unpublished result like a folded-local result and
closes it once. Successful final storage releases it instead of indexing a
slot. The dictionary first-probe path follows the same rule. A temp-only shape
alias cannot be selected for IR_NO_DST; ordinary shape handling retains sink
support. Multi-output producer heads are excluded from discard capability,
and compiler lowering must prove unique fresh-result use before selecting it.

Window producers can still publish into their real canonical window before
final sink retirement. That is remaining slot traffic, not an executable POP
boundary, and is not credited as an eliminated store. Direct Python/native
call completion instead uses the shared C-owner recipient protocol.
