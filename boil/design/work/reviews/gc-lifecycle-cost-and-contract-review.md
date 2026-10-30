# GC lifecycle cost and contract review

Source/design review, 2026-10-30. No runtime change or native execution.

The next useful changes are local: finish selective death classification,
avoid repeated registry lookups within weakref clearing, and separate proven
native-only generator cleanup from Python finalization. The fresh evidence
puts that shared cleanup distinction ahead of exact-layout reclamation. It does not justify a
new cleanup scheduler or restoring reference counting now. It does justify
keeping reference counting as an architectural alternative if these simpler
cuts leave collection dominant.

This review reads primary `4ce8117c29b4`, the qualified serving source, pinned
upstream `321925c92c9e`, and isolated selective-death candidate `10d8773815c`.
It refines [the architecture plan](../gc-architecture-next.md), using the
existing [counter/profile summary](../../../../build/checkpoint-i-current-cost-attribution/coroutines-whole-profile/SUMMARY.txt).
It does not independently requalify the safety changes previously authored
in this lane; their recorded independent reviews remain authoritative.

## What the cost evidence establishes

The four existing hot counter windows put GC at 47.06% of coroutine wall
time, 54.06% of comprehension time, 29.68% of generator time, and zero for
Richards. Root/mark closure is already included in those figures. Coroutine
collections visited 56.42 million candidate records and collected 48.56
million objects; these are cumulative visits/counts, not an age distribution.
The last live-byte gauge cannot describe the live set throughout the run.

The one whole-process coroutine profile contains 2,124 user samples and no
lost samples, including setup and shutdown. Inclusive shares overlap:
`finalizer_order` is 4.43%, with eligibility checks accounting for 4.14%;
`clear_callback_weakrefs` is 4.28%, and `queue_destroy_hooks` is 4.24%.
`_PyWeakref_ListPtr` has 4.66% physical self samples and `gen_dealloc` 3.77%.
Sampled source locations can skid. They do not assign exact instruction
latency, show expensive user callback delivery, or prove that callback
delivery is free when `run_queue_ex` has no resolved sample.

In particular, the finished-generator predicate is rejecting no-op
finalizers. Treating that cost as a reason to weaken callback semantics
would optimize the wrong operation.

## Required behavior versus collector policy

| Boundary | Required behavior or compatibility | Current policy that can be reconsidered |
| --- | --- | --- |
| Weakref callbacks | A surviving weakref is dead before its callback; callbacks for one referent run newest-registration first; callback exceptions are reported rather than propagated. | Process-global registry layout and how callback records are staged. |
| Finalizers and resurrection | Finalizers see valid objects; resurrection must retain reachable storage. Cyclic finalizer order is unspecified. | A global referrer-before-referent finalizer DFS and one interpreter-wide FIFO drainer. |
| Native teardown | A buffered writer must not lose its underlying resource before required flush/release; exported acquisitions must remain valid. | Which exact builtin layouts need a `tp_dealloc` call before collector-owned block recycling. |
| Timing and shutdown | Python does not promise immediate collection or that every `__del__` runs at exit. `weakref.finalize` has its own once-only and reverse-creation exit contract. | Bounded follow-up collection and drain scheduling, subject to the supported resource tests. |

