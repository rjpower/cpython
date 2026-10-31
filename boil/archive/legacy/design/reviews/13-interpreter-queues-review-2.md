## Summary

Per-owner queues and explicit opening address the reported startup failure.  
Cross-owner ordering, interpreter lifetime, and metadata retirement still have correctness holes.  
This is a source-based design review; no implementation was tested.

## Blocking issues

1. **Cross-owner ordering can lose buffered output.**  
   **Where:** §2’s “Cross-interpreter order,” §§3.2 and 8.5; [iobase.c](/home/power/code/cpython/.worktrees/boil-7a/Modules/_io/iobase.c:325), [textio.c](/home/power/code/cpython/.worktrees/boil-7a/Modules/_io/textio.c:3275). A buffered wrapper allocated in an idle subinterpreter can reference a raw file allocated in main. Routing both finalizers lets main close the raw file before the wrapper flushes. This graph need not contain a cycle, so PEP 442’s unspecified cycle order does not justify the change. Rooting both objects preserves memory, but does not preserve the resource.  
   **Fix:** Defer a referent’s finalizer until queued referrers that need it have finished, without synchronously waiting for another interpreter. Add a split-owner buffered-write test that checks the resulting bytes.

2. **The copied domain can contain freed interpreter pointers before routing.**  
   **Where:** §3.2’s deletion exclusion and §5.3; [pystate.c](/home/power/code/cpython/.worktrees/boil-7a/Python/pystate.c:878), [gc_trace.c](/home/power/code/cpython/.worktrees/boil-7a/Python/gc_trace.c:3011). `PyInterpreterState_Delete()` detaches the current thread, releasing the GIL, **before** unlinking the interpreter. It can therefore finish deletion after another thread’s mark releases the head lock but before that collection routes its batch. `_PyGC_LiveOwner()` would then inspect a freed domain entry to apply the close rule.  
   **Fix:** Establish lifetime protection through routing. Canonicalize closed owners under the head lock and retain any remaining interpreter references until the splice, or extend the protected interval. Add deterministic synchronization between mark completion, deletion, and routing.

3. **An idle queue erases the legacy finalizer’s delivery marker.**  
   **Where:** §§3.3 and 8.4; [gc_live.c](/home/power/code/cpython/.worktrees/boil-7a/Python/gc_live.c:649), [gc_trace.c](/home/power/code/cpython/.worktrees/boil-7a/Python/gc_trace.c:2110). `queue_legacy()` sets `_Py_GC_DEL_CALLED_FLAG` when enqueueing. A subsequent collection marks the pending object through its queue and clears that flag as “reachable again.” `run_queue()` invokes `tp_del` without restoring it. After delivery, the next death can invoke `tp_del` again without any application resurrection.  
   **Fix:** Distinguish pending retention from resurrection and record completed delivery at execution. Extend §9.4 with an acyclic legacy-finalizer object, several collections while its owner is idle, then delivery and further collections.

4. **Passing the allocation owner still does not establish metadata ownership.**  
   **Where:** §3.4 and test §9.9; [codeobject.c](/home/power/code/cpython/.worktrees/boil-7a/Objects/codeobject.c:1583), [dictobject.c](/home/power/code/cpython/.worktrees/boil-7a/Objects/dictobject.c:6201). Interpreter A can set an extra on B’s code object using A’s index. Discovery and destruction through B’s table can then invoke B’s unrelated free function. Similarly, A watching B’s dict sets a shared bit: a colliding B watcher can receive DEALLOCATED despite never watching that dict. Documenting that A misses the event does not prevent this wrong notification.  
   **Fix:** Enforce owner-local registration and access, or retain registration identity with the metadata. Apply the rule to code extras and dict/type watchers. Test foreign registration itself; §9.9 currently tests only objects registered by their allocation owner.

