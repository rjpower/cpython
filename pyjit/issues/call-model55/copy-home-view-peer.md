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

## First source packet

The nine-file frames packet has bounded source GO, including its final
zero-local NONE takeover adapter and outlined created-copy helper. The latter
keeps its addressable view in a NO_INLINE frame.c helper rather than a
tail-called handler. All three copy callers supply explicit authority; current
staged/retiring producers use declared NULL-control prefixes and disarm before
code detach. The shared visitor uses the existing NextHomeRange iterator for
MUST ranges. Candidate authority never permits that selected read.

The private poison-word probe exercises the actual selected-copy primitive,
balances the single payload/code owner and skips inactive source words. Heap
escape, code replacement, local values, created/public generators, resume and
clear fixtures cover actual lifecycle adapters. Current production copies
predominantly use NULL_CONTROL_READABLE; the probe is not evidence of complete
mapped entry/suspend coverage. Root owns generation and native validation.

The separate executing-code allocation proposal and four-file source change
also have GC GO. Kind and capacity must use the frame's pinned code through
MakeCoro/make_gen, while names remain function metadata. The real PEP523
predefault fixture replaces a large executing generator's function code with
a small ordinary/generator image, then checks original kind/code/high local
and release through both provider routes. The proposed created certificate
remains separate from that correctness fix and the CALL packet.

## Empty-created certificate source

The frames six-file consumer/counting packet and IR's isolated five-file
221-line staged event packet have bounded GC source GO. The fast copy uses the
shared header mover, transfers dynamic locals and fills destination scratch
without reading source scratch. Its addressable control view stays in a
separate outlined continuation. Only the whole-image all-site certificate
selects this path; public frame-to-generator copies do not use it.

The actual marker is guarded by the full RETURN_GENERATOR body and IO shape.
Its terminal ownership event requires complete incoming proof, physical NULL
and no initialized or possible-owner scratch. Unreachable physical markers
remain in the denominator. Exact MAKE_CELL/COPY_FREE_VARS bodies preserve only
scratch facts; common admission now checks COPY_FREE_VARS against nfreevars
before granting that local-only contract. Static flag-only plans keep the
existing allocated-plan free/size distinction. Tests cover body perturbations,
mixed sites, recovery entries, large physical extents and real created kinds.
These are source dispositions; root owns combined generation and native checks.

The follow-on created-selector proposal also has bounded design GO: consume
only an explicit producer marker with generator ownership, atomic CREATED
state and an already ready execution certificate. CAPTURED takes precedence,
and phase zero cannot reacquire authority. The two collector visitors may use
the common iterator under their existing value policy, with FT selection only
while the world is stopped. Other lifecycle states retain explicit NULL
control until separately ported. No deferred-reference or final M4 claim follows.

The two existing collector visitor ports now have frames' actual source GO:
exact marker equality, common MUST selector, shared iterator, zero-local empty
case and unchanged per-value tag/count handling. FT tests world-stopped before
reading the marker or resolving the view. The addressable 16-byte view can
affect the collector function's prologue even on ordinary traversals; root
must account for that native cost. Source is frozen separately from CALL,
pending combined marker/producer/getter integration and precise visitor tests.

The frames producer/getter/test follow-on (+97/-3) also has actual source GO.
The marker follows destination initialization; the getter has no callback or
allocation and checks the ready image and atomic generator state. A private,
untracked embedded-generator probe now poisons inactive destination scratch
and invokes the actual GIL collector visitor, counting only the selected
payload. It supplies the required containing generator and never executes or
closes poison. FT does not forge a stopped world for this probe. The real
generator fixture covers named-local mutation, collection, resume invalidation
and a callback-free zero-marker denial control.
