# Delegated receiver consumers

Source packet: GC lane after `ff85778402c`, source only. The common selector
and phase interface are frozen; the consumer changes await combined source
review and root validation with IR admission and frames publication.

The canonical delegated form uses A for the resume recipient, B for the
outgoing value, C unused,
and D for the retained borrowed receiver. Ordinary YIELD
has no receiver operand. Current YIELD metadata leaves D unused. D must be
remapped as a real input through allocation and verified against all incoming
SEND identity facts; neither its bounds nor its proximity to A proves that
identity. The terminal event also proves that the outgoing owner is detached
and A is a valid resume recipient.

The cold selector must match the reached delegated phase, ready executing-code
image, actual saved continuation, canonical delegated word and its A/D roles.
It returns receiver identity only when D has a positive finite readable-role
certificate and the live return_slot agrees with the certified A. This does
not require or grant full suspended-home MUST authority: incomplete whole-home
views remain NONE/candidate/explicit NULL control. Reading D directly
from that matched canonical word needs no saved-D frame field or protocol row.
All canonical delegated sites require positive admission. Unknown/reset phases do not
fall back to return_slot arithmetic or to the NULL-control contents of a cell.

| Reader | Baseline behavior | Consumer change |
| --- | --- | --- |
| `Objects/genobject.c:_PyGen_YieldFromRef` | Reads `base[return_slot - 2]` | Use the matched explicit D role; report lookup failure before reading a home. |
| `gen_getyieldfrom` (`gi_yieldfrom`, `cr_await`, `ag_await`) | Tests suspended state, converts borrowed home | Hold a suspended read capability through lookup and reference acquisition. Nondelegating state returns None. |
| `gen_close` | Claims EXECUTING, then reads receiver using prior state | Supply the successful claim's saved delegated state; keep old protocol valid through the delegated close callback. |
| `_gen_throw` | Claims EXECUTING, may call the receiver's throw and restore suspension | Use the same saved-state claim. Preserve the old protocol when the outer frame has not resumed; invalidate before actual resumption/mutation. |
| `Modules/_remote_debugging/asyncio.c:get_awaited_coro_address` | Uses saved return_slot minus two after owner/code bounds checks | Match suspended state, phase, image and canonical delegated word; read D with independent bounds/overflow checks. Keep remote access failure handling and the stopped-target/read-lock policy. |

The successful EXECUTING claim is not itself a mutation of suspended homes.
Close/throw can run Python on the delegate before resuming the outer frame;
invalidating the phase at the claim would discard the root/read authority
needed during those callbacks. Conversely, a captured receiver index cannot
authorize a read after a callback has resumed or redirected the frame. The
selector and reference-acquisition boundary must stay callback-free. Resume,
clear and accepted redirect invalidate before the first relevant mutation.
Future GC selection during claimed-but-not-resumed callbacks must follow this
same lifetime under its existing quiescence policy.

The free-threaded getter claims
`FRAME_SUSPENDED_YIELD_FROM_LOCKED` only from delegated suspension, waits for
another reader, and restores the suspended state after acquiring its own
receiver reference. Resume already waits on that claim. Concurrent read safety
still requires root's free-threaded validation; a phase check alone does not
establish it. Existing concurrent-yieldfrom and close-race tests exercise this
boundary.

The GC lane can own genobject.c receiver lookup plus close/throw/getter callers,
remote asyncio.c and the necessary debug-offset fields. IR owns the canonical
family, A/B/C/D metadata, identity proof, admission and common cold selector;
frames owns suspension publication, capture/resume/clear and observer ports.
The remote descriptor adds phase_state, the ready executing image and its
word count. The reader matches those with the immutable canonical word,
preserves PC alignment and integer overflow checks, then reads D. Descriptor
size validation rejects incompatible layouts. There is no legacy
slot-subtraction path after the port.

Root validation should exercise delegated generators, coroutine/asyncgen
await, delegate close/throw callbacks inspecting and collecting the outer
frame, restored suspension after delegate throw, accepted/rejected redirects,
and remote await chains. Admission controls must distinguish a moved valid D
from an arbitrary in-bounds old-position object, conflicting joined SEND
identities, missing D and a changed continuation. Existing ordinary YIELD and
discarded resume-input forms remain separate admitted contracts.
