# Frames/calls lane progress

2026-11-12: read applicable instructions, official CPython AI policy, canonical
README/STATUS/GOAL/HANDOFF/PROCESS, target design, migration plan and resolution.
Inspected selected54 frame layout, allocation, copy/clear, entry/trampoline,
generator construction and typed-header consumers. No native execution/builds.

M0 comparison and M1 contract are in [pins-proposal](pins-proposal.md), accepted
by root and [GC peer review](pins-peer.md). Shared placement/NULL boundness remains the
reference; separate regions, clean ranges and explicit local validity have
distinct entry/exit/packing/observation costs. Mortal bits do not encode boundness.

Important findings: current code/function PyValues already own counted mortal
references under FT; independent code lifetime must survive function mutation;
cleared generators retain code; partial generator construction has no code yet;
escaped copies require a temporary departing-source code pin. Interpreter entry
and base frames use None today; typed NULL sentinels require remote owner-first
validation. GC lane has the proposed field types and reader obligations.

Implemented the approved M1 scope in `BASE/worktrees/call55-pins`: typed pins,
direct accessors, strong acquisition/clear/traversal, NULL entry/base and partial
generator sentinels, code-backed trampolines, all lane-owned readers and internal
hook helpers. Kept source code duplication on escaped-frame takeover. Removed
the value-returning code-home helper and migrated its test to typed pin counts.
Added independent-pin escaped-frame and completed-generator lifetime regressions.
Own diff review and `git diff --check` pass. No candidate tests/builds run here;
root owns generation, focused execution, native evidence, integration and commits.

Reviewed the GC proposal in [gc-peer](gc-peer.md). GC incorporated fixed-last
code lifetime and reserved-storage assertions before M2; M1 reader source review
requested preservation of remote high-pointer rejection before sentinel bypass.
Awaiting merged release/debug checks and independent post-review.

## M2 producer and first precise bridge

Implemented in `BASE/worktrees/call55-pins` over integrated M1
`49c081b035821feb472faf59d905ad73bdcc6d4b`; GC source peer GO in
[m2-peer](m2-peer.md). Root's subsequent M1 cold API and escaped-frame test
corrections are separate integration changes and must be retained when applying
this patch.

`_PyFrame_TryClearExceptCode` clears only callback-free owners and returns a
stage/remaining prefix before the first possible reentry. Outlined noinline
continuations then link the nested GC record and finish all retirement. Thread
retirement keeps the record through final code release, disarms the result, pops
the record and releases the reservation without further callbacks. Generator
completion and direct close/deallocation share `_PyFrame_ClearGenerator`, keeping
the record through exception-state release while preserving the original lock
scope. Code remains pinned for completed generators. The obsolete general clear
entry has no remaining callers.

The real generator exception owner required mask bit16; takeover moves homes and
function/mapping/association authority while preserving independent code plus
the original generator exception. Every cursor/header removal precedes close.
Collectors enumerate explicit fields after code detach and never decode freed
geometry. Debug assertions check nested calls restore chunk/top/limit and the
thread frame's saved frontier. FT routes any counted object close to the slow
path; the GIL last-reference test alone is not a concurrent proof.

The precise test helper visits transition roots directly, bypassing conservative
fallback. Eight fixtures cover nested result/remaining/outer roots with arena
growth and collection, late publication after caller-local mutation, final code
constant destruction, earliest frame-association/trace destruction, unstarted
generator deallocation, generator completion, direct deallocation retaining
GeneratorExit state, and unwind cleanup. No tests or native builds run in this
lane. Own diff check passes; root quiet timing is respected.

Source cost ledger: no home clears or conservative scans are removed in M2.
Callback-free retirement adds the last-reference proof before a close, the
result-aware boundary passes/returns one value, and the thread gains one nullable
head pointer. The agreed record is 32 bytes on the initial 64-bit GIL layout and
exists only in a reached noinline continuation; nested reentry adds one record
per active retirement. These are source/layout obligations, not measured native
instruction or elapsed claims. Root must regenerate the RETURN template and
escaping-call classification, compile both roles, execute the fixtures and
inspect actual fast/slow native stack frames before integration.

## M3 selected policy and first staged arena adapter

