# Forward same-caught PRE transfer

Proposal only. No runtime/test changes or native result are attached. Actual
stopped PCs, target requirements and context roots must first come from the
coordinator's capability probe. The earlier
[scope audit](remaining-debugger-scope-audit-after32.md) identifies source-level
questions, not certified physical transfer recipes.

## First useful capability

Allow a forward source jump within one already active caught context when its
existing owners survive unchanged and the target needs no other scratch owner.
The first witness is `test_jump_within_except_block` (relative line5→6): skip
one ordinary statement, preserve the handled exception and saved-previous
chain, then finish its existing cleanup. This does not leave a caught context,
enter another handler, change an exception link or authorize arbitrary forward
jumps.

Use the existing RegisterResumeAnalysis graph, requirements, origins, context
parents, stopped-state factory and RegisterResumePlan. No separate unwind
engine, serialized region table, hot state, replacement exception stack or
per-method rule is proposed.

## Admission facts

Every new permission requires all of these facts:

- The shared factory binds a fresh real-LINE PRE_WORD on the actual observed
  frame, with exact PC and generation. Source/error cursors, child overlap,
  pending redirect, committed POST, yield-ready and suspended states retain
  their existing adapters; none acquires this permission from a LINE label.
- Current and target share the same positive derived caught node and complete
  parent chain. The target is a selected real source boundary. Requesting a
  different lexical arm is neither approval nor refusal by itself.
- Stopped and target saved-binding columns are positively empty in the first
  rung. Restoration actions do not get folded into the first forward witness.
- Every target ALL-demand home belongs to the existing caught-root preserve
  set. Its exact home has the same positive origin in the bound stopped row
  and target row, and its physical value satisfies the existing None/exception
  representation check. Static producer-name equality alone is insufficient.
- A bounded forward survival slice proves that those owners are unaffected
  between current and the first arrival at target. A fresh output or NULL at
  the same home cannot stand in for the required old owner.

N's VALUE/NULL_ONLY matrices remain separate. This capability preserves actual
VALUE roots; it does not broaden N's inherited-only NULL permission or erase
NULL_ONLY demands from the ALL projection. If a target needs both retained
VALUE roots and additional pathwise NULL homes, combining those capabilities
requires a separate explicit Plan admission.

## Forward survival slice

Keep the existing backward caught adapter unchanged. A new bounded forward
adapter can reuse its graph and cut predicates after the shared context/root
checks. It must not just delete `target >= current` or reverse a PC comparison.

1. Traverse the existing sparse graph from current, treating target as a
   terminal landing. Intersect that set with reverse reachability to target.
   If no compatible path reaches target, this first adapter refuses; it does
   not invent a control edge as proof of owner survival.
2. For every vertex before target in this induced slice, require the same
   caught context and reject PUSH_EXC_INFO/POP_EXCEPT, preserved-home reads or
   definitions under the current conservative cut contract, physical
   transports/retirements that affect a preserved home, and child overlap.
   Target itself may read its legitimate owners after landing.
3. Check every retained error/source-error edge's unwind floor against all
   preserved homes. Preserve the ordinary PRE versus completed source-error
   distinction already encoded in the shared graph; do not use success
   definitions to justify an ordinary error path.
4. Reject cycles or repeated context/owner producers in the induced slice.
   A topological worklist or bounded DFS on this same adjacency is sufficient.
   This first proof does not equate repeated static PUSH sites with the same
   dynamic owner generation. PC direction is not a substitute for this check.

Paths which cannot reach the selected target do not establish arrival facts.
The slice is deliberately conservative: an unknown read/transport loses this
new permission rather than manufacturing a root. Any fallback is one of the
existing independently proved recipes, not a blanket forward grant.

## Application and callbacks

The resulting Plan only marks the proved caught homes to preserve; it adds no
exception-link action. Preparation remains fallible and finishes before any
target owner changes. Warning hooks require full recapture/rebinding of phase,
generation, graph facts, root values and Plan. A getter is not redirect intent;
a newer successful setter wins.

Apply the existing transaction: detach surplus owners, publish all admitted
bindings/PC/generation, invalidate abandoned source-error recipes at the actual
commit, then retire detached owners. Do not write old slots or PC afterward.
The target's handled exception stays in its existing restoration chain. A
callback may itself handle LookupError; its `sys.exception()` must remain that
exception throughout the setter, and callback cleanup must restore the target's
unchanged handled context. No direct assignment to `tstate->exc_info` is needed.

## Bounds and first tests

Reuse the request's O(N+E) adjacency and existing checked analysis budget. Extra
reachability/seen/indegree/queue arrays must have checked products and aggregate
size; allocate nonzero backing for zero logical extents. Allocation failure
propagates before mutation. No all-pairs graph search or source-record rescan
per vertex is needed.

Retain the original same-except positive unchanged. Add a minimal ordinary
forward skip with exact output, original exception identity and unchanged saved
previous-owner identity. Run its trace callback inside a distinct LookupError.
Add refusal/state controls for a new caught generation, missing/replaced root,
root-consuming or transport cut, target VALUE demand outside the preserve set,
active save column, cycle/reentry, source/POST phase and warning-hook nested
redirect. On refusal, normal continuation still performs the original cleanup.
Existing backward caught, NULL PRE, loop-root, pending-child and ordinary
monitoring controls must retain their contracts.

The two finally-internal positives are follow-up witnesses, not automatic
members. Their actual RERAISE payload/lasti demands may exceed the saved-previous
origins currently tracked; add a positively derived payload-root role to the
same origin model only if the census requires it. The two unresolved except-arm
cases may lack a normal current→target path even when their active context is
compatible. They need a common-PUSH/owner/binding proof beyond this first slice;
leave their original positives pending rather than impose a lexical-arm ban.

