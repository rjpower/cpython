## Summary

Per-owner queues and one GIL per domain address the reported startup failure.  
Heap retirement and call cancellation still have correctness holes; wakeups can also be lost.  
This is a source-based design review; no implementation was tested.

## Blocking issues

1. **Dropping a destroy call does not prevent it from running during sweep.**  
   **Where:** §5.2; [Objects/codeobject.c](/home/power/code/cpython/.worktrees/boil-7a/Objects/codeobject.c:2333), [Objects/memoryobject.c](/home/power/code/cpython/.worktrees/boil-7a/Objects/memoryobject.c:120). `_Py_GC_NOTIFIED_FLAG` suppresses queueing, but `code_dealloc()` still frees code extras through arbitrary C functions, and buffer deallocation still calls `PyBuffer_Release()`. Dropping their queued hooks therefore leaves code-bearing cleanup for a later sweep, potentially on the main interpreter after adoption. Legacy and non-GC finalizer flags are also reset when conservative roots mark an object reachable.  
   **Fix:** Define cancellation for each call kind, including its payload and later deallocator behavior. Where cleanup cannot safely be cancelled, retain the object and its referents explicitly. Test all four kinds, subsequent collections, and retention by a conservative C-stack word; §9.5 tests only callbacks.

2. **Adopting blocks does not preserve the roots needed to trace them.**  
   **Where:** §§2, 5.3; [Python/gc_live.c](/home/power/code/cpython/.worktrees/boil-7a/Python/gc_live.c:250), [Python/gc_trace.c](/home/power/code/cpython/.worktrees/boil-7a/Python/gc_trace.c:2125). Contrary to §2, heap immortals exist: `pycore_init_builtins()` immortalizes `all` and `any`. Their referents are traced through the interpreter’s immortal registry. `_PyGC_Fini()` frees that registry before `free_interpreter()` retires the heap. The heap walk preserves unmarked immortals without traversing them, leaving their mortal referents collectible.  
   **Fix:** Transfer surviving root metadata before `_PyGC_Fini()`, including the immortal registry; explicitly audit registered `_PyGCRoot` nodes and their interpreter links. Add a retired immortal whose only outgoing reference keeps a mortal object alive, then collect under kill/ASan.

3. **Retired segments have no specified reclamation path.**  
   **Where:** §5.3; [Objects/mimalloc/arena-abandon.c](/home/power/code/cpython/.worktrees/boil-7a/Objects/mimalloc/arena-abandon.c:329), [Objects/mimalloc/segment.c](/home/power/code/cpython/.worktrees/boil-7a/Objects/mimalloc/segment.c:1706). Visiting abandoned blocks collects cross-thread free lists, but does not clear empty pages or release their segments. Ordinary reclamation rejects a different sub-process, and this design deliberately leaves no thread in the retired sub-process. Thus “memory returns when it is empty” does not follow from the named operations.  
   **Fix:** Specify an allocator operation that safely releases empty retired pages and segments, including lock ordering and object-map cleanup. Also separate retired sub-process capacity from the 64-live-interpreter limit: otherwise serial interpreter creation can exhaust the table and permanently stop collections. Test more than 64 create/destroy cycles and measure retained mappings.

4. **Allocation ownership does not establish destruction-metadata ownership.**  
   **Where:** §§3.4, 5.3; [Python/gc_trace.c](/home/power/code/cpython/.worktrees/boil-7a/Python/gc_trace.c:2020), [Objects/codeobject.c](/home/power/code/cpython/.worktrees/boil-7a/Objects/codeobject.c:2349), [Objects/dictobject.c](/home/power/code/cpython/.worktrees/boil-7a/Objects/dictobject.c:6201). `_PyCode_HasExtraFree()` consults the current interpreter’s free-function table, even when the surrounding predicate receives another interpreter. Retired code extras retain indices into the deleted interpreter’s table; interpreting those indices through the main table can invoke the wrong free function. Retired dict/type watcher bits have the same identity problem. Moreover, `PyDict_Watch()` currently permits registering a watcher on a shared dict allocated elsewhere.  
   **Fix:** Pass the owner explicitly through live destruction discovery, define watcher-registration constraints, and preserve or safely retire interpreter-local metadata before adoption. Never reinterpret old indices through unrelated tables. Test colliding watcher IDs and code-extra indices in two interpreters, both before and after deletion. Section 3.4 cannot be cut while invariant 8 remains promised.

