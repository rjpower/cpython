# Cancellation and named-local restoration

Revised after the first source28 native gate: the global restoration TOP model
failed a valid distinct-local positive. Actual normal code restores left then
right while its EH path restores right then left. Root and A admitted the
per-distinct-local correction below. Corrected release/debug focused gates now
passed124 tests with1 skip each, but a subsequent original-module run found a
running-line nontermination regression. The bounded real-LINE identity
correction now passes125 focused tests with1 skip and all capped original probe
cases per profile. The obsolete capacity-test port awaits its narrow native gate
before integration; full debugger qualification remains open.
The failed draft and fixture receipts remain evidence. Further restoration/POST
actions are still proposals, not implementation admission.

This builds on source26's request-owned Analysis/Plan, sparse graph and one
application transaction. It does not restore a frontend value stack.

## Why the current test is too strong

`register_resume_recipe` requires both current and target incoming sets to be
empty, or to fit a checked preserved pair. At a cancellable real LINE before the
operation, its operands need not survive if the operation is deliberately
abandoned. A target with no scratch requirements can often accept a plan that
retires those current operands. Requiring current emptiness confuses availability
for executing the cancelled operation with availability for entering the target.

That relaxation alone is unsound. The actual comprehension census has
LOAD_FAST_AND_CLEAR moving outer `marker` local1 to scratch4, SWAP placing its
saved state in scratch3, and normal/exception restore paths moving it back.
Jumping to a return with an empty scratch input set must restore that named local;
closing scratch3 would lose the outer binding. Current liveness incidentally
protects this obligation. Removing that protection requires a positive protocol
proof, not an exception for comprehension test names.

## Actual boundary phase

Use one cold `RegisterResumePhase` description: frame, real producer/current PC,
normal successor/edge when relevant, and PRE_OPERATION, POST_EDGE, PENDING_CHILD
or SUSPENDED kind. The first implementation recognizes ordinary real LINE before
its handler using the existing observed-frame, wrapper, source/error-cursor and
child gates. No new hot frame state is needed for that case.

Source LINE is post-from and uses the actual source record/outcome. Ordinary
branch callbacks are also post-operation: the monitoring failure at
`if s.startswith('t')` stops after the predicate was consumed, while current
liveness still asks for that predicate. Generated instrumented POP_JUMP_IF_FALSE
consumes and closes the input before invoking the branch callback. Its LEFT
callback is fallthrough, RIGHT is taken; SEND completion LEFT has a different
semantic outcome. Do not classify all branches by one event bit.

For the later POST rung, the existing observed wrappers already know exact
`site`, `next` and semantic outcome. Prefer publishing a scoped cold phase record
around their callback, saved/restored through reentry, over reconstructing a
phase from adjacent PCs. A proposed record must carry no Python ownership and
must not add ordinary dispatch work. It also needs an audit that held C owners
are isolated from cleared frame homes and that redirect paths close them without
reading stale borrowed operands. Yield-before-suspension and already-suspended
frames remain separate contracts.

## Derive named-local obligations from final IR

Extend the existing origin analysis, with a distinct SAVED_LOCAL token kind:
producer PC, actual LOCAL_MOVE local index, and parent save for that SAME local.
LOAD_FAST_AND_CLEAR is currently emitted by
`codegen_push_inlined_comprehension_name`; its actual output mapping supplies
the saved home. A token describes saved binding state, including genuine NULL,
not necessarily a non-NULL owned object.

Maintain one active top per DISTINCT saved local. Independent named locals can
restore in either order, including different normal/EH orders. Nested saves of
the SAME encoded local must restore youngest first. Derive compact columns from
actual LOCAL_MOVE indices and check N×column allocation products; the state is
not an inferred frontend stack.

COPY duplicates a value origin with a distinct owned edge; SWAP transports it.
Ordinary fresh definitions, actual semantic retirements and unknown uses lose
the fact. STORE_FAST ends an obligation only when its actual input carries the
youngest active token for its actual LOCAL_OUT index. A copied same-local
ancestor cannot skip a younger save. Losing every known origin leaves that
local's obligation active but unavailable; it never empties the column. There
is no inference from equal names, source lines, stack depth or a later STORE_FAST
alone. Unknown transports or inconsistent per-column joins refuse the request.
Use the same normal, PRE-EH and POST-source-error graph as source26.

