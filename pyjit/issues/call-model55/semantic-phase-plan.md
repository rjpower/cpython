# Semantic ownership and phase plan

Status: root integrated the bounded certificate, literal retirement reasons and
provider kinds; targeted plan/provider cases pass both roles. This is partial
M2 NULL-shadow coverage. The shared-emitter increment below has frames/GC
design GO and root source authorization; implementation is in progress. Its
separate collector component awaits the IR definition and complete producer/
reset ports before merged linkage and validation.
Root approved the first exact normal-retirement
descriptor and actual cold shadow consumer; frames hook review and independent
GC design review are GO. The SiteId validation requirement below incorporates
GC's required clarification. This
extends the bounded normal-transfer worklist committed as `26b1e8d4900` and
merged with current runtime contracts in `0bed5688eae`. NULL homes remain the
execution authority. No static-map or complete M2/M3 claim follows from this
first empty-scratch retirement certificate.

## One semantic representation

Use the same canonical code word SiteId and family-assigned uint8 PhaseId for
admission, callback publication, teardown, collector roots and observers. SiteId
is an immutable word offset, never an executing handler address or a function's
mutable code. PRE, POST, ERROR, RETIRING and SUSPENDED have common role names;
additional staged roles describe finite family states, not frontend stack
depths. Dynamic output progress is one validated prefix/suffix recipe plus a
cursor, rather than a phase per item.

Each shape separates:

- named binding storage, which may contain unbound canonical NULL and can be
  changed by a local proxy;
- initialized/readable scratch values, including permitted canonical NULL;
- scratch owner obligations, including inline/immortal logical values whose
  runtime tags require no decref;
- detached counted native owners and results under the selected native policy;
- staged arena owners and moved heap edges, with their actual unique authority.

The persistent plan interns initialized/owner shapes and indexes reached
site/phase rows. Ranges use the full admitted physical home domain through 4094;
no 64-home cutoff. A compact empty/default shape is shared. A row has explicit
coverage and exactness; unknown or merely possible ownership never creates a
readable range. Potential nullable owners use runtime canonical tags when
enumerated; an owner range does not assert every home contains a mortal object.
The same release iterator will supply teardown and remaining-root enumeration.

Generate these facts at the existing `IRStack`/`IREmitter` events that actually
load, consume, take, duplicate, retire, flush and publish. Reuse `external_io`,
`through_items`, field roles, Outcomes, result publication and helper marker
effects. A general scalar/window IO recipe supplies normal-edge transport;
explicit helper contracts describe opaque frame/window transactions. Exact body
templates remain useful regression guards for a narrow contract but cannot
certify arbitrary C semantics. A helper's input/output ownership, early exits
and callback phase must be declared and checked against its caller. No parallel
manually maintained opcode decoder is the long-term source of truth.

Admission meets readable/initialized facts across all predecessors and unions
possible release obligations. Exact shape coverage needs a second agreement
check: alternate paths must have compatible unique-owner/readable shapes, or
must canonicalize inactive homes to NULL, split a finite family phase, or reject
the image. A union of possible owner bits alone is never that certificate.
Exceptions and observer entries need their actual incoming transfer, not an
unknown seed once authority switches. Compare virtual regcfg transfers with
physical allocation, aliases and the whole child clobber extent.

## Family and boundary coverage

