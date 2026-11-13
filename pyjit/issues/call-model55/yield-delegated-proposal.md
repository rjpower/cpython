# Explicit delegated yield

This is the frozen shared interface, reviewed with the GC and IR lanes.
Source packets are being implemented separately; root owns combined generation
and native validation. C is unused unless the
actual format requires a fixed value, and logical cursor lifetime never
imposes a contiguous physical window. Consumer details are in
[yield-delegated-consumers.md](yield-delegated-consumers.md).

## Canonical word and logical operands

Introduce `YIELD_DELEGATED` and its instrumented twin. Ordinary `YIELD_VALUE`
keeps its separate nondelegating contract. A delegated word remains eight bytes:

| Field | Meaning | Ownership |
| --- | --- | --- |
| A | Resume recipient | No sent value exists while suspended. Scratch must be vacant; a named binding remains dynamic until actual resume publication. |
| B | Outgoing yielded source | TAKE consumes the proved SEND result scratch owner. The outgoing C result is separate from frame homes. |
| C | Unused | The encoder emits zero; the family determines delegation. Add no kind validation or runtime branch for an unused field. A fixed value is justified only if the actual shared format requires it. |
| D | Explicit retained SEND receiver | A real borrowed operand/use, never TAKE/INCREF/CLOSE. It names a stable owned scratch home, not a mutable named-local borrow. |

D occupies the existing free 12-bit field. D must differ from A and from a
consumed scratch B. A may equal B after that outgoing owner is consumed, or
name a separately proved vacant scratch home; a named A reloads its current
binding on resume. Two homes holding the same Python object are not a slot
alias. D remains a live selected home through suspension, including no-count
receivers; a mortal bitmap is neither its identity proof nor its read authority.

The frontend's logical SEND stack still contains receiver, cursor/control and outgoing
value. Lower `YIELD_VALUE(oparg=1)` to the explicit family before register
conversion. Its logical IO retains receiver and cursor/control and replaces
the outgoing value with the future sent value. That exposes the receiver as
an actual register operand while preserving the frontend stack effect. The
cursor is a retained finite-protocol value, not an extra object owner or an
implicit read of an unused physical hole. Do not reconstruct D from allocated
B/A positions. The canonical family supplies delegation as a compile-time fact, not a
second generic runtime-kind branch. Retained cursor/control lifetime is
compile-time transport unless an actual handler operand reads it; it imposes
no contiguous allocated D/cursor/B window. A, B and D are independently
allocated/remapped fields. Existing explicit SEND operands retain their own
roles; cursor IO does not recreate the old physical window contract.

## Conversion, allocation and public admission

`regcfg.c` records the logical receiver ID from the SEND transport before
allocation. The new operation's retained read/through operands participate in
renaming, use counts, liveness, folding, compaction, remapping and emission.
A concrete lowering can expose three inputs/three outputs with two through
values rather than one input/one output: receiver and cursor keep their IDs;
the future sent value gets the new result ID. This costs four extra operand
pool integers (16 bytes) per delegated operation during compilation, with no
new `_PyRegOp` field. IR must verify that exact shape against current metadata.

B and D are stable scratch metadata uses and cannot fold into mutable named
bindings. D cannot be coalesced with consumed B, disappear as unused, or be
substituted by a nearby home. A must satisfy the existing dynamic-local/vacant-scratch recipient
rule. Register/install APIs, source maps, instruction remapping, assembler
bounds, observer outcomes and the instrumented twin all carry D as a slot.
No opcode-name-only capability grants this transport. D remains a metadata
use even if the C handler needs no receiver load; the shared emitter event must
keep that use visible to allocation and proof. Native inspection checks that
this does not introduce a dead receiver load or spill.

Common admission proves the finite role, not merely bounds/non-NULL state:
actual SEND yielded on the incoming edge; D retains its receiver; B carries
that SEND's outgoing result; A is a valid recipient after B detachment.
Moves/renames transport those identities. Completion/error edges do not create
a delegated-yield fact. The old `dst == base`/`return_slot - 2` geometry is
removed; a valid moved D passes, and an unrelated object at the old position
fails. The old generic YIELD kind-1 encoding is not a compatibility route.
Normal Python compilation lowers to the new form; public executable images
must provide and prove the explicit form. The canonical-number/generated
metadata and bytecode magic changes belong to the same format packet; cached
old encodings must not be interpreted under the new operand contract.

