# Reduce old-object work during minor collection

- State: REJECTED on elapsed; tracing-GC work closed
- Owner: coordinator
- Updated: 2026-11-01
- Budget: closed; no extra source attempts, samples or qualification
- Depends on: [minimal nursery](../gc-nursery-minimum/status.md) and
  [coverage/reclamation result](../gc-nursery-coverage/status.md)

Independent [final elapsed review](reviews/maintained-object-map-elapsed-outcome-20261101-v2.md)
rejects source76fb: coroutine ratios to parent **1.080163 / 1.054832** and
four-row geometric means **1.044114 / 1.041969**. All120 values and twelve
origins were valid; cleanup completed. RSS was diagnostic, not the rejection
rule. Focused correctness acceptance remains valid but earns no adoption.

The coordinator then [selected RC early](../gc-parity-deadline/decision-20261101.md).
All source attempts, failed audits and raw products remain preserved. The
proposal and progress snapshots below explain the rejected experiment; they
do not authorize more GC work.

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
root activated and launched session25200 at06:13:43. It stopped06:15:20 at
the debug build's GC audit, before focused tests/release. Missing `mi_ctz` body
visibility and outlined unannotated `live_walk_block` must be corrected without
weakening the checker. All587 products and idle cleanup independently verified.
[Failure and next scope](maintained-object-map-native-failure-20261101-v1.md).
The corrected source froze at`3a9f70e065bf4112ef3a070746011df7b08892f3`.
Exact [source review](reviews/maintained-object-map-source-review-20261101-v2.md)
and [helper review](reviews/maintained-object-map-helper-review-20261101-v2.md)
accept fresh feedback. The debug wrapper is removed; scalar observation occurs
at the original mark-clear store. Release classifier/callback/type snapshot
remain unchanged, with no new precision boundary or per-death root overhead.
The GNU bit scan uses its direct nonzero builtin; non-GNU fallback stays.

Root separately authorized a fresh45-minute v2 window and launched session85319
at06:30:50, deadline07:15:50 inclusive, within the unchanged hard GC cutoff.
Fresh debug-v2/release-v2 use the same16 methods and one named release skip.
This is a new reviewed source/batch; failed v1 is not resumed or relabeled.
Held elapsed preparation overlaps compilation; candidate image/outcome bindings
remain unavailable until independent native acceptance.

V2 stopped06:34:38 at release GC audit: GCC emitted a direct call from precise
`live_walk_page` to the existing opaque classifier. Debug16 passed, including
the strict witness, with440 collections/fallback0/unpinned0. No release tests or
timing ran. [Independent rejection](reviews/maintained-object-map-native-outcome-20261101-v2.md).
Root independently verified588 debug and587 release products plus idle locks.

The separately [reviewed callback-boundary hypothesis](reviews/maintained-object-map-callback-boundary-design-20261101-v3.md)
changes only the new page callback's annotation to the ordinary allocator
callback convention. Collection, listing, failed-clear and diagnostic modes
are explicit; no checker, effect table or call-hiding attribute changes.
Exact source`76fb129ec28179488fd2ee012a0ae5f599779bc1` and v3 helpers passed
independent review. Root launched fresh session82961 at06:46:35,07:31:35
inclusive ceiling. Same16 methods/one release skip. **Any further precision
edge stops this hypothesis.** Held elapsed preparation overlaps; no performance
or full qualification credit exists.

V3 completed all four rows, root session82961 exit0. Independent
[focused outcome](reviews/maintained-object-map-native-outcome-20261101-v3.md)
accepts debug16 and release15 passes/one exact skip;440/423 observed collections,
zero fallback/unpinned. Both588-product inventories and idle cleanup verify.
Strict failed-prefix assertions actually passed; discarded nested stdout does
not provide exact scalar/PID telemetry.

Exact [elapsed bindings](reviews/maintained-object-map-elapsed-bindings-20261101-v2-root.md)
passed independent review. With all three agents explicitly quiescent, root
activated the fixed pilot07:00:44, session21198. It completed120 values and
twelve origins, terminal0 and all three builds idle; quiet hold released07:02:20.
Initial C/P coroutine ratios1.080163/1.054832 and GM4 ratios1.044114/1.041969
fail the fixed gates. Independent raw/origin/RSS review is pending. No extra
samples, source work or expensive qualification is planned for this candidate.

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
