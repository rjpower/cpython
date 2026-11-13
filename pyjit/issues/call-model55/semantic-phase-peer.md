# First persistent retirement certificate: GC peer

2026-11-12. Independent source/design review of
[semantic-phase-plan.md](semantic-phase-plan.md). GO for its bounded next
increment, with the query validation below made explicit before implementation.
This does not admit static authority or scanner removal.

The extra code-exec pointer and default/sparse certificate have a reached cold
consumer: a linked actual retirement record checks the canonical NULL control
before cursor mutation. Exactness requires both complete predecessor coverage
and all scratch homes known canonical NULL; MAY_OWNER union alone is insufficient.
An unsupported/error/observer entry poisons completeness. Checking inline and
immortal leftovers as well as counted objects is necessary. Nullable dynamic
locals, hidden constant binding homes and the detached result remain separate
from this empty-scratch certificate.

The shared all-return default must still validate the canonical SiteId against
the selected immutable image and require an actual normal-return site. A
NORMAL_RETURN reason alone cannot turn an arbitrary PC into a certified site.
Capture/validate the site before callbacks; after code detach the record stores
only the outcome and the collector reads no code geometry. Sparse lookup has
the same site-domain requirement.

Reason/status bytes can use existing padding without frame growth, subject to
root ABI review. Define zero as the valid no-reason/uncovered state for existing
prefix-only records, validate all enum values on push, and distinguish unchecked
from checked-exact in the test query. The query observes the actual linked
outcome; it must not rerun the comparison or reconstruct a plan. MISMATCH is a
development failure, never silent coverage. A cold test query must therefore
prove a reached producer performed the comparison rather than proving only
that the image could have been certified.

Code-exec lifetime, static versus owned sparse storage, checked allocation and
size accounting remain mandatory. New executable-header fields require the
existing words-to-header address arithmetic and fixed-image constructors to be
updated together. Existing static sentinel/trampoline images stay explicitly
uncovered until their own contract is proved.

The bounded component prices one executing-header pointer, sparse plan bytes,
reason argument traffic and reached cold query/status work. It adds no frame
field, per-call plan allocation, generic hot decoder or native root head. The
next useful expansion should consume emitter IO/consume/publication facts and
real unpack/binder phases rather than lengthening exact C-body template lists.
Root owns generated output, builds, both-role tests and native evidence.

GC source implementation is frozen over `97850931449`: transition reason/status
occupy offsets22/23 with a 64-bit 40-byte size assertion; push checks enum
domains and NONE implies UNCOVERED. The internal query returns stored
`[(reason,status)]` in linked head-to-outer order, including prefix-only `(0,0)`.
It performs no frame/code read or comparison. IR source postreview GO; diff
check passes. The exact frozen enum block was copied from IR for header
coherence; root integrates the actual producer/query/plan and owns runtime
evidence. No native execution was performed in this lane.

The separate bounded `_testinternalcapi.check_ir_retirement` control helper is
implemented and IR source postreview GO. It validates a real admitted code,
site/reason/home bounds, prepares full initialized NULL homes in an unpublished
fake frame and optionally installs inline17 or an unaligned/one-past PC. It
calls the cold query directly, frees storage and returns status. The argument
pins the borrowed code; no fake object owners are published or closed. This
supports MISMATCH/site-domain tests without executing malformed images. It
does not replace tests observing actual linked producer status.

Persistent-plan source postreview GO after two corrections. The immutable-view
worklist keeps completeness as a separately queued monotone meet, requires
canonical NULL plus no may-owner after EXIT, and emits sorted sparse sites or
the static all-return default. Query validates reason, image identity, integer
site bounds/alignment and canonical EXIT before lookup/control reads. Plan
allocation/failure/free/size accounting is balanced; the executing-header
offset assertion preserves the hot local-count displacement.

Resolved findings: zero-sized work storage could divide by zero; storage now
has at least one word, while builder geometry must match the normalized
physical domain. Public zero-stack requests are already normalized to one
scratch cell by `init_code`; noncanonical static geometry remains uncovered.
The initial RESUME token blacklist allowed cached-pointer stores such as
`*slots`; it now requires complete audited helper bodies and IO shape. Actual
AST perturbation tests reject opaque, cached-pointer, pointer-arithmetic and
base-index writes. This remains a bounded normal-edge contract, not callback
coverage, and the next emitter-based phase work must not grow another general
manual semantic decoder. Root owns generation and runtime evidence.

Integrated literal-reason hooks postreview GO over `e5a51d269a8`, seven-file
source delta. WithResult has only the RETURN_VALUE caller and fixes NORMAL_RETURN;
void callers supply ERROR_UNWIND/STAGED_FAILURE, direct generator clear supplies
HEAP_CLEAR, and binding failure transfers stage authority callback-free. Both
outlined slow producers set reason before push and store the cold comparison
immediately after push, before remaining-cursor mutation/takeover. Code stays
pinned through that query and the final geometry read; detached result and
generator exception lifetimes are preserved. Existing genobject inclusion of
interpframe→IR exposes its enum. No source must-fix; regeneration, build and
actual producer-status tests remain coordinator gates. Review was source-only.