| Family or boundary | Required normal and staged facts |
| --- | --- |
| Constants, local/cell/free loads, binding store/clear | Explicit fresh/duplicate/move/NULL; local boundness remains a runtime exception where required. Stores publish replacement before displaced-owner close. Hidden constant homes stay readonly. |
| COPY/SWAP and pass-through windows | Parallel source snapshots; COPY adds one logical owner, SWAP transports existing owners; holes are not reads. |
| Arithmetic, comparisons, indexing, attribute readers/stores | PRE borrowed bindings versus consumed scratch; helper/provider owners; early result publication; error before/after retirement; folded local destination and post-callback refresh. Method loads define both callable and self-or-NULL. |
| Container building/mutation, imports, intrinsics and miscellaneous helpers | Fixed/dynamic consumed inputs, new output owner, transferred container edges and failure cleanup; an actual helper contract when IO alone cannot prove the transfer. |
| Conditional/fused branches | Separate taken/fallthrough transport and nullable rules; deopt restores original PRE before any consume. Specializations and instrumented forms must preserve the certified family role. |
| CALL, CALL_KW, EX and specialization/binder forms | Explicit supplied-self-or-NULL protocol, borrowed versus counted argument ownership, child reservation/clobber, precommit fallback, committed parent/child partition, staged binding/provider cleanup, discarded/folded-local result and errors. Parent survivors remain below the child clobber extent. |
| FOR/SEND and generator entry/completion | Body/yield, exhaustion, exception and redirection outcomes; receiver/cursor/tagged-index roles; returned/yielded result detached until recipient publication. |
| Generic unpack | Validated source/window/capacity plus initialized suffix; keep its outer descriptor through iterator/surplus/sequence cleanup; final output POST before descriptor disarm. Exact tuple/list paths use checked fixed output recipes. |
| Exception construction/matching/unwind, source errors | Explicit partial consume/output failure, exception-slot/lasti transfer, handler incoming shape and saved native exception owner. A PC alone does not identify the error shape. |
| RESUME, polls, monitoring, observer terminal/redirection | Shared emitter event records the state before the callback and committed state afterwards; reload mutable bindings/destinations. Redirect recipes preserve actual owner/protocol prerequisites. |
| RETURN, unwind, direct heap clear, staged initialization failure | Producer-supplied reason selects the actual retirement entry; result is separate; code pin fixed-last; reservation survives callbacks; escaped takeover changes edge authority before later closes. |
| Yield/suspend/resume/throw/close and heap construction | Every exposed initialized prefix and suspended state, including allocation failure and unstarted generator; code independent of function mutation. |
| Entry trampoline and InitCleanup | Fixed explicit bounded images need their own contracts; ordinary code admission does not certify a NULL-code sentinel or different static executable words. |

Implicit ordinary CALL NULL needs a producer/site census before changing the
encoding. Compare a certified ordinary-call form that supplies NULL without
reading an inactive home against an explicit canonical-NULL producer. Prefer
the family form when it expresses actual producers cleanly and saves a store.
Method calls retain a real self value. Elided PUSH_NULL/empty physical scratch
is development evidence only and cannot become the stale-scratch definition.
Update compiler, allocation verifier, assembly, all public/marshal/frozen
admission, handler metadata and disassembly together; no hidden constant pool
or borrowed mortal scratch workaround.

## Next bounded increment: exact normal-retirement certificates

The first persistent consumer deliberately certifies only normal RETURN paths
whose complete predecessor transfer coverage yields the exact empty scratch
retirement shape. It is infrastructure evidence; a locals-only/empty scratch
plan does not by itself remove useful work or establish general authority.

1. Extend the existing worklist with monotone complete-coverage tracking;
   entry scratch is physically NULL but unavailable as a language value.
   Unsupported rows and error/observer entry seeds poison completeness.
   Certified transfers do not recover completeness after an unknown barrier.
   Add a checked RESUME normal-preserve recipe so ordinary compiled scalar
   functions can reach the first consumer. This describes its normal edge,
   not its callbacks/errors. Stable exit inspection requires no remaining
   scratch obligation and a certified physical NULL for every scratch home.
2. Build a code-owned sparse certificate indexed by the immutable return site.
   Initially there is one used shape: nullable bindings plus empty scratch and
   separately detached result. Unsupported code uses NULL; code whose every
   normal-return site is certified can use a shared static default with no
   side allocation; mixed sites use a sorted uint32 site vector. A shared
   default still requires an aligned, in-bounds canonical SiteId naming an
   actual source-certified normal RETURN; a reason constant alone cannot
   certify an arbitrary PC. Do not add
   unused generic range/phase arrays before the next consumer needs them.
   Place one plan pointer in `_PyCodeExec` (eight bytes on the measured ABI),
   not the interpreter frame. Fixed static images remain explicitly uncovered.
   Root must review executable-header addressing and regenerate/rebuild.
