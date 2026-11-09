# Retained owner bundles for ordinary frame jumps

Proposal only. No runtime edits, test ports or native results are attached.
The next owner capability should extend the existing request-owned stopped
view, sparse graph and Plan rather than add separate finally/loop/arm setters.
Coroutine NULL ingress remains an independent physical-fact packet.

## Concrete gaps

The retained 36 capability census identifies these distinct requirements:

| Case | Actual retained geometry | Missing proof |
| --- | --- | --- |
| nested_finally_2 | PRE16; exceptional target20 shares PUSH15. Normal target10 has a different context. Saved previous home1 and re-raise payload2 survive. | Preserve the existing propagation payload as well as the saved-previous root; choose the compatible actual target. |
| nested_finally_3 | PRE31; exceptional target53 shares PUSH30. Normal target16 does not. Saved previous1 and payload2 survive. | Same owner bundle, including any actual RERAISE lasti input established by the encoded EH route. |
| between_except_blocks, including the named variant | PRE18→target28; common PUSH9. Normal continuation after18 executes POP_EXCEPT22, so the old current→target slice has no path. | Prove a common already-established context/root generation across alternative continuations. Lexical arm names are neither authority nor a refusal reason. Inspect skipped name-binding work separately. |
| executing inner async-for exit with outer ordinary FOR | Original source bodies require the outer iterator to continue after cancelling the inner operation. | Retain the actual outer receiver/cursor generation independently of discarded inner owners. Exact stopped phase and current/target demands still need fresh masks. |

These are source/retained-geometry facts, not a promise that one new rule passes
all methods. No missing exception, iterator or binding may be fabricated.

## One root bundle, one Plan

Introduce a cold derived owner-bundle description in RegisterResumeAnalysis.
It records a positive establishment anchor, owner members with physical homes
and origin tokens, the required handled-context ancestry, and the encoded
member roles. A member can be a required owned value, a paired cursor that is
NULL-or-index, or an unwind-supplied lasti value. It is not a generic bag of
non-NULL cells.

The first descriptors come from existing executable contracts:

- The existing PUSH_EXC_INFO context supplies its saved-previous root. Its
  actual output mapping also retains the current exception payload used by
  finally cleanup. Track that owned payload through exact COPY/SWAP transports
  and POP/retirement effects; do not infer identity from equal exception types.
- An encoded EH ingress supplies its exception/lasti owners at the decoded
  homes. Multiple ingresses merge only with a positive compatible context and
  lineage proof; otherwise the payload origin is ambiguous. A zeroed home after
  unwind is not that payload.
- The existing successful FOR edge establishes the receiver/cursor pair. Reuse
  its actual ABI validation and successful-edge phase proof. A surviving
  iterator object alone does not prove a compatible cursor or active iteration.

Use one explicit origin namespace/table rather than overloading saved-binding
negative tokens or assuming that every positive token is a PUSH PC. Existing
caught-root lookup can become an accessor over that description; saved-local
obligations keep their current per-local ordering and cannot disappear when a
root loses its home. This is request-owned metadata, not a new frame/header
field or serialized compiler region scheme.

## Shared establishment and survival proof

The current forward same-caught adapter proves a current→first-target slice.
Retain that inexpensive case. The next common-anchor proof is:

1. Bind an unchanged fresh real-LINE PRE stopped view. Require matching handled
   context at the selected target; first scope has empty saved-local columns
   at both endpoints. No source cursor, committed POST, active child, yield or
   suspended frame acquires this permission.
2. Collect target ALL demands and the implicit caught restoration/propagation
   roots. Each must be covered by a compatible established bundle or an
   existing independently admitted NULL fact. First source may require all
   target scratch demands to belong to retained bundles and defer mixing NULL
   permission; it must not drop NULL_ONLY from ALL.
3. Prove the positive establishment edge dominates current and the target's
   required origin. For FOR use its successful body edge, not the header's
   receiver comparison. For caught roots use the common active PUSH generation
   and the actual payload lineage. Static producer-PC equality is insufficient.