Follow-up actual RESUME enabling correction peer GO at integrated `9fdc`:
`Stmt.tokens()` omitted MacroIf else/endif, so the prior literal token comparison
left actual RESUME UNKNOWN. The source contract now compares recursive parsed
Stmt fields and Token(kind,text), retaining preprocessor arm boundaries while
excluding source positions/comments. A test moves a statement between macro
arms without changing flattened tokens and requires UNKNOWN; pointer/opaque
mutations remain rejected. This closes the source mismatch, with actual
regeneration/PRESERVE metadata and candidate execution still root-owned.
# Shared-emitter collector schema peer

The bounded shared-emitter increment has GC design GO. Its first reached
comparison uses the existing frame walk with nonempty arithmetic survivors;
NULL authority remains in force. Must-readable facts intersect, possible owner
obligations union, and exact nullable ranges require every incoming arm to hold
a value or canonical NULL. Absence checks reject inline/index leftovers too.
Native root discovery stays separate. Cyclic subtraction and external marking
retain their respective edge/tag policies even when sharing a descriptor.

The uint16 publication/status field preserves PRE=0 and the measured header
extent; the code plan reuses its existing pointer. Source review must verify
zero publication for unsupported reached boundaries, matched parent WAIT and
heap-suspend states, and explicit initialization/reset on copy, redirect,
takeover and retirement before callbacks. A stale published bit alone never
grants an arbitrary asynchronous reader permission. The cold query validates
code/image/site before range reads and stores reached status; observation must
not recompute it. No map-only scan, arena uninitialization or native policy
change is admitted by this increment. Source and merged native gates remain.

## Frames lifecycle peer of the shared-emitter increment

2026-11-12. Design GO for the bounded event-derived admission and reached
generic-binary collector shadow in `semantic-phase-plan.md`, starting at its
shared-emitter section. This is not approval to replace NULL authority.

The first positive fixture must carry a nonempty scratch survivor through the
actual provider or operand-finalizer collection. Argument/binding loads,
constants and COPY/TAKE transport therefore need positive events in the same
graph; leaving those predecessors opaque would make the first consumer
permanently uncovered. Unknown helpers and unconverted families remain
explicitly uncovered. The old exact-body checks may protect audited leaves,
but must not become a second semantic decoder beside emitter events.

The uint16 state at GIL offset 110 fits the measured 112-byte header. Keeping
PRE=0 separate from the PUBLISHED bit preserves the PhaseId domain. Initialize
all producers, including partial/public heap frames and fixed trampolines.
Reset before generator resume installs a sent owner, before unsupported
provider/binder publication, and before retirement or takeover changes the
authority partition. A copied old PRE or SUSPENDED state is invalid after a PC
advance or home mutation unless the new lifecycle explicitly matches it.

The parent WAIT recipe must exclude the child-owned overlapping argument
window exactly once while preserving surviving parent homes. Provider entry
starts with an unpublished child; disarming its outer staged record does not
itself certify an active phase. Redirect acceptance must reset or select the
new site's state before callbacks; rejected redirects leave the current state
alone. Dynamic local boundness and callback-local mutation remain independent
of the static scratch shape.

Only a reached collector walk supplies the stopped-state interval in which
the saved site and published shape agree. An arbitrary remote sample cannot
use a stale PUBLISHED bit as permission to read homes. Cold lookup validates
code/image/site under the independent code pin. Stored status can outlive code
closure, but its query must never decode the old frame or descriptor afterward.
No ordinary inline lookup/reset or handler-local owner array is needed by this
increment. Root owns generation, both-role execution and the emitted-store,
stack/spill and persistent-plan cost review.

## GC source peer of first shared-emitter shapes

2026-11-12. Bounded source GO for the IR lane's first generic-binary phase
increment under NULL authority, with the separately reviewed GC collector/probe
and accepted-redirect reset ports integrated. This is a continuing cross-lane
source review, not a fresh independent review of the GC-authored components.
No generation or native execution ran in this lane.

The worklist intersects NULL/initialized/nonNULL facts, unions possible owner
obligations, and builds phase rows after convergence. The error branch restores
its own snapshot; its restored owners cannot alter the normal successor state.
Unconverted specialization families deliberately stop subsequent completeness.
Code-owned rows remain sorted, equal ranges are hash-interned, and allocation
failure/free/sizeof paths cover the single plan and temporary builder storage.

The cold selector validates the independent code/image/site/alignment before
home reads. Generic binary emission takes each evaluated input before provider
publication, publishes a scratch result before operand retirement, restores
only scratch inputs on error, and preserves callback-mutated named bindings.
Republishing after the provider requires the same PC and unreplaced published
phase; observer invalidation cannot silently regain exactness.

The accepted redirect port resets before any committed home mutation or
retired-value callback; rejected attempts retain their current state. The cold
probe uses only inline values in a full initialized unpublished native buffer,
and perturbs image/PC only for the actual query. Stored-status observation does
not start comparisons.

Nullable joined ranges are NULL-control owner candidates, not a guarantee that
every path initializes a readable value. Their header contract now says so.
Future stale-cell copy/GC readers require normalization or explicit validity;
the shadow comparison and this review do not admit M4. Root still must verify
actual regenerated events/stores, complete lifecycle initialization/reset,
reached nonempty-survivor collection, FT stopped-state gating and native costs.
