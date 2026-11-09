# Next stopped-state repairs: committed edges and NULL requirements

Proposal only during H's isolated canonical-result experiment. No runtime edit
or native execution by B. Grounding is the pinned417 receipt
ownership31-remaining-phases.json, not an inferred SOURCE_POST phase.

## Four actual witnesses

Monitoring's callback is BRANCH_LEFT at pc4 INSTRUMENTED_POP_JUMP_IF_FALSE,
physical next pc5, requesting the next return line. The condition is consumed
before the certified terminal observer. All three peepholer callbacks are real
INSTRUMENTED_LINE at pc17/18 on line9, requesting line3. All four maps have zero
IRSM rows. The first gap is a non-neutral committed edge; the other three are
PRE target requirements. SOURCE_POST is not needed to reproduce either gap.

The actual peepholer IR has nlocalsplus5. COPY writes scratch6, STORE_FAST takes
it into x, and a later range CALL uses6 as optional self. FOR exhaustion clears
its iterator/cursor homes5/6 before the line9 stop. The line3 target flows to
LOAD_GLOBAL5 and the CALL with optional self6. The current never-written-anywhere
heuristic excludes6 because COPY once wrote it. ABI NULL acceptance alone cannot
replace an old method receiver, so deleting that check is not a repair.

## 1. Exact committed normal-edge view

Extend RegisterStoppedState, not an event-specific setter branch. Keep neutral
JUMP's borrowed-row fast case. A positive non-neutral POST_COMMITTED scope must
validate exact frame/event/generation/site, committed return_pc, selected normal
edge and physical successor, and the shared generated provider declaration.
The event may be monitoring BRANCH_LEFT/RIGHT despite a Python callback spelling;
reported source is not the owner site.

Allocate one checked request-owned context/save/origin row pair for the exact
site. Reject token-creating transfers before calling the existing normal
transfer: PUSH_EXC_INFO and actual LOCAL_MOVE save creation cannot write Analysis
parent tables in this packet. A shared read-only mode can be extracted later;
copying rows alone is not sufficient. Apply the existing selected normal transfer
once. Overlay/check the shared physical observed-home facts. Consumed inputs lose
their origin, even if a new result or NULL uses the same home; output publication
cannot revive an old token. No global landing row substitution.

Use the same Plan: initially inherited context, save-empty target and no required
value roots. Actual saved-token/no-reentry and saved-NULL binding checks remain.
Existing strict recipes are unchanged where their original phase proof applies;
never pass a non-neutral post-edge row to a pre-site survivor slice. Callback
preparation recaptures the scope and rebuilds the row/Plan; a changed generation
invalidates old edge facts. Application remains detach-all, install, commit,
invalidate stale source errors, then close with no old stores afterward.

First positive is original monitoring callback. Add consumed saved-token and
aliased replacement negatives, and warning/nested same-PC controls. No general
SOURCE_POST, caught exit, actual suspended frame or yield authority is implied.

## 2. NULL-only target requirements with independent expectation proof

Replace the whole-code never-written omission with request-owned flow facts and
two incoming requirement bitsets: VALUE and NULL_ONLY. A read becomes NULL_ONLY
only when both (a) every actual aliased read permits ABI NULL, and (b) the original
encoded control flow proves this home NULL at that read. Capability without that
expectation remains VALUE. Named-local binding safety remains separate.

Compute forward must-NULL facts on the existing sparse graph. Fresh ordinary
frame entry scratch is NULL; hidden constants/named homes are excluded. This
first rung is restricted to ordinary nonsuspending functions. Generator resume,
external entry and active child overlap do not inherit fresh-frame NULL facts;
without a separately proved entry state they retain the prior strict path.
Joins intersect reachable-path
facts. Use checked allocation products and a monotonic worklist, not repeated
source-table scans. Ordinary exception edges do not gain successful definitions. Before unwind,
affected homes lacking an exact error-publication contract become UNKNOWN;
a PRE NULL must not be assumed to survive partial publication. Positive
committed source errors take the selected POST state. Unproved legacy source
hooks lose affected-home knowledge before unwind; untouched homes survive.
Exception/lasti definitions are not NULL. This is a physical availability analysis, not
old-origin reconstruction or a whole bytecode type verifier.

