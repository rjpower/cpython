# Fable GC audit: measure actual cheap death before changing storage

2026-11-01. Independent source/artifact audit, 02:03–02:18 UTC grant;
checkpoint 02:13. This reviews the GC recommendations in
[Fable's retained review](../../../build/claude-fable-peer-review-20261101/review-medium-v1.md).
No runtime patch, compiler, target, workload, probe or new timing was run.
I authored the earlier stable-storage design; this audit challenges both that
proposal's sequencing and Fable's replacement. It is not independent acceptance
of my earlier proposal. All held sources/helpers remain held.

## Decision

Accept Fable's demand for **actual allocation, death-state, survival and memory
attribution**. Narrow its first GC mechanism to a read-only census of actual
object-state cheap-death eligibility. Do that before implementing either a
new coroutine allocator or general dead-record omission. Do not make a week
of all-29 profiles, counter access, a threshold sweep or a new backend a
prerequisite for this small question.

Reject B2 as a ready implementation: its type-only flag excludes the named
generator/coroutine traffic, and its page-free step assumes allocator and
lifecycle machinery it proposes to defer. Reject B1's root-skipping timing
control. Accept remembered edges as necessary to avoid old-heap traversal,
but not as proof that adding them now will beat a cheaper death path.

| Recommendation | Disposition | Reason |
| --- | --- | --- |
| A4: actual allocation/death/survival census | Accept, scope first to the nominated workloads and eligibility question | Constructor predictions and inclusive profile shares do not measure cheap eligible bytes |
| B1: read existing root timers | Accept with corrected interpretation | Timers include closure and transitive native-buffer scanning |
| B1: skip data roots to time the saving | Reject | Missing roots can change liveness, work and correctness; no replacement completeness proof exists |
| B2: once-per-type trivial flag reaches common coroutines/generators | Reject | Both types support managed weakrefs and have a finalizer; useful eligibility is state dependent |
| B2: omit dead records and bulk-free pages without storage changes | Reject as specified | Dead graph, rollback, reporting, allocator occupancy and quarantine still need authoritative representation |
| B3: cost real barriers before adopting old-scan avoidance | Accept as a future separate experiment | A dummy card store is not a complete remembered-set cost or mutation-coverage proof |
| B4: dirty-owner/young tracing replaces all-old seeding | Accept as an architectural hypothesis | Requires complete core barriers, opaque-owner handling, young membership, overflow and lifecycle proof |
| B5: allocator integration must wait for both B2 and B4 | Reject that dependency | B2's proposed page-free mechanism itself needs allocator ownership; integration can be tested at unchanged major policy |
| Whole-workload memory versus upstream | Accept | A fork-parent-only guard can preserve a large inherited deficit; report absolute and upstream RSS as well |

## B2 does not identify the proposed hot cohort

[`PyGen_Type`](../../../Objects/genobject.c) and `PyCoro_Type` both declare
`Py_TPFLAGS_MANAGED_WEAKREF` and `gen_finalize`. Their source initializer's
zero `tp_weaklistoffset` does not mean no weakref support:
[`type_ready_managed_offsets`](../../../Objects/typeobject.c) replaces it
with `MANAGED_WEAKREF_OFFSET`, a nonzero marker. `_PyType_SUPPORTS_WEAKREFS`
then returns true. Lists live in the side table described in
[`weakrefobject.c`](../../../Objects/weakrefobject.c), and an actual side-list
entry is recorded by `_Py_HAS_WEAKREFS_FLAG`.

Therefore a predicate requiring no finalizer and no weakref *support*
excludes these types even when a particular completed object has neither
remaining finalization work nor any weak references. The distinction is
not hypothetical: [`_PyGen_FinalizerIsNoop`](../../../Include/internal/pycore_genobject.h)
already checks the finalizer function and finished frame state per object.
The current dead classifier suppresses FINALIZE for that case.

Conversely, deleting those two restrictions from a type flag is unsafe.
Created coroutines may require unawaited warnings; suspended generators can
run `finally` or delegate close; origins, attached frames, exception state,
weakrefs and observers vary per object. `make_gen` even uses `FRAME_CLEARED`
for its partial prefix before `RETURN_GENERATOR` installs a real owned frame.
Completion alone is insufficient.

A valid cheap completed-coro candidate is an exact native coroutine, genuinely
generator-owned, cleared, with no attached frame, exception value or predecessor,
frame predecessor, origin, actual weakref list, pending lifecycle kind, verifier
kill/quarantine or incompatible observer. Keep its original dead kinds exactly
GC-only; do not count a former weakref/finalizer object as originally trivial
merely because preparation cleared its immediate state. Revalidate after the
existing lifecycle preparation. Suspended native-clearable generators are a
different state/path, using `_PyGen_CanClearUnreachable`; do not merge their
preparation cost into the completed-coro category.

For extensible types, a cache computed once also needs a proof that type slots,
watchers and allocator/native obligations cannot invalidate it. Absence of the
particular flags Fable lists is not a general proof that an arbitrary
`tp_dealloc` owns no native resource. A fixed exact-core-type whitelist may
support a structural predicate; it does not prove every instance has cheap
death or justify bypassing unknown extension deallocators.

## Dead-record omission and page-free are larger changes

Current [`live_walk_block`](../../../Python/gc_trace.c) allocates a dead
record before publishing DEAD and stores object, block size and lifecycle
kinds. [`gc_live.c`](../../../Python/gc_live.c) consumes those records for
weakrefs, finalizer/hook selection, SAVEALL/debug reporting, rollback,
accounting, sweep and eventual storage return.

An object with no own finalizer can still connect finalizable objects.
`finalizer_order` explicitly traverses such connectors; `resurrect_visit`
can clear DEAD on an initially cheap object reached from a retained owner.
Skipping its own finalizer dispatch does not make it disappear from that
graph. Even a leaf can be retained by a queued finalizer, and a referent-reading
native deallocator can still require its bytes before the final free stage.

This does not prove dead-record omission impossible. It means a replacement
must still identify every omitted corpse, preserve DEAD/traversal for graph
visits, observe later resurrection, roll back visited prefixes on OOM, produce
statistics/debug output, and enumerate exactly the still-retireable set at
commit. A compact page mask plus exception records is a possible separately
reviewed representation, not an absent cost. A pointer list is also a record
format, even if smaller than `_PyGCDeadObject`.

Mimalloc owns its pages, used count, free/delayed-free lists, heap queues,
abandoned ownership and allocation/debug instrumentation. A page containing
only unmarked *initialized Python objects* is not thereby all reclaimable:
there may be live blocks, unpublished allocations, nontrivial corpses or
quarantined storage. The object-start map says where initialized headers are;
it is not the allocator occupancy bitmap. Pages may mix types sharing a size
class. `mi_heap_visit_blocks` first reconciles delayed frees; its free-map
reconstruction is part of proving allocated-block identity.

[`_PyGC_FreeDeadBlock`](../../../Objects/obmalloc.c) drops any weakref side
entry, clears the object map, counts the free and invokes the object allocator.
KILL quarantine uses `mi_usable_size`, poison/trap state and delayed release.
Skipping those effects or manually declaring a mimalloc page free is unsafe.
A new checked mimalloc page-retire operation could retain allocator ownership;
GC-owned spans are another design. Either needs explicit allocator/quarantine
and failure contracts. Keeping individual `mi_free` calls avoids that change
but no longer implements B2's bulk page-free promise.

The previous span design deliberately retained dead records. Fable is right
that this limits its possible gain and adds a common owner-index cost. It is
not right that its B2 achieves the larger gain with no corresponding ownership
or lifecycle representation work. Neither deserves implementation priority
until actual eligible bytes and their death work are known.

## Root cost: the saved timers already narrow B1

Subtracting the saved after-warmup snapshot from after-hot gives:

| Saved `0b15` workload | Collections | Precise phase / hot | Stack phase / hot | Data phase / hot | Struct phase / hot | All root phases / hot |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Coroutines | 301 | 6.413% | 3.366% | 1.095% | 0.076% | 10.951% |
| Comprehensions | 115 | 4.361% | 1.678% | 0.324% | 0.038% | 6.402% |
| Generators | 59 | 9.920% | 1.106% | 0.368% | 0.025% | 11.420% |
| Richards | 0 | 0 | 0 | 0 | 0 | 0 |

These are inclusive wall-clock phase intervals divided by the saved hot
interval, not a disjoint sampled CPU decomposition or a measurement of today's
head. Precise roots call `live_drain`; stack/data/struct phases drain queued
conservative ranges and native buffers. `live_scan_words` also drains reached
objects. Source at `live_mark_roots` and `scan_queued` therefore rules out
equating these intervals with fixed root discovery alone. The root set that
first reaches an object pays its closure cost, so attribution also depends on
scan order.

The historical data phase is below Fable's 3%-of-elapsed decision threshold
on all three GC workloads, even including its closure. That supports dropping
the proposed B1 implementation from this evidence. It does not prove global
scan cost is zero or permanently small; a current source-matched profile may
change the selection. The approximately 10.7% coroutine sampled roots-plus-
closure bucket cannot establish a 10.7% fixed scan opportunity.

Reject a build that skips data roots as a performance upper-bound control.
It may free reachable objects, reduce the graph and subsequent allocation
work, or appear to succeed only because the kernel did not exercise a C global.
To replace a scan safely, first provide complete audited static-root registration,
all owning modules/late imports and unload/TLS/native-buffer transitions, and
an independent full-root liveness oracle with missing-root negatives. Extension
segments without that proof remain scanned. No such grant or proof exists here.

## Barriers and the actual cost envelope

The all-old minor has age membership and young reclamation semantics, but
does not provide the usual avoidance of old-heap work. It seeds old objects,
drains their graph, then performs another inventory. Calling it a fully useful
generational cost design would be wrong; calling its implemented age semantics
nonexistent would also be wrong. Its default-adoption/cadence commitment is
already reopened and held, not performance accepted.

Only exact cells and nonempty base tuples are currently admitted young.
Generators/coroutines are old from birth. More frequent all-old minors cannot
reclaim this nominated exact-coro traffic as young garbage. They may reclaim
other useful traffic, but their cost campaign is incomplete and supplies no
complete policy win. Source-correct cadence is not a reason to retain it.

Complete barriers and a remembered-set/native-owner boundary are necessary
for the proposed old-work avoidance. Traversal metadata describes edges, not
every mutation site: C API setters, direct native struct stores, constructor
publication, resumed frames, callbacks and opaque owners need explicit rules.
Permanently rescanning opaque old owners is safe only with the complete
transitive core-store contract; it may also consume much of the saving.
Dirty overflow, promotion and card clearing must fail closed and retain major
debt/resource-reclamation bounds.

A barrier-without-consumer experiment measures that exact added store path,
not the whole cost of lookup, conditionals, dirty-set insertion, scanning,
clearing, synchronization and overflow. Nor is it necessarily the lower bound
for a later coalesced representation. Comparing that slowdown with an inclusive
"old tracing" bucket would mix costs: the saved precise phase includes live
closure and root work, not a measured avoidable old-only amount. Before broad
barrier implementation, nominate the real old-owner/young traffic and measure
the always-rescanned opaque share. Do not invent its benefit from tree size.

The saved coroutine collection timer is 3.247/8.393 seconds (38.7%); roots are
inside it and queued delivery can lie outside it. That is not an additional
38.7% on top of the sampled subphases. Death count is not time: half of deaths
being eligible does not imply half of collection time is avoidable. The claim
that deferred deaths are cold tens of milliseconds later is plausible but
unmeasured; average collection spacing and a larger RSS do not prove cache
temperature or its causal cost. Richards has no timed GC in these profiles,
so no GC mechanism explains its interpreter result.

## Selected first hypothesis and smallest next artifact

**Hypothesis:** exact completed native coroutines account for a substantial
fraction of actually reclaimed bytes after lifecycle preparation, and their
ordinary deallocator dispatch is redundant under a bounded object-state
predicate. This is narrower than general trivial-type death or page reclaim.

First propose a small observational debug-source candidate, preserving every
existing record, deallocator, allocator, collection schedule, root and kernel.
This document does not grant it. Counters belong at the existing allocation,
dead classification and sweep stages; no new per-object profiling table or
Python callback is needed.

1. Count exact coroutine and generator constructor calls/requested bytes in
   both constructor routes, alongside existing total requested-byte debt.
   Keep logical requested bytes distinct from actual allocator block bytes.
   This is not a claim to have instrumented every allocation site or produced
   a complete all-type allocation census.
2. At existing dead classification, partition all candidate/death counts and
   bytes by a fixed core-family roster (coro, gen, tuple, cell, list, dict,
   other). Within each family count
   original lifecycle kinds and cheap-state exclusion reason. Keep the actual
   block size already supplied by the allocator; do not infer it from a
   constructor's requested size.
3. At actual ordinary-phase sweep dispatch, after lifecycle preparation and
   referent-reader phase zero, count the same proposed cheap predicate again.
   Record candidate, subsequently retained/resurrected, still-dead eligible,
   ordinary dispatch and finally freed counts/bytes separately. Constructor
   partial prefixes are excluded before inspecting uninitialized frame fields.
   Never call the predicate on another type's layout.
4. Take fixed scalar snapshots outside hot work; separate setup, warmup and
   the unchanged registered hot window. No heap inspection that allocates
   Python lists inside the interval; no clock stubs, source-spelled replacement
   kernel, altered globals or forced per-iteration collections. This is a
   behavioral census, not scored throughput.

Compile the counters out of release builds. Use bounded integer buckets,
not retained `PyObject *` profiling arrays or callback-defined type labels.
Any outlined helper must obey the existing precise-frame/root annotation
contract and pass its linked audit; adding a scalar statistics helper is not
an exemption from that contract. A counter must not introduce a collection,
new root, different callback ordering or publication point.

One finite native-feedback proposal can cover coroutines as primary,
comprehensions/generators as mixed-state controls, and Richards as zero-hot-GC
control, after a separately reviewed source/helper freeze. Preserve the actual
first failure. Debug instrumentation perturbs timing and is not a performance
result; do not use its elapsed values to accept an optimization.

Required controls: a completed actual-coro batch; weakly referenced, origin-
tracked, created, suspended, executing and escaped-frame variants; retention
from a queued finalizer/owner; observer/debug/KILL contexts; both constructors;
and allocation/mark/order failures. Separate scalar counters must partition
actual dead bytes without double counting. A test-source predicate forced false
must zero only eligibility counters while ordinary results and deaths remain
valid. OOM must preserve the old keep-everything/rollback path. Positive
membership must use actual object state and lifetime, not a duplicated Python
model that agrees with the same source error.

If actual eligible bytes are negligible, reject the completed-coro shortcut
and select another measured consumer; do not weaken the predicate. If they
are substantial, the next minimal causal implementation retains the existing
dead records and final individual allocator frees, omits only eligible
`gen_dealloc` dispatch/redundant field clears, and compares an exact shortcut-
disabled source arm. It must preserve notifications/accounting and fail closed
in observer/quarantine modes. This isolates dispatch/teardown benefit before
paying for span ownership or a compact corpse representation. A large eligible
fraction is a selection witness, not performance acceptance.

If native review shows the omitted work is too small, stop there; the census
can instead motivate a separately scoped compact death journal or integrated
storage design. Any later cost acceptance uses whole-workload elapsed and
declared memory/noise rules, not an arbitrary 50%-of-deaths gate or instruction
counter availability. Report peak RSS in bytes, against the matched fork and
against upstream, plus live/committed/slack/retained counts. The existing
2.3–4.5× upstream historical RSS is inherited debt, not a result fixed by a
5%-versus-parent rule.

This settles a first useful GC question without pretending to settle collector
architecture. It preserves Fable's strongest criticism—missing actual cost
attribution—while rejecting its unsafe root ablation and the claim that a
type flag makes allocator/lifecycle integration unnecessary. No previous
candidate is revived, and no new implementation or execution is authorized.
