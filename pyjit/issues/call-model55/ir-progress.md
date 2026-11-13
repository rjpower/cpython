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
