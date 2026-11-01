# Reduce old-object work during minor collection

- State: review
- Owner: restart_audit proposes; qualification_review_resumed reviews; coordinator selects
- Updated: 2026-11-01
- Budget: source design scope 00:37–01:02 UTC, frozen early at 00:47; no implementation grant
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

Source proposal frozen at 00:47 UTC, SHA-256
`1411d4567bc1a85f124b6631a570a15e016b0ae6a19c061ada83957551bbbfe2`.
Independent review is active. Nursery adoption/cadence costs remain a separate
decision. No iterator implementation, barrier or measurement is running here.
