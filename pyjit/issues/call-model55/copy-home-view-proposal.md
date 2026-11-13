# Selected-home view and copy/escape packet

2026-11-12. Bounded design for the next M4 preparation packet. No source edit
until IR/GC/root freeze the view and durable-capture interfaces. Shared storage
and initialized NULL named locals remain selected. The current full-N copy
has three callers: retirement `take_ownership`, RETURN_GENERATOR, and public
`gen_new_with_qualname`. They have different lifecycle authority.

## One view, explicit adapters

Use IR's proposed const `_PyIRHomeView` and `_PyIR_GetFrameHomeView(frame,out)`;
the reached phase shadow consumes that query first. Propose compact fields:
uint16 nbindings, uint16 scratch_limit, uint32 nranges, and const
`_PyIRPhaseRange *ranges` (16 bytes on 64-bit). Limits/ranges are scratch-relative
0..S; the iterator adds nbindings once. Named bindings are always the separate
initialized/dynamic [0,L) domain, including NULL and hidden constants. This is
readable-value authority, not a mortal bitmap. Extend explicit range roles when
index/control families are admitted; current nullable canonical-home roles do
not certify tagged-index protocols.

Lookup validates executing code, image, canonical site and published phase,
then selects the immutable range row whose bounds the builder establishes.
Adapters separately validate their cursor/geometry before any home read.
Borrowed ranges live under the independent
code pin. A single const range iterator/selector supplies copy, GC and teardown;
those consumers retain their distinct actions/tag/count policy. There must not
be another opcode decoder in frame.c. Local boundness is not frozen by a view.

The current IR query supplies a nullable owner-candidate NULL-shadow shape.
MAY_OWNER union alone does not prove every joined word initialized once stale
scratch is admitted. Actual copy authority requires the separate must-readable
proof or explicit branch normalization/validity. The common representation can
be shared now, but its current shadow coverage must not be relabelled as that
stronger proof. All source uses remain NULL-control preparation until this
distinction is certified.

Retirement supplies a separate explicit adapter from the saved entry view plus
`roots.remaining`; it cannot call active lookup after phase reset or infer a
shape from RETURN. Save the view before reset/progress. Reuse the record's last
pointer as an explicit kind-discriminated union: EXTERNAL prefix versus
RETIRING/STAGED home-view pointer, with NULL-only empty provider state. This
keeps 40 record bytes and ports every union consumer. The agreed two-bit
authority field uses NONE/SHADOW_CANDIDATE/MUST_READABLE/NULL_CONTROL_READABLE.
Current initialized-prefix control uses authority NULL_CONTROL_READABLE plus a
NULL view pointer, avoiding an extra producer-local descriptor. A later static
MUST_READABLE producer needs a priced 16-byte const view in reached outlined
storage. Empty/metadata-only nodes use NONE plus NULL. A retiring range is
clipped to remaining scratch;
external marking of remaining locals clips to its actual cursor. Takeover
copies all initialized current local homes, including NULLs, rather than
inventing boundness from the cursor. Mutable live tp_clear remains separate.

## Durable escaped state

Put one inline `_PyIRHomeView f_capture` in PyFrameObject: 16 cold bytes with
the compact form above, no extra hot interpreter-frame field or allocation at
takeover. Its ranges borrow from the captured frame's independent code pin.
This works for partial retirement because the capture records the actual
remaining range/cursor, not an old PC. An extra allocation during takeover is
undesirable: an existing observer must not lose owners if that allocation fails.
The common iterator clips ranges to scratch_limit before reading; an original
code range may extend beyond a partial-retirement limit. During NULL shadow,
an explicit full-control adapter may use a shared immutable all-nullable range
clipped to the actual admitted S. It must never capture a pointer to a temporary
stack-constructed range. Remove that development control before M4 authority.

Define an explicit CAPTURED lifecycle bit in reserved phase_state space, separate
from PUBLISHED/status; do not guess from nonNULL range pointers. A frame-owned
heap frame derives its owning PyFrameObject by the existing embedded-layout
ownership proof. A generator copied from a public frame already keeps that
PyFrameObject in frame_obj, so it can retain this same durable capture without
another generator header/allocation. Active associated frame objects do not
become a second home owner. One common selector dispatches CAPTURED explicitly;
the code-phase getter must continue rejecting that bit rather than accepting
arbitrary reserved bits. Before resume installs a sent owner or otherwise
changes this captured state, clear/replace CAPTURED in the callback-free commit
and publish the actual created/resume/error shape before any reentry. An
unsupported phase must never fall back to an obsolete capture.

