# Nonmoving generations: independent direction review

2026-10-29. Reviewed draft632dd1d01ad0db65827a90e2baf34ad1603887528f1e53da5fec11cfea0c65d3,
7-nonmoving-generations-design.md, against note11 §11 and actual f2a5b3ca33d
collector/allocator/root source. The reviewer did not author the proposal.
No source edit, compiler, native/test/profiling execution or new instrumentation.

Verdict: **DESIGN_DIRECTION_CLEAR; IMPLEMENTATION_NOT_READY**. The proposal
is sound as a gated direction and honestly identifies missing interfaces;
it is not a complete minor-collector algorithm or a grant to enable minors.
Keep its first deliverable the closed owner/store/constructor/domain ledger.
No concrete reason was found to replace note11 owner remembering with cards
or a target-only retention set. Generational benefit is unmeasured: current
candidate/death counts are repeated visits/deaths, not allocation-age data.

Required proof interfaces before implementation activation:

1. **Promotion evidence and commit epoch.** The correct rule keeps unsafe
   SHALLOW survivors young, but live_walk_block currently clears SHALLOW and
   MARKED while listing the heap (gc_trace.c2150); live_mark can upgrade a
   conservative SHALLOW hit after a precise visit (1882). Specify where each
   object's safe-traversal/leaf evidence survives until promotion commits.
   Aggregate nshallow is insufficient. A leaf proof must include existing
   type/shape/dict edges, not merely a NULL tp_traverse.
2. **Remembered-set clearing across reentry.** Specify a no-Python metadata
   commit point before gc stop callbacks and before collect_pass runs queued
   Python. Current collect invokes stop at its tail (gc_live1609), then
   collect_pass runs the queue (1636). Clearing remembered state after either
   can erase newly barriered mutations or expose stale age state to nested
   collection. Preserve state on failed mark/oracle/death preparation; state
   updates and subsequent mutation must have an explicit epoch/order contract.
   This is a small collector interface obligation, not another general audit.

The remaining declared prerequisites are necessary and appropriately scoped:

- **Owner/shared storage closure.** The ledger must identify the logical
  object whose traversal exposes every strong edge, including external arrays,
  shared keys/buffers, type/shape data and extension fields. Barriering only
  one new view cannot cover aliases. Weak referents/caches must not become
  strong remembered roots. Root spans/queues are enumerated storage, distinct
  from old heap mutation; opaque C frames do not exempt stores. The stated
  GIL/domain restriction is necessary and makes no free-threaded claim.
- **Construction, address reuse and relocation.** Reserve side metadata
  without a Python-allocation barrier; initialize/clear it before CREATE
  callbacks and before address reuse. new_reference currently sets the object
  map before CREATE (object.c2694). Realloc success/failure must transfer or
  retain both age and remembered identity with all aliases/roots reconciled.
  Constructors that survive/promote before later stores need real barriers;
  SHALLOW survival is not proof every existing constructor is safe to traverse.
- **Actual domain enumeration.** initialized&~old can avoid old object walks
  only with correct pinned-domain segment membership and block/debug-offset
  reconstruction, including exited heaps. Current live_walk_heaps enumerates
  whole object heaps. The draft correctly names this missing interface and
  does not claim an age predicate alone removes enumeration cost.
- **Omission oracle.** Independently trace full young reachability with age/
  remembering ignored, before reclaiming anything, with separate mark/death
  state and the same root/safepoint context. OOM/incomplete verification cannot
  pass. Scrub incidental conservative roots in omitted-barrier controls;
  missing old-owner stores must fail a named young-reachability assertion.
  Allowed retention by minors is not proof against omitted live young objects.
- **Lifecycle/major policy.** Reuse weakref/finalizer/destroy/resurrection/order
  semantics and owner queue routing for young candidates; preserve transit
  graphs and once-only flags. Majors must ignore age/remembering as roots and
  rediscover old death normally. A separate finite allocation-since-major
  budget is needed even with zero promotion; promoted bytes alone can leave
  old garbage indefinitely under allocation. Numeric cadence/headroom and
  observable gc API/stress/freeze semantics remain explicit activation gates.

The first omitted-barrier cases, running-old-generator/root handoff tests,
shared/bulk storage, SHALLOW upgrade, reuse/resize and failed transactions
are relevant controls. The full store/constructor inventory is substantial
already-declared work, not a new blanket grant. Keep production minors off
until all compiled mutators/ports are covered; do not invent an always-dirty
unknown-type compatibility layer. Preserve inherited HTTP/queue-progress
correctness debt and final GC/performance blockers. One coherent source
proposal plus the named proof interfaces is the next step, not a nursery
sweep over today's unbarriered tree or an uncalibrated repeat campaign.