The [collector policy](collector-policy.md) separates counted native owners from
arena authority. Revised [helper proposal](helper-roots-proposal.md) removes the
blanket CValue/CObject registration plan. Existing native stack/register/PyMem
discovery, including live numeric leaf boxes, remains explicit; M4 removes arena
scans only. The narrow dead-FT-CValue-loop cleanup has
[source peer GO](cvalue-cleanup-peer.md), and the full placement census has
[source peer GO](allocation-peer.md). Neither selects a placement or qualifies FT.

Implemented generic unpack's staged output prefix plus the typed owned-function
transfer in `call55-pins` over integrated `d3ee27923a9`. GC's
[source post-review](unpack-peer.md) is GO; root generation, builds and focused
execution remain pending. The helper consumes the sequence owner on all exits,
keeps initialized backwards outputs through iterator/sequence cleanup, advances
prefix authority before every error close and hands successful output authority
back to the NULL frame without an intervening callback. Five fixture methods
check precise simple/starred cleanup, short/excess/raised partial outputs, nested
prefixes and traced ancestor redirect rejection plus named-local mutation.

The initializer's typed transfer now uses an explicit owned-function accessor
with debug canonical/type/immortal assertions and no generic ownership fallback
in release. The helper proposal records every source initializer route and
upstream owning producer, including explicit vector-provider DUPs, guarded
window transfers, property/getitem paths, constructor and init trampoline.
Its count fixture checks that transfer adds no second count. Debug bits alone
cannot distinguish a borrowed mortal; the source ownership contract is required.

No M4 POST-phase handoff, complete binder staging or complete callback matrix is
claimed. Those remain required before removing arena scans or scratch NULL
authority. The typed helper does not change independent executing-code lifetime.
Own diff review and whitespace checks pass; this lane runs no native jobs.


## M3 bounded generic binder staging

Implemented the [reviewed outer-lifetime binder scope](binder-staging-proposal.md)
in `call55-pins` after frozen unpack component `f80a7c23676`. Both private
constructors require caller-owned transition storage. The child links its full
initialized NULL/value range and independent typed headers before the first
binding callback. EX vector, borrow-scope and tuple/dict cleanup remain inside
the outer lifetime. The fallback publishes current_child before disarming; a
failed private child transfers to M2 without a callback or poll between pop and
retirement. Exact overlap routes have no new record or per-argument stores.

Exported constructors are outlined and retain the record through their own
cleanup; their unknown caller's post-return/provider/PEP523 handoff remains an
explicit open arena-authority gate. This bounded increment does not complete
M3/M4. The source matrix preserves native counted-owner policy and identifies
actual activation and observer obligations.

Five precise-ID fixture methods cover generic EX and native provider binding,
installed positional/prior-keyword owners, defaults/kw-only/kwargs, nested
partial children, callback collection, caller-local mutation, equality failure,
selected-code mutation and record-chain exhaustion. EX finalizer-last-release
coverage remains a separate fixture obligation; ordinary fallback parent homes
retain provider tuple/dict owners until publication, so no fabricated last-close
claim is made. Host AST parse, usage census and diff whitespace checks pass.
No candidate native code or build ran in this lane. GC's
[source post-review](binder-staging-peer.md) is GO for both production changes
and all five fixtures; the two-file runtime diff is frozen for root validation.

## M3 provider handoff protocol

Root and GC approved the full
[provider/PEP 523 protocol](provider-handoff-proposal.md). A typed STAGED node
keeps initialized child homes through the actual outer provider and hook scope.
External evaluator publication or gapless retirement handoff disarms all
frame authority and sets its pointer NULL; the empty node stays linked through
post-default cleanup. Strict custom consumption and optional default handoff
share one lookup primitive. Outer providers retire an unconsumed child on hook
substitute/error, using the node's state rather than inspecting consumed frame
storage. No head lookup enters ordinary exact CALL/SEND dispatch or hot return.

GC owns packed kind/header, visitor, retiring kind and all-node four-field query
ports. The current record remains 40 bytes and reason/status offsets remain
22/23. Frames began the thin value-provider delegation and explicit ceval
interfaces; evaluator and fixture edits await root's coherent reason/schema
merge. Source-only peer GO for GC's four-file kind delta is recorded in team
messages. Full source post-review and root execution remain pending.

The production perf-trampoline hook delegates once to saved/default evaluation;
its preceding compile/write-state callbacks are inside the live stage, and it
reads no frame state after delegate return. Raw pop/copy consumers and initshim
construction/failure are enumerated. Shim ownership remains a separate actual
arena gate; successful callback-free handoff alone does not prove failure paths.