These distinctions follow the [weakref API](https://docs.python.org/3/library/weakref.html),
[C lifecycle contract](https://docs.python.org/3/c-api/lifecycle.html), and
[data model](https://docs.python.org/3/reference/datamodel.html).
Unspecified cyclic order is not permission to break concrete file/buffer
dependencies. The fork's [finalizer ordering](../../../../Python/gc_live.c#L827)
explicitly preserves `TextIOWrapper` before `FileIO`; deleting that order
needs a replacement resource-lifetime proof, not merely a language citation.

Pinned upstream also makes a finer distinction than “clear all weakrefs,
then finalize.” It clears callback-bearing refs before callback delivery,
but deliberately leaves callback-free refs usable through finalization to
preserve subclass cache invalidation and resurrection behavior
([upstream rationale](https://github.com/python/cpython/blob/321925c92c9e67864cafa1566840927f6f07e90a/Python/gc.c#L772)).
The selective candidate's corrected test captures this distinction. Do not
replace it with uniform eager invalidation.

Upstream's cost structure is different: it stages weakref callbacks, calls
eligible finalizers, checks resurrection, then calls `tp_clear` on remaining
cyclic garbage. Reference decrements cause reclamation; finalizers are
visited in list order rather than a global referrer DFS
([upstream cycle completion](https://github.com/python/cpython/blob/321925c92c9e67864cafa1566840927f6f07e90a/Python/gc.c#L1555)).
The fork pays for complete dead classification, explicit dependency
retention, native teardown and a later block-free pass instead. Its
[single drainer](../../../../Python/gc_live.c#L481) protects callback roots
and serializes Python reentry. Simplifying that policy will not eliminate
the sampled eligibility scans; there is no callback-cost evidence here to
justify reopening its safety contract.

## Complexity: repeated linear work, not a demonstrated quadratic collector

Let `D` be dead records, `V/E` reachable dead ordering nodes/edges, `F` the
native-first subset, `H` registry entries, and `L` legacy `tp_del` candidates.

| Mechanism | Source-derived cost |
| --- | --- |
| Dead-stage selection | Several fixed `O(D)` scans. Most current coroutine stage checks reject work. |
| Modern finalizer order | `O(D + V + E)` traversal with a shared visited set; ordinary connector nodes remain necessary. |
| Native-first order | Two selection scans, sorting/searching selected nodes, then DFS: `O(D + F log F + E_F log F)`. |
| Legacy cycle test | A separate reachability test per legacy candidate can cost `O(L(V + E))`; no evidence it dominates this profile. |
| Weakref registry | Expected constant-time chained lookup/removal; occasional `O(H + buckets)` resize. Clearing `W` refs currently repeats head lookups `O(W)` times. |

Anchors are [finalizer order](../../../../Python/gc_live.c#L834),
[legacy cycle handling](../../../../Python/gc_live.c#L759),
[native-first ordering](../../../../Python/gc_live.c#L1189), and
[hashtable lookup/removal/rehash](../../../../Python/hashtable.c#L140).
These algorithms warrant reducing redundant work, not describing every
ordering pass as an all-pairs graph problem.

## Three bounded structural recommendations

### 1. Finish actual-work classification before adding stage vectors

The isolated selective-death slice already replaces type capability with
collection facts: registered weakrefs, unfinished/unfinalized finalizers,
and exact buffer-cleanup categories. This addresses the sampled no-op
eligibility and all-dead buffer-retention scans directly. Keep this as the
first cut and assess its release result before adding another representation.

If residual selection scans matter on mixed heaps, add sparse per-stage
indices into the existing complete dead array. Keep the complete graph for
ordering connectors; retain dynamic DEAD/FINALIZED and acquisition-readiness
checks after resurrection. Any index allocation must share the existing
retain-on-OOM behavior. Do not add permanent object header state for a fact
needed during one collection. Relevant boundaries are
[classification](../../../../Python/gc_trace.c#L2183),
[weakref selection](../../../../Python/gc_live.c#L720), and
[hook/buffer selection](../../../../Python/gc_live.c#L1019).

The meaningful check is mixed ordinary/no-op/real work with resurrection,
callback-free weakrefs, and ordering through an ineligible connector. A
negative that restores broad eligibility can establish extra work, but
cannot alone establish elapsed benefit. The bounded candidate controls are
already recorded; this review requests no repeat campaign.

### 2. Keep the sparse registry; clear a referent's list with one captured head

The fork already uses sparse registrations rather than a weakref word in
every eligible object. Entries are separately allocated because callers may
hold `&entry->value` across allocations. The registered flag can remain set
when the list is empty; it does not mean a live weakref is present
([registry contract](../../../../Objects/weakrefobject.c#L20)).
Also, `_PyWeakref_ListPtr` returns before locking/hashing for an absent flag.
Its sampled self cost cannot all be called hash lookup cost.

There is nevertheless a concrete redundant operation: the GC captures a
head once, then `_PyWeakref_ClearRef` looks it up again for each member;
the no-callback loop does the same
([unlink](../../../../Objects/weakrefobject.c#L173),
[bulk loop](../../../../Objects/weakrefobject.c#L1269)).
A private GC bulk helper can accept the captured stable head, unlink under
the existing lock discipline, and stage surviving callbacks in list order.
Keep entry lifetime unchanged and preserve callback-free clearing as a later
phase. Obtain a callback node before irreversibly clearing that callback's
ref; retain partial-OOM and dead-weakref behavior. This needs no new public
weak handle, moving hash table, allocator whitelist, or unlocked registry.

Separate invalidation from delivery, but preserve Python's two weakref
phases. Java's [weak-reference contract](https://docs.oracle.com/en/java/javase/25/docs/api/java.base/java/lang/ref/WeakReference.html)
also separates clearing from later queue notification; it is a precedent
for the separation, not a replacement for Python's observable rules.
Existing multiple-callback/order, dead-weakref, resurrection and OOM tests
are the acceptance boundary. Actual residual weakref-rich cost should
decide whether further registry changes are worthwhile.

### 3. Prove direct reclamation for one exact layout, then generalize only with evidence

An exact nonempty tuple is a small proof target. Its current
[deallocator](../../../../Objects/tupleobject.c#L240) no longer walks elements:
it untracks and calls `tp_free`. In this collector every GC object remains
traceable; [GC deletion](../../../../Python/gc_live.c#L2712) calls
`PyObject_Free`, which defers the current sweep block's actual free. The
collector later performs the real map/registry/allocator release in
[`_PyGC_FreeDeadBlock`](../../../../Objects/obmalloc.c#L1701).

An exact-type, approved-slot fast path could avoid the indirect deallocator
and deferred-free call while retaining tracer DESTROY, swept marking,
accounting, allocator/debug hooks, object-map removal, and the final block
free. Static empty tuples and subclasses keep their existing handling.
This is a narrow implementation of collector-owned reclamation, not a new
generic cleanup slot. Tuple frequency/elapsed cost is not established by
the coroutine profile; that decides priority before implementation.

Do not start by skipping finished generator deallocation: its finalizer may
be a no-op while frame/exception/name storage still requires cleanup
([generator teardown](../../../../Objects/genobject.c#L202)). Lists still own
separate item storage. Resource-bearing and opaque native types retain
their existing teardown contract. Post-resurrection DEAD checks and the
final free pass are safety boundaries, not redundant loop noise.

## Does restoring reference counting warrant consideration?

Yes as a fallback design choice; no as the next patch. The strongest local
evidence cuts both ways. Historical same-fork step 5b-to-5c instruction
counts improved Richards from 675.1M to 520.0M and go from 1623.8M to
1176.4M, while nbody changed 1407.1M to 1427.2M and float 948.5M to
1022.1M. Lost tuple reuse contributed to the iterator regression; lost
uniqueness made repeated string concatenation quadratic
([historical record](../../../WORKLOG.md#2026-10-26-1645--step-5c-milestone-2-reference-counting-is-gone-rung-2-gate-passes)).
These are old instruction counts with bundled representation changes, not
a controlled current elapsed comparison. The current upstream/fork gap
cannot isolate reference counting either.

An eight-byte NaN-boxed `PyValue` with inline ints/floats is compatible with
reference-counted pointer objects; restoring counts need not lose those
numeric representation gains. The costs are count storage and complete
ownership transitions: borrowing, container/frame copies, call/return paths,
reuse semantics, native port contracts, and cyclic collection. It is not a
collector switch or an inherent value-model incompatibility.
Explicitly restoring selected reuse/uniqueness information may
recover some losses without global counting, but needs its own sound facts.
If selective death processing and a proven reclamation path leave GC
dominant, the next decision needs a same-source/config attribution of
mutator ownership cost, allocation/reuse, collector time, RSS, pauses, and
resource latency. It should compare complete costs, not GC time alone or
the convenience of deterministic destruction. The existing eight historical
ports show a migration was possible; they do not qualify today's ABI.

For now: complete the existing selective slice, take the captured-head
weakref cut only if residual evidence supports it, and use one exact core
layout to test collector-owned reclamation. Preserve resource ordering and
the simple drainer while the profile points chiefly at selection and death
processing rather than callback execution.

## Selective-death elapsed evidence

The subsequent [paired-four diagnostic](../../../../../boil-gc-selective-death/build/selective-death-paired-four/run/score.json)
compares qualified parent `4da`/runtime `dac6`/native `3c6` with new candidate
O3 `10d8773815c`. Both use the declared GCC/O3/frame-pointer/zero-init and
evaluator `-fno-gcse` policy. Candidate O1 focused behavior is bounded
evidence, not candidate broad qualification or landing. The two actual
JSON-tail origin witnesses and final native/outer cleanup passed.

I recomputed all 40 durations from the eight raw `stdout.log` files,
normalized by actual row loops (4/6000/2/2), and checked measurement vectors,
medians, ratios, order, and empty measured stderr. No values were trimmed.
Values below are microseconds per registered loop; spread is
`100 * (maximum - minimum) / median` within each process's five serial calls.

| Row | Order | Parent median [min, max] µs | Candidate median [min, max] µs | Parent/candidate | Spread parent / candidate |
| --- | --- | --- | --- | --- | --- |
| Coroutines | parent, candidate | 54,916.821 [50,768.963, 55,945.993] | 41,964.526 [40,368.291, 42,855.628] | 1.308649 | 9.43% / 5.93% |
| Comprehensions | candidate, parent | 34.178 [32.127, 46.043] | 32.173 [29.711, 38.060] | 1.062317 | 40.72% / 25.95% |
| Generators | parent, candidate | 52,397.708 [51,288.688, 63,801.975] | 48,600.099 [47,065.797, 58,486.014] | 1.078140 | 23.88% / 23.50% |
| Richards | candidate, parent | 30,817.655 [30,675.364, 31,631.004] | 31,283.367 [31,081.286, 31,375.123] | 0.985113 | 3.10% / 0.94% |

Coroutine median elapsed fell 23.59%, with nonoverlapping observed ranges.
This is useful evidence that the local classification cut addresses real
cost. Comprehension/generator medians fell 5.87%/7.25%, but their observed
ranges overlap substantially. Richards was 1.51% slower and also overlaps.
These are five correlated calls within one fresh process per variant/row,
not five independently randomized pairs. Row order alternates; variant
samples are not interleaved. There is no formal confidence interval,
four-row-to-29-row extrapolation, or measured upstream ratio here.

The fresh CPU-phase and budget evidence below further limits the diagnosis.
The elapsed result strengthens the case
for this finite collector simplification before a reference-counting
rewrite; it does not identify callback delivery cost or the next residual
bottleneck by itself.

### A stronger conditional candidate for suspended generator cleanup

If the fresh comprehension profile shows finalizer/retention cost, inspect
the existing clear-only close path before prioritizing tuple reclamation.
`_any_knobby` uses short-circuiting `any` over a generator expression
([workload](../../../../boil/bench/internal/kernels/comprehensions.py#L39));
those generators can remain suspended and genuinely require close, unlike
the already-finished coroutine no-ops removed by the first slice.

[`gen_close`](../../../../Objects/genobject.c#L583) already avoids evaluation
at RESUME with the depth-one exception fact and no unwind monitoring tools.
The compiler supplies the depth bit
([flowgraph](../../../../Python/flowgraph.c#L974));
the monitoring check only reads current local/global tool bits
([ceval helper](../../../../Python/ceval.h#L293)).
`gen_clear_frame` unlinks `previous` before frame ownership transfer; that
precludes the allocating `f_back` branch of `take_ownership`
([frame cleanup](../../../../Objects/genobject.c#L155),
[ownership transfer](../../../../Python/frame.c#L47)).
This makes an exact ordinary suspended-generator subset a plausible native
cleanup operation, not a no-op. Yield-from, locked/executing states, async
hooks, never-awaited coroutine warnings, finally/unwind execution, custom
slots, monitoring and frame storage remain explicit fallback boundaries.

There is an ordering constraint: a referrer finalizer can inspect or
resurrect a still-suspended generator before its scheduled close. Eagerly
clearing that generator changes observation; merely dropping its queue
record and waiting for deallocation leaves a resurrected generator open.
The current source deliberately queues every finalizer before retaining
closures for this reason
([queue-before-retain](../../../../Python/gc_live.c#L950)).
A conservative collection-level fallback when actual Python-capable
lifecycle work is present may be simpler than new dependency metadata.
Registered weakrefs, including callback-free refs accessible through a
different callback, also put a generator in the slow cohort. Generators
reached from real callbacks/finalizers/hooks retain their closure and the
existing order; recheck the close guard when used. Preserve tests of a
finalizer observing/resurrecting its generator, a surviving plain weakref
read by another callback, finally/delegation, monitoring, and escaped frame
ownership. The positive proposal is a shared distinction between native
teardown and Python finalization. No code change or claim of a complete
native-only classification is made here. The fresh profile supports pursuing
this finite source proof before tuple freeing or a broader registry redesign.

## Fresh thirteen-profile attribution

The operator's [frozen 97-reference completion](../../../../build/checkpoint-i-current-cost-attribution/native-comparison-v1/completion.json)
records all thirteen jobs returning zero, unchanged inputs, and final cleanup.
After its quiet-window release, I decoded existing perf files on host CPUs
2/3; no target ran. The [phase JSON](../../../../build/gc-lifecycle-review-analysis/analysis.json)
and [one-off decoder](../../../../build/gc-lifecycle-review-analysis/analyze.py)
retain exact counts, periods, phase rules, physical-leaf DSOs and hot bounds.

Method: filter to the probe PID and inclusive `hot_start_ns..hot_end_ns`,
using perf's monotonic clock and the probe's verified CLOCK_MONOTONIC clock.
For each leaf-to-root callchain, assign the first recognized collector phase
once; nested collector work takes precedence over an enclosing drainer.
Otherwise recognize the drainer/finalizer call path, then a deallocator
subtree outside GC, then residual user CPU. Weight by recorded cpu-clock
period. Compiler-inlined `collect_pass` frames without a narrower resolved
phase remain ambiguous, rather than becoming invented callback or GC cost.
All thirteen streams have zero recorded LOST/LOST_SAMPLES events; all their
sample records fell within the selected PID/hot bounds. No sample has an
entirely unresolved stack. Unknown physical leaves remain included under
their resolved ancestor phase and are counted below, not silently discarded.
An auxiliary `perf report --header-only` read reported an invalid optional
HEADER_EVENT_DESC section. The phase decoder's `perf script` reads emitted
no stderr, and independently counted raw SAMPLE records match every decoded
sample; event/clock selection comes from the frozen literal launch.

| Profile | Samples (unknown physical leaf) | Definite GC core | Drainer subtree | Ambiguous collect_pass | Outside-GC dealloc subtree | Other user CPU |
| --- | --- | --- | --- | --- | --- | --- |
| Fork32 coroutines | 2348 (177) | 48.51% | 0.00% | 1.62% | 0.00% | 49.87% |
| Fork64 coroutines | 2047 (154) | 46.17% | 0.00% | 1.56% | 0.00% | 52.27% |
| Upstream coroutines | 958 (0) | 0.00% | 0.00% | 0.00% | 17.75% | 82.25% |
| Candidate32 coroutines | 1722 (161) | 35.77% | 0.00% | 1.86% | 0.00% | 62.37% |
| Fork64 comprehensions | 2315 (99) | 46.13% | 4.06% | 1.94% | 0.00% | 47.86% |
| Fork32 comprehensions | 2463 (63) | 48.19% | 4.06% | 2.27% | 0.00% | 45.47% |
| Upstream comprehensions | 1104 (20) | 0.00% | 0.00% | 0.00% | 11.50% | 88.50% |
| Fork32 generators | 1089 (25) | 27.46% | 0.00% | 1.19% | 0.00% | 71.35% |
| Fork64 generators | 1073 (28) | 23.11% | 0.00% | 0.93% | 0.00% | 75.96% |
| Upstream generators | 603 (0) | 0.00% | 0.00% | 0.00% | 4.48% | 95.52% |
| Fork64 Richards | 750 (59) | 0.00% | 0.00% | 0.00% | 0.00% | 100.00% |
| Fork32 Richards | 637 (68) | 0.00% | 0.00% | 0.00% | 0.00% | 100.00% |
| Upstream Richards | 1248 (6) | 0.00% | 0.00% | 0.00% | 0.48% | 99.52% |

Each row is disjoint, subject to rounding. These are user-CPU sample shares,
not wall-time phases. A subtree includes its descendants and overhead;
it is not an invocation count or pure callback-body cost. Inlined reference
count operations outside deallocator subtrees remain in upstream's residual;
17.75% is not its total reference-counting cost. Its three allocation-heavy
rows recorded no hot cyclic collections; Richards recorded one tiny
collection but no resolved GC sample. Zero samples do not establish zero
cost. Unknown libc leaves cannot be named as a particular memory operation.

The source timer starts after the start callback and ends after mark,
selection, ordering and sweep, before publication/stop callbacks, debug
flush and drainer. Thus `gc.duration`, enclosing `collect_pass` CPU and
drainer CPU are different boundaries. Snapshot allocations can perturb the
counter endpoints; perf/stat FIFO enable/disable also bracket control work
outside the exact timestamp-filtered CPU sample interval. Both PMU counters
report 100% running time for every row, but they retain that broader hardware
interval. Samples are finite and serial, with scheduler/frequency variation;
there is no formal phase confidence interval or exact wall decomposition.

The decisive new split is within preparation. Fork32 coroutine predicate
rejection is 4.51% of user CPU, versus only 0.21% in actual ordering.
Candidate32 has no resolved predicate/weakref/hook-selection samples; mark
is now 22.71%, sweep/free 8.13%, native teardown 4.94%, and ambiguous
collector/drainer ancestry 1.86%. Absolute mark work did not disappear.
In contrast, Fork32 comprehensions spend 13.07% in retention traversal,
9.05% in finalizer ordering, 2.23% in predicates and 0.28% in other finalizer
preparation, versus 4.06% in the resolved drainer subtree. This supports
avoiding unnecessary native-only finalizer preparation/closure retention,
not weakening Python callback delivery.

### Budget, memory and instruction evidence

These are hot wall seconds, controlled hardware instructions in billions,
hot collection counts/duration, whole-process peak RSS MiB, and lifetime
maximum pause milliseconds. Peak RSS and maximum pause are not hot-window
distributions. Upstream uses its own default policy; this is a mechanism
diagnostic, not a replacement elapsed score or an RC-only comparison.

| Profile | Hot s | Instructions G | Hot collections / GC s | Peak RSS MiB | Lifetime max pause ms |
| --- | --- | --- | --- | --- | --- |
| Fork32 coroutines | 12.294 | 85.275 | 301 / 6.022 | 67.35 | 33.65 |
| Fork64 coroutines | 10.386 | 82.516 | 150 / 4.922 | 102.43 | 42.06 |
| Upstream coroutines | 4.819 | 58.731 | 0 / 0 | 16.17 | not exposed |
| Candidate32 coroutines | 8.729 | 79.338 | 301 / 3.302 | 67.50 | 15.16 |
| Fork64 comprehensions | 11.675 | 83.544 | 57 / 5.593 | 145.77 | 118.00 |
| Fork32 comprehensions | 12.430 | 84.954 | 115 / 6.214 | 84.45 | 70.26 |
| Upstream comprehensions | 5.554 | 58.351 | 0 / 0 | 19.35 | not exposed |
| Fork32 generators | 5.509 | 44.351 | 59 / 1.574 | 74.12 | 48.34 |
| Fork64 generators | 5.489 | 41.257 | 29 / 1.334 | 136.70 | 67.42 |
| Upstream generators | 3.036 | 35.024 | 0 / 0 | 28.24 | not exposed |
| Fork64 Richards | 3.781 | 35.388 | 0 / 0 | 37.45 | 5.36 |
| Fork32 Richards | 3.211 | 35.388 | 0 / 0 | 37.06 | 4.70 |
| Upstream Richards | 6.275 | 53.942 | 1 / 0.000158 | 16.31 | not exposed |

Doubling the floor roughly halves collection frequency and root time, but
reduces instructions only 3.24%/1.66%/6.98% on coroutines/comprehensions/
generators while raising peak RSS 52.09%/72.62%/84.41%. Lifetime pause
maxima rise too. Richards has essentially identical instruction counts and
no hot GC in both fork arms, yet wall time differs 17.76%. Therefore the
15.52%/6.08%/0.36% budget wall reductions cannot all be treated as stable
causal benefits. No default threshold change follows from this one sweep.

Candidate coroutines keep 301 collections and nearly identical RSS while
GC duration falls 6.022 to 3.302 seconds and instructions fall 6.96%.
Together with the independent paired-four result, this supports the
selective-death simplification rather than a memory-for-time workaround.
Upstream still does materially less work on these allocation-heavy rows,
including immediate RC deallocation and its reuse/storage choices; this
does not isolate which representation or ownership policy causes the gap.

The resulting order is: qualify/land selective death processing; prove the
native-only cleanup distinction with the conservative fallbacks above; then
develop a common movable nursery with stable native space and the plan's ownership,
barrier and enumeration proofs. Exact tuple freeing and further weakref
registry changes are subordinate measured opportunities. Reference counting
remains a technically compatible fallback, but the demonstrated local gain
and identified preparation cost favor these finite structural cuts first.

## Review of the revised coordinator plan

The measured architecture rewrite clears independent read-only review: its
paired-four and native13 tables, exclusive phase interpretation, RC comparison
and required-versus-policy ordering distinction match this evidence. Exact
preparation weight is600/2463 samples,24.3605359%; the displayed component
percentages are rounded. The final plan includes complete pin discovery before
relocation and protects SHALLOW construction survivors through age/remembered
publication. No native requalification or runtime change is implied.