5. **The GC bit can be consumed without draining the queue.**  
   **Where:** §4; [Python/gc_live.c](/home/power/code/cpython/.worktrees/boil-7a/Python/gc_live.c:1837). `_Py_RunGC()` drains only when allocation is below the limit. At or above it, a false `can_collect()` returns without draining, after the caller has cleared the bit. This occurs with `gc.disable()` and during finalization; stress repeatedly sets the limit to zero. Setting the bit in `take_gil()` therefore does not establish the promised progress.  
   **Fix:** Separate permission to collect from permission to drain. Drain eligible queued work even when automatic collection is disabled, and preserve/reschedule notifications when execution is temporarily forbidden. Test disabled GC, disabled thresholds, finalization, and `PyErr_CheckSignals()` without another GIL acquisition.

6. **Routing has no allocation-failure contract.**  
   **Where:** §3.2; [Python/gc_live.c](/home/power/code/cpython/.worktrees/boil-7a/Python/gc_live.c:270). Destination queue growth uses fallible C `realloc`. After partial routing, foreign calls may remain in the collecting queue, where draining violates ownership. Dropping them violates invariant 4. The existing collection failure path says “what was queued runs,” which becomes unsafe once the queue temporarily contains several owners.  
   **Fix:** Reserve every destination before irreversible weakref clearing and flag changes, or specify another failure-atomic protocol. Inject failure at each destination reservation and prove that no call runs on another owner, disappears, or loses its roots.

7. **Routing to an interpreter after `_PyGC_Fini()` is undefined.**  
   **Where:** §§3.2, 4, 5.2; [Python/pystate.c](/home/power/code/cpython/.worktrees/boil-7a/Python/pystate.c:795), [Python/pylifecycle.c](/home/power/code/cpython/.worktrees/boil-7a/Python/pylifecycle.c:2075). The interpreter remains in the domain after `_PyGC_Fini()` frees its queue state; later teardown even allocates objects. If teardown runs a watcher that releases the GIL, another domain member can discover calls for this still-listed owner. The document specifies neither a destination nor cancellation for them. Calling `get_live()` would recreate state after its sole close and discard point.  
   **Fix:** Keep a terminal queue state until interpreter removal, or cancel and record late arrivals directly. Specify the retirement transition and forbid lazy state recreation after close. Add an injected collection from another domain thread during this interval.

## Non-blocking issues

- **§5.1 overstates shutdown readiness.** `finalize_modules()` already calls `finalize_clear_sys_builtins_dict()` before the last collection in `interpreter_clear()`. Distinguish a structurally safe error-reporting path from functioning `sys.unraisablehook`, module globals, and builtins. The first two shutdown collections also do not normally take the claimed second pass while `imports.modules` remains present.

- **The test oracles need repair.** In §9.2, retain weakref objects outside the discarded cycles; unreachable weakrefs do not receive callbacks. In §9.7(c), clearing the stash cannot empty a sub-process whose interned strings remain rooted by the main dictionary or whose heap immortals remain. Separate traversal, transient-block reclamation, and persistent-root tests.

- **Ordering needs a routed test.** The existing §8.5 tests exercise one interpreter. Add mixed-owner batches with pre-existing queue entries, growth beyond initial capacity, nested collections, and a finalizer that releases the GIL while another owner thread joins the drain.

- **§11 measures little of the new cost.** Add shared-domain pause and RSS measurements varying live interpreters, retired sub-processes, and queued calls, plus a frequent GIL-release workload. Single-interpreter kernels cannot establish routing or retirement cost.

- **§3.3 gives no useful memory bound.** A queued callback or object can retain a graph allocated by other interpreters. Describe retained transitive graphs and idle-owner latency explicitly.

## Questions

- Does “owner” mean the first allocating interpreter or the current block’s interpreter? `PyObject_Realloc()` and `_PyObject_GC_Resize()` can move an object into the current thread’s heap. Section 2’s lifetime claim needs an enforced restriction on foreign resizing or preservation of the original sub-process.

- What precisely runs in the fork-child deletion path? `_PyInterpreterState_DeleteExceptMain()` currently calls `PyInterpreterState_Clear()` with no current thread state. Name the no-Python disposal path and test queue dropping and heap adoption after fork.

- Is main-close dropping decided? Decisions 4 and §5.2 prescribe it, while §13.1 leaves it as a recommendation. Resolve that, and state when its counters can be observed before the main live state is freed.

## Verdict

revise
---
Reviewer: codex, 2026-10-28 09:59 PDT
