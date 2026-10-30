# GC performance: measured costs and implementation plan

Coordinator decision, 2026-10-30, replacing revision 1 after review 33. The
supported Linux/main-interpreter/GIL collector is landed in `75e8a1e0c01`.
The next change, selective death processing, is implemented and measured in
an isolated branch; **it is not landed**. Its coroutine throughput improves
**30.9%** in the unprofiled paired comparison, with smaller, noisier changes
on the other rows. We will qualify that change, separate native-only generator
cleanup from Python finalization, then develop a common copying nursery with
stable native/large-object space. Restoring reference counting remains a
technically valid alternative; the measured win favors these simpler cuts first.

This plan supersedes the earlier requirement to ship nonmoving generations
before movement. The existing generation/store ledgers remain source inputs.
A nursery is a substantial implementation, with the finite contracts below;
we will not build two young collectors to satisfy an old sequence.

## 1. What we measured

The [matched 29 baseline](pre-I-post-GC-timing.md) remains **0.8243× upstream
throughput** geometrically: 21 rows slower, eight faster. Coroutines,
comprehensions and generators are 0.449×/0.510×/0.574×; float, Richards and
nbody are 2.039×/1.866×/1.668×. These are release interpreter results, all JITs
off. The new four-row result does not replace that aggregate.

### Unprofiled selective-death comparison

The candidate classifies actual weakref/finalizer/buffer work once, using
existing dead-record bits and stage guards. It preserves graph connectors,
resurrection, OOM rollback and callback policy; it adds no registry or scheduler.
Source `10d8773815c` includes implementation `788695e3` and corrected behavioral
tests `69c34fb1`. The O3 candidate is compared with qualified source `4da292b1`,
ordinary native `3c6`. Both use GCC 15/O3, frame pointers and eval `-fno-gcse`,
without PGO/LTO/BOLT, debug, statistics, JIT or tail dispatch.

| Workload | Parent median ms/run() | Candidate ms/run() | Parent/candidate throughput |
| --- | ---: | ---: | ---: |
| Coroutines | 219.667 | 167.858 | **1.309×** |
| Comprehensions | 205.070 | 193.040 | 1.062× |
| Generators | 104.795 | 97.200 | 1.078× |
| Richards | 61.635 | 62.567 | 0.985× |

Eight fresh processes, one warmup and five serial samples per process produce
40 retained values, with arm order alternating across rows. These are correlated
samples, not five independent process pairs. Coroutine ranges do not overlap;
the other rows' ranges overlap. The coroutine elapsed reduction is **23.6%**;
30.9% describes throughput, not elapsed reduction. Independent replay reproduces
all medians from raw stdout. The [163-reference completion index](../../../../boil-gc-selective-death/build/selective-death-paired-four/paired-completion.json)
has SHA256 `34033f1352e2979760c6587a80db293c3a74d12ae4d35439ccab1931cd40a4a7`.

### Native profiles, hardware counts and memory

A separate finite batch uses **13 processes**, not 100 repeated suites:
qualified fork at32/64 MiB on four rows, upstream defaults on four rows, and
candidate32 MiB coroutines. Each performs one warmup and 50 hot calls of the
registered workload. `perf record` samples user CPU at 199 Hz with frame-pointer
call chains; `perf stat` counts user cycles/instructions. FIFO acknowledgements
enable both around the hot calls. Analysis filters the actual PID and monotonic
hot boundaries; every sample belongs to one exclusive bucket. All13 streams
reconcile with raw sample records, with zero lost/excluded samples. Hardware
counts have 100% running coverage. GC remains enabled; verifier/stress/extras
are off, and all fork runs report zero root fallback.

| 32 MiB/default condition | Hot seconds | Collections | Timed GC seconds | Cycles, billions | Instructions, billions | Peak RSS, MiB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Fork coroutines | 12.294 | 301 | 6.022 | 50.08 | 85.275 | 67.35 |
| Selective-death coroutines | 8.729 | 301 | 3.302 | 37.21 | 79.338 | 67.50 |
| Upstream coroutines | 4.819 | 0 | 0 | 20.38 | 58.731 | 16.17 |
| Fork comprehensions | 12.430 | 115 | 6.214 | 53.43 | 84.954 | 84.45 |
| Upstream comprehensions | 5.554 | 0 | 0 | 23.34 | 58.351 | 19.35 |
| Fork generators | 5.509 | 59 | 1.574 | 23.29 | 44.351 | 74.12 |
| Upstream generators | 3.036 | 0 | 0 | 12.78 | 35.024 | 28.24 |
| Fork Richards | 3.211 | 0 | 0 | 13.70 | 35.388 | 37.06 |
| Upstream Richards | 6.275 | 1 | 0.000158 | 26.89 | 53.942 | 16.31 |

