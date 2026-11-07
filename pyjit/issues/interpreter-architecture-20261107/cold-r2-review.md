# Cold R2 review: Python call transfer boundary

**Decision: accept** runtime commit `25ce1654801` against parent
`22f49d676db`. I found no semantic, ownership, frame-state, or error-order
regression in the committed diff. This accepts the source boundary, not a
throughput improvement.

The 21-line `Python/ceval.c` diff moves the consuming body of
`call_transaction_push_python()` unchanged into
`Py_NO_INLINE call_transaction_push_python_fallback()`. The wrapper calls
`call_transaction_overlap_python()` once, returns its `-1` failure or `0`
published-child status directly, and calls the fallback only for its `1`
decline. `call_transaction_invoke()` still initializes `*child = NULL`, runs
method/EX preparation and any CALL monitor event before choosing the Python
path, and calls the native provider only after a Python-path decline. The
`Py_GIL_DISABLED` overlap branch still declines into the same fallback.

The ownership and frame protocol stays in the original bodies: the overlap
path keeps the parent window owning through its fallible keyword-dict setup
and final revalidation, then transfers inputs and publishes the child without
an intervening callback. The consuming path still copies inputs before the
binder can consume them, refreshes after binding, detaches the parent window
only after a viable child exists, publishes the child before retired-owner
release, and restores the parent before clearing a failed child. Splitting the
functions adds no new operation on those edges. A failed overlap returns
without entering either the consuming or native provider; a decline enters
each provider at most once. The overlap cleanup already refreshes the call
record before a decline, and the fallback rereads the current homes.

The retained R2 release ELF (`r2-gcc/candidate-python`, build ID
`adade7880217a84fa00988e51aff4bf04a6a61de`) has distinct local symbols
for overlap (`0x881` bytes) and fallback (`0x917` bytes). In
`call_transaction_run_vector`, GCC calls overlap and branches past fallback
when its result is not `1`; the fallback call occurs only on `1`. Its stack
reservation and canary are in the fallback, while overlap reserves `0x58`
bytes after its first eligibility guards. The older qualified C ELF in
`interpreter-throughput-20261106/final-identities/candidate-python` has one
`call_transaction_push_python` symbol (`0x1275` bytes) and reserves `0xf8`
bytes before those guards. That ELF predates R1, whose exact ELF was not
retained, so this comparison establishes the present code shape and a
plausible reduction in common-path setup, not an R2-only size or speed delta.
Declined calls acquire a second function call in the present ELF.

`r2-identities.json` ties the release and debug images to `25ce1654801`;
both recorded builds returned zero and checked 117 modules. The five focused
test files passed 916 tests with one skip in each profile, according to
`r2-checks.json` and the release/debug logs. I ran no new build, test, or
native workload. The active basic performance screen and later merged/full
checks remain outside this review, and there is no timing conclusion here.

**Bounded follow-up:** finish the already active matched basic screen and
inspect its complete vectors and before/after identities before attributing
any elapsed-time effect. Keep any later empty-varargs simplification as a
separate reviewed increment. No source fix is required for R2.
