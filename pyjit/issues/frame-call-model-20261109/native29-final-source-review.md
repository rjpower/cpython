# Native29 final frozen source review

Disposition: approve the reviewed bounded source for coordinator native gates.
No concrete must-fix remains in this pass. This is an independent review of the
current uncommitted runtime diff against4a1f70fcef49, including the applied
instrumented-FOR and synthetic-hole corrections, not a restatement of earlier
proposal approval. No native process, build, regeneration or test was run by
this reviewer. Runtime and test source remained unchanged during this pass.

## Saved-local restoration and binding safety

`RegisterResumePlan` carries explicit ordinary-local restoration actions. Every
active same-local chain member needs its surviving origin and a no-reentry
witness; distinct local columns commute. Cells, missing tokens, ambiguous joins,
cyclic producers and targets with active saves remain refused. Oldest-binding
selection is separate from scratch availability; equal values or NULL contents
are not generation witnesses.

The selected actual NULL binding is checked against the existing sparse graph.
The new edge phase distinguishes ordinary PRE errors from successful normal and
source-callback POST edges. `_PyIR_BindingRead` checks all named and physical
aliases. Only exact checked binding operations accept an unbound local; ABI
nullable inputs do not. In particular, attribute/global deletion capability is
not evidence that an unbound Python name can be consumed. Generic STORE_FAST,
physical writes and FORMAT_VALUE do not discharge the obligation. The narrow
semantic result definitions have a positive value contract. Checked reads leave
the obligation active, and ordinary error edges bypass same-word success defs.

The final action application allocates before mutation, detaches all selected
sources and displaced/surplus owners, installs final bindings including genuine
NULL, then commits PC/cursors/generation before any closes. Restored NULL is
excluded from None fill rather than fabricated. Later callbacks cannot be
followed by old local stores. Actual ordinary/identity commits invalidate old
source-error recipes before closes; pending-child requests defer invalidation
until the frame-owned completion commit. The overflow and warning-reproof gates
remain intact.

Focused source tests preserve the existing unbound positive and explicitly
witness LOAD_FAST_CHECK. New branch-proven raw, folded and attribute targets
must refuse without changing the stopped frame or attribute sentinel. Nested
same-local, distinct-local, cyclic, cell and close-time redirect cases remain.
The coordinator's first release29 focus passed all95 code-object tests; this
supports the bounded source review without qualifying broader phases.

## Committed observer ownership and source recipes

The callback scope is activation-owned and initialized before publication.
Unknown hooks mask outer authority. Scope/event/observed-frame state restores
before arbitrary callback-result closes. The setter still admits its bounded
PRE recipes; the presence of POST/YIELD enum values alone does not authorize
those phases.

Terminal output publication precedes observation. Deferred displaced/ignored
owners are held in C and drained after the chosen continuation is published.
Typed status selects callback-error cleanup; the raised exception is saved
across closes and the latest frame PC supplies the error adapter. There are no
old destination stores after callbacks. The generator's dotted-owner checks,
deferred early-exit rejection and path-specific output checks retain the
concrete source-proof obligations found during review. Ordinary providers do
not acquire terminal behavior from event-name heuristics.

The constructor retains one selected-home worklist, separate old-origin cuts
and physical missing/value/NULL facts, checked home geometry, and PRE versus
POST seeds. New replacement values never revive old required owners. The actual
source-handler test now independently proves that the fresh FOR item is valid
for a committed source error but unavailable for an ordinary PRE error using
the same encoded handler read and preserved-prefix floor. Cursor replacement
and required discarded-item negatives remain. OR_POP positive/negative cases
continue to distinguish a fresh defined NULL from an old consumed input.

`_PyIR_Outcome` now uses the generated semantic HandlerBase table. The checked
metadata maps INSTRUMENTED_FOR_ITER236 to FOR_ITER65, so valid instrumented
completion geometry shares the actual semantic recipe. Public opcode deopt/view
behavior is unchanged. Near/far geometry tests retain actual relaxation adapters
and invalid temporary-A rejection. The hole fixtures now give the selected
B..B+2 trio untouched middle positions: DICT_MERGE requires B-4/B-1/B+4 and
SWAP requires B-1/B+3. Their extra outer loop supplies the verified scratch
prefix; required-owner mutants remain. These synthetic handlers are constructor
proof witnesses, not execution/type-validity claims.

## Build and fixture changes

Makefile.pre.in adds the existing exception, observer, outcomes and source IR
headers to the common PYTHON_HEADERS dependency set. All four paths exist;
bootstrap/library/embed consumers already depend on that set. This adds proper
incremental rebuild dependencies without a generated target or interpreter
cycle.

The generated metadata snapshot port adds only NONE/NONE outcome fields to
synthetic LOAD/IR_PASS expectations; prior roles, flags, effects and twin checks
remain. The sys_settrace port changes21 obsolete projection-message assertions
to the anchored register-owner diagnostic. ValueError and existing expected
execution/state lists remain asserted; no unsupported positive becomes a
passing refusal expectation.

Host AST parsing of the changed Python tests and `git diff --check` pass.
Merged release/debug validation remains coordinator-owned. Known unsupported
SOURCE_POST/yield-ready/caught-exit and broader retained-root capabilities are
not waived by this source disposition; whole-model behavior, remaining release
words and full qualification remain open.

## Executed evidence addendum

Read the coordinator's actual ownership29-final-debug-focus.log: all six modules
pass,363 tests with1 skip, including code95, completion26, terminal5 and
contract24. The22 release targeted diagnostic/constructor methods pass in
ownership29-final-minimal-release.log. The owner-exit receipt reports zero live
weak references after explicit collection across ordinary, loop-return,
exception, comprehension, generator and escaped-frame cases. That owner probe
labels its image as the first native29 build with constructor repairs pending;
it is runtime ownership evidence, not proof that the final constructor snapshot
was independently reexecuted by that receipt. Wider original-module results
and basic measurements remain coordinator work; this addendum makes no global
qualification or prompt-finalization claim.