H owns one shared cold normal physical-transfer contract, derived from actual
emission and existing roles/output mapping, for constructor/debugger agreement.
Minimal transfers required by these witnesses are:

- Explicit register temporary inputs are taken on normal success; outputs may
  repopulate their physical homes. Named-local borrows are not killed. A generic
  output is UNKNOWN rather than promised non-NULL or NULL.
- COPY/SWAP transport simultaneous source facts; untouched holes/through homes
  are preserved. Neither creates old provenance. Unknown window transports
  conservatively clear must-NULL knowledge; do not infer kills from all reads.
- Semantic FOR exhaustion clears iterator/cursor homes. FOR body cursor is
  NULL-or-index and therefore UNKNOWN; a moved/discarded item can be NULL.
  Reuse decoded physical facts, not the coarse Outcome.first/count as all-NULL.
- Opaque child overlap loses knowledge over its affected scratch tail. There
  is no need to invent a precise CALL-success cleanup fact for the first three
  witnesses: the later FOR exhaustion establishes homes5/6 NULL again.

The shared selected-home decoder already separates physical state and old_cut.
A physical replacement can update must-NULL while its old origin stays cut.
Unproved normal shapes are UNKNOWN, not a new observer-publication capability.
Do not advertise POST merely because normal physical transfer is known.

Backward liveness propagates VALUE and NULL_ONLY separately through the existing
edge/definition structure. COPY/SWAP are simultaneous transports: an output
requirement maps to its actual source home, not a fresh-definition kill. Merge
all aliased demands, with any VALUE demand defeating NULL_ONLY. A guaranteed
fresh definition makes incoming ownership unnecessary; NULL transfer is not a
fresh old owner. All required aliases and
all normal/error/source paths must participate. A conflict or unsupported fact
refuses conservatively. The Plan may omit a VALUE owner only for a proved
NULL_ONLY requirement, and then it requires both the stopped-view must-NULL fact
and the actual current cell being NULL. For pending-child overlap, keep the
existing no-NULL-analysis path; do not inspect child-owned cells.

At a NULL-only use, check the actual executable handler's capability, not only
its pristine family. In particular an attribute specialization must not inherit
generic deletion semantics. Monitoring reconfiguration/warning reproof repeats
this check. A required alias defeats a nullable alias through the shared helper.

This proves peepholer's6 originally expects NULL at CALL, that the current
post-exhaustion6 is NULL, and that the target path does not replace it with an
unrelated owner before its optional-self read. It does not admit a missing method
receiver merely because generic CALL can accept NULL. Tests retain all three
original warning/result/code-byte assertions, plus required-method/alias and
saved-binding negatives and EH phase tests.

## Work split and admission ladder

B: stopped view, shared Analysis bitsets/worklist consumers, Plan checks and
focused tests in frameobject/test_code. H: minimal shared physical-transfer
facts plus semantic/nullable alias tests; no hot schema unless actual emission
requires one. A: existing certified provider interfaces and phase/error/close
review. Root: concrete source admission after H's isolated measurement,
regeneration, focused native gates and integration. First gate is monitoring1
and peepholer3 plus existing restoration/refusal controls. Retained with roots,
caught-link exits and YIELD_READY remain separately evidenced capabilities.


A possible minimum shared cold interface is a per-home physical transfer fact
(PRESERVE, NULL, UNKNOWN or TRANSPORT from a named source home), parameterized by
actual word/arg/outcome and normal versus error phase. B applies simultaneous
COPY/SWAP transport to request rows. H should pin the exact representation from
actual emitter contracts before source admission; this is not permission to
add a general verifier or a hot cleanup program. Unknown affected-home facts
remain conservative. The first four witnesses have no EH/source records, so
there is no need to invent error-phase precision to make their gate pass.
