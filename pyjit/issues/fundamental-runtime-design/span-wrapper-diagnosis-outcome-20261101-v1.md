# Wrapper lifetime observations

2026-11-01, coordinator. **Original cause inconclusive; original release case
remains failed.** Session58683 returned zero; both existing-image diagnostic
children completed and cleanup verified idle before the activated deadline.
Each executed the frozen method once with a temporary Python observer and
worker-thread call graph. Neither reproduced the original assertion failure.

Both observed the weakref dead after callback unwind and thread exit, with
the observer restored. Debug recorded17 collections and1,748 conservative
hits; release recorded16 and118. Both reported zero fallback, unpinned and
dropped hits. Neither identified the original failing object's address, so
its conservative root cannot be correlated.

The changed call graph limits the result. It establishes neither stale-register
causation nor a pass for the original release cohort, and supplies no timing
readiness. Original source, failure logs and products remain untouched.

Evidence: `build/worktrees/gc-integrated-spans/build/span-wrapper-diagnosis-v1`;
interpretation and receipt: `build/span-wrapper-outcome-v1`. Receipt SHA256
`1f9ee76582f553e267c15e94b79f930bf35b79ade394c47f4e23ad581224184d`.
The independently reviewed next fixture separates actual public-free and
published-sweep forwarding from the lifetime assertion; it needs its own
source review and fresh native outcome.
