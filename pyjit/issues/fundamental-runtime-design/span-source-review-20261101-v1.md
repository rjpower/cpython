# Independent stable-span source review

2026-11-01. **ACCEPT for bounded fresh-image correctness feedback only.**
Reviewed clean commit `7f982b3ecb6ae8e7f1a5c0433daa9acde870b7e4`, parent
`5ece3dc211a3cae1a4aadc8aac41826f9c133e61`, in
`build/worktrees/gc-integrated-spans`. All 13 changed files were read. Author
capsule `build/span-source/capsule.json` has SHA256
`d5f1ce8ffddc598fb5d22622f2936a0c44b1a69a5bef874e60a6a310953aac2b`.
There is no remaining concrete source blocker. This verdict grants neither
execution nor landing, performance, upstream parity or broad qualification.

## Ownership and lifecycle closure

The span index owns actual backing extents independently of publication,
current allocator selection and thread lifetime. Headerless lookup distinguishes
outside storage, invalid/free/padding storage, and allocated raw extents. It
resolves span ownership before mimalloc block geometry. Canonical object offsets
remain separate from debug raw bases; the existing object-start map remains
the publication authority. Unpublished and quarantined slots are unavailable
for reuse without treating their headers as valid objects.

Typed allocation scopes a scalar route beside a genuine heap pointer. The
backend consumes it once; collection saves, clears and restores it across
callbacks. Admission checks the complete normal allocator tuple, or the complete
debug tuple and its backing allocator. Unknown wrappers and failed/oversize
reservations retain ordinary allocation. Existing slots retain their owner-aware
free path through a forwarding wrapper. No permanent collector option was added.

Descriptors retain no thread or heap pointer. Existing NOSCAN heap abandonment
preserves backing after worker exit, and mimalloc supports subsequent backing
free from another attached thread. The GIL serializes the new metadata; this is
not a free-threaded allocator. Closing the collector detaches its state while
keeping descriptors/index ownership for outstanding storage; final slot removal
releases backing and metadata without allocation. Shutdown behavior still needs
native qualification.

Both verifier enumeration and live inventory reuse the existing initialized
object classifier. Shallow scanning is bounded to one raw reservation. Ordinary
dead records, weakrefs, finalization/resurrection, referent-reading deallocators
and deferred final block release remain intact. Slot reuse starts at that final
storage boundary. There is no cheap `gen_dealloc` bypass or bulk lifecycle
replacement in this candidate. Realloc copies the requested raw extent into
ordinary storage; failure retains old ownership. Quarantine tracks a distinct
occupied state and uses owner-aware size, poisoning, OOM retirement and expiry.

Failure cleanup visits owned span reservations even though NOSCAN backing is
excluded from ordinary object enumeration. Fault 5 captures actual marked and
shallow span counts before the injected inventory failure; cleanup records zero
remaining MARKED/SHALLOW/DEAD counts. The unfinished constructor also checks its
own flags before direct storage disposal. This is a real failed-mark witness,
not the older object-map prototype's independent mimalloc block oracle. Native
feedback must establish that the fault is consumed and both positive pre-counts
exist; a missing witness fails the case.

The implementation deliberately indexes an actual, potentially unaligned
64-KiB backing base and uses the existing private NOSCAN heap rather than assuming
64-KiB alignment or adding a heap. Its geometry and separate redzones are
coherent source refinements. Requested allocation debt and scheduling are
unchanged. Legacy class charge, raw request, physical stride, backing request,
metadata and slack remain distinct; none is RSS or OS commit.

## Findings and verification

One deterministic fixture defect was found during review and corrected before
freeze: the metadata-fault search initially selected raw size 8, which admission
rejects before consuming the fault. It now starts at
`ceil(sizeof(PyObject) / 8) - 1`, retaining the fresh-class and exact fault
consumption assertions. Descriptor-derived empty/min/max index rejection was
also reviewed. Neither change weakens a runtime guard.

Independent host verification passed the source-root checker over the six
affected C/header files and the existing 22 root-checker regression methods
(21 ordinary passes, one existing expected failure). The new module and its
three literal children compile; there are 20 actual methods. Debug expects all
20 methods and three explicit child invocations; release has four named debug
skips and one boundary child. Boundary/guard aborts are expected auxiliaries,
not successful ordinary child executions. The frozen commit passes diff checking
and remains clean.

The first reviewer regression extraction omitted `tempfile` from its host
namespace. Its two harness errors remain in
`build/span-source-review-20261101-v1/root-regressions.log`; the corrected log
is separate. No target or compiler ran. Independent binding/result receipt:
`build/span-source-review-20261101-v1/receipt.json`, SHA256
`e81008cfeb9ed7db2b7bef03c94717760944808ef4cbd1668631b0913053200d`.

## Gates after this review

The proposed fresh debug/release builds and focused module are reasonable first
feedback, subject to exact executor/manifest review and coordinator activation.
Require actual default/debug allocator identity, span admission, interior and
invalid-boundary lookup, both constructors, large-frame fallback, forwarding
wrapper free, collection-route reset, origin/escaped-frame behavior,
weakref/resurrection, worker backing abandonment, realloc failure, real failed
prefix cleanup and quarantine/OOM/expiry. Preserve first failures and terminal
cleanup. Source inspection does not earn any of these native witnesses.

Later acceptance still requires supported native-owner/root and shutdown/fork
coverage, stress and sanitizer closure, recovery/death after failed marking,
and measured backing retention. The common-path index probe, descriptor cost
and partly live span slack may erase the allocator saving. Existing per-object
classification, dead records and old/root tracing remain, so their costs cannot
be credited as removed. Actual elapsed and memory against the declared parent
and matched upstream, including all 29 workloads, decide performance. All GC
execution must remain within the user's fixed **15:14:44 UTC** decision deadline.
