# GC and interpreter performance: current state and plan

Updated 2026-10-31. This is the current recommendation. Measurement history,
qualification records and implementation detail live in the linked documents.

## TLDR

- **Landed:** GC fixes, direct managed scanning, register IR and eight-byte
  tagged values. No nursery yet.
- **Current result:** 0.871× upstream throughput over 29 workloads. Aggregate
  parity needs about **15% more throughput**; generators/coroutines need roughly
  **66%/62%**. Simple int/float diagnostics already show large wins.
- **Next GC change:** one useful young-space path covering generator/coroutine
  objects and frames, with cheap allocation and bulk reclamation. Avoid repeating
  per-object bookkeeping for safely disposable garbage. Copying is optional.
- **Parallel interpreter work:** attribute the remaining cost, then simplify
  shared frame/call/value interfaces where it matters. Small dispatch cleanups
  cannot yet be credited with closing these gaps.
- **Parity++:** extend existing value/arithmetic wins to calls, containers and
  library boundaries, while reducing memory use. Roughly 2× remains guidance;
  whole-suite 2× is not a prerequisite for JIT work.

## Detailed plan

### Where we are

Runtime integration is `aee46aacae2`, qualified source `0b15b4bbc9b`. Scoped
qualification, ASan, closure and both postmerge smokes passed independent review.
Selective death processing, native-only generator cleanup and direct managed
scanning are implemented. All measurements below have JITs off and use matched
GCC/O3 build policy with upstream, not its fastest production configuration.

| Workload | Our throughput / upstream | Gain needed from here for parity |
| --- | ---: | ---: |
| 29-workload geometric mean | 0.871× | 1.15× |
| Comprehensions | 0.847× | 1.18× |
| Coroutines | 0.618× | 1.62× |
| Generators | 0.603× | 1.66× |
| Richards | 1.886× | Already ahead |

This is an uneven performance profile, not a universally slow interpreter.
The same-fork comparison shows large coroutine/comprehension gains, an uncertain
8.5% generator gain with overlapping ranges, and a 1.3% Richards regression.
Five serial samples per arm provide descriptive ranges, not confidence intervals.
The aggregate gap is a geometric-mean comparison, not a weighted application cost.

### 1. Make common allocation and reclamation cheap

The remaining collector cost includes these substantial components:

| Exclusive sampled CPU | Coroutines | Comprehensions | Generators |
| --- | ---: | ---: | ---: |
| Roots plus transitive closure | 10.75% | 5.96% | 11.55% |
| Allocated-block inventory/dead classification | 12.14% | 7.28% | 4.46% |
| Sweep work, including dispatch and storage return | 9.60% | 5.26% | 3.95% |

A minor collection that still inventories and dispatches cleanup for every
young corpse preserves much of this work. A moving nursery limited to cells
and tuples misses the generator/coroutine allocations prominent in these kernels.
Neither is a sufficient first performance milestone.

Build a complete experimental young-space path with these properties:

1. **Cover the common allocations.** Admit known layouts, including generator/
   coroutine bodies and embedded frames. Unknown native and resource-bearing
   layouts use stable storage. Start with addresses stable where that avoids
   relocation work; make copying an additional capability of the same collector.
2. **Track actual exceptional work.** Maintain compact, non-rooting lifecycle metadata so
   completed, safely disposable objects can avoid a full per-object dead journal.
   Suspended frames, weakrefs, finalizers, buffers and observers retain their
   required handling. Reserve membership before publication; completion, free,
   address reuse and allocation failure must leave the index correct.
3. **Close the age boundary.** Barrier old core stores; scan allocated old native
   owners for young references. Active frame arrays remain roots. Do not rescan
   all old managed objects on every minor or register a root on every bytecode.
4. **Reclaim storage in bulk after required cleanup.** Use the existing object-
   start map and compact space metadata. Preserve allocator hooks, accounting,
   debug behavior, page ownership and thread-exit handling. Charge survivor slack.
   Keep the existing full collector as major collection and the liveness oracle.

The allocator boundary needs explicit engineering: public `mi_heap_delete()`
does not promote survivors into our old object heap. The first source review
must resolve page transfer or stable page retirement before admission expands.
Keep the current 32 MiB major floor; young capacity and bounded major debt are
separate policy settings, declared before the experiment.