GC seconds cover mark, classification, retention, ordering and sweep; they
exclude later queued Python callbacks. RSS is whole-process peak, not hot-only
live memory. PMU boundaries include a little control work around the hot window.
Upstream's zero collections means zero cyclic collections: ordinary reference
counting and destruction still run. Setup, code and allocator differences remain;
this is a matched interpreter comparison, not an isolated RC toggle.

The candidate keeps 301 collections and essentially the same RSS, while timed
GC falls **45.2%** and instructions **7.0%**. This corroborates the unprofiled
win without attributing the entire elapsed change to an instruction count.
The candidate still takes 8.729s versus upstream 4.819s on this profile; parity
has not been demonstrated. Coroutine cycles/instruction fall from0.587 to0.469
with selective death, versus upstream0.347. The deficit includes both extra
instructions and more cycles per instruction; no cache-miss counters were
collected, so a locality explanation remains a hypothesis.

The [97-reference native index](../../../build/checkpoint-i-current-cost-attribution/native-comparison-v1/completion.json)
(`9ef2649b698565b56f73fc2e95809a220d4c155dc3c46bccd4f04c9788698598`)
contains raw perf data, counters, source/product witnesses and cleanup checks.
[Disjoint analysis](../../../build/gc-lifecycle-review-analysis/analysis.json)
(`5016ed981ec549c88b03ba226cf8a2f4bba9ee1a17371fbebea200a0a1e17019`)
and the [independent lifecycle review](reviews/gc-lifecycle-cost-and-contract-review.md)
retain all 13 rows, decoding rules and limits. These build artifacts are retained
locally; the tracked reviews preserve the useful tables and interpretation.

## 2. Where time goes, and where the design wastes work

The current collector allocates with mimalloc, records initialized object starts,
and charges gross requested object/PyMem bytes, including full realloc requests.
With the GIL held, collection captures supported roots, marks the reachable
heap, inventories unreachable objects, handles weakrefs/finalizers/hooks and
retains their closures, orders necessary native teardown, then frees storage.
A rooted interpreter-wide drainer delivers queued callbacks at safe points.
Every automatic collection inspects the whole heap.

The following percentages are **exclusive sampled user CPU**, not inclusive
stacks, callback counts or exact wall times. Mark includes heap enumeration and
classification. A nearest recognized frame assigns each sample once; an inlined
`collect_pass` ancestry without a narrower phase remains ambiguous.

| Phase | Fork coroutine 32 | Candidate coroutine 32 | Fork comprehension 32 |
| --- | ---: | ---: | ---: |
| Mark/enumerate/classify | 17.63% | 22.71% | 8.69% |
| Finalizer eligibility | 4.51% | — | 2.23% |
| Finalizer ordering | 0.21% | — | 9.05% |
| Retain finalizer closure | — | — | 13.07% |
| Other finalizer preparation | — | — | 0.28% |
| Weakref stage | 5.15% | — | 2.92% |
| Destroy-hook selection | 3.92% | — | 2.48% |
| Sweep/storage release | 8.30% | 8.13% | 5.77% |
| Native teardown under sweep | 3.45% | 4.94% | 1.26% |
| Other collector | 5.32% | — | 2.44% |
| Callback drainer subtree | — | — | 4.06% |
| Ambiguous collect/drain ancestry | 1.62% | 1.86% | 2.27% |
| Residual user CPU | 49.87% | 62.37% | 45.47% |

A dash means no resolved samples in that bucket, not literally zero work.
Shares change denominator when total runtime falls. Root/transitive-mark timers
are inside GC, not an additional charge. Unresolved physical libc leaves retain
resolved ancestry; they are not guessed to be memcpy or allocation.

Three avoidable costs emerge:

1. **Testing type capability instead of actual work.** Finished generators can
   have a finalizer slot yet need no finalization; weakref-capable types often
   have no registered weakrefs. Repeated negative checks were expensive. The
   measured selective-death patch removes much of that work.
2. **Treating native frame cleanup as Python-capable finalization.** The
   comprehension workload leaves ordinary generator expressions suspended
   through short-circuiting `any`. Their frames need cleanup. Some already
   satisfy `gen_close`'s existing clear-only proof, yet collection retains and
   orders them as if they must execute Python. Eligibility, ordering and closure
   retention total **24.36%** of sampled CPU; delivery is 4.06%. That selects the
   next distinction. It is a target cost, not a forecasted24% speedup.
