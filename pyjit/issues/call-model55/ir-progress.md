# IR lane progress

2026-11-12: Read canonical coordination, model55 proposal/resolution, applicable
AGENTS and the official CPython AI policy. Audited selected54 source in the
isolated `BASE/worktrees/call55-ir`; canonical runtime source was not used.

M0 found a split admission boundary: `_PyCode_Validate` is common, but public
`check_words` alone checks operands/normalizes changed inline constants, and
`_PyCode_New` trusts callers. Existing regcfg, outcome and encoded worklist
proofs are the extension points. See [proposal](ir-proposal.md) for the bounded
first executable M1 change, follow-on contracts and tests.

Root peer review approved the bounded admission increment. The isolated source
now makes `_PyCode_New` the common structural-check, word-normalization and
semantic-proof boundary, using a copied constructor. Assembly, marshal and
public construction no longer perform separate admission. Existing zero-code
padding remains accepted. Window/output extents, local/register domains,
branch targets, name/constant indices and typed feedback entries are checked;
frame geometry is bounded before integer addition. Inline constants normalize
on every route. This is admission hardening, not full definite assignment or
an ownership-map proof.

Authored `Lib/test/test_ir_admission.py` covers constructor/replace and patched
marshal records without executing malformed code, plus valid round trips and
constant normalization. Host AST parsing and `git diff --check` pass. No builds,
candidate runtime runs or native tests were performed by this lane. Diff is
ready for independent GC-lane review and root compilation/execution. Root owns
generation, validation, integration and commits. NULL homes remain authoritative.

M1b component is committed as `617ae00272d`; root integrated it separately.
Root's first diagnostic exposed test-only handler-name lookup and earlier-error
fixture issues. Corrected only the authorized integrated test files, preserving
completion-proof coverage with valid synthetic names and matching variable
counts. Root owns their rerun and commit.

The reviewed first M2 increment is now source-complete in the IR worktree:
generated, source-checked normal transfer kinds; a cold logical/home transfer
query; and an admission worklist that consumes those facts. It covers actual
scratch extent with separate definite-NULL, definite-initialized and possible
remaining-owner bitvectors per basic block. No arbitrary home cutoff, persistent descriptor,
frame phase field or dispatch-time decoder was added. Unknown rows never grant
logical/owner facts; independent physical proof can retain only already-known
NULL facts for untouched homes. Existing completion provenance checks remain
unchanged. Error and observer handler entries are seeded unknown, and checks
run after the fixed point. This is partial normal-edge hardening, not full
phase/owner admission or M4 authority.

GC peer found and root accepted one join correction: definite value facts
intersect, while remaining-owner obligations union. The implementation now
uses explicit `SEM_MAY_OWNER` bits and tests dangling owners on either diamond
arm at both overwrite and exit, independent of predecessor arrival order.
A may-owner bit does not authorize reading a home on another incoming path;
M4 still needs initialized join normalization or rejection. The other peer
integration obligation is root's exact RETURN token-template update for the
new result-aware retirement helper; the new transfer table reuses that one
existing predicate and introduces no duplicated RETURN/YIELD template.

Tests cover all public/marshal routes, duplication/move/retirement, overwritten
and abandoned owners, joins/loops, unknown barriers, unbound-local transfer,
implicit call NULL, failure edges, scratch homes through 4094 and a long straight
image. Source perturbation and real-family coverage checks make generated facts
nonvacuous. Host AST parsing and diff checks pass. No generators/builds/candidate
tests were run by this lane. GC independent postreview approved the corrected
mixed-lattice worklist and diamond tests; that finding is closed. Source is
frozen for root generation and release/debug checks. The exact RETURN/YIELD
source predicates must match the integrated runtime without weakening their
token checks.

Root merged/regenerated the normal-transfer increment and built both roles.
New admission tests pass. Generator source fixtures needed newline/updated
RETURN-helper expectations, and observer fixtures now distinguish statically
unavailable-input rejection from valid runtime restoration/jump behavior.
Root's subsequent merged M3 checkpoint reports 961 focused tests passing in
both roles. The worklist still supplies partial admission hardening only.

