# GC proposal peer review

Reviewed [gc-proposal](gc-proposal.md) against selected54 frame ownership and
allocation, 2026-11-12. Source review only; no builds or runtime evidence.

M1 disposition: GO. Typed header fields are counted strong owners. Ordinary
object traversal is appropriate; FT deferred-stack adjustment must not add a
count for them. NULL entry sentinels require owner-before-code checks in remote
and debugger readers. Both pins remain independent after function code mutation.

M2 disposition: the nested record and external-root distinction are coherent.
The revised proposal resolves the following schema condition. The record derives the
remaining-owner map from the retiring frame's code; that independent strong code
pin must remain until the last map reader and remaining owner operation has
finished. Define code release as the final header action. A header mask alone
must not allow early code release. Optional marking visits the remaining code
pin until that action; subtraction never treats the record as a heap container.

Record/frame/prefix storage remains reserved until LIFO pop, including nested
callback chunk growth. The allocator must retain the retiring reservation and
restore its saved frontier only after pop; add assertions at that boundary.
No scanner may infer roots from bytes in that reservation. Prefix cursors and
detached-root bits advance before closing; initialized installation precedes
publication. Escaped frames need separate heap ownership before retirement
starts, and observer writes must not insert owners behind a cursor.

These conditions permit NULL-authoritative M2 development, not stale scratch or
removal of conservative roots. Focused nested collect/recurse/escape/mutation
tests and source/native post-review remain required before integration.

M1 source post-review covers the seven changed GC/observation files. Typed
clear/traversal, optional FT marking of both pins and no deferred count addition
match the contract. Requested one correction to remote validation: retain
high-address rejection before interpreter-owner bypass, then reject NULL code
after owner recognition. This preserves torn/reused-pointer rejection while
allowing NULL entry sentinels. Compilation and executed evidence remain pending.

M2 GC source post-review: GO for the four-file head/header/visitor patch,
including the actual generator-exception bit16 addition. The visitor enumerates
the nested chain, direct initialized prefix, independent typed pins, original
generator exception and optional result. It does not decode geometry after code
detach, and its callsites perform external marking only (GIL precise traversal
and optional FT marking). Deferred-count adjustment and subtraction remain
separate. The new header is untracked in the worker and must be included when
the root copies the patch. Native compilation and producer/collector execution
remain root-owned admission gates.
