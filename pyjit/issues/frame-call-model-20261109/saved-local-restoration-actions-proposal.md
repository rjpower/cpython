# Saved-local restoration actions

Root admitted the bounded first implementation after independent peer review.
The PRE cancellation checkpoint is committed as4a1f70fcef49. Its timing freeze
has lifted and the bounded restoration source is in progress in frameobject and
focused test_code cases. Caught-exception exits remain unadmitted.

The committed checkpoint permits cancellation only when every saved-local column
is positively empty. This document defines the next capability, not a claim
that restoration is already implemented or qualified.

The first implementation uses a cached sparse-graph producer-acyclic check as
its generation witness. It requires origins for every active same-local chain
member and restores the oldest token on full exit. Ordinary nested comprehension
producers can be cyclic; those still refuse. A separate executable encoded
fixture makes two noncyclic saves name one actual local, orders normal/EH
restoration correctly, and checks oldest-binding restoration without broadening
the cycle proof.

## Actual model and evidence

`Objects/frameobject.c` now derives a column per distinct local named by
`LOAD_FAST_AND_CLEAR`. A producer records the previous top of its own column.
Negative origin labels identify saved binding values, including a saved NULL;
losing a physical origin does not empty its column. Normal and source-error
edges retain separate PRE/POST transfers. `RegisterResumePlan` currently contains
only a scratch-preservation bitmap; application detaches surplus scratch owners,
commits PC/cursors/generation, then closes detached owners.

The first global-stack draft failed a genuine positive. In
`ownership28-ancestor-inventory/inventory.json`, saves8/9 name left/right and
normal restores22/23 restore left/right; exceptional restoration reverses them.
Independent locals therefore commute. The nested witness in
`ownership28-same-local-inventory` instead has saves7/14 both naming encoded
local1, normal stores23/27 restoring inner then outer, and exceptional cleanup
starting34. Only saves of the same local impose youngest-first ordering.

The next action must use those existing columns and origins. It must not infer
restoration from a matching local name, a non-NULL home, or an exception-table
depth.

## Bounded first capability

Admit only an already-authorized real PRE_WORD phase, no active child, the same
handled-exception context, and saved-local exits whose target columns are all
empty. This first action restores the original local bindings while abandoning
an active comprehension. It does not reconstruct an active comprehension at a
target, change cells, enter a save, repair a lost origin, or alter an exception
link. Retained outer loop/with operands still need their independent survival
proof; restoration alone does not make target inputs available.

For each nonempty current column:

1. Require an unambiguous finite same-local parent chain ending at0. Validate
   every producer's actual LOCAL_MOVE index against this column.
2. Select the oldest active token, whose parent is0, as the binding to install.
   Younger saved bindings represent abandoned inner scopes. They remain owners
   to retire, not intermediate assignments that callbacks may observe.
3. Require a unique, available physical source home for that token, or an
   explicitly checked set of duplicate owners from supported COPY transport.
   A saved NULL is a valid binding-state token, not an absent proof.
4. Prove that source belongs to the currently active dynamic save generation.
   Static producer identity alone is insufficient across a backedge. Reuse the
   sparse normal/EH/source graph and cut rules: producer reentry, origin kill,
   unwind clearing, unsupported transport, and opaque child overlap cut the
   witness. The initial implementation may refuse cyclic save producers rather
   than claim that an old copied token is the current generation.

All columns must pass before producing any action. An ambiguous column or lost
origin is a refusal, even if the current physical source happens to be NULL.
Distinct columns can be planned in local-index order because their stores
commute. A future target-with-active-saves extension must restore only the
suffix above its componentwise ancestor and preserve that target's active token
owners; that extension is outside this first action.

## One request-owned plan

Extend the existing Plan with a checked array of `{local, source_home}` restore
actions and a binding-restored bitmap for named locals. Analysis remains the
same request-owned graph/context/origin computation. Allocation products use
size_t guards; action count is bounded by the number of distinct saved locals.
The plan owns no frame value before final validation.

Keep action planning and application separate:

- Planning proves source identity/survival, target needs, retained caught roots,
  phase and local kind. It marks action sources so ordinary scratch retirement
  cannot close them twice.
- Warnings and all fallible allocations precede mutation. The current warning
  count must exclude locals restored to genuine NULL: those bindings are known
  unbound states, not missing target values to fill with None. Existing behavior
  for unrelated unbound locals remains a separate policy.
- If a warning hook commits another redirect, the newer generation wins.
  Otherwise reanalyse and rebuild the complete action/preservation plan. A
  pointer/PC comparison alone is insufficient.
- In the callback-free commit section, move every selected source into a C
  owner array and NULL its home, collect displaced local owners, and detach all
  surplus scratch owners. Only then install all restored bindings, including
  NULL, and commit target PC, return_pc, cursors and redirect generation.
- Close displaced and surplus C owners after the whole new frame state is
  valid. A finalizer may redirect again or inspect every restored local. The
  old transaction performs no frame/home writes after its first close.