A bounded proof representation is one current SEND-yield tuple (origin site,
receiver home, outgoing home, valid state) per join state, plus one work tuple.
A new successful SEND edge establishes it; verified transfers update its
homes; unknown operations, lost owners or disagreeing joins invalidate it.
This adds 16 bytes per join state, not a blocks-by-registers origin array.
Only complete all-path tuples certify a terminal yield. IR must identify any
legitimate frontend path requiring more than this single pending transport
before accepting this bound. Unknown/uncertified public roles must be reported
explicitly; they cannot silently reacquire the old physical-slot rule.

## Actual event and saved continuation

The shared emitter emits a terminal delegated-yield event after outgoing
ownership is detached. It contains the actual A/B/D field roles and saved
continuation, not a manual opcode-family owner table. Every admitted canonical
delegated site must have the finite D/A transport proof. Consequently the first
increment needs no duplicate protocol row, per-site role allocation or cached
D: the immutable admitted word itself carries A and D.

The common cold selector checks the reached literal delegated phase,
independent code pin, ready immutable image, saved continuation bounds/alignment
and the canonical preceding YIELD_DELEGATED word at `yield_pc + 1`. Its A must
equal live `frame->return_slot`; D has the positive finite retained-receiver
certificate from admission. No return-slot subtraction occurs. A role lookup
must reject before any home read if these conditions fail.

This finite readable-D certificate is separate from whole suspended home-view
MUST_READABLE authority. A phase with no complete home recipe remains explicitly
uncovered for full-map traversal; it does not mean empty scratch. The first
packet does not add a whole-home row merely to enable receiver lookup. A later
complete suspended view uses the same actual terminal event and immutable
continuation relation, with separately priced phase/range rows.

The producer publishes the literal delegated phase after terminal
header/exception state is committed and before release-publication of the
suspended generator state. Monitoring runs under its actual pre-yield recipe;
a redirect/error there does not publish suspension. No hot phase lookup or
addressable handler record is introduced. Instrumentation's SEND receiver
remains its explicit SEND operand, with actual observer outcomes audited for
the new family. The common getter must not grow another owner-map decoder.

## Claimed execution, resume and observers

A successful close/throw CAS from delegated suspension to EXECUTING does not
mutate the suspended homes or continuation. Keep the proven delegated phase
through the delegated close/throw callback. The claiming caller passes its
actual saved prior state through a private claimed-route selector, which
shares the matched-word lookup with the suspended getter. State alone cannot
create a claimed capability. Resolve and acquire the receiver reference
callback-free, then retain that native owned reference through callbacks.
Do not reuse a captured index after callback-time resume/reset.

If delegate throw restores suspension without resuming the outer frame, the
same unchanged recipe survives. Invalidate immediately before the first real
outer-frame home/continuation mutation, resume-input installation, clear or
accepted redirect. GC can select unchanged claimed-but-not-resumed homes
under its usual quiescence rule; Python's getter and remote lookup remain
suspended-only. In particular, do not make a generic getter treat every
EXECUTING generator as delegated.

The FT getter currently loads state without acquiring the advertised
YIELD_FROM_LOCKED read claim. The port must acquire that claim (or an agreed
equivalent) through receiver reference acquisition; resume already waits on
that state. Phase validation alone does not fix the race.

Capture preserves the exact immutable suspended view under the code pin.
Named bindings remain dynamic. Debugger redirects must preserve both receiver
and resume obligations or reject before mutation, then invalidate before
commit; phase zero cannot reacquire delegation. Close/throw temporary frame
links and exception attachment keep their existing lifetime rules.

## Remote reader and costs

The remote asyncio reader replaces slot subtraction with the same finite
canonical-word contract. Export phase_state and only the code/image fields
needed to bound and validate the saved preceding word; it already reads
instr_ptr/ir_words. Match owner, suspended state, phase, code/image, A and D;
retain overflow, remote-read-failure and address guards. A matched D, not a
Python-object-looking stale word, supplies the awaited object. This packet
must connect the actual read-lock/stopped-target contract, not assume it exists.

