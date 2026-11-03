# GC and interpreter performance

The architectural retention recommendation below has been reopened by the
2026-11-01 [integrated reassessment](integrated-runtime-candidate.md). Historical
measurements and accepted correctness scopes remain valid. The particular IR,
all-old nursery and allocator/collector split are performance choices under
review; pending correctness increments are not owed integration.

## TLDR

Keep the register/value interpreter and tracing collector. Measured throughput
is 0.871× upstream geometrically over 29 workloads: about 15% more normalized
throughput reaches aggregate parity, while generator and coroutine gaps are
larger. The landed cleanup improves those shared costs; it does not establish
broad parity or a general 2× result.

The opt-in nonmoving nursery for cells/tuples and one shape admission per
polymorphic attribute operation passed coherent qualification and are integrated
in source. [Post-merge checks](../issues/nursery-shape-landing/status.md) and
independent landing closure passed; neither change establishes a new throughput result.
Bulk frame transfer was rejected after its finite comparison.
Remove the temporary whole-runtime nursery switch in the
[adoption increment](../issues/gc-nursery-adoption/status.md), then extend
coverage, reclamation and old-work reduction in separate measured steps.

## Current runtime

The collector is full-heap mark/sweep with hybrid roots, actual death-work
selection, guarded native suspended-generator cleanup and direct managed scans.
Supported execution is Linux, one main interpreter, ordinary threads and the GIL.
This landed snapshot retains its temporary opt-in control. That is not a
long-term runtime choice: the nursery architecture is kept, and adoption removes
the switch while settling real minor/checker semantics. The nursery uses
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
   major-path compatibility, negative controls and combined qualification passed.
   Its conservative old scan and cell/tuple cohort establish no speedup.
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
The integrated [shape admission](../issues/shape-admission-reuse/status.md)
passed source, behavior, emitted-code review and combined qualification:
four matcher sites become two,
one per existing load/store family. This removes duplicate work without a new
protocol; the sampled duplicate share is only 0.774% of Richards CPU, so it is
not a broad performance explanation or an elapsed speed claim.

Existing frame-window sharing, compiler folds and tagged payloads are already
present; preserve their initialization, root and exception contracts.
The [lowering comparison](../issues/interpreter-lowering-research/results.md)
distinguishes these local optimizations from general SSA and LuaJIT's trace
optimizer. The thirteen-handler
[constant-operand prototype](../issues/interpreter-constant-operands/status.md)
was rejected after both matched elapsed rounds regressed. Its 18.42% evaluator
text growth is diagnostic; the earlier 5% size veto was withdrawn before timing.
The [slot-base prototype](../issues/interpreter-dispatch-state/status.md)
removed its intended baseline return/yield transport, but its fixed elapsed
comparison regressed all four selected workloads in both rounds and was rejected.
The [next candidate design](vm-design-candidate.md) compares existing compiler
and backend choices before one explicit frame-argument change. Literal read
subscripts remain an unassigned design candidate.

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
