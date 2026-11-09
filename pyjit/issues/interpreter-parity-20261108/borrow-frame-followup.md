# Borrowed call arguments across frames: source comparison

2026-11-08. Bounded comparison of pinned upstream
`upstream-3a440b6` with the six-commit candidate. The source comparison below
preceded the coordinator's native probes, audited in the addendum. This review
makes no compiler change or performance attribution.

**Finding:** the earlier recommendation 5 is correct only within the current
candidate's owning-home policy. It is too broad as an architectural
comparison. Upstream can put a nonowning `LOAD_FAST_BORROW` result in a call
argument and transfer that stack reference into a child frame while the
caller's local remains its owner. The candidate's surviving warm message load
does `PyValue_DUP` into an owning temporary/window home. This is a legitimate
structural gap to measure and design-review. It does not show that the
candidate's ten message increments are all avoidable, that this difference
dominates the 90 increment/100 decrement aggregate per loop, or that changing
it improves elapsed time.

Upstream `Python/flowgraph.c:optimize_load_fast` rewrites a `LOAD_FAST` to
`LOAD_FAST_BORROW` when its per-block reference tracking finds no consuming
or local-killing conflict; it fixes that decision before later superinstruction
insertion. `Python/bytecodes.c:LOAD_FAST_BORROW` uses
`PyStackRef_Borrow(GETLOCAL(oparg))`, which does not add an object reference.
The specialized `_INIT_CALL_PY_EXACT_ARGS` copies the argument stackrefs into
child locals and marks inputs dead. The general call's
`Python/ceval.c:initialize_locals` likewise assigns positional `args[j]`
directly to `localsplus[j]`, closing them only on failed paths. Neither step
unconditionally turns a borrowed argument into an owned one. The synchronous
caller frame remains present during its child call. If a frame-locals proxy
replaces the original caller local, `Objects/frameobject.c` adds the old object
to `f_overwritten_fast_locals` before closing the slot so existing borrowed
stackrefs remain valid. If a frame is copied to a heap frame,
`_PyFrame_Copy` applies `PyStackRef_MakeHeapSafe` to live slots, promoting
unsafe mortal borrows to owners. These are concrete lifetime mechanisms, not
an assumption that a borrowed pointer remains stable merely because a GIL is
held.

The candidate's `Python/bytecodes.c:LOAD_FAST_BORROW` instead duplicates
`GETLOCAL` for a materialized temporary. Its register compiler already folds
single-reader borrowable local loads, but the retained warmed
`LOAD_FAST_BORROW` message producer preceding each outer `CALL_PY_GENERAL`
has an owning home. Current `PyValue` home invariants reject a no-count tag on
a mortal object; `PyValueBorrow` is a short-lived C view, not a transferable
frame home. The candidate also preserves overwritten local objects in its
frame-locals proxy. Thus the gap concerns its chosen home/ownership contract
and call transfer, not a lack of a way to protect overwritten locals, nor an
inherent impossibility in a 64-bit word representation.

Upstream `Python/gc.c:_PyGC_VisitStackRef` does not subtract a borrowed
stackref as an internal counted edge during cyclic-GC accounting; it still
visits that value during ordinary traversal. The candidate's analogous
`_PyGC_VisitValue` has the same no-count distinction but first rechecks the
current owning-home invariant. A candidate frame view needs both the correct
edge count and an explicit anchor; merely relaxing that assertion would leave
close, copy and native-call paths unsound.

The safe follow-up question is whether one proven local-to-argument-to-child
chain can carry a *represented* borrow and retain an independently rooted
source until the child no longer uses it. It must define which owner anchors
the argument through every fallible call-preparation step; how frame-local
replacement, callbacks, exception unwind and ordered retirement behave; how
GC/frame traversal and escaped generator/coroutine frames acquire ownership;
and how the free-threaded path differs. A fast path cannot merely put a
borrowed pointer in an existing candidate home: its cleanup would treat it as
owned, and the current home checks would be violated. Nor can a borrowed
argument rely on last future read of the caller local when callbacks can
replace that local. A separate focused design and independent review would be
needed before implementation.

For the decision ledger, replace “one necessary message snapshot per call”
with “one owning message snapshot per call under the current candidate
contract; upstream demonstrates a cross-frame borrowed alternative.” Keep
the “no demonstrated avoidable copy chain” conclusion for the *current IR*
and defer a compiler rewrite. To rank this alternative, first obtain
upstream/candidate warmed call-shape evidence and type-specific refcount
deltas, distinguish the message load from kwargs/frame/function activity,
then inspect generated GCC and balanced timings. The present aggregate 90
increments and 100 decrements per loop cannot isolate the message chain.
The current batch's correctness and performance qualification is unchanged.

## Executed warmed-shape addendum

The coordinator later retained four native probes in
E8/warmed-shape-comparison, each exiting zero with CPU affinity 15, JIT off,
and the expected pinned source identity. Both interpreters report the actual
logging `MESSAGE` as a mortal string and the logger as mortal. The reported
message refcount of two was taken outside the workload interval; it is not an
attribution of any hot-path reference operations. Warmed disassembly confirms
`LOAD_FAST_BORROW` at the logging calls in both interpreters and a surviving
candidate register-IR argument load. This closes the source-shape and
immortality questions, but leaves dynamic owner provenance, a per-call
increment/decrement budget, and any elapsed benefit unproved. The peer-reviewed
[design packet](borrowed-frame-views-proposal.md) remains diagnostic only.