3. **Paying death processing for every temporary.** Whole-heap enumeration,
   dead-record construction and per-object recycling remain even after empty
   stages disappear. A nursery can make storage reclamation proportional mainly
   to survivors, while genuine native cleanup remains explicit.

We have not measured expensive user weakref callbacks here. Calling this a
“callback performance problem” obscures the preparation and no-op costs.

## 3. Required lifetime guarantees and choices we made

| Required behavior | Current implementation choice and next action |
| --- | --- |
| A surviving callback-bearing weakref is dead before notification; callbacks on one referent run newest-first; exceptions are reported. | Keep invalidation before storage reuse, then deliver rooted notifications at safe points. |
| Callback-free weakrefs can remain usable through finalization/resurrection in our pinned upstream. | Preserve that distinction; after resurrection, later death must clear the surviving references correctly. |
| Finalizers see valid objects; resurrected graphs remain usable. | Keep required storage alive. The fork's global referrer-before-referent finalizer DFS is a stronger policy than Python's unspecified cyclic-finalizer order. |
| Native cleanup can read referents, flush a stream or release a live buffer acquisition. | Preserve those actual dependencies until teardown completes; storage remains intact while a consumer can use it. |
| Delivery remains safe under reentry and ordinary threads. | Keep the simple single rooted drainer for now. A global FIFO/single drainer is our policy, rather than a general Python API guarantee. |

