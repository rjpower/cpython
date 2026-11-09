# Python-call fallback boundary: source review

2026-11-07. Reviewed runtime commit `25ce1654801` against the accepted
`calls-proposal.md` Stage 2. **Source review: accept, subject to the
coordinator's build and measurement gates.** I made no runtime edits or
measurements.

The 21-line `Python/ceval.c` diff moves the existing slow provider body into
`Py_NO_INLINE call_transaction_push_python_fallback`. The common wrapper calls
`call_transaction_overlap_python` once, returns its `-1` or `0` result
unchanged, and invokes the fallback only on `1`. The former provider body,
including `small[16]`, allocation, duplicate-before-consume binding,
detachment, reverse cleanup, child publication, parent refresh and decline
return, is textually unchanged. The child/output pointer and transaction
arguments are identical. No source-level ownership or error-path regression
is visible in this split.

This establishes a clear boundary for the common-path stack-frame hypothesis;
it does not prove GCC removed the 248-byte frame/canary, that a decline is
cheaper, or that elapsed workloads improved. Compare the generated common and
decline paths and the coordinator's matched screen before making a performance
claim. The wrapper itself is not forced inline.

The prior release build emits GCC `-Wmaybe-musttail-local-addr` warnings for
generated handlers' `value_owners`. This is a live `_PyValueTemps` box record,
not a dead ownership array to delete as cleanup. Generated handlers pass
`&value_owners` to bridge helpers. `DISPATCH` and `JUMP_TO_LABEL` close a
nonempty record via `finish_value_bridges` before tail dispatch;
`DISPATCH_SAME_OPARG` instead asserts that it is already empty.
`_PyValueTemps_CloseNonempty` decrements its count to zero.
`TAIL_CALL_ARGS` does not pass the record pointer. Thus the warning alone does
not show an escaping pointer, but the source proof depends on every terminal
path using those macros or an equivalent close. Keep the warning visible and
avoid suppressing or deleting the record without a full path audit.
