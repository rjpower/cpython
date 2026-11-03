# Independent design review: retirement oracle correction

2026-10-31, 21:38 UTC. **ACCEPT the bounded test-only design for a separately
authorized implementation.** This is not an implementation grant or source
acceptance. Inputs are the coordinator's proposed isolated one-entry metadata
fixture, the reviewed two-case observations, and the unchanged
`_PyGC_TraceForgetObject()` at source
`b34931a881b33b47a2b2f4b1b2a644c50fed963b`.

The proposed scratch fixture is useful and minimal. The actual function reads
only `interp->gc.trace`, `diagnostic_q_nonempty`, and the trace state's q/nq;
it compares addresses and moves diagnostic metadata without reading object
headers, allocating or consulting interpreter lifecycle state. A zeroed
scratch layout can therefore call the same compiled function without
constructing an interpreter or exposing a fake object to GC. Use bounded raw
heap storage if the interpreter structure is too large for a C stack local.
Existing private trace test helpers already exercise isolated metadata paths.

Keep the fixture's contract explicit:

- With one sorted address-only record and hint 1, an absent-address request
  preserves the count, hint and original record/provenance.
- Removing the exact address leaves count 0 and hint 0. A subsequent empty
  request is harmless. Return/check observed values rather than a hard-coded
  success result; the existing live retirement function must perform the work.
- Scratch state is never installed in the real interpreter, thread state or
  live ledger. Do not call interpreter lifecycle APIs, mark objects, change
  ages, publish roots, collect or alter the allocator protocol. Release raw
  scratch storage on every exit.

This is a metadata invariant unit control, not an actual B-reachability or
allocator-free witness. The real per-target free/resize cases continue to
cover those paths. They must require an initial target B record, prove its
original provenance disappears, and preserve the unrelated records observed
around the operation. A later new-origin record at a reused address is not
the retired allocation. Replacing those conditions with whole-ledger emptiness
would recreate the source-oracle error.

The design restores the missing last-record hint transition without clearing
ambient diagnostics, adding live-ledger reset machinery or refactoring the
collector. No runtime retirement-hook change is justified by the current
observations. The resize prefix changed its failure location, so the original
failure remains preserved rather than being relabeled a pass.

The new internal helper, declaration/CAPI exposure and fixture require exact
source review and a fresh debug build. The current sealed image cannot test
a helper it does not contain. Keep counts, observer bindings and the remaining
finite feedback window explicit when preparing the subsequent native plan.
No implementation, native command or source edit was performed for this
design review.
