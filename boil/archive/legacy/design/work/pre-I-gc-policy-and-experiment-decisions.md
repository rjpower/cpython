# GC cost, policy and experiment closure

Coordinator assessment,2026-10-30, after the qualified GC landing and matched
[29-workload baseline](pre-I-post-GC-timing.md). This separates observed costs,
source-based explanations and proposed changes. It does not reopen GC
correctness qualification or implement a new collector.

The following first cost study remains evidence at its original scope. The
[revised forward plan](gc-architecture-next.md) adds completed paired-four
selective-death timing and13 exact hot-window perf/counter/RSS conditions.
It supersedes the pending-budget recommendation: retain32MiB, qualify the
measured selective-death patch, then distinguish native-only cleanup before
region reclamation and a common movable nursery with stable native space.

## Why the fork is slower

The aggregate is0.8243 times upstream throughput, but the result is uneven:
the float workload is about2.04 times upstream and Richards1.87 times;
coroutines, comprehensions and generators are0.449,0.510 and0.574 times.
Canonical values and object operations are useful, while the allocation and
execution mechanisms exercised by the slow rows need work. These rows do not
by themselves identify a particular instruction, native interface or GC phase.

Upstream reference counting promptly reclaims ordinary acyclic temporaries.
Our replacement defers reclamation to full-heap collections and performs
heap enumeration, tracing, death classification and native deallocation.
That is a real additional bulk-work path, balanced against removing reference
count updates and many numeric allocations. Native object/value conversions,
frame creation/resume and dispatch remain other source-backed opportunities.
An elapsed residual is not a measurement of those individual costs.

The historical54%/47%/31% GC shares predate the final GC composition. Fresh
four-row counter windows on the qualified release now establish:

| Workload | Hot interval | Timed GC / wall | Mark/root closure / wall | Collections |
| --- | ---: | ---: | ---: | ---: |
| coroutines |12.668s|47.1%|8.0%|301|
| comprehensions |24.050s|54.1%|3.7%|115|
| generators |7.066s|29.7%|10.8%|59|
| Richards |4.224s|0%|0%|0|

The [raw counter analysis](../../../build/checkpoint-i-current-cost-attribution/analysis.json)
and [four completed runs](../../../build/checkpoint-i-current-cost-attribution/result.json)
retain the snapshots, source bindings and cleanup checks. All four native
return codes and post-cleanup guards pass, with zero fallback. The unchanged
probe repeats each registered run call50 times for a useful hot interval;
this is not repeated correctness qualification or a replacement29-row score.

Most timed GC work in the first two rows is outside the recorded mark/root
closure. The coroutine window collects48.56 million GC-tracked objects from
56.42 million cumulative candidate visits; comprehensions collect23.76 million
from42.85 million visits. These show substantial reclamation work, not counts
of cumulative allocations or independent objects visited.

One whole-process coroutine CPU profile records2,124 samples with zero lost
events. It includes setup, warmup and shutdown, rather than exactly the hot
window. Inclusive collector share is45.76%; heap walk/classification8.47%
and sweep10.40% are substantial. These inclusive shares overlap and must not
be summed. Source-correlated samples also expose repeated dead-array flag
checks, weakref handling, finished-generator eligibility checks and an
all-dead buffer-retention pass. The finalizer-order function's4.43% inclusive
share is mostly eligibility rejection, not graph ordering.

[Profile analysis](../../../build/checkpoint-i-current-cost-attribution/coroutines-whole-profile/SUMMARY.txt)
and the [33-artifact final index](../../../build/checkpoint-i-current-cost-attribution/final-cost-attribution.json)
retain raw samples, reports and cleanup. Some libc symbols remain unresolved;
sampled source lines are not exact instruction-latency measurements.

The first source-backed optimization candidate is **selective death
processing**: record actual work eligibility once and use stage subsets
instead of repeatedly inspecting unrelated dead objects. Existing weakref
presence flags already allow a quick negative; repeated calls and enumeration
still cost time. Preserve static-type weakrefs, resurrection rechecks,
transitive ordering connectors, readiness retention and fail-closed OOM.
This is a proposal with no measured speedup, not permission to fuse stages
whose ordering is part of Python's lifetime semantics.

The GC duration counter covers marking/root closure, death preparation,
ordering and sweep. It excludes later Python callback delivery and some
other surrounding work. `roots_ns` includes reachable-graph traversal, not
just finding roots, and is already inside GC duration. The counters cannot
separately attribute mark, sweep, native destruction, or Python frame costs.
Endpoint snapshots allocate and are not an atomic cross-counter snapshot.

## Current policy and established practice