4. Use the existing forward/reverse adjacency to form the anchor→first-current
   and anchor→first-target slices. Both endpoints are terminal for their walk.
   They can fork into different arms; current need not normally reach target.
   Require compatible context and no consumed/replaced member on either slice.
   Current physical member checks then witness the surviving dynamic instance.
5. Exclude producer reentry and ambiguous joins. Initially reject cycles in
   these induced slices and any target-reaching error/source-error edge unless
   its complete member-preservation transfer is already proved. A second
   execution of the same PUSH/FOR is a new generation, not a matching origin.
6. Member effects use shared ReadsHome, DefinesHome, semantic Outcomes, physical
   transfer and unwind-floor rules. Exact COPY/SWAP can transport origin only
   simultaneously from old rows. Unknown transport loses the capability; a
   replacement VALUE/NULL never repairs an old-origin cut. Child overlap and
   any floor clearing a member cut survival. Target itself may legitimately
   consume its retained members after landing.

This unifies the existing caught/FOR slice mechanics. Do not create one new
graph engine per root kind. The bundle certifier owns producer/ABI specifics;
the graph walker owns generation, survival and endpoint rules; Plan owns the
preserve union and transaction. A producer-specific certificate is acceptable;
a test-name or lexical-arm branch is not.

Named exception bindings remain a separate explicit obligation. If target
needs a binding established only by its skipped matcher/STORE_FAST, either
prove the current binding has the required identity or refuse. Do not assign
the current exception to an arbitrary target name. A target that never reads
that name and overwrites it before cleanup need not be rejected merely because
its source arm used an `as` clause. Existing warning and binding-safety rules
remain in force; the new bundle does not assert extra named bindings.

## Application and limits

The Plan unions proved bundle members with existing implicit context roots.
It adds no exception-link action and invokes no context-manager exit protocol.
It detaches surplus owners, publishes the selected PC/cursors/generation and
any independently admitted local actions, invalidates abandoned source-error
recipes at commit, then closes detached owners. No stale frame stores follow
close-time callbacks. Warning callbacks require fresh phase and full proof;
the last successful redirect wins.

Caught ancestor exits still require a resolver for the target's actual handled
link through callback saved-previous owners. Directly writing tstate->exc_info
would corrupt a callback's own exception. No new bundle permission substitutes
for that resolver. Likewise crossing an active manager exit or cancelling
PY_YIELD needs its own protocol action; retention alone cannot execute it.

## Bounds, ownership and first source order

Reuse the request's O(N+E) sparse adjacency and aggregate checked cold-analysis
budget. Descriptor/member tables are bounded by actual producers/encoded EH
entries. Origin rows extend the existing checked row allocation rather than
adding an unrelated all-pairs matrix. Reachability/dominance/survival work uses
bounded queues/bitsets; cache a root's slices within the request where useful.
If the number of bundles makes repeated walks exceed the admitted budget,
refuse the capability before mutation. No hot dispatch work is added.

First source should add only the caught propagation bundle and shared-anchor
walk, repairing ordinary same-finally payload retention and compatible
same-caught alternative continuations. Keep the existing FOR certificate as a
separate adapter behind the same walker interface; migrate/extend outer-FOR
retention only after actual current/target root masks identify its missing
member. This gives one model and two reviewable source chunks, not a universal
rewrite before a useful positive.

B would own frameobject analysis/Plan and focused test_code; H shared cold
owner/physical helpers only if a missing actual contract requires them; A
independently audits producer/payload retirement and callback/context ownership.
Agree the descriptor/interface before touching each other's files. Root owns
source admission, native execution, integration and qualification.

Keep unchanged original finally/arm/outer-FOR positives. New focused controls
must pin real PCs and members: propagation preserves original exception and
lasti; a callback-owned LookupError survives before/after setter; normal and
exceptional target candidates select only compatible context; missing/replaced
payload, younger caught generation, ancestor-token use, producer reentry,
ambiguous alternative ingress, missing FOR cursor and target binding refuse
without changing PC/owners/context. Normal continuation must still perform the
original cleanup. Include warning and close-time newer redirects.
