# Exact coroutine storage: one integrated nonmoving slice

2026-11-01. Design preparation only, granted 01:44–02:04 UTC, with a
01:54 checkpoint. First retained evidence edit: 01:47 UTC. No runtime patch,
compiler, target interpreter, workload, probe or timing was run. The current
maintained source reference is `4615e52c9bb9dfc101a633a3b65221903064ed64`;
the evidence records the observed primary HEAD and exact source hashes.
The isolated cadence and object-map candidates remain held. This proposal
does not revive their execution grants or the rejected interpreter candidates.

## Selection and evidence

Select **exact native coroutine objects, including their embedded frame and
owned slots**, for the first collector-owned storage prototype. Keep ordinary
generators, async generators, await wrappers, thread-frame chunks, containers
and native payloads in their existing storage. The alternative considered is
exact four-item tuples, a simpler layout with actual use in comprehensions.

The retained profiles provide more specific selection evidence when combined
with their unchanged kernel source:

| Cohort and saved workload | Source-derived hot construction count | Actual total GC deaths in that hot window | What is established |
| --- | ---: | ---: | --- |
| Exact coroutines in recursive `fibonacci(25)`, 4 loops × 50 repeats | 48,557,000 | 48,557,697 | A very large source-nominated family, consistent with almost all counted deaths; not an observed type census |
| Four-item sort-key tuples in comprehensions, 6,000 loops × 50 repeats | 5,400,000 | 23,889,823 | Real coverage, mixed with generator, list, dictionary and other allocations |

For coroutines, `T(0)=T(1)=1` and `T(n)=1+T(n-1)+T(n-2)` give
`T(25)=242,785` coroutine constructions per loop. Every recursive call creates
a native coroutine; its frame storage is part of the same variable-sized
allocation. For comprehensions, setup creates 24 widgets and the predicate
removes 6; `_add_widgets` constructs 18 sort-key tuples per hot iteration.
Both kernel files match their historical `0b15` versions byte for byte.
No kernel was imported to obtain these counts.

The close coroutine totals are a consistency check, not a claim that exactly
48,557,000 of those collected objects were coroutines: collection boundaries
can include older garbage and leave hot allocations alive. Neither profile
measures per-type block sizes, survival or the fraction satisfying the cheap
death predicate. Those are the precise missing census. Tuples would simplify
the proof but would select a less compelling measured consumer; convenience
alone does not outweigh that difference.

The saved coroutine window contains 301 collections, 10,876,836,400 freed
bytes and 8.393 seconds of hot execution. Exclusive sampled CPU includes
12.138% inventory/dead classification, 9.601% sweep/storage and other sweep
work, and 3.261% native deallocator descendants. One explicit allocator leaf
accounts for 1.147%, not the whole inventory category. These historical
buckets cannot be added up as promised savings. The slice retains roots,
tracing, classification and lifecycle work described below.

Evidence and reproducible host-only arithmetic:
[receipt](../../../build/fundamental-runtime-review-20261101/stable-storage-evidence.json),
[script](../../../build/fundamental-runtime-review-20261101/stable-storage-evidence.py),
[collector assessment](../../../build/fundamental-runtime-review-20261101/gc-allocator.md),
[coroutine source](../../bench/internal/kernels/coroutines.py), and
[tuple consumer](../../bench/internal/kernels/comprehensions.py).
The older [coroutine coverage proposal](../gc-nursery-coverage/design.md)
supplies lifecycle obligations, not a current implementation grant or its
superseded opt-in collection policy.

## Hypothesis and fixed comparison

For this high-volume stable family, one collector-owned size-class span
authority can replace general per-object allocator enumeration and storage
return. Allocation can take a free slot directly; collection can retire
eligible slots by span bitmap words after lifecycle processing. A narrowly
proved completed-coroutine path can omit redundant ordinary deallocator
dispatch. The expected win must exceed descriptor/index lookup, publication,
free-slot selection, retained slack and any extra mutator/root traffic.

Keep the maintained full-major collection policy and requested-byte debt
accounting, roots, closure, callbacks, finalizer ordering and heap budget.
No minor/adoption/cadence change, remembered set, barrier, copying, new value
layout, frame-home change, raised threshold or replacement hot workload is
part of this slice. There is no age assignment or promotion census. Keeping
the comparison to full majors avoids attributing a scheduling change to an
allocator improvement. The current all-old minor design is not a prerequisite
or a retained architectural assumption.

## Storage ownership

Use GC-owned 64 KiB aligned payload spans, with slots grouped by one rounded
raw allocation size. This is a prototype geometry to review and measure, not
a permanent tuning option. Derive the rounded size from the existing selected
allocator's size/alignment policy, including debug prefix, padding and tail;
do not assume a source-level frame-slot count is an allocator block size.
Admit only classes whose raw slot size is at most 4 KiB. Larger coroutines
continue through the existing large-object path and are counted separately.

