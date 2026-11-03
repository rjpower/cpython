# Attribute one interpreter boundary

Source review and offline analysis, 2026-10-31. The decision budget is one hour,
with an interim checkpoint by 30 minutes. This issue changes no runtime code.

Use the four frozen `0b15b4bbc9b` profiles to find a shared operation with a
specific source contract and a bounded implementation experiment. Residual CPU
does not mean call overhead: allocation, container work, unclassified native
instructions and interpreter execution all occur outside the collector buckets.
The approximately 2× objective is guidance, not a threshold for this decision.

## Evidence and question

The accepted profiles already separate collector ancestry from residual work.
Reparse their immutable, symbolized sample stacks, using the same PID, integer
monotonic bounds and event-period weights. Within residual samples, distinguish
the physical evaluator from innermost inlined helpers. Inspect the corresponding
qualified O3 image with offline `perf report`/`perf annotate`; these commands read
captures and ELF/debug information, and start no measured interpreter process.

The first question is whether frame creation/transfer performs demonstrable
extra work. Compare coroutine and generator rows with comprehensions and Richards;
do not assign the entire evaluator symbol to calls or dispatch. Existing
argument-window sharing, return-slot publication and RESUME skipping are inputs,
not proposed missing features. Native value interfaces, bool facts and shape-row
reuse remain alternatives only if the captured paths support their cost.

## Decision

Select one child experiment: **full-range frame payload transfer**. The current
`_PyFrame_Copy` interleaves one load, destination store and source clear per slot.
The qualified O3 coroutine image retains that scalar loop. A bulk nonoverlapping
copy followed by source clearing could express the same ownership transfer more
clearly to the compiler. This is a proposal, not a measured improvement.

Do not select locals-only copying or direct allocation of every call into a
generator frame. Generator expressions can execute GET_ITER before
RETURN_GENERATOR, so a live temporary can exist outside locals. Direct allocation
would also expand initialization, observer and error-path changes substantially.
The existing full physical extent is the safe first consumer; no liveness map,
new frame ABI, opcode family, type threshold or allocation registry is needed.

The child must prove all three `_PyFrame_Copy` callers have disjoint source and
destination payloads before changing the shared implementation. If that proof
fails, retain the generic helper and reconsider the experiment rather than adding
an unproved fast path. Keep header fields, all slots, raw roots, source clearing,
frame ownership and return publication identical.

## Scope and ownership

This attribution issue owns only this design, its plan and its results. A child
implementation would own `Include/internal/pycore_interpframe.h` and focused
`Lib/test/test_ir.py` coverage, with source review of the callers in
`Python/bytecodes.c`, `Python/frame.c` and `Objects/genobject.c`. GC runtime files
remain in the young-space owner's lane. No source checkout or native job is
created by this issue.

See [results](results.md) for the actual shares and limits, and [plan](plan.md)
for the finite child proposal and checks.
