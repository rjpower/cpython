# GC performance: measured costs and implementation plan

Coordinator decision, 2026-10-30, replacing revision 1 after review 33. The
supported Linux/main-interpreter/GIL collector is landed in `75e8a1e0c01`.
The next change, selective death processing, is implemented and measured in
an isolated branch; **it is not landed**. Its coroutine throughput improves
**30.9%** in the unprofiled paired comparison, with smaller, noisier changes
on the other rows. Final debug stress exposed a reproducible warning-delivery
bug under restricted builtins; selective qualification remains FAIL and its
closure is unrun. We will qualify one reviewed union of selective processing,
the warning repair, native-only generator cleanup and direct managed scanning,
then develop a common copying nursery with
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

### A. Select actual death work and distinguish native-only cleanup

The selective patch has an O3 build, independent source/performance review,
focused 13 behavioral controls and four O1-debug modules passing 599 tests with
46 explicit skips. The topic documentation is now `d3331398c624`; it changes
only the record, with all 588 O3 products/aliases unchanged from measured `10d`.
Its initial resurrection test had the wrong plain-weakref
expectation; the corrected test matches pinned upstream and checks eventual
second death. Keep that original failure in the record. Complete the existing
landing gates and independent landing review on the composed candidate, then
merge once. The three selective full suites and ASan pass, but final debug
stress reports an unraisable import KeyError. A deterministic ordinary-API
probe loses the intended never-awaited warning and reports two import errors:
native warning retrieval inherits the interrupted frame's restricted builtins.
Repair runtime-owned module retrieval rather than suppressing the test.
Keep this failed campaign distinct from the final union's qualification.

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
Then reuse the bounded comprehension/generator comparison and report observed
native-clear execution/CPU cost and the admitted scope. Sampled execution does
not establish an exact admission rate. The distinction must earn its benefit
without widening semantics.

### B. Make common storage reclamation collector-owned

First streamline the proven finished-generator/storage cleanup within existing
mimalloc, using the current profiles to select the work. Define an exact-layout
contract: safe publication, outgoing references, side storage, native cleanup,
manual free/realloc and observer notifications. Exact cells and nonempty tuples
are small proof layouts; they are not claimed to dominate current traffic.
Completed generators require a separate frame/exception/origin cleanup proof.

