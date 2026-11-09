# Exact committed POST view: implementation brief

Design checkpoint during the native31 full diagnostic freeze. No runtime
changes or native jobs are authorized by this document. The shared transfer
interface still needs final source agreement. This packet addresses the actual
monitoring conditional callback in ownership31-remaining-phases.json; NULL-flow
requirements for peepholer remain a separate change.

## Scope and witness

The existing failure is a BRANCH_LEFT callback at actual
INSTRUMENTED_POP_JUMP_IF_FALSE pc4, after its condition has been consumed, with
the committed physical continuation pc5. The callback requests the next return
line. There are no source-map records. Treating this as a PRE word requires an
input which the provider has already retired.

Use the existing stopped-state capture and Plan. Add a materialized committed
view for positively declared normal edges, alongside the borrowed neutral-JUMP
case. First permission requires inherited handled context, positively empty
saved-local columns at both the stopped state and target, and a safe target
with no incoming temporary-owner requirement. PRE restoration and the prior
neutral-JUMP/strict adapters retain their existing behavior. Nonneutral POST
does not gain a pre-site FOR/caught survivor proof or saved-local restoration.

SOURCE_POST, YIELD_READY, pending children, actual suspended frames and caught
exits are outside this packet. Public event spelling is not phase authority.

## Capture, then bind one view

Keep `register_stopped_state()` as the central phase factory. Its captured
descriptor can retain a copied `_PyIRObserverSpec` for a declared committed
edge. Validate the original provider intent before accepting this capture:

- exact target/observed frame and callback event;
- POST_COMMITTED, current generation, no pending redirect/child or active
  source-display/error cursor;
- bounded, aligned actual site equal to instr_ptr/current;
- return_pc equal to the scoped physical next;
- literal normal edge and the corresponding actual encoded successor equal
  to that next;
- positive generated observer outcome, valid shared `_PyIR_ObservedHomes`
  geometry and no frame-entry/suspension transition.

The reported branch offset never substitutes for the owner site. Do not reject
a provider merely because an earlier part of its handler may poll or call
Python; the certified callback is after the terminal commit. Existing provider
declarations and exact physical decoding establish this boundary.

After `register_resume_analyze()`, bind the captured phase once, before the
candidate-target loop. A small `register_stopped_state_bind()` helper receives
the analysis and fills the view's context/save/origin pointers. PRE and neutral
JUMP borrow exact site rows as today. For nonneutral POST:

1. Validate the captured association/generation/PCs again. Reject an unreachable
   or ambiguous site row.
2. Reject token-creating transfers before invoking `register_context_normal()`:
   PUSH_EXC_INFO and actual LOCAL_MOVE save creation currently update Analysis
   parent tables. There is no copying-and-restoring of those tables.
3. Allocate one integer block for `nsave_columns + nt`, using checked size_t
   addition and multiplication. Allocate at least one element for a zero-length
   logical row, so row partitioning never performs arithmetic on NULL. Copy
   only the logical exact-site save-top and origin extents.
   Compute context as a scalar copied from the site.
4. Apply `register_context_normal(code, site, edge, ...)` once to those copied
   rows. With token creators excluded, its only mutable row arguments are the
   copies; parent tables remain read-only.
5. Decode the selected committed physical homes. For every `old_cut` home,
   clear its copied origin regardless of a replacement VALUE or NULL at the
   same home. Physical publication never revives a consumed saved/caught token.
   MISSING cannot satisfy a target value requirement. No physical replacement
   is a substitute for dynamic provenance.
6. Require resulting context zero and every save column positively zero for
   the new cancellation path. Keep ambiguous/lost obligations active/refused.

Add the owned block to request-local `RegisterResumeAnalysis`, freed by
`register_resume_analysis_clear()`. `RegisterStoppedState` only borrows its
row pointers. Allocation is once per analysis, not once per target candidate.
This avoids copied owning structs and new cleanup branches throughout the
setter. OOM returns -1 with MemoryError; invalid authority/geometry returns a
refused view without changing frame state.

## Plan and revalidation

`register_resume_plan()` must stop unconditionally replacing every phase row
with `analysis[current]`. PRE/neutral views bind that row; nonneutral POST binds
its request-owned transferred row. The target remains the existing global
analysis target row. The actual stopped row is never replaced by the merged
landing row.

The new nonneutral permission is only the inherited/empty-save/safe-target
case above. Otherwise it refuses; it must not fall through into an adapter
whose survivor proof starts before the consumed operation. Existing PRE and
neutral strict fallback remain unchanged. The Plan continues to own preserve
bits/restoration actions, and application keeps its current detach/install/
PC-generation commit/invalidate-source-errors/close order.

Before warning callbacks, only remember the selected kind and logical current
coordinate as comparison data. Analysis-owned view rows cannot survive
`register_resume_analysis_clear()`. If preparation calls Python and no newer
redirect supersedes the request, recapture the live scope, build a fresh
analysis, bind a fresh view and repeat the whole Plan. Compare the expected
kind/current coordinate, then use the fresh rows and actual handlers. A source
target deliberately selected OTHER cannot upgrade during this reproof.

Generation changes discard the old edge. A fresh PRE landing may be recognized
only through the existing independently validated real-LINE landing protocol;
the old POST rows or successor never authorize it.

## Negative controls and first gate

Preserve the original monitoring test. Add bounded cold/warm controls for:

- consumed conditional input cannot supply a required target value;
- a replacement owner or defined NULL in an aliased home cannot recreate its
  old saved/caught origin;
- active/lost save columns, ambiguous context, token-creating transfers and
  stale generation remain refused without changing locals or continuation;
- warning-time nested same-PC redirect wins, while getter-only preparation
  causes a full successful reproof;
- original neutral JUMP, PRE restoration, same-caught and FOR strict positives
  retain their existing path.

No synthetic release word, event or setter opcode whitelist is added. Root's
first native gate is the original monitoring case plus these focused controls
and the existing stopped-state/restoration methods. H and A review the actual
shared decoder/capture/row transfer and cleanup before source freeze. B owns
Objects/frameobject.c and focused test_code cases; A owns provider scope/error
contracts; H owns shared physical/old-origin helpers. Root owns all native
execution, regeneration and integration.