The current [semantic phase plan](semantic-phase-plan.md) has root, frames and
GC design GO. Its first persistent retirement certificate/query is implemented
and frozen after GC source postreview GO: the shared proof uses an immutable image
view and separate monotone all-path completeness; exact normal returns require
every scratch home canonically NULL and no remaining obligation. Unsupported
and all-certified/default code allocate no side plan, mixed coverage stores
sorted site IDs. An eight-byte executing-header pointer preserves the existing
local-count offset relative to words; interpreter frame size is unchanged.
The cold no-allocation query validates image/site/reason before inspecting the
full scratch control. GC supplies stored status/query/probe, root supplies
literal producer hooks. No generated files/builds/native tests ran in this lane.
Peer corrections preserve canonical physical zero-stack geometry and reject
unproved RESUME pointer writes through a positive complete-body contract.
The cold probe entry is exported for the internal test extension.

This first empty-scratch shape is plumbing evidence, not useful complete map
authority. Shared emitter callback/retire/publication events, call NULL protocol,
error/staged/suspended phases and lifecycle consumers follow before M4.

Root approved the profile-led fast-close contract after GC review. The isolated
source patch adds `_PyValue_TryXCloseNoReentry` and changes only the first
retirement slot loop: existing ownership classification, one existing mortal
decrement on a GIL-proved non-last owner, false with owner intact otherwise.
Focused probes cover exact count changes, last-owner retention and one finalizer,
pending exception identity, inline/NULL/index/no-count tags, both wrapped-pointer
encodings and a dynamically immortalized prior untagged owner. Host syntax and
diff checks pass; independent GC source postreview GO is recorded. Source is
frozen for root native/performance checks.
The integrated RESUME contract now compares parsed preprocessor structure rather
than its flattened token stream; actual-source host analysis covers RESUME and
rejects shifted macro branches and opaque scratch mutations. GC source GO;
root owns generation and executable validation.

The next concrete shared-emitter increment is specified in
[semantic-phase-plan.md](semantic-phase-plan.md): one event graph from the
actual value transport/publication/escape emission drives normal admission and
reached collector phase shadowing during generic binary operations with live
scratch survivors. It includes an explicit padding-field validity protocol,
nullable joins, helper capabilities, call NULL forms, multi-exit examples,
instrumentation/error/observer requirements and full M4 coverage gates. Frames
and GC design peer review are GO. The source implementation is in progress in
the isolated IR lane; the independent remote fixture repair is already committed.

The remote fixture repair is source-reviewed commit `08a5dc225b0`: prepare the
valid executing image before deliberately corrupting the child source bytes.
The cold normal proof currently reads only observer recovery handler targets
from IRSM, not source positions; this semantic-container coupling is documented
as debt. Shared phase metadata must separate recovery edges from optional debug
positions. Ordinary dispatch and retirement shadow queries do not decode IRSM.

The shared-emitter source now emits a compact ownership program beside the
generic binary transaction's actual TAKE, provider, result and restore commits.
The existing audited leaf contracts enter that same consumer as LEAF events;
their old positive source guards remain until their emission is ported. Normal
admission consumes the program. A unified `_PyIRCodePlan` owns sparse reached
phase rows and hash-interned scratch ranges through home4094, using the existing
execution-image pointer. `_PyIR_GetFrameHomeView` selects one borrowed view for
the actual collector shadow and later lifecycle ports. Nullable joined ranges
are canonical NULL-shadow candidates, not permission to read inactive stale
cells after the authority switch.

GC's bounded source postreview is GO, subject to the separately owned accepted
redirect resets and complete producer manifest. Root's host generator smoke
succeeded for cases and metadata without changing generated files. A subsequent
formatting correction separates generated closing braces and continuation
lines; final root regeneration remains required. New fixtures cover cold shape
mismatches, image/site/phase rejection, full extent and all public admission
routes, duplicate scratch TAKE, actual provider/finalizer frame-root walks with
a scratch survivor, and dynamic binding changes before error restore. Runtime
results, native layout/costs and performance are not yet claimed.

