# GC peer of selected-home copy and capture

2026-11-12. Bounded design GO for `copy-home-view-proposal.md`, with the
conditions below. This is a schema/lifetime peer, not source or M4 admission.

The 16-byte code-pinned capture avoids a new takeover allocation. The explicit
CAPTURED state, all three copy callers, dynamic current local bindings and
saved retirement cursor are the right boundaries. Active phase lookup must
not reconstruct a retiring or unsupported suspended shape from an old PC.

The record union must use exact kind discrimination at every producer and
consumer. EXTERNAL reads only its optional prefix; RETIRING/STAGED reads only
its home view. Empty provider nodes and metadata-only caller_staged barriers
have no home-view authority. Keep the initialized-mask and NULL-node assertions.
Do not infer a view from nonNULL frame or code fields.

Borrowed ranges remain code-owned. Finish the final cursor/view access and
disarm the record's view pointer before releasing its fixed-last code pin.
The linked record can then continue rooting a C result during code-constant
finalizers without touching a dangling descriptor. Captured heap ranges stay
valid through their independent execution-code pin and must be replaced or
invalidated before resume, redirect, takeover or other geometry mutation.

IR's current nullable joined ranges are owner-candidate ranges under initialized
NULL control. MAY_OWNER union does not imply must-readable initialization on
each path. They are useful shadow/copy controls while NULL fills remain, but
cannot authorize stale-cell loads without normalization or explicit validity.
Resolve that distinction in the view contract before the M4 switch. Typed
index/control roles also need their actual contract before a generic language
value view covers them.

Copy current local homes independently of the descending retirement cursor:
callbacks can write already passed bindings. Clip scratch ranges using the
saved actual cursor. Preserve edge uniqueness during source disarm/destination
publication and keep arena storage reserved through the last source access.
Root freezes the first real copy/collector consumers before this union changes;
no unused schema or permanent NULL fallback is approved here.
