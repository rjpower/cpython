# GC and interpreter performance

## TLDR

Keep the register/value interpreter and tracing collector. Measured throughput
is 0.871× upstream geometrically over 29 workloads: about 15% more normalized
throughput reaches aggregate parity, while generator and coroutine gaps are
larger. The landed cleanup improves those shared costs; it does not establish
broad parity or a general 2× result.

Two bounded candidates now await coherent qualification: an opt-in nonmoving
nursery for cells/tuples, and one shape admission per polymorphic attribute
operation. Their behavior checks passed; neither is a runtime landing or a new
throughput result. Bulk frame transfer was rejected after its finite comparison.
Extend nursery coverage, reclamation and old-work reduction in separate steps.

## Current runtime

The collector is full-heap mark/sweep with hybrid roots, actual death-work
selection, guarded native suspended-generator cleanup and direct managed scans.
Supported execution is Linux, one main interpreter, ordinary threads and the GIL.
The default has no nursery or moving collector. The experimental nursery uses
existing storage, scans all old objects and promotes survivors in place; it
establishes a generation boundary before attempting a performance improvement.
Root and native lifetime rules remain in force; see
[the contract](gc-roots-and-lifetimes.md).

The [measurement report](../issues/gc-cleanup-scanning/performance.md) gives
matched inputs, all rows, raw vectors, phases and limits. Five serial samples
are correlated, not a confidence interval. Root/closure, object inventory and
sweep all remain material; sampled inventory is not an object-age histogram.

## Collector sequence

1. [Minimum nursery](../issues/gc-nursery-minimum/status.md): bounded behavior,
   major-path compatibility and negative controls passed independent review.
   Qualify the small opt-in candidate with the interpreter change. Its
   conservative old scan and cell/tuple cohort establish no speedup.
2. [Coverage and reclamation](../issues/gc-nursery-coverage/status.md): extend to
   actual allocation-heavy traffic, including generator/coroutine bodies and
   embedded frames. Make safe dead-storage reclamation cheaper; report retained
   metadata and partial-page slack rather than assuming bulk discard is free.
3. [Reduce old work](../issues/gc-nursery-old-work/status.md): use measured costs
   to refine core barriers, native-owner scanning and age/promotion policy.

Keep one collector and major/oracle path. Preserve publication, map/domain,
weakref, resurrection, buffer, hook and OOM behavior. Lifecycle indexes are not
roots. Public `mi_heap_delete()` does not provide CPython survivor promotion;
the chosen storage boundary must close that ownership problem explicitly.
Stability and age are independent; copying requires writable-alias/pinning
contracts and is not a prerequisite for the first age experiment.

## Interpreter track and parity++

[Residual attribution](../issues/interpreter-residual-costs/status.md) led to
the [bulk frame-transfer experiment](../issues/frame-transfer-bulk/status.md),
rejected after two matched process rounds showed no repeatable benefit.
The next [shape-admission candidate](../issues/shape-admission-reuse/status.md)
passed source, behavior and emitted-code review: four matcher sites become two,
one per existing load/store family. This removes duplicate work without a new
protocol; the sampled duplicate share is only 0.774% of Richards CPU, so it is
not a broad performance explanation or an elapsed speed claim.

Existing frame-window sharing, compiler folds and tagged payloads are already
present; preserve their initialization, root and exception contracts.

Generators measure 0.603× upstream and need about 1.66× more throughput for
parity. Their separate profile spends about 22% in timed GC; removing that timed
work alone illustrates only 1.28×. This is not a transferable forecast: queued
delivery and mutator allocation costs extend beyond that timer. The residual
is not yet proved to be frame, call or native-conversion overhead.

Aim for repeatable broad parity and then a lead across calls, containers and
library boundaries, with lower GC CPU and excess RSS. Invariant arithmetic
diagnostics are useful evidence, not `i += 1` recurrences or a general 2× promise.
Start JIT work when shared contracts and major pre-JIT costs are understood;
whole-suite 2× is not a prerequisite. No measured budget selects a new dispatch
backend or restoration of reference counting now.