Handled-exception context remains separate from this per-local vector. Their
requirements compose in the Plan. A target may retain each local's current top
or an ancestor of that local's top. Entering a new save cannot fabricate an outer
binding. A repeated active producer or ambiguous parent is unavailable. Bounds
follow actual save nodes/request memory, not CO_MAXBLOCKS: one comprehension
can save many locals even though block nesting is small.

This proof is derived on each request from actual words. Code.replace does not
retain trusted compiler flags or a serialized restoration recipe. The first
analysis rung may positively establish that both phases have no active saved
locals; it must refuse an unknown context rather than treat unknown as empty.

## One cancellation/restoration Plan

The Plan keeps its preserve bitmap and adds explicit restore-local actions.
Targets still require every incoming nonconstant owner to be independently
available: preserved dynamic FOR/other anchors, caught-context roots, invariant
NULL under actual-reader capability, or a proved action. Current expression
operands are not target requirements. Same-caught saved-previous roots remain
implicit protocol requirements independently of raw target reads, including
RAISE_VARARGS0 targets; an empty operand set never permits dropping them.

For each exited save in each local's parent chain, identify its surviving actual
token home and original local. Plan the named-local restoration. When nested scopes save
the same local, coalesce to the outermost exited binding at the target; intermediate
values are retired without publishing transient local states. Retain saved roots
of contexts still active at the target. No target exception-link write is part
of this rung; differing caught contexts remain refused.

Compute warnings against the planned local state, not the inner comprehension
binding. In particular a genuine saved NULL is not replaced silently with an
unrelated object. The existing debugger policy of warning/binding None for an
unbound target local must be explicit in the Plan and tested after restoration.
Cell locals restore the saved cell object, not the inner cell's contents; the
initial implementation may refuse unsupported cell proofs rather than improvise.

Application stays atomic with respect to callbacks:

1. Finish all fallible analysis/allocation and warning hooks; reprove unchanged
   generation and actual phase/owners afterward.
2. Detach all selected saved homes and displaced named-local/surplus scratch
   owners into C-owned storage without callbacks.
3. Install the final target local bindings and preserved homes; commit target PC,
   cursors and redirect generation.
4. Close detached owners with no later writes by the old transaction. A closing
   callback's newer redirect wins, even when it selects the same PC.

## Existing with/loop policy

Existing tests explicitly skip the exited with body's __exit__ call when a
jump leaves that scope: `test_jump_out_of_with_block_within_for_block` expects
`[1,3,5,6]`, without `-3`. `test_jump_out_of_with_block_within_with_block` expects
`[1,2,3,5,-2,6]`: the inner exit is skipped, the retained outer exit runs normally.
A debugger transfer must not synthesize __exit__ calls while retiring holders.
Acquiring a new with scope still requires executing its real setup operations;
a target inside an existing outer scope must preserve its actual exit roots.

A loop exit may retire the iterator/cursor while retaining the last loop-variable
binding. It does not restore a shadowed outer local; inlined comprehensions do.
The existing dynamic FOR recipe remains the admission path for targets needing
that live pair. A target-only empty-input cancellation must never authorize
entering a missing loop/with scope.

## Bounded next implementation and evidence

First rung: phase factory for ordinary PRE real LINE, derived saved-local
obligations, and cancellation only when the target has no incoming scratch needs
and both current/target save contexts are positively empty. Keep caught contexts
unchanged and preserve every independently checked saved-previous root even
when the target has no explicit reads. Preserve current gates for child, source POST, suspension and unknown
protocol roots. This repairs the underlying disposable-input distinction without
dropping comprehension restoration. Add an active-save negative and completed-
comprehension positive so the exclusion is a phase fact, not a whole-function ban.

Second rung, using the same records/application path: actual saved-local exit
restoration to a componentwise ancestor target. Add prebound/unbound and nested same-name
comprehensions; verify outside handlers/finally see restored bindings, and that
all bindings/PC are committed before displaced-value finalizers reenter. Keep
original trace/comprehension tests unchanged. Forward same-caught moves and POST
observation need their own survival/phase proofs, not a numeric-order switch.

Source26's 65 remaining trace errors group by fixture as 21 each-LINE
comprehension/unpack/call subcases, nine other comprehension cases, sixteen with
cases, eight caught/finally cases, five loop-owner cases, three ordinary expression
cases, two async-for cases and one yield case. These are investigation groups,
not predictions that one change fixes every member. The other 21 trace failures
are refusal diagnostics and one is the separate LINE epoch repair. Monitoring
branch, three peepholer jumps and Pdb expression jumps add concrete phase/current-
surplus witnesses; actual encoded inventories must identify their target roots
before claiming repair.

