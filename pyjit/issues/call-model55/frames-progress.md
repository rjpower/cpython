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

M5's concrete rival design is in `bitmap-implementation.md`: chunk-tail and
heap-tail physical bits, a priced eight-byte frame pointer, unchanged handler
arguments, actual generated/raw write ports and local mutation/teardown rules.
GC bounded storage/handoff peer is positive after correcting worked heap-tail
arithmetic and endpoint masking; FT deferred roots retain the separate tag
scan. IR design peer is pending. No bitmap source is implemented. Root next
prioritizes the M4 selected-home copy/escape packet; active code-plan lookup,
explicit remaining-record authority and durable cold captured state must share
one view before editing the full-N copy loop.

The full-foundation injected-OOM crash takes priority over copy preparation.
Root identified the SIMPLE_FUNCTION/SPEC_FAIL_OTHER zero collision in optional
specialization. `exec-preparation-oom-proposal.md` records the bounded diagnosis
and selected fix: pure typed signature classification plus explicit bool image
readiness before every cache nomination. GC/IR design and source peers are GO.
The frozen patch is only Python/specialize.c (53+/23-) over `b722330ef3f`;
stats-only StoreSubscr classification no longer prepares code. GENERAL/KW,
property/getattribute, init and getitem all require positive readiness before
publication. The original dict OOM fixture is unchanged. Root owns the fault,
retry, control and both-role checks; no native job ran in this lane.

Root's unchanged OOM fixture now passes both roles with the readiness fix.
Pins merges the integrated phase foundation at `9dedff18010`. The first
selected-copy packet is concrete in nine files: shared range iteration,
kind-discriminated home authority, all producer/disarm ports, actual retirement
cursor capture and all three copy callers. It adds 16 cold frame-object bytes,
no interpreter-header or transition-record growth. Candidate shapes never
authorize a read; unsupported/created states use an explicitly named temporary
NULL-control adapter. Destination gaps stay NULL-filled while that control
remains. GC/IR source peers are requested; their small shared authority/union
components must be combined before generation. Five focused fixtures and the
selected-copy poison probe pass host AST/whitespace checks only. Actual normal
generator created/suspended certification is the next bounded useful packet;
this schema port does not claim it or M4 scanner removal.

IR found two bounded copy issues before freeze: the valid zero-binding
takeover starts with NONE/remaining zero, and an addressable view cannot live
in RETURN_GENERATOR's tail-called handler. Both are corrected: explicit empty
control capture plus a fifth fixture, and an outlined created-copy helper
owning the query/view/PC advance/copy. GC and IR final source peers are GO;
the nine-file patch is frozen over `9dedff18010` (352+/10-). Root owns coherent
schema integration, generation and runtime verification. No native job ran in
this lane.

The first copy packet commits as `f54f99d0914`. Next, the independently reviewed
created allocation fix passes the executing code pin through `_Py_MakeCoro`
and its three private constructors. Kind and geometry no longer consult
mutable function code; names remain function metadata. The separate four-file
patch adds one provider regression with four subcases (object/value provider,
larger generator replaced by smaller ordinary/generator code before default).
AST/whitespace and full caller census checks pass; source peers are requested.
The actual created-shape certificate remains deferred until the shared builder
and event generator are available. No native execution occurred here.

GC and IR source peers approve the allocator patch; its four files are frozen
over `f54f99d0914` (51+/9-). Root's integrated first-copy composition passes
the separate 84 precise and 588 core tests per role. Root native inspection
prices the outlined copy helper at 88 local plus 48 saved bytes (136 below
entry), including its addressable-view stack canary. RETURN_GENERATOR keeps
16 local bytes; exact CALL/RETURN keep 64/16. The next created certificate must
give its certified common path a separate callback-free copy from the outlined
query/view control path, rather than paying the latter's entry cost always.

Created certificate source is concrete on `81a76326039`: six frames files
(186+/29-) count actual event sites against stable empty proofs, share all
flag-only defaults, and consume the flag in a no-view common copy with a
separate outlined control continuation. IR owns the event/semantic-step and
audited local-only prefix contracts on disjoint hunks. Three new test methods
cover all generator kinds via actual poison-copy probes, cell/free-variable
resume and genexpr rejection; IR adds synthetic branch/site negatives. IR
and GC actual source peers are GO. Host AST/diff checks pass; the packet is
frozen for root generation/native validation. Normal-created/suspended reader
authority is not implied by the copy flag.

The copy component commits as `514fb9af661`. The next created-reader packet
has both design peers and concrete producer/selector source: exact standalone
bit12 marker after certified copy, CAPTURED-first common getter with actual
GEN/CREATED/ready-pin checks, and no phase-zero reacquisition. GC's two actual
collector ports use the common iterator under GIL or FT world-stop, retaining
the existing control path otherwise. The three-file frames patch (97+/3-)
adds live local-write/collect/resume and marker-invalidation coverage plus an
actual GIL visitor poison probe on private untracked embedded storage. GC
producer and collector source peers are GO; IR final producer peer and root
coherent generation/native checks remain pending. No native job ran here.