Mimalloc supplies backing spans, alignment, commit and final backing release
through a dedicated private storage heap associated with the same collector
domain. It does not allocate/free every coroutine slot. A backing span has
an explicit private tag: legacy object enumeration must not mistake its base
for one object, and MEM scanning must not treat its entire payload as a
conservative native buffer. Do not put such spans in the ordinary MEM heap
and then rely on the garbage inside them not to look like roots.

One domain-owned descriptor records span base/extent, raw stride, canonical
object offset, committed slots, owner domain and allocation bitmap. A sparse
address-to-span index is the authoritative owner/geometry lookup for these
spans; there is no manually maintained object list or duplicate per-thread
registry. Descriptors, indexes and temporary masks are allocator metadata,
outside conservative payload scanning. Their existence must not root every
slot. Reserve their storage before publishing a new span or changing ownership.

Allocation caches may point to partially free descriptors, but the domain
registry owns them independently of thread lifetime. The GIL serializes this
first implementation's allocation, publication, free and collection. Thread
detach only relinquishes a cache; it cannot orphan live spans. Domain close
must retain/rehome spans and lifecycle queues under the existing closed-domain
rules until every allocation/quarantine is gone. No cross-domain reuse while
objects remain, and no attempt to restore currently unsupported public
subinterpreter creation in this change.

Partly live spans retain their survivors and reuse holes of the same class;
objects never move. At the end of a completed major, release every completely
empty, unquarantined span and its owner-index entries before freeing its
backing allocation. Retain no empty-span cache in this initial comparison.
The backing allocator may retain physical pages; report that separately.
An abandoned thread is not an empty span, and a span with a raw construction
reservation or death-pending slot is not empty. Oversize and unbound-thread
allocations keep their existing visible storage/ownership rules.

The mutator takes a free bit from a cached descriptor/bitmap word, sets its
allocation bit and advances the cursor; only exhaustion enters the refill
path. Reclaimed holes become free bits at the collection commit. No linked
free-list pointer is stored inside an initialized live object. This is not
presumed cheaper than mimalloc's existing per-thread allocation fast path:
the comparison must charge the new bitmap, owner-index and class-selection
work as well as the eliminated general allocator calls.

## States and publication

Do not collapse these states into one bitmap:

| State | Authority | Rule |
| --- | --- | --- |
| Allocated/reserved | Descriptor allocation bit `A` | Storage cannot be handed out twice, including a raw constructor reservation or quarantine |
| Header initialized | Existing object-start map `I` | `_Py_NewReference` publishes a valid canonical object header; it does not prove all traversed fields are initialized |
| Traversal safe | Existing tracked/partial-construction and frame-owner rules | Preserve current SHALLOW handling and constructor-prefix exclusions; do not introduce a second publication lattice |
| Marked/reachable | Existing header MARKED/SHALLOW flags | Same full trace, precise/native edge policy and rollback semantics |
| Death pending | Existing DEAD state and prepared dead record | Candidate for lifecycle processing, still allocated and addressable; not reusable |
| Swept and retireable | Existing SWEPT/lifecycle result, then a transient span retire mask | An execution result after all required deallocators have read their referents; not an independent liveness authority |
| Quarantined | Existing verifier quarantine ownership, extended to span slots | No object-start publication, but allocation remains unavailable until quarantine release |

Normal reserve is `A=1,I=0`; normal object construction publishes `I=1` at
the existing header point. Constructor field initialization and tracking
remain later, unchanged steps. No new collection point belongs in that
prefix. `make_gen` temporarily sets `FRAME_CLEARED` before `RETURN_GENERATOR`
installs a genuine generator-owned frame; that prefix is not a completed
coroutine. Origin construction may collect after tracking. Both `make_gen`
and `gen_new_with_qualname`/`PyCoro_New` must use the allocation route while
retaining their distinct prefix/failure behavior.

Failed reserve or descriptor growth publishes no partial span/index entry.
A failed constructor releases its exact reservation once through the same
allocator ownership dispatch. Metadata preparation cannot allocate a Python
object or run a collection after partially publishing a slot. Preserve the
existing fatal object-map reservation contract rather than silently claiming
that previously fatal map allocation now returns an ordinary MemoryError.

## Lookup and the native boundary

Decode tagged values with existing rules. Before treating an address as a
mimalloc object block, query the stable-span owner index; validate domain,
committed bounds and overflow-safe slot geometry without reading an object
header. Exact precise lookup requires the canonical object address and `A`
plus `I`. Conservative lookup may accept an interior address under the
current conservative policy, but only inside an allocated, initialized raw
slot, including the padding already covered by that policy. An address in
metadata, an uncommitted slot, the unused span tail or a free slot must not
become an object. Interior native pointers conservatively retain the containing
object as today. No writable pointer update or pin protocol is needed because this
slice never moves an object.

