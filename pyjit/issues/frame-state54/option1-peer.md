# Option 1 independent peer review

Approve implementation under the proposal's complete descriptor contract and
root's semantic/native gates. Reviewed selected source and proposal; no runtime
execution. Carrying this producer fact is narrower than changing home validity.
The result owner remains live through redirect/retirement and the empty route
removes actual old-home work without changing reference ownership.

The important semantic boundary is redirect completion: it can clear scratch
and invoke finalizers, so reread return_slot after it, not just recompute an
index from a cached descriptor. Local/cell/free destinations always require
replacement because observer callbacks can change their actual owner. First
send and static trampoline destinations need explicit empty descriptors rather
than treating every high-bit-clear raw value as a dynamically proven slot.

Port every raw numeric reader, especially genobject yield-from subtraction,
frameobject same-PC/redirect comparisons and remote asyncio inspection. Discard
remains exactly untagged4095; a malformed tagged discard must not silently index
home4095. Keep encoding assertions and remote validation explicit. A descriptor
is internal frame state and cannot consume bits in the already full IR word.

No unresolved proposal blocker. Native review must include producer boundary
loads and new branch/masks as well as saved publication load/close. Broad wins
remain unproved; a new classification branch can lose to a predictable NULL
old-home branch. Tests must cover reentrant redirect and actual replacement
owners, not just accessor arithmetic.
