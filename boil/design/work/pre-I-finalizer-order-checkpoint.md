# Finalizer-order eligibility: bounded as-built result

Evidence root: `/home/power/code/cpython/.worktrees/boil-gc-finalizer-order`.
Relative positive paths below refer to that worktree.

Positive source a285a658b36ec611eb6d135af5769940f9eb85d3, based on
f2a5b3ca33d1. The three-file patch changes only Python/gc_live.c, the
closed-generator graph regression in Lib/test/test_gc.py, and the source
checkpoint document. Shared finalizer eligibility now controls DFS roots,
postorder emission and enqueue. General order_visit traversal remains
unchanged: cleared generators, code and constants still connect active
finalizers. Queue/deallocation/resurrection/weakref/OOM semantics and flags
are unchanged. Root reviewed the exact source before builds.

The regression constructs outer -> cleared generator -> code -> co_consts
-> inner, checks the surviving code edge, outer-before-inner finalizer order,
access to inner's name, and eventual weakref clearing. Reverse postorder
still supplies the ordering argument; eligibility does not prune transit.
This establishes a bounded implementation and regression, not a speedup.

Both owned GCC15 builds completed normally: O3 release and default Og
Py_DEBUG with assertions/symbols, retained frame pointers and eval
-fno-gcse; GIL, non-tail, nonstats, JIT/PGO/LTO/BOLT off. Each reported
67 shared modules, zero failed imports and ten missing optional modules.
Build result IDs: default 99fb1a2d, debug a1094819. Frozen native inventory
IDs: default 519ba383, debug 590dbac3. All target runs used global normal
admission/build locks, fresh task caches, unchanged state/worker/observer/
module/root validators, actual off/kill modes with extras0, and stress disabled
(N0).

| Whole-module job | Actual mode | Reported tests / skips | Collections / fallback / report-only unpinned |
| --- | --- | ---: | ---: |
| test_gc, O3 | off | 142 / 17 | 11292 / 0 / 2121 |
| generators + coroutines + asyncgen, O3 | off | 268 / 0 | 238 / 0 / 0 |
| test_gc, Og | kill | 142 / 2 | 12914 / 0 / 3132 |
| generators + coroutines + asyncgen, Og | kill | 268 / 0 | 251 / 0 / 0 |

All four jobs returned native0/PASS with unchanged complete native
inventories. Protected cases were unskipped: fourteen release-GC, fifteen
debug-GC and ten in each companion run. Whole GC's two common skips require
free-threaded GC and the tail-call interpreter; release additionally skipped
fifteen cases requiring a debug kill build. Each job's skips-summary.json
lists exact names/reasons from JUnit. Reported tests retain upstream's
counting convention; skipped cases are not executed-test credit. Unpinned
is reported across observed interpreter exits; no zero-unpinned claim is
made for whole GC. All observed fallback counters were zero. Module coverage
uses fixed whole-module argv plus the standard loader/unchanged hooks and
validated worker results, not an invented per-passed-case journal.

Relative to build/gc-finalizer-order, actual positive receipts are:

- default-whole-gc-off/result.json: 2dd1f252; cleanup.json: 97536a18.
- release-companions/result.json: ad4d399a.
- debug-whole-gc-kill/result.json: f37eb1ec; cleanup.json: aee7ebab.
- debug-companions-kill/result.json: 05d857cf; cleanup.json: aee7ebab.

Each result binds its raw run.log, JUnit, state/root journals and pre/post
product maps. Final debug cleanup found no exact-runtime processes and a
free build lock. The two debug cleanup paths hold the same post-pair
observation, rather than two independent observations. Release companions
completed before the debug build; no
later release target has run.

The separately managed negative tree boil-gc-finalizer-order-negative is
clean at 025d549368a0fec22a755a7448987df0d27cbd77. It removes only the
reverse-postorder loop, preserving eligibility, traversal and the regression.
Its own O3 build passed (products bb248406). The one exact regression then
returned native2 with exactly one named event-order AssertionError:
inner-before-outer instead of outer-before-inner, with no crash/import error,
no skips, 50 collections, zero fallback/unpinned and unchanged products.
Negative receipt: that tree's build/gc-finalizer-order/
negative-transit-order-off/result.json d70446b2; cleanup 01ca7b89.
This proves the regression rejects broken reversal, not every possible
transit-edge omission. No retries or C-memory fixtures were used.

No broad candidate suite, timing, weakref-classification patch or integration
has run. The original tracked source checkpoint remains historical and the
positive tracked HEAD stays clean a285 for the held matched timing package.
The coordinator may copy this ignored as-built record into primary docs;
parent qualification/known lifecycle debt and actual-union requirements
remain separate.

Post-all-jobs release cleanup observation is additionally retained at
release-companions/cleanup.json 8db84687: no exact release-runtime PIDs and
a free release build lock. Its timestamp is later host evidence, not a
retrospective original launch-time guard.