Lookup returns the logical slot extent and owning domain as well as its
canonical object. Reuse that result for shallow scanning and bounds checks:
scanning a whole backing span would retain unrelated dead coroutines. The
existing object map remains the sole header-publication authority; do not
invent a second initialized-object bitmap in the descriptor.

The integration audit must include `live_lookup`, its `live_page_of` and
`live_block_of` callers, shallow-range creation, exact owner checks,
`_PyObject_CheckObjectHeap`, `gc_block_object`, frame/value-span debug bounds,
heap verifier/kill readers, heap/object reporting, accounting and every free,
realloc, usable-size or quarantine operation that assumes a public object
pointer is a mimalloc block. `mi_usable_size`, `_mi_ptr_page` and `mi_free`
cannot receive an interior slot pointer. Oversize ordinary blocks use the
existing path after span lookup rejects them. Custom allocator wrappers must
still observe their existing calls; never bypass a wrapper merely to obtain
the proposed fast path.

This is a real allocator/collector boundary change, not a three-line special
case in `make_gen`. If an affected reader cannot be migrated coherently,
stop the prototype rather than hiding it behind a runtime option.
The new index probe may also add cost to every unrelated precise edge;
Richards and the mixed-allocation controls therefore remain part of the
decision even if this workload's own allocation and reclaim counters improve.

## Allocation, lifecycle and batch reclaim

Preserve `PyObject_Malloc`'s current requested-byte charge and threshold check
before taking a slot, including allocations satisfied from an existing span.
Backing acquisition and metadata are accounted separately and do not charge
the same object debt twice. Reuse the existing allocator entry/wrapper chain:
exact type selection chooses the internal span backend beneath it. Preserve
statistics, debug guards, poison/unpoison, tracing and legitimate allocator
wrappers. Debug/instrumented modes may require the ordinary per-object checks
and notification calls even when release mode can use a batched retirement.
They must exercise the same ownership and storage state machine.

Enumerate allocated slots with the publication map to feed the existing
per-object mark/classification and dead-record transaction. For this first
slice, **retain the dead records and lifecycle graph work**. Removing them
would require a separate compact lifecycle representation and a new rollback
proof; a bitmap cannot make that work disappear. The current source-grounded
targets are general allocator inventory and per-object storage return, with
the narrow completed-coro dispatch below. Report their actual remaining work.

Run existing weakref, resurrection, queued finalizer/hook and referent-reader
ordering before releasing storage. Revalidate cheap eligibility after that
preparation, not at allocation or initial mark. A cheap object must be dead,
unswept, exact `PyCoro_Type`, have original dead kinds exactly `_PyGC_DEAD_GC`,
be genuinely generator-owned with `FRAME_CLEARED`, and have no exception
value/predecessor, frame predecessor, attached escaped frame, origin,
weakref list or pending lifecycle obligation. Preserve exclusions for
SAVEALL, reference tracing, quarantine and other observers that require
individual destruction. A previously escaped but now correctly detached
independent frame is not a reason to discard that frame's storage.

For those objects only, preserve untracking/SWEPT and collected accounting;
omit ordinary `gen_dealloc` dispatch and redundant clearing of already
unobservable code/name/qualname fields. This relies on the fork's nonrecursive
DECREF semantics, and does not bypass execution-time frame clearing. For every
ineligible slot, run the existing coroutine deallocator and all its required
effects. Never-created warnings, suspended `finally`, origins, weakrefs,
resurrection and native-visible state are correctness cases, not benchmark
exceptions.

Do not reuse storage during any deallocator phase: other dead native objects
may still read a dead coroutine. Existing `PyObject_Free` deferral remains in
force. Only at the final free stage, after all referent readers and type
destruction obligations, combine successfully retired slots by descriptor,
clear their object-start bits and allocation bits, and make the resulting
free bits available to allocation. Record counts/bytes before poisoning or
releasing storage. Exceptions retained by lifecycle processing remain
allocated. Whole-span release follows the empty rule above; partial spans
only reuse holes. Complete every dead-record, ordering and diagnostic read
before releasing a containing backing span or its descriptor. No
`mi_free(slot)` call occurs. Collection statistics continue to count objects
and logical slot/block bytes under the existing definitions; backing-span
reserved/committed bytes are separate memory counters, never counted as one
giant collected object or charged again for each slot.

