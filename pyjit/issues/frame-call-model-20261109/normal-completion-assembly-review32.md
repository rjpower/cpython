# Normal generator completion: compiled source32 review

Read-only preparation for root's location profiles. Current release build ID is
`c22c55552d50db3a77068f1f4ecaae14149c2079`; golden is
`61fbf1bb863cc2c72050228cc7704979a7779589`. No candidate or profiler was launched
by A. Root's accepted counts show extra instructions for generators/coroutines;
the following is a route inventory, not attribution of that excess.

## VM child return with an ordinary SEND parent

Current RETURN_VALUE retires the child with the result C-owned and calls
`_PyEval_CompleteGeneratorReturn`. Its VM-parent path:

1. Saves five callee-saved registers plus its frame pointer and reserves a
   stack area. It decodes the parent's semantic operation with a real call to
   `_PyIR_BaseOpcode` at function-relative+0x60.
2. Calls `_PyCode_InstrOparg` at+0x73, extracts destination/base, and selects
   SEND versus FOR. This is actual compiled work, not just an assertion retained
   in source.
3. Pushes two stack arguments and calls `_PyEval_CompleteStep` at+0xb1. The
   eight-argument helper returns a typed next/status pair. RETURN_VALUE then
   checks status before dispatch.

The helper has its own saved-register/stack prologue. On the assumed normal
unobserved SEND route, an already-inline result skips the object
canonicalization call. A NULL monitoring-data pointer skips observer calls.
The compiled path still selects SEND versus FOR/THROW, detaches the three-home
window, selects/publishes the destination, installs continuation/PC and reaches
the shared owner-close tail. That tail tests exception, sent, cursor, receiver,
displaced and remaining result owners; some are known NULL for a particular
semantic outcome but remain runtime tests in this generic function. A dying
receiver can invoke deallocation, so this is not an allocation-free leaf.

These assumptions must be checked against sampled workload routes: inactive
tools do not necessarily imply monitoring-data pointer NULL, returned values
need not always be inline, and destination/receiver ownership varies.

## Golden's different partition

Golden `_PyEval_GeneratorReturnTarget` has a smaller target-selection job: its
VM route calls `_PyCode_InstrOparg`, computes the completion target and returns.
Generated return publication and subsequently dispatched END_SEND perform the
remaining transfer/receiver retirement. Comparing only that small helper to
current CompleteStep would omit real golden work. Current direct publication
avoids intermediate result movement and preserves explicit PRE/POST semantics;
the removed dispatches and stores must be included in any net ledger.

## Questions for location evidence

- Does excess sampling concentrate in CompleteGeneratorReturn/CompleteStep and
  their caller stack, or in receiver/frame allocation and retirement?
- How often do normal completion routes pay metadata decoder calls, generic
  kind/destination branches and close tests, versus meaningful deallocation?
- Does the instruction excess correspond to this path while common retirement
  work stays similar, or is a separate creation/resume path responsible?

If supported, a shared callback-free normal commit with a cold observer/error
adapter is a structural candidate. It must keep canonical result ownership,
alias-safe detach/publication, correct receiver retirement, callback error
preservation and newer redirects. No inline rewrite, new continuation fields
or handler shortcuts are admitted by this review.