| Property | Current fork | Assessment |
| --- | --- | --- |
| Algorithm | Nonmoving full-heap mark/sweep, collection with the GIL held | A defensible simple baseline; nonmoving collection is not intrinsically slow. |
| Trigger | After a collection, new allocation budget is `max(32MiB floor, last live object bytes)` by default | Already a proportional growth policy, not a fixed tiny interval. Its CPU/memory trade-off has not been established by the baseline alone. |
| Pressure accounting | Gross requested object and PyMem bytes, including full realloc requests; frees do not subtract | Allocation traffic and retained memory differ. Charge policy needs evidence before changing it. |
| Live-size accounting | Live object blocks; not all retained PyMem payloads or RSS | A limited proxy for future collection work and actual memory use. |
| Roots | Precise where checked, conservative elsewhere, including C stacks/PyMem/data | Appropriate compatibility scaffolding; precision and scan costs remain work. |
| Reclamation | Ordered native destruction where deallocators read referents; deferred rooted callbacks | Supports the existing native object model, with more work than a collector that can simply recycle all unreachable blocks. |
| Generations | None | Potentially useful for repeated old-live scans; not a substitute for efficient allocation and dead-object processing. |

[BDWGC's allocation policy](https://hboehm.info/gc/gcdescr.html) expands the
heap partly to amortize root scanning when roots are large relative to the
live heap. Its nonmoving design also illustrates that sweep scheduling and
cheap allocation paths matter. The lesson is cost-aware amortization, not a
particular constant copied into our runtime.

[Go's GC guide](https://go.dev/doc/gc-guide#GOGC) describes a nonmoving
mark/sweep collector whose growth goal accounts for both live heap and roots.
It makes the CPU/memory trade-off explicit. Its concurrency and pacing are
not prerequisites for this fork's first performant collector.

[Lua's collector](https://www.lua.org/manual/5.4/manual.html#2.5) offers
incremental and generational modes, with growth controls. Minor collections
avoid repeatedly traversing old objects, but require the mutator/collector
contract that our future-generations design must establish.

Our policy follows the broad live-size growth idea. The weaker parts are
unmeasured cost accounting and full-heap/native-lifetime work, not absence of
a fashionable collector architecture. One cheap diagnostic remains
[32/64MiB budget sensitivity](pre-I-first-slice-decision.md#bounded-next-question-existing-gc-budget),
with RSS, pauses and reclamation reported. Larger budgets amortize repeated
work; they cannot eliminate processing of all dead objects. The new profile
makes selective death processing a stronger first code candidate than a
generational rewrite or a claim that doubling the floor will fix the deficit.

## H1 and H2 in plain language

| Experiment | Meaning | Current state | Recommended closure |
| --- | --- | --- | --- |
| H1: passed-frame tail dispatch | Pass the current interpreter-frame pointer directly between tail-call handlers instead of loading it again through thread state | Real code and matched Clang products exist; no elapsed benefit measured. It does not affect the GCC computed-goto interpreter used for the current score | Archive for the current GCC milestone. If Clang tail dispatch becomes an active target, reopen for one bounded tail-baseline versus H1 comparison and current composition checks before a merge. |
| H2: physical-word root effects | Generate descriptions of temporary-slot reads/writes and decode simple IR words, as a foundation for future liveness | Tested, inactive decoder for simple scalar handlers; complex operations explicitly unsupported. No constructor/root-reader use, active maps or clearing removal | Archive the tested patch, design and evidence, then close the active tree. Bring it back when implementing a usable liveness slice; there is no immediate consumer that justifies merging unused code now. |

H1 does not remove Python frame allocations. H2 does not yet make GC scan
fewer slots. Neither explains away the current aggregate deficit or supplies
an unmeasured speedup. H3, the handler-domain/monitoring/statistics work, is
already landed and needs no further experiment merge.

The source audit found both H1/H2 trees clean, based on5a45, with no textual
composition conflicts againstad69. H1's separate `strrchr` classification fix
can be evaluated independently of the calling convention. H2's two commits
are5df33656912 andb3e6847348d; its only C caller is currently a test adapter.
The parser correction is needed only with that new effect table, not as an
independent fix to primary. Archival closes the experiment while preserving
the tested starting point for an actual consumer.
Neither branch closure requires another GC full-suite campaign.

## Reduce the working set

The [39-tree inventory](worktree-closure-plan.md) distinguishes22 landed
historical trees,8 deliberate negative controls,1 deferred topic,2 dirty
historical trees and6 retained trees including the repository anchor. Most
of this forest is evidence storage, not unfinished implementation.

Keep three operational checkouts: primary, the qualified serving products,
and the upstream comparator. H1/H2 are selected for archival, not runtime
merges for this milestone. Preserve unique commits with durable refs and archive the required
raw receipts before closing historical checkouts. Never merge a negative
fixture or deferred foreign-owner machinery just to empty a worktree list.

Current evidence has absolute paths into the serving and negative4 trees;
those references need archival locators before removal. Dirty review work,
an idle shell and an unknown ambient build need their own ownership handling.
These exceptions should hold only their affected trees. The existing drop
helper force-removes trees and deletes branches, so it is unsuitable for bulk
closure. The inventory records each concrete disposition and dependency.

Future work should have one integration branch and at most the topic trees
needed by active owners. Each experiment ends with a merge, an archived result
or an explicitly deferred patch. Keep original failures and useful raw data,
not every compiler object and cache indefinitely. Retaining history should
not require preserving every old checkout as an active build environment.
