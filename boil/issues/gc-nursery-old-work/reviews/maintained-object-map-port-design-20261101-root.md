# Maintained object-map port design review

2026-11-01 05:44 UTC. Coordinator **accepts one bounded source attempt** under
the [design](../maintained-object-map-port-design-20261101.md), SHA256
`6a7982abc29c79f2f50c2d5c48550330b091a808a11e9acf64dedacdf47f0a7b`.
The four applicable production hunks and existing independent c43 review make
a selective port credible. This accepts neither new source nor execution.

Keep maintained417/e9's classifier and allocation/lifecycle/cadence policy
unchanged. Port the page/map iterator, route split and cold allocator-based
clear, with explicit baseline shims. Do not import47's fault selector, nursery
adoption, ledger or rejected spans. Preserve their frozen worktrees/evidence.

The actual failed-prefix witness is the critical condition. Strengthen the
proposal to require a **nonempty real death-record prefix**, positive actual
cleared marks and positive remaining MARKED/SHALLOW before abort. After the
unchanged cold clear and outer rollback, independent allocator callbacks must
observe zero residual flags and a discarded prefix with no death dispatch.
The trigger belongs to the real production page walk, is consumed before
cleanup and cannot recursively fault the recovery walk. Do not manufacture
header flags, count an unavailable observation as zero or retry missing evidence.

The fixed fixture may interleave rooted and deliberately unrooted empty-list
allocations to supply both live marks and dead records across bounded pages.
Explicitly untrack the opaque-stack roots before collection, restore valid
tracking on every prefix, preserve returned results through packaging and keep
the forced manual collector call inside an exported cold core boundary. The
debug-only context must restore on error/reentrancy and compile out of release
hot paths. Existing publication/set-equality negatives remain required.

One new managed worktree from417 preserves the prior c43 and span attempts.
Source scope ends06:16 UTC, checkpoint06:00; no compiler or target. Fresh exact
source review and native feedback precede timing. Use the selected compiler
profile once its independent control concludes; do not create a permanent
collector/backend alternative. Every later grant respects15:14:44 UTC.