Selecting one COPY duplicate moves one owner; other duplicate homes are retired
normally. Aliased action sources, named-local sources, constants, and sources
also required at the target refuse unless a specific owner-transfer proof
resolves them. Equality of Python values does not resolve an ownership alias.

Ordinary local slots are the initial domain. A cell or free-variable operand
must refuse until its cell-content and exposure semantics have an explicit
action contract. This is a representation bound, not a claim that Python
requires prompt restoration/destruction at historical CPython offsets.

## Minimal tests and boundaries

Keep the existing active-comprehension refusal test as historical coverage until
the new capability is admitted, then change only its previously refused positive
branch to assert the promised restoration. Preserve lost-origin and wrong
same-local ancestor negatives. Add independently exercised cases:

- Prebound and genuinely unbound comprehension variables; both restore exactly,
  and the unbound result remains absent from locals without a None-binding
  warning.
- Two distinct locals restoring despite opposite normal/EH order, and nested
  saves of the same local restoring the oldest binding on full exit.
- A displaced iteration value whose finalizer inspects all restored locals and
  redirects to a second target; the later redirect wins.
- Warning-hook same-PC redirect and getter-only controls, using the existing
  generation protocol.
- A lost origin, an ancestor-token substitution, a retained stale token across
  producer reentry, and a cell-backed local; each refuses without partial
  binding or scratch mutation.

B owns frameobject Analysis/Plan/application and test_code. Shared cold
ReadsHome/DefinesHome/Outcome helpers remain authoritative; no generator or hot
dispatch changes are needed for this bounded action. Root owns native tests and
integration. A should peer the dynamic-generation witness and detach/install/
close transaction before source admission. Caught exits, source POST,
suspension and retained outer protocol roots remain separate proposals.

## Current source checkpoint

The draft extends RegisterResumePlan with checked per-local source-home actions.
Each selected chain member needs its actual surviving origin and a cached
no-reentry proof over the existing sparse graph. A cell/free/constant kind,
ambiguous parent, missing token, cyclic producer or target with active saves
refuses. The selected oldest token may genuinely be NULL. No new runtime owner
tag or per-dispatch tracking is added.

Application allocates retirement/value buffers before mutation. It detaches all
action sources and displaced locals, then surplus scratch, installs every final
binding, and commits PC/cursors/generation before closing C owners. Plan cleanup
covers every allocation/refusal path. Restored locals are excluded from both
unbound warning count and None fill. Actual ordinary/identity commits call A's
cold source-error invalidation; pending-child publication does not.

The single PRE factory now consumes A's scoped observer getter. An original
real-LINE spec needs matching site/generation; a prior successful LINE/source
redirect can supply a newly committed PRE landing under the existing exact
wrapper/cursor/return-PC gates. Old outcome facts never authorize that landing.
POST, STEP and YIELD specs remain unavailable in this rung.

Focused draft cases cover bound/unbound restoration with an actual unbound read,
distinct locals, executable noncyclic nested same-local restoration, displaced
iteration-owner reentry, cyclic refusal and cell refusal. The prior completed-
comprehension test remains; genuine lost-origin/ancestor negatives and original
trace tests are unchanged. Host AST/whitespace checks pass. Actual peer and
coordinator native qualification remain pending; this is not a passing-source
or performance claim.

## Saved-NULL target binding guard

Actual source review found that preserving a genuine NULL binding also requires
checking the destination's named-local reads. A branch-proven binding can have
an unchecked local borrow, or a folded physical input, even though the debugger
arrives with the original binding unbound. Scratch liveness alone cannot certify
that destination.

The corrected draft walks the existing request-owned sparse graph once for each
restored NULL local. Every unchecked named-local or physical alias refuses until
a positively guaranteed bound definition. Only exact checked binding readers
such as LOAD_FAST_CHECK and DELETE_FAST admit an unbound binding; a protocol
input accepting NULL does not establish Python binding semantics. Generic
STORE_FAST, physical output writes and declared FORMAT_VALUE outputs do not
prove a non-NULL binding. The shared cold helper discharges only audited semantic
result definitions. This intentionally keeps unknown writes conservative.

Edges carry their actual definition phase: ordinary exception successors use
the pre-operation state, while normal and source-callback error successors use
the selected successful outcome. No new graph or hot check is introduced.
The existing genuine-unbound positive explicitly witnesses LOAD_FAST_CHECK.
New earlier-branch targets cover raw reads, folded arithmetic and attribute
assignment (where accepting NULL would incorrectly delete an attribute), with
refusal preserving the active frame and attribute state. Native validation is
pending; the coordinator owns that gate.

## First native29 release evidence

The release build completed all117 modules. The focused run passed all95
code-object cases, including the saved-NULL checked positive and raw/folded/
attribute refusal witnesses, bound/unbound and nested restoration, plus the
five terminal-observer tests and24 contract tests. This is bounded restoration
and provider evidence, not whole-image qualification. Constructor instrumented
FOR normalization and two synthetic handler fixtures failed in that same gate
and are under independent repair; those failures remain recorded by the
coordinator. No full-suite or performance conclusion follows from this focus.