Python explicitly leaves cyclic finalizer ordering unspecified. That permits
simpler ordering for Python finalizers, but does not make a raw stream safe to
free before a buffered writer finishes using it. Our first change removes
*unneeded participants*, preserving the existing resource guarantees; the next
native-only cohort avoids finalizer preparation altogether when nobody can
observe it. The [lifecycle specification](https://docs.python.org/3/c-api/lifecycle.html)
and [pinned upstream weakref rationale](https://github.com/python/cpython/blob/321925c92c9e67864cafa1566840927f6f07e90a/Python/gc.c#L779-L787)
are the semantic references.

The dead array is a contiguous inventory. Traversals follow object edges when
needed; there is no permanently materialized callback graph. Modern finalizer
ordering uses a shared visited set, approximately `O(D+V+E)`; native-first
ordering adds sorting/searching selected objects. Ordinary dead objects can
connect two relevant objects, so filtering a cohort must retain those connectors.
The current sparse weakref table uses expected-constant-time chained lookups.
Its entries have stable addresses because callers can hold the list-head slot
across allocations. These are reasonable structures. The first gains come from
less work, rather than replacing everything with a new graph representation.

### A simpler weakref path

We will keep the sparse registry, select actual registered work and preserve
separate invalidation/notification. Java similarly clears weak references when
weak reachability is established and allows later queue notification; Python's
callback-free/resurrection rules still determine our exact phases.
[Java WeakReference](https://docs.oracle.com/en/java/javase/25/docs/api/java.base/java/lang/ref/WeakReference.html)
provides that architectural comparison.

If a weakref-rich residual profile shows clearing itself is material, add a
private bulk-clear helper using the already captured registry head. Today each
cleared weakref can repeat the lookup. The helper will preserve locking, entry
lifetime, newest-first order and OOM preparation before irreversible clearing.
The first nursery pins an object when it gains a weakref, avoiding a relocation
handle table or a new resurrection-identity mechanism. This is a concrete small
migration boundary, with weakref-heavy coverage measuring its cost.

## 4. Allocation policy: retain32 MiB while reducing actual work

After a collection the allowance is `max(32 MiB, last_live_object_bytes)`.
This is gross new-allocation headroom, not total heap, nursery size or RSS.
The32/64 MiB experiment sets the floor before setup/warmup in both arms, using
`gc.set_threshold`; it performs no forced collection. The setter's initial
limit and the subsequent post-collection formula are recorded separately.

| Workload | Instructions saved at64 MiB | Peak RSS32 →64 MiB | Lifetime max pause32 →64ms |
| --- | ---: | ---: | ---: |
| Coroutines | 3.24% | 67.35 →102.43 | 33.65 →42.06 |
| Comprehensions | 1.66% | 84.45 →145.77 | 70.26 →118.00 |
| Generators | 6.98% | 74.12 →136.70 | 48.34 →67.42 |

Collections roughly halve, but peak RSS rises52–84% for modest instruction
reductions. Richards has no hot GC and identical instructions in both arms,
yet 17.76% wall variation: this single sweep cannot establish stable budget
wall-time benefits. Lifetime maxima include earlier process activity; they
are not hot pause distributions. **Keep32 MiB as the current default.** It is
not proven optimal; increasing it is less attractive than the demonstrated
same-memory reduction in work.

Comparable runtimes separate quantities that our current policy combines:

- [PyPy](https://doc.pypy.org/gc_info.html#environment-variables) uses a nursery
  sized around half last-level cache,4M fallback; its major threshold defaults
  to 1.82 times previous major live memory, with threshold growth capped at 1.4.
- [Go](https://go.dev/doc/gc-guide#GOGC) targets total heap
  `L + (L + roots)*GOGC/100`, default 100, with a4 MiB minimum total target.
  Its guide's simplified CPU model omits sweep cost, which is material here.
- [BDWGC](https://www.hboehm.info/gc/gcdescr.html) compares allocation since GC
  against heap/free-space-divisor, adjusts for root/fragmentation costs, and
  integrates lazy sweeping with allocation.

Those constants are not interchangeable with our32 MiB gross-allocation floor.
For the actual minor prototype, start with a declared **8 MiB experimental
nursery**: CPU0's last-level-cache group on this Ryzen 3800X is16 MiB. Record
survival, pinned bytes, slack, pauses and RSS before choosing a production size.
Major allocation debt and promotion/old-space pressure remain separate. A
finite major allowance collects old garbage even during zero-promotion traffic.

## 5. Three changes, in implementation order

### A. Land selective death processing, then distinguish native-only cleanup

The selective patch has an O3 build, independent source/performance review,
focused 13 behavioral controls and four O1-debug modules passing 599 tests with
46 explicit skips. The topic documentation is now `d3331398c624`; it changes
only the record, with all 588 O3 products/aliases unchanged from measured `10d`.
Its initial resurrection test had the wrong plain-weakref
expectation; the corrected test matches pinned upstream and checks eventual
second death. Keep that original failure in the record. Complete the existing
landing gates and independent landing review, then merge. No new repetition
campaign is needed.

The [five-file native-cleanup handoff](gc-native-cleanup-next.md) is the next
code slice. Share `gen_close`'s existing side-effect-free eligibility check:
exact ordinary generator, suspended frame, no escaped frame object, safe
RESUME-depth state and no unwind monitoring. Classify the entire dead set;
use the native cohort only when there are no actual Python-capable finalizers,
registered weakrefs, hooks, buffer/read-before-free dependencies, old queued
work or active observers. Other collections use the existing path.

Finish all fallible preparation and revalidate the whole cohort before clearing
any frame. The commit interval performs no allocation, Python execution or
thread detachment. Native-clear the frame once, then use normal deallocation
and accounting. This preserves OOM rollback before commit and avoids inventing
rollback after irreversible cleanup. Validate observable finally/delegation,
resurrection, weakref, escaped-frame, monitoring and payload reclamation behavior.
Then reuse the bounded comprehension/generator comparison and report admission
coverage. The clean distinction must earn its benefit without widening semantics.

### B. Make common storage reclamation collector-owned

First streamline the proven finished-generator/storage cleanup within existing
mimalloc, using the current profiles to select the work. Define an exact-layout
contract: safe publication, outgoing references, side storage, native cleanup,
manual free/realloc and observer notifications. Exact cells and nonempty tuples
are small proof layouts; they are not claimed to dominate current traffic.
Completed generators require a separate frame/exception/origin cleanup proof.

Then implement a page-segmented bump region behind a space-aware object
lookup/enumeration interface, initially retaining full marking. Reserve start,
size, initialization and pin metadata before publication. Adapt
`_PyObject_MallocWithType`, `new_reference`, free/realloc/address reuse,
`live_lookup` and heap enumeration together; region objects must remain visible
to conservative interior-pointer lookup. Full marking visits both spaces;
no remembered set is needed for this allocator experiment.

Reset wholly dead pages after lifetime processing; retain partially live pages
at their addresses and charge their slack. This first experiment still builds
dead records: it measures allocation/recycling, not a magically eliminated
classification pass. Verify bounded memory with a survivor per page, address
reuse, allocation failure and cyclic graphs. Keep the region route only if its
coverage and memory cost justify it. This backend is scaffolding for the nursery,
not a separate generational collector to ship first.

### C. Copy common young objects; keep native exceptions stable

Yes: common objects can use a nursery while opaque, large or resource-bearing
objects go directly to stable space. Objects already exposed through raw pointers
must be pinned **in place**; moving them into a pinned pool would already break
the pointer. Retire pages with pinned survivors into stable-space metadata and
charge unused space. Allocate known-native/large layouts directly there.

Implement these contracts as a single bounded nursery milestone:

1. **Writable roots and fields.** Add visitors for actual PyValue/object slots
   in admitted core layouts. Register addresses of actual C locals that survive
   safepoints, rather than protecting only duplicate values. Every surviving
   alias must reload/recompute after a safepoint; a derived interior pointer
   pins its owner or is recomputed. Legacy readonly or
   ambiguous roots pin targets. The [OCaml C interface](https://ocaml.org/manual/5.4/intfc.html)
   illustrates registered local/global slots plus barriered field stores; it
   permits moving GC without adding an indirection to every object access.
2. **Core stores and constructors.** Add nonallocating old-to-young owner
   barriers at container, frame, type/shape and runtime stores, using the existing
   ledgers. Publish only safely traversable constructor prefixes. Partly
   initialized SHALLOW survivors remain young/protected until traversable, even
   on retired pinned pages. Commit age and remembered edges before reentrant
   Python execution. Native writes into core
   arrays use barriered setters.
3. **Stable native owners.** Permanently scan all allocated old native owners
   at each minor, using the authoritative space index. Their outgoing edges
   retain young objects; readonly edges pin those targets. The list is not a
   root at a major. This trades explicit native scanning for initially porting
   every native store; measure that tax and port hot owners later. Complete
   traversal, constructor safety and GIL-serialized strong-field mutation are
   admission requirements.
4. **Relocation and pinning.** Discover all conservative/readonly/native pin
   targets, including their required pinning closure, before evacuating any
   object. Otherwise a later raw alias could pin the original after a writable
   alias was already relocated. Reserve destination/forwarding/map metadata
   before rewriting references; copy survivors, rewrite writable slots, then
   recycle dead pages. Pin on address-based identity/hash, weakref registration
   or buffer export before exposing the address. Test rollback before commit and a complete
   no-failure relocation commit. The first admitted layouts are cells/tuples;
   expand to measured common containers and completed frames after their layout
   proofs, while unknown layouts retain their stable path.
5. **Minor oracle and major policy.** Compare minor liveness against an
   independent full-trace oracle before enabling reclamation. Missing-barrier
   negatives must expose a lost child. Test zero-promotion old cycles, pinned
   page pressure and resource release. Keep finite major allocation debt and
   explicit full collection. Region capacity stays separate from major budget.

A small native fixture set accompanies this work: old native owner→young child,
raw borrowed local across allocation, native→core container mutation, weakref
resurrection and exported buffer lifetime. Tests check usable data, expected
live sets, bounded memory and eventual resource release; narrowly scoped unit
checks cover relocation/barrier invariants. These fixtures become the first
ports-tree contract examples, then a representative Cython/cffi package exercises
the actual extension boundary. [The nursery review](reviews/gc-nursery-policy-implementation-review.md)
contains file-level interfaces and the stable-owner scanning proof.

## 6. Reference counting and the decision after these steps

RC gave prompt acyclic destruction and reuse, naturally amortized across the
mutator. Our full collector replaces that with repeated heap/death work and
larger bursts of temporary storage. Removing RC also removed ownership facts
used for in-place string concatenation and prompt tuple reuse. Historical
same-fork5b→5c instruction measurements were mixed: Richards 675→520 million,
go 1624→1176M, nbody 1407→1427M, float 949→1022M. These predate current integration
and bundle several changes; they establish neither present RC superiority nor
present parity. The lifecycle review preserves their original context.

In the new upstream coroutine profile, 17.75% of sampled CPU is in non-GC
deallocator subtrees, versus our large bulk collector charge. Comprehensions
show 11.50%, generators 4.48%. Inline reference-count updates remain in the
residual, so these are not complete RC costs. Nursery locality and prompt region
reuse may reduce the fork's larger temporary working set; that is an inference,
not a measured cache-miss result.

**Recommendation: keep tracing and execute A, B and C in that order, with a
performance decision after each.** A already has a substantial measured win
and another specific preparation cost to remove. B tests the storage mechanism
before committing to the moving backend. C implements the established nursery
architecture, with native scanning/pinning as explicit migration costs.

RC is compatible with eight-byte tagged values: inline scalars need no counts,
heap values could carry counts. Restoring it requires count storage plus coherent
ownership/borrowing rules across frames, containers, calls, native APIs and cycles;
it is a real architecture branch, not a switch. Reopen that comparison if the
simplified collector still spends substantial time on unavoidable death work,
or nursery pinning/native scanning dominates representative workloads. Use a
bounded matched experiment then, rather than maintaining two ownership systems
preemptively. After A refresh the current interpreter profile; after the next
structural gain refresh the29-row result and commission design reflection on
remaining call/value/dispatch costs. Roughly 2× remains guidance for simple work,
with clean orthogonal changes deciding what we keep.