3. Construct the certificate from the same immutable normalized image/proof,
   with checked allocation arithmetic and balanced failure cleanup. The
   executing image owns any sparse allocation until code destruction and
   reports its bytes in size accounting. Prefer attaching results while
   preparing the existing executing image; no per-call plan allocation,
   image refcount or debug-info dependency. If cold worklist reuse needs an
   input view, refactor its input access rather than copying a second validator.
   The implemented builder uses the same canonical physical scratch extent as
   admission (`max(co_stacksize, 1)`) and requires it to agree with frame geometry.
   Noncanonical static geometry is uncovered. Bitvector storage independently
   has at least one word, so a zero logical extent cannot divide by zero.
4. Frames supplies NORMAL_RETURN, ERROR_UNWIND, HEAP_CLEAR or STAGED_FAILURE
   to private retirement paths. No-result is not itself a proof of error;
   staged binding failure precedes RESUME. Thread/heap slow continuations link
   their record, then call a cold shadow query before the cursor changes.
   Capture and validate the selected site once before cleanup callbacks; no
   later observer recomputes the comparison. The query checks only certified NORMAL_RETURN here; other reasons and
   unproved sites return UNCOVERED. EXACT compares all scratch homes with
   retained canonical NULL control, catching inline/immortal leftovers as well
   as counted objects. It never rereads geometry after code detach.
5. Declare reason and shadow-status bytes in the current record's padding
   (offsets 22/23 on the measured ABI) if root/GC approve; validate enum domains
   on push, with zero meaning uncovered/no retirement reason for existing
   prefix-only producers. Hidden constant homes remain in the named binding
   region and retain their owner authority. A precise test query observes the linked reached status, so a test
   cannot pass solely because a separate inspection helper rebuilt a plan.
   EXACT, UNCOVERED and MISMATCH have distinct query results; MISMATCH fails a
   development assertion/diagnostic, never silently becomes EXACT. Root freezes
   diagnostic behavior before implementation. No scanner or NULL store changes.

Complexity remains O(words + blocks * ceil(scratch/64)) admission memory plus
the queue/transfer work. Persistent unsupported/default code adds only its
executing-header pointer; mixed normal-return coverage adds a small header
and four bytes per certified site, with overflow-checked O(return sites)
storage. Lookup is constant for shared defaults or logarithmic in sparse sites;
scratch shadow comparison is O(scratch) only in reached slow retirement.
Price extra reason argument traffic, cold query/link/status stores, executable
header growth, actual native stack/spills and plan bytes. No elapsed benefit
is assumed, and no large hot automatic record or per-inline-consume decode
is introduced.

Focused tests use real compiled scalar functions plus synthetic constructor,
replace and patched marshal images. Require actual EXACT slow retirement,
UNKNOWN after unsupported rows, branch/loop predecessor coverage, full scratch
extent through 4094, nullable/unbound bindings, inline/immortal/NULL values,
independent result presence and chain exhaustion. Corrupt a development control
home only through a bounded test-only helper to require MISMATCH if such helper
is justified; do not execute malformed public code as a substitute. Root owns
generation, both-role runs and native evidence.

GC source postreview closed two required corrections: canonical zero-stack
geometry plus nonzero bitvector storage, and positive full-body RESUME contracts
that reject direct cached-pointer writes. Perturbations include `*slots`, pointer
arithmetic and indexed frame writes. Source AST/diff checks pass. This one RESUME
normal contract remains deliberately narrow; it does not replace the shared
emitter callback/ownership event work below.

## Immediate following increment and M4 gates

After this consumer, record per-instruction callback phases from shared emitter
IO/consume/retire/publication events. Start the actual generic unpack suffix
and staged binding consumers in parallel with the finite scalar callback
ledger. Their outer-lifetime proofs matter more than adding more exact-body
normal templates. Add persistent initialized/owner range shapes only as those
reached consumers need them, reusing the same code pin/site/phase lookup.

