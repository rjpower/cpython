# Maintained object-map first feedback: failed build

2026-11-01 06:22 UTC. **STOP: native correctness remains unproved.** Root
confirmed supervisor25200 terminal exit1. Exact source was
`dc2ec2c059152933e8804f5f997e81370883460f`; the accepted plan/held launch remain
unchanged. Configure succeeded, but debug `make all` returned2 at the linked
precise-inline audit; the outer build row returned1. No test row, release
build, failed-prefix witness, workload or timing ran. The debug post-gate
receipt records empty live PIDs before the original 06:58:43.586097 deadline.

The raw failure identifies two closure gaps: precise `live_map_area` inlines
`mi_ctz`, for which the C audit lacks a body, and precise `live_fault_block`
calls the outlined, unannotated `live_walk_block`. The selected GNU `mi_ctz`
implementation is a zero guard and `__builtin_ctzl`/`__builtin_ctzll`, not a
collecting operation. A direct `__builtin_ctzll(bits)` on the already-nonzero
64-bit bitmap, retaining a portable non-GNU fallback, can remove that source
visibility problem without changing a checker or vendor header.

Simply marking `live_walk_block` precise is insufficient: an independent
host check reports two original-type borrows across `_PyObject_IS_GC` and
`live_has_destroy_hook`. `_PyObject_IS_GC` really invokes `tp_is_gc`, whose
documented local contract provides no no-reentry or no-mutation guarantee.
It must keep its conservative effect. The read-only hook predicates do not
justify weakening that call's classification.

The minimal snapshot-preserving candidate is a precise root for the original
`tp` around the existing kind expression, keeping the original weakref/type
snapshot and expression intact. An in-memory version passes the actual root
checker and has no intervening error exits. Its extra root push/append/pop
per dead record is real runtime work for later elapsed measurement. My
earlier reload suggestion also passed the checker, but could change the
pre-call type snapshot; that equivalence claim was withdrawn before any
acceptance. Neither proposal was applied by this reviewer. A frozen successor
still requires exact source review and fresh separately activated feedback;
no exemption, retry or new deadline follows from this report.

Evidence is retained under
`build/maintained-object-map-native-review-20261101-v1`:
`failure-receipt.json` SHA-256
`eaed566fcbba6ffd97e4a62014d22a98265ee6a94b62f736c20dfdb156c8587c`,
plus the actual annotation-only and snapshot-root host checks. The receipt
binds the raw build/gate/failure/cleanup records and frozen product manifest;
it does not claim completed product qualification or native case evidence.
No compiler, target or additional diagnostic ran in this independent review.
