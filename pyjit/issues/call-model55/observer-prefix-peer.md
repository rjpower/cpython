# Observer prefix proposal peer review

2026-11-12, source review of [observer-prefix-proposal](observer-prefix-proposal.md)
against the selected runtime's `frame_lineno_set_impl`. Disposition: GO for the
bounded adapter. No native or executed evidence is claimed.

The existing commit detaches surplus scratch and displaced local owners into
the allocated array, installs restored bindings, publishes the continuation,
then closes the array. The proposed prefix describes those actual owners
without changing allocation, jump admission, owner commit or active-frame
mutation semantics. Earlier fast-closed entries are excluded from `begin`;
advance the cursor before each later close and unlink before freeing storage.

A prefix-only record has zero home/header/result authority. Its visitor must
not dereference the associated active frame, derive geometry or reconstruct old
bindings, including after nested redirect or takeover. The active frame follows
its normal current authority independently. Push/pop should validate only the
prefix bounds/exhaustion and the record's claimed authorities; they must not
infer home authority merely because a frame pointer is present.

The optional descriptor pointer is reasonable for this first real dynamic
consumer: ordinary handlers gain no record, and existing outlined records gain
one word. This is an explicit cold-storage cost to check in native frames.
Retain all-mortal slow routing under FT, whose concurrent decrements invalidate
a GIL-only last-reference proof. The prefix is external marking, never an extra
container subtraction edge.

The proposed actual line-jump Trigger/Payload fixture meaningfully checks the
precise bridge independently of conservative discovery. Also require nested
redirect/mutation to leave newly installed frame owners intact and the chain
empty after completion. Existing observer admission failures must remain visible.
This review does not establish dynamic unpack or full helper-root coverage.

Source post-review: GO for the four-file patch in `call55-gc` over component
`530f26f084e`. The noinline continuation links only on a potentially reentrant
close, zeros home/header/result authority, advances the prefix before close and
unlinks before array free. Neither prefix visitation nor its push/pop checks
read frame geometry. Designated initialization leaves the new descriptor NULL
in existing producers. FT counted owners always route slow. The real line-jump
fixture inspects precise Payload IDs, collection, optional local mutation and
chain exhaustion; candidate execution must still prove the recipe is admitted.
The worker's diff check and host parse are not candidate runtime evidence.