Deliver **one working consumer**, including barriers, lifecycle behavior and
memory checks. Measure allocation coverage, native scan cost, per-death work,
RSS and pauses. A proof of age or movement without those savings does not earn
a default change. The [young-space implementation plan](gc-young-space-next.md)
contains the concrete contracts and acceptance checks.

### 2. Close the interpreter's remaining shared costs

Run this investigation alongside the collector implementation. Prioritize the
large generator/coroutine gaps and slower library paths such as serialization.
Use captured stacks and instruction counts first; collect a bounded additional
profile only where the current evidence cannot distinguish the costs.

The generator profile spends about 22% of its hot interval inside the GC timer.
Removing that timed work entirely illustrates only a 1.28× gain, versus the
1.66× needed in its separate timing run. This is not a transferable forecast:
queued work is partly outside the timer, and an allocation backend can improve
mutator cost too. It does show why the investigation must extend beyond that timer.
Residual sampled CPU is not yet attributed specifically to frame or call overhead.

Select general changes from the measured budget:

- **Frame/call/resume transitions:** remove duplicated work when proven, while
  preserving initialized roots, ownership and exception state. Argument-window
  sharing and several compiler/branch folds already exist; build on them.
- **Native value interfaces:** remove repeated boxing/conversion through one
  coherent receiver/result convention if it is material. Cover generic,
  specialized and escaped-method routes together.
- **Dispatch and feedback:** consume an established type/shape fact once within
  an operation. Repeated matching on later polymorphic attribute rows is proven
  in emitted code, but its frequency and cost remain unmeasured; row0 is already
  optimized. Treat it as a small cleanup until the budget says otherwise.

Keep operations orthogonal. Tagged attribute payloads already exist; merely
adding a `LOAD_ATTR_INT32` spelling creates no new guarantee. Express useful
representation facts consistently across producers, consumers and fallback paths.

### 3. Establish parity, then extend the lead

After each material change, rerun the unchanged matched workload set and report
per-row elapsed ratios, ranges, instructions and memory. Broad parity means a
credible aggregate near upstream **with the large remaining deficits visible**.
A higher mean that conceals severe regressions is not a complete result.

For parity++, extend the existing arithmetic/value advantages to calls,
containers and library boundaries, while reducing GC CPU and excess RSS. Current
profile RSS is roughly 68–87 MiB on the three allocation-heavy rows; historical
upstream contexts are 16–28 MiB. Compare memory contemporaneously before claiming
an improvement. No measured budget currently supports a general pre-JIT 2× promise.

Begin JIT work once the value, frame, root and allocation contracts are coherent
and the major pre-JIT costs are understood. Whole-suite 2× is not a prerequisite.
Retain tracing for now: the landed gains support continuing it. Restoring reference
counting would be a separate ownership redesign and matched experiment, not a switch.

### GC code and data structures

Frame registers already form contiguous root arrays. Temporary C spans use an
allocation-free LIFO chain; durable registrations link caller-owned arrays.
These lists do not track every object, and no measured bottleneck justifies
replacing them now. Cells, tuples and frame values have shared range definitions
and direct managed marking; native layouts retain the traversal adapter.

Preserve that edge knowledge. Add writable-slot/reload/pinning contracts when
copying needs them. Share space access and lifecycle-state rules with their actual
consumers; avoid a large mechanical file split or another object registry first.

### Evidence and implementation detail

- [Measured results and all 29 rows](gc-post-cleanup-performance.md).
- [Young-space implementation and acceptance plan](gc-young-space-next.md).
- [Collector source feasibility](reviews/gc-young-space-feasibility.md) and
  [allocation coverage](reviews/gc-young-space-coverage.md).
- [Lifecycle and callback contracts](reviews/gc-lifecycle-cost-and-contract-review.md).
- [Root/code interface map](reviews/gc-code-organization-review.md) and
  [interpreter review](reviews/interpreter-holistic-review-post-gc.md).
- [Landing and qualification record](reviews/gc-cleanup-final-landing.md).

Historical comparisons and failed attempts remain in those records, WORKLOG,
Git history and earlier artifact revisions. They are not part of the current plan.
