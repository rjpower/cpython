# Cost helpers: host-only preparation review

**ACCEPT the exact held preparation for additive native binding work.** No
blocking helper-design finding. It remains explicitly unready for builds,
workloads, timing or activation; no performance result exists here.

Directory: nursery-adoption worktree `build/nursery-cadence-cost-preparation`.
Ready SHA256 `8f81af1b402e3ed3e4fded79f4a58ef225602a71466aed6b111c32d936ddb8c4`;
plan `9c2b0778f871da4befd5f784620e07c9cb3ed35dfe1313f6fe56ef94c3ea95c7`;
held launch `d8a6d345db0bf64d200470ebbe3c5fe1ef76da995b904431a24f2d26da01a9e5`.
The helper diff is
`9e6a0c5bcc831cfe51fc64297a6823c15ab00ee94f49679f4e90e6a9ae705442`.
Independent receipt
`build/nursery-adoption-review/cadence-cost-helper-preparation-20261031-v1.json`
is `d0964f59c1c67855c6056934afe617123cf8771c4a56c1f58d5b4fba21acd1a9`.

## Timed work and observations

The wrapper sets the 32 MiB policy and enables OFF/N0 before loading the workload,
calling optional setup once and performing one registered-loop warmup. No
forced major or policy reset occurs between warmup and timing. The measured
interval contains only the original `module.run(loops)` between the existing
clock calls. Diagnostic imports, callback/state checks, GC snapshots, RSS and
retention observations are outside it. Native collection and delivery caused
by that call remain included. No Python pause callback or profiler is installed.

The historical schema uses `gc.get_count()[0]`, never the newer state getter.
Adopted state exposes D/P; cadence exposes D/P/M/H/pending. Snapshot begin/end
times and labels honestly describe allocating bracketing observations rather
than exact hot-boundary debt. GC collection/duration deltas can therefore
include observation work. Cumulative duration is not used as a single pause.

Clock-source inspection confirms matching nanosecond units on this Linux
source: gc_live records PyTime_PerfCounterRaw timestamps; that delegates to
PyTime_MonotonicRaw/CLOCK_MONOTONIC; PyTime_t is nanoseconds; the remote reader
exports those timestamps with PyLong_FromInt64. The wrapper also asserts its
actual perf-counter implementation. Bind those source/configuration facts for
all three actual images in the additive native capsule before exposure.

The ring reader preserves raw rows, selects main-interpreter generations 0/2,
reports missing cumulative sequences and limits hot events to timestamps wholly
inside the measured interval. Eleven/three ring capacities can lose events;
missing rows never become inferred pauses or complete coverage. Native
intervals exclude later queue/finalizer delivery. RSS is a lifetime peak read
before post-hot snapshots/retention, and max_pause remains lifetime high-water.
No hot-only or subtractable-maximum claim follows.

Retention uses two preallocated three-slot scalar lists. Each of three later
registered-loop batches is followed by an explicit major, then gen2 live_bytes,
then immediate `len(gc.get_objects())`; the heap list is not kept. All points
and final-minus-first growth remain in output. The fixed 32-object/16KiB bounds
are checked without widening. Mocks verify observation order and that their
temporary list is discarded; actual tracing-runtime footprint/lifetime behavior
still needs the real finite witness and separate correctness prerequisites.

## Process and admission scope

The literal schedule has 120 scored fresh processes, one timed call each,
plus 12 zero-sample origin processes. Every scored process performs three
unscored retention calls/majors, giving 360 of each. Origin processes perform
setup/warmup but no timed or retention call. Both rounds contain exactly five
fresh samples per arm/workload. Keep that fixed order and original loops;
do not substitute serial-in-one-process scores or append samples after results.

The supervisor adaptation retains both machine slots and threads three actual
build locks through separate descriptors 11/12/8. The historical lock survives
adopted acquisition, which survives cadence acquisition. Real lock verification,
quiet CPU 0/sibling 8, load 1.5 and source/product/ELF guards remain required at
activation. The maximum 60-minute window includes admission and cleanup and is
also bounded by the issue deadline. Post-replay, post-inventory and final-hash
expiry checks prevent late cleanup from granting success. Raw child streams,
gate return and failure envelopes remain preserved. No automatic retry exists.

## Evidence and remaining binding review

The reviewer checked ready/plan/helper/receipt hashes and independently replayed
the six wrapper schema/scored combinations and parser/132-row/three-lock mocks
using byte-identical copies in a reviewer-owned directory. Frozen preparation
outputs were not overwritten. These are host mocks, not real processes,
native stats, imports, OS locks or performance. The original host generator
failure remains pinned. inventory.md's statement that driver.load optionally
calls setup is inaccurate: load imports/registers; the wrapper correctly calls
setup once separately. Record that correction additively, preserving the draft.

Both bindings_ready and execution_ready are false and activation is absent.
Actual source/product/configuration/tools/libraries/ELF aliases, correctness
and independent reviews, origin parser closure, exact outer environment and
quiet reservation remain unbound. Review their concrete successor before any
activation. Preserve the denied-counter receipt already selected; no new probe
or counter substitute is needed. Elapsed, memory, pauses and finite retention
inform the declared decision; instruction/text proxies supply no automatic
veto. The three-arm cost comparison does not qualify two alternative sources.