IR final selector peer is GO; root explicitly requested the three-file commit,
which is `4a8b591a78d`. The literal remains separate (`37d3947eec8`). Root's
current created-empty composition passes its both-role groups; selector
integration/native costs remain coordinator work.

Full M5 implementation is authorized in isolated `call55-bitmap` on `9f28`.
Storage/address source receives GC GO, and actual writer ports now cover the
emitter commits, core copies, bind/default/unpack/call paths and observer local
writes. The ordinary-home API is strict; native entry has its own checked
capability. Complete writer census, helper/failure ports and collector policy
remain gates before builds/authority. Root approves an earlier GIL counted
bitmap switch after those gates, independently of complete general static
callback maps; zero bits confer no read and generic introspection/FT deferred
semantics remain separate. The amended bitmap design records this schedule.

The bounded bitmap snapshot now includes chunk/heap storage, ordinary writer
ports, checked native-entry emission, raw forward/reverse owner iteration and
the private GIL trace-root adapter. GC's storage peer and my collector peer
are GO. A new cold probe covers all 64 first-bit alignments, clipped word
endpoints, alias-frame moves, count-to-inline replacement and balanced drain;
size tests charge the larger header and conservative heap tail. Host AST and
whitespace checks pass; none of these fixtures has run on the candidate.

Freeze gates are the IR writer-source peer and GC's probe/reverse peer. Root
then merges the stable CALL ABI, created selector and vacant-return changes
before further rival ports. The lower frame checker/clear routines remain a
separate GC-owned slice. Full writer closure, coherent generation and native
checks are still required; this snapshot is not a working authority claim.

Root independently peers the emitter/writer slice and authorizes the bounded
commit without waiting for IR's urgent CALL proof work. The exact 19-file
snapshot is `dd633ad69b8` (682+/176-), with a clean bitmap worktree. All bitmap
mutations pause for root's coherent ABI merge. GC confirms its lower frame
clear/checker regions are still untouched; those remain the next writer and
validation gate. There is no candidate execution or completeness claim.

After the CALL merge, alias/helper census finds and ports raw array writers
and actual BUILD_STRING/MAP argument cleanup. GC's mixed-storage helper and
fixture peer is GO. Eight frames-owned files remain frozen for root review;
GC independently commits reverse retirement/checker as `cc362d4a23c` after
my actual source peer. The checker now respects the live/remaining prefix;
waiting THREAD full-range probes reject, while callback fixtures use separate
generator storage. Generated output review and native writer checks remain
root gates. A delegated-suspension role proposal records the receiver/resume
obligations without reusing the old slot subtraction as proof.

GC architecture peer approves the explicit YIELD_DELEGATED proposal. It now
fixes A/B/unused-C/D operand meanings, actual through-operand lowering and transient
costs, independent finite-role versus full-map authority, saved successful
close/throw claims, the missing FT getter read claim and remote validation.
The proposed single pending SEND tuple is an IR feasibility gate. Source-file
ownership is explicit; no shared IR runtime edits occurred. The completed
constructor audit found no additional home-bearing fake without bitmap
storage. All candidate execution remains coordinator work.


Bitmap generation exposes a proof-capability gap before native validation:
the existing normal-vacate gate rejects every `frame` token, including strict
home writers and checked helper frame arguments. A bounded generator patch
adds nine complete body plus stack-IO footprint certificates, independent of
instruction names. Other frame access and raw slots/window/dispatch remain
rejected; recognized helper calls also require the certificate even if their
frame argument changes. Both new host-only parser checks pass, covering the
eleven affected families and altered bounds, storage, helper arguments, IO,
and mutable AST bodies. GC/IR source peers and root regeneration remain
required. Root's generated files were left untouched; no native job ran.

IR's required AST branch preservation and GC's renamed capability-family gate
are applied. All three host parser checks pass; both peers give bounded source
GO. The two source files are frozen for root regeneration. Whole-body source
matches remain a temporary bridge, with emitter-derived writer/helper ports
recorded as an explicit M5/M7 gate in bitmap-writer-capability-plan.md.


The delegated frames packet is implemented on pins e03: literal terminal
publisher, pre-disarm D selection with copy-view inclusion, exact captured
combo, incomplete-PC invalidation, and six focused fixture methods. The
existing private copy probe now covers selected/clipped D and the captured
selector without executing fake storage. Suspended frame.clear retains its
existing RuntimeError; rejected clear preserves the marker, actual close and
subsequent clear invalidate it. PY_YIELD observes before publication and its
recipe-less redirect still rejects. GC/IR source peers are requested; enum,
finite lookup, bytecode terminal invocation and consumers are separate lane
packets. No native execution occurred.


Root's urgent CALL pattern2 is a separate three-file generator/test packet:
exact observer helper frame-use footprints enable the already-emitted normal
flush to prove consumed class input absence when optional self is not defined.
No runtime fill, SUPER name grant, new semantic event or row is added. The
source parser check passes; IR source GO received and GC final peer is pending.
See super-call-transport.md. The four delegated files stay unstaged separately.


Both source peers give GO for the urgent super footprint. The isolated commit
is de1e3de49ef (three files); the delegated four-file packet remains unstaged.
The earlier bitmap footprint bridge was separately committed c0646e06f1a,
excluding root's generated files. Root owns both composition validations.


