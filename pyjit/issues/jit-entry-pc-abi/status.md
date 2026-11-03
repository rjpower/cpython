# JIT entry PC by value

**Rejected after the fixed all-29 gate, 2026-11-01.**
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
These were the pre-score requirements. The completed outcome below closes this
gate; no additional target execution is authorized.

The source grant ended13:16:17 with source frozen early; no extension or source
edits remain authorized. Publication/leaf-CFI support and broad functional gates
follow only a useful full-suite result and need separate grants. Main RC and the
old candidate rejection remain unchanged. GC work stays closed.

## Full-suite outcome

The [corrected all-29 campaign](broad-outcome-review-20261101-v2.md) completed:
116 scored processes, 580 values, 58 origins and 348 quiet checks. Supervisor
46516 exited zero, observed 13:50:35 UTC. Root cleanup at 13:50:50 verified
sixteen locks, native idleness, both source/product/ELF bindings and eighteen
unchanged parent fingerprints. Independent review reproduces all 1,158 artifacts.

**Reject progression.** GM29 regresses 2.26% and 1.94%; every aggregate
single-value deletion remains slower. Spectral improves 12.73% and 13.86%,
but 18 workloads lose over 2% in at least one round and 13 repeat the loss.
The [material disposition](broad-disposition-review-20261101-v2.md) and
[complete row evidence](broad-material-losses-20261101-v2.md) retain each loss.
GM28 concentration is diagnostic only; the predeclared GM29 rule rejects.
The [full-suite decision](broad-decision-20261101-v2.json) supersedes permission
to advance from the pilot, without changing its saved scores or narrower result.

The first full-suite run was [incomplete](broad-incomplete-review-20261101-v1.md):
28 origin probes completed and the 29th failed because the inherited wrapper
could not parse the deepcopy variant. No scored process or value existed.
The prior root relative-path launch failure and all partial evidence remain.
A separately reviewed successor forwards setup(variant) and verifies the
reported variant; 14 host controls cover all 58 routes and 29 real-driver API
comparisons. Scored commands, sources, images and acceptance rules stayed exact.

No publication, broad functional qualification, landing or replacement samples
follow this candidate. Source and images remain frozen. Main RC stays accepted.
The next assignment is read-only diagnosis/design of cheap JIT entry admission
for already rejected code; it grants no source, compiler or target execution.
