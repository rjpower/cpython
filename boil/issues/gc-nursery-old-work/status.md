# Reduce old-object work during minor collection

- State: maintained-baseline source/helper accepted; focused native batch running
- Owner: qualification_review_resumed implements; coroutine_design_review reviews; coordinator selects
- Updated: 2026-11-01
- Budget: native batch06:13:43–06:58:43 UTC inclusive, four rows, stop first failure
- Depends on: [minimal nursery](../gc-nursery-minimum/status.md) and
  [coverage/reclamation result](../gc-nursery-coverage/status.md)

## Hypothesis

Page-scoped object-start enumeration can remove allocator free-map reconstruction
from inventory, old seeding and promotion without changing object classification,
tracing or lifecycle work. A complete remembered-owner boundary remains a later
hypothesis; the present profiles do not justify selecting a cell-only barrier.

## Experiment

Review the [object-map inventory proposal](object-map-inventory-proposal-20261101.md)
against actual publication/free/realloc paths, page/domain and abandoned heaps,
huge allocations, raw-memory coverage, OOM and native lifecycle obligations.
An independently compared allocator-set oracle and omitted/stale-start controls
must precede destructive collection. Set a finite prototype budget after review.

## Decision rule

Accept one useful iterator only if complete source/native checks preserve the
object set and observed elapsed benefit exceeds its cost. Reject incomplete
membership, changed lifecycle semantics or an elapsed loss. The 12.14% historical
inventory/classification share is not all removable; the explicit allocator-leaf
example is only 1.15% of total sampled coroutine CPU. No speedup is forecast.

## Now / next

The current selective port is `dc2ec2c059152933e8804f5f997e81370883460f`,
in `build/worktrees/gc-object-map-maintained`, from maintained417. It excludes
old nursery/cadence/ledger changes and rejected spans, preserving the production
classifier. Exact independent [source acceptance](reviews/maintained-object-map-source-review-20261101-v1.md)
permits focused native preparation. Five added methods contain seven actual
child templates. The failed-prefix witness now counts outside allocator
visitation and independently verifies nonempty death prefix, actual remaining
marks/SHALLOW, unconditional cold cleanup, outer rollback and recovery. Those
observations must still pass natively. GCC computed is the fixed profile.
Exact [helpers accepted](reviews/maintained-object-map-helper-review-20261101-v1.md);
root activated and launched session25200 at06:13:43. Debug build/test precedes
release build/test;16 methods per image, one declared debug-only release skip.
No retries, timing or broad qualification are authorized by this native batch.
Held elapsed preparation proceeds in parallel, without target calls.

The earlier isolated prototype froze at `c43c8374dbd56ca7862be49a31820ded6ac5a9d9`.
Independent [source review](reviews/object-map-source-20261101-v1.md) accepted
bounded native feedback preparation. A causal failed-inventory-prefix cleanup
witness is still missing; its proposed source correction has not been granted
or implemented. Host feedback drafts are suspended, no native job has started,
and the [integrated review](../fundamental-runtime-design/status.md) treats this
iterator as a transitional hypothesis rather than a complete collector design.

Source proposal frozen at 00:47 UTC, SHA-256
`1411d4567bc1a85f124b6631a570a15e016b0ae6a19c061ada83957551bbbfe2`.
Independent [design review](reviews/object-map-inventory-design-20261101-v1.md)
accepted the proposal and its explicit route-selection/stale-fixture obligations.
Root granted a separate thirty-minute host-only prototype in one managed
worktree from exact `47ccf35e21a94fde9351d1c2e4571bf207257a4c`.
Native execution and performance remain held. Nursery adoption/cadence costs
are a separate decision; no barrier or collector-policy alternative is added.
