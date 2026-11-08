# Exact due-major trigger correction

**ACCEPT** frozen `47ccf35e21a94fde9351d1c2e4571bf207257a4c` for separately
reviewed remaining feedback. Its parent and reused compiled source are
`5cc1e5310e2cb5f11feade09195b5a3695188161`. No runtime change, rebuild requirement,
native pass, qualification or performance acceptance follows.

The capsule in the nursery worktree at
`build/nursery-cadence-trigger-correction/capsule.json` is SHA256
`9fbd18d1d8edad17ca3720e3dd5d13d0d9d7e002e1a2e95b8f2484cc9f751028`;
its patch is `d89f68734913ab20ae7a5dc021c13947cbd4a260002b5288dc606505c4971fa5`.
Independent host receipt
`build/nursery-adoption-review/cadence-trigger-correction-source-20261031-v1.json`
is `6a721d422b60cc9c79c0bbab854aa2f27eb843ad788003c54c400f058e749532`.

The worktree is clean. The entire git diff changes only `Lib/test/test_gc.py`.
Replaying the exact trigger/comment replacement and its matching final result
assertion reconstructs the complete current test file. Every major/minor count,
all-major event, due-debt, old-cycle, rooted-cell payload and retained-block
assertion remains byte-identical. Both every=0 and every=1 variants remain.
The [design review](cadence-trigger-correction-design-20261031-v1.md) establishes
why this fork's empty-list constructor has one accounted object allocation and
no list freelist bypass; it does not predict an executed pass.

All 22 source-derived assembled child strings host-compile. Only children6/7
changed; all other case IDs, variant labels and body digests match the earlier
inventory, including nominated fault children1/18/11. The independent replay
rehashed all 588 compiled products and checked resolved paths/symlink identities
against the pinned pre-cleanup index. They remain unchanged. No target or
compiler ran during this review.

The original every=1 failure remains failed. A remaining helper may reuse the
5cc image only through this explicit test-only bridge. Existing negative arms
still need their own unchanged fault-source/product bindings, full test-file
difference and exact nominated-child comparison before activation. Preserve
the original 00:14:59.201344 UTC batch deadline and cleanup reserve; no repeated
build solely for this test delta and no automatic retry are authorized here.
