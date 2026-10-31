## Summary

Reviewed `5f74a991472..1a17992b09b`, all C and test hunks, and the requested records.  
Reproduced a callback-delivery stall on the existing `1a17992b09b` release binary.  
Full gates were not rerun during this read-only review.

## Blocking findings

1. **Locally queued calls do not publish readiness to other threads.**  
   **Where:** [gc_live.c:1327](/tmp/boilreview/Python/gc_live.c:1327), [gc_live.c:1345](/tmp/boilreview/Python/gc_live.c:1345); note 13 §8 invariant 6.

   The single-member route returns without setting `queue_ready`; the multi-member route sets it only for another interpreter. An inline collection therefore schedules only the collecting thread. If that thread blocks before handling its breaker, another thread of the owner cannot discover the queue.

   **Evidence:** Using `gc_collect_inline_then(lock.acquire, True)`, the head release binary left 110 queued calls with `ready=0`. A second thread acquired the GIL and called `check_signals_loop(1)`: lengths remained `[110, 110]`, with zero callbacks delivered. All 50 application callbacks ran when the original thread resumed.

   **Fix:** Maintain readiness for every open, nonempty queue, including local and single-member splices. Add this two-thread regression with automatic collection disabled on the receiving thread.

2. **The new closed-state guard causes a NULL dereference on late collection entry.**  
   **Where:** [gc_live.c:155](/tmp/boilreview/Python/gc_live.c:155), [gc_live.c:1570](/tmp/boilreview/Python/gc_live.c:1570), [gc_live.c:2102](/tmp/boilreview/Python/gc_live.c:2102); work record “What changes,” commit 2.

   `get_live()` now returns NULL for a closed interpreter. Although `collect()` handles that, `collect_pass()` subsequently dereferences `get_live(...)->no_python`. Consequently, `_PyGC_CollectNoFail()` after close crashes instead of doing nothing as promised. Other explicit collection entry points share this path.

   **Fix:** Reject closed interpreters at the common collection entry before reaching `collect_pass()`, and handle an unavailable live state consistently. Add a test that invokes collection after close and verifies no allocation, delivery, or crash.

3. **The recorded gates do not cover the landing head.**  
   **Where:** [7a-1.md:269](/tmp/boilreview/boil/design/work/7a-1.md:269), “Gates and the verification set”; PROCESS §3’s rerun rule.

   The record attributes the builds to `ae73621efff`. Later, `ca8ceef0a13` changes test 9.12, and `1a17992b09b` merges integration changes across 145 files, including getset signatures, memory/module object code, traversal, and tests. The narrow rework checks are recorded, but no gates cover the combined code at the reviewed head. Changing the ASan default also does not establish a passing rerun of the failed ASan gate 0.

   **Fix:** Rebuild and rerun the affected gate ladder and verification set on the combined head after fixes. Record each command, build, tested commit, result, and retained exclusion. State explicitly why any older line remains applicable.

4. **The routing and allocation-failure tests cannot establish their advertised results.**  
   **Where:** [test_gc.py:3083](/tmp/boilreview/Lib/test/test_gc.py:3083), [test_gc.py:3097](/tmp/boilreview/Lib/test/test_gc.py:3097), [test_gc.py:3260](/tmp/boilreview/Lib/test/test_gc.py:3260); note 13 §§9.11 and 9.17.

   Both tests record `me` captured when objects were created. A callback executed on the wrong interpreter can therefore print the expected owner ID. The “every kind” test also collects both owners’ garbage from main before the supposed reverse-direction collection. The allocation-failure sweep covers only positions 1–12, although its nine application objects require eighteen callback/finalizer nodes.

   **Fix:** Read the current interpreter inside each callback or hook. Exercise each direction separately and assert that foreign calls remain pending until their owner runs. Control collection during setup, cover every intended allocation position, and assert that each injected failure actually fired.

5. **The pin test does not test routing through the deletion race.**  
   **Where:** [test_gc.py:3213](/tmp/boilreview/Lib/test/test_gc.py:3213), [gc_live.c:1402](/tmp/boilreview/Python/gc_live.c:1402); note 13 §9.16 and the as-built deviation at [7a-1.md:233](/tmp/boilreview/boil/design/work/7a-1.md:233).

   The test creates plain cycles, leaves `ran` unused, and asserts only that a pause observed a waiter. It neither ensures a nonempty batch naming the closing interpreter nor verifies delivery. Moving unpinning before routing could therefore escape this oracle.

   **Fix:** Arrange call-bearing garbage and a nonempty batch, synchronize deletion and routing with an explicit barrier, assert that unlink cannot precede routing, and verify exactly-once delivery on the remapped owner. Run under kill and ASan.

6. **The finalization hazard lacks the required trace evidence, and its stack scrub has no retirement owner.**  
   **Where:** [7a-1.md:250](/tmp/boilreview/boil/design/work/7a-1.md:250), [test_gc.py:3158](/tmp/boilreview/Lib/test/test_gc.py:3158); PROCESS §4.

   The record gives the 19-in-20 failure rate and attributes it to a stale stack word, but supplies no `gc_stack_hits(addr)` frame/offset or retain-mode evidence. The replacement test adds `clobber(50)` and repeated collections without naming an owner or removal condition.

   **Fix:** Record the retained object’s stack-hit and retain evidence. Fix the responsible root discipline, or assign the temporary scrub to a named owner with a precise retirement condition. Preserve the useful assertion that every referent already dead has had its callback delivered.

## Non-blocking findings

- **Promised documentation is missing.** [7a-1.md:113](/tmp/boilreview/boil/design/work/7a-1.md:113) promises `gc.rst` and the architecture record; neither changes. Document owner selection, deferred foreign delivery, and domain-wide collection counts, or name the accepted deferral in “As built.”
- **The instruction outlier is recorded without attribution.** [7a-1.md:358](/tmp/boilreview/boil/design/work/7a-1.md:358) reports `comprehensions` at +2.19%. The four samples in each JSON record are tightly grouped. Attribute the delta, or explicitly retain it as unexplained with an owner; no performance tuning is required.
- **The audit description overstates the source.** [7a-1.md:14](/tmp/boilreview/boil/design/work/7a-1.md:14) says `collect()` retains `Py_NO_INLINE`, and [7a-1.md:54](/tmp/boilreview/boil/design/work/7a-1.md:54) calls it precise. Its definition has neither annotation. Reconcile these claims with the actual head and applicable audit evidence.

## Questions for the coordinator

Will the B-class exclusion and skipped regression be retired with this landing? The work document assigns those changes to 10a and the coordinator, but [test_gc.py:2312](/tmp/boilreview/Lib/test/test_gc.py:2312) remains skipped.

## Verdict

fix first
---
Landing reviewer: codex, 2026-10-28 14:09 PDT; base 5f74a991472, head 1a17992b09b