All fallible scratch/order/queue allocation must finish before the nonfailing
retirement commit. On an earlier failure, cancel retire masks and use the
existing DEAD/MARKED/SHALLOW rollback across both storage kinds; no slot or
span is reusable. Debug/KILL quarantine keeps the allocation bit and descriptor
alive until the existing verifier releases it. Large-span arithmetic and
accounting must not inherit the dead record's 32-bit block-size field for
an entire backing span; admitted individual slots fit that field.

## Missing census and finite proof obligations

The family is selected; a winning fraction is not. Before interpreting any
performance result, a bounded separately authorized debug feedback batch must
measure these actual counters on the unchanged registered coroutine kernel:

- Requested allocations/bytes by exact type, constructor and raw size class;
  span admissions versus oversize/unbound or wrapper exclusions; actual
  backing allocations and committed/reserved bytes.
- Initialized/tracked slots, live survivors and dead candidates per major;
  cheap dispatches versus ordinary dispatches with exclusion reasons; actual
  retired slot bytes, quarantine and retained lifecycle bytes. Distinguish
  per-object size-class bytes from backing bytes and logical requested bytes.
- Inventory slot/word visits, legacy allocator callbacks avoided, dead-record
  volume still present, per-object frees avoided and backing releases. A source
  branch existing is not a witness that a workload used it.
- Occupied slots, free reusable holes, raw/unpublished reservations, partial
  spans, empty spans, high-water committed backing, descriptor/index memory
  and slack after each major. Track mixed lifetime/size classes, not only an
  all-dead synthetic batch. Record collection counts and debt to expose any
  accidental policy change.

Do not add per-object production profiling records. Debug counters and
preallocated scalar snapshots outside hot measurement are enough for this
question. If ordinary death dominates, too few bytes are admitted, or a few
survivors hold most spans, reject the selected cheap-death cost hypothesis
rather than expanding its eligibility predicate until it passes.

Correctness feedback must cover both constructors and allocation failures,
collection during origin creation, created/suspended/executing/completed
states, independent escaped frames, native/interior roots, old owners, weakref
callbacks, finalizers/resurrection, debugging/observers, thread detach,
partial/empty spans, oversize coroutines and shutdown. Faults must exercise
real metadata/queue/order failure and prove marks/reservations recover. The
failure cleanup must visit owned descriptors even though their backing blocks
are deliberately absent from legacy object enumeration: the old mimalloc
fallback alone cannot clear their MARKED/SHALLOW/DEAD state. This rollback
must need no new allocation and must cover an actually visited prefix.
Negative source ablations separately disable span admission and cheap dispatch;
the corresponding positive witnesses must fail while ordinary semantics stay
valid. A second collector/allocator mode is not retained in the product.

## Whole-workload and memory decision

After independent source and actual behavior review, freeze one temporary
source comparison: maintained parent versus this single slice, same compiler,
configuration, stdlib, origin proof, full-major budget and original registered
hot loops. No simultaneous nursery or iterator candidate. Coroutines is the
primary workload; comprehensions, generators and Richards are fixed controls
for allocator/lookup and unaffected interpreter costs. Preserve raw vectors,
actual process origins, collection counts, peak RSS and post-major retention.

A later ready plan must pin sample counts and order before scores. Proposed
minimum: two rotating-order rounds of five fresh processes per arm/workload,
80 scored processes total, at most 45 minutes including origins/admission and
cleanup, provided the frozen per-process ceilings fit. No adaptive extra
samples. Denied hardware counters do not veto elapsed measurement. Five
samples are not a universal confidence interval: an accepting round must have
within-arm relative median absolute deviation at most 0.5% in both arms,
and the primary improvement must exceed twice the larger of those relative
spreads. Otherwise it is inconclusive, not a win.

Accept the slice for subsequent qualification only if each adequate round
improves primary elapsed median by at least 1%, every control stays within
1% regression, and candidate peak RSS median is at most 5% above the matched
parent for each workload/round. These are proposed premeasurement decision
rules, subject to independent review before execution. No score has been
viewed for this slice. Record raw spreads; conflicting/noisy rounds reject
the comparison as inconclusive rather than trigger a rerun.

Require separate repeated-batch feedback to return span live/retirement
counts to their source-expected baseline and reach a stable post-major
committed-byte/slack bound, with no growth across fixed-size batches. Peak
RSS includes baseline VM/native costs and cannot be replaced by summing
freed logical bytes. A coroutine-only microbenchmark saving that loses on
whole work or violates the declared memory budget rejects the slice.

The new owner lookup and reader migration are its main engineering cost.
Preserving per-death records deliberately limits the first win; it also
keeps lifecycle correctness review tractable. A negative result is useful:
it can show that allocation/storage alone are too small, or that lookup and
nonmoving slack outweigh them. It does not justify automatic barrier,
generation, compaction, compact-journal or next-family work. Those need a new
concrete hypothesis and grant.