Before M4, every admitted executable family, edge and static image must have
complete source-checked normal and callback/error/suspend contracts. UNKNOWN
cannot remain a selectable runtime fallback. Close the callback/arena/heap-edge
census and migrate collector, escaped-frame, debugger, remote reader, exception,
constructor and suspension consumers. Shadow comparisons check logical values,
initialized readable homes and unique counted edges separately; they do not
equate nonNULL with mortal ownership. Only then can static authority replace
NULL interpretation and arena scans together. Native C/PyMem discovery remains
the separate selected policy. Root owns full composition/performance admission.

## Shared-emitter increment: one event graph, two immediate consumers

The next source increment replaces the normal-only table interface with an
ownership event graph emitted while `IRStack` and `IREmitter` emit the actual
handler. Its immediate consumers are the existing admission worklist and a
reached collector shadow check during generic binary operations. A live scratch
survivor must be present in the integration test; empty retirement alone does
not establish useful phase coverage. NULL stores/scans remain authoritative.

The current hooks already supply most transport facts:

| Existing source hook | Event it must emit |
| --- | --- |
| `IRStack.pop/load`, `external_io`, `through_items` | Read a value or nullable protocol role; bind a view without granting a count. |
| `consume`, `ensure_consumed`, `TAKE`, `OWNER` | Move a scratch owner out; TAKE of a binding acquires a separate evaluated owner, OWNER of a binding yields NULL. Kill all aliases of the consumed owner. |
| `store`, `vacate`, `flush`, `emit_final_outputs` | Publish/move an output, replace a dynamic binding, or leave canonical NULL control; record actual folded/discarded recipient. |
| DUP/steal/release replacers and `DECREF_INPUTS` | Create one independent logical owner, move one owner, or end its obligation. An ordinary C assignment aliases; it never creates a count. |
| `IREmitter.emit_SimpleStmt` and escaping-call analysis | Snapshot after emitted flush/kill and before the actual call; record normal/error continuations separately. |
| Explicit call/reader/retirement/completion markers | Generate the helper invocation and its declared transaction events together; ordinary spill/reload must not replay stale inputs around them. |
| Stack copy/alignment and emitted branch joins | Fork/join event states; guards/deopt may return to PRE only before consuming its owners. |

This is positive effect accounting, not inference from an opcode name, return
type, `escapes` flag or arbitrary C text. Covered statements may use only known
value constructors/transports and checked helper capabilities. An opaque call,
untracked `.bits` reconstruction, cached-pointer store or raw frame/window
access loses coverage. A helper that can touch homes must receive a declared
window/cursor capability and emit its commits through that API. Typed borrowed
arguments alone do not prove the helper has no other frame effects. Existing
marker effects, canonical-output/result-publication validation, ownership
checks and analyzer call classification become inputs to this graph; exact-body
guards remain regression checks for audited leaves, not general semantics.

First convert generic `_BINARY_OP` to a structured value-provider marker using
its existing TAKE/restore/complete protocol. The emitter owns the direct
provider call and completion/error emission, so a separate handwritten summary
cannot drift from the generated transaction. Recognized scalar/local/copy leaf
contracts lower into the same event interface. Unconverted families emit an
explicit uncovered recipe; the old manual transfer enum is not a second runtime
decoder. Port the remaining families through the same interface before M4.

### Recipe and publication ABI

The generator builds a finite event CFG per handler. The binary form has
eight-byte events (`uint8 kind, flags; uint16 dst, src, extent`) and generated
operand selectors. Selectors name a word field plus signed offset, a named
binding, a local owner identity, a typed header edge, or a declared provider
window. They do not embed a physical stack depth or a borrowed scratch bank.
`extent` indexes an existing checked `_PyIR_Extent` expression or a declared
bounded dynamic cursor, never an unchecked C string. Full physical home indices
remain 0..4094; event/ref counts and offsets are checked before narrowing.
Program/block offsets use uint32. Dynamic loops use one cursor recipe rather
than unrolling a phase/event for each element.

The finite event vocabulary is read/view, duplicate, move/take, bind/publish,
release/transfer-out, canonical-NULL control, helper-capability call, phase
snapshot, and normal/error/branch/suspend/retire exit. Events refer to logical
owner identities: two aliases of one evaluated input cannot be retired twice;
two DUPs remain two owners even when their object identity is equal. Inline and
immortal values remain logical values while runtime tags decide counted edges.
An uncovered event never grants initialized/readable or owner facts.

