# JIT entry PC by value

**Source frozen and reviewed; native feedback held, 2026-11-01.**
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
qualification reviews its final binding independently. Compiler, native and
timing execution require later finite grants. Register recovery is unproved.

Gates: source review; the existing 54 focused methods in fresh release/debug
images; actual ordinary-handler PC and unchanged native-leaf evidence; then a
separately frozen two-round/five-value comparison against RC on spectral_norm,
coroutines, Richards and unpack_sequence. Preserve the same primary/aggregate
and single-value-deletion rules. No old values are replaced or averaged in.
Broad qualification follows only a useful elapsed result. GC work stays closed.