After native cleanup and direct managed scanning, repeat the bounded profiles.
If individual storage return remains material, first assess batching through
the existing mimalloc owner/page boundary. It already supplies size classes,
page occupancy and reusable blocks; use that knowledge before duplicating it.
Batching still performs required object cleanup and wrapper notifications and
respects remote frees, debug fills and delayed page retirement. Its benefit
must exceed the bitmap/grouping work. The
[storage implementation packet](gc-region-implementation.md#storage-decision-after-managed-scanning)
defines this decision and the compact-metadata requirement for a new backend.

If allocation/locality remains the larger opportunity, implement a
page-segmented bump region behind a space-aware object lookup/enumeration
interface, initially retaining full marking. Reserve start,
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
coverage and memory cost justify it. This backend is scaffolding for the nursery.
Its metadata must be compact: an exact cell is16 bytes, so a24–32-byte record
per object would outweigh its payload before page slack. Prefer page descriptors,
stride knowledge and bitmaps where sufficient; count all metadata in the
comparison. A second allocator must earn its complexity through measured
allocation/reclamation benefit rather than becoming a required detour before
nursery work.

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

## 7. Code design and the interfaces we will improve

Implementation authorization, 2026-10-30: proceed with the framed design and
land reviewed increments. The coordinator owns integration and documentation;
parallel agents qualify the composed GC candidate, independently review evidence,
and examine interpreter design. The completed
[independent code review](reviews/gc-code-organization-review.md) verifies
this source map and the interface direction. Performance is summarized after
the broad improvements land. Existing measurements above remain at their actual
source identities until then.

### Roots today: a small mechanism with a lifetime-only contract

`_PyValueRoot`, declared in `Include/internal/pycore_structs.h`, is a linked
record containing an inline value/object union, a `PyValue *base`, initialized
length `n`, and `next`. A thread's `gc_trace.value_roots` points to its LIFO
chain. `Include/internal/pycore_value.h` supplies the push, append, rebase and
pop helpers. Registration allocates no memory. `_PyGC_TraceVisitThreadState`
in `Python/gc_trace.c` visits the object-valued entries of every active span.

The normal recipe for a temporary C borrow is:

1. Identify which object/value must survive a call that can allocate or run
   Python. Reuse a real frame slot or traced owner when its lifetime already
   covers the call; otherwise push a local span before the call.
2. Append each initialized value, extending `n` only after storing it. Use
   `_PyValueSpan_AppendObject` for a raw `PyObject *`, including a numeric box:
   canonicalizing that box to an inline number would leave its storage unrooted.
3. Keep the span linked through the last use. Pop it in reverse push order on
   every success/error exit. If a backing array reallocates, rebase the span
   before the next collecting call.
4. Mark a function `Py_GC_PRECISE` only when all surviving references are
   anchored. The C root checker runs at build time; the stack walker then skips
   the function's opaque local words. Exercise collection/reentry at the actual
   lifetime boundary, including kill mode when relevant.

This is reasonably clean for nonmoving code, but manual cleanup paths and copied
anchors need care. `Py_GC_PRECISE` means “safe to omit conservative scanning,”
not “all pointer locations are writable by GC.” `_PyValueSpan_ObjectSlot` is an
existing narrow way for an out-parameter to write a raw object pointer directly
into an anchored slot. Prefer that existing facility to adding another root API.

For longer-lived non-object storage, `_PyGCRoot` and `_PyGC_AddRoot/RemoveRoot`
register an array of `PyObject *` in an interpreter's doubly linked list. The
runtime head lock protects insertion/removal and collection; the caller owns
the record and storage lifetime. Public `PyGC_AddRoot/RemoveRoot` wrap a single
actual pointer variable with an allocated registration node. This is distinct
from a thread-local span and does not make arbitrary mutation thread-safe.

A **heap field is an edge, not a new root**. Add it to the owner's traversal
using `Py_VISIT` or `_Py_VISIT_VALUE`, including separately allocated backing
storage. `_PyGC_TraceTraverse` also supplies runtime-owned attribute/type edges.
A persistent interpreter/thread field belongs in its corresponding root visitor;
borrowed caches need an explicit weak/invalidation contract rather than accidental
strong rooting. Tests check usable data while reachable and reclamation after the
last real owner disappears.

### Data structures and who owns them

| Structure | Definition/use today | Consequence for the next work |
| --- | --- | --- |
| `_PyValueRoot` chain | Thread-owned LIFO records over initialized value slots; value helpers register, root visitors read. | Reuse storage and balanced lifetime rules. Add explicit writable-slot visitation for admitted relocation paths; legacy copies/aliases still pin or reload. |
| `_PyGCRoot` list | Interpreter list of registered object-pointer arrays, protected by the runtime head lock. | Visit actual pointer locations where their alias contract permits relocation; retain pinning for legacy consumers. |
| Object-start map | Two-level sparse bitmap in `pycore_gc_trace.h`, set on initialized-object publication and cleared before reuse; one bit per eight address bytes, 32 MiB chunks. | Keep authoritative object identity. Region lookup/enumeration must join this contract rather than introduce a conflicting object registry. |
| `addrset`, `rangelist` | Local open-addressed sets (linear probing, at most half full) and sorted range vectors/binary search in `gc_trace.c`. | Useful scratch/diagnostic structures. The live collector also uses object-header mark/shallow bits; it does not require a hash node per ordinary marked object. |
| `trace_t` | Per-mark context for roots, heap domain, scratch stacks, range maps and verification diagnostics. | Separate root/heap access from verifier-only policy as real nursery consumers appear; keep one mark engine. |
| `_PyGCMarkResult` / `_PyGCDeadObject` | Contiguous dead inventory with sizes/kind bits and aggregate facts, plus live/root statistics and domain pin information. | Preserve connectors and actual-work facts. Reuse the inventory for native cleanup instead of adding another object worklist. |
| `objstack` and ordering scratch | Growable libc arrays in `gc_live.c`; sticky OOM stops traversal and triggers retention/rollback. | Preserve failure semantics. Share a helper only when ownership, failure injection and rollback truly match; no generic container rewrite is required. |
| `live_call` / `call_batch` | Singly linked libc callback nodes; head/tail batches splice without allocation, with queue roots published before delivery. | Keep the simple drainer. Separate native-only cleanup before objects reach this machinery. |
| Weakref registry | Stable separately allocated hash entries; each entry owns the head slot of a per-referent doubly linked weakref list. | Stable head addresses matter across allocation. Keep the sparse representation and improve actual-work selection/bulk clearing where measured. |
| `_PyGCWalkCache` | Per-interpreter open-addressed frame-descriptor cache, invalidated by loader/code-range generations. | Retain opaque-frame fallback and attribution; writable roots are an additional contract, not a reason to delete compatibility scanning. |

Collector scratch uses libc allocation so it neither triggers collection nor
appears as hidden strong references in scanned managed heaps. Each structure
has an explicit owner and teardown point. OOM in discovery/preparation retains
objects; irreversible cleanup begins only after the needed storage is ready.
These are more important shared rules than a single universal vector/hash API.

### Shared concepts to introduce with implementation

**Reference locations and access.** Today `visitproc(PyObject *, void *)` reports
child values. The nursery needs visitors over actual `PyValue *` and
`PyObject **` slots, plus a pin path for readonly/ambiguous references. Implement
that narrow interface when converting the first root/layout consumers. Reuse
existing span/root chains and traced owners. Updating a copied anchor does not
update its C alias; every alias surviving a safepoint must reload/recompute or
pin the object. Discover all pin targets before rewriting any reference.
The existing `Tools/cases_generator/call_effects.py` remains the authority for
allocation versus Python reentry; reuse it in checker changes. Its current
proof does not certify relocation/reload safety.

**Heap-space access.** `live_page_of`, `live_lookup`, block walking and owner
queries currently know mimalloc's segments/tags/debug offsets. Add a small
space-aware lookup/enumeration boundary when admitting the first region layouts.
It must answer initialized start, extent and owning space for exact/interior
addresses and enumerate live allocation records. Preserve the current mimalloc
fast path; region code supplies a second concrete backend, without fabricating
mimalloc page structures or building a registry of unused allocator plugins.

**Construction publication.** A published object start, a safely traversable
object and a movable object are different states. Today initialized-start,
TRACKED and SHALLOW checks encode parts of this distinction. Give allocation,
constructor completion and promotion named entry points with stated invariants
as the region backend needs them. Current object-map extension failure is fatal;
new returnable metadata/evacuation reservation needs an earlier fallible boundary.
A pinned page does not make a partially built object safely old.
Metadata/remembered edges must be committed before reentry.

**Lifecycle facts and effects.** Keep actual weakref/finalizer/hook/buffer facts
in dead records rather than recomputing type capability in every stage. Share
native cleanup eligibility with its existing operation (`gen_close` first).
Separate pure eligibility, fallible preparation and nonfailing cleanup. This
makes the proof local and avoids a second registry of type-specific exceptions.

**Collection transaction.** Keep discovery/retention/preparation distinct from
irreversible frame cleanup, pointer relocation and recycling. Name the commit
boundary in the code at each new consumer. Reuse sticky failure/retention before
commit; require a no-allocation/no-Python/no-detach interval afterward. Callback
publication is already partly irreversible, so its existing partial-delivery
contract must remain explicit rather than be called a universal rollback.

### Code organization and landing sequence

`gc_trace.c` currently combines root enumeration, mimalloc address interpretation,
marking, the historical verifier, live verification/quarantine and diagnostics;
`gc_live.c` combines scheduling, lifecycle graphs, queue delivery and sweep.
The boundaries are understandable, but the names/header introduction still
reflect the original verifier and obscure today's live collector.

We will keep native-only cleanup small in its current files. During region work,
extract heap-space access with its second implementation; during writable-root
work, place root declarations/visitors together and keep platform unwind code in
`gc_walk.c`. Move verifier/debug helpers only when that separation removes real
dependencies from the hot mark path. Update obsolete comments at those boundaries.
This avoids interleaving a large mechanical file split with lifecycle fixes.

The active candidate composes selective death, warning-context repair,
native-only cleanup with obsolete deallocator ceremony removed, and direct
managed slot scanners. Qualify and land that union once, then choose measured
storage/space work → core barriers/stable-owner scans and oracle → copying minors.
Slot/scanner and region interfaces can be developed in parallel once their
field/space contracts are agreed; the region experiment remains conditional
on common-path coverage and allocator-hook costs. Independent review can examine the next slice while
the preceding frozen source runs its gates. Runtime branches merge the landed
integration head before final qualification. PROCESS names the current
coordinator-approved composition; ordinary cross-lane consumption stays
prohibited. Release measurements serialize after the broad runtime union
lands. Its report will include the full interpreter comparison,
allocation-heavy rows, GC CPU, RSS and pauses. Nursery coverage and pinning
belong to the later moving prototype; no nursery is implemented in this union.

## 8. Review35: choose GC structures for this runtime

The resemblance to CPython mixes two different things. The live collector no
longer uses its per-object GC generation links, reference-count copying/subtraction
or cycle-isolation algorithm. Historical subtraction code remains in diagnostics. `_PyValueRoot`, `_PyGCRoot` and `PyGC_AddRoot` are fork
interfaces, absent from the pinned upstream source. The inherited part is mostly
the native object-edge protocol (`tp_traverse`/`visitproc`) and native cleanup.
Those were useful for establishing complete coverage; they are not the intended
universal hot path for a moving collector.

### Frame registers already form the common root set

Ordinary IR operations store directly into the frame's `PyValue` array. They
do not register/unregister each result as a separate root. At collection,
`_PyGC_TraceVisitFrames` walks pushed frames and active generator/coroutine
frames; `_PyFrame_Traverse` and `_PyGC_VisitFrameStack` scan their physical slot
extent and header references. The extent is `co_framesize - FRAME_SPECIALS_SIZE`.
Slots are initialized before use; temporary slots are cleared as inputs die.
Extra C spans protect values that are not safely held there, such as newly
boxed receivers or temporaries across callbacks. One span can cover many values.

We still scan the full slot extent, rather than a final-IR liveness map. Object
mark bits suppress repeated object traversal, not repeated examination of slots.
Some conservative compatibility coverage also overlaps precise frame storage.
A usable physical-liveness map could reduce scanning/retention and clearing
work, but needs complete final-word effects, alternate/exception entries,
monitoring and frame-publication proofs. Archived H2 is only a partial decoder,
not an existing usable map. Keep frame-array roots; pursue a complete map as a
separate interpreter/root improvement when the remaining profile selects it.

### Linked roots: distinguish lifetime from scan density

An intrusive link is not inherently a bad GC structure. Our long-lived list
contains **explicitly registered arrays**, not every heap object or frame slot.
Its embedded nodes support arbitrary lifetimes, stable addresses and constant-time
internal insertion/removal without allocation. The public wrapper allocates a
node and removal by slot address currently searches the list, `O(R)` in registered
records. Scanning is `O(R + S)` for records plus their slots, with pointer chasing
between records. We have not measured its population or isolated its CPU cost.

| Root family | Current fit | Better alternative when needed |
| --- | --- | --- |
| Interpreter registers | Contiguous existing frame arrays; no root-node update on ordinary stores. | Static safepoint liveness maps over these same arrays, rather than another root stack. |
| Temporary C scopes | Allocation-free local span records; LIFO and initialized-length contract. | A per-thread chunked shadow stack with saved-top scope restoration, if span registration/scanning materially costs time. Reserve chunks before exposure and preserve reentry, detach, raw-box and alias rules. |
| Durable external arrays | Intrusive interpreter list; arbitrary removal and caller-owned storage. | Stable records in blocks plus a dense active index/handle for removal and scanning if `R` becomes material. Reserve capacity before publication; a growing vector cannot silently replace an allocation-free registration contract. |

The next measurement should distinguish record count, slot count, registration
frequency and scanning CPU. A large array behind one node is already dense where
its values live; replacing that node with a hash table would add costs without
improving the array scan. Conversely, many one-slot persistent roots can justify
blocks and direct handles. We will change the representation for that workload,
not retain a linked list as a permanent architectural commitment.

The local/durable lifetime distinction is also present in
[V8's embedding handle API](https://v8.dev/docs/embed). LLVM documents a linked
shadow stack as a portable rooting strategy and describes compiler stack maps
as the lower-overhead alternative when the compiler can provide them.
[LLVM GC integration](https://www.llvm.org/docs/GarbageCollection.html#the-shadow-stack-gc)
These are useful tradeoffs, not evidence of our root-registration cost.

### Retain edge knowledge; replace generic callbacks on common layouts

`Py_VISIT` is a small null-check/callback helper, not CPython's cycle algorithm.
Finding strong references remains necessary in any tracing collector. The useful
inherited asset is knowledge of which native fields are strong, weak or owned
elsewhere. The current protocol nevertheless has costs: dispatch through
`tp_traverse` once per object, a callback for each reference, and precise child
visitation that still performs allocator/object-start lookup in `live_visit`.
It also reports values instead of writable locations. Our measurements do not
isolate those costs from marking/classification; a speedup is not established.

The selected code direction is now explicit:

1. **Managed core layouts use direct slot/range scanning.** Start with the
   admitted cell/tuple layouts, then frame/value arrays, lists and shape slots.
   Select a layout once per object; scan contiguous tagged values with inline
   tag checks and the mark operation visible to the compiler. Pointer-free
   payloads need no child scan. Preserve type/header and runtime-owned attribute
   edges alongside payload fields.
2. **Share slot descriptions across marking and relocation.** A compact exact
   layout walker supplies actual value/object locations, with initialization
   bounds and separate weak edges. Instantiate the mark/update loops so that
   introducing a slot visitor does not merely replace one per-edge indirect
   callback with another. Use existing shape/value layout facts, adding only
   the native-layout information they do not contain. Barrier ownership uses
   the same field boundary; it is not inferred from a raw backing-array address.
3. **Pinned native layouts keep the traversal adapter.** Their complete strong
   edges seed minor liveness; readonly targets pin before evacuation. Port a
   measured hot native family to slot/barrier contracts when useful. This makes
   `tp_traverse` the compatibility path rather than the universal managed path.
4. **Separate trusted slots from ambiguous words.** Validated exact managed
   slots can eventually mark through a simpler path; raw/interior C words still
   require safe address-to-object lookup. Make publication and slot validity
   explicit before removing a lookup, with debug/verifier checks covering the
   stronger contract. Weak fields never become strong merely to simplify a loop.

This is an implementation refinement of the approved writable-slot/nursery
milestone. The collection transaction, complete pin discovery and native
resource rules stay intact. Candidate33944 contains reviewed programC4d8e690
and exact decoder/libc effect repairs. Both original linked-audit failures remain
recorded; O1/O3/ASan/tail builds, focused12 and whole10 pass. Broad qualification remains pending;
root-list replacement and a universal layout-description framework are not
prerequisites. The [region/code packet](gc-region-implementation.md) records
where allocator integration costs may favor existing-mimalloc cleanup first.
The [comparison plan](gc-post-cleanup-performance-plan.md) freezes which existing
elapsed/profile/arithmetic protocols we will reuse after landing.

The [precise-edge review](reviews/gc-precise-edge-validation.md) identifies a
smaller possible next cut: trusted managed fields could skip repeated allocator
lookup for already fully marked objects, while first discovery, SHALLOW upgrades,
native inputs and diagnostic modes keep the full validator. MARKED also serves
other lifecycle phases, so any shortcut must belong to the root-mark phase.
This remains a source-reviewed proposal, selected only if post-landing profiles
show material repeated-edge cost; it adds no registry or moving-GC dependency.

### Audit lifetime assumptions at their actual boundaries

The warning failure is a concrete example of useful code carrying an unsuitable
context assumption. Native finalization calls `get_warnings_attr`, which imports
through the current Python frame's builtins even if the warning module is
already loaded. Delayed cleanup can interrupt a generated namedtuple lambda
whose builtins dictionary is legitimately empty. The failed import then repeats
on the fallback warning path, explaining two errors without proving duplicate
finalizer delivery. Runtime-owned diagnostics should resolve current
interpreter modules and use an explicit interpreter import path when loading
is necessary. Dynamic warning hooks and genuine hook errors still matter.

This calls for a local contract repair, not changing the documented ambient
semantics of public `PyImport_Import`. Apply the same review question to native
operations that collection defers: which interpreter state is authoritative,
which context can vary before delivery, and which cleanup genuinely executes
Python? Explicit callbacks keep their ordinary Python call semantics. Edge
coverage, roots and raw-memory lifetime remain separate obligations; this
failure is not evidence that an object disappeared from the live graph.
