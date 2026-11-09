# Cold architecture review at checkpoint 28

2026-11-09. Read-only review of runtime `192cfbe8aa0` plus the frozen,
uncommitted per-local PRE cancellation diff in `Objects/frameobject.c` and
`Lib/test/test_code.py`. This is a source and design review, not native
qualification or a new performance result. I read the current plan, fourth
integration report, terminal-commit and scoped-phase proposals, the actual
generated handlers, frame/call helpers, observer code and cold debugger
analysis. The CPython AI-use policy's minimal-change and review requirements
apply. No runtime source, build or test was changed or run by this reviewer.

## Judgment

The baseline is considerably more coherent than a stack-bytecode port: one
compact executable word domain, canonical owning homes, explicit call result
recipients, source edges in metadata, and request-local debugger proofs.
`call_transaction_overlap_python` transfers a guarded argument window into a
child before releases; `_PyEval_CompleteStep` distinguishes PRE observation
from a committed POST outcome. The present weak point is the boundary between
*producing an owner* and *letting a callback observe it*. Final output placement
is still partly a generator epilogue, while the observer and frame-object
redirection code can run earlier. That split is a correctness issue, not merely
an abstraction preference. Cold analysis is complex but off the dispatch path;
size alone is no reason to replace its proofs with a permissive rule.

## Ranked actions and admission conditions

1. **P0 — Commit observed outputs before callbacks.** In
   `Python/generated_ir_cases.c.h:10936`, INSTRUMENTED_FOR_ITER leaves owned
   `next = item` in C across `INSTRUMENTED_JUMP`, then publishes window[1],
   window[2] and A. The two OR_POP handlers at 11116/11153 likewise store
   `res` after observation. Callback error bypasses those stores; a redirect
   can be overwritten by them. The filtered FOR error probe in the fourth
   report confirms a retained C owner on candidate and upstream. The proposed
   terminal outcome is the right direction: one path-sensitive finalization
   using existing `IRStack` layout/store-mode facts, with all selected outputs
   and continuation committed, C-owned displaced/discarded values detached,
   and no old stores after callback. Admit it only with generator negatives for
   undefined output, duplicate commit, live input, alias, and an observer-error
   path; then verify collectability and redirect semantics natively. Upstream's
   matching leak does not make an unreachable owner acceptable.

2. **P0 — Complete the terminal-provider census before redefining
   `INSTRUMENTED_JUMP`.** A global terminal meaning has a source blocker beyond
   the proposal's POP_ITER example: `Python/bytecodes.c:1813` composes
   `_MONITOR_END_ASYNC_FOR + _END_ASYNC_FOR`; the first uop observes a branch,
   while the second still matches the exception, retires inputs or reraises.
   Terminalizing the first uop without migrating the second changes behavior.
   `INSTRUMENTED_END_SEND` at bytecodes.c:386–413 is a separate direct
   `_Py_call_instrumentation_jump` provider that holds `val` and closes receiver
   after observation; a token replacer for `INSTRUMENTED_JUMP` would miss it.
   Inventory every helper call and compound macro, migrate each whole semantic
   path or fail generation, and compare unrelated generated handlers byte for
   byte. Do not add release-only executable words to make the proof easier.

3. **P0 — Specify the error/redirect transaction through deferred closes.**
   The proposed order must distinguish the semantic site used for traceback
   and the committed continuation used after a callback. Its provisional
   `IR_DEFER_CLOSE(owner)` needs a checked, exactly-once transfer from an
   existing owner; neither a C spelling alias nor `_PyValueTemps` is a generic
   owner record. Close-time finalizers can redirect again. The adapter must
   publish the selected PC before the first potentially reentrant close, then
   return the *actual* PC/generation and cancel only its own stale activation
   source-error recipe. `Python/ceval.c:_PyEval_CompleteStep` already detaches
   its window before POST callbacks, but its error path and owner closes show
   why this rule must be explicit. The exact source-error cancellation and
   pending-exception preservation API is an admission blocker, not a later
   cleanup; tests should include observer error followed by a close-time
   redirect and same-PC intent.

