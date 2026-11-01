# JIT entry PC by value

**Focused native checks accepted; separate fixed timing held, 2026-11-01.**
The [30-minute grant](source-grant-20261101-v1.json), 12:46:17–13:16:17 UTC, authorizes a separate
ABI hypothesis in `build/worktrees/jit-entry-pc-abi`, based on `67f5d22bb38`.
The [previous increment remains rejected](../jit-first-native-tier/decision-20261101-v1.json).
Its source, images and scores stay frozen. Main continues to run RC.

The [static finding](static-finding.md) shows that taking `&next_instr`
coincides with a stack-resident interpreter PC in ordinary opcode handlers.
The parent retains the PC in a register. This can affect a loop with few Python
entries; it does not establish the measured regression's cause.

The [reviewed design](design.md) passes the PC by value. The entry returns NULL
on every decline and a validated continuation only after the existing commit
and success record. Callers update the PC only for a non-NULL result. Keep the
same native region, leaf, allocator, owner, threshold, guards and observer order.

Source froze at `599897b6b5d`, parent `67f5d22`, at 12:51:20. The
[independent source review](source-review-20261101-v1.md) accepts all seven
paths and rollback semantics. All 26 generator tests pass; only fourteen hook
expansions change, and the other six generated products are exact. The native
leaf and owner remain unchanged. Restart prepares the held feedback controller;
qualification accepted its final binding independently. The
[finite feedback grant](native-feedback-grant-20261101-v1.json) runs four fresh
build/focus rows, 54 methods per profile, 13:01:38–14:01:38 UTC; supervisor 78640.
It exited zero, observed 13:05:55. Both profiles passed all 54 methods with zero
skips. Cleanup at 13:06:07 verified fifteen locks available, native executables
idle, eighteen reference fingerprints unchanged and the owned cache removed.
The [independent outcome](native-outcome-review-20261101-v1.md) accepts focused
behavior. The [offline comparison](emission-review-20261101-v1.md) confirms
register-based PC handling on inspected load/store/range/tuple/list paths and
all fourteen value-passing entry calls. Both native leaf captures remain exact.
This does not establish whole-CFG behavior, causation or useful elapsed.
The [fixed timing plan](elapsed-plan-20261101-v1.md) remains held for exact
image/source binding, independent helper review and a separate root grant.

Gates: source review; the existing 54 focused methods in fresh release/debug
images; actual ordinary-handler PC and unchanged native-leaf evidence; then a
separately frozen two-round/five-value comparison against RC on spectral_norm,
coroutines, Richards and unpack_sequence. Preserve the same primary/aggregate
and single-value-deletion rules. No old values are replaced or averaged in.
Broad qualification follows only a useful elapsed result. GC work stays closed.