The bounded NULL-control packet invalidates CAPTURED at clear/resume entry;
it does not claim static heap-clear authority after that invalidation. A future
selected clear must update its captured scratch limit before callbacks.
Already passed mutable local bindings remain in [0,L), so a
later observer write is still a heap edge. Code closes only after the final
view read; clear the saved record view before final code detach. After its bit
disarms, roots/query/free paths read no borrowed ranges. Empty provider and
metadata-only caller_staged nodes have no home-view authority.
`sizeof`, frame layout/debug offsets and remote/GDB readers get the cold-object
offset changes together. Source/location-only readers still need no home decode.

## Actual copy routes

| Caller | Required authority and publication |
| --- | --- |
| `take_ownership(frame,roots)` | Build the copy view from the saved retiring view/current cursor, before detaching source headers. Install destination f_capture and CAPTURED, transfer selected homes/header owners, preserve independent source code pin, then disarm source range/header bits and publish f_frame/FRAME_OBJECT before any callback/tracking. No active-phase lookup after reset. |
| RETURN_GENERATOR | Use an explicit created shape from the event/entry contract: dynamic NULL locals and the actual retained scratch roles. Advancing instr_ptr must select the new matched created state, not copy old PRE. Install destination before tracking/publication; arena source has no owners before pop. Normal creation needs no cold capture object solely for maps. |
| `gen_new_with_qualname` / public frame-to-gen | Resolve the source heap capture/created view, copy it with actual values, retain its existing associated PyFrameObject as durable captured authority, and publish FRAME_CREATED. Do not assume arbitrary caller-supplied heap frames have the ordinary function-entry scratch shape. Resume must explicitly replace that captured state. |

Make `_PyFrame_Copy` require the explicit const view, and update all three
callers. Copy L locals (including NULL/inline values) and selected readable
scratch; NULL source homes only for actual transfers under the same commit.
Do not load, type-check, close or copy inactive source scratch. Destination
inactive scratch is initialized to NULL while NULL shadow control remains;
the M4 switch may omit that fill only with complete destination authority.
No callback/poll occurs between destination stores and source disarm. Physical
overlap/clobber reservation is unchanged, and source arena stays reserved until
its actual pop. Header moves preserve the M1 strong code/function contract.

## Staging and review gates

This packet cannot equate today's status-only query with complete copy coverage.
While NULL control remains, unsupported source states need an explicit declared
NULL-control view and shadow accounting. They are a development control, not
a permanent fallback permitting M4 scanner removal. First useful actual mapped
cases are explicit created generators and captured retirement; all arbitrary
suspended/public-frame states must receive a matched view before the switch.

The source census and P0/P1 queue are in `lifecycle-readers.md`. Required peers
are IR for view/roles/lifecycle bits, GC for the record union/selector and heap
edge partition, and frames for the three copies. Freeze interfaces first, then
port one real caller with shadow coverage before broad reader conversion.
Tests need partial retirement takeover with a surviving scratch owner, inline
and NULL roles, nested finalizers/collection, code mutation, created and
suspended generators, public frame-to-gen, and observer-local writes. A precise
root-ID survival test alone is insufficient: compare copied logical values and
each unique edge, and prove skipped source homes were never decoded. Root owns
generation, both-role tests and native copy/header/record costs.

IR and GC bounded design peers are GO, with the shadow-versus-readable
qualification above. Both require explicit lifecycle dispatch, correct union
discriminants/cursor protocol and clearing borrowed views before final code
close. Root has frozen the authority and union interfaces. The first source
packet is concrete on the integrated phase base: all three copy callers use
the explicit view API, copy uses one shared range iterator, retiring takeover
uses its actual cursor, and cold frame capture adds 16 bytes. Current created
and unsupported suspended states deliberately use the named NULL-control
adapter; they are uncovered static authority, not M4 fallback. Five focused
fixtures cover selected-home poison avoidance, escaped locals/code mutation,
ordinary generator suspension/escape and public frame-to-generator capture.
The zero-binding escaped-frame case has explicit empty control authority.
RETURN_GENERATOR passes two frame pointers to an outlined helper owning the
16-byte addressable view, query, PC advance and copy; it adds no addressable
interpreter-handler local. Coordinator generation/native validation remains
pending.
