# Cheap JIT entry admission

**Rejected by the fixed all-29 gate, 2026-11-01.** Main remains the accepted RC
runtime. Source `11aefbdd` and its images stay isolated and frozen. All earlier
native candidates remain rejected.

## Decision

[Reject progression](broad-decision-20261101-v1.json). The 29-workload geometric
mean regresses **0.52% and 0.45%**. Every aggregate single-value deletion remains
slower in both rounds. Spectral norm improves 5.98% and 4.45%, including its
sensitivity checks. Ten workloads lose over 2% in at least one round; five repeat
the loss. The [independent raw review](broad-outcome-review-20261101-v1.md),
[complete row evidence](broad-material-losses-20261101-v1.md) and
[material disposition](broad-disposition-review-20261101-v1.md) retain each loss.
No cause or noise exception is asserted. GM28 and resource figures are diagnostic.

The [fixed timing grant](broad-feedback-grant-20261101-v1.json) completed 116
scored processes, 580 values, 58 origins and 348 quiet checks. Session 89400 exited
zero, observed 14:52:36 UTC. Cleanup at 14:52:49 verified 18 locks, native idleness,
exact source/product/ELF bindings and 24 historical fingerprints. Independent
review verified all 1,165 artifacts. No value was replaced.

No publication, broad functional qualification, landing or replacement samples
follow. The frame-entry filter experiments are closed. GC work stays closed.

## What passed

The [reviewed design](design.md) added one necessary-only inline state/PC check
and cold immutable entry exclusions. Full entry validation, dynamic guards,
decoder, leaf, threshold, owner and transactional commit stayed authoritative.
No field, runtime selector or release counter was added. Generator flags were
entry exclusions, distinct from decoder exclusions.

Source froze clean at `11aefbdd6405953a0bf3605b3a9f703c01285bf6`, parent `599897`,
within its original 30-minute cap. Seven paths changed. Six generators, seven
products and 26 host generator tests passed. Other generated products and the
executing-code layout stayed exact. The initial host harness failure and
[prefreeze corrections](source-disposition-20261101-v1.json) remain recorded.
[Independent source review](source-review-20261101-v1.md).

The [native batch](native-feedback-grant-20261101-v1.json) passed 54 methods in
each fresh release/debug profile, across eight modules with zero skips. It exited
zero, observed 14:32:13; root cleanup at 14:32:27 verified 17 locks, native
idleness, historical images unchanged and the owned cache removed.
[Independent outcome review](native-outcome-review-20261101-v1.md).

The [emission review](emission-review-20261101-v1.md) verifies all 14 negative
branches bypass the full call, inspected ordinary PC paths retain r15/rbx, and
the native leaves remain 94/234 bytes with six pressure spills. Focused behavior
and mechanism passed; useful full-suite elapsed did not. No whole-CFG, causal,
publication or production claim follows.

## Next work

The [selected IR block design](../jit-quickened-block/status.md) uses existing
cold specialization and explicit fallback, ownership, monitoring and safepoint
contracts. Its exact source and cache boundaries remain under review. No source,
compiler, probe or target execution is granted. The accepted RC runtime and all
measured source/images/results remain unchanged.
