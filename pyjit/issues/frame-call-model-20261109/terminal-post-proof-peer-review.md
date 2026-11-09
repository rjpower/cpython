# Terminal POST proof interface peer

Design-only review of `terminal-post-ownership-proof-proposal.md`; no header,
constructor or runtime edits are admitted here.

The generated per-edge declaration must come from A's checked terminal commit,
not a handler-name list or event bit. Reusing actual output_kind, roles, shared
read/definition helpers and the existing completion worklist is the right
boundary. Constructor and debugger consume one physical contract, while target
context/origin planning remains distinct from physical validity.

The first proposal incorrectly allowed a producer's replacement output to clear
an old-owner obligation at the same home. This was corrected before admission.
A new non-NULL value cannot stand in for an old receiver/cursor expected by a
handler. Keep that seed obligation even if physical publication succeeded. A
later actual handler definition can discharge it. Fresh output at a distinct
home, and explicit defined NULL versus absent output, are separate physical
facts. No existing source-row tuple attests that an aliased old-home reader
expects its replacement.

The amended proposal preserves this distinction. Its conservative rejection of
old cursor reads and OR_POP input/output alias reads is sound in direction.
Compute PRE exceptional requirements must remain independent of committed POST
callback exceptional requirements. Joining them by applying output definitions
first would reintroduce the same flaw.

One physical geometry condition extends beyond the FOR example. Existing
IR_MOVE_TO_DST and IR_MOVE_TO_DST_OR_DISCARD move a window result only for local
A; NoDst detaches it. For any admitted WINDOW_DST_FIRST word, a temporary A must
therefore equal the original physical result home B+fmt.dst, unless the new
generator commit explicitly implements a different mapping. FOR's semantic
OUTCOME needs its equivalent A==B+2 check. Shared DefinesHome metadata does not
by itself validate arbitrary encoded A. Use fmt.dst, not through, to locate
the moved first output; other new window tail outputs remain B-relative.

Disposition: amended ownership split approved in direction. Exact selected-home
transfer, checked geometry, declaration generation and PRE/POST worklist seeding
still require actual source review. No generic POST authorization or constructor
qualification is claimed from this design peer.