The delegated frames packet is committed da0c629bfeb (four files), after both
source peers give GO and the private source owner matches the finite selector
contract. The pins tree is clean. GC's consumer/remote packet also has frames
source GO, including the stronger phase-zero/invalid-slot control. Combined
IR/GC/frames generation and native validation remain root work.


The bitmap/CALL merge review found dropped nonconflicting bitmap generator
ports before regeneration: local writer classification, native-entry storage
contract, consume/store/vacate, SEND detach and source proof guards. Root
authorized exact restoration from prior B HEAD while retaining incoming
window/folded/result-offset/observer semantics. Corrected combined source has
both IR and GC GO; all five host parser checks pass, including mixed helper
and preprocessing fallthrough negatives. Root retains merge/regeneration
ownership. No new runtime result follows from these source checks.

The next urgent CALL repair checks actual independent child transport for
LOAD_ATTR property/getattribute variants. A distinct normal-event bit permits
only their certified outside-parent footprint; the reached SAVE_STACK checks
the actual consumed-input drain, and family closure still applies. The five
source/test files are ready for final peers, GC gives source GO, and the host
parser-only test, AST checks and diff check pass. Native and emitted-receipt
checks remain root work. See implicit-child-transport-proposal.md.

The six-file GC abrupt-prefix packet also has frames lifecycle/source GO:
actual suspension release supplies a separate A/B cutoff certificate, EH
imports only incoming NO_HOME below its own floor, and unknown source or
unreached protected ingress remains an independent poison predecessor. No
readability, normal completion or pending SEND role is inherited.

The disjoint child packet is committed 5f2d245b96d after root, GC and IR actual
source GO. It contains exactly five source/test files and leaves P clean.
Emitted family receipts and native validation remain coordinator work.

The CALL role fixture-only repair is committed 2613dbe5f63 after IR/GC source
GO: valid local tuple output, variable identity comparison, explicit pair
retirement controls, exact counts below the frontend EX threshold, separate
33-argument EX coverage and arena CALL_KW native-copy coverage above eight
arguments. No production code changed.

The shared writer first packet is implemented in P and under source review.
It ports six fixed-output/helper uops with separate ordinary helper effects,
Storage-bound arrays and strict frame helpers. Two parser-only tests pass;
emission and runtime fixtures remain root work. Lexical branch-identity
confusion is fixed; parenthesized address/member-lvalue contexts remain a
source hold while root performs the frozen host emission check. See
bitmap-writer-capability-plan.md for scope and costs.

The writer address/member hold is resolved in source using complete typed
rvalue argument shapes and unary-address rejection, including parenthesized
forms. Root's first host smoke passed the two parser checks but exposed an
invalid synthetic scalar output layout; its positive fixture now declares
window storage. Revised source is frozen for host retry and final peers.

The fixed/helper writer packet is committed 99560a5115e after IR/GC actual
source GO. Root's final host checks pass the three selected source methods and
full cases/metadata generation. The delegated CAPI probe correction was
separately committed d3f16918251; it selects actual immutable delegated words
and D/A roles instead of collapsing through the old YIELD base opcode.

The next six-file loop packet is committed 948577105fb after root/GC source
GO: tuple/list output indices and SET take-before-hash share checked Storage
operations. Three parser-only checks pass; coordinator emitted/native checks
remain. The remaining writer census and costed counted-versus-occupancy reader
assessment are writer-closure-ledger.md and bitmap-authority-assessment.md.
They distinguish positive counted roots from non-counted copy/introspection
and control readers; neither is a bitmap authority or full closure claim.

The B shared-writer merge is source-resolved/staged with physical consume,
store/vacate/native-entry/SEND hooks retained. Duplicate plain WriteHome and
TakeHome definitions are removed; bitmap implementations remain authoritative.
All nine copied opcode-body bridges are replaced by the shared checked
operations. Six parser-only checks pass, GC gives actual source GO, and root
commits the merge as 209e51e521d. IR did not add an independent final source
review. Generated/native results remain coordinator evidence.

The direct tuple/list handoff is committed 2c27c4bd2e9 (five files), after GC
and root actual source GO. It uses existing helpers and final vacate, with no
extra C argument, wrapper or sweep. Native helper escape classification is
shared with lowering. Four parser checks and AST checks pass; the actual
boxing-discovery/output-order and two runtime fixtures remain root work.
Source review caught and fixed the provisional-record gate before freeze.
The next requested refinement is the costed proposal known-home-capabilities.md;
it changes no runtime source pending peer/interface freeze.

Known-home packet `6a80182f670` is committed with root/GC/IR actual source GO.
The B merge from I `3b4547380d0` is resolved and staged, including matching
counted helper bodies and expanded existing alignment probe. Four conflicts
were combined in place, preserving physical writer hooks and the new explicit
general-return extent. GC gives actual source GO; five source/parser checks
and diff checks pass. Root owns commit/regeneration/native qualification.
The authority assessment now distinguishes already-implemented counted roots
and drains from the proposed same-byte non-NULL plane for copy/traversal.
