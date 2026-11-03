# Maintained object-map first native failure

2026-11-01 06:16 UTC. **Stop: debug build failed its GC inline audit.**
Session25200 returned1. Source dc2ec2 remains the failed source; no focused
test ran, no release build started, and no timing or qualification credit exists.

Configure succeeded. `make all` returned2 because precise `live_map_area`
inlined `mi_ctz` without a C body the auditor could inspect, and precise
`live_fault_block` called outlined `live_walk_block` without a precise
annotation. These are actual gate failures. The first reports missing effect
visibility, not an observed collection. The second requires checking and
annotating the actual callee closure, rather than exempting it.

Root independently verified587 failed-build products, the raw log, failed
result and post-gate idle receipt. Both machine slots, global serialization
and build lock were available; no target Python remained. Failure and cleanup
finished06:15:20, before the grant deadline. All v1 evidence stays untouched.

Evidence: `build/object-map-native-failure-review-20261101-v1/receipt.json` and
`build/worktrees/gc-object-map-maintained/build/object-map-maintained-feedback-v1`.

One bounded source correction is authorized through06:30/checkpoint06:22:
make bit-scan effects visible using existing portable conventions and annotate
the actual block visitor/callee closure truthfully. Preserve classifier body,
collector policy and checker strength; no opaque exemption. Independent exact
source review, fresh versioned builds and separately reviewed helpers precede
another native attempt. No automatic retry or deadline extension.
