# Cheap JIT entry admission

**Separate reviewed source attempt active, 2026-11-01 14:05:30 UTC.**
Main remains the accepted RC runtime. Both earlier native candidates are rejected;
[full-suite evidence](../jit-entry-pc-abi/broad-decision-20261101-v2.json) remains frozen.

The [design](design.md) and [peer review](design-review-20261101-v1.md) permit
one cheap inline admission predicate and cold immutable rejection seeds. Full
TryEntry, dynamic guards, direct CAPI behavior, leaf, decoder, owner and transactional
commit remain authoritative. Generator flags are entry exclusions, distinct from
decoder exclusions. Numeric state values and the success mask remain exact.

The [source grant](source-grant-20261101-v1.json) expires 14:35:30 UTC;
checkpoint 14:13:30. Worktree `build/worktrees/jit-entry-admission`,
base `599897b6b5d`. No compiler, target, runtime probe, timing or landing is granted.
The source owner implements; an independent reviewer checks the final diff.

After source acceptance: focused release/debug54 and actual admission/PC/leaf
inspection, then directly one fixed all29 comparison. Avoid repeating the four-row
pilot. Keep the same mean/sensitivity/regression rules. Expensive publication
and full functional gates follow only a useful broader result. No optional
collector/JIT selector or arbitrary text/RSS veto is introduced. GC stays closed.
