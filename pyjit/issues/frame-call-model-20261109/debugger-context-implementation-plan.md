# Next context increment: derived inherited and same-caught transfers

Exact implementation proposal, not admission. Runtime/tests remain frozen for
native24. No target exception-link writes are part of this increment.

## Scope and private records

Build on the existing cold register analysis, not a frontend stack simulator.
Keep one request-owned `RegisterResumeAnalysis` containing physical liveness,
invariant-NULL facts, decoded EH edges and derived context state. Keep a separate
`RegisterResumePlan` with target phase and a preserve-home bitmap; NULL has an
empty bitmap, FOR has its checked pair, caught has its checked saved owners.
Apply all plans through the existing detach-all/commit-before-close transaction.
This replaces a growing sequence of independent setter branches.

The context table has one root INHERITED node (which may represent a non-None
caller exception), and one candidate CAUGHT node per actual PUSH_EXC_INFO PC:
parent context, producer PC and saved-previous value token. A site state is
UNREACHED, one exact node, or AMBIGUOUS, plus physical-home previous-value origins.
These are ownership/protocol facts, not simulated Python values or stack depth.

Use a flat join: the first edge initializes; equal contexts meet owner origins;
differing contexts become AMBIGUOUS. Origin disagreement becomes unknown.
AMBIGUOUS is absorbing. At a PUSH reached with differing parent contexts, refuse
that node's use rather than clone an unbounded context tree. Detect parent
cycles/depth beyond CO_MAXBLOCKS21 as unavailable. This bounds malformed looping
PUSH programs without rejecting their execution merely to build debugger data.

A COPY creates another owned edge with the same previous-value origin; it does
not create a new caught generation. Actual dynamic-owner preservation is proved
separately for a transfer. The initial plan only preserves homes whose origins
and role agree at both selected sites and whose path slice has no kill/read/
PUSH/POP/reentry. Unknown transports remain unavailable.

## Derive from actual final operations

- Entry context is INHERITED, every tracked previous-origin unknown.
- PUSH_EXC_INFO consumes the delivered exception, places the previous-value
  token in actual B and the new exception in B+1, then enters its caught node.
  Use actual WINDOW output mapping; A is not a fresh-output promise.
- POP_EXCEPT restores parent only when its explicit input carries the active
  node's saved-previous origin. Otherwise context becomes AMBIGUOUS.
- COPY maps source B to its actual fresh output; SWAP permutes its endpoints.
  Preserve untouched window holes. Snapshot sources before applying definitions.
- For all other operations, fresh definitions/outcomes kill origins. Any use of
  a tracked origin through an unaudited semantic read invalidates that origin
  conservatively: do not infer generic consumption or leave a stale token after
  a possible move. Unknown pushed-child overlap invalidates affected homes.

The normal CFG uses the existing physical successor helper, including real
relaxation jumps. Add canonical EH edges: unwind clears suffix at floor,
lasti/exception writes are fresh, and active handled context remains until real
PUSH/POP changes it. Raising operations use pre-success facts, conservatively
invalidating origins that an unknown consuming read could have taken. No success
output may justify an error-path input.

Source-only callback EH edges use each outgoing normal outcome's post-from
state, then its own recorded unwind recipe. Source current phase is the actual
post-from edge, not blindly the instruction state at event.to. Include both
ordinary and source-error ingress so a forged map cannot manufacture context.

## Shared EH reader and bounded validation

Factor the constructor's canonical EH validation/decoding into one cold helper
consumed by constructor and debugger: marked entry starts, bounded varints,
sorted nonoverlapping ranges, valid targets and payload/floor homes. Preserve
existing constructor diagnostics and semantics. Do not add a second scan with
different runtime lookup assumptions. Before new analysis dereferences roles or
successors, check the relevant named-local/scratch bounds and valid handler.

No serialized context input is trusted: every request derives from the final
encoded words and EH/source maps. Code.replace naturally gets a fresh proof.
No marshal/codegen/assembler/remote schema migration, code field or hot work is
needed for this increment. Checked allocation/memory limits reject the debugger
request before mutation. A later cache requires a measured reason.

## Target selection and transfer proof

Retain the identity path. For a nonidentity request, select among existing real
and source targets in the current order, testing the full phase/owner recipe:

1. INHERITED→INHERITED: existing target incoming owner/NULL proof must pass.
   Ordinary try entry/exit and normal finally copies need no caught-link action.
2. Same exact CAUGHT: keep the active node and parents unchanged, require target
   and current ordinary needs to fit the available checked saved-owner homes,
   and prove an acyclic no-cut slice between endpoints in the admitted direction.
   The initial witness is backward PC11→10 in caught_exception, preserving home3.
   Preserve its actual None-or-exception owner; never assign None to reconstruct
   an absent previous exception. Match the original bounded FOR dynamic proof.
3. Different/ambiguous caught context, with/comprehension roots, suspension,
   pending child or target needing unrelated temporary reconstruction: retain
   refusal for this increment. No writes to tstate->exc_info or callback cells.

Include EH dependencies in physical liveness: preserved-prefix reads of a
handler are incoming requirements; exception/lasti definitions are not. This
prevents an ordinary-context label from authorizing lost with/comprehension
owners. Same-caught requirements also include the saved previous-context owner
even when the target is bare RAISE_VARARGS0 with no explicit operand.

Warning hooks precede final revalidation. A nested generation commit cancels the
outer request; otherwise rerun/revalidate context, handlers and actual homes.
The preserve bitmap then feeds the single application transaction. No frame or
context writes follow closing callbacks.

## Minimal source boundary and tests

B: frameobject cold analysis/plans + narrow shared EH factoring in codeobject
and an internal cold header; focused code/source-context tests. Coordinate shared
header placement with root/A before edit. A peers PUSH/POP/COPY/SWAP/error phases.
No bytecodes/generator/interpreter entry changes for this increment.

Keep existing debugger tests unchanged. First new witnesses:

- ordinary try-body backward replay and outside→try-body, exact body effects;
- root→bare-reraise caught body refused before mutation;
- caught PC11→10 replay with the same sys.exception() object and saved owner;
- exceptional finally copy is not confused with normal same-line copy;
- callback-owned LookupError over target exception stays untouched;
- malformed EH/role/source-ingress negatives use existing constructor geometry.

Coordinator runs these plus original try-entry/finally/refusal tests first, then
existing trace/code/source-error modules. This is a bounded implementation rung,
not a waiver for caught exits, nested contexts, comprehensions or yield phases.

## Source26 implementation and review

Root admitted this increment after the A peer. Source26 is frozen for merged
native qualification; no worker native jobs were run.

The implementation factors marked canonical EH decoding into
`pycore_ir_exception.h`, consumed by construction and request-owned debugger
analysis. The constructor's rejection path is unchanged. Analysis owns decoded
per-PC handler recipes, indexed source records, physical incoming-owner bits,
context/origin facts, and a sparse forward/reverse graph. Capacity is bounded by
two normal edges, one ordinary EH edge per word, and one source-error edge per
record; products are checked before allocation. Reverse reachability and dynamic
survival walks use queues, rather than rebuilding all-pairs adjacency. Source
records are decoded and indexed once per request.

Ordinary EH requirements join after success definitions have been killed;
source-error requirements join the outgoing post-outcome state first. EH payload
writes also exclude homes from the whole-code invariant-NULL proof. The actual
COPY/SWAP BASE field, PUSH window, and explicit POP_EXCEPT input come from their
physical roles. Unknown transports and scratch permutations crossing named locals
lose the proof. No exception-link writes occur.

Every selected plan has one preserve bitmap. Inherited empty-owner plans,
existing unprotected FOR pairs, and same-caught saved-previous owners all use the
same detach-all, publish continuation/generation, then close transaction.
Warning callbacks precede reanalysis; nested commits retain precedence. The
existing FOR adapter still refuses protected anchor cases because its dynamic
phase proof covers ordinary edges only. Protected source-only destination
recipes, caught exits, suspension and comprehension restoration remain separate
work; an actual source callback origin must agree with its post-operation
context before a transfer is considered.

A reviewed the actual initial draft and the sparse-graph refinement. One C
identifier collision was fixed before freeze. Focused new tests cover protected
inherited replay/entry, same-caught replay with both target and callback exception
owners unchanged, and refusal to fabricate a caught bare-raise context or leave
one without a restoration recipe. Existing tests were retained. Host AST and
whitespace checks pass; native results belong to the coordinator.
