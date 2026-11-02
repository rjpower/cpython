# Dictionary feedback and cold/warm reader source grant

Adopt the first source cut in
build/pyvalue-reader-consumer-design-20261102-v1/proposal.md,
SHA256e1742f72c371e013d685ea714d21cff6a93d55ed3d101b7fb8d9be016a98b3c3,
review4b08e2702ebb8ba52db5c475fa59d9e77a58a24516cadb460ba14f81aa207c7d.
A implements/generates; D reviews actual source, emission and error liveness.
Second managed-store/generic-attribute proposal remains design only.

Separate90-minute cut: conservative start 2026-11-02T01:14:50.000Z,
concrete checkpoint 2026-11-02T01:44:50.000Z,
ceiling 2026-11-02T02:44:50.000Z. Record actual first edit.
Prior VM-store source window is closed and accepted, not extended.

A owns the proposal's exact first-cut ledger: three specialize.c functions/
four lookups; generic BINARY_OP and dictionary subscript bodies/macros; generic/
fast globals and four guarded attribute-reader bodies/macros; only the bounded
ceval global/binary/capture/publication helpers; precise generator/analyzer
support, serial products and affected tests. Preserve IDs/external RegIR roles,
formats, O-call/slot/store semantics. No second typed arithmetic engine or
unbounded cross-dispatch owner queue.

C owns only the additive index+owned-value provider and retained owned-object
_PyDict_Subscript adapter/declarations under the explicit same-deadline storage
amendment. Agree/freeze those exact bodies before A's final check; other C
storage can continue. Legacy object-out getter names remain unchanged.

All four feedback outputs are owned and closed after needed metadata copies.
Eligibility must prove native Unicode/split lookup nonescaping before feedback
publication; decline general-key hints. If a path can callback, capture and
revalidate current frame/code/logical PC/epoch/feedback before any cached write.
Inline numeric globals must never enter AsPyObjectBorrow for lazy-import checks.

Actual cold and warmed dictionary reads share canonical ownership, while the
cold generic binary bridge uses existing opaque semantics through independently
owned fallible object views. Capture PCs, input/output scalar locations before
callbacks. Temporary output is owned before original rhs/subscript then lhs/
dictionary retirement; folded locals keep their previous value until assignment.
Preserve actual failure frame-unwind order, callback-current locals, finite
capacity and C-only result cleanup before final forced refresh.

Mandatory emitted-error gate from D: after restoring input homes, output flush
must NOT overwrite a restored home with NULL/stale result when output reuses an
input slot. Branch before publication or mark failed output dead explicitly.
Review input/output aliasing, deeper temporaries, Temp-versus-C result lifetime,
retired primitive result boxes and Temps0 refresh. No callback-capable cleanup
may be followed by decoded-field use without validation/reload.

Cheap gates: concrete owned feedback and actual cold/warm emissions at checkpoint,
exact callback/alias/error/metadata review, affected host checks/serial generation
and at most3 meaningful changed syntax-only TUs30seconds each. Do not duplicate
C provider checks. No objects/link/candidate/runtime/timing. A partial cold bridge
must be reported partial; a warm-only read is not coherent reader acceptance.

Actual first edit01:17:54 UTC. Additive adaptive-prelude amendment adopted
under the unchanged02:44:50 ceiling: proposal67534709 and statistics/guard
addendum4bb5cf62 in build/pyvalue-reader-consumer-design-20261102-v1/.
D's critical review accepts this finite nonescaping design with the stats
condition included. The exact extra specialize.c hunks are fb_save_mono,
fb_begin/fb_begin_binary private arguments/calls, specialize_binary_op,
_Py_Specialize_BinaryOp and binary_op_fail_kind. Necessary private declarations
and affected host source assertions belong to this amendment; no public API,
new representation/runtime mode, deadline extension or general feedback rewrite.

Binary derives value types without scalar boxes. Its private feedback path
skips only historical-row saves that would allocate POLY capacity; retain all
already allocated rows and the current nomination. Other families keep their
existing allocation policy. EXTEND accepts only real object operands and
statically NULL guards, with a host table proof. The Py_STATS classifier must
use values/types without nullable-pointer dereference, descriptor/type-name
lookup or DECREF callbacks. User-GETITEM nomination is deferred here; its
generic canonical operation retains complete Python semantics. Record this
eligibility/history limitation as required future feedback/type-cache work,
not full adaptive-family closure or a performance result. No post-call refresh
may be used to excuse stale writes occurring inside a feedback allocator.

Window closed early after final receipt ce914498 and independent review
c745b682. The final source has44 passing host checks, seven successful
generators and two stable actual syntax checks. Sixteen runtime methods are
AST-only (seven new); five ID/format/target products remain exact. Binary
noallocation nomination/statistics changes are included in this acceptance.
User-GETITEM nomination and feedback-history growth remain later work. The
second attribute/native caller designs do not extend this closed grant.