Use the primary proposal's family PhaseIds: PRE=0, POST=1, ERROR=2,
RETIRING=3, SUSPENDED=4, staged/outcome roles from 5 through 255. A specialization
and instrumentation wrapper must use a checked phase in that same family
namespace; matching external IO does not itself prove equivalent phases.
Normalize/split or reject a family that cannot fit. Unknown coverage is separate
from the PhaseId, so PRE=0 is not confused with unpublished state.

Tentative frame ABI is a uint16 `phase_state` in the GIL header's existing
padding at offset 110: bits 0..7 PhaseId, bit 8 PUBLISHED, bits 9..10 stored shadow
status (UNCOVERED/EXACT/MISMATCH), other bits zero. This preserves the measured
112-byte header and 32-byte function offset. All frame producers initialize zero;
this includes interpreter/base sentinels, trampolines, generic/partial binding,
heap generators and public frame construction. Copy/escape/RETURN_GENERATOR
must preserve an explicitly matched state or publish their new lifecycle state;
advancing the PC and blindly copying an old PRE is invalid. Root freezes this
field with frames and GC before implementation.

Publish one literal state after its required owner stores and existing
`instr_ptr` save, immediately before a reached callback, allocation that can
collect, poll, child transfer or
heap suspension. Unsupported boundaries publish zero. No phase lookup, decode
or reset runs per ordinary inline instruction. A stale phase after a callback
returns is not an active scan permission: the next reentry/quiescence point
must publish again. Child-waiting and heap-suspended frames retain their matched
site/phase until resumed. Async/remote readers need the same established stopped
state protocol; sampling an arbitrary running PC does not grant value access.
The PUBLISHED bit records a publication, not proof that an arbitrary external
reader has stopped the thread inside its validity interval. The collector's
reached walk supplies that quiescence context; future remote value inspection
needs an explicit handshake. Source/location-only remote inspection grants no
home access and keeps its current malformed-location degradation behavior.

Every lifecycle/cursor change must select its own phase. Retirement unlinks and
publishes a retirement/provider record before closing; invalidate the active
phase unless an explicit retirement recipe covers it. For a close cursor, move
the owner into the declared detached authority before a callback, remove it from
the frame shape, and publish the remaining suffix/prefix before closing it.
After that close returns, advance only the declared cursor and republish before
the next possible close callback. A synchronous non-reentrant decrement needs
neither publication nor a per-owner map lookup. The record and code pin outlive
all cursor reads; code detaches last. Escaped takeover partitions heap/local
edges before any further callbacks rather than retaining the old frame map.

Reuse the executing image's existing plan pointer for a unified code plan; do
not add another pointer or allocation per code. Extend the current return plan
with sparse `(uint32 site, uint8 phase)` keys, uint32 shape offsets and interned
sorted half-open uint16 scratch ranges with explicit nullable/scalar roles.
Bindings/constants stay in their dynamic initialized binding domain. Unsupported
and shared-default plans remain allocation-free. Allocate once with checked
size arithmetic, include all bytes in code size accounting and free once.
No position/debug bytes enter this code-owned ownership representation.

Cold construction retains per-basic-block full-home bitvectors and queued
monotone work. Persistent space is O(phase sites + distinct shape ranges),
worst-case O(phase sites * scratch); cold state memory remains
O(words + blocks * ceil(scratch/64)). Intern shapes with a hash and exact
collision comparison; do not quadratic-search every earlier shape. Sparse
lookup is logarithmic in phase sites plus the visited ranges. These are cold
collector/retirement costs. The first patch adds initialization stores and one
literal publication store per reached covered boundary, not a handler-local
owner array; GC status updates occur only on reached comparisons. Root must
price emitted stores, code/data bytes, allocations, lookup work and actual
register/spill changes, not just unchanged frame size.

### Worked paths and required joins