Root owns probe execution, source admission, native checks and qualification.
The Python source-jump contract does not promise every cross-context move, but
that limit is not a reason to refuse proved ordinary same-protocol transfers.

## Capability-probe source peer

Independently read `probe-trace-jump-capabilities36.py` and its existing describe
dependency. Accepted for coordinator diagnostic execution: it delegates each
original tracer call and output assertion once, mirrors decorated first-line
and ancestor matching, records actual callback frame separately from requested
code, and reraises the same ValueError instance. Before/after handled-exception
snapshots occur outside the probe's temporary except suite. Diagnostic event/
request caps are reported separately; callback/compare hooks and tracing are
restored in finally. Descriptions run after the target trace has ended.

Snapshots use bounded exact primitives and object identities without custom
repr. They allocate and read public frame getters; the explicit caveat prevents
treating this as an unperturbed phase oracle or qualification. Host AST/roster
checks pass: 24 distinct original methods, split 11 ordinary, two unresolved and
11 transition cases. No candidate execution occurred in this peer.

## Committed36 witness and bounded implementation shape

Root executed the unchanged originals; the retained source is
`E11/null36-trace-capabilities-v2/stdout.json`. There were no caps and each case
made one setter request. This receipt describes refusal, not owner mutation:
the five caught/finally cases keep their before/after physical PC and handled
exception identity when the setter raises ValueError.

| Case | Actual PRE stop | Relevant target candidates | Concrete distinction |
|---|---:|---:|---|
| within-except | 15 | 19 | PUSH9 establishes saved previous at home1; normal15→19 stays in that context. CALL windows begin at2, above the retained home. |
| finally2 | 16 | normal10, exceptional20 | PUSH15 leaves saved previous1 and live re-raise payload2. Exceptional20 has the active context; normal10 does not. Payload2 is beyond the existing saved-previous origin fact. |
| finally3 | 31 | normal16, exceptional53 | PUSH30 leaves saved previous1 and payload2. Same-context exceptional53 exists, but retains the additional re-raise payload. |
| between arms | 18 | 28 | Both arms derive from PUSH9; normal control after18 exits through POP_EXCEPT22 and never reaches28. |
| between arms with name | 18 | 28 | Same common PUSH, plus the other arm's STORE_FAST27 binding of e and cleanup at33/34. No normal18→28 survival path. |

For within-except the exceptional cleanup of CALL18 goes to25→26→27 and
cannot reach target19. The target-reaching slice is exactly15–19; its old
saved-previous home1 is neither read/defined nor covered by the child window.
This is a useful first increment without a new root kind or exception-link
action. The payload/arm cases remain outside it; no lexical restriction is
introduced.

Implement in N `Objects/frameobject.c` and `Lib/test/test_code.py` only after
independent peer/root admission. Extend the existing caught-slice helper with
an explicit direction argument. For the old backward adapter, retain its
target-dominates-current check. The shared verification then has start=target,
stop=current. For the new forward PRE adapter, start=current, stop=target and
require the admission facts above. Both use the same reverse-to-stop map,
forward worklist, owner/context/error-floor cuts and existing conservative
nonforward-edge refusal. That last refusal is a sufficient bounded cycle cut;
do not claim it accepts every acyclic layout or broaden it in this packet.

H's actual-source peer adds a stricter first-rung condition: every edge in the
new forward induced slice must have phase>=0 and floor<0, meaning a normal
outcome. The old caught helper's floor test alone is not a source/PRE error
phase proof. Reject any target-reaching error/source-error edge in this first
forward permission; retain the old backward adapter's behavior unchanged.

At Plan admission, keep OTHER/POST/suspended callers out and explicitly require
empty stopped and target save columns for the forward direction, even though
the old backward adapter can combine with restoration. Preserve the existing
caught-root collection/shape checks and target ALL-demand subset test. No
source/ordinary exception decoder, parent table or application transaction is
duplicated. Existing backward behavior must be textually and natively checked.

Focused new tests should cover the unchanged within-except output, a distinct
LookupError handled inside the trace callback, and the target's original
ZeroDivisionError identity before and after the jump. Include a missing-root
or different-context target refusal, a saved-binding target refusal and a
warning reproof/nested-redirect control where practical. First root selectors
are these methods plus the original within-except case, existing caught/backward
and NULL PRE controls; finally/arm originals remain useful retained negatives
for this packet's limited authority, not newly ported refusal tests.

## N source checkpoint

After H's direction peer and root admission, the bounded implementation changes
only N `Objects/frameobject.c` and `Lib/test/test_code.py`. The existing caught
slice gets explicit start/stop direction; backward dominance remains intact.
Forward traversal requires normal phase/floor on every induced edge and retains
all old context/root/cycle cuts. Plan additionally requires fresh PRE and empty
stopped/target save columns, with the existing context/origin/ALL checks.

New ConstSlotsTest methods are
`test_settrace_forward_caught_redirect_preserves_exception_owners` and
`test_settrace_forward_caught_refuses_missing_loop_owner`. They cover cold/warm
forward skip with callback-owned LookupError and target exception identity, and
a target loop requiring an absent iterator with unchanged stopped PC/locals/
exception plus correct normal continuation. Existing originals are unchanged.

Host AST and diff checks pass. H's actual two-file source peer accepts the native
gate; root must still inventory the negative target's iterator geometry and
execute release/debug selectors. Source is frozen. No native or qualification
claim is attached to this checkpoint.
