# Overlap initialization: artifact review

2026-11-06. **ACCEPT** the reviewed `frame-overlap-init.patch` source artifact
for normal integration; no remaining must-fix source finding. This is static
review, not evidence of application, compilation, execution or speedup.

The helper computes the previous frame's extent before moving stack frontiers,
clamps its already-valid overlap prefix to the child extent, asserts it covers
the transferred arguments, and debug-checks all skipped non-argument homes for
NULL. `_PyFrame_Initialize` zeros only the remaining tail and still fills hidden
constant homes with the normal ownership duplicates. No new bitmap, code
layout, dispatch write or GC rule is introduced.

The proof relies on held-owner occupancy ordering and the established call
window invariant, not future-read liveness alone. Dead-but-owned temporaries
remain below the call window; caller locals and constants also lie below it.
Child arguments keep their original cells, while defaults/empty varargs/fresh
kwargs are installed only after valid storage exists. Current exact-call
producers and the separately reviewed common-call commit obey this contract.
Generator caller and non-overlap initialization remain unchanged.

Tests warm mixed caller/callee extents, reserve extra caller temporaries with
an untaken wide call branch, cover unwind and mortal argument lifetime, and
replace a hidden constant with a mortal object to check ownership retention.
Actual coverage of the skipped and newly initialized ranges still needs the
coordinator's debug execution and generated-code inspection. Tiny memset savings
may not repay the bounds arithmetic; no measured benefit is claimed.

The reusable bounds and initialization-order specification is in
[call-and-frame-lifetimes.md](../../design/call-and-frame-lifetimes.md).
