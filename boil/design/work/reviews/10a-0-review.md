## Summary

The rung’s scope is appropriate, and the stated C baseline matches the repository.
The proposed suite total loses collections, and deferred scans need explicit attribution.
Revise the measurement path and ASan trial before implementation.

## Blocking issues

1. **The “suite sum” omits subinterpreter collections.**  
   **Where:** “What changes” §3; [gc_live.c](/home/power/code/cpython/.worktrees/boil-10a/Python/gc_live.c:1058), `collect()`, `live_domain()` in `Python/gc_trace.c`, and `_PyGC_Fini()`. Collections run by a subinterpreter are charged to its live state, even when their mark covers shared heaps. Dumping only the main interpreter’s totals loses that work. Environment inheritance also does not cover children using `os._exit()`, while a forked child that finalizes normally inherits its parent’s counters.

   **Fix:** Charge each production mark once to its initiating interpreter and emit each interpreter’s totals with PID and interpreter identity. Exclude inherited pre-fork counts. Document abnormal-exit omissions alongside environment-clearing omissions. Test subinterpreter teardown, normal fork-child teardown, and repeated interpreter initialization.

2. **Queued scans have no specified way to preserve their counter class.**  
   **Where:** “What changes” §1; [gc_trace.c](/home/power/code/cpython/.worktrees/boil-10a/Python/gc_trace.c:499), `queue_range()`, `queue_mem_block()`, `live_mark()`, and `scan_queued()`. PyMem blocks and shallow-object blocks share an untyped `(start, end)` queue. Counting their sizes when queued does not identify which class receives their hits when scanned later. Leaving the caller’s class active would charge those hits to stack, data, or structs.

   **Fix:** Carry `mem` versus `shallow` with each queued range and set the class when consuming it. Define words as actual reads after clipping, without counting them again at enqueue. Keep “hit” as every successful `live_lookup()`, including already-marked objects; “Risks” currently calls it “per marked root,” which suggests a different count. Preserve scan order and reachability. Test duplicate pointers, transitive PyMem chains, shallow blocks, and clipped ranges.

3. **The established benchmark runner removes the measurement switch.**  
   **Where:** “Ladder”; [boil/bench/run.py](/home/power/code/cpython/.worktrees/boil-10a/boil/bench/run.py:85). `CHILD_ENV` removes every `PYTHON*` variable, including `PYTHON_GCROOTSTATS`. Thus running the standard runner with the variable set produces no benchmark-child records. Simply preserving it would also mix build probes, warm-ups, and low/high instruction-count runs unless records are separated.

   **Fix:** Specify the exact sampling command. Either invoke `internal/driver.py` directly for dedicated root-stat runs or add an explicit runner option that passes the switch and identifies benchmark, loops, and run. Add an end-to-end check that the collector run produces nonempty, parseable records with collections greater than zero.

4. **The ASan trial does not configure the collector described in note 11.**  
   **Where:** “What changes” §5 versus note 11 §8.3; [obmalloc.c](/home/power/code/cpython/.worktrees/boil-10a/Objects/obmalloc.c:288), `Include/internal/pycore_mimalloc.h`, and the conservative scanners in `Python/gc_trace.c`. The configure flags enable compiler instrumentation, but ASan defaults select libc allocation, mimalloc tracking is not enabled here, and scanner reads lack sanitizer exclusions. This trial can fail for predictable configuration reasons—or provide no tracking of mimalloc blocks.

   **Fix:** Specify the mimalloc allocator selection, `MI_TRACK_ASAN`, stack-use-after-return setting, and narrowly unsanitized scan functions required by §8.3. Verify the collector is active and poisoning works before classifying ASan as available. Record any remaining failure under §13.6.

## Non-blocking issues

- **The probe’s non-collecting list conflicts with the value API.** The current `boil/tools/measure/c_roots.py` prototype exempts every `*_GET_*` call. `PyList_GET_ITEM()` and [PyTuple_GET_ITEM()](/home/power/code/cpython/.worktrees/boil-10a/Include/cpython/tupleobject.h:45) can box inline values and therefore collect. Replace blanket exemptions with reviewed entries and add small classification fixtures. Reporting a changed count is appropriate; silently excluding collecting calls is not.

- **Counter movement and schema checks are insufficient.** Add checks for `hits <= words`, totals versus last-collection deltas, diagnostic marks leaving production totals unchanged, and snapshots remaining coherent when constructing the result dict triggers allocation. Use raw bits for scanner fixtures: canonicalizing a boxed numeric `PyObject *` into `PyValue` would erase the pointer being tested.

- **Measure instrumentation overhead now.** End-step timings with counters enabled cannot isolate their cost. Compare the counter-only commit against `c2ab4382dea` on the same collector workloads, recording instructions and time. Phase timings that include transitive marking are order-dependent; document that interpretation.

- **Complete the stated gate ladder.** “After the C commit” conflicts with `PROCESS.md`’s pre-commit gate requirement. Rung completion also requires the debug suite, `test_cext`, and unchanged regeneration at level 3.

## Questions

- Does `collections` count successful mark passes or public collection requests? `collect_from_entry()` can run two passes, and finalizers can trigger further collections. What happens on mark failure?
- Where will note 11 §9’s separate walk time and baseline pause distribution be recorded? Totals and the last collection cannot reconstruct median or p99 pauses.
- Does benchmark sampling include startup, setup, warm-up, and shutdown collections? Fix that boundary before recording the baseline.

## Verdict

revise