CALL-role source packet is frozen for root integration after independent GC
source GO. Thirty-five generated canonical heads/twins/specializations retain
explicit self/kwargs virtual-input masks through cold specialization and
instrumentation. Public admission requires a separate must-NO_HOME fact at each
virtual hole; UNKNOWN never supplies absence. The fact covers inline/index
values as well as counted owners, and differs from physical NULL because a
returned child may leave nonowning metadata bytes. Actual successful call drains
and same-frame final flush/vacate produce finite normal-edge events; all family
members must have a positive recipe. Exception/observer recovery seeds only the
actual cleared tail, with unknown retained prefix. One additional scratch
bitvector extends transient proof storage; no frame field or hot plan lookup is
added. The finite cold role-dispatch table costs 1024 bytes plus 35 handlers and
format rows; root owns final compiled-size/native pricing.

Root's composite host cases/metadata generation passed. New source fixtures
cover serialized/public malformed holes, joins/UNKNOWN, reused tuple/child-call
homes, full extent, real loops/except/generator code, specialization/hooks,
post-expansion weakref callbacks, and actual fixed-arity arena CALL_KW native
prepend with local mutation/GC/errors. They have not yet run natively. This
normal-edge packet does not certify complete callback/error/suspended maps or
switch GC authority. The next packet must extend the shared emitted event graph
to remaining finite family outcomes and reached publication phases, rather than
adding another independent decoder.

The first merged CALL build exposed missing positive consumption facts after
fused branches. The checked instrumented branch wrappers perform their own
final owner commit and dispatch, so the outer common-flush path is unreachable.
The generator now records both checked terminal normal edges and retains the
normal-clear recipe only when both exist and satisfy the same source capability.
Independent GC source review approved this boundary. Root's host contract check
then confirmed the recipe for all eleven fused heads, specializations and
instrumented twins. That resolves the source-coverage hold; merged native
admission and runtime validation remain pending. A separate admitted-feedback
decoder repair removed the earlier freezer crash, but freezer admission had
still rejected unproved virtual inputs before this branch repair.

Read-only postreview of integrated `63ec08b5545` confirms all eleven generated
fused metadata rows select the single NORMAL_CLEAR_INPUTS event. A host scan of
66 generated CALL handlers with initialized scalar roles found 218 canonical
redispatch sites, each with an EFFECTS bridge to its full-state fallback label.
The no-self exact and exact-bound handlers use `args = &window[2]`; after bound
wrapper release, version, argument, capacity and recursion misses retain the
promoted SELF state in the typed runner. This is generated-source evidence,
not runtime qualification. Repeated adjacent phase-zero stores remain visible
in generated text; native optimization and the final cost remain root gates.

The next freezer attempt progressed beyond those fused branches and found two
further proof gaps: DICT_MERGE's consumed trailing input and FOR_ITER's taken
completion edge. Source repair `566bec78552` excludes the unchanged through
prefix when consuming normal inputs. A separate emitted completion event,
intersected across the canonical loop family, applies only to the taken edge.
It proves logical absence of the retired receiver/cursor and preserves prior
absence of the unused body destination. It does not invent physical NULL or
readability. The body state is enqueued before that completion mutation.

Independent GC source review approved the final proof and its public-image
negative for a live body cursor. Root's host generation and actual recipe
assertions passed for through operations and the complete FOR family. The
combined native freezer probes and runtime fixtures remain pending; the
earlier full build did not pass freezer admission.

Later bounded source repairs cover actual unpack/local-result moves and
folded collection inputs. Root's bootstrap probes reached5/5 after SEND
completion transport. The folded-input footprint repair91f52d60cab initially
missed its WINDOW guard and regressed scalar peek preservation;25f090c08a4
adds that guard and an all-route GET_LEN prior-empty/prior-live regression.
GC independently source-reviewed both changes. Root owns the repeat native
census; no full CALL qualification is claimed here.

The delegated-yield source draft now uses explicit D plus a finite all-path
SEND body-role tuple. Frames and GC consumer source reviews are complete;
IR emitter/admission reviews, host residuals and combined native checks are
still pending. The current whole-frame phase map remains partial and NULL
control remains authoritative.
