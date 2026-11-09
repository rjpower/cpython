# Storage pilot v5 stopped before benchmarks

2026-11-01 05:24 UTC. Root session48525 returned one. The outer inventory gate
stopped before admission, target processes, origin warmups or elapsed samples.
There is no candidate performance result.

The coordinator's activation opened every build lock with append/create mode.
Pinned upstream lacked `.workstack.lock` when its product inventory was sealed;
activation created it, so the correct subsequent inventory comparison rejected
the additional file. Root's review had checked products before this mutation.
This was a host-preflight mistake, not an image change or collector failure.

Root independently recomputed all three inventories: candidate and parent are
exact; upstream differs only by `.workstack.lock`, size zero, SHA256
`e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855`.
Both machine slots, shared serialization and all three build locks are available;
a current `/proc` scan found no repository Python target. There is no run
directory or inner launch/gate receipt. Quiet hold is released.

The complete v5 plan/activation/failure streams remain immutable. An additive
v6 binding may include this exact bookkeeping delta, retaining source, images,
workloads, samples, order and scoring. It requires its own review and fresh
activation; unused v5 time grants no retry. Create required lock paths before
sealing inventories; the corrected root activation must require existing paths.

Exact diagnosis and cleanup: `build/span-elapsed-launch-failure-20261101-v5/receipt.json`.