There is no frame-header, tailcall-argument or `_PyCodeExec` growth. Instruction
words remain 8 bytes, with one ordinary/instrumented handler pair. One literal
phase store is added at reached delegated suspension; D adds no refcount or
normal-handler value load. Finite role metadata adds no per-site allocation: the existing word
contains A/D and all delegated sites positively admit. Future full suspended
views, when implemented, cost an existing 16-byte phase row plus actual 6-byte
selected ranges and must be counted by allocation/free/sizeof independently
of retirement rows. They are not part of this finite-role packet. Cold proof workspace and
operand-pool costs are stated above. Root measures actual native registers,
spills and dispatch after implementation; no timing claim follows from size.

## Source ownership and validation packet

| Owner | Bounded files / functions |
| --- | --- |
| IR | `bytecodes.c` canonical family/twin and actual event; generator metadata/handler/role emission; regcfg conversion, passes/allocator/remapping; assemble encoding; common code admission, phase builder and matched selector in `ir.c`; source/IO perturbation and public-image tests |
| Frames | Shared suspension helper/publication, `_PyFrame_SetReturnValue`/copy/capture/clear invalidation and observer-lifetime audit; actual gen/coro/asyncgen suspension and callback fixtures. Bytecode helper hunks are requested from IR rather than simultaneous replacement. |
| GC | `genobject.c` receiver helper/getter/close/throw ports, actual FT read claim, remote asyncio/debug-offset validation; GC selected-home consumer peer. See the consumer census. |
| Root | Shared interface/source freeze, generation, builds, native inspection, all execution and integration |

First source packet should include the real lowering/family/admission plus a
matched selector and its actual close/throw/getter consumers, rather than an
unused API. Test normal delegation, coroutine/asyncgen await, moved D, changed
A, wrong old-position object, role-disagreeing joins, ordinary/discarded yields,
close/throw callback inspection/collection, restored suspension, local mutation,
escape/copy, accepted/rejected redirects and remote await chains. All runtime
source remains frozen pending root and IR interface approval.

## Peer disposition

GC architecture peer is GO on the explicit operand/lifetime design. IR has
agreed the A/B/D direction, continuation relation and actual frontend
single-SEND tuple feasibility. The cursor is compiler-only unchanged
passthrough; B is logical input 2, D logical input 0, A the result. Exact
signatures and emitter hook placement are being frozen after the CALL repair. The single pending SEND tuple remains
an IR feasibility gate, not implemented source authority. Root source freeze
and ownership assignment are still required before shared-source edits.

## Bounded frames packet (historical scope at integrated e03b4628941)

The pins worktree merged e03b4628941 as 2310df048d4 and is clean with
zero source diff from that integrated base. Runtime source waits for the
exact root/IR interface freeze. Do not mix the bitmap rival's writer changes into this packet.

| File / owned hunk | Concrete change | Precondition / limit |
| --- | --- | --- |
| `Include/internal/pycore_interpframe.h`, beside return/resume publication | Add inline `_PyFrame_PublishDelegatedSuspension(frame)`, storing the literal PUBLISHED plus delegated phase ID with zero shadow status | Only the admitted terminal delegated family calls it; no lookup, handler-local address, refcount action or header growth |
| IR-requested `Python/bytecodes.c` terminal hunk | Invoke that helper after the outgoing owner and final exception/header/previous/profile state are committed, immediately before release-publishing suspended state | IR owns the family/body. A is already assigned from the actual encoded recipient; do not publish early at that assignment |
| IR-requested enum/analyzer/emitter hunk | Supply the delegated phase ID, actual terminal event and leaf classification for the helper | The finite D/A certificate is established by actual admission/event proof; no opcode-name-only grant |
| `Python/frame.c:_PyFrame_CompleteRedirect` | Assert the caller has invalidated publication before changing continuation; retain the current cold commit/clear sequence | `_PyFrame_SetReturnValue` already resets first. This adds no release-build store or new redirect route |
| `Include/internal/pycore_interpframe.h` existing resume/copy resets | Audit and retain first-operation invalidation in SetReturnValue/PublishReturnSlot, and both source/destination resets in CopyHeader | Do not copy a delegated marker into CREATED/CAPTURED lifecycle or reset at an execution-state claim |
| `Python/frame.c` clear/capture entry and `Objects/frameobject.c` observer commit | Audit/assert existing pre-mutation reset; retain suspended/PY_YIELD redirect rejection until an actual recipe exists | Generic SUSPENDS/observer effects must include the new family. Do not add an offset-based repair or whole-map authority |
| `Modules/_testinternalcapi.c` | Cold `frame_has_delegated_phase(frame)` observation for fixtures; reads only the phase word after type validation | Distinguishes claimed-but-unchanged phase lifetime without granting any home read or forged claimed capability |
| `Lib/test/test_frame.py` | Focused producer/invalidation, local-write/collect/capture, close/throw callback, ordinary-yield and observer fixtures | Actual getter behavior tests the production selector; whole-home authority may remain NONE/CANDIDATE |