5. **Metadata retirement is not a defined, safe close transition.**  
   **Where:** §§5.1–5.4; [pycore_gc_trace.h](/home/power/code/cpython/.worktrees/boil-7a/Include/internal/pycore_gc_trace.h:388), [codeobject.c](/home/power/code/cpython/.worktrees/boil-7a/Objects/codeobject.c:2333). The heap snapshot explicitly does not keep its objects alive. Calling arbitrary extra free functions can collect, release the GIL, enqueue more work, or create new extras and code objects outside that snapshot. A single drain followed by one walk cannot establish an empty queue and retired metadata. The walk is also fallible, with no failure policy. For a never-opened interpreter, those free functions can run code before readiness—the original failure class.  
   **Fix:** Specify the complete transition: root the snapshot with a span, define snapshot-allocation failure, prevent or reconcile new metadata during retirement, drain resulting calls, and only then publish closed ownership. Never execute arbitrary retirement hooks on a never-opened interpreter. Test reentry, GIL release, new extras, and allocation failure.

6. **Absorption’s stated locks do not protect arena enumeration.**  
   **Where:** §5.3; [arena-abandon.c](/home/power/code/cpython/.worktrees/boil-7a/Objects/mimalloc/arena-abandon.c:232). Holding the main domain’s GIL does not stop isolated interpreters with their own GILs from allocating and reclaiming. Arena bitmaps are process-wide. Existing cursors take each arena’s `abandoned_visit_lock` because even a foreign-subprocess search temporarily clears abandonment bits. The proposed subprocess OS locks alone cannot prevent absorption from missing a segment and subsequently freeing its `subproc`.  
   **Fix:** Specify absorption using the arena synchronization protocol, plus the exact OS-lock order; existing OS visitation takes the visit lock before the list lock. Test concurrent isolated-interpreter allocation/reclamation during repeated adoption, covering arena and OS segments.

7. **Skipping GC does not make fork-child disposal safe without a thread state.**  
   **Where:** §5.5; [pystate.c](/home/power/code/cpython/.worktrees/boil-7a/Python/pystate.c:699). `interpreter_clear()` asserts that `tstate` is non-NULL and uses it before reaching `_PyGC_CollectNoFail()`. `PyThreadState_Clear()` also requires a matching current interpreter. The surrounding cleanup includes operations that can notify watchers, and thread deletion takes the head lock while this caller already holds it. Changing only the GC entry points leaves those failures intact.  
   **Fix:** Define a dedicated fork-child disposal path that requires no current thread, invokes no callbacks, and uses helpers compatible with the already-held lock. Make §9.13 assert both successful disposal and the absence of forbidden callbacks.

## Non-blocking issues

- **The test matrix does not substantiate the review-closure claims.** §§9.4 and 9.10 exercise `__del__`, rather than every call kind. Add routed legacy, capsule/module, and buffer-release hooks, including retention by a conservative C-stack root. Also test a weakref owned by B watching A’s object; the existing cases do not distinguish weakref ownership from referent ownership.

- **The single-interpreter measurements miss node-allocation cost.** `collector.py` creates tuples and lists, rather than callback-heavy garbage. Add a burst of cheap callbacks or C destroy hooks, measuring allocation/drain instructions and peak RSS. Record metadata-retirement teardown time separately.

- **Avoid the redundant helper form.** The work document retains `_PyCode_HasExtraFree()` beside `_PyCode_HasExtraFreeIn()`, although the current helper has one caller. Change its signature and update the caller.

- **Resolve implementation ownership.** The work document leaves several files “To be ruled” and treats disjoint hunks as sufficient isolation. `PROCESS.md` §2 prohibits concurrent edits to the same file; settle those hand-offs before implementation.

## Questions

- Where do discarded-call counters remain observable after an isolated interpreter’s live state is freed?
- Which main-interpreter deletion path frees adopted states and releases kept-thread references, including across repeated `Py_Initialize()`/`Py_FinalizeEx()` cycles?
- Are the isolated-heap and fork-extra leaks permanent accepted behavior? If temporary, name their retirement step and condition.

## Verdict

revise
---
Reviewer: codex, 2026-10-28 10:22 PDT
