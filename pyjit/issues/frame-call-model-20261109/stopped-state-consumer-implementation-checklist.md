# Stopped-state consumer implementation checklist

Prepared against clean runtime c1424be11db46b5d9e2aca84bcc96ddad6a7ea62.
Design/checklist only during coordinator timing freeze. The next merged packet
is source publication + explicit-edge terminal JUMP/POP_ITER + this consumer.
YIELD_READY is a separate later gate in the same workstream.

## Exact scope and ownership

- A owns the four audited source-delivery call sites and a bool committed
  publication argument, plus physical-next threading. Only completed Step POST,
  certified terminal publication and completed-predecessor landing publish
  SOURCE_POST. Legacy inline jump delivery remains visible but UNSUPPORTED.
- A changes the single terminal marker to four explicit arguments, updates all
  current providers/host fixtures, and admits no-output JUMP/POP_ITER. POP_ITER
  owns FALLTHROUGH despite public BRANCH_RIGHT; the reported FOR source never
  selects ownership/EH state. H peers the decoder's zero-output/retirement facts.
- B owns Objects/frameobject.c and focused Lib/test/test_code.py cases. H owns
  shared physical ownership/binding facts and constructor witnesses. Root owns
  all generation/build/native runs/commits. No hot frame field or new engine.

## Capture and derive one request-owned view

1. Extend the existing centralized phase capture into RegisterStoppedState;
   remove duplicate phase authorization rather than add a parallel event rule.
   Capture the scoped spec once, validating frame/event/observed association and
   generation. Existing PRE identity behavior remains allocation-free.
2. After existing Analysis construction, PRE borrows its exact pre-word rows.
   SOURCE_POST requires the active source cursor/record, no source-error or
   pending child, exact site==record.from, selected edge and physical next. The
   separately proven normalized landing must equal record.to; do not replace
   physical next or traverse an arbitrary jump chain.
3. For SOURCE_POST allocate one checked save-top/origin row pair, copy the
   actual site's rows and apply the existing selected normal transfer once.
   Positively exclude transfers creating caught/save tokens in this first rung.
   The transfer must not mutate Analysis parent tables while deriving the view.
   Refuse ambiguity; never substitute a global landing merge or fabricated
   producer identity. Keep current site/edge and target graph endpoint distinct.
4. Borrowed/owned row storage is explicit and freed on every refusal, allocation
   error and warning-reproof path. A changed generation discards old outcome
   authority; only the existing separately proved committed PRE landing can
   authorize the newer state.

## Reuse the existing Plan

5. Refactor saved-local action selection to consume the view's context,
   save-top and origin rows. Targets still use global Analysis rows. First
   SOURCE_POST requires inherited context, save-empty target and no required
   target scratch. Existing PRE/caught/FOR adapters keep their prior scope;
   do not call their pre-site survivor slices for a post-edge current state.
6. Use the existing actual-origin/no-reentry witness, ordinary-local kind gate,
   oldest same-local selection, target binding BFS and constant-slot exclusion.
   Shared ReadsHome/DefinesHome/Outcome facts remain authoritative for normal
   transfer; use declared observed-home facts where applicable. Physical fresh
   outputs must never revive an old token. Unknown required ownership refuses.
7. Keep one application transaction: allocations, detach all source/displaced/
   surplus owners, install final locals including genuine NULL, commit PC/
   cursors/generation and invalidate old source recipes, then close C owners.
   No later outer stores after callbacks. Warning callbacks trigger new scoped
   capture and full row/plan derivation, not reuse of the old copied edge state.

## Tests and actual source feasibility

8. Keep existing expected trace/event lists. New focused cases distinguish PRE
   and SOURCE_POST on the same apparent line, actual bound/unbound restoration,
   required raw/folded/attribute refusal, same-PC intent and setter-then-raise,
   and two ingresses sharing a landing with different saved states. An unfinished
   legacy inline source hook must not acquire authority merely by event spelling.
9. Before source admission, use the exact original each-LINE comprehension
   fixture inventory to verify that the non-token-creating restriction admits
   the useful stops. Do not widen that restriction merely to make a count pass.
   The existing ownership28-cancellation-inventory actual multiline fixture has
   no IRSM events: SAVE is pc5, FOR pc10 and body pc11 are real LINE PRE words.
   It therefore neither needs nor proves SOURCE_POST token creation. The six
   one-line each-LINE fixtures differ, and ownership29 still reports list/set/
   dict event2/3/4 errors; those event ordinals alone do not identify the source
   producer. Coordinator-only probe-sourceview-provider-inventory.py compiles
   those unchanged literal sources and annotates each actual source record with
   its from-operation. Its host AST passes; it has not been executed by a worker.
10. Root's first merged native gate runs the focused view/provider/constructor
    cases and original each-LINE witnesses. Classify exact unsupported producers
    before considering token creation, retained roots or yield-ready widening.
    Zero source records or already-PRE stops are evidence to retain the simpler
    path, not justification for inventing a new POST phase.
