# Recovery interface: independent re-review

2026-11-07. **Decision: accept the amended design contract.** This reviews the
uncommitted `pyjit/design/jit-e2e-system.md` amendment against
`recovery-design-review.md` and current runtime `225480a4250`. It does not
approve a native implementation. The whole-method JIT, `JitContext`, native
`JitAction` ABI, shared transfer trampoline, statepoint checker and native
recovery are all planned; the existing register interpreter and perf JIT
trampoline do not provide them.

The four earlier handoffs are now explicit enough to guide a bounded first
slice. `EnterFrame` carries a prepared transaction and forbids detaching the
parent or changing `current_frame` before the shared, once-only commit. The
document correctly says the current overlap and consuming call providers
cannot be invoked unchanged by this new adapter. `Return` and `Yield` transfer
one owned value, name a recipient, and assign failure cleanup to the transfer
routine. Return's unlink-before-clear order and Yield's generator suspension,
send slot, exception link and logical resume state match the obligations in
`Python/bytecodes.c`; Yield keeps the current no-raise rule.

`Raise` now carries a pending raised exception and fault word, leaving raise
monitoring, traceback attribution and handler selection to the shared
unwinder. The fault word, next semantic word and handler entry are separate;
`RERAISE` may replace the fault position. Every exit records an effect prefix
and phase, and post-callback/store/release exits cannot retry the word. The
`FrameState` names physical and virtual homes, NULLs, one owner per count,
borrow roots, error-edge owners and materialization effects. It separates
raised from handled exception state and does not confuse future-read
liveness with frame ownership.

The first recovery slice retains canonical homes at helper/effect boundaries
and prohibits fallible materialization after an irreversible effect. The
two-phase plan preserves old roots during fallible preparation, publishes
observer-visible state before a callback, revalidates frame/code/facts after
it, and commits homes once. Later post-effect allocation is deliberately
conditional on a separately specified OOM route. The offline fixture is
properly described as a contract-shape check, while forced exits from compiled
code remain required to admit native recovery.

**Implementation boundary:** the proposed commit routine still needs a
concrete owner ledger for a private child, parent call-window values and any
displaced homes, with proof that final commit has no fallible or reentrant
step before publication. In particular, an overlapping child cannot be
materialized into parent homes during fallible preparation. A first adapter
may carry only a prepared description until an infallible overlap commit, or
use private copies; either must follow the stated once-only transfer order.
This is an implementation proof obligation, not a reason to add another
general Action layer now. The design makes no claim that it has been met.

No runtime source, build or test was changed or run for this review.
