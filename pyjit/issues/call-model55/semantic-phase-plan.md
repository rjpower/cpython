# Semantic ownership and phase plan

Status: bounded certificate source implemented and independently peer approved;
root generation, producer integration and runtime qualification remain pending.
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