B owns frameobject analysis/Plan and focused context tests. Shared cold role
helpers are extended only for a demonstrated contract and used by all existing
consumers. Any scoped observer-phase publication is coordinated with A's
instrumentation ownership before edits. Root owns native probes, gates and
matched performance. No old tests or debugger transcripts are weakened.

## Historical first PRE source28 freeze (failed global-order hypothesis)

Root admitted the first rung and A reviewed that draft before native28. Both
builds passed, but both focused runs failed the valid two-local positive: the
global TOP assumption was too strong. The per-local correction was then reviewed
and admitted. Changes remain confined to frameobject analysis/planning and
focused test_code methods; no compiler restore order is changed.

The existing origin worklist now carries a second active-context coordinate for
saved locals. Positive origins remain caught previous-exception labels; negative
origins label saved local state, including NULL. Ordinary unknown reads,
definitions and unwind clear origins without emptying the saved context. Only a
matching TOP token restored to its original local pops that context. Conflicting
parents, repeated active producers and joins remain unavailable. No restoration
action is executed by a debugger request in this rung.

One `RegisterResumePhase` factory recognizes actual PRE real LINE. Source,
child, error and other observations do not acquire that permission. The selected
phase survives warning reproof; source-only destination candidates explicitly
retain OTHER. With positively empty save contexts, an inherited target with no
scratch requirements can abandon current operands. Same-caught plans still
preserve their implicit previous-exception roots and retain existing survival
checks. Application remains the shared detach/commit/close transaction.

New test methods cover mortal discarded-callee retirement with nested redirect,
active-save refusal and completed-comprehension cancellation in the same code
(prebound and genuinely unbound outer locals), lost-origin refusal, and a
malformed unexecuted ingress whose ancestor token tries to skip TOP. Existing
behavior assertions were retained. Host AST and whitespace checks pass; native
results remain pending and coordinator-owned.

## Admitted correction

The existing worklist now carries N×distinct-saved-local active tops, while
physical origin tokens remain shared with caught-root analysis. Each SAVE
producer's parent comes only from its own local column. Both normal and error
transfers join every column independently. The original distinct-local positive
control and refusal for its genuinely missing restoration remain; a separate
nested SAME-local ancestor-token negative tests the actual ordering rule. Losing
origins still cannot turn active state into empty.

The corrected builds passed117 modules each, and the previously failed
distinct-local positive passed the release focus. Its new nested same-local
negative then failed before analysis because the fixture replaced code while
retaining a source-map container by identity. Supplying that same object
explicitly also failed: the constructor's opt-in is a distinct matching map.
Both harness failures are retained. The fixture now passes an equal copied
container, asserting both content equality and distinct identity; its mutation
changes only an input home, not source/control/EH geometry. The constructor and
runtime C remain unchanged.

The distinct-map fixture retry passed both profiles. The corrected focused gate
then passed124 tests with1 skip per profile, after both builds passed117 modules.
The wider original-module inventory then exposed running-line nontermination,
so these focused results are insufficient for integration. Saved-local
restoration actions, POST and caught exits are not implemented by this gate.

## Running-line identity correction

The coordinator's capped unchanged-fixture probe ran16 rows per profile. Only
`with_after_call`, cold and warm, hit the80-event bound: the real exit word at
pc15/line3 repeatedly redirected to pc8 on the same line, reacquiring the manager.
Other rows completed with the expected identity result. The prior identity gate
required current PC to be the first word selected for a line; that condition
misclassified later real LINE observations as requests to replay earlier work.

The admitted correction removes only that first-word requirement. At an actual
observed real INSTRUMENTED_LINE whose physical source line equals the resolved
requested line, identity means preserving current PC, owners and context. All
source/error/pending/suspension exclusions and generation overflow checks remain.
Source-only same-PC replay remains a separate phase and cannot use this shortcut.
The new cold/warm bounded test checks successful identity at two distinct header
PCs and exact call/enter/body/exit counts, without changing the original fixture.
Post-correction builds pass117 modules and focused125 tests with1 skip per
profile. The capped original eight-case cold/warm probe now finishes all16 rows
per profile with correct results and no errors. The original four-module
inventory completes but still fails known debugger capabilities/diagnostics.
The obsolete projection-capacity test port has A's actual peer and awaits its
narrow native gate; source remains frozen. Full qualification is open.
