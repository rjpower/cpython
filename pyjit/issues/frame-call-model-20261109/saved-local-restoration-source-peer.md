# Saved-local restoration source peer

Actual source review, before native admission. The per-local action and callback
transaction are coherent, but one target-binding safety issue remains pending.

Checked chain traversal validates producer opcode/local identity, requires a
surviving origin for every active token, and refuses any producer reentry cycle
using the request-owned sparse graph. The oldest same-local token becomes the
source. Named-local kind excludes cells/free/constant homes, sources cannot be
constants, retained target needs or another action's source. Array allocations
are bounded and freed on refusal/error. Warning callbacks trigger complete
reanalysis unless newer redirect generation already won.

Application first detaches action sources, displaced locals and all surplus
scratch owners; then installs all restored bindings and commits PC/cursors/intent
before closing. There are no frame/slot writes after the first close. Pending
child requests have zero restoration actions and defer source-error invalidation
to actual CompleteRedirect. Identity/ordinary actual commits invalidate exact-frame
recipes before callbacks. The shared scope consumer retains PRE-only authorization
and does not infer POST/yield ownership from event bits.

## Pending target-binding safety

A genuinely NULL saved binding is restored without None filling, correctly
preserving an unbound local. However `analysis.safe[target]` is a scratch-owner
fact, not proof that target code checks this named local before reading it.
LOAD_FAST and LOAD_FAST_BORROW explicitly assert a non-NULL local; folded IN fields
also rely on the compiler's bound-local proof. Consider a function whose earlier
`if flag: x=1; return x+1` target normally has x definitely bound, while flagFalse
executes `[x for x in [1]]` with original x unbound. A PRE jump from the active
comprehension to that earlier target can restore NULL into an unchecked read.

The first action needs a target-before-definition binding-safety check or
conservative refusal for that NULL restoration. Checked LOAD_FAST_CHECK and
locals inspection can remain valid positives. Filling None instead would violate
the promised restored binding state. This concern was sent to B/root before
native admission; actual disposition and regression must be appended separately.

## Repair disposition: source admitted

Reviewed the actual shared `_PyIR_BindingRead`/`_PyIR_DefinesBinding` helpers and
`register_unbound_target_safe` implementation. Every explicit or window alias
and named-local read role is checked; a required alias defeats NULL tolerance.
Only audited checked reads accept the unbound state. The BFS retains that
obligation through checked reads and ordinary PRE-error edges. A certified
non-NULL semantic output may discharge it on a normal or source-POST edge.
Ordinary STORE_FAST is deliberately not a discharge: its source can itself be
NULL. This is conservative and does not assume physical output means bound.

The check runs before adding an action only when the selected saved binding is
actually NULL. Request allocations are checked and freed on every result, and
refusal has no frame effects. Raw and folded compiler-proven-bound target
negatives assert that failed jumps preserve PC, line and the active binding.
The existing checked-read unbound positive remains. No concrete source blocker
remains for native admission. Native results are a separate coordinator gate;
this review did not run a build, regeneration or candidate Python process.

## Binding semantics correction

The preceding admission missed a semantic distinction identified by the
coordinator before native execution. A C operand's NULL capability does not make
it a checked Python unbound-local read: STORE_ATTR/GLOBAL interpret NULL as
deletion, and an optional CALL self slot interprets it as receiver omission.
Therefore `_PyIR_NullableHome` cannot justify CHECKED_NULL here. Admission was
withdrawn pending a narrow helper correction: physical IN/window aliases require
a bound value; exact LOAD_FAST_CHECK and DELETE_FAST named-local roles remain
checked reads. An attribute-assignment target must refuse without deleting the
attribute. The original review is retained above as failed-round evidence.

Actual narrowed source re-reviewed: every physical IN/window alias now returns
REQUIRES_VALUE, with no nullable-capability shortcut. Only the two exact checked
named-local operations can return CHECKED_NULL. The regression adds an
attribute-assignment target and preserves a sentinel attribute, plus the refused
frame PC/line/binding, so NULL cannot silently become deletion. Raw and folded
unchecked-target negatives remain, and the checked unbound positive asserts its
actual checked opcode. This exact repair is source-admitted for the coordinator's
native gate; no remaining concrete source blocker was found.