- Arithmetic: all guards/deopt leave PRE unchanged. Generic TAKE moves scratch
  inputs to native owners (binding inputs acquire evaluated snapshots), then
  provider phase records their absent frame homes and surviving scratch. Native
  C owners retain the selected conservative C-root policy. Success publishes a
  temporary result before input retirement; a folded-local/discarded result
  stays in the declared detached owner until its actual commit/close. Provider
  failure restores scratch owners where the current protocol does, closes only
  the remaining native snapshots, and enters the actual ERROR recipe. A fast
  specialization and its generic fallback share a normal result interface only
  after both event paths prove it; a partial generic phase does not certify an
  unreviewed specialized callback.
- Method load: success has callable plus a self-or-NULL value, never an
  uninitialized second output. The nullable self home is readable and may own a
  counted edge; NULL is not evidence that the callable owns the receiver.
  Publish both before retiring the receiver. Non-method lookup publishes just
  its actual output. Failure carries no fictitious fresh output obligation.
- CALL: evaluated inputs remain caller owners during guards/preparation;
  precommit fallback restores that state. Commit partitions callable/header,
  zero-copy argument window, surviving parent homes and child clobber extent.
  Publish parent WAIT before entering the child; staged binder/provider state
  covers initialized subsets and cleanup. Return keeps its independent result
  through child retirement, then publishes the callback-current recipient.
  Folded binding/discard commits and error unwind are distinct recipes, not
  inferred from result NULLness. No parent map also counts the child's aliases.
- SEND/FOR: NEXT/YIELD, exhaustion/RETURN and ERROR are separate outcome events.
  Receiver, nullable/tagged-index cursor and sent value have explicit roles;
  the index is initialized control, not an object owner or absent home. Generator
  entry publishes parent WAIT; YIELD/RETURN detach the transferred result before
  publishing recipient state. Completed-edge observers see POST's actual
  retired protocol owners, not the original PRE inputs.
- Unpack: the consuming helper owns the sequence independently; its output
  capability has fixed capacity and an initialized suffix `[begin,end)`.
  Publish/write each canonical output before advancing `begin`; retain the
  outer prefix through iterator/surplus/sequence cleanup. Failure cleanup removes
  each output from that prefix before its close can call Python. Full outputs
  become POST before disarming the provider. Folded-local publication moves one
  output and displaces one current binding at a time, using the same cursor.

Provider capabilities validate `0 <= begin <= end <= capacity` and the complete
frame/arena span, including checked one-past addresses that are never homes.
Their initialized ranges are assigned to one authority: a static frame iterator
must not also count a provider-owned suffix, child argument alias or detached
heap edge. Root marking may encounter an object through several independent
roots, but cyclic subtraction, transfer and teardown cannot duplicate the same
owning edge. Keep native C/PyMem conservative discovery separate; the event graph
does not insert borrowed mortal values into scratch to make them discoverable.

Must-readable language facts intersect at joins; possible remaining obligations
union. To form an exact readable owner range, each candidate home must hold an
owner/value or canonical NULL on every predecessor, with no competing authority.
A consumed arm's physical NULL can normalize a nullable joined owner; a merely
uninitialized/stale arm cannot. Insert edge normalization, choose a finite phase,
or reject before M4. Do not turn the union into a read permission. Branch order,
loops and duplicate logical owners require explicit tests. Mutable named locals
and cells retain their independent boundness and displacement protocol.

Ordinary calls currently elide PUSH_NULL in `regcfg_passes.c` and many handlers
and helpers read `window[1]`. Before stale scratch, introduce a certified
no-self call form chosen from virtual NULL provenance, not observed bits or
source locations. A distinct canonical handler/variant supplies NULL without
reading that inactive home; keep the reserved window geometry for zero-copy
arguments. Generate its specialized forms from the same contract. A real method
form reads initialized self. Bound-method expansion may explicitly publish a
new self owner and switch the transaction's role; later helpers use the role
rather than rediscovering it from inactive bytes. Dynamic public CALL forms
must prove a readable nullable self home. Compiler, allocator, assembler,
constructor/replace/marshal/frozen admission, disassembly and every helper read
of self-or-NULL change together. No per-call NULL store is selected by default.