Provider source is now complete over `63c35c2b6d7`: five files, with one shared
outlined kind/identity handoff, object/value outer scopes and explicit custom
activate/retire contracts. Empty nodes hold no frame pointer after publication;
missing-token paths test identity before storage reads, including NULL and
second consumption. Constructor-success flags guard record use. No inline
CALL/SEND/hot-return branch or generated source changed.

Seven fixture methods and a scoped selective C hook cover default delegation,
pre/post nested callback reuse, arena growth/collection, abandonment success and
error, explicit retire/custom activation, forced recoverable entry failure,
code mutation, object scalar-result identity and constructor failure. Existing
retirement queries already select RETIRING explicitly. AST parse, usage census
and diff checks pass. GC source post-review is requested; root owns native
builds, execution, cost evidence and integration. Independent initshim arena
ownership remains open and is being designed in the GC lane.


GC provider production post-review is GO. Its test-helper reentry finding is
fixed with an explicit running guard across all selected-code/observer/result
closes. A sixth fixture triggers actual last-code-constant destruction after
code replacement, attempts nested probe entry during cleanup, checks rejection
and later reuse. Host fixture AST and whitespace checks pass. Final fixture
peer and root native validation remain pending; no candidate execution ran here.


STAGED nullable LOCALS/FRAME_OBJECT authority is unconditional after GC contract
review, permitting sanctioned hook field installation before publication while
execution pins and storage geometry remain fixed. A seventh fixture installs an
actual strong locals mapping and checks direct transition identity before versus
body/post handoff. RETIRING masks remain progress snapshots. Fixture AST and
whitespace checks pass; final peer and root execution remain pending.


Provider final source/fixture peer is GO. The frozen diff is exactly
Include/internal/pycore_ceval.h, Objects/call.c, Python/ceval.c,
Modules/_testinternalcapi.c and Lib/test/test_pyvalue_api.py over `63c35c2b6d7`
(619+/43-). All seven fixture methods parse; usage and whitespace checks pass.
Root has the commit-ready manifest and focused provider/retirement/binder/unpack/
PEP523 test list. No runtime/generation job ran here; root owns validation and
integration. Actual phase maps and initshim arena ownership remain independent
M3/M4 gates, with bounded peer work continuing in the other lanes.

The coordinator's full provider composition `de95475accb` passes the separate
precise, core and borrow-scope groups in both roles. The bounded cold/native
review is in `m3-shadow-integrated-cold-review.md`; exact CALL/RETURN retain
their 64/16-byte reservations without token search, while outlined provider
fixed frames and constructor stack arguments are explicitly charged. The
placement cost model has GC arithmetic GO; root selects shared storage with
initialized NULL named locals as the M4 reference. Dynamic validity remains an
optional alternative, not a consequence of splitting bases.

Revised init-shim design and implementation receive frames source GO in
`init-shim-peer.md`, with generated error-before-DEAD and publication ordering
left to root validation. Shared-emitter lifecycle design GO is recorded in
`semantic-phase-peer.md`: a reached nonempty binary survivor and positive leaf
transport events are the next evidence, not another empty-retirement claim.

Pins merges the coherent provider base as `56a8a38fcf8`. Next bounded source
work is the private heap-frame deallocation proposal: code fixed last, actual
home/header transition cursor, and detached observer metadata in native typed
workspace. It is separate from mutable live tp_clear. No schema/native head
expansion is proposed; GC peer precedes implementation.

The private heap deallocation proposal receives written GC design GO in
`heap-dealloc-peer.md`. Its two-file implementation is ready for source
postreview: a NO_INLINE owned-frame helper links remaining homes/typed headers
before weakref callbacks, preserves function/mapping positions, reuses the
existing cursor sweep, then closes native metadata and fixed-last code. The
nonowning/trashcan address gate remains. A focused fixture runs both plain and
nested deallocation, observing weakref/home/metadata order, remaining/code
precise IDs, and final release. Host AST parsing and diff checks pass; no
candidate execution occurred. IR's phase-field merge must add this helper's
zero reset before its first callback; the current source base has no field.

GC's full heap source/fixture postreview is GO. The two-file patch is frozen
over `56a8a38fcf8` (129 insertions/17 deletions), with the phase reset retained
as an explicit coherent-integration requirement. Root has its manifest and
focused test group; native evidence remains pending.
