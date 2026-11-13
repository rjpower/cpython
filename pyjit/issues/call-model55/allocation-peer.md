# Allocation census peer review

2026-11-12 source review of [allocation-alternatives](allocation-alternatives.md).
Disposition: GO as a source cost census, not a placement selection or timing
claim. No source/native execution was performed by this reviewer.

The shared geometry matches selected54: two growing fronts, independent saved
top, initialized frontier, callback-free argument overlap and local-only normal
retirement after scratch cleanup. The census correctly distinguishes a zero-copy
argument prefix from clean nonargument locals. Merely putting callee scratch in
another span leaves the remaining contiguous local tail in caller scratch.
Preserving clean-local reuse and zero-copy binding requires argument production
in a staged clean-local reservation, with its nested-call/yield/EX authority, or
another argument/local span; copying remains a separately charged alternative.

The same-chunk extra-reservation equation is correct: with D=P-(B+L), the split
cost is 0 for D<=0, D for 0<D<S, and S for D>=S. It does not predict chunk-miss
frequency or peak allocation. Dual-base addressing/encoding, heap copies and
foreign readers need the stated complete ports rather than fictitious pointer
contiguity.

Local validity remains distinct from mortal ownership and must gate every
potentially unbound read/observer before reading stale bytes. In implementation,
also preserve virtual empty-kwargs boundness/materialization (`pending_kwargs`)
and the separate unbound contents of cell/freevar objects; a bare local-valid
bit is not a replacement for those existing semantic states. These are concrete
consumer obligations, not reasons to force NULL-based boundness forever.

Both static and bitmap candidates must use the same selected placement and
boundness policy and account for cold fills, warm reuse, retirement, packing,
metadata and observer work. The ledger supports those experiments without
selecting one from source inspection alone.
