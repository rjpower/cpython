# Canonical list-child result: source audit checkpoint

Proposal only; defer implementation until after isolated shared-completion
measurement. A list-item duplicate is a plausible canonical source, but the
current proof cannot accept it merely by adding a helper name.

## Positive source boundary

`Objects/listobject.c:PyList_GET_VALUE` returns a borrow of the actual PyValue
array cell. `PyValue_XDUP` preserves its bits and retains its count before list
retirement. A live language list cell is intended to be a canonical home:

- `PyList_SET_VALUE` checks language/NULL and `_PyValue_CheckHome`; public
  `PyList_SET_ITEM` first calls `PyValue_PrepareOwned`, which canonicalizes the
  stolen object through `PyValue_FromObjectSteal`.
- `_PyList_SetItemTakeValue`, insertion, append and the steal-on-success list
  constructor install owned values and check their home representation.
  Object-ABI insertion borrows through canonical `PyValue_FromObjectBorrow`.
- Slice/copy/concatenate/repeat and fast extend preserve existing list/tuple
  home bits with DUP or count-adjusted copies. Sorting/reverse/memmove and
  `_heapq` swaps transport whole PyValue cells, not object pointers.
- The raw specialized STORE_SUBSCR write takes a canonical frame input;
  UNPACK_EX detaches list cells to NULL. Other core writers use SET_VALUE and
  canonical frame/container/ABI inputs. These input contracts remain trusted
  API obligations; a raw noncanonical extension write violates the home ABI.

The audit searched raw `ob_item`, `_PyList_ITEMS`, SET_VALUE and SET_ITEM uses
across Objects/Python/Modules/Include. It inspected the principal array writes
and moves above, not every transitive container producer. Debug assertions
detect a broken home contract; they do not themselves canonicalize release
values. Admission must explicitly accept the shared list-home invariant, just
as the prior local-home proof accepts canonical frame locals.

## Exact source shape required

The current list-int handler consists of integer guard, exact-list guard,
feedback padding, a bounded-index producer, integer retirement and list
retirement. `canonical_result_source` currently starts at the first uop and
recognizes one pure local duplicate or exact numeric producer chains. Extend
that existing function with a complete body/stack-effect proof for this chain;
do not infer it from BINARY_OP_SUBSCR_LIST_INT or PyList_GET_VALUE token presence.

Prove both guards and the negative-index/bounds exits precede any consumption;
prove exactly `result = PyValue_XDUP(PyList_GET_VALUE(list, index))`, non-NULL
assertion, exact input pass-through and INPUTS_DEAD; prove the two suffixes
consume only their respective index/list owners and preserve the result.
Exclude named caches, helper shadowing/rebinding, preprocessor ambiguity,
result aliases, arrays, extra calls and unproved parts. Reuse CanonicalResult
and SlotLayout; no new opcode metadata or parallel capability flag.

## Publication and reentry constraint

The generated handler currently duplicates the child before consuming inputs,
publishes a temporary result before closing the list, then performs final DST
publication. A local result stays C-owned until final publication; NoDst retires
the duplicate. A list's last-owner destruction can run arbitrary sibling
finalizers, unlike the existing numeric retirement suffix. The canonical-result
fact must not suppress that early temporary publication or alter existing
displaced-local/alias/error behavior.

The smallest candidate changes final publication only. It leaves the early
IR_STORE_TEMP_DST intact, even though that still checks canonical bits. Extending
early flush to use canonical publication requires a separate exact lifetime
proof: publish only after the duplicate exists, preserve temporary ownership,
retain local/NoDst selection and never write a stale result after an admitted
redirect. The current generated tail must not be mistaken for proof that every
close-time setter is supported. Reentry authority remains its existing scoped
contract; no expansion is part of this optimization.

## Focused admission tests

Use a renamed source-identical handler positive and mutations for raw borrow
without XDUP, duplicate after list retirement, changed bounds/negative-index
order, helper/cache shadowing, result/retired-owner alias, suffix rebind or opaque
callback, and early publication removal. Runtime selectors need local/temp/
NoDst, negative indices, misses, inline numeric/big-int/boxed-NaN/object children,
self-containing lists, last-owner list with sibling finalizer, displaced local
and source/destination aliases. Assert logical result/owner behavior and eventual
collection, not prompt finalizer timing.

The 932→1225-byte compiled list-subscription difference is an observation about
shared macro/layout expansion, not attribution to a redundant publication.
Record exact affected generated handlers and compiled bytes/spills before any
performance claim. Shared completion is the next admitted experiment; this
proposal remains deferred and requires independent full-chain proof review.

## Concrete recognizer implementation plan

Keep the existing common eligibility checks in `canonical_result_source`:
scalar explicit DST, one fresh external output, no output alias, no arrays,
only Uop/Skip parts, unnamed cache padding. Before its current first-uop numeric
and local-source branches, add one bounded five-uop chain recognizer. It returns
the existing `CanonicalResult(producer, result)`; no new tag or runtime flag.

1. Require exactly five code-emitting uops. Identify the middle producer from
   its third position and its two-input/three-output stack shape, not its name.
   The complete external shape must be two inputs and one fresh result. Require
   the first two guards to pass both original operands through unchanged; the
   suffixes consume top index then list, leaving only that same result owner.
   Verify this through the parsed stack mapping, not equal variable spellings.
2. Compare complete comment-free bodies against five exact templates. The
   first guard only exits for non-inline-int. The second extracts the lower
   operand and exits unless exact list. The producer extracts list/index,
   adjusts a negative index by size, exits on bounds, duplicates the list-cell
   borrow, asserts non-NULL, records the hit and passes original inputs to its
   two retirement outputs. The suffixes are exactly the checked-int specialized
   close and nullable list-owner close. Alpha-substitute only verified stack
   input/output names; keep C helper bindings and local declarations explicit.
3. Audit trusted helper names against every input/output/cache binding and all
   local declarations. Reject preprocessor tokens and any extra declaration,
   call, assignment, result rebind or unsupported part. Guard-local `o` and
   producer locals `list`/`index` must not collide with stack owner names. The
   known global helper boundary includes list access/size/type tests, XDUP and
   exact retirement helpers. A source-identical renamed instruction remains
   eligible; a familiar handler name with changed code does not.
4. Set only SlotLayout.canonical_result through the existing return value.
   Final emission chooses IR_STORE_CANONICAL_DST through its existing shared
   destination/owner logic. Leave nonfinal `stack.flush()` calls at their
   default canonical_result=None, so the existing early IR_STORE_TEMP_DST
   remains byte-for-byte unchanged. Do not globally propagate this fact into
   flush or change either store macro in this packet.

This final-only implementation leaves some redundant temporary publication
work deliberately in place. Its measurable scope is one ordinary specialized
handler's final store; it neither rewrites list retirement nor claims to fix
the full 932→1225-byte expansion. Existing local/NoDst/temporary aliases and
displaced-owner retirement follow the shared store exactly once.

Host tests should execute the recognizer and emitter for the real chain, a
renamed equivalent, local/temp/NoDst formats and each mutation above. Pin the
early generic store and late canonical store separately. Regeneration must
change only the expected final store in the one specialized handler; the
existing four numeric and two local-load canonical producers and all observed
providers remain unchanged. Root owns that generated diff, native safety
selectors and isolated component measurement. No implementation is admitted
by this plan.