Fixed InitCleanup has slot0=self and slot1=init result, then EXIT_INIT_CHECK and
RETURN_VALUE; its successful, error and staged-failure phases need an explicit
fixed code-owned recipe. The NULL-code interpreter exit sentinel uses its
separate bounded entry contract, never ordinary code geometry or a fake code
pin. Neither is silently certified by the ordinary plan builder.

### First reached shadow and completion gates

The bounded first consumer is `_PyGC_VisitFrameStack`: at its existing frame
walk, a cold read-only query validates code/image/site/published phase and
compares that certified scratch shape against current NULL control, then stores
the reached status. Keep subtraction and external marking's existing distinct
tag/edge policies; sharing a descriptor must not conflate their visitors. Absent
homes must be canonical NULL, including rejection of leftover inline or index
bits. Owned nullable homes permit NULL; nonnullable language homes must contain
a canonical value; scalar/index roles are checked explicitly. The query neither
infers mortal ownership from non-NULL nor certifies native C discovery. It does
not authorize a map-only scan. Unknown/error/poll/observer phases remain visibly
uncovered until their own events are implemented.

Tests force actual collection inside generic arithmetic, success operand
finalizers and provider-error cleanup, with a surviving scratch object and
folded bindings. Observe stored reached state only; a test query cannot perform
the comparison. Include NULL/inline/immortal survivors, branch/loop nullable
joins, folded-local mutation, recursion, changed `function.__code__`, and a
test-only bounded shadow mismatch probe. Source perturbations must reject
duplicate moves, opaque writes, unreported escapes, missing output publication,
incorrect close progress and specialized deopt after consumption. All public
image routes use the same event-derived admission consumer.

Observer writes to bindings invalidate saved views and retain current dynamic
binding authority. Accepted `f_lineno` redirects must apply a certified transfer
and publish its target shape before any further callback; an unsupported
observer/redirect publishes uncovered now and blocks M4. A normal edge never
certifies a callback, exception entry, partial close or suspension by itself.
Exception recipes take the actual failing phase after its partial consume and
cleanup, preserve only its declared surviving prefix, and install exception and
optional lasti owners at the validated handler homes. Every handler entry joins
those real incoming shapes; it must stop using the current unknown seed before
authority changes. Transparent INSTRUMENTED_LINE/INSTRUCTION wrappers publish
their own pre-delegate snapshot, not an inferred result POST. Instrumented call,
resume, yield and terminal observers must account for their extra callbacks,
errors and redirects while retaining the family's operand geometry. A wrapper
cannot borrow a positive callback row merely because its base normal transfer
was covered. Generation compares both structural branches of preprocessor
conditionals, including their delimiters.
Before M4, generation must positively cover every admitted handler, specialized
member, instrumented wrapper, exception/observer recovery and fixed executable
image. The coverage census includes every escaping statement and helper
capability; UNKNOWN cannot survive the authority switch. All lifecycle readers
must consume this same contract, followed by poison/control checks, native
inspection, complete root-census and performance admission.

### Current IRSM dependency and its removal

The current cold normal worklist decodes `co_linetable`'s IRSM container only
to split and seed `event.handler` observer-failure landing blocks as unknown.
It does not use line/column fields, and ordinary dispatch and retirement shadow
lookup do not decode it. Ordinary executable EH entries come separately from
`co_exceptiontable`. The observer recovery triple `(handler,exc_slot,lasti)` is
semantic metadata currently serialized beside source-only positions; it is not
optional position data. Existing observation resolves that recovery before
unwind, which then consumes pending semantic recovery without a source lookup.

This is still a container dependency, so the earlier blanket claim of no debug
dependency needs this qualification. The shared recipe input must carry observer
recovery edges separately from optional positions. The compiler/assembler emits
those semantic entries from actual observer/completion events; universal image
admission validates them; the code-owned descriptor supplies the worklist and
runtime observer failure. Position stripping cannot alter owner/exception
coverage. Public construction and marshal must validate the same semantic data;
do not silently ignore a malformed source container as a way to grant coverage.

The remote corruption fixture is repaired independently by preparing its valid
executing image before tampering with source bytes. This preserves remote-memory
decoder coverage; it does not change this semantic-source separation obligation.