No extra normal-path reset is needed: current SetReturnValue and direct
PublishReturnSlot already invalidate before reading/replacing a binding.
ClearTemporaries, TryClear, InitClearRoots, live local clear and observer
commit likewise invalidate before home mutation. Existing suspended/PY_YIELD
f_lineno rejection is an explicit first-packet limit; generic SUSPENDS flags
must keep it true for the new canonical family. No speculative suspension
redirect recipe or complete suspended map is added.

Frames fixtures will check: phase published only after actual yield; ordinary
yield has no delegated phase; named-local mutation preserves a valid suspended
route; resume invalidates before user code; delegate close/throw callbacks see
the old phase while the outer is claimed EXECUTING; restored suspension keeps
it only when homes/continuation did not change; clear/escape never reacquires
it from zero. PY_YIELD monitoring checks pre-suspension state and unchanged
rejection of recipe-less f_lineno changes. GC owns getter locking and receiver
reference acquisition; IR owns moved-D/public-image and complete IO proof
fixtures; remote fixtures remain with GC.

Implementation waits for IR's finite transport feasibility check and exact
shared helper/event freeze. This is the precise proposed frames source scope,
not authorization to edit the shared family or infer whole-map authority.


### Frames source packet

The pins lane implements the literal publisher, exact captured-phase combo,
and selected-copy preservation. The copy queries the actual finite D before
CopyHeader disarms the source, retains the marker only if its supplied view
includes D, and publishes it after all selected values and destination gaps
are committed. Capture installs CAPTURED without dropping that retained bit;
incomplete-frame PC normalization drops the finite marker first. Existing
resume/store, clear, teardown, and observer redirect invalidations remain in
force; CompleteRedirect asserts prior invalidation.

The helper packet observes only the phase (no receiver acquisition) and
extends existing private copy buffers for selected versus clipped D. The
latter also installs a private capture descriptor and checks the actual common
home selector plus finite selector, without publishing/executing a malformed
frame or claiming generator state. Fixtures cover local mutation, close/throw
claimed callbacks, actual resume/clear, PY_YIELD prepublication and redirect
rejection, and selected/clipped poison copying. Host AST/diff checks pass;
source peers and combined native validation remain pending.

### Current IR source draft

Root froze the shared interface and authorized implementation. The old
preparation notes above record the earlier scope; the publisher stores the
literal `_PyIR_PHASE_YIELD_DELEGATED` bit13, not the general active-phase
PUBLISHED encoding. Both frames and GC consumer packets have source reviews;
combined generation and native validation still depend on the IR packet.

The IR draft lowers frontend YIELD_VALUE1 to IR-only handlers 361/362 before
physical allocation. Its logical3-in/3-out transport retains receiver and
cursor, but a checked metadata-only input1 has no encoded home read. The word
is A=resume recipient, B=outgoing, C=unused, D=retained receiver. B/D stay
scratch inputs; no contiguous receiver/cursor/outgoing layout is required.

SEND.A belongs to its completion route. The actual yielded route publishes
BASE+2, independently of A. A source-checked producer transport and the actual
final flush or entered-child return publication provide that role receipt;
the complete family must agree. The current four parsed source contracts are
a bounded bridge to the shared emitter events, not whole-family ownership
coverage. Changed helper/window/body transport revokes the receipt.

The common admission worklist carries one 16-byte pending pair per block and
one working pair. All predecessors must establish the same D/B roles; distinct
SEND origins may agree on roles without claiming object equality. Missing or
opaque paths lose the certificate. SWAP remaps the role homes; COPY does not
give its duplicate a second designation. Every canonical delegated site,
including unreachable sites, needs proof. The draft cold selector shares the
existing ready-code/image/site validator and reads no frame home. It requires
the reached literal lifecycle marker plus the matched immutable preceding
word and resume recipient. This finite certificate grants no complete
suspended-home map and allocates no per-code protocol rows.
