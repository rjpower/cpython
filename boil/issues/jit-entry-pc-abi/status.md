# JIT entry PC by value

**Pilot accepted with Richards tradeoff; all-29 elapsed gate held, 2026-11-01.**
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
The [fixed timing plan](elapsed-plan-20261101-v1.md) completed without replacement
samples: 80 values, eight unscored origins and 48 quiet checks. Supervisor45640
exited zero, observed13:15:19; cleanup13:15:56 verified sixteen locks, native
idle, exact source/product/ELF bindings and eighteen unchanged parent fingerprints.
The [independent outcome](elapsed-outcome-review-20261101-v1.md) reproduces all
point and single-value-deletion gates. Spectral norm improves12.65%/10.13%;
GM4 improves2.78%/1.86%. Richards loses5.15%/3.49%, a real retained tradeoff with
unproved cause. The [root decision](decision-20261101-v1.json) accepts only
advancement to broader measurement, not production or integration.

The [reviewed all-29 design](broad-elapsed-design-20261101-v1.md) uses two rounds,
five values per arm:116 scored processes/580 values plus58 origins. Both GM29
and all290 single-value deletions must improve each round; the primary retains
its point/ten-deletion gates. GM28 workload deletions diagnose concentration;
they are not a new veto. Every material regression needs explicit disposition.
Host helpers are held for literal binding and independent review, then a separate
finite root execution grant. No full29 target execution is yet authorized.

The source grant ended13:16:17 with source frozen early; no extension or source
edits remain authorized. Publication/leaf-CFI support and broad functional gates
follow only a useful full-suite result and need separate grants. Main RC and the
old candidate rejection remain unchanged. GC work stays closed.