4. **P1 — Publish one scoped, typed observer phase after the provider proof.**
   Today `Python/ceval_macros.h:395` sets `next_instr` around a jump callback;
   `Python/instrumentation.c` scopes `observed_frame` but does not publish a
   provider-certified output/owner phase. The proposed borrowed record should
   bind frame, real site, physical next, semantic outcome, event and entry
   generation for one synchronous callback, restoring the prior record before
   arbitrary callback-result disposal. A later tool must not capture a fresh
   generation that revives an obsolete phase. Mask unsupported nested entries
   and verify activation/thread/frame lifetime before dereferencing the C
   record. Only a provider with no post-redirect writes may advertise
   POST_COMMITTED; OR_POP and FOR body do not qualify until action 1 lands.

5. **P1 — Keep debugger cancellation and saved-local restoration as distinct
   proofs.** The uncommitted frameobject diff now tracks a top per actual
   LOCAL_MOVE local index (`register_resume_contexts`,
   `register_context_normal`). This corrects the failed global-order model:
   distinct locals can restore in either order, while nested saves of the
   same local require youngest-first. `register_resume_plan` still permits
   PRE cancellation only when current and target saved-local columns are
   positively empty. Retain that conservative boundary; derive later
   restoration actions from actual origin homes and commit them with the
   existing detach/commit/close Plan. Do not infer safety from an empty target
   scratch set, a source line, or a global save stack. The malformed-ingress
   and same-local test cases in `Lib/test/test_code.py` are useful controls;
   native results for the corrected diff are coordinator-owned and pending.

6. **P1 — Give call, step and ordinary output completion one ownership
   vocabulary, without a second runtime protocol.**
   `Tools/cases_generator/ir_generator.py:4298–4334` currently performs final
   `IRStack.flush`, optional window-to-A move, displaced-owner release and
   dispatch. `Python/ceval.c:1719–1804` performs a related call-result
   temp/local/NoDst split; `_PyEval_CompleteStep` performs an explicit
   window/output/observer transition. Factor the generator's final-output
   emission for ordinary and terminal paths, as proposed, and document the
   shared conceptual states: C-owned result, published home, detached old
   owner, observed commit, retired owner. Keep call binding's guarded fast
   path and its complete fallback; don't add a generic transaction object to
   the hot path just for terminology. Every exceptional exit must identify
   which state owns each value.

7. **P1 — Prove or retire every scratch owner at ordinary frame end.**
   `Python/frame.c:_PyFrame_ClearExceptCode` clears named locals for a thread
   frame and only debug-asserts its temporaries are NULL; release builds do
   not scan them there. That is sound only if all ordinary return and
   completion paths really leave scratch homes empty. The current design/status
   says semantic owner retirement remains unfinished. Audit actual return,
   yield, call-error and terminal-observer exits against the initialized-home
   invariant, with a bounded owner census and debug assertions. If a live
   scratch owner is possible, repair its producing completion rather than
   relying on future-read liveness or silently weakening the assertion.

8. **P2 — Extend recovery metadata at effect boundaries, then measure current
   native cost.** The compact register IR and `_PyIRFormat` describe physical
   roles and whole-handler effects, but a future JIT statepoint also needs
   before/after/error owner maps for callbacks, publication and detached
   values. Derive those facts from the same semantic completion declarations;
   do not infer them from nearest words or create an opcode-name table.
   Keep baseline physical homes separate from a later typed SSA optimizer.
   The fourth screen's 0.817460 two-pair logging/upstream result is promising,
   but final six-round campaigns, broad regressions and full qualification
   remain open. The stable nbody census counts handlers, not native work.
   After the correctness packet, profile the *current* coherent image before
   changing call/dispatch hot paths again; no speedup follows from the cold
   proposals alone.

The terminal-commit proposal should proceed after actions 2 and 3 have exact
interfaces and tests. The per-local PRE diff remains a separate, frozen
candidate; this review neither admits nor rejects its pending native gate.
