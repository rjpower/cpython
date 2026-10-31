## Summary

The tooling/promotion split is reasonable, and transitional mechanisms have named retirement steps.  
The checker’s proposed rules still accept programs with missing roots.  
Detached snapshots, quarantine, and build enforcement need firmer contracts before promotion.

## Blocking issues

1. **Anchor invalidation misses aliases and mutation helpers.**  
   **Where:** §1, “Sources” and “Calls”; proposed `root_check_c.py`.  
   `q = p; x = p[0]; q[0] = PyValue_NULL; collect(); use(x);` leaves `x` classified as `slot(p)`: the stated rule invalidates only stores through `p`. Field aliases have the same problem. Non-collecting setters and `memset()` can also destroy anchors; non-collecting does not mean read-only.  
   **Fix:** Track anchoring storage by provenance and possible aliases, with mutation summaries for macros and helpers. Unknown writes must invalidate affected anchors. Add negative tests for alias stores, field setters, and bulk clears.

2. **A span store is credited before the value becomes a root.**  
   **Where:** §1, “Sources”; §2.  
   Push a span with `n = 0`, store `base[0] = x`, omit `Grow()`, and collect. The proposed analysis accepts `span(r)`, while the visitor reads nothing. Checking that `n` fits the allocation cannot detect this omission. Kill mode detects only exercised failures; it does not establish the checker’s claimed invariant.  
   **Fix:** Credit an anchor only after proving its index is inside the current initialized prefix. Otherwise reject it or require a checked store-and-grow operation. Test missing growth, conditional growth, overwritten elements, and rebase invalidation.

3. **Object-only liveness misses the backing objects of interior pointers.**  
   **Where:** §1, “Types,” “Parse,” and “Liveness.”  
   A supported function can retain only a character buffer pointer across collection after its owning `PyObject *` becomes dead. Likewise, casts through `void *` or integer storage hide object references from the listed types. The existing release-barrier contract explicitly covers this pattern in [Include/refcount.h](/home/power/code/cpython/.worktrees/boil-10a/Include/refcount.h:106), but a precise frame loses conservative protection.  
   **Fix:** Track owner dependencies for derived pointers and reject unsupported ways of hiding references. Add a negative buffer-pointer case and a positive case with the owner in a span. Also forbid address escape of `Py_GC_ARGS_ANCHORED` functions unless indirect callers are checked.

4. **`PyDict_Next()` outputs are not necessarily borrowed from the dict.**  
   **Where:** §1, “Parse”; [Objects/dictobject.c](/home/power/code/cpython/.worktrees/boil-10a/Objects/dictobject.c:2627).  
   `_PyDict_Next()` boxes inline keys and values. The dict does not retain those boxes, even across a non-reentrant allocation. Boxing the value can itself collect after the key box has been written into a precise caller’s unrooted out-parameter.  
   **Fix:** Use `_PyDict_NextValue()` in precise code and specify its out-parameter effects. Treat object-returning outputs as fresh boxes; retaining that interface requires protecting each output before subsequent boxing. Add an inline-float key/value test with collection during the second box.

5. **“Innermost precise frame” does not guarantee a persistent detach boundary.**  
   **Where:** §4, “Start contexts”; note 11 §2’s precise detach paths; [Python/ceval_gil.c](/home/power/code/cpython/.worktrees/boil-10a/Python/ceval_gil.c:613) and [Python/pystate.c](/home/power/code/cpython/.worktrees/boil-10a/Python/pystate.c:2286).  
   Once detach entry paths become precise, the search can stop at `PyEval_SaveThread()`, `detach_thread()`, or a swap wrapper. These return while the thread remains detached—the exact lifetime failure the snapshot scheme intends to prevent.  
   **Fix:** Define a persistent boundary independently of precision. Exclude returning detach/swap helpers from the stopping condition, or keep them opaque. Test a **precise** returning detach helper followed by stack reuse; the opaque-helper test alone does not cover promotion.

6. **Quarantine lacks the allocator integration needed to preserve its evidence.**  
   **Where:** §7; [Python/gc_live.c](/home/power/code/cpython/.worktrees/boil-10a/Python/gc_live.c:997), [Objects/obmalloc.c](/home/power/code/cpython/.worktrees/boil-10a/Objects/obmalloc.c:1652), and `list_dealloc()`.  
   Starting quarantine after the sweep is too late unless physical frees have already been intercepted. Deallocators release PyMem buffers during the sweep, and `_PyGC_FreeDeadBlock()` directly frees object blocks. Buffers may be reused before post-sweep processing.  
   **Fix:** Name the interception points and ownership records. Defer physical frees when they occur, preserve normal destruction ordering, then install traps and poison after legitimate accesses finish. Specify mark separation between A and B, object-map removal, and quarantine expiry. Demonstrate the cached-interior-pointer case under ASan with allocator reuse pressure.

7. **The mandatory proofs are not connected to the mandatory gates.**  
   **Where:** §§1 and 3; [Makefile.pre.in](/home/power/code/cpython/.worktrees/boil-10a/Makefile.pre.in:793), [boil/tools/gate.sh](/home/power/code/cpython/.worktrees/boil-10a/boil/tools/gate.sh:24).  
   A CLI and an optional `check-gc-inline` target do not make unchecked precision fail the build. The work doc omits note 11’s tree-wide stamp rule and does not connect the linked-image audit to normal builds or gates. The deliberately failing inline fixture also needs isolation from ordinary successful builds.  
   **Fix:** Specify dependencies that run the C checker over every marked definition and audit the final core image. Build negative fixtures separately. Add an integration test showing that the actual build/gate rejects an unchecked precise function and an opaque inline.

## Non-blocking issues

- **Frame-stack boundaries need exact arithmetic.** §6 says “to each chunk’s end,” but [Python/pystate.c](/home/power/code/cpython/.worktrees/boil-10a/Python/pystate.c:2760) deliberately reserves the final word. Name that boundary and cover cached chunks, overlapping frames, and unchecked push paths when publishing the NULL-executable sentinel.
- **Add the span tests required by note 11 §8.4.** Explicitly include raw boxed-float pointer identity, NULL entries, partial initialization failure, rebase, and failure cleanup. Canonicalizing a raw box would root the inline value instead of the box.
- **Resolve the fallback-test contradiction.** §4 requires zero suite fallbacks and deliberately introduces one. Distinguish expected test fallbacks from unexpected ones.
- **Strengthen native-stack fixtures.** Verify emitted register saves and CFI, and prevent sibling-call elimination. A local register declaration alone guarantees placement only at constrained assembly operands; see [GCC’s documentation](https://gcc.gnu.org/onlinedocs/gcc/Local-Register-Variables.html). The loader test should verify actual unload and address reuse.
- **`dladdr()` cannot provide every promised owner name.** `collect_from_entry` is a local symbol in the current binary and absent from its dynamic symbols. Record image plus offset and symbolize offline.
- **Make measurements executable.** Besides detach cost, assign commands and baselines for cold decoding, cache memory, deep callbacks, many detached threads, and release-build injection overhead, as note 11 §9 requires.

## Questions

- In this tooling-only landing, does A use today’s enabled root policy or the final post-10a policy? Specify when struct and core-PyMem roots leave A.
- How does the trap type work with 6c’s shape-based header, and which implementation waits for 6c?
- Are `retain` runs deliberately debug-only, or will the release run recommended by note 11 question 13.4 be supported?

## Verdict

revise