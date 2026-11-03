## Summary

Reviewed `1a17992b09b..a0338fd7634`, every C/test hunk, and the requested records; no handler or generator hunks changed.  
The readiness and closed-entry fixes address the earlier defects, but verification and test evidence still block landing.  
No runtime checks completed: the read-only sandbox prevented `uv` from locking its shared cache.

## Blocking findings

1. **The changed child tests do not inherit the reported kill mode.**

   **Where:** Calls such as [test_pin](/tmp/boilreview2/Lib/test/test_gc.py:3296) pass no environment overrides to [_queue_child()](/tmp/boilreview2/Lib/test/test_gc.py:2831). [script_helper.py:115](/tmp/boilreview2/Lib/test/support/script_helper.py:115) consequently selects `-I`. [gc_trace.c:4027](/tmp/boilreview2/Python/gc_trace.c:4027) reads `PYTHON_GCVERIFY` only when `use_environment` is true.

   Thus the parent suite’s kill mode does not activate kill inside these children. This affects all seven new or changed tests and undermines the coverage claimed under “The bounded barrier” and [“The ladder at the landing head”](/tmp/boilreview2/boil/design/work/7a-1.md:574). ASan instrumentation remains active; its claimed combination with kill is unproved.

   **Fix:** Forward the requested collector settings explicitly, preserving test-specific overrides, and verify the active mode inside the child. Merely adding `__isolated=False` is insufficient: this helper can then select `-E`. Rerun the affected tests and verification set at the resulting commit, and correct the record.

2. **Aggregate delivery counts do not establish the promised per-call invariants.**

   **Where:** [test_allocation_failure](/tmp/boilreview2/Lib/test/test_gc.py:3090) records only kind and interpreter; its [comparison](/tmp/boilreview2/Lib/test/test_gc.py:3116) expects three identical entries per kind/owner. [test_pin](/tmp/boilreview2/Lib/test/test_gc.py:3308) likewise records five indistinguishable callbacks and finalizers. The record nevertheless claims [each call ran once](/tmp/boilreview2/boil/design/work/7a-1.md:475).

   Losing one callback and delivering another twice produces the expected totals. Neither test checks the contents of a queued object’s graph, so premature reclamation of a referent unused by the callback can also escape detection. These are weaker oracles than note 13 invariants 3–4.

   Separately, [test_ready_for_every_thread](/tmp/boilreview2/Lib/test/test_gc.py:3227) permits delivery after many GIL acquisitions within thirty seconds; invariant 6 promises delivery at the next eligible safepoint.

   **Fix:** Give each application object and callback a stable token; compare individual deliveries, including duplicates, and validate retained payloads after intervening collections. For readiness, synchronize the blocked collector and check delivery after one receiving acquisition and safepoint. Run these checks with kill actually active.

3. **Two hazards remain attributed to load without a demonstrated cause.**

   **Where:** [Risks](/tmp/boilreview2/boil/design/work/7a-1.md:196), [landing disposition 5](/tmp/boilreview2/boil/design/work/7a-1.md:478), and [the earlier ladder’s environment failure](/tmp/boilreview2/boil/design/work/7a-1.md:519).

   Identifying the eleventh remap as `_testinternalcapi` establishes *which object* differed, but the record explicitly leaves *why* it differed untraced. It also does not distinguish retention from the interpreter having been unlinked before the second pass and therefore leaving the domain. The widened [assertion](/tmp/boilreview2/Lib/test/test_gc.py:3344) does not resolve that uncertainty.

   Likewise, ten isolated passes do not explain the dangling `QueueFeederThread`. The exercised [teardown code](/tmp/boilreview2/Lib/test/_test_multiprocessing.py:7765) already distinguishes live threads from stopped threads awaiting collection; the record does not identify which case failed.

   **Fix:** Correlate second-pass domain membership, deletion, and module reachability; obtain stack-hit and retain evidence if retention is responsible. Capture the feeder’s state, stacks, and pending finalization at failure. Record each demonstrated cause and assign any remaining workaround an owner and retirement condition. “Hand off if it recurs” does not satisfy PROCESS §4.

4. **A required gate is incorrectly described as optional.**

   **Where:** [The record](/tmp/boilreview2/boil/design/work/7a-1.md:392) says `check_unbuilt.sh` was optional and omitted. [PROCESS §3](/tmp/boilreview2/boil/PROCESS.md:199) requires it for every rung editing `Modules/`; this diff changes [_testinternalcapi.c](/tmp/boilreview2/Modules/_testinternalcapi.c:4403). The final ladder supplies no replacement result.

   **Fix:** Run the required syntax checks and record their commit and result, or record an explicit coordinator ruling granting a scoped exception. Correct the statement that the standing rule makes them optional.

## Non-blocking findings

- **The final ladder’s commit coverage is otherwise current.** It names `7ce8d95f6b1`; `a0338fd7634` changes only the work record. The earlier stale-head objection is resolved, subject to finding 1.
- **The production fixes are focused.** [queue_splice()](/tmp/boilreview2/Python/gc_live.c:359) now publishes local readiness; [collect_from_entry()](/tmp/boilreview2/Python/gc_live.c:1650) rejects an unavailable live state before passing it onward. I found no added reference counting, inline values stored in `PyObject *` fields, or shape/version changes.
- **Measurements remain historical.** [Measurements](/tmp/boilreview2/boil/design/work/7a-1.md:395) explicitly measure `ae73621efff`, before these fixes. The JSONs support the reported ratios, including comprehensions at +2.19%, now assigned to step 8. Refresh the final-code measurements or explicitly assign their later refresh; also account for PROCESS §3’s allocation and box-count requirement.
- The 9.12 retention explanation now supplies a frame, offset, retain evidence, and a 10a owner. The stack scrub was removed. Documentation and module-`m_free` coverage are named deferrals/deviations.

Every new or changed test has these limits:

| Test | Failure it would not detect |
|---|---|
| `test_allocation_failure` | A lost callback offset by a duplicate; premature loss of an uninspected payload. Finding 2. |
| `test_progress_during_finalization` | Compensating callback loss/duplication: it compares totals rather than weakref identities. |
| `test_ready_for_every_thread` | Delivery delayed across several receiving acquisitions. Finding 2. |
| `test_collect_after_close` | Failure of the separate live-state allocation/OOM path; this fixture exercises closure. |
| `test_pin` | Compensating callback loss/duplication, or corruption of uninspected object contents. Finding 2. |
| `test_every_kind_routed` | A module-specific `m_free` defect: capsule and buffer coverage do not execute that branch. This deviation is recorded. |
| `test_weakref_owner_runs_callback` | An early automatic collection substituting for the intended foreign collection; automatic collection remains enabled. |

All seven also share the kill-coverage gap in finding 1.

## Questions for the coordinator

- Who owns the outstanding module-variance and feeder-thread traces now, before landing?
- Was `check_unbuilt.sh` explicitly waived? If so, where is that ruling recorded?

## Verdict

fix first
---
Landing reviewer: codex, 2026-10-28 17:53 PDT; base 1a17992b09b, head a0338fd7634
