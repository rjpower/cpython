# Independent completion challenge review

**GO for the common-completion design lane; no implementation or ledger removal approved yet.** Reviewed the author's [completion proposal](C2-completion-review.md), useful48 return/setter/refresh code, and the descendant redirect classifier. Nonfinal retirement is sufficient, not necessary. Normal reference counting handles aliases if each detached owner is closed exactly once; changed finalizer timing alone is not an obstruction under the project's lifetime policy.

The proposed abstraction is a completed caller scene followed by retirement, shared by ordinary return and elision. It should replace the reason for the ledger, not merely make its table smaller. Selected leaf CALL handlers and certified feedback units remain independently useful design directions; neither supplies this completion proof.

## Concrete gap in the current implementation

`_RETURN_VALUE` unlinks and clears the child while the result remains C-owned; generated `return_to_caller` subsequently calls `_PyFrame_SetReturnValue`. That setter may run callbacks while closing an old LOCAL or discarded result. Neither it nor `value_operation_refresh` provides the required primitive: the latter deliberately resets `instr_ptr` to the old operation, which would overwrite a committed NEXT scene or a callback redirect.

`frame_lineno_set_impl` treats an observed descendant as a pending source CALL before later phase analysis. A finalizer callback is a descendant too. Publishing NEXT can therefore reject an otherwise valid completed ancestor when NEXT is nonCALL, or misclassify an unexecuted next discard CALL as pending. Existing observer-context lookup requires the observed frame itself to match, so it does not supply the missing ancestor capability unchanged.

PC equality is insufficient. A preceding call's saved return PC can already equal the CALL currently executing, and its return slot can still be discard. An INTERPRETER entry shim proves native reentry but also occurs during unfinished native calls. Combining these existing observations still does not prove completed ownership. A new global invariant that normalizes all operation-entry state could make a scene derivable, but that is broader work and must be specified, not assumed.

## Minimum common boundary

Before first close: acquire the canonical result, finish fallible admission, detach the full window and any displaced LOCAL into actual C owners, NULL original homes, install the result or retain its discard owner, and publish the real PRE-next caller continuation. Invalidate obsolete source-error/observer state as part of that commit. Every public home must already match the completed scene; no publication step may close an owner halfway through preparing it.

Retirement needs explicit authority that this caller has no outstanding source-call window. A scoped activation/thread completion context identifying caller and phase is a plausible small representation; it need not be a frame flag, object bank or second transaction record. The caller's current homes/PC remain authoritative. The implementation proposal must define lookup across nested finalizer tracing and how a successful setter supersedes its landing/generation. A stale scope must never authorize the old landing again. Other representations are acceptable if they prove this same distinction.

After commit, close only detached owners, clearing each local owner before its close. Result/argument/function aliases need no identity table. GC and nested callbacks must see strong C owners and canonical caller homes. A callback may replace the published result via f_locals, redirect the caller, enable observation or change providers; cleanup must not write old homes or restore a captured result/PC afterward. Drain remaining detached owners on every error route.

After first release, selected callee function/code/region/proof/counter pointers may be dead. Record counters before release and make all such borrows dead. The active caller's independently owned frame code anchors its own execution; refresh through that owner, reload current slots/handler/actual continuation, and dispatch through the existing observation mechanism. Do not run a completed-CALL error path that restores its source PC. Error attribution/source state and unsupported redirects need explicit shared outcomes, not an assertion that the old refresh helper happens to work.

Ordinary child overlap prevents blindly publishing into caller storage before child clearing. The shared publisher may initially run after ordinary child retirement and before elided detached-window retirement; it can still share old-LOCAL/discard publication and redirect semantics. This does not require identical cleanup ordering or an immediate migration of all ordinary child cleanup callbacks. Preserve already supported ordinary behavior while defining the common boundary.

## Required proof and stop condition

The author's witnesses are appropriate: final argument/self/function decrements; weakrefs and cyclic GC; result aliases; LOCAL/discard; f_locals replacement; trace/monitor installation; next nonCALL and next discard CALL; ancestor f_lineno redirect; nested redirects that invalidate an earlier generation. Demonstrate exactly-once next effects and no post-callback result overwrite. Inspect actual generated terminal/owner flow and the shared ordinary consumer, not only tests. No malformed body or fabricated child is needed.

Until one authoritative scene and its observer/redirect refresh are concrete, retain the bounded nonfinal fallback. If the common boundary requires duplicated publisher/evaluator logic or an ad hoc persistent owner bank, stop and simplify the design. This review establishes no implementation validation or performance result. No runtime edits, native jobs, builds or commits were performed.
