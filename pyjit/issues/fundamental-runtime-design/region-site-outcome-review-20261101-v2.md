# Current comparison/branch site: independent outcome review

2026-11-01, coordinator. **Accept current-site attribution for the bounded
region source prototype.** This is neither compiler-admission proof, per-PC
counting, a performance result nor runtime qualification.

V2 tool session **95945** returned zero. The one planned child completed and
verified at **03:17:49.357 UTC**, before its **03:27:38.671 UTC** ceiling.
The existing supervisor's row, verification and idle cleanup all succeeded.
V1's earlier host import failure remains preserved with zero target calls;
v2's independently reviewed pinned import closure was the only repair.

Root independently rehashed all **44 completion artifacts**, replayed the
actual capture contract and matched the retained result, raw dump and all
three before/warmed/after code snapshots. The child PID was **1402450**;
root receipts record **27 collections, zero fallbacks and zero unpinned**.
Unpinned remains a typed report-only field, not a newly imposed zero invariant.
No target was rerun by this review.

The actual kernel's closed source call graph is
`run → bench_coroutines → recursive fibonacci`. Start/end function/global,
code, observer, source, CAPI, configuration and module bindings agree. Across
the three captured functions there is exactly one semantic comparison:

| Current Fibonacci word | Actual state |
| --- | --- |
| 4 | Pristine COMPARE_OP; warmed/after-count COMPARE_OP_INT, arg58, destination1, inputs0/2, feedback1 |
| 5 | Pristine POP_JUMP_IF_FALSE; warmed/after-count POP_JUMP_IF_FALSE_BOOL, input1, forward target8, feedback2 |
| Source context | Both positions line15, columns7–13; nlocalsplus1, so result1 is a temporary |

Before warmup the forms were generic; after the unchanged registered warmup
and counted call the same specialized pair remains. One counted `run(4)`
records **971,140** corresponding pairs, matching **242,785 per loop** in the
independent shared baseline. The warmup also called `run(4)`. Snapshots,
feedback/history and origin inspection occur outside counting. There are no
extra version probes, workload calls or elapsed samples.

The raw dump SHA256 is
`957c9895acbe1dbe523b7b26247ce625c88ba0639e46e4eff641472cfcf25646`.
Exact plan `7936781c70f37846879efe8c02348e12ff2c2d895f82521a1c4c244de5a7a348`,
manifest `094b5e2e16f6a15ca24cd329f35e37b23601b85d4f91af6bd6db9f177969dedd`
and original streams, snapshots, maps, raw bytes, result and completion remain
in `build/region-site-attribution-20261101-v2`.

This closes the current consumer-selection question. The implementation must
still prove virtual single use, implicit/exception liveness, context, physical
aliases and release-mode nomination. It must preserve ordinary branch entry,
generic callbacks/errors, monitoring, both IP publications and breaker/root
contracts. Complete emitted paths and fixed elapsed/RSS decide the mechanism;
global pair activity alone cannot accept it.
