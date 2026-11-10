# Minimal first owning-entry contract

**Keep the first design smaller.** If preparation and commit are proved to contain no callback, interpreter poll or frame/code mutation opportunity, reuse **MISS / ERROR / COMPLETE**. Do not add a general position/generation classifier or REDISPATCH/REDIRECTED outcomes to handle precommit events the admitted contract excludes. This is design calibration, not implementation approval.

## Admission and preparation

Admit only the already proposed empty-prefix, exact positional, NULL-self, ordinary-GIL generator/coroutine shape. Decline observations, origin tracking, unsupported allocation hooks (including arena hooks), and ambiguous executing-entry state **before** acquiring resources. The admission check must establish actual allocator behavior, not trust a name or a mutable public flag.

Audit the finite preparation path, including allocation failure and private-shell cleanup. Scheduling GC is not running GC; that distinction needs source evidence. Exact-type allocation, increfs and field writes can fit the contract, but arbitrary destructor releases, weakref callbacks, custom deallocators, monitoring, tracing or a periodic check cannot. No lock may span allocation merely to manufacture this proof.

If the proof holds under the GIL, another Python action cannot replace the function code, materialize a debugger redirect, or reinstrument the site midway through this interval. Pin function/code as required for ownership and use the selected code consistently for object type, slots, constants and continuation. Debug assertions can check unchanged current frame/site/exec and cleared source homes at commit. They document a proved invariant; they cannot replace a missing release-build admission check.

Outcomes remain simple:

- **MISS:** before transfer, unsupported entry, unchanged owning caller window; existing CALL fallback once.
- **ERROR:** allocation/preparation failed; safely destroy private resources, preserve the primary exception, leave the original owning inputs to the established error recipe.
- **COMPLETE:** complete owning object published, inputs consumed once, no child push, no fallback or replay.

If a real supported callback point is discovered before commit, stop and narrow admission or revise the proof. Do not quietly retain the callback while asserting it cannot happen, and do not automatically build a general reentry framework to preserve first-scope coverage.

## Publication: reuse semantics, not a universal native-call tail

Mandatory extraction of `call_transaction_native`'s whole success tail is unnecessary. That tail owns monitor/callable resources, arbitrary residual window retirement, periodic checks, and failure staging that this route deliberately lacks. A new general resource record and policy flags would broaden the change and burden unrelated native-call behavior.

The closer existing primitive is ordinary exact-return recipient publication. `_PyFrame_PublishReturnSlot` installs the owning result before closing a displaced local; temporary destinations must be NULL. `_PyFrame_SetReturnValue` adds discard handling and consumes pending redirect state. Those are the relevant ownership semantics once the new route has NULLed every input and owns a complete result.

Do not call either blindly: the caller must install the correct recipient and completed continuation, and `_PyFrame_SetReturnValue` currently reports redirect state sampled **before** result retirement. A newly triggered redirect during displaced-local or discard finalization is not covered merely by that return value. Review the exact-return emitter and the active-frame setter contract. Reuse the safe store/discard primitive and a narrow continuation-selection rule; do not import child-return pending-redirect semantics unless their preconditions hold.

Publication is the end of the no-callback interval. A displaced local or discarded coroutine can now finalize, enable instrumentation, or redirect under the supported observer rules. Publish a complete operation before that release, then dispatch the authoritative current continuation; never restore the captured CALL PC or replay the completed call. A local numeric continuation/exec refresh, or an existing redirect mechanism with proved applicability, may suffice. This postcommit obligation does not require five provider outcomes: COMPLETE can mean the operation is complete and its continuation is already selected. Any truly necessary generation check belongs at this actual callback boundary, not in a universal precommit classifier.

`complete_step_impl` is not a drop-in replacement: it implements SEND/FOR windows, receiver retirement, completion branches and observation events. Likewise no native-style periodic poll should be added solely to share code. One ownership/continuation contract can have small specialized consumers without becoming duplicate parameter binding or an alternate frame ABI.

## Required versus excessive

Required: clearable shell fields; no hidden owners across GC; pinned-code allocation; exact once-only argument transfer; recursion/error admission; call-time prologue/observation behavior; canonical result publication; exception preservation; and honoring a legitimately newer continuation after retirement.

Not required: reproducing an incidental CPython reference-release transcript, allocating a retirement buffer for an already-empty window, handling forbidden precommit debugger callbacks, or extracting unrelated native monitoring/periodic machinery before the first entry experiment. Existing native ordering should remain unchanged because that adapter is outside scope, not because every new provider must imitate it.

The next reviewable artifact should be the short admitted-path call graph, shell ownership table, exact-handler integration and the existing primitive's required preconditions. Keep the current eligibility census and measured full creation/call/completion gate. Do not expand entry shapes or transaction outcomes to make the proposal appear general.

This review inspected existing frame publication, exact-return emitter, native-call tail and SEND/FOR completion source. It made no runtime edits and ran no native work.
