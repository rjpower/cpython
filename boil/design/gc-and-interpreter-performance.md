# GC and interpreter performance

## TLDR

Keep the register/value interpreter and tracing collector. Measured throughput
is 0.871× upstream geometrically over 29 workloads: about 15% more normalized
throughput reaches aggregate parity, while generator and coroutine gaps are
larger. The landed cleanup improves those shared costs; it does not establish
broad parity or a general 2× result.

Work in two bounded tracks: establish the smallest useful nursery path, and
attribute residual interpreter cost before proposing shared-interface changes.
Then extend coverage, cheap reclamation and old-work reduction as separate useful
increments. Do not require every future contract in the first experiment.

## Current runtime

The collector is full-heap mark/sweep with hybrid roots, actual death-work
selection, guarded native suspended-generator cleanup and direct managed scans.
Supported execution is Linux, one main interpreter, ordinary threads and the GIL.
There is no active nursery or moving collector. Root and native lifetime rules
remain in force; see [the contract](gc-roots-and-lifetimes.md).

The [measurement report](../issues/gc-cleanup-scanning/performance.md) gives
matched inputs, all rows, raw vectors, phases and limits. Five serial samples
are correlated, not a confidence interval. Root/closure, object inventory and
sweep all remain material; sampled inventory is not an object-age histogram.

## Collector sequence

1. [Minimum nursery](../issues/gc-nursery-minimum/status.md): prove a functional
   young-domain collection and survivor path with the smallest coherent source
   change. Conservative scanning of old storage is acceptable for this first
   experiment. Declare its budget and observable decision checks.
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

[Residual attribution](../issues/interpreter-residual-costs/status.md) selects
the [bulk frame-transfer candidate](../issues/frame-transfer-bulk/status.md).
It is a bounded shared-path experiment, not a measured speedup. Existing
frame-window sharing, compiler folds and tagged payloads are already present;
preserve their initialization, root and exception contracts.

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